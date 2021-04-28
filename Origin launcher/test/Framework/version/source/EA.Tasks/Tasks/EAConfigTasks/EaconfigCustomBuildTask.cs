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
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
	[TaskName("eaconfig-set-custom-build-data")]
	sealed class EaconfigCustomBuildTask : Task
	{
		protected override void ExecuteTask()
		{
			var stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
			if (stepsLog != null)
			{
				stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' + '{1}' steps", "backend.nant.pregenerate", "backend.pregenerate");
			}

			Project.ExecuteGlobalProjectSteps((Properties["backend.nant.pregenerate"] + Environment.NewLine + Properties["backend.pregenerate"]).ToArray(), stepsLog);

			SetCustomBuildTargetProperties(GetCustomBuildModule());
		}

		private ProcessableModule GetCustomBuildModule()
		{
			var key = Project.Properties["build.module.key"];

			IModule module;
			if (Project.BuildGraph().Modules.TryGetValue(key, out module))
			{

			}
			else
			{
				Log.Error.WriteLine(LogPrefix + "Can not find module: {0}", key);
			}

			return module as ProcessableModule;

		}

		private void SetCustomBuildTargetProperties(ProcessableModule module)
		{
			module.Package.Project.Properties["custom-build.usedependencies"] = module.Dependents.Where(d => !d.IsKindOf(DependencyTypes.Build) && d.Dependent.Package.Name != module.Package.Name).Select(d => d.Dependent.Package.Name).ToString(" ");
			module.Package.Project.Properties["custom-build.builddependencies"] = module.Dependents.Where(d => d.IsKindOf(DependencyTypes.Build) && d.Dependent.Package.Name != module.Package.Name).Select(d => d.Dependent.Package.Name).ToString(" ");

			Project.Properties["custom-build.buildname"] = module.OutputName;

			if (module.BuildType.Name == "StdProgram")
			{
				Project.Properties["custom-build.buildtype"] = "CustomProgram";
			}

			if (module.BuildType.Name == "StdLibrary")
			{
				Project.Properties["custom-build.buildtype"] = "CustomLibrary";
			}

			Project.Properties["custom-build.modulename"] = module.Name;
			Project.Properties["custom-build.outputdir"] = module.IntermediateDir.Path;

			if (module.IsKindOf(Module.Program))
			{
				Project.Properties["custom-build.buildtype"] = "CustomProgram";
			}
			else if (module.IsKindOf(Module.Library | Module.DynamicLibrary))
			{
				Project.Properties["custom-build.buildtype"] = "CustomLibrary";
			}
			else
			{
				Log.Warning.WriteLine(LogPrefix + "'custom-build.buildtype' is undefined for module '{0}', buildtype '{1}'", module.GroupName, module.BuildType.Name);
			}

			SetCustomBuildToolProperties(module.Tools.FirstOrDefault(t => t.ToolName == "cc") as CcCompiler);
			SetCustomBuildToolProperties(module.Tools.FirstOrDefault(t => t.ToolName == "asm") as AsmCompiler);
			SetCustomBuildToolProperties(module.Tools.FirstOrDefault(t => t.ToolName == "link") as Linker);
			SetCustomBuildToolProperties(module.Tools.FirstOrDefault(t => t.ToolName == "lib") as Librarian);
		}

		private void SetCustomBuildToolProperties(CcCompiler cc)
		{
			if (cc != null)
			{
				Project.NamedFileSets["custom-build.sourcefiles"] = cc.SourceFiles.SafeClone();
				Project.Properties["custom-build.ccdefines"] = cc.Defines.ToString(Environment.NewLine);
				Project.Properties["custom-build.ccoptions"] = cc.Options.ToString(Environment.NewLine);
				Project.Properties["custom-build.ccincludedirs"] = cc.IncludeDirs.ToString(Environment.NewLine, dir => dir.Path.Quote());
			}
			else
			{
				Project.NamedFileSets["custom-build.sourcefiles"] = new FileSet();
				Project.Properties["custom-build.ccdefines"] = String.Empty;
				Project.Properties["custom-build.ccoptions"] = String.Empty;
				Project.Properties["custom-build.ccincludedirs"] = String.Empty;
			}
		}

		private void SetCustomBuildToolProperties(AsmCompiler asm)
		{
			if (asm != null)
			{
				Project.NamedFileSets["custom-build.asmsourcefiles"] = asm.SourceFiles.SafeClone();
				Project.Properties["custom-build.asdefines"] = asm.Defines.ToString(Environment.NewLine);
				Project.Properties["custom-build.asoptions"] = asm.Options.ToString(Environment.NewLine);
				Project.Properties["custom-build.asincludedirs"] = asm.IncludeDirs.ToString(Environment.NewLine, dir => dir.Path.Quote());
			}
			else
			{
				Project.NamedFileSets["custom-build.asmsourcefiles"] = new FileSet();
				Project.Properties["custom-build.asdefines"] = String.Empty;
				Project.Properties["custom-build.asoptions"] = String.Empty;
				Project.Properties["custom-build.asincludedirs"] = String.Empty;
			}
		}

		private void SetCustomBuildToolProperties(Linker link)
		{
			if (link != null)
			{

				Project.Properties["custom-build.linkoptions"] = link.Options.ToString(Environment.NewLine);
			}
			else
			{
				Project.Properties["custom-build.linkoptions"] = String.Empty;
			}
		}

		private void SetCustomBuildToolProperties(Librarian lib)
		{
			if (lib != null)
			{
				Project.Properties["custom-build.liboptions"] = lib.Options.ToString(Environment.NewLine);
			}
			else
			{
				Project.Properties["custom-build.liboptions"] = String.Empty;
			}
		}

		/// <summary>The prefix used when sending messages to the log.</summary>
		public override string LogPrefix
		{
			get
			{
				return " [custom-build] ";
			}
		}
	}
}