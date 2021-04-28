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
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using EA.Eaconfig.Backends.VisualStudio;
using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends
{

	// TODO fix up task heiarchy
	// -dvaliant 2018/02/07
	/*
		This task is a bit confusing because it was hacked up in a hurry. The legacy version just handled VisualStudio generation but the new version can handle VisualStudio.

		Much more sane would be:
			BackendGenerateTask - handles generation mode switching
			SlnGenerator - (new class) handles things in VisualStudio mode
	*/

	[TaskName("backend-generator")]
	internal class BackendGenerateTask : BackendGenerateBaseTask
	{
		[TaskAttribute("generator-name", Required = true)]
		public string GeneratorTaskName { get; set; }

		[TaskAttribute("startup-config", Required = true)]
		public string StartupConfig { get; set; }

		[TaskAttribute("configurations", Required = true)]
		public string Configurations { get; set; }

		[TaskAttribute("generate-single-config", Required = false)]
		public bool GenerateSingleConfig { get; set; } = false;

		[TaskAttribute("split-by-group-names", Required = false)]
		public bool SplitByGroupNames { get; set; } = true;

		/// <summary>The Name of the Solution to Generate.</summary>
		[TaskAttribute("solution-name", Required = false)]
		public string SolutionName { get; set; }

		[TaskAttribute("enable-sndbs", Required = false)]
		public bool EnableSndbs { get; set; }

		public override string LogPrefix
		{
			get
			{
				return " [solution-generation] ";
			}
		}

		protected override void ExecuteTask()
		{
			if (GeneratorTaskName == "VisualStudio" || String.IsNullOrEmpty(GeneratorTaskName))
			{
				GenerateVisualStudioSolution();
			}
			else
			{
				throw new BuildException("Unknown generator task '" + GeneratorTaskName + "'. Valid values are VisualStudio.");
			}
		}

		public bool IsPortable
		{
			get
			{
				return Project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false);
			}
		}

		public string SlnFrameworkFilesRoot
		{
			get
			{
				if (String.IsNullOrEmpty(mSlnFrameworkFilesRoot))
				{
					if (IsPortable)
					{
						if (Project.Properties.Contains("eaconfig.portable-solution.framework-files-outputdir"))
						{
							mSlnFrameworkFilesRoot = System.IO.Path.GetFullPath(Project.Properties.ExpandProperties(Project.Properties["eaconfig.portable-solution.framework-files-outputdir"]));
						}
						else
						{
							mSlnFrameworkFilesRoot = System.IO.Path.Combine(Project.Properties["nant.project.temproot"], "MSBuild");
						}
					}
					else
					{
						mSlnFrameworkFilesRoot = NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path;
					}
				}
				return mSlnFrameworkFilesRoot;
			}
		}
		private string mSlnFrameworkFilesRoot = null;

		private void GenerateVisualStudioSolution()
		{
			var timer = new Chrono();
			Log.Minimal.WriteLine(LogPrefix + "Generating Solution(s)... ");

			DoPortableFrameworkCopy();

			var generatedFiles = new List<PathString>();

			var confignames = (Configurations + " " + StartupConfig).ToArray();

			FileFilters.Init(Project);

			BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
			if (stepsLog != null)
			{
				stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' + '{1}' steps", "backend.VisualStudio.pregenerate", "backend.pregenerate");
			}

			var procesed = new HashSet<string>();

			foreach (var package in Project.BuildGraph().TopModules.Select(m => m.Package).Distinct())
			{
				var steps = new HashSet<string>((package.Project.Properties["backend.VisualStudio.pregenerate"] + Environment.NewLine + package.Project.Properties["backend.pregenerate"]).ToArray());

				package.Project.ExecuteGlobalProjectSteps(steps.Except(procesed), stepsLog);

				procesed.UnionWith(steps);
			}

			IEnumerable<IModule> topmodules = GetTopModules();

			var duplicateProjectNames = Generator.FindDuplicateProjectNames(Project.BuildGraph().SortedActiveModules);

			int groupcount = 0;
			foreach (var modulegroup in Generator.GetTopModuleGroups(Project, topmodules, GenerateSingleConfig, SplitByGroupNames))
			{
				var output = RunOneGenerator(modulegroup.Key, modulegroup, confignames, duplicateProjectNames, StartupConfig, groupcount);

				generatedFiles.AddRange(output);
				groupcount++;
			}

			if (stepsLog != null)
			{
				stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' + '{1}' steps", "backend.VisualStudio.postgenerate", "package.postgenerate");
			}

			foreach (var package in Project.BuildGraph().TopModules.Select(m => m.Package))
			{
				var steps = new HashSet<string>((package.Project.Properties["backend.VisualStudio.postgenerate"] + Environment.NewLine + package.Project.Properties["package.postgenerate"]).ToArray());

				package.Project.ExecuteGlobalProjectSteps(steps.Except(procesed), stepsLog);

				procesed.UnionWith(steps);
			}

			if (generatedFiles.Count() == 0)
			{
				Log.Minimal.WriteLine(LogPrefix + "Finished - Module list is EMPTY.  Solution file was NOT generated! {0}", timer.ToString());
			}
			else if (generatedFiles.Count() == 1)
			{
				Log.Minimal.WriteLine(LogPrefix + "Finished {0} [{1}]", timer.ToString(), generatedFiles.First().Path);
			}
			else
			{
				string padding = String.Empty.PadLeft(Log.IndentLength);
				Log.Minimal.WriteLine(LogPrefix + "Finished {0} {1}Solution Files Generated:{2}", timer.ToString(), Environment.NewLine + padding, generatedFiles.ToString(Environment.NewLine + padding, p => String.Format("\t\t{0}", p.Path.Quote())));
			}
		}

		private IEnumerable<PathString> RunOneGenerator(Generator.GeneratedLocation location, IEnumerable<IModule> topmodules, IEnumerable<string> confignames, IDictionary<string, Generator.DuplicateNameTypes> duplicateProjectNames, string startupConfigName, int groupnumber)
		{
			var solutionName = location.Name;
			var generatorTemplates = location.GeneratorTemplates;

			if (!String.IsNullOrEmpty(SolutionName))
			{
				solutionName = SolutionName;
				generatorTemplates = GeneratorTemplatesData.CreateGeneratorTemplateData(Project, solutionName);

			}

			var generator = CreateGenerator(solutionName, location.Dir);

			generator.DuplicateProjectNames = duplicateProjectNames;

			IEnumerable<IModule> excludedRootModules = GetExcludedRootModules();

			if (generator.Initialize(Project, confignames, startupConfigName, topmodules, excludedRootModules, GenerateSingleConfig, generatorTemplates, groupnumber))
			{
				generator.Generate();
				generator.PostGenerate();
			}
			return generator.GeneratedFiles;
		}

		private Generator CreateGenerator(string slnname, PathString slndir)
		{
			string strVisualStudioVersion = SetConfigVisualStudioVersion.Execute(Project);
			string configVsVersion = Project.Properties["config-vs-version"];
			string configVsToolSetVersion = Project.Properties["config-vs-toolsetversion"];

			switch (strVisualStudioVersion)
			{
				case "15.0":
					return new VS15Solution(Project.Log, slnname, slndir, configVsVersion, configVsToolSetVersion, EnableSndbs);
				case "16.0":
					return new VS16Solution(Project.Log, slnname, slndir, configVsVersion, configVsToolSetVersion, EnableSndbs);
				default:
					break;
			}

			throw new BuildException(string.Format("This version of Framework does not support Visual Studio version {0}", strVisualStudioVersion));
		}

		private void DoPortableFrameworkCopy()
		{
			//copy files only for portable solution gen (and only if source and dest folders are different)
			if (IsPortable && String.Compare(SlnFrameworkFilesRoot, NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, true) != 0)
			{
				Log.Minimal.WriteLine(LogPrefix + "Copying (portable) framework files...");
				//This is the directory the files will be moved to.
				PathString destDir = (new PathString(Path.Combine(SlnFrameworkFilesRoot, "data"))).Normalize();
				PathString DLLdestDir = (new PathString(Path.Combine(SlnFrameworkFilesRoot, "bin"))).Normalize();

				List<string> portableFWFiles = new List<string>();
				List<string> portableDLLFiles = new List<string>();
				//adding files individually into a list to make the code doing the copy much simpler and easier to add files to in the future until there is a better solution for these portable files.
				portableFWFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"data\FrameworkMsbuild.props")).ToString());
				portableFWFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"data\FrameworkMsbuild.tasks")).ToString());
				portableFWFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"data\intellisense_undef_win.h")).ToString());

				//execute copy tasks for each portable file
				foreach (string file in portableFWFiles)
				{
					Log.Info.WriteLine(LogPrefix + "Coping file from: {0} - To: {1}", file, destDir);
					CopyTask task = new CopyTask();
					task.Project = Project;
					task.SourceFile = file;
					task.ToDirectory = destDir.ToString();
					task.Clobber = true;
					task.Execute();
				}

				portableDLLFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"bin\FrameworkCopyWithAttributes.dll")).ToString());
				portableDLLFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"bin\EAPresentationBuildTasks.dll")).ToString());
				portableDLLFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"bin\EAAssemblyResolver.dll")).ToString());
				portableDLLFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"bin\MaybePreserveImportLibraryTimestamp.dll")).ToString());
				portableDLLFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"bin\Microsoft.Build.dll")).ToString());
				portableDLLFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"bin\Microsoft.Build.Framework.dll")).ToString());
				portableDLLFiles.Add(Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"bin\Microsoft.Build.Utilities.Core.dll")).ToString());

				//execute copy tasks for each portable file
				foreach (string file in portableDLLFiles)
				{
					Log.Info.WriteLine(LogPrefix + "Coping file from: {0} - To: {1}", file, DLLdestDir);
					CopyTask task = new CopyTask();
					task.Project = Project;
					task.SourceFile = file;
					task.ToDirectory = DLLdestDir.ToString();
					task.Clobber = true;
					task.Execute();
				}
			}
		}
	}
}
