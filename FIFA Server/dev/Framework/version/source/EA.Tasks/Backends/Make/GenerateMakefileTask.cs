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

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.Eaconfig.Backends
{
	[TaskName("generate-makefile")]
	class GenerateMakefileTask : Task
	{
		[TaskAttribute("startup-config", Required = true)]
		public string StartupConfig { get; set; }

		[TaskAttribute("configurations", Required = true)]
		public string Configurations { get; set; }

		[TaskAttribute("generate-single-config", Required = false)]
		public bool GenerateSingleConfig { get; set; } = false;

		[TaskAttribute("split-by-group-names", Required = false)]
		public bool SplitByGroupNames { get; set; } = true;

		[TaskAttribute("use-compiler-wrapper", Required = false)]
		public bool UseCompilerWrapper { get; set; } = false;

		[TaskAttribute("for-config-systems", Required = false)]
		public string ForConfigSystems { get; set; }

		[TaskAttribute("warn-about-csharp-modules", Required = false)]
		public bool WarnAboutCSharpModules { get; set; } = true;

		protected override void ExecuteTask()
		{
			var timer = new Chrono();

			var generatedFiles = new List<PathString>();

			var confignames = (Configurations + " " + StartupConfig).ToArray();

			// Trim the confignames list if input specified to be used for a specific set of config systems.
			if (!String.IsNullOrEmpty(ForConfigSystems))
			{
				List<string> trueConfigList = new List<string>();
				List<string> configSysList = new List<string>(ForConfigSystems.ToArray());

				// When this task is called, it is expected that the build graph is already created!
				EA.FrameworkTasks.Model.BuildGraph buildGraph = EA.FrameworkTasks.Model.ProjectExtensions.BuildGraph(Project);
				if (buildGraph.IsBuildGraphComplete)
				{
					foreach (EA.FrameworkTasks.Model.Configuration cfg in buildGraph.ConfigurationList)
					{
						if (ForConfigSystems.Contains(cfg.System))
						{
							if (!trueConfigList.Contains(cfg.Name))
							{
								trueConfigList.Add(cfg.Name);
							}
						}
					}
					if (trueConfigList.Count > 0)
					{
						confignames = trueConfigList;
					}
				}
			}

			var generator = new MakeGenerator(Project, confignames, SplitByGroupNames, UseCompilerWrapper, WarnAboutCSharpModules);

			generator.GenerateMakefiles(generatedFiles);

			Log.Status.WriteLine(LogPrefix + "  {0} generated files:{1}{2}", timer.ToString(), Environment.NewLine, generatedFiles.ToString(Environment.NewLine, p => String.Format("\t\t{0}", p.Path.Quote())));
		}

	}
}
