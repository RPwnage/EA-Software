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
using System.Collections.Generic;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;

namespace EA.Eaconfig
{

	/// <summary>Executes a system command. Repeats execution if condition is met</summary>
	/// 
	[TaskName("repeatexecif", NestedElementsCheck = true)]
	public class RepeatExecifTask : ExecTask
	{
		public RepeatExecifTask(): base()
		{
		}

		/// <summary>List of patterns.</summary>
		[TaskAttribute("repeatpattern", Required = false)]
		public string RepeatPatterns { get; set; } = String.Empty;

		/// <summary>List of patterns.</summary>
		[TaskAttribute("maxrepeatcount", Required = false)]
		public int MaxCount { get; set; } = 2;

		[TaskAttribute("maxlinestoscan", Required = false)]
		public int MaxLines { get; set; } = -1;

		protected override void ExecuteTask() 
		{
			bool repeat = false;
			Exception error = null;
			int count = 0;

			if (!String.IsNullOrEmpty(RepeatPatterns))
			{
				ICollection<string>  temp_matches = StringUtil.ToArray(RepeatPatterns, "\n");
				if (temp_matches != null && temp_matches.Count > 0)
				{
					_matches = new List<string>();

					foreach (string match in temp_matches)
					{
						if (!String.IsNullOrEmpty(match))
						{
							string clean_match = match.Trim(TRIM_CHARS_KEEP_Q);
							if (!String.IsNullOrEmpty(clean_match))
							{
								_matches.Add(clean_match);
							}
						}
					}
				}
			}

			do
			{
				error = null;
				repeat = false;
				_matchFound = false;
				_scannedLines = 0;
				_matchLine = String.Empty;
				count++;            
				
				try
				{
					base.ExecuteTask();

				}
				catch (Exception ex)
				{
					error = ex;
					repeat = ComputeRepeat(count);

					if (repeat)
					{
						Log.Status.WriteLine("");
						Log.Status.WriteLine("------------------------------------------------------------------------------");
						Log.Status.WriteLine("----------");
						Log.Status.WriteLine("---------- Framework2");
						Log.Status.WriteLine("----------");
						Log.Status.WriteLine("---------- detected recoverable error: '{0}'", _matchLine);
						Log.Status.WriteLine("---------- restarting external command, restartcount = {0} [Max={1}] ...", count, MaxCount);
						Log.Status.WriteLine("----------");
						Log.Status.WriteLine("------------------------------------------------------------------------------");
						Log.Status.WriteLine("");
					}                    
				}

				count++;
			}
			while (repeat);

			if (error != null)
			{
				throw error;
			}
		}

		protected bool ComputeRepeat(int count)
		{
			return (_matchFound && (count < MaxCount));
		}

		/// <summary>Callback for procrunner stdout</summary>
		public override void LogStdOut(OutputEventArgs outputEventArgs)
		{
			base.LogStdOut(outputEventArgs);

			if (MaxLines <  0 || (MaxLines >  -1 && _scannedLines < MaxLines))
			{
				if (_matches != null && !_matchFound && outputEventArgs != null && outputEventArgs.Line != null)
				{
					foreach (string match in _matches)
					{
						if (outputEventArgs.Line.Contains(match))
						{
							_matchFound = true;
							_matchLine = match;
							break;
						}
					}
				}
			}
			_scannedLines++;
		}


		/// <summary>Callback for procrunner stderr</summary>
		public override void LogStdErr(OutputEventArgs outputEventArgs)
		{
			base.LogStdErr(outputEventArgs);

			if (MaxLines < 0 || (MaxLines > -1 && _scannedLines < MaxLines))
			{
				if (_matches != null && !_matchFound && outputEventArgs != null && outputEventArgs.Line != null)
				{
					foreach (string match in _matches)
					{
						if (outputEventArgs.Line.Contains(match))
						{
							_matchFound = true;
							_matchLine = match;
							break;
						}
					}
				}
			}
			_scannedLines++;
		}

		private ICollection<string> _matches = null;
		private bool _matchFound;
		private int _scannedLines = 0;
		private string _matchLine;

		private static readonly char[] TRIM_CHARS_KEEP_Q = new char[] { '\n', '\r', '\t', ' ' };

	}
}
