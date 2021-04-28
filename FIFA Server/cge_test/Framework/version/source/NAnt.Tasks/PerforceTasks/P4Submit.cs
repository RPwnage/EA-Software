// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001 Gerry Shaw
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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Jim Petrick (jpetrick@ea.com)

using System;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;
using System.Text.RegularExpressions;

namespace NAnt.PerforceTasks
{
	/// <summary>
	/// Submit a changelist to the depot.
	/// </summary>
	/// <remarks>
	/// When a file has been opened by <c>p4add, p4edit, p4delete,</c> or <c>p4integrate</c>, the file is
	///	added to a changelist.  The user's changes to the file are made only within in the client
	///	workspace until the changelist is sent to the depot with <c>p4submit</c>.
	///	
	/// If the <c>change</c> parameter is not specified, the default changelist will be used.
	/// 
	/// After task is executed the <c>change</c> parameter will contain actual submitted changelist.
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// The first command submits all the files from changelist <c>2112</c>, then reopens them for edit.
	/// <para>The second submits all files that match <c>//sandbox/*.txt</c>.</para>
	/// <para>Perforce Commands:</para>
	///	<code><![CDATA[
	///	p4 submit -r -c 2112
	///	p4 submit //sandbox/*.txt]]></code>
	///	Equivalent NAnt Tasks:
	///	<code><![CDATA[
	///	<p4submit reopen="true" change="2112" />
	///	<p4submit files="//sandbox/*.txt" />]]></code>
	/// </example>
	[TaskName("p4submit")]
	public class P4SubmitTask : P4Base 
	{
		public P4SubmitTask() : base("p4submit") { }

		string	_files		= null;
		string	_change		= null;

		/// <summary>
		/// A single file pattern may be specified as a parameter to a p4 submit of the 
		/// default changelist. This file pattern limits which files in the default changelist are 
		/// included in the submission; files that don't match the file pattern are moved to the 
		/// next default changelist.
		/// The file pattern parameter to p4submit may not be used if <c>change</c> is specified.
		/// </summary>
		[TaskAttribute("files")]
		public string Files
		{
			get { return _files; }
			set
			{
				if (value != null && Change != null)
				{
					throw new BuildException(this.GetType().ToString() + ": either 'files' or 'change' may be specified, but not both", Location);
				}

				_files = value;
			} 
		}
				
		/// <summary>Changelist to modify. After task is executed it contains actual submitted changelist</summary>
		[TaskAttribute("change")]
		public string Change
		{ 
			get { return _change; }
			set
			{
				if (value != null && Files != null)
				{
					throw new BuildException(this.GetType().ToString() + ": either 'files' or 'change' may be specified, but not both", Location);
				}

				_change = value;
			}
		}

		/// <summary>Reopens the submitted files for editing after submission.</summary>
		[TaskAttribute("reopen")]
		public bool Reopen { get; set; } = false;


		/// <summary>Gets the command line arguments for the application.</summary>
		public override string ProgramArguments		
		{ 
			get	
			{ 
				string options = GlobalOptions + " submit ";
				if (Reopen)
					options += " -r";
				if (Change != null)
					options += " -c " + Change;
				options += " " + Files;
				return options;
			} 
		}	//	end ProgramArguments

		/// <summary>
		/// Executes the P4Submit task
		/// </summary>
		protected override void ExecuteTask() 
		{
			P4Form form = new P4Form(ProgramFileName, BaseDirectory, FailOnError, Log);

			try	
			{
				//	Execute the command and retrieve the form (perforce output).
				Log.Info.WriteLine(LogPrefix + "running: '{0}{1}' workingDir='{2}'", form.ProgramName, ProgramArguments, form.WorkDir);
				form.GetForm(ProgramArguments);

				//	Extract any useful data from the output, in the line starting with '... path'
				string result = form.Form.Trim();
				Log.Info.WriteLine(LogPrefix + "{0}", result);

				if (result != null && result.Length > 0)
				{
					Match match = Regex.Match(result, @"(.*)Change(\s+)(?<number>[0-9]+)(\s+)submitted(.*)");
					if (!match.Success)
					{
						match = Regex.Match(result, @"(.*)change(\s+)(?<number>[0-9]+)(\s+)and(\s+)submitted(.*)");
					}
					if (match.Success)
					{
						Change = match.Groups["number"].Value;                        
					}
					//	Now set the ${p4.change} property. 
					Properties.Add("p4.change", Change);                    
				}
				else if (FailOnError)
				{
					throw new BuildException(LogPrefix + ": 'p4 submit' returned no result.");
				}
			}
			catch (BuildException e) 
			{
				if (IsZeroChange(form))
				{
					string result = form.Form.Trim();
					if (Verbose)
					{
						foreach (string line in result.Split(new char[] { '\n', '\r' }, StringSplitOptions.RemoveEmptyEntries))
						{
							Log.Info.WriteLine(LogPrefix + "{0}", line);
						}
					}
				}
				else
				{
					if (FailOnError)
					{
						throw new BuildException(this.GetType().ToString() + ": Error during P4 program execution.", Location, e);
					}
					else
					{
						Log.Error.WriteLine(LogPrefix + "P4 returned an error. {0}", e.Message);
					}
				}
			}
		}	// end ExecuteTask

		//Check for zero-change submit. It can return non-zero exit code.
		bool IsZeroChange(P4Form form)
		{
			bool isZeroChange = false;

			string result = form.Form.Trim();

			if (result != null && result.Length > 0)
			{
				Match match = Regex.Match(result, @"(.*)unchanged, reverted(.*)No files to submit(.*)", RegexOptions.Singleline);
					if (match.Success)
					{
						isZeroChange = true;
						if (Change == null)
						{
							Change = "0";
						}
					}
			}
			return isZeroChange;
		}

	}	// end class P4Submit
}
