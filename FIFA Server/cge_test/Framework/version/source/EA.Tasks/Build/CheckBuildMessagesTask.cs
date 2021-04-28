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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

namespace EA.Eaconfig.Build
{
	[TaskName("check-build-messages")]
	public class CheckBuildMessagesTask : Task
	{
		[TaskAttribute("group", Required = false)]
		public string BuildGroup { get; set; }

		[TaskAttribute("module", Required = false)]
		public string ModuleName { get; set; }

		protected override void ExecuteTask()
		{
			Execute(Project, BuildGroup, ModuleName);
		}

		public static void Execute(Project project, string group, string module=null)
		{
			if (String.IsNullOrEmpty(group))
			{
				group = project.ExpandProperties("${eaconfig.${eaconfig.build.group}.groupname}");
			}
			if (String.IsNullOrEmpty(group))
			{
				group = "runtime";
			}

			var warnprop = String.Empty;
			var errorprop = String.Empty;

			if (!String.IsNullOrEmpty(module))
			{
				warnprop = String.Format("{0}.{1}.warn.message", group, module);
				errorprop = String.Format("{0}.{1}.error.message", group, module);
				ProcessMessages(project, warnprop, false);
				ProcessMessages(project, errorprop, true);
			}
			// Process package level messages:
			if (String.IsNullOrEmpty(warnprop) || String.IsNullOrEmpty(project.Properties[warnprop]))
			{
				ProcessMessages(project, String.Format("package.{0}.warn.message", group), false);
			}
			if (String.IsNullOrEmpty(errorprop) || String.IsNullOrEmpty(project.Properties[errorprop]))
			{
				ProcessMessages(project, String.Format("package.{0}.error.message", group), true);
			}
		}

		private static void ProcessMessages(Project project, string propname, bool fail)
		{
			DoOnce.Execute(project, propname, () =>
			{
				var val = project.Properties[propname];
				if (!String.IsNullOrEmpty(val))
				{
					if (fail)
					{
						throw new BuildException(val);
					}
					else
					{
						project.Log.Warning.WriteLine(val);
					}
				}
			},
			DoOnce.DoOnceContexts.project);

		}
	}
}
