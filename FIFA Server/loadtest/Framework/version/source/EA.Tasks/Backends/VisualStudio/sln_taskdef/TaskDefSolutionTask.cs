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
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Backends.VisualStudio.sln_taskdef
{
	[TaskName("taskdef-sln-generator")]
	class TaskDefSolutionTask : Task
	{
		protected override void ExecuteTask()
		{
			var timer = new Chrono();

			var name = "TaskDefCodeSolution";
			var outputdir = new PathString(Path.Combine(Project.Properties["nant.project.temproot"], "TaskDefVisualStudioSolution"));
			var configurations = new List<string>() { "Debug", "Release" };            
			var startupconfig = configurations.First();

			string strVisualStudioVersion = SetConfigVisualStudioVersion.Execute(Project);
			string configVsVersion = Project.Properties["config-vs-version"];
			string configVsToolSetVersion = Project.Properties["config-vs-toolsetversion"];

			var solution = new TaskDefSolution(Project.Log, name, outputdir, configVsVersion, configVsToolSetVersion);

			var topmodules = solution.GetTopModules(Project);
			var includedmodules = topmodules.Union(topmodules.SelectMany(m => m.Dependents.FlattenAll(DependencyTypes.Build), (m, d) => d.Dependent).Where(m => !(m is Module_UseDependency)));

			solution.Initialize(Project, configurations, startupconfig, topmodules, includedmodules);

			solution.Generate();

			Log.Status.WriteLine(LogPrefix + "  {0} generated files:{1}{2}", timer.ToString(), Environment.NewLine, solution.GeneratedFiles.ToString(Environment.NewLine, p => String.Format("\t\t{0}", p.Path.Quote())));
		}
	}
}
