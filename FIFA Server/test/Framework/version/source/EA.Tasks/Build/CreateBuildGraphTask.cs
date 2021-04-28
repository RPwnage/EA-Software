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
using System.Threading;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using NAnt.Core.Util;

using EA.Eaconfig.Build.MergeModules;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;

using Threading = System.Threading.Tasks;
using System.Runtime.ExceptionServices;
using EA.Tasks.Model;

namespace EA.Eaconfig.Build
{

	[TaskName("create-build-graph")]
	public class Task_CreateBuildGraph : Task
	{
		private readonly NAnt.Core.PackageCore.PackageAutoBuildCleanMap ModuleProcessMap = new NAnt.Core.PackageCore.PackageAutoBuildCleanMap();

		[TaskAttribute("build-group-names", Required = true)]
		public string BuildGroupNames { get; set; }

		[TaskAttribute("build-configurations", Required = true)]
		public string BuildConfigurations { get; set; }

		[TaskAttribute("process-generation-data", Required = false)]
		public bool ProcessGenerationData { get; set; } = false;

		protected BuildGraph BuildGraph { get; private set; }

		protected override void ExecuteTask()
		{
			var timer = new Chrono();
			Log.Minimal.WriteLine(LogPrefix + "Creating Build Graph... ");

			BuildGraph = Project.BuildGraph();

			VerifyStructuredXml(BuildGraph.Modules.Values);

			var configs = new HashSet<string>(BuildConfigurations.ToArray());
			var buildgroups = new HashSet<BuildGroups>(BuildGroupNames.ToArray().Select(gn => ConvertUtil.ToEnum<BuildGroups>(gn)));

			if (BuildGraph.IsBuildGraphComplete)
			{
				Log.Info.WriteLine(LogPrefix + "[Packages {0}, modules {1}, active modules {2}] - build graph already completed.", BuildGraph.Packages.Count, BuildGraph.Modules.Count, BuildGraph.SortedActiveModules.Count());
				return;
			}

			BuildGraph.SetConfigurationNames(configs);
			BuildGraph.SetBuildGroups(buildgroups);

			var topmodules = GetTopModules(Project, configs, buildgroups);

			// We store a list of config objects as well, because this give us extra info such as config system, compiler info, etc
			// which is not guarantee to be in the config name itself.
			List<Configuration> configList = new List<Configuration>(topmodules.Select(m => m.Configuration).Distinct());
			BuildGraph.SetConfigurationList(configList);

			ExecuteGlobalProcessBlock(topmodules, "preprocess");

			var activemodules = GetActiveModules(topmodules);
			var sortedActivemodules = SortActiveModulesBottomUp(activemodules).ToList();

			SetBuildGraphActiveModules(topmodules, sortedActivemodules);

			SetBuildGraphDotNetFamily(sortedActivemodules);

			try
			{
				// Note that this is not using ForEachModule since this is really going full parallel which makes a big difference
				NAntUtil.ParallelForEach(sortedActivemodules, (m) =>
				{
					if (!(m is Module_UseDependency))
						SetModuleDataAndPreprocess((ProcessableModule)m);
				});

				// Add dependent public data:
				var propagatedTransitive = new ConcurrentDictionary<string, DependentCollection>();
				HashSet<IModule> topModulesLookUp = new HashSet<IModule>(topmodules);
				
				NAntUtil.ParallelForEach(sortedActivemodules, (m) =>
				{
					if (!(m is Module_UseDependency))
					{
						AddDependentPublicData((ProcessableModule)m, topModulesLookUp, ref propagatedTransitive);
					}
				});

				// calculate copy local files
				CopyLocalProcessor copyLocalfilesProcessor = new CopyLocalProcessor();
				copyLocalfilesProcessor.ProcessAll(Project, sortedActivemodules);

				// calculate addtional runtime dependencies from custom build tools
				NAntUtil.ParallelForEach(sortedActivemodules, (m) =>
				{
					if (!(m is Module_UseDependency))
					{
						ProcessableModule mod = (ProcessableModule)m;
						foreach (BuildTool buildTool in mod.Tools.OfType<BuildTool>()) // custombuildtools
						{
							if (buildTool.OutputDependencies != null && !buildTool.OutputDependencies.FileItems.IsNullOrEmpty())
							{
								foreach (string customOutputPath in buildTool.OutputDependencies.FileItems.Select(item => item.Path.NormalizedPath))
								{
									if (PathUtil.IsPathInDirectory(customOutputPath, mod.OutputDir.NormalizedPath))
									{
										mod.RuntimeDependencyFiles.Add(customOutputPath);
									}
								}
							}
						}
					}
				});

				// run post process
				NAntUtil.ParallelForEach(sortedActivemodules, (m) =>
				{
					if (!(m is Module_UseDependency))
					{
						ProcessableModule mod = (ProcessableModule)m;
						var buildOptionSet = OptionSetUtil.GetOptionSetOrFail(mod.Package.Project, mod.BuildType.Name);
						ExecutePostProcessSteps(mod, buildOptionSet);
					}
				});

				// Add transitive dependencies to module:
				AddTransitiveDepsToModules(activemodules, propagatedTransitive);
			}
			catch (Exception e)
			{
				ThreadUtil.RethrowThreadingException(String.Format("Error in task '{0}'", Name), Location, e);
			}

			ProcessMergeModules(sortedActivemodules);

			ExecuteGlobalProcessBlock(sortedActivemodules, "postprocess");

			// Remove Module.ExcludedFromBuild. This flag can be set in user pre/post process tasks.
			RemoveExcludedFromBuildModules(ref topmodules, ref sortedActivemodules);

			sortedActivemodules = DetectCircularBuildDependencies(Project, sortedActivemodules);

			SetBuildGraphActiveModules(topmodules, sortedActivemodules);

			Log.Info.WriteLine(LogPrefix + "{0}", BuildGraph.BuildGroups.ToString(" "));
			Log.Minimal.WriteLine(LogPrefix + "Finished - {0} [Packages {1}, modules {2}, active modules {3}]", timer.ToString(), BuildGraph.Packages.Count, BuildGraph.Modules.Count, BuildGraph.SortedActiveModules.Count());

			ValidateBuildGraph(sortedActivemodules);
		}

		private void SetModuleDataAndPreprocess(ProcessableModule module)
		{
			//Tiburon sets module build type in the preprocess steps ("vcproj.prebuildtarget"). Verify and reset build type if endded"
			VerifyResetModuleBuildType(module);

			module.SetModuleBuildProperties();

			using (var moduleDataProcessor = new ModuleProcessor_SetModuleData(module, LogPrefix))
			{
				moduleDataProcessor.SetModuleAssetFiles(module);
				ExecutePreProcessSteps(module, moduleDataProcessor.BuildOptionSet);
				moduleDataProcessor.Process(module);
			}

			if (ProcessGenerationData)
			{
				using (var moduleAdditionalDataProcessor = new ModuleAdditionalDataProcessor(module))
				{
					moduleAdditionalDataProcessor.Process(module);
				}
			}
		}

		private void SetBuildGraphDotNetFamily(List<IModule> sortedActivemodules)
		{
			DotNetFrameworkFamilies? buildGraphFamily = null;
			IModule settingModule = null;
			foreach (IModule module in sortedActivemodules.AsEnumerable().Reverse()) // reverse order for a top down scan - makes more sense to user to see conflicts between higher and lower modules
			{
				// get module framework family if applicable
				DotNetFrameworkFamilies? moduleFamily;
				if (Project.NetCoreSupport)
				{
					string moduleFamilyPropertyName = module.PropGroupName("targetframeworkfamily");
					string moduleFamilyPropertyValue = module.Package.Project?.Properties[moduleFamilyPropertyName];
					if (moduleFamilyPropertyValue != null)
					{
						if (Enum.TryParse(moduleFamilyPropertyValue, ignoreCase: true, out DotNetFrameworkFamilies parsedModuleFamily))
						{
							moduleFamily = parsedModuleFamily;
						}
						else
						{
							throw new BuildException($"Module '{module}' has unrecognised value for '{moduleFamilyPropertyName}': {moduleFamilyPropertyValue}. Expected values are: {String.Join(", ", Enum.GetValues(typeof(DotNetFrameworkFamilies)).Cast<DotNetFrameworkFamilies>().Select(e => e.ToString())).ToLowerInvariant()}.");
						}
					}
					else
					{
						string dependentTaskFamilyPropertyName = "dependenttask.dot-net-family";
						string dependentTaskFamilyPropertyValue = module.Package.Project?.Properties[dependentTaskFamilyPropertyName];
						if (dependentTaskFamilyPropertyValue != null)
						{
							if (Enum.TryParse(dependentTaskFamilyPropertyValue, ignoreCase: true, out DotNetFrameworkFamilies parsedLoadPackageFamily))
							{
								moduleFamily = parsedLoadPackageFamily;
							}
							else
							{
								throw new BuildException($"Property '{dependentTaskFamilyPropertyName}' has unrecognised value: {dependentTaskFamilyPropertyValue}. Expected values are: {String.Join(", ", Enum.GetValues(typeof(DotNetFrameworkFamilies)).Cast<DotNetFrameworkFamilies>().Select(e => e.ToString())).ToLowerInvariant()}.");
							}
						}
						else
						{
							moduleFamily = DotNetFrameworkFamilies.Framework;
						}
					}
				}
				else
				{
					moduleFamily = DotNetFrameworkFamilies.Framework;
				}

				// NUGET-TODO:logging
				// set module target family before we set module data
				if (module is Module_DotNet dotNetModule)
				{
					dotNetModule.TargetFrameworkFamily = moduleFamily.Value;
				}
				else if (module is Module_Native nativeModule && module.IsKindOf(Module.Managed))
				{
					nativeModule.TargetFrameworkFamily = moduleFamily.Value;
				}
				else
				{
					continue; // anything else will default to .net Framework - we currently rely on this to set up some properties for resolution in C++ only vcxproj
					// files // NUGET-TODO: refactor this to be more clearly two different things
				}

				// if this is first applicable module, just record family and continue
				if (!buildGraphFamily.HasValue)
				{
					settingModule = module;
					buildGraphFamily = moduleFamily;
					continue;
				}
				
				// validate module is compatible with previous settings
				if (moduleFamily == DotNetFrameworkFamilies.Standard)
				{
					continue; // standard is always acceptable
				}
				else if (buildGraphFamily == DotNetFrameworkFamilies.Standard)
				{
					// upgrade from standard to a more specific .net family
					settingModule = module;
					buildGraphFamily = moduleFamily;
				}
				else if (buildGraphFamily != moduleFamily)
				{
					string settingModuleScriptFile = settingModule.ScriptFile != null ? $" ({settingModule.ScriptFile})" : String.Empty;
					string moduleScriptFile = module.ScriptFile != null ? $" ({module.ScriptFile})" : String.Empty;

					throw new BuildException($"Incompatbile .NET families in buildgraph. " +
						$"Module '{settingModule.Name}'{settingModuleScriptFile} uses .NET {buildGraphFamily} whereas '{module.Name}'{moduleScriptFile} uses .NET {moduleFamily}. " +
						$"Buildgraphs can only contain compatible .NET families.");
				}
			}

			BuildGraph.SetBuildGraphNetFamily(Project, buildGraphFamily);
		}

		private void AddDependentPublicData(ProcessableModule module, HashSet<IModule> topModules, ref ConcurrentDictionary<string, DependentCollection> propagatedTransitive)
		{
			var package = module.Package;

			if (!topModules.Contains(module)) // we don't have publicdata for top modules, as there is nothing depending on them that needs to load it
			{
				using (NAnt.Core.PackageCore.PackageAutoBuildCleanMap.PackageAutoBuildCleanState package_Verify = ModuleProcessMap.StartBuild(package.Key, "Verify Package Public Data" + package.ConfigurationName))
				{
					if (!package_Verify.IsDone())
					{
						ModuleProcessor_AddModuleDependentsPublicData.VerifyPackagePublicData(package as Package, LogPrefix);
					}
				}
			}

			var packagePropagatedTransitive = package.Project.Properties.GetBooleanProperty(Project.NANT_PROPERTY_TRANSITIVE) ? propagatedTransitive : null;

			var buildOptionSet = OptionSetUtil.GetOptionSetOrFail(module.Package.Project, module.BuildType.Name);

			using (var moduleDataProcessor = new ModuleProcessor_AddModuleDependentsPublicData(module, ref packagePropagatedTransitive, LogPrefix))
			{
				moduleDataProcessor.Process(module);
			}
		}

		private static void AddTransitiveDepsToModules(IEnumerable<IModule> modules, ConcurrentDictionary<string, DependentCollection> propagatedTransitive)
		{
			foreach (var module in modules.Where(m => !(m is Module_UseDependency)))
			{
				if (propagatedTransitive.TryGetValue(module.Key, out DependentCollection propagated))
				{
					module.Dependents.AddRange(propagated);
				}
			}
		}

		private void ValidateBuildGraph(IEnumerable<IModule> sortedActiveModules)
		{
			// Using NAntUtil.ForEach<T> wrapper to allow us easily turn off multithreading during debugging.
			NAntUtil.ParallelForEach(sortedActiveModules, module => 
			{
				foreach (Dependency<IModule> dependency in module.Dependents)
				{
					IModule dependentModule = dependency.Dependent;

					if (dependentModule is Module_UseDependency || dependentModule is Module_Utility || dependentModule is Module_ExternalVisualStudio)
					{
						// we simpy can't make useful deductions about use dependencies since we can't know when they produce
						// there outputs - we could validate that their outputs already exist but we might be relying on a
						// custom build or another solution which was not build yet to create them

						// utility modules and external vs modules are the same, they might actually produce a dll / lib / etc 
						// via custom means and we have to accept that
						continue;
					}

					// TODO fix spurious warning during lib only build
					// -dvaliant 2015/05/27
					/* there's an interesting case that can trigger warnings below. If I build lib A, and lib A has a use / build dependency on lib B,
					and lib B has not been previously built AND I'm not building something that ultimately causes the link dependency to be propagated to
					something that links then lib B will only eve be loaded as a Module_UseDependency and we won't know its output library path. However 
					its initialize.xml declares it exports a lib that doesn't exist and it triggers our warning. The warning isn't even incorrect but
					it might confuse people to see warning about lib B not building a lib it does in fact build (when depended on by something that links).
					Code below might help up limit this check to only depending modules that have a link step but even this quite cut it as we also need
					to search propagated dependents in that case (see propagation code)

					bool moduleHasLink = false;
					ProcessableModule processableModule = module as ProcessableModule;
					if (processableModule != null && module.Package.Project != null)
					{
						moduleHasLink = OptionSetUtil.GetOptionSetOrFail(module.Package.Project, processableModule.BuildType.Name).Options["build.tasks"].ToArray().Contains("link") || module.IsKindOf(Module.DotNet);
					}*/

					IPublicData publicData = dependentModule.Public(module);

					// We should skip publicdata validation in cases where the dependencies are between modules in the same package
					// Some package owners refrain from declaring internal modules in publicdata and publicdata is only needed by dependencies between packages
					if (module.Package != dependentModule.Package)
					{
						if (publicData != null)
						{
							// validate libs
							if (dependency.IsKindOf(DependencyTypes.Link))
							{
								string outputLibPath = null;
								if (dependentModule.IsKindOf(Module.Library) && !dependentModule.IsKindOf(Module.Managed))
								{
									outputLibPath = dependentModule.PrimaryOutput();
								}
								else if (dependentModule.IsKindOf(Module.DynamicLibrary) && !dependentModule.IsKindOf(Module.Managed))
								{
									if ((dependentModule as Module_Native).Link.ImportLibFullPath != null) // eaconfig doesn't set up the properties for this on unix, this seems like an oversight but adding null safety here for now
									{
										outputLibPath = (dependentModule as Module_Native).Link.ImportLibFullPath.Path;
									}
								}
								if (outputLibPath != null)
								{
									// Can't do any validation if we don't even setup outputLibPath!
									ValidatePublicFileSet(module, dependentModule, "libs", publicData.Libraries, outputLibPath, Log.WarningId.PublicDataLibMismatch, Log.WarningId.PublicDataLibMissing);
								}
							}

							// validate dlls
							if (dependency.IsKindOf(DependencyTypes.Link))
							{
								string outputDllPath = null;
								if (dependentModule.IsKindOf(Module.DynamicLibrary) && !dependentModule.IsKindOf(Module.Managed) && !dependentModule.IsKindOf(Module.DotNet))
								{
									outputDllPath = dependentModule.PrimaryOutput();
								}
								if (outputDllPath != null)
								{
									// Can't do any validation if we don't even setup outputDllPath!
									ValidatePublicFileSet(module, dependentModule, "dlls", publicData.DynamicLibraries, outputDllPath, Log.WarningId.PublicDataDllMismatch, Log.WarningId.PublicDataDllMissing);
								}
							}

							// validate assemblies
							if (dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
							{
								string outputAssemblyPath = null;
								if ((dependentModule.IsKindOf(Module.Managed) || dependentModule.IsKindOf(Module.DotNet)))
								{
									outputAssemblyPath = dependentModule.PrimaryOutput();
								}

								// In case we still have old manually constructed Initialize.xml (ie without using buildtype to have
								// Framework auto setup assemblies fileset to include programs, we test for it to allow this exception
								// to be skip so that we don't need to force user doing massive changes to build files.
								// We used to not set 'outputAssemblyPath' variable under following conditions.
								bool backwardcompatibility_allowException = dependentModule.IsKindOf(Module.Program) &&
									(dependentModule.IsKindOf(Module.Managed) || dependentModule.IsKindOf(Module.DotNet)) &&
									!publicData.Assemblies.FileItems.Any(); // User didn't setup assemblies fileset for C#/Managed program module!

								if (outputAssemblyPath != null && !backwardcompatibility_allowException)
								{
									// Can't do any validation if we don't even setup outputAssemblyPath!
									ValidatePublicFileSet(module, dependentModule, "assemblies", publicData.Assemblies, outputAssemblyPath, Log.WarningId.PublicDataAssemblyMismatch, Log.WarningId.PublicDataAssemblyMissing);
								}
							}
						}
					}
				}
			});
		}

		private static void ValidatePublicFileSet(IModule module, IModule dependentModule, string fileSetName, FileSet publicFileSet, string outputPath, Log.WarningId mismatchErrorId, Log.WarningId missingErrorId)
		{
			// reporting name for error messages, use dependencies don't load module data so use package name
			string reportingName = "Module " + dependentModule.Package.Name + (dependentModule.BuildGroup == BuildGroups.runtime ? "" : "/" + dependentModule.BuildGroup.ToString()) + '/' + dependentModule.Name;

			// use module's project for reporting so that its warning settings get used i.e 
			// package.blah.nant.warningsupression will be used to supress wanrings about blah's initialize.xml
			// if project is null (use dependency) then fall back to depending module's Project
			Project dependentProject = dependentModule.Package.Project;

			// get package level exports to compare module level exports against
			FileSet packagePublicLibs = (module.Package.Project.NamedFileSets[String.Format("package.{0}.{1}", dependentModule.Package.Name, fileSetName)] ?? new FileSet()).SafeClone();
			packagePublicLibs.FailOnMissingFile = false;

			// check files are exported properly
			bool foundOneFile = false;
			foreach (FileItem fileItem in publicFileSet.FileItems)
			{
				string itemFullPath = PathNormalizer.Normalize(fileItem.FullPath);
				bool fileIsInOutputDir = itemFullPath.StartsWith(dependentModule.OutputDir.Path); // if the path is within the output dir we can be pretty sure it is a file the module is supposed to build
				if (!String.Equals(itemFullPath, outputPath, PathUtil.PathStringComparison) && !fileItem.AsIs && (!File.Exists(itemFullPath) || fileIsInOutputDir))
				{
					// before warning check if this is in package public data - if it is there is a good chance this file is actually from another module
					// in this package but the package doesn't declare module level specifics
					if (packagePublicLibs.FileItems.Any(fi => String.Equals(itemFullPath, PathNormalizer.Normalize(fi.FullPath))))
					{
						// file is exported by package level public data, this is not ideal but not necessarily incorrect
						// only complain if warn level is advise or higher
						dependentProject.Log.ThrowWarning(mismatchErrorId, Log.WarnLevel.Advise, "Package {0} exports '{1}' at package level (package.{0}.{2}). Ideally this should be exported at module level (package.{0}.{3}.{2} or structured xml).", dependentModule.Package.Name, itemFullPath, fileSetName, dependentModule.Name);
					}
					else
					{
						string message = String.Format("{0} exports '{1}' from its initialize.xml but it does not produce the file and ", reportingName, itemFullPath);
						message += (fileIsInOutputDir) ? "the file is an output" : "the file does not exist";
						message += String.Format(" (produces '{0}').", outputPath);
						dependentProject.Log.ThrowWarning(mismatchErrorId, Log.WarnLevel.Normal, message);
					}
				}
				else
				{
					foundOneFile = true;
				}
			}

			// this isn't a perfect check but if we found no files exports, but the package exports a file
			// then something is probably wrong with the initialize.xml
			if (!foundOneFile && outputPath != null)
			{
				dependentProject.Log.ThrowWarning(missingErrorId, Log.WarnLevel.Normal, "{0} is depended on by module {4}/{1} however it does not declare the {2} it builds ('{3}') in its Initialize.xml.", reportingName, module.Name, fileSetName, outputPath, module.Package.Name);
			}
		}

		private void ExecutePreProcessSteps(ProcessableModule module, OptionSet buildOptionSet)
		{
			var prefix = ".vcproj";
			if (module.IsKindOf(Module.CSharp))
			{
				prefix = ".csproj";
			}
			else if (module.IsKindOf(Module.FSharp))
			{
				prefix = ".csproj";
			}

			if (ProcessGenerationData)
			{
				string targetproperty = module.Package.Project.Properties[module.GroupName + prefix + ".prebuildtarget"];
				string targetnames = targetproperty ?? module.GroupName + prefix + ".prebuildtarget";

				foreach (var targetname in targetnames.ToArray())
				{
					module.Package.Project.ExecuteTargetIfExists(targetname, force: false);
				}
			}

			// Collect all preprocess definitions. It can be task or a target.
			string preprocess = buildOptionSet.Options["preprocess"] + Environment.NewLine + module.Package.Project.Properties["eaconfig.preprocess"] + Environment.NewLine +
				(module.Package.Project.Properties[module.GroupName + ".preprocess"] ?? (Project.TargetExists(module.GroupName + ".preprocess") ? module.GroupName + ".preprocess" : String.Empty));

			BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
			if (stepsLog != null)
			{
				stepsLog.Info.WriteLine(LogPrefix + "{0}: executing '{1}' + '{2}' + '{3}' steps", module.ModuleIdentityString(), "preprocess", "eaconfig.preprocess", module.GroupName + ".preprocess");
			}

			module.Package.Project.ExecuteProcessSteps(module, buildOptionSet, preprocess.ToArray(), stepsLog);
		}

		private void ExecutePostProcessSteps(ProcessableModule module, OptionSet buildOptionSet)
		{
			// Collect all postprocess definitions. It can be task or a target.
			string postprocess = buildOptionSet.Options["postprocess"] + Environment.NewLine + module.Package.Project.Properties["eaconfig.postprocess"] + Environment.NewLine +
								 module.Package.Project.Properties[module.GroupName + ".postprocess"];

			BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
			if (stepsLog != null)
			{
				stepsLog.Info.WriteLine(LogPrefix + "{0}: executing '{1}' + '{2}' + '{3}' steps", module.ModuleIdentityString(), "postprocess", "eaconfig.postprocess", module.GroupName + ".postprocess");
			}

			module.Package.Project.ExecuteProcessSteps(module, buildOptionSet, postprocess.ToArray(), stepsLog);
		}

		private void VerifyResetModuleBuildType(ProcessableModule module)
		{
			BuildType buildtype = module.Package.Project.CreateBuildType(module.GroupName);
			if (buildtype.Name != module.BuildType.Name)
			{
				string msg = String.Format("package {0}-{1} module {2} ({3}): buildtype changed from {4} to {5} in package/module preprocess steps(targets). ", module.Package.Name, module.Package.Version, module.GroupName, module.Package.ConfigurationName, module.BuildType.Name, buildtype.Name);

				if (buildtype.IsCLR != module.BuildType.IsCLR || buildtype.IsMakeStyle != module.BuildType.IsMakeStyle)
				{
					throw new BuildException(msg + "Framework can not recover from this change. Make sure property '" + module.GroupName + ".buildtype' is fully defined when package build script is loaded.");
				}
				else
				{
					module.Package.Project.Log.Warning.WriteLine(LogPrefix + msg + "Framework autorecovered, but it is advisable to make property '" + module.GroupName + ".buildtype' fully defined when package build script is loaded.");
					module.BuildType = buildtype;
				}
			}
		}

		private IEnumerable<IModule> GetTopModules(Project project, HashSet<string> configNames, HashSet<BuildGroups> buildGroups)
		{
			string packagename = project.Properties["package.name"];
			string packageversion = project.Properties["package.version"];

			var topModules = new List<IModule>();

			bool foundPackage = false;
			foreach (var p in BuildGraph.Packages)
			{
				if (p.Value.Name != packagename || p.Value.Version != packageversion || !configNames.Contains(p.Value.ConfigurationName))
					continue;

				foundPackage = true;

				foreach (var m in p.Value.Modules)
					if (buildGroups.Contains(m.BuildGroup))
						topModules.Add(m);
			}

			if (!foundPackage)
			{
				project.Log.Warning.WriteLine(LogPrefix + "Found no packages that match condition [name={0}, version={1}, configurations={2}]. Total packages processed: {3}",
					packagename, packageversion, configNames.ToString(";"), BuildGraph.Packages.Count);
			}

			return topModules;
		}

		private class ProcessModulesInfo
		{
			public ProcessModulesInfo(IEnumerable<IModule> topmodules, bool collectCustomDependencyTasks)
			{
				Queue = new ConcurrentQueue<IModule>(topmodules);
				Queued = Queue.Count;
				Processed = 0;
				CollectCustomDependencyTasks = collectCustomDependencyTasks;
			}
			public ConcurrentQueue<IModule> Queue;
			public long Queued;
			public long Processed;
			public bool CollectCustomDependencyTasks;
			public Exception Exception;
		}

		private IEnumerable<IModule> GetActiveModules(IEnumerable<IModule> topmodules)
		{
#if NANT_CONCURRENT
			// Under mono most parallel execution is slower than consecutive. Until this is resolved use consecutive execution 
			bool parallel = (PlatformUtil.IsMonoRuntime == false) && (Project.NoParallelNant == false);
#else
			bool parallel = false;
#endif

			ConcurrentDictionary<string, IModule> uniques = new ConcurrentDictionary<string, IModule>(topmodules.Select((m) => new KeyValuePair<string, IModule>(m.Key, m)));

			// Some modules can have custom dependency tasks where the dependency is calculated using the dependency graph.
			var customDependencyTasks = new List<KeyValuePair<ProcessableModule, string>>();

			// This function is used to process modules and calculate their dependencies.. 
			Action<ProcessModulesInfo> processModules = (processInfo) =>
			{
				try
				{
					while (processInfo.Exception == null)
					{
						long p = Interlocked.Read(ref processInfo.Processed);
						long q = Interlocked.Read(ref processInfo.Queued);
						if (p == q)
							break;

						IModule m;
						if (!processInfo.Queue.TryDequeue(out m))
							continue;

						var module = m as ProcessableModule;
						if (module != null && !(module is Module_UseDependency))
						{
							string customTask = processInfo.CollectCustomDependencyTasks ? module.Package.Project.Properties[PropertyKey.Create("package.", module.Package.Name, ".", module.Name, ".customdependencytask")] : null;
							if (customTask != null)
							{
								lock (customDependencyTasks)
									customDependencyTasks.Add(new KeyValuePair<ProcessableModule, string>(module, customTask));
							}
							else
							{
								using (var moduleProcessor = new ModuleProcessor_ProcessDependencies(module, LogPrefix))
								{
									moduleProcessor.Process(module);
								}

								foreach (var d in module.Dependents)
									if (uniques.TryAdd(d.Dependent.Key, d.Dependent))
									{
										Interlocked.Increment(ref processInfo.Queued);
										processInfo.Queue.Enqueue(d.Dependent);
									}
							}
						}

						Interlocked.Increment(ref processInfo.Processed);
					}
				}
				catch (Exception ex)
				{
					processInfo.Exception = ex;
				}
			};

			int parallelTaskCount = parallel ? Environment.ProcessorCount - 1 : 0;
			{
				// Need to create new ProcessModulesInfo since the old one can still be in flight because of the tasks above we don't wait on

				var processInfo = new ProcessModulesInfo(topmodules, true);

				// If parallel, we kick of helper tasks to do the work
				for (int i = 0; i != parallelTaskCount; ++i)
					System.Threading.Tasks.Task.Run(() => processModules(processInfo));

				// Do as much work as possible, skip waiting for kicked off tasks since we might be done before they even start.
				processModules(processInfo);

				if (processInfo.Exception != null)
					throw processInfo.Exception;
			}

			// If we found modules with custom dependency tasks we've deferred their calculations to here where we will run tasks first and then calculate dependencies
			if (customDependencyTasks.Any())
			{
				var processInfo = new ProcessModulesInfo(Enumerable.Empty<IModule>(), false);
				BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
				foreach (var task in customDependencyTasks)
				{
					task.Key.Package.Project.ExecuteGlobalProjectSteps(task.Value.ToArray().ToList(), stepsLog);
					processInfo.Queue.Enqueue(task.Key);
					++processInfo.Queued;
				}

				for (int i = 0; i != parallelTaskCount; ++i)
					System.Threading.Tasks.Task.Run(() => processModules(processInfo));
				processModules(processInfo);

				if (processInfo.Exception != null)
					ExceptionDispatchInfo.Capture(processInfo.Exception).Throw();
			}

			return uniques.Values;
		}

		private IEnumerable<IModule> SortActiveModulesBottomUp(IEnumerable<IModule> unsortedModules)
		{
			// Create DAG of build modules:
			var unsortedKeys = new Dictionary<string, DAGNode<IModule>>();
			foreach (var module in unsortedModules)
				unsortedKeys.Add(module.Key, new DAGNode<IModule>(module));

			var unsorted = new List<DAGNode<IModule>>(unsortedKeys.Values);
			unsorted.Sort((a, b) => { return String.CompareOrdinal(a.Value.Key, b.Value.Key); });

			// Populate dependencies
			foreach (var dagNode in unsorted)
			{
				// Sort dependents so that we always have the same order after sorting the whole graph
				foreach (var dep in dagNode.Value.Dependents)
				{
					if (!dep.IsKindOf(DependencyTypes.Build | DependencyTypes.Link))
						continue;

					DAGNode<IModule> childDagNode;
					if (unsortedKeys.TryGetValue(dep.Dependent.Key, out childDagNode))
					{
						dagNode.Dependencies.Add(childDagNode);
					}
					else
					{
						throw new BuildException("INTERNAL ERROR");
					}
				}

				// Pre-sort to get some sort of determinism
				dagNode.Dependencies.Sort((a, b) => { return String.CompareOrdinal(a.Value.Key, b.Value.Key); });
			}

			var sorted = DAGNode<IModule>.Sort(unsorted, (a, b, path) => { });

			return SetGraphOrder(sorted);
		}

		private void ExecuteGlobalProcessBlock(IEnumerable<IModule> sortedActiveModules, string type)
		{
			var sortedActivePackages = sortedActiveModules.Select(m => m.Package).Distinct().ToList();
			foreach (var package in sortedActivePackages)
			{
				if (package.Project != null && package.Project.Properties.Contains("builgraph.global." + type))
				{
					Project.Log.ThrowWarning(Log.WarningId.SyntaxError, Log.WarnLevel.Normal, "Correct property name from builgraph.global.{0} to buildgraph.global.{0}", type);
					break;
				}
			}

			NAnt.Core.Tasks.AsyncStatus.WaitAt beforewait = NAnt.Core.Tasks.AsyncStatus.WaitAt.None;
			NAnt.Core.Tasks.AsyncStatus.WaitAt afterwait = NAnt.Core.Tasks.AsyncStatus.WaitAt.None;

			if (type == "preprocess")
			{
				beforewait = NAnt.Core.Tasks.AsyncStatus.WaitAt.BeforeGlobalPreprocess;
				afterwait = NAnt.Core.Tasks.AsyncStatus.WaitAt.AfterGlobalPreprocess;
			}
			else if (type == "postprocess")
			{
				beforewait = NAnt.Core.Tasks.AsyncStatus.WaitAt.BeforeGlobalPostprocess;
				afterwait = NAnt.Core.Tasks.AsyncStatus.WaitAt.AfterGlobalPostprocess;
			}
			else
			{
				throw new BuildException("INTERNAL ERROR: invalid parameter to ExecuteGlobalProcessBlock(" + type + ").");
			}

			NAnt.Core.Tasks.AsyncStatus.WaitForAsyncTasks(beforewait);

			var globalpreprocesstimer = new Chrono();
			int globalpreprocesscount = 0;

			globalpreprocesscount += ExecuteGlobalProcessSteps(sortedActivePackages, "builgraph.global." + type);
			globalpreprocesscount += ExecuteGlobalProcessSteps(sortedActivePackages, "buildgraph.global." + type);

			if (globalpreprocesscount > 0)
			{
				Log.Status.WriteLine(LogPrefix + "Global {0} {1} step(s) {2}", type, globalpreprocesscount, globalpreprocesstimer.ToString());
			}

			NAnt.Core.Tasks.AsyncStatus.WaitForAsyncTasks(afterwait);
		}

		private int ExecuteGlobalProcessSteps(IEnumerable<IPackage> sortedActivePackages, string propertyName)
		{
			BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
			int count = 0;
			if (stepsLog != null)
			{
				stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' steps", propertyName);
			}

			var procesed = new HashSet<string>();

			foreach (var package in sortedActivePackages)
			{
				if (package.Project != null)
				{
					var steps = new HashSet<string>(package.Project.Properties[propertyName].ToArray());

					count += package.Project.ExecuteGlobalProjectSteps(steps.Except(procesed), stepsLog);

					procesed.UnionWith(steps);
				}
			}
			return count;
		}

		// Only added for backward compatibility with XcodeProjectizer
		public static IEnumerable<IModule> DetectCircularBuildDependencies(Project project, IEnumerable<IModule> unsortedModules, bool ignore_dependents = false)
		{
			return DetectCircularBuildDependencies(project, unsortedModules.ToList(), ignore_dependents);
		}

		public static List<IModule> DetectCircularBuildDependencies(Project project, List<IModule> unsortedModules, bool ignore_dependents = false)
		{
			// Create DAG of build modules:
			var unsortedKeys = new Dictionary<string, DAGNode<IModule>>(unsortedModules.Count);

			foreach (var module in unsortedModules)
				unsortedKeys.Add(module.Key, new DAGNode<IModule>(module));

			var unsorted = new List<DAGNode<IModule>>(unsortedKeys.Values);
			unsorted.Sort((a, b) => { return String.CompareOrdinal(a.Value.Key, b.Value.Key); });

			// Populate dependencies
			var added = new List<DAGNode<IModule>>();
			foreach (var dagNode in unsorted)
			{
				// Sort dependents so that we always have the same order after sorting the whole graph
				foreach (var dep in dagNode.Value.Dependents)
				{
					if (!dep.IsKindOf(DependencyTypes.Build))
						continue;

					DAGNode<IModule> childDagNode;
					if (unsortedKeys.TryGetValue(dep.Dependent.Key, out childDagNode))
					{
						dagNode.Dependencies.Add(childDagNode);
					}
					else if (ignore_dependents == false)
					{
						project.Log.Status.WriteLine("[create-build-graph][DetectCircularBuildDependencies] Added new module: {0} ({1}) Module '{2}.{3}', through dependency from [{4}] {5}.{6} ({7})",
							dep.Dependent.Package.Name, dep.Dependent.Configuration.Name, dep.Dependent.BuildGroup.ToString(), dep.Dependent.Name, dagNode.Value.Package.Name, dagNode.Value.BuildGroup.ToString(), dagNode.Value.Name, dagNode.Value.Configuration.Name);
						var newNode = new DAGNode<IModule>(dep.Dependent);
						unsortedKeys.Add(dep.Dependent.Key, newNode);
						dagNode.Dependencies.Add(newNode);
						added.Add(newNode);
					}
				}
				// Pre-sort to get some sort of determinism
				dagNode.Dependencies.Sort((a, b) => { return String.CompareOrdinal(a.Value.Key, b.Value.Key); });
			}

			unsorted.AddRange(added);

			var sorted = DAGNode<IModule>.Sort(unsorted, (a, b, path) =>
			{
				StringBuilder exceptionBuilder = new StringBuilder();

				exceptionBuilder.AppendFormat("Circular build dependency between modules {0} ({1}) and {2} ({3})", a.Value.Name, a.Value.Configuration.Name, b.Value.Name, b.Value.Configuration.Name).AppendLine().AppendLine();
				exceptionBuilder.AppendLine("Path:");

				foreach (var node in path)
				{
					exceptionBuilder.Append("    -> ");
					exceptionBuilder.AppendLine(node.Value.Name);
				}

				throw new BuildException(exceptionBuilder.ToString());
			});

			return SetGraphOrder(sorted).ToList();
		}

		private static IEnumerable<IModule> SetGraphOrder(IEnumerable<DAGNode<IModule>> sorted)
		{
			return sorted.Select((node, idx) =>
			{
				(node.Value as ProcessableModule).GraphOrder = idx;
				return node.Value;
			});
		}

		private void ProcessMergeModules(IEnumerable<IModule> buildmodules)
		{
			if (ProcessGenerationData)
			{
				MergeModules_Invoker.MergeModules(Project, buildmodules);
			}
		}

		private void RemoveExcludedFromBuildModules(ref IEnumerable<IModule> topmodules, ref List<IModule> sortedActivemodules)
		{
			topmodules = topmodules.Where(m => !m.IsKindOf(Module.ExcludedFromBuild));
			sortedActivemodules = sortedActivemodules.Where(m => !m.IsKindOf(Module.ExcludedFromBuild)).ToList();

			foreach (var mod in sortedActivemodules)
			{
				mod.Dependents.RemoveIf(dep => dep.Dependent.IsKindOf(Module.ExcludedFromBuild));
			}
		}

		private void SetBuildGraphActiveModules(IEnumerable<IModule> topmodules, List<IModule> activemodules)
		{
			BuildGraph.SetTopModules(topmodules.OrderBy(m => (m as ProcessableModule).GraphOrder).ToList());
			BuildGraph.SetSortedActiveModules(activemodules);
		}

		private void VerifyStructuredXml(IEnumerable<IModule> buildmodules)
		{
			foreach (Module module in buildmodules)
			{
				if (module.Package.Project != null)
				{
					module.Package.Project.Log.ThrowWarning
					(
						Log.WarningId.StructuredXmlNotUsed, Log.WarnLevel.Advise,
						"Structured XML has not been used for module '{0}' in package '{1}'.", module.GroupName, module.Package.Name
					);
				}
			}
		}
	}
}
