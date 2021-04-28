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

using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;



namespace EA.Make
{
	public class TopMakeFile : MakeModuleBase
	{
		private Project Project;
		private readonly PathString makeFileDir;
		private readonly PathString packageBuildDir;
		private readonly string makeFileName;

		public TopMakeFile(Project project, PathModeType pathmode, bool splitbyGroupNames) : base(project.Log, pathmode, new MakeToolWrappers())
		{
			Project = project;
			makeFileDir = GeneratedMakeFilePath.GetMakeFileDir(project);
			makeFileName = GeneratedMakeFilePath.GetMakeFileName(project, splitbyGroupNames);
			packageBuildDir = PathString.MakeNormalized(Project.GetPropertyValue("package.builddir"));
		}

		protected override string MakeFileDir 
		{
			get 
			{
				return makeFileDir.Path; 
			}
		}

		protected override string PackageBuildDir
		{
			get
			{
				return packageBuildDir.Path;
			}
		}


		protected override string MakeFileName 
		{ 
			get 
			{
				return makeFileName;
			}
		}


		public PathString GenerateMakeFile(IEnumerable<IModule> modules)
		{
			MakeFile.HEADER = "Generated makefile for " ;
 
			MakeFile.PHONY += "build clean";

			MakeFile.Variable("FLAGS").Value = "";
			MakeFile.Variable("TARGET").Value = "";

			var buildtarget = MakeFile.Target("build");

			AddTarget("clean");

			foreach(var module in modules)
			{
				var module_build_target = MakeFile.Target(TargetName(module));

				foreach(var dep in module.Dependents)
				{
					if (dep.IsKindOf(DependencyTypes.Build) && !(dep.Dependent is Module_UseDependency))
					{
						module_build_target.Prerequisites += TargetName(dep.Dependent);
					}
				}

				module_build_target.AddRuleLine("@$(MAKE) $(FLAGS) -C{0} -f{1} $(TARGET)", ToMakeFilePath(module.MakeFileDir()), module.MakeFileName().Quote());

				buildtarget.Prerequisites += module_build_target.Target;
			}

			return PathString.MakeCombinedAndNormalized(MakeFileDir, MakeFileName);
		}

		public static string TargetName(IModule module)
		{
			return String.Format("{0}_{1}_{2}", module.Package.Name, module.BuildGroup, module.Name);
		}

		private void AddTarget(string name)
		{
			var target = MakeFile.Target(name);
			target.Prerequisites += "TARGET="+name;
			target = MakeFile.Target(name);
			target.Prerequisites += "build";
		}

	}
}
