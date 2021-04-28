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
using System.Collections.Concurrent;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;

using EA.Make;
using System.Threading.Tasks;

namespace EA.Eaconfig.Backends
{
	internal class MakeGenerator
	{
		private readonly Project Project;
		private readonly Log Log;
		private readonly string LogPrefix;
		private readonly IEnumerable<string> Configurations;
		private readonly bool SplitByGroupNames;
		private readonly bool UseToolWrapper;
		private readonly bool WarnAboutCSharpModules;

		private MakeModuleBase.PathModeType PathMode = MakeModuleBase.PathModeType.Auto | MakeModuleBase.PathModeType.SupportsQuote;

		internal MakeGenerator(Project project, IEnumerable<string> configurations, bool splitbygroupnames, bool usetoolwrapper, bool warnaboutcsharpmodules)
		{
			Project = project;
			Log = project.Log;
			LogPrefix = "[makegenerator] ";
			Configurations = configurations;
			SplitByGroupNames = splitbygroupnames;
			UseToolWrapper = usetoolwrapper;
			WarnAboutCSharpModules = warnaboutcsharpmodules;
		}

		internal void GenerateMakefiles(List<PathString> generated)
		{
			var topmodules = Project.BuildGraph().TopModules.Where(m => Configurations.Contains(m.Configuration.Name));
			if (topmodules.FirstOrDefault() == null)
			{
				Log.Warning.WriteLine(LogPrefix + "There are no modules to generate makefiles for configurations '{0}'.", Configurations.ToString(", "));
				return;
			}

			ConcurrentBag<PathString> concurrentList = new ConcurrentBag<PathString>();

#if NANT_CONCURRENT
			if (Project.NoParallelNant == false)
			{
				Parallel.ForEach(topmodules.GroupBy(m => m.Configuration), topConfigGroup =>
				{
					var makefile = GenerateSingleConfigMake(topConfigGroup, topConfigGroup.Key);

					if (!makefile.IsNullOrEmpty())
					{
						concurrentList.Add(makefile);
					}
				});
			}
			else
#endif
			{
				foreach (var topConfigGroup in topmodules.GroupBy(m => m.Configuration))
				{
					var makefile = GenerateSingleConfigMake(topConfigGroup, topConfigGroup.Key);

					if (!makefile.IsNullOrEmpty())
					{
						concurrentList.Add(makefile);
					}
				}
			}

			generated.AddRange(concurrentList.ToList());
		}

		internal PathString GenerateSingleConfigMake(IEnumerable<IModule> topmodules, Configuration config)
		{
			PathString topMakePath = null;

			if (!topmodules.IsNullOrEmpty())
			{
				var topproject = topmodules.First().Package.Project;

				var buildablemodules = topmodules.Union(topmodules.SelectMany(m => m.Dependents.FlattenAll(DependencyTypes.Build), (m, d) => d.Dependent).Where(m => !(m is Module_UseDependency)));

				using (var topmakefile = new TopMakeFile(topproject, PathMode, SplitByGroupNames))
				{
					topMakePath = topmakefile.GenerateMakeFile(buildablemodules);
				}

				var toolWrappers = GetVsimakeToolWrappers(topproject);

				foreach (var module in buildablemodules)
				{
					if (WarnAboutCSharpModules || !(module is Module_DotNet))
					{
						using (var modulemakefile = new ModuleMakeFile(module, PathMode, toolWrappers))
						{
							modulemakefile.GenerateMakeFile();
						}
					}
				}
			}

			return topMakePath;
		}

		internal MakeToolWrappers GetVsimakeToolWrappers(Project project)
		{
			const string compiler_wrapper_prop = "package.vstomaketools.compiler_wrapper.exe";

			var toolWrappers = new MakeToolWrappers();
			if (UseToolWrapper)
			{
				toolWrappers.UseCompilerWrapper = true;
				toolWrappers.CompilerWrapperExecutable = PathString.MakeNormalized(project.GetPropertyValue(compiler_wrapper_prop) ?? String.Empty);

				if (toolWrappers.CompilerWrapperExecutable.IsNullOrEmpty())
				{
					project.Log.Warning.WriteLine(LogPrefix + "Can't use compiler wrapper, property '{0}' is not defined", compiler_wrapper_prop);
					toolWrappers.UseCompilerWrapper = false;
				}
			}

			return toolWrappers;
		}
	}
}
