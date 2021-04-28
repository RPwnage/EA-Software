// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// File Maintainers:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;

using NAnt.Core.Logging;
using NAnt.Core.Attributes;
using NAnt.Core.Reflection;

namespace NAnt.Core.Tasks
{

	/// <summary>Optionset is a dictionary containing [name, value] pairs called options.</summary>
	/// <include file="Examples/OptionSet/OptionSet.remarks" path="remarks"/>
	/// <include file="Examples/OptionSet/Simple.example" path="example"/>
	[TaskName("optionset")]
	public class OptionSetTask : Task
	{
		private OptionSet _optionSet = null;
		private OptionSet _previousOptionSet = null;
		private bool _needAdd = true;

		/// <summary>The name of the option set.</summary>        
		[TaskAttribute("name", Required = true)]
		public string OptionSetName { get; set; } = null;

		/// <summary>If append is true, the options specified by
		/// this option set task are added to the options contained in the
		/// named option set.  Options that already exist are replaced.
		/// If append is false, the named option set contains the options 
		/// specified by this option set task.
		/// Default is "true".</summary>
		[TaskAttribute("append", Required = false)]
		public bool Append { get; set; } = true;

		/// <summary>The name of a file set to include.</summary>
		[TaskAttribute("fromoptionset", Required = false)]
		public string FromOptionSetName { get; set; } = null;

		/// <summary>Optimization. Directly initialize</summary>
		[XmlElement("option", "NAnt.Core.OptionSet+Option", AllowMultiple = true)]
		public override void Initialize(XmlNode elementNode)
		{
			if (Project == null)
			{
				throw new InvalidOperationException("Element has invalid (null) Project property.");
			}

			// Save position in build file for reporting useful error messages.
			if (!String.IsNullOrEmpty(elementNode.BaseURI))
			{
				try
				{
					Location = Location.GetLocationFromNode(elementNode);
				}
				catch (ArgumentException ae)
				{
					Log.Warning.WriteLine("Can't find node '{0}' location, file: '{1}'{2}{3}", elementNode.Name, elementNode.BaseURI, Environment.NewLine, ae.ToString());
				}

			}
			string _appendVal = null;
			try
			{
				foreach (XmlAttribute attr in elementNode.Attributes)
				{
					switch (attr.Name)
					{
						case "name":
							OptionSetName = attr.Value;
							break;
						case "fromoptionset":
							FromOptionSetName = attr.Value;
							break;
						case "append":
							_appendVal = attr.Value;
							break;
						default:
							if (!OptimizedTaskElementInit(attr.Name, attr.Value))
							{
								string msg = String.Format("Unknown attribute '{0}'='{1}' in OptionSet task", attr.Name, attr.Value);
								throw new BuildException(msg, Location);
							}
							break;
					}
				}

				if (OptionSetName == null)
				{
					throw new BuildException("Missing required <optionset> attribute 'name'", Location.GetLocationFromNode(elementNode));
				}

				if (IfDefined && !UnlessDefined)
				{
					OptionSetName = Project.ExpandProperties(OptionSetName);
					if (FromOptionSetName != null) FromOptionSetName = Project.ExpandProperties(FromOptionSetName);
					if (_appendVal != null) Append = ElementInitHelper.InitBoolAttribute(this, "append", _appendVal);

					// Just defer for now so that everything just works
					InitializeTask(elementNode);
				}
			}
			catch (Exception ex)
			{
				throw new ContextCarryingException(ex, Name, Location);
			}
		}

		protected override void InitializeTask(XmlNode elementNode)
		{
			// before doing anything else make a copy of any if previous optionset with this name if this is a traced optionset
			if (OptionSetDictionary.IsTraceOptionSet(OptionSetName) && Project.NamedOptionSets.TryGetValue(OptionSetName, out OptionSet previousOptionSet))
			{
				_previousOptionSet = new OptionSet(previousOptionSet);
			}

			// Since each optionset task starts with an empty optionset, so before initializing this optionset, 
			// we need to populate it with its existing options if this optionset already exists.
			// This makes sure that the ${option.value} property always works.
			if (!(Append == true && Project.NamedOptionSets.TryGetValue(OptionSetName, out _optionSet)))
			{
				_optionSet = new OptionSet();
				_elementName = Name;
				_optionSet.Location = Location;
				_optionSet.FromOptionSetName = FromOptionSetName;
			}
			else
			{
                _needAdd = false;
            }
			_optionSet.Project = Project;
			_optionSet.SetOptions(elementNode, FromOptionSetName);
		}

		protected override void ExecuteTask()
		{
            if (OptionSetDictionary.IsTraceOptionSet(OptionSetName))
            {
				string PrettifyOption(string optionValue, string linePrefix = "")
				{
					IEnumerable<string> prettyLines = optionValue
						.Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries)
						.Select(line => line.Trim())
						.Where(line => line != String.Empty);

					if (prettyLines.Count() == 1)
					{
						return prettyLines.First();
					}

					return String.Join
					(
						String.Empty,
						prettyLines.Select(line => $"\n{linePrefix}{line}")
					);
				}

				List<KeyValuePair<string, string>> additions = new List<KeyValuePair<string, string>>();
				List<Tuple<string, string, string>> edits = new List<Tuple<string, string, string>>();
				List<KeyValuePair<string, string>> removals = new List<KeyValuePair<string, string>>();

				if (_previousOptionSet != null)
				{
					foreach (KeyValuePair<string, string> option in _optionSet.Options)
					{
						if (_previousOptionSet.Options.TryGetValue(option.Key, out string previousValue))
						{
							if (option.Value != previousValue)
							{
								edits.Add(Tuple.Create(option.Key, previousValue, option.Value));
							}
						}
						else
						{
							additions.Add(option);
						}
					}
					foreach (KeyValuePair<string, string> option in _previousOptionSet.Options)
					{
						if (!_optionSet.Options.ContainsKey(option.Key))
						{
							removals.Add(option);
						}
					}

					if (additions.Any() || edits.Any() || removals.Any())
					{
						Project.Log.Minimal.WriteLine
						(
							$"TRACE modified optionset {OptionSetName} at {Location}" +
							(
								additions.Any() ?
									"\n\tAdditions\n\t\t" + String.Join("\n\t\t", additions.OrderBy(kvp => kvp.Key).Select(kvp => $"{kvp.Key} : {PrettifyOption(kvp.Value, linePrefix: "\t\t\t")}")) :
									String.Empty
							) +
							(
								edits.Any() ?
									"\n\tEdits\n\t\t" + String.Join("\n\t\t", edits.OrderBy(tpl => tpl.Item1).Select
									(
										tpl =>
										{
											string prettifiedPrevious = PrettifyOption(tpl.Item2, linePrefix: "\t\t\t");
											string prettifiedCurrent = PrettifyOption(tpl.Item3, linePrefix: "\t\t\t");
											bool previousHasNewLine = prettifiedPrevious.Contains("\n");
											bool currentHasNewLine = prettifiedCurrent.Contains("\n");
											if (currentHasNewLine || currentHasNewLine)
											{
												if (!prettifiedPrevious.Contains("\n"))
												{
													prettifiedPrevious = $"\n\t\t\t{prettifiedPrevious}";
												}
												if (!prettifiedCurrent.Contains("\n"))
												{
													prettifiedCurrent = $"\n\t\t\t{prettifiedCurrent}";
												}
												return $"{tpl.Item1} : {prettifiedPrevious}\n\t\t=>{prettifiedCurrent}";
											}
											return $"{tpl.Item1} : {prettifiedPrevious} => {prettifiedCurrent}";
										}						
									)) :
									String.Empty
							) +
							(
								removals.Any() ?
									"\n\tRemovals\n\t\t" + String.Join("\n\t\t", removals.OrderBy(kvp => kvp.Key).Select(kvp => $"{kvp.Key} : {PrettifyOption(kvp.Value, linePrefix: "\t\t\t")}")) :
									String.Empty
							) +
							"\n"
						);
					}
					else
					{
						Project.Log.Minimal.WriteLine($"TRACE task for optionset {OptionSetName} at {Location} had no effect");
					}
				}
                else
                {
                    Project.Log.Minimal.WriteLine
                    (
                        $"TRACE added optionset {OptionSetName} at {Location}\n\t" +
						(
                            _optionSet.Options.Any() ? 
								String.Join("\n\t", _optionSet.Options.OrderBy(kvp => kvp.Key).Select(kvp => $"{kvp.Key} : {PrettifyOption(kvp.Value, linePrefix: "\t\t")}")) :
								"(No options)"
                        )
                    );
                }
            }

			if (_needAdd)
			{
				Project.NamedOptionSets[OptionSetName] = _optionSet;
			}
		}
	}
}
