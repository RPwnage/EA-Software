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
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Core.Logging;
using NAnt.Core.Events;
using NAnt.NuGet;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;
using NAnt.NuGet.Deprecated;

namespace EA.Eaconfig.Backends.VisualStudio
{
	// DAVE-REFACTOR-TODO: having a class (slns and projects) hierarchy based on VS editions is pretty pointless given the range we support and their similar
	// they are and the lack of overrides we need, should refactor into fewer classes
	internal abstract class VSSolutionBase : Generator
	{
		internal readonly string VSVersion;
		internal readonly string VSToolsetVersion;

		internal VSSolutionBase(Log log, string name, PathString outputdir, string vsVersion, string vsToolSetVersion, bool enableSndbs)
			: base(log, name, outputdir)
		{
			VSVersion = vsVersion;
			VSToolsetVersion = vsToolSetVersion;

			LogPrefix = " [visualstudio] ";
			SolutionFolders = new SolutionFolders(LogPrefix);

			_solutionConfigurationNameOverrides = new Dictionary<Configuration, string>();

			SndbsEnabled = enableSndbs;
		}

		/// <summary>The prefix used when sending messages to the log.</summary>
		public readonly string LogPrefix;
		public IEnumerable<string> Platforms
		{
			get
			{
				return ConfigToPlaforms.Values;
			}
		}

		public bool SndbsEnabled { get; private set; }

		public Dictionary<Configuration, string> ConfigToPlaforms = new Dictionary<Configuration, string>();

		public NugetVSPowerShellEnv NugetVSEnv; // environement for running post generate nuget scripts, we only expect this to be valid during postgenerate

		protected void OnSolutionFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} solution file  '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
					Log.Minimal.WriteLine(message);
				else
					Log.Status.WriteLine(message);
			}
		}

		public override bool Initialize(Project project, IEnumerable<string> configurations, string startupconfigName, IEnumerable<IModule> topmodules, IEnumerable<IModule> excludedRootModules, bool generateSingleConfig = false, GeneratorTemplatesData generatorTemplates = null, int groupnumber = 0)
		{
			PopulateVisualStudioPlatformMappings(project);

			IsPortable = project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false);

			bool ret = base.Initialize(project, configurations, startupconfigName, topmodules, excludedRootModules, generateSingleConfig, generatorTemplates, groupnumber);
			if (!ret)
			{
				return ret;
			}

			var topModulesForConfigs = topmodules.Where(m => Configurations.Contains(m.Configuration));

			// If all the topmodules are ExternalVSProjects, then add the configuration overrides for the solution (because nothing else is going to do it and the solution will end up with irregular config names)
			// The external vs projects must still have a ProjectConfig property set in their VisualStudioProject task, that is what will be used to fill the solution config names
			bool allTopModulesAreExternal = topModulesForConfigs.All(x => x.IsKindOf(Module.ExternalVisualStudio));

			foreach (var topmodule in topModulesForConfigs)
			{
				ConfigToPlaforms[topmodule.Configuration] = GetVisualStudioPlatformName(topmodule.Package.Project);
				var genkey = ModuleGenerator.MakeKey(topmodule);
				ModuleGenerator topgenerator;
				if (ModuleGenerators.TryGetValue(genkey, out topgenerator))
				{
					VSProjectBase vsproject = topgenerator as VSProjectBase;
					string configoverride;
					if (
						vsproject != null && 
						(!(vsproject is ExternalVSProject) || allTopModulesAreExternal) && // external vs projects are ignored (if there are non-external vs projects) for the purposes of solution config names, as the override is specific to the modules, not the overall Framework configuration - we will map from the project config to the solution config in project section
						vsproject.ProjectConfigurationNameOverrides.TryGetValue(topmodule.Configuration, out configoverride))
					{
						if (!_solutionConfigurationNameOverrides.TryGetValue(topmodule.Configuration, out string foundconfigoverride))
						{
							_solutionConfigurationNameOverrides.Add(topmodule.Configuration, configoverride);
						}
						else if (foundconfigoverride != configoverride)
						{
							Log.Warning.WriteLine("Solution configuration name overrides contain conflicting settings {0} -> {1} and {0} -> {2}. Second override is ignored.", topmodule.Configuration.Name, foundconfigoverride, configoverride);
						}
					}
				}
			}

			PopulateSolutionFolders();

			// --- find startup module generator:
			_startupModuleGenerator = GetStartupProject(project, topmodules);

			return ret;
		}

		public override void PostGenerate()
		{
			string solutionFileName = Path.Combine(OutputDir.NormalizedPath, SolutionFileName);
			NugetVSEnv = new NugetVSPowerShellEnv(solutionFileName, VSVersion);
			try
			{
				// gather nuget *init* files - these should only be run once per solution so we gather them all here
				List<FileSet> nugetInitFiles = new List<FileSet>();
				IEnumerable<VSDotNetProject> dotNetGenerators = ModuleGenerators.Values.Where(mg => mg is VSDotNetProject).Cast<VSDotNetProject>();
				foreach (VSDotNetProject dotNetProject in dotNetGenerators)
				{
					dotNetProject.GatherNugetFilesets("nuget-init-scripts", new string[] { "supress-nuget-init", "suppress-nuget-init" }, ref nugetInitFiles, true);
				}

				// run init files
				if (nugetInitFiles.SelectMany(fs => fs.FileItems).Any())
				{
					Log.Minimal.WriteLine("Executing {0} post-generate nuget init scripts for {1}.", nugetInitFiles.Count, solutionFileName);
					foreach (FileSet scriptSet in nugetInitFiles)
					{
						// path tp init.ps1 script
						foreach (FileItem scriptFile in scriptSet.FileItems)
						{
							string scriptPath = scriptFile.Path.NormalizedPath;
							FileInfo fileInfo = new FileInfo(scriptPath);

							// parameters for init script
							string toolsPath = fileInfo.Directory.FullName;
							string installPath = fileInfo.Directory.Parent.FullName;
 
							// run script
							Log.Info.WriteLine("Executing {0}.", scriptPath);
							NugetVSEnv.CallInitScript(scriptPath, installPath, toolsPath, null);
						}
					}
				}

				base.PostGenerate();

				ExecuteExtensions(extension => extension.PostGenerate());

				foreach (bool doingBefore in new bool[] { true, false })
				{
					StringBuilder buildTargetsMSbuildXml = new StringBuilder();
					if (doingBefore)
					{
						ExecuteExtensions(extension => extension.BeforeSolutionBuildTargets(buildTargetsMSbuildXml));
					}
					else
					{
						ExecuteExtensions(extension => extension.AfterSolutionBuildTargets(buildTargetsMSbuildXml));
					}
					string msbuildTargetsFile = (doingBefore ? "before." : "after.") + Name + ".sln.targets";
					string targetsFileFullPath = Path.Combine(OutputDir.NormalizedPath, msbuildTargetsFile);

					// Create this before/after targets file only if there are content.
					if (buildTargetsMSbuildXml.Length > 0)
					{
						using (MakeWriter writer = new MakeWriter())
						{
							writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnSolutionFileUpdate);
							writer.FileName = targetsFileFullPath;
							writer.WriteLine("<Project>");
							writer.Write(buildTargetsMSbuildXml.ToString());
							writer.WriteLine("</Project>");
						}
					}
					else if (File.Exists(targetsFileFullPath))
					{
						// Remove old stale file if no longer needed.
						File.Delete(targetsFileFullPath);
					}
				}
			}
			finally
			{
				NugetVSEnv.Close();
				NugetVSEnv = null;
			}
		}

		internal static string ToString(Guid guid)
		{
			return guid.ToString("B").ToUpperInvariant();
		}

		private class PackageComparerByName : IEqualityComparer<IPackage>
		{
			public bool Equals(IPackage pkg1, IPackage pkg2)
			{
				if (pkg1 == null && pkg2 == null)
					return true;
				else if (pkg1 == null || pkg2 == null)
					return false;
				else
					return pkg1.Name.Equals(pkg2.Name);
			}

			public int GetHashCode(IPackage pkg)
			{
				return pkg.Name.GetHashCode();
			}

			static public PackageComparerByName ComparerInstance = new PackageComparerByName();
		};

		protected virtual void PopulateSolutionFolders()
		{
			// We include the packages from the excluded module list in case we have solution folder setup
			// we want to include.  This typically happen to top module being a dummy driver util module.
			IEnumerable<IPackage> packagesInExcludes = ExcludedModules.Where(m=>m.Package != null && m.Package.Project != null).Select(m => m.Package).Distinct(PackageComparerByName.ComparerInstance);

			var packages = ModuleGenerators.Values.Select(mg => mg.Package).Concat(packagesInExcludes).Distinct(PackageComparerByName.ComparerInstance);

			var processor = new SolutionFolders.ModuleGeneratorProcessor(ModuleGenerators);

			foreach (IPackage package in packages)
			{
				SolutionFolders.ProcessFolders(package.Project);

				SolutionFolders.ProcessProjects(package.Project, ModuleGenerators.Values, processor);
			}
		}

		protected virtual ModuleGenerator GetStartupProject(Project project, IEnumerable<IModule> topmodules)
		{
			string startupProject = project.Properties[Name + ".sln.startupproject"] ?? project.Properties["package.sln.startupproject"];
			if (null == startupProject)
			{
				// Select executable projects.
				// Generator keys for executables. If we have more than one, try select module with name equal to package name:
				var generatorKeys = topmodules.Where(m => m.IsKindOf(Module.Program)).Select(m => ModuleGenerator.MakeKey(m)).Distinct();

				// If the build file does not specify which project to use as the
				// default startup project, use the package name as the start of
				// the project.  If there is no project that matches the name
				// of the package, the projects just get sorted by name making the startup project
				// the 1st alphabetically in the list
				var package = project.Properties["package.name"];

				ModuleGenerator gen;

				foreach (var key in generatorKeys)
				{

					if (ModuleGenerators.TryGetValue(key, out gen))
					{
						if (gen.ModuleName == package || gen.Name == package)
						{
							return gen as VSProjectBase;
						}
					}
				}

				if (ModuleGenerators.TryGetValue(generatorKeys.FirstOrDefault() ?? "nonexistent", out gen))
				{
					return gen as VSProjectBase;
				}

				startupProject = package;
			}

			ModuleGenerator startup = ModuleGenerators.Values.Where(proj => proj.Name == startupProject).FirstOrDefault();
			if (startup != null)
			{
				return startup;
			}
			return startup;
		}

		protected void PopulateVisualStudioPlatformMappings(Project project)
		{
			foreach (var module in project.BuildGraph().SortedActiveModules)
			{
				if (module.Package.Project != null
					// Dynamically created packages may use existing projects from different packages. Skip such projects
					&& (module.Package.Name == module.Package.Project.Properties["package.name"] && module.Configuration.Name == module.Package.Project.Properties["config"])
					&& !_targetPlatformMapping.ContainsKey(module.Configuration.Platform))
				{
					_targetPlatformMapping.Add(module.Configuration.Platform, GetVisualStudioPlatformName(module.Package.Project));
				}
			}
		}

		protected override void CreateModules(Project startupProject, IEnumerable<IModule> topmodules)
		{
			base.CreateModules(startupProject, topmodules);
			return;
		}

		protected VSProjectTypes GetVSProjectType(IEnumerable<IModule> modules)
		{
			var types = modules.Select(m =>
				{
					if (m.IsKindOf(Module.Native))
						return VSProjectTypes.Native;
					else if (m.IsKindOf(Module.ExternalVisualStudio))
						return VSProjectTypes.ExternalProject;
					else if (m.IsKindOf(Module.CSharp))
						if (m.IsKindOf(Module.ForceMakeStyleForVcproj))
							return VSProjectTypes.Native;
						else
							return VSProjectTypes.CSharp;
					else if (m.IsKindOf(Module.Utility))
						if (m.IsKindOf(Module.ForceMakeStyleForVcproj))
							return VSProjectTypes.Native;
						else
							return VSProjectTypes.Utility;
					else if (m.IsKindOf(Module.MakeStyle))
						if (m.IsKindOf(Module.ForceMakeStyleForVcproj))
							return VSProjectTypes.Native;
						else
							return VSProjectTypes.Make;
					else if (m.IsKindOf(Module.Python))
						return VSProjectTypes.Python;
					else if (m.IsKindOf(Module.Apk) || m.IsKindOf(Module.Aar))
					{
						// when using tegra for apk projects - even if we're just building java code - we need to create a native style vcxproj so we map apk to "native"
						if (m.Package.Project.Properties.GetPropertyOrFail("package.android_config.build-system").StartsWith("msvs-android"))
						{
							return VSProjectTypes.Android;
						}
						else
						{
							return VSProjectTypes.Native;
						}
					}
					else if (m.IsKindOf(Module.Java))
						return VSProjectTypes.Utility; // java modules that aren't package as .aar or .apk do not build, but are included for reference
					else
					{
						throw new BuildException("Internal error: Unknown module type in module '" + m.Name + "'");
					}
				}).Distinct();

			if (types.Count() > 1)
			{
				throw new BuildException(String.Format("Module '{0}' has different types '{1}' in different configurations. VisualStudio does not support this feature.", modules.FirstOrDefault().Name, types.ToString(", ", t => t.ToString())));
			}

			return types.First();
		}

		protected String GetSolutionConfigName(Configuration config)
		{
			string solutionConfigName;
			if (!_solutionConfigurationNameOverrides.TryGetValue(config, out solutionConfigName))
			{
				solutionConfigName = config.Name;
			}
			return solutionConfigName;
		}

		#region Abstract Methods Implementation

		public override IEnumerable<PathString> GeneratedFiles
		{
			get
			{
				return new List<PathString>() { PathString.MakeNormalized(Path.Combine(OutputDir.Path, SolutionFileName), PathString.PathParam.NormalizeOnly) };
			}
		}

		public override ModuleGenerator StartupModuleGenerator
		{
			get { return _startupModuleGenerator; }
		}

		public override void Generate()
		{

			WriteToFile();
		}


		internal void WriteToFile()
		{
			using (IMakeWriter writer = new MakeWriter(writeBOM: false))
			{
				writer.FileName = Path.Combine(OutputDir.Path, SolutionFileName);

				writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnSolutionFileUpdate);

				if (writer.Encoding.GetPreamble().Length > 0)
				{
					writer.WriteLine();
				}

				WriteHeader(writer);

				// If folders are listed first then the default startup project is as expected
				// otherwise if folders are used it selects a seemingly random module as the startup project
				WriteSolutionFoldersDefinitions(writer);

				// --- Write Startup module
				if (StartupModuleGenerator != null)
				{
					WriteProject(writer, StartupModuleGenerator as VSProjectBase);
				}

				// --- Write project list
				foreach (ModuleGenerator project in ModuleGenerators.Values.OrderBy(g => g.Name, StringComparer.InvariantCultureIgnoreCase).Where(g => !Object.ReferenceEquals(g, StartupModuleGenerator)))
				{
					WriteProject(writer, project as VSProjectBase);
				}

				writer.WriteLine("Global");
				{
					WriteSolutionFoldersContent(writer);

					// --- Write global configuration section  
					WriteSolutionPlatformMapping(writer);

					// --- Write project configuration mappings
					writer.WriteTabLine("GlobalSection(ProjectConfigurationPlatforms) = postSolution");
					foreach (var project in ModuleGenerators.Values.OrderBy(g => (g as VSProjectBase).ProjectGuidString))
					{
						WriteProjectConfigMapping(writer, project as VSProjectBase);
					}
					writer.WriteTabLine("EndGlobalSection");

					ExecuteExtensions(extension => extension.WriteGlobalSection(writer));

					writer.WriteTabLine("GlobalSection(SolutionProperties) = preSolution");
					writer.WriteTab(String.Empty);
					writer.WriteTabLine("HideSolutionNode = FALSE");
					writer.WriteTabLine("EndGlobalSection");
				}
				writer.WriteLine("EndGlobal");

			}
		}

		protected void ExecuteExtensions(Action<IVisualStudioSolutionExtension> action)
		{
			foreach (var ext in GetExtensionTasks())
			{
				action(ext);
			}
		}

		protected IEnumerable<IVisualStudioSolutionExtension> GetExtensionTasks()
		{
			List<IVisualStudioSolutionExtension> extensions = new List<IVisualStudioSolutionExtension>();

			var extensionTaskNames = new List<string>();
			extensionTaskNames.AddRange(StartupProject.Properties["visualstudio-solution-extensions"].ToArray());

			// Since snow cache project generation is baked into Framework, we'll test for snowcache stuff here as well.
			// Ideally, this should be moved out of Framework.
			string snowCacheMode = StartupProject.Properties.GetPropertyOrDefault("package.SnowCache.mode", null);
			if (snowCacheMode=="upload" || snowCacheMode == "uploadanddownload" || snowCacheMode=="download" || snowCacheMode == "forceupload")
			{
				if (!extensionTaskNames.Contains("snowcache-solution-extension"))
				{
					extensionTaskNames.Add("snowcache-solution-extension");
				}
			}

			foreach (var extensionTaskName in extensionTaskNames)
			{
				var task = StartupProject.TaskFactory.CreateTask(extensionTaskName, StartupProject);
				var extension = task as IVisualStudioSolutionExtension;
				if (extension != null)
				{
					extension.SolutionGenerator = new IVisualStudioSolutionExtension.SlnGenerator(this);
					extensions.Add(extension);
				}
				else if (task != null)
				{
					Log.Warning.WriteLine("Visual Studio Solution generator extension Task '{0}' does not implement IVisualStudioSolutionExtension. Task is ignored.", extensionTaskName);
				}
			}

			return extensions;
		}

		internal string SolutionFileName
		{
			get
			{
				return Name + ".sln";
			}
		}

		#endregion

		#region Internal Static Methods

		internal virtual string GetTargetPlatform(Configuration configuration)
		{
			string targetPlatform = DefaultTargetPlatform;

			if (!_targetPlatformMapping.TryGetValue(configuration.Platform, out targetPlatform))
			{
				targetPlatform = DefaultTargetPlatform;
			}

			return targetPlatform;
		}

		public static string GetVisualStudioPlatformName(Project project)
		{
			if (!project.Properties.Contains("config-vs-version"))
			{
				SetConfigVisualStudioVersion.Execute(project);
			}

			var platformname = project.Properties["visualstudio.platform.name"];
			if (String.IsNullOrEmpty(platformname))
			{
				project.Log.Warning.WriteLine("Unable to determine Visual Studio Platform name for configuration '{0}'. Property 'visualstudio.platform.name' is not defined. Will use 'x64' platform.", project.Properties["config"]);
				platformname = "x64";
			}
			return platformname;
		}

		/*
		internal virtual string GetVSConfigurationName(Configuration configuration)
		{
			return String.Format("{0}|{1}", configuration.Name, GetTargetPlatform(configuration));
		} 
		*/
		#endregion Internal Static Methods

		#region Writing Solution

		protected virtual void WriteSolutionPlatformMapping(IMakeWriter writer)
		{
			writer.WriteTabLine("GlobalSection(SolutionConfigurationPlatforms) = preSolution");

			var dups = new HashSet<string>();

			foreach (var config in Configurations)
			{
				var vs_config_platform = String.Format("{0}|{1}", GetSolutionConfigName(config), ConfigToPlaforms[config]);

				if (dups.Add(vs_config_platform))
				{
					writer.WriteTab(String.Empty);
					writer.WriteTabLine("{0} = {0}", vs_config_platform);
				}
			}

			writer.WriteTabLine("EndGlobalSection");
		}

		protected virtual void WriteSolutionFoldersDefinitions(IMakeWriter writer)
		{
			// Write Solution Folders:
			foreach (SolutionFolders.SolutionFolder folder in SolutionFolders.Folders.Values.OrderBy(folder => folder.PathName.Path, StringComparer.InvariantCultureIgnoreCase))
			{
				if (folder.IsEmpty)
				{
					continue;
				}

				writer.WriteLine("Project(\"{0}\") = \"{1}\", \"{2}\", \"{3}\"", ToString(SOLUTION_ITEM_GUID), folder.FolderName, folder.FolderName, ToString(folder.Guid));
				if (folder.FolderFiles.Count > 0)
				{
					writer.WriteTabLine("ProjectSection(SolutionItems) = preProject");
					foreach (var file in folder.FolderFiles)
					{
						//string relativePath = file.Path;
						string relativePath = PathUtil.RelativePath(file.Path, OutputDir.Path, addDot: true);
						writer.WriteTab(String.Empty);
						writer.WriteTabLine("{0} = {0}", relativePath);
					}
					writer.WriteTabLine("EndProjectSection");
				}
				writer.WriteLine("EndProject");
			}
		}

		protected virtual void WriteProject(IMakeWriter writer, VSProjectBase project)
		{
			foreach (SolutionEntry slnEntry in project.SolutionEntries)
			{
				string relativePath = Path.Combine(slnEntry.RelativeDir, slnEntry.ProjectFileName);
				if (!Path.IsPathRooted(relativePath))
				{
					relativePath = "." + Path.DirectorySeparatorChar + relativePath;
				}

				writer.WriteLine(@"Project(""{0}"") = ""{1}"", ""{2}"", ""{3}""",
					ToString(slnEntry.ProjectTypeGuid), slnEntry.Name, relativePath, slnEntry.ProjectGuidString);

				if (project.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.sln.explicit-proj-dependencies", false))
				{
					if (project.Dependents.Count > 0)
					{
						writer.WriteTabLine("ProjectSection(ProjectDependencies) = postProject");
						foreach (var dependent in project.Dependents)
						{
							WriteDependent(writer, dependent as VSProjectBase);
						}
						writer.WriteTabLine("EndProjectSection");
					}
				}
				writer.WriteLine("EndProject");
			}
		}

		protected virtual void WriteDependent(IMakeWriter writer, VSProjectBase project)
		{
			writer.WriteTab(String.Empty);
			writer.WriteTabLine("{0} = {0}", project.ProjectGuidString);
		}

		protected virtual void WriteProjectConfigMapping(IMakeWriter writer, VSProjectBase project)
		{
			//for each global configuration find corresponding project configurations.\
			foreach (SolutionEntry solutionEntry in project.SolutionEntries)
			{
				foreach (Configuration config in Configurations)
				{
					//IMTODO: I can mark wrong name|platform combinations as unbuildable. Now I just map them to correct project configs

					string activeprojectconfig = null;
					if (project.ConfigurationMap.TryGetValue(config, out Configuration active))
					{
						bool entryIsValidForConfig = solutionEntry.ValidConfigs?.Contains(active) ?? true; // null means all configs valid
						if (entryIsValidForConfig)
						{
							activeprojectconfig = project.GetVSProjConfigurationName(active);
							//IMTODO: do better implementation. This looks a bit hacky.
							if (project is ExternalVSProject || project is VSDotNetProject || project.ProjectTypeGuid == VSProjectBase.CSPROJ_GUID)
							{
								activeprojectconfig = activeprojectconfig.Replace("AnyCPU", "Any CPU");
							}
						}
					}
					foreach (string solutionplatform in Platforms)
					{
						var solutionplatformName = (solutionplatform == "AnyCPU") ? "Any CPU" : solutionplatform;

						var solutionConfigPrefix = GetSolutionConfigName(config);

						string solutioncfgName = solutionConfigPrefix + "|" + solutionplatformName;

						if (activeprojectconfig != null)
						{
							string activeprojectplatform = activeprojectconfig.Substring(activeprojectconfig.IndexOf('|') + 1);

							// if this solution 'Config|Platform' is not 'native' for config, and there are other sln configs that map to the same 'Config|Platform' natively, we skip it
							if (solutionplatformName == GetTargetPlatform(config) || false == Configurations.Any(cfg => cfg != config && (solutionConfigPrefix == GetSolutionConfigName(cfg) && solutionplatformName == GetTargetPlatform(cfg))))
							{
								writer.WriteTab(String.Empty);
								writer.WriteTabLine("{0}.{1}.ActiveCfg = {2}", solutionEntry.ProjectGuidString, solutioncfgName, activeprojectconfig);

								if (solutionEntry.ProjectTypeGuid != VSProjectBase.PYPROJ_GUID // python project cannot be enabled for build and deploy in VisualStudio but marking them as such will confuse MSBuild
									&& (activeprojectplatform == solutionplatformName ||    // Adding Build.0 means project selected for build for that config.  If platform doesn't match up, it shouldn't be selected for build.
										(activeprojectplatform == "Any CPU" && (config.System == "pc" || config.System == "pc64")))
									)
								{
									if (solutionEntry.ProjectTypeGuid != VSProjectBase.CSPROJ_GUID || config.System == "pc" || config.System == "pc64")
									{
										writer.WriteTab(String.Empty);
										writer.WriteTabLine("{0}.{1}.Build.0 = {2}", solutionEntry.ProjectGuidString, solutioncfgName, activeprojectconfig);
									}
									if (solutionEntry.DeployConfigs.Contains(active))
									{
										writer.WriteTab(String.Empty);
										writer.WriteTabLine("{0}.{1}.Deploy.0 = {2}", solutionEntry.ProjectGuidString, solutioncfgName, activeprojectconfig);
									}
								}
							}
						}
						else
						{
							//Just select any project config to map as non active. Match with the same config name if possible:
							var projectconfig = project.AllConfigurations.SingleOrDefault(c => c == config);
							if (projectconfig == null)
							{
								projectconfig = project.AllConfigurations.FirstOrDefault(); // TODO potential determinism issue?
								if (projectconfig != null)
								{
									string vsproj_target_config = project.GetVSProjTargetConfiguration(projectconfig);
									string vsproj_target_platform = project.GetProjectTargetPlatform(projectconfig);
									vsproj_target_platform = (vsproj_target_platform == "AnyCPU") ? "Any CPU" : vsproj_target_platform;

									writer.WriteTab(String.Empty);
									writer.WriteTabLine("{0}.{1}.ActiveCfg = {2}|{3}", project.ProjectGuidString, solutioncfgName, vsproj_target_config, vsproj_target_platform);
								}
							}
						}
					}
				}
			}
		}

		protected virtual void WriteSolutionFoldersContent(IMakeWriter writer)
		{

			if (SolutionFolders.Folders.Count > 0)
			{
				writer.WriteTabLine("GlobalSection(NestedProjects) = preSolution");

				foreach (var folder in SolutionFolders.Folders.Values.OrderBy(f => f.PathName.Path, StringComparer.OrdinalIgnoreCase))
				{
					if (folder.IsEmpty)
					{
						continue;
					}

					foreach (VSProjectBase gen in folder.FolderProjects.Values.OrderBy(g => g.Name, StringComparer.OrdinalIgnoreCase))
					{
						foreach (SolutionEntry solutionEntry in gen.SolutionEntries)
						{
							writer.WriteTab(String.Empty);
							writer.WriteTabLine("{0} = {1}", solutionEntry.ProjectGuidString, ToString(folder.Guid));
						}
					}
					foreach (var child in folder.children.Values.OrderBy(c => c.FolderName, StringComparer.OrdinalIgnoreCase))
					{
						if (child.IsEmpty)
						{
							continue;
						}

						writer.WriteTab(String.Empty);
						writer.WriteTabLine("{0} = {1}", ToString(child.Guid), ToString(folder.Guid));
					}
				}
				writer.WriteTabLine("EndGlobalSection");
			}
		}

		#endregion Writing Solution


		#region Abstract Methods

		protected abstract void WriteHeader(IMakeWriter writer);

		protected abstract string TeamTestSchemaVersion
		{
			get;
		}

		#endregion Abstract Methods


		#region Private Fields
		private const string DefaultTargetPlatform = "Win32";
		protected readonly IDictionary<string, string> _targetPlatformMapping = new Dictionary<string, string>();
		private ModuleGenerator _startupModuleGenerator;

		private readonly IDictionary<Configuration, string> _solutionConfigurationNameOverrides;
		#endregion Private Fields

		public readonly SolutionFolders SolutionFolders;

		protected enum VSProjectTypes
		{
			Native,
			Make,
			Utility,
			CSharp,
			Python,

			// .androidproj projects are built by MSVC-android
			Android,

			ExternalProject
		}

		#region Solution Types Guid definitions
		protected static readonly Guid SOLUTION_ITEM_GUID = new Guid("2150E333-8FDC-42A3-9474-1A3956D46DE8");
		#endregion Solution Types Guid definitions

	}

}
