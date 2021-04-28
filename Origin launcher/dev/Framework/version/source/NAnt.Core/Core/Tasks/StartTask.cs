// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2002-2003 Gerry Shaw
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
// File Maintainer
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Diagnostics;
using System.Collections;
using System.Collections.Generic;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks {

	/// <summary>Launches an application or document.</summary>
	/// <remarks>
	///   The main purpose of this class is to open documents, web pages and launch GUI apps.  
	///   If you want to capture the output of a program use the <see cref="ExecTask"/>.
	/// </remarks>
	/// <example>
	/// <para>Open a document:</para>
	/// <code>
	/// <![CDATA[
	/// <project>
	///     <start filename='file.txt' />
	/// </project>
	/// ]]>
	/// </code> 
	/// </example>
	/// <example>
	/// <para>Open a web page:</para>
	/// <code>
	/// <![CDATA[
	/// <project>
	///     <start filename='iexplore.exe'>
	///         <args>
	///             <arg value='www' />
	///         </args>
	///     </start>
	/// </project>
	/// ]]>
	/// </code> 
	/// </example>
	[TaskName("start", NestedElementsCheck = true)]
	public class StartTask : Task {

		public static readonly string EnvironmentVariablePropertyNamePrefix = "exec.env";

		/// <summary>The program or document to run.  Can be a program, document or URL.</summary>
		[TaskAttribute("filename", Required = true)]
		public string FileName { get; set; }

		/// <summary>The working directory to start the program from.</summary>
		[TaskAttribute("workingdir")]
		public string WorkingDirectory { get; set; }

		/// <summary>If true the process will be killed right after being started.  Used by automated tests.</summary>
		[TaskAttribute("kill")]
		public bool Kill { get; set; } = false;

		/// <summary>The set of command line arguments.</summary>
		[ArgumentSet("args")]
		public ArgumentSet ArgSet { get; } = new ArgumentSet();

		/// <summary>Set true to clear all environmental variable before executing, environmental variable specified in env OptionSet will still be added as environmental variable.</summary>
		[TaskAttribute("clearenv")]
		public bool ClearEnv { get; set; } = false;

		/// <summary>The set of environment variables for when the program runs.
		/// Benefit of setting variables, like "Path", here is that it will be
		/// local to this program execution (i.e. global path is unaffected).</summary>
		[OptionSet("env")]
		public OptionSet EnvironmentVariables { get; set; } = new OptionSet();

		/// <summary>Start process in a separate shell. Default is true. 
		/// NOTE. When useshell is set to true environment variables are not passed to the new process.</summary>
		[TaskAttribute("useshell")]
		public bool UseShell { get; set; } = true;

		[TaskAttribute("detached")]
		public bool Detached { get; set; } = false;

		protected override void ExecuteTask()
		{
			Log.Status.WriteLine(LogPrefix + "{0} {1}", FileName, ArgSet.ToString());
			try 
			{
				ProcessStartInfo startInfo = new ProcessStartInfo(FileName);
				PrepareStartupInfo(startInfo);
				if (Detached)
				{
					var envDict = new Dictionary<string, string>();
					foreach (DictionaryEntry e in startInfo.EnvironmentVariables)
					{
						envDict.Add(e.Key as string, e.Value as string);
					}
					ProcessUtil.CreateDetachedProcess(startInfo.FileName, startInfo.Arguments, startInfo.WorkingDirectory, envDict, ClearEnv);
				}
				else
				{
					Process p = null;
					try
					{
						p = Process.Start(startInfo);
					}
					catch (Exception e)
					{
						// Under unix environment, if file doesn't have exe bit set, we usually get permission denied message.
						// But unfortunately, we get "cannot find specified file" which is confusing. So we explicitly test for
						// this scenario and give better message.
						bool exeBitSet = true;
						try
						{
							if ((PlatformUtil.IsUnix || PlatformUtil.IsOSX) && System.IO.File.Exists(startInfo.FileName))
							{
								exeBitSet = ProcessUtil.IsUnixExeBitSet(startInfo.FileName, startInfo.WorkingDirectory);
							}
						}
						catch (Exception e2)
						{
							throw new Exception(e.Message + Environment.NewLine + e2.Message);
						}
						if (!exeBitSet)
						{
							throw new Exception(String.Format("ERROR: The executable '{0}' appears to be lacking execute attribute.  Please make sure that the execute bit is set!", startInfo.FileName));
						}
						else
						{
							// Seems to be some other exception.  Just re-throw the error.
							throw e;
						}
					}

					if (Kill)
						p.Kill();
				}
			} 
			catch (Exception e) 
			{
				string msg = String.Format("Could not start '{0}'.", FileName);
				throw new BuildException(msg, Location, e);
			}
		}

		protected void PrepareStartupInfo(ProcessStartInfo startInfo)
		{
			if (WorkingDirectory != null)
			{
				startInfo.WorkingDirectory = WorkingDirectory;
			}
			startInfo.Arguments = ArgSet.ToString();
			startInfo.UseShellExecute = UseShell;
			if (!UseShell || Detached)
			{
				Dictionary<string, string> envMap = new Dictionary<string, string>();
				if (!ClearEnv)
				{
					Log.Info.WriteLine(LogPrefix + "<start> {0} without clearing environmental variable.", startInfo.FileName);

					// LEAVE IN FOR BACKWARDS COMPATIBILITY (old school style of passing env vars)
					// Set all the property values in exec.env.* to the Process's environment.
					// This allows people to be pass a specific environment to the program being executed.
					foreach (Property property in Project.Properties)
					{
						if (property.Prefix == EnvironmentVariablePropertyNamePrefix)
						{
							envMap[property.Suffix] = property.Value;
						}
					}
				}
				else
				{
					ProcessUtil.SafeInitEnvVars(startInfo);

					startInfo.EnvironmentVariables.Clear();
				}

				// set each option in the env option set as an environment variable
				foreach (KeyValuePair<string,string> entry in EnvironmentVariables.Options)
				{
					envMap[entry.Key] = entry.Value;
				}

				ProcessUtil.SetEnvironmentVariables(envMap, startInfo, ClearEnv, Log);
			}
		}

	}
}