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
using System.IO;
using System.Text;

using EA.Eaconfig;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace LinuxTasks
{
	[TaskName("CreateGDBDebuggingConfig")]
	public class CreateGDBDebuggingConfigTask : Task
	{
		[TaskAttribute("deployment-executable", Required = true)]
		public string DeploymentExecutable { get; set; }

		[TaskAttribute("deployment-commandlineargs", Required = false)]
		public string DeploymentArguments { get; set; }

		[TaskAttribute("deployment-host", Required = true)]
		public string DeploymentHost { get; set; }

		[TaskAttribute("deployment-port", Required = true)]
		public string DeploymentPort { get; set; }

		[TaskAttribute("deployment-username", Required = true)]
		public string DeploymentUsername { get; set; }

		[TaskAttribute("deployment-password", Required = true)]
		public string DeploymentPassword { get; set; }

		[TaskAttribute("deployment-path", Required = true)]
		public string DeploymentPath { get; set; }

		protected override void ExecuteTask()
		{
			TaskUtil.Dependent(Project, "PuTTY", Project.TargetStyleType.Use);
			string gdbConfigFile = Path.Combine(Properties["package.builddir"], DeploymentExecutable + ".gdbconfig.xml");

			string pipePath = Path.Combine(Properties["package.PuTTY.dir"], "bin/plink.exe");
			string pipeArguments = "-P " + DeploymentPort + " -pw " + DeploymentPassword + " " + DeploymentUsername + "@" + DeploymentHost + " -batch -t gdb --interpreter=mi";
			string exePath = PathExtensions.PathToUnix(Path.Combine(DeploymentPath, DeploymentExecutable));
			string targetArchitecture = "X64";
			string additionalSOLibSearchPath = "";

			StringBuilder gdbDebuggingConfigStringBuilder = new StringBuilder();
			gdbDebuggingConfigStringBuilder.AppendLine("<PipeLaunchOptions xmlns=\"http://schemas.microsoft.com/vstudio/MDDDebuggerOptions/2014\"");
			gdbDebuggingConfigStringBuilder.AppendLine("\tPipePath=\"" + pipePath + "\" PipeArguments=\"" + pipeArguments + "\"");
			gdbDebuggingConfigStringBuilder.AppendLine("\tExePath=\"" + exePath + "\" ExeArguments=\"" + DeploymentArguments + "\"");
			gdbDebuggingConfigStringBuilder.AppendLine("\tTargetArchitecture=\"" + targetArchitecture + "\" WorkingDirectory=\"" + DeploymentPath + "\" AdditionalSOLibSearchPath=\"" + additionalSOLibSearchPath + "\">");
			gdbDebuggingConfigStringBuilder.AppendLine("</PipeLaunchOptions>");

			Directory.CreateDirectory(Path.GetDirectoryName(gdbConfigFile));
			using (StreamWriter sw = new StreamWriter(gdbConfigFile, false))
			{
				sw.Write(gdbDebuggingConfigStringBuilder.ToString());
			}
		}
	}
}
