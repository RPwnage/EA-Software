// Originally based on NAnt - A .NET build tool
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

// P4Where.cs
//
//
// Thanks to FusionBuildHelpers (Clayton Harbour, Steve Watson and Darren Li) for the inspiration
// OrcaCM li

using System;
using System.IO;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace NAnt.PerforceTasks
{
	/// <summary>
	/// Return the file name in the local view of the client given the name in the depot.
	/// </summary>
	/// <remarks>
	/// NOTE: If verbose is true and the local file (or directory) doesn't exist,
	/// a warning will be printed. 
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref/where.html#1040665">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	///	Print the local file name which is identified in the depot as <c>//depot/orca/bob.txt</c>.
	/// <para>Perforce Command (the third line in the output):</para>
	/// <code><![CDATA[
	/// p4 -ztag where //depot/orca/bob.txt]]></code>
	/// Equivalent NAnt Task:
	/// <code><![CDATA[
	/// <p4where filespec="//depot/orca/bob.txt" property="p4.where"/>
	/// <echo message="Local path is: ${p4.where}"/>]]></code>
	/// </example>
	[TaskName("p4where")]
	public class P4WhereTask : P4Base
	{
		public P4WhereTask() : base("p4where") { }

		private string _typeDesc = string.Empty;

		/// <summary>Perforce file spec in depot syntax.</summary>
		[TaskAttribute("filespec", Required = true)]
		[StringValidator(AllowEmpty = false, Trim = true)]
		public string FileSpec { get; set; }

		/// <summary>The property to set with the required path.</summary>
		[TaskAttribute("property", Required = true)]
		[StringValidator(AllowEmpty = false, Trim = true)]
		public string ToProperty { get; set; }

		/// <summary>The type of path required Can be one of these: Local, Depot or Client..</summary>
		[TaskAttribute("type")]
		[StringValidator(AllowEmpty = false, Trim = true)]
		public string Type { get; set; } = "Local";

		/// <summary>
		/// Initializes the P4Where task
		/// </summary>
		protected override void InitializeTask(XmlNode taskNode)
		{
			base.InitializeTask(taskNode);
			switch (Type)
			{
				case "Local":
					_typeDesc = "path";
					break;
				case "Depot":
					_typeDesc = "depotFile";
					break;
				case "Client":
					_typeDesc = "clientFile";
					break;
				default:
					throw new BuildException("P4Where type must be one of the following: 'Local', 'Client', 'Depot'");
			}
		}

		/// <summary>
		/// Executes the P4Where task
		/// </summary>
		protected override void ExecuteTask()
		{
			this.Properties[this.ToProperty] = String.Empty;
			P4Form form = new P4Form(ProgramFileName, BaseDirectory, FailOnError, Log);

			//	Build a command line to send to p4.
			// -ztag will place the output in separate lines
			string command = GlobalOptions + "-ztag where " + FileSpec;
			// p4 -ztag where <filespec> returns 3 lines: depotFile, clientFile, path
			try
			{
				//	Execute the command and retrieve the form (perforce output).
				Log.Info.WriteLine(LogPrefix + "running: '{0}{1}' wd='{0}'", form.ProgramName, command, form.WorkDir);
				form.GetForm(command);

				//	Extract any useful data from the output, in the line starting with '... path'
				string result = form.Form.Trim();
				if (result != null && result.Length > 0)
				{
					string[] outputlines = result.Split('\n');
					string path = null;
					string parseStr = "... " + _typeDesc;
					foreach (string outputline in outputlines)
					{
						if (outputline.StartsWith(parseStr))
						{
							path = outputline.Substring(parseStr.Length);
							path = path.Trim();
							continue;
						}
					}
					if (path == null)
						throw new BuildException(LogPrefix + ": 'p4 where' returned result has no path specified.");
					if (path.EndsWith("..."))
					{
						path = path.Substring(0, path.Length - 4);
					}
					if ((Type == "Local") && !File.Exists(path) && !Directory.Exists(path))
					{
						Log.Info.WriteLine(LogPrefix + "Warning: {0} does not exist on file system.", path);
					}
					this.Properties[this.ToProperty] = path;
				}
				else if (FailOnError)
				{
					throw new BuildException(LogPrefix + ": 'p4 where' returned no result.");
				}
			}
			catch (BuildException e)
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
		}	// end ExecuteTask
	}	// end class P4WhereTask
}
