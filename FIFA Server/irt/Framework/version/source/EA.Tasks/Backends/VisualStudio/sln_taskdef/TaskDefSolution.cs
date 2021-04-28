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

using System.Collections.Generic;
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Logging;

using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

using PackageCore = NAnt.Core.PackageCore;

namespace EA.Eaconfig.Backends.VisualStudio.sln_taskdef
{
	internal class TaskDefSolution : VSSolutionBase
	{

		public TaskDefSolution(Log log, string name, PathString outputdir, string vsVersion, string vsToolsVersion)
			: base(log, name, outputdir, vsVersion, vsToolsVersion, false)
		{
			 Packages = new Dictionary<string, Package>();
		}


		internal new IEnumerable<IModule> GetTopModules(Project project)
		{
			var configurations = new List<Configuration>() { new Configuration("Debug", "pc-vc", "vc", "pc", "debug", "x86"), new Configuration("Release", "pc-vc", "vc", "pc", "release", "x64") };

			return project.TaskDefModules().SelectMany(entry =>
				{
					var modules = new List<IModule>();
					foreach (var config in configurations)
					{
						modules.Add(CreateModule(entry.Value, config, project));
					}
					return modules;
				});
		}


		protected override ModuleGenerator CreateModuleGenerator(IEnumerable<IModule> modules)
		{
			return new TaskDefProject(this, modules);
		}


		#region Virtual Overrides for Writing Solution File

		protected override void WriteHeader(IMakeWriter writer)
		{
			writer.WriteLine("Microsoft Visual Studio Solution File, Format Version 12.00");
			writer.WriteLine("# Visual Studio 2013");
		}

		protected override string TeamTestSchemaVersion
		{
			get { return "2012"; }
		}

		#endregion Virtual Overrides for Writing Solution File

		private Module_DotNet CreateModule(TaskDefModule module_data, Configuration config, Project project)
		{
			var name = Path.GetFileNameWithoutExtension(module_data.AssemblyFileName);
			var buildtype = new BuildType("Library", "CSharp");

			// Find package based on the location of the script file

			var package = GetPackage(project, config, module_data);

			var module = new Module_DotNet(name, BuildGroups.runtime.ToString() + name, project.Properties["eaconfig." + BuildGroups.runtime + ".sourcedir"], config, BuildGroups.runtime, buildtype, package);

			module.OutputDir = PathString.MakeNormalized(Path.GetDirectoryName(module_data.AssemblyFileName));
			module.IntermediateDir = module.OutputDir;
			module.Compiler = new CscCompiler(new PathString("csc.exe"), DotNetCompiler.Targets.Library);
			module.Compiler.OutputName = name;
			module.Compiler.OutputExtension = Path.GetExtension(module_data.AssemblyFileName);
			module.Compiler.SourceFiles.IncludeWithBaseDir(module_data.Sources);
			foreach (var reference in module_data.References)
			{
				module.Compiler.Assemblies.Include(reference);
			}
			module.ExcludedFromBuild_Files.Include(module_data.ScriptFile);
			// ---- options -----
			module.Compiler.Options.Add("/nowarn:1607");  // Suppress Assembly generation - Referenced assembly 'System.Data.dll' targets a different processor
			module.Compiler.Options.Add("/platform:anycpu");

			module.Compiler.Defines.Add("FRAMEWORK3");

#if NETFRAMEWORK
			module.TargetFrameworkFamily = Tasks.Model.DotNetFrameworkFamilies.Framework;
#else
			module.TargetFrameworkFamily = Tasks.Model.DotNetFrameworkFamilies.Core;
#endif

			return module;
		}

		private Package GetPackage(Project project, Configuration config, TaskDefModule module_data)
		{
			PackageCore.Release release = PackageCore.PackageMap.Instance.FindOrInstallCurrentRelease(project, module_data.Package);

			var key = Package.MakeKey(release.Name, release.Version, config.Name);

			if (!Packages.TryGetValue(key, out Package package))
			{
				package = new Package(release, config.Name);
				package.Project = project;

				Packages.Add(key, package);
			}

			return package;

		}

		protected override void PopulateSolutionFolders()
		{
			foreach(var generator in ModuleGenerators.Values.Cast<TaskDefProject>())
			{
				SolutionFolders.SolutionFolder folder;

				string folder_name = generator.Package.Name + "-" + generator.Package.Version;

				if(!SolutionFolders.Folders.TryGetValue(folder_name, out folder))
				{
					folder = new SolutionFolders.SolutionFolder(generator.Package.Name, new PathString(folder_name));
					SolutionFolders.Folders.Add(folder_name, folder);
				}
				folder.FolderProjects.Add(generator.ProjectGuid, generator);
			}
		}


		internal override string GetTargetPlatform(Configuration configuration)
		{
			return "Any CPU";
		}

		private readonly Dictionary<string, Package> Packages;
	}
}
