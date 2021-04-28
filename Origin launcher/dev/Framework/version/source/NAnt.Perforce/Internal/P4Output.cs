// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Linq;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Text;
using System.Reflection;
using System.Diagnostics;

namespace NAnt.Perforce
{
	namespace Internal
	{
		internal class P4Output
		{
			private static readonly Regex _PreGroupTaggedRegex = new Regex(@"(?<PRE>(?:(?!\.\.\. ).)+?)?(?<POST>(?:\.\.\. )(?<KEY>(?:[a-zA-Z0-9]+)?) (?<VALUE>[^\r]*)\r?\Z)", RegexOptions.Compiled);
			private static readonly Regex _StrictNewLineTaggedRegex = new Regex(@"\A(?<POST>(?:(\.\.\. )+)(?<KEY>(?:[a-zA-Z0-9]+)?) (?<VALUE>[^\r]*)\r?\Z)", RegexOptions.Compiled);
			// REGEX EXPLANATION //
			// pre:
			//  (?<PRE>(?:(?!\.\.\. ).)+?)?                             optionally match anything at the start (but not empty string) that comes before the first 
			//                                                          "...", group will be unsuccessful if nothing is here

			// strict:
			// \A                                                       match start of string, we don't allow a pre group here

			// both:
			//  (?<POST>(?:(\.\.\. )+)(?<KEY>{0}?) (?<VALUE>[^\r]*)\r?\Z)  matches 1 or more "... " and everything after i.e. anything not in the pre group, we capture this so 
			//                                                          we can add it to captured text for the record, aside from key and value groups also matches
			//                                                          a final carriage return optionally (\r?) and end of line (\Z)
			//  (?<KEY>{0}?)                                            ungreedily matches first pattern after "...", this is the record field key
			//  (?<VALUE>[^\r]*)                                        matches everything after key up until end or carriage return

			private List<string> _UnprocessedOutput = new List<string>();
			private P4Record _FirstRecord = null;
			private P4Record[] _Records = null;

			private readonly bool _MultiRecord = false;
			private readonly bool _SupportPreGroups = false;
			private readonly List<P4Exception> _Exceptions = new List<P4Exception>();
			private readonly List<string> _Errors = new List<string>();
			private readonly List<string> _Messages = new List<string>();
			private readonly string[] _ResponseFileArgs = null;

			internal string[] Errors { get { return _Errors.ToArray(); } }

			internal P4Record FirstRecord
			{
				get
				{
					if (_UnprocessedOutput == null)
					{
						return Records.First();
					}
					ProcessFirstRecord();
					return _FirstRecord;
				}
			}

			internal P4Record[] Records 
			{ 
				get 
				{ 
					ProcessOutput(); 
					return _Records.ToArray(); 
				} 
			}

			internal string[] Messages 
			{ 
				get 
				{  
					ProcessOutput(); 
					return _Messages.ToArray(); 
				} 
			}

			internal string Command { get; } = null;

			internal string Input { get; } = null;

			internal string[] ResponseFileArgs
			{
				get
				{
					return _ResponseFileArgs;
				}
			}

			internal string[] CommandHistory { get; } = null;

			internal P4Output(string cmdLine, string[] commandHistory, string input, bool multiRecord, bool allowPreGroups, string[] responseFileArgs)
			{
				Command = cmdLine;
				CommandHistory = commandHistory;
				Input = input;
				_MultiRecord = multiRecord;
				_SupportPreGroups = allowPreGroups;
				_ResponseFileArgs = responseFileArgs ?? new string[] { };    
			}

			internal void AddOutput(string input)
			{
				// If anything tries to add output after the output has been processed that additional output will be ignored
				// There are cases where P4Exceptions get thrown and process the output while output is still coming in, in these cases we just want to ignore any additional output.
				if (_UnprocessedOutput != null)
				{
					_UnprocessedOutput.Add(input);
				}
			}

			internal void AddError(string error)
			{
				if (!String.IsNullOrWhiteSpace(error))
				{
					bool found = false;

					foreach (Regex regex in P4Exception.ExceptionRegexs.Keys)
					{
						if (regex.IsMatch(error))
						{
							ConstructorInfo cInfo = P4Exception.ExceptionRegexs[regex].GetConstructor
							(
								BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public,
								null,
								new Type[] { typeof(string), typeof(P4Output) },
								new ParameterModifier[] { new ParameterModifier(1), new ParameterModifier(1) }
							);
							_Exceptions.Add((P4Exception)cInfo.Invoke(new object[] {error, this}));
							found = true;
						}
					}
					if (!found)
					{
						_Errors.Add(error);
					}
				}
			}

			internal void ThrowAnyExceptions()
			{;
				if (_Errors.Any())
				{
					_Exceptions.Add(new RunException(String.Join("\n", _Errors), this));
				}
				else
				{
					// encoding problems never present as errors, but will always be first output so we only check first unprocessed
					// output line (these regex could potentially match CL description or file contetns but never on first line)
					string firstOutput = null;
					if (_UnprocessedOutput != null)
					{
						// check if output is unprocessed - it's more efficient to just read the unprocessed output
						if (_UnprocessedOutput.Any())
						{
							firstOutput = _UnprocessedOutput.First();
						}
					}
					else if (Messages.Any())
					{
						firstOutput = Messages.First();
					}
					if (firstOutput != null) // soometimes when running a multi-record command with no matching records we'll get a single null here
					{
						foreach (Regex regex in P4Exception.EncodingErrorRegexes.Keys)
						{
							if (regex.IsMatch(firstOutput))
							{
								ConstructorInfo cInfo = P4Exception.EncodingErrorRegexes[regex].GetConstructor
								(
									BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public,
									null,
									new Type[] { typeof(string), typeof(P4Output) },
									new ParameterModifier[] { new ParameterModifier(1), new ParameterModifier(1) }
								);
								_Exceptions.Add((P4Exception)cInfo.Invoke(new object[] { String.Join("\n", _UnprocessedOutput), this }));
							}
						}
					}
				}

				if (_Exceptions.Count == 1)
				{
					throw _Exceptions.First();
				}
				else if (_Exceptions.Count > 1)
				{
					throw new P4AggregateException(_Exceptions.ToArray());
				}
			}

			private void ProcessFirstRecord()
			{
				if (_FirstRecord == null)
				{
					_FirstRecord = ProcessRecords(firstOnly: true).First();
				}
			}

			private void ProcessOutput()
			{
				if (_UnprocessedOutput != null)
				{
					_Records = ProcessRecords();

					// try and claim back some memory
					_UnprocessedOutput.Clear();
					_UnprocessedOutput = null;
					_FirstRecord = null;
				}
			}

			private P4Record[] ProcessRecords(bool firstOnly = false)
			{
				bool endofRecord = false;
				Dictionary<string, string> taggedOutput = new Dictionary<string, string>();
				List<P4Record> records = new List<P4Record>();
				string lastKey = null;
				string lastMessage = null;
				StringBuilder allText = new StringBuilder();

				// The CreateP4Process has output handler that would call P4Output.AddOutput() which will add
				// an entry to _UnprocessedOutput.  However, at the same time, if a P4Exception is thrown, the
				// P4Exception constructor will trigger executing of ProcessRecords() which will iterate through 
				// all _UnprocessedOutput.  So to avoid instances when _UnprocessedOutput being updated when 
				// this object is being iterated on, we extract a copy first before iterating on it.
				List<string> unprocessedOutputCopy = new List<string>();
				lock (_UnprocessedOutput)
				{
					unprocessedOutputCopy = _UnprocessedOutput.ToList();
				}
				foreach (string output in unprocessedOutputCopy)
				{
					if (!String.IsNullOrWhiteSpace(output))
					{
						Match match = (_SupportPreGroups ? _PreGroupTaggedRegex : _StrictNewLineTaggedRegex).Match(output);
						if (match.Success)
						{
							// when using p4 print we can catch the end of a file text on the start of our next record line
							if (_MultiRecord)
							{
								if (_SupportPreGroups && match.Groups["PRE"].Success)
								{
									endofRecord = true; // we must be at the end of a print record if we have content before the ztag
									allText.Append(match.Groups["PRE"].Value + "\n"); // add pre before we finalize this record
								}
								if (endofRecord)
								{
									endofRecord = false;
									lastKey = null;
									if (taggedOutput.Count > 0)
									{
										records.Add(new P4Record(taggedOutput, allText.ToString()));
										if (firstOnly)
										{
											return records.ToArray();
										}
										taggedOutput.Clear();
										allText.Clear();
									}
								}
								allText.Append(match.Groups["POST"].Value + "\n");
							}
							else
							{
								allText.Append((output ?? String.Empty) + "\n");
							}
							lastKey = match.Groups["KEY"].Value.ToLowerInvariant();
							// we should never find a key twice in a single record, if you hit this assert check that your command has been added to MultiRecordCommands in P4Caller.cs                        
							Debug.Assert(!taggedOutput.ContainsKey(lastKey));
							taggedOutput[lastKey] = match.Groups["VALUE"].Value;
						}
						else if (lastKey != null)
						{
							// if we were holding onto a last message add it to the last record
							if (lastMessage != "\r")
							{
								taggedOutput[lastKey] += lastMessage;
								taggedOutput[lastKey] += "\n";
								taggedOutput[lastKey] += output;
							}
							allText.Append((output ?? String.Empty) + "\n");
						}
						else
						{
							allText.Append((output ?? String.Empty) + "\n");
							_Messages.Add(output);
						}
						lastMessage = null;
					}
					else
					{
						allText.Append((output ?? String.Empty) + "\n");

						// note that we set _EndOfRecord here but we don't know *for sure* that the record is over, we could be a in a print or a changelist description with empty lines in
						// so we don't process it for now in case we're wrong, and cache the last message in case its actually part of the record. We delay processing until we hit a new ztag
						// and then we assume we're starting a new record, this works only because we rely on the assumption that the last key in a record can have multiline information
						endofRecord = true;

						// if we already have a last message it means it was actually part of the previous record
						if (lastMessage != null)
						{
							taggedOutput[lastKey] += lastMessage;

							// if we get a null we've reached the end of output and don't need a newline
							if (output != null)
							{
								taggedOutput[lastKey] += "\n";
							}
						}
						lastMessage = output;
					}
				}

				if (taggedOutput.Count > 0)
				{
					records.Add(new P4Record(taggedOutput, allText.ToString()));
					taggedOutput.Clear();
				}

				return records.ToArray();
			}
		}
	}
}
