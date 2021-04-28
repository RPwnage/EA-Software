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
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

using PackageMap = NAnt.Core.PackageCore.PackageMap;

namespace EA.Eaconfig.Build
{
	[TaskName("load-package")]
	public class LoadPackageTask : Task
	{
		[TaskAttribute("build-group-names", Required = true)]
		public string BuildGroupNames { get; set; }

		[TaskAttribute("autobuild-target", Required = false)]
		public string AutobuildTarget { get; set; } = null;

		[TaskAttribute("process-generation-data", Required = false)]
		public bool ProcessGenerationData { get; set; } = false;

		private List<string> m_processedModuleDependentsList = new List<string>();

		protected override void ExecuteTask()
		{
			if (!Project.TryGetPackage(out IPackage package))
			{
				throw new BuildException(String.Format("Can't find package definition. Make sure package build file '{0}' has <package> task defined.", Project.BuildFileLocalName), this.Location);
			}

			// using a DoOnce since every config after first will account for almost no time
			Chrono timer = new Chrono();
            bool showTimingResult = false;
            DoOnce.Execute(Project, String.Format("LoadPackageTask-{0}", package.Name), () =>
            {
                showTimingResult = Project.Properties.GetBooleanProperty(Project.NANT_PROPERTY_PROJECT_ISTOPLEVEL);
            });
			if (showTimingResult)
			{
                Log.Minimal.WriteLine("{0}Loading package dependencies of {1}... ", LogPrefix, package.Name);
			}

			// calling GetPackageDependentProcessingStatus will create a new entry in a shared map of package load statuses,
			// if one does not already exist, at that start of this LoadPackageTask we are almost guranteed to be creating a new
			// entry
			BuildGraph.PackageDependentsProcessingStatus processingStatus = Project.BuildGraph().GetPackageDependentProcessingStatus(package);

			// assign build style and auto build target, normally these are assigned when this package is fully loaded (follow call tree down
			// from LoadUnprocessedModuledependents at the end of this function) however there is a case where they can be read before they are
			// correctly assigned unless we assign them here
			//
			// this happens if our top level package has a cyclic *package* dependency (not a cyclic *module* dependency) with itself for example:

			// Package A (top):  ModuleA1		ModuleA2
			//						^				|
			// Package B:			|--- ModuleB <---
			//
			// in this scenario while constructing ModuleB's definition, we evaluate ModuleB's dependency on the *package* Package A, and try to access
			// it's processing status entry however we ended up at ModuleB because we were in the middle of loading PackageA to begin with, because these
			// variables are not yet assigned when we access it trying to use them causes logic bugs, assigning them here prevents these
			processingStatus.CurrentTargetStyle = Project.TargetStyle;
			processingStatus.AutobuildTarget = AutobuildTarget;

			if (Project.TargetStyle != Project.TargetStyleType.Clean)
			{
				ExecutePackagePreProcessSteps(package);
			}

			string restoreBuildGroupTo = Properties["eaconfig.build.group"];
			try
			{
				foreach (var buildgroup in GetBuildGroups())
				{
					Properties["eaconfig.build.group"] = buildgroup.ToString();

					string modules = Properties[buildgroup + ".buildmodules"];

					//IMTODO: Since I moved to processing in several passes and Project instances are stored in the Package
					// I can move module creation code to the Dependencies process. This will reduce duplication and 
					// make the code simpler and more flexible 

					if (String.IsNullOrEmpty(modules))
					{
						AddPackageWithoutModules(package, buildgroup);
					}
					else
					{
						AddPackageWithModules(package, modules, buildgroup);
					}
				}
			}
			finally
			{
				if (restoreBuildGroupTo != null)
				{
					Properties["eaconfig.build.group"] = restoreBuildGroupTo;
				}

				((Package)package).SetType(Package.FinishedLoading);
			}

			BuildGraph.PackageDependentsProcessingStatus packageDependentProcessingStatus = Project.BuildGraph().GetPackageDependentProcessingStatus(package);

			foreach (string key in packageDependentProcessingStatus.GetUnProcessedModules(m_processedModuleDependentsList, Project.TargetStyle, AutobuildTarget, GetModulesWithDependencies))
			{
				if (package.Modules.Where(m => m.BuildGroup + ":" + m.Name == key).FirstOrDefault() is ProcessableModule module)
				{
					ProcessModule(module);
				}
			}

			CheckPublicDataModulesExist(package);

			if (showTimingResult)
            {
                Log.Minimal.WriteLine(LogPrefix + "Finished Loading Packages {0}", timer.ToString());
            }
            else
            {
                if (timer.GetElapsed().TotalMilliseconds > 500)
                    Log.Status.WriteLine(LogPrefix + "Load of package '" + package.Name + "' Took a Long time! {0} - Investigate?", timer.ToString());
            }
		}

		/// <summary>
		/// Ensure that all modules defined in the initialize.xml file are also defined in the build files. If not also check to see if perhaps the build group was declared incorrectly.
		/// This is to help catch a simple mistake that would otherwise fail silently and be hard to figure out.
		/// </summary>
		private void CheckPublicDataModulesExist(IPackage package)
		{
			foreach (var buildgroup in GetBuildGroups())
			{
				string[] publicBuildModules = Properties["package." + package.Name + "." + buildgroup + ".buildmodules"]?.Split(new char[] { '\n', '\r' }, StringSplitOptions.RemoveEmptyEntries);
				if (publicBuildModules == null) continue;
				
				foreach (string buildModule in publicBuildModules)
				{
					if (!package.Modules.Where(x => x.BuildGroup == buildgroup && x.Name == buildModule).Any())
					{
						List<string> alternateGroupModules = package.Modules.Where(x => x.Name == buildModule).Select(x => x.BuildGroup + "." + x.Name).ToList();
						if (alternateGroupModules.Count() > 0)
						{
							Log.ThrowWarning(Log.WarningId.ModuleInInitializeButNotBuildScript, Log.WarnLevel.Normal,
								"Public data declares a module '{0}.{1}' but no such module was defined in any of the build files, " +
								"However the following modules with the same name were found under other build groups: {2}", buildgroup, buildModule, string.Join(",", alternateGroupModules));
						}
						else if (Log.WarningLevel >= Log.WarnLevel.Advise) 
						{
							// the more general situation, where it is not likely a incorrect build group, is far too common right now to make a warning as error so it is just an advise level warning
							Log.Warning.WriteLine("Public data declares a module '{0}.{1}' but no such module was defined in any of the build files.", buildgroup, buildModule);
						}
					}
				}
			}
		}

		private void AddPackageWithModules(IPackage package, string modules, BuildGroups buildgroup)
		{
			IList<string> moduleList = modules.ToArray();

			string packageBuildTypePropertyName = buildgroup + ".buildtype";
			if (Properties.Contains(packageBuildTypePropertyName))
			{
				string extraDetails = "";
				// this if check is backwards compatibility to deal with large number of these cases in the wild:
				/*
					<package  name="MyThing"/>
					<property name="runtime.buildmodules" value="MyThing" />
					<property name="runtime.buildtype" value="Library"/> <!-- should be 'runtime.MyThing.buildtype' -->
				*/
				if (moduleList.Count() == 1 && moduleList.First() == package.Name)
				{
					string moduleBuildTypePropertyName = $"{buildgroup}.{package.Name}.buildtype";
					if (!Properties.Contains(moduleBuildTypePropertyName))
					{
						Properties[moduleBuildTypePropertyName] = Properties[packageBuildTypePropertyName];
						extraDetails = $" Given this package only specifies a single module with the same name as the package, {moduleBuildTypePropertyName} has been automatically set to have the same value as {packageBuildTypePropertyName} ({Properties[packageBuildTypePropertyName]}).";
					}
				}

				Log.ThrowWarning(Log.WarningId.GroupBuildTypeSpecifiedWithModules, Log.WarnLevel.Normal, "Property '{0}' specified in package {1} ({2}) - however this package specifies modules. Buildtypes should be set per module via property '{3}.<module name>.buildtype'. " +
				"Note updating to structured XML syntax would handle this automatically.{4}",
				packageBuildTypePropertyName, package.Name, package.ConfigurationName, buildgroup, extraDetails);
			}

			var buildModules = GetmodulesWithDependencies(LogPrefix, Project, buildgroup, moduleList, package.Name, hasmodules: true);

			var constraints = package.Project.GetConstraints();

			IEnumerable<BuildModuleDef> constrainedModules = constraints == null ? null : LoadPackageTask.GetmodulesWithDependencies(LogPrefix, package.Project, buildgroup, constraints.Select(c => c.ModuleName).ToList(), package.Name, hasmodules: true);

			bool processedmodules = false;

			foreach (var module in buildModules)
			{
				string groupsourcedir = Path.Combine(Properties["eaconfig." + module.BuildGroup + ".sourcedir"], module.ModuleName); // DAVE-FUTURE-REFACTOR-TODO - get rid of group source dir

				if (buildgroup != module.BuildGroup && module.Groupname == module.BuildGroup.ToString())
				{
					groupsourcedir = Properties["eaconfig." + module.BuildGroup + ".sourcedir"];
				}

				IModule moduleInstance = AddModule(Project, package, module.BuildGroup, module.ModuleName, module.Groupname, groupsourcedir);
				if (constrainedModules == null || constrainedModules.Any(c => c.BuildGroup == moduleInstance.BuildGroup && c.ModuleName == moduleInstance.Name))
				{
					ProcessModule(moduleInstance);
				}
				CheckBuildMessagesTask.Execute(Project, module.BuildGroup.ToString(), module.ModuleName);
				processedmodules = true;
			}

			if (!processedmodules)
			{
				CheckBuildMessagesTask.Execute(Project, buildgroup.ToString());
			}

		}

		private void AddPackageWithoutModules(IPackage package, BuildGroups buildgroup)
		{
			// If <group>.buildtype isn't defined then we assume there are
			// no modules in that build group.  Before this check was added
			// there was no way to specify that there were 0 modules in a
			// particular group - it was always assumed that there was at
			// least one module in each group, with default source filesets
			// and properties.  This would lead to build failures when calling
			// the build targets for groups containing 0 modules.  This check
			// provides an explicit way to check for (and ignore) 0 module groups.

			// We also check for <group>.builddependencies, for backwards
			// compatibility.  There are several instances of end-users setting
			// up build files which are really programs in disguise, so we
			// look for <group>.builddependencies to catch that case.
			bool processedmodules = false;

			if (Properties.Contains(buildgroup + ".buildtype"))
			{
				string packagename = Project.Properties["package.name"];

				Log.ThrowDeprecation
				(
					Log.DeprecationId.ModulelessPackage, Log.DeprecateLevel.Advise,
					new string[] { packagename }, // throw once per package
					"Package '{0}' does not define property '{1}' but does define '{2}'. This package will automatically create a module with the same name as the package " +
					"but this behaviour may not be supported in future versions. It is highly recommended you convert your package to declare a module explicitly (ideally " +
					"using structured syntax).",
					packagename, buildgroup + ".buildmodules", buildgroup + ".buildtype"
				);

				var buildModules = GetmodulesWithDependencies(LogPrefix, Project, buildgroup, String.Empty.ToArray(), packagename, hasmodules: false);

				foreach (var module in buildModules)
				{
					string groupsourcedir = Path.Combine(Properties["eaconfig." + module.BuildGroup + ".sourcedir"], module.ModuleName);

					if (module.Groupname == module.BuildGroup.ToString())
					{
						groupsourcedir = Properties["eaconfig." + module.BuildGroup + ".sourcedir"];
					}

					IModule moduleInstance = AddModule(Project, package, module.BuildGroup, module.ModuleName, module.Groupname, groupsourcedir);
					ProcessModule(moduleInstance);

					CheckBuildMessagesTask.Execute(Project, module.BuildGroup.ToString(), module.ModuleName);
					processedmodules = true;
				}
			}

			if (!processedmodules)
			{
				CheckBuildMessagesTask.Execute(Project, buildgroup.ToString());
			}
		}

		private IEnumerable<BuildGroups> GetBuildGroups()
		{
			var buildgroups = new List<BuildGroups>();

			foreach (var groupstr in BuildGroupNames.ToArray())
			{
				buildgroups.Add(ConvertUtil.ToEnum<BuildGroups>(groupstr));
			}
			// Add additional groups that can be specified by module constraints.
			var constraints = Project.GetConstraints();
			if (constraints != null)
			{
				buildgroups.AddRange(constraints.Select(c => c.Group));
			}
			return buildgroups.OrderedDistinct();
		}

		private static IModule AddModule(Project project, IPackage package, BuildGroups buildgroup, String modulename, string groupname, string groupsourcedir)
		{
			IModule module = CreateModule(project, package, modulename, buildgroup, groupname, groupsourcedir);
			return module;
		}

		private void ProcessModule(IModule module)
		{
			using (var moduleProcessor = new ModuleProcessor_LoadPackage(module as ProcessableModule, AutobuildTarget, Project.TargetStyle, LogPrefix))
			{
				moduleProcessor.Process(module as ProcessableModule);
				m_processedModuleDependentsList.Add(module.BuildGroup + ":" + module.Name);
			}			
		}

		private static IModule CreateModule(Project project, IPackage package, string modulename, BuildGroups buildgroup, string groupname, string groupsourcedir)
		{
			BuildType buildtype = project.CreateBuildType(groupname);
			var scriptfile = project.Properties[groupname + ".scriptfile"].TrimWhiteSpace();

			// When targets are chained "build test-build . . " there may be residual UseDependency module. Instead of GetOrAdd use AddOrUpdate here, and overwrite UseDep module
			IModule module = project.BuildGraph().Modules.AddOrUpdate(Module.MakeKey(modulename, buildgroup, package),
				key => ModuleFactory.CreateModule(modulename, groupname, groupsourcedir, CreateConfiguration(project), buildgroup, buildtype, package),
				(key, mod) => (mod is Module_UseDependency) ? ModuleFactory.CreateModule(modulename, groupname, groupsourcedir, CreateConfiguration(project), buildgroup, buildtype, package) : mod);

			if (!String.IsNullOrEmpty(scriptfile))
			{
				module.ScriptFile = PathString.MakeNormalized(scriptfile);
			}
			((ProcessableModule)module).SetModuleBuildProperties();
					

			return module;
		}

		private void ExecutePackagePreProcessSteps(IPackage package)
		{
			BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
			if (stepsLog != null)
			{
				stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' steps", "package.preprocess");
			}

			package.Project.ExecuteGlobalProjectSteps(package.Project.Properties["package.preprocess"].ToArray(), stepsLog);

			if (ProcessGenerationData)
			{
				package.Project.ExecuteTargetIfExists(package.Project.Properties["vcproj.prebuildtarget"], force: false);
				package.Project.ExecuteTargetIfExists(package.Project.Properties["csproj.prebuildtarget"], force: false);
				package.Project.ExecuteTargetIfExists(package.Project.Properties["fsproj.prebuildtarget"], force: false);
			}
		}

		public static Configuration CreateConfiguration(Project project)
		{
			string config = project.Properties.GetPropertyOrFail("config");
			string configSystem = project.Properties.GetPropertyOrFail("config-system");
			string configCompiler = project.Properties.GetPropertyOrFail("config-compiler");
			string configPlatform = project.Properties.GetPropertyOrFail("config-platform");
			string configName = project.Properties.GetPropertyOrFail("config-name");
			// Not all configs has config-processor (especially when we are dealing with legacy configs).  Empty string will
			// be used for unsupported configs.
			string configProcessor = project.Properties.GetPropertyOrDefault("config-processor", "");

			return new Configuration
			(
				config,
				configSystem,
				configCompiler,
				configPlatform,
				configName,
				configProcessor
			);
		}

		internal static IEnumerable<BuildModuleDef> GetmodulesWithDependencies(string LogPrefix, Project project, BuildGroups group, ICollection<string> modulenames, string packageName, bool hasmodules)
		{
			ISet<BuildModuleDef> dependencies = new HashSet<BuildModuleDef>();
			Stack<BuildModuleDef> stack = new Stack<BuildModuleDef>();

			if (hasmodules)
			{
				foreach (var modulename in modulenames)
				{
					var entry = new BuildModuleDef(modulename, group, group + "." + modulename);
					stack.Push(entry);
					dependencies.Add(entry);					
				}
			}
			else
			{
				var entry = new BuildModuleDef(packageName, group, Enum.GetName(typeof(BuildGroups), group));
				stack.Push(entry);
				dependencies.Add(entry);
			}

			string configSystem = project.Properties["config-system"];

			while (stack.Count > 0)
			{
				var moduledef = stack.Pop();

				foreach (BuildGroups dependentGroup in Enum.GetValues(typeof(BuildGroups)))
				{
					bool needRuntimeModuleDependents = true;
					string depPropPrefix = moduledef.BuildGroup + "." + moduledef.ModuleName + "." + dependentGroup;

					// Collect all dependencies that are build or autobuilduse type:
					foreach (var v in Mappings.ModuleDependencyPropertyMapping)
					{
						if (((v.Value & DependencyTypes.Build) == DependencyTypes.Build) || ((v.Value & DependencyTypes.AutoBuildUse) == DependencyTypes.AutoBuildUse))
						{
							string depPropName = depPropPrefix + v.Key;
							string depPropNameConfig = depPropName + "." + configSystem;

							var depPropVal = project.Properties[depPropName];
							var depPropValConfig = project.Properties[depPropNameConfig];

							if (modulenames.Count <= 1 && depPropVal == null && depPropValConfig == null)
							{
								//There is alternative format when one module only is defined. Convert property to standard format, 
								// so that we don't have to deal with these differences later.

								var depPropNameLegacy = moduledef.BuildGroup + "." + dependentGroup + v.Key;
								var depPropValLegacy = project.Properties[depPropNameLegacy];

								if (depPropValLegacy != null)
								{
									project.Properties[depPropName] = depPropVal = depPropValLegacy;
								}
							}

							// Automatically add dependency of other groups on runtime (except of the tool group) in case dependencies were not defined explicitly

							if (dependentGroup == BuildGroups.runtime && moduledef.BuildGroup != dependentGroup)
							{
								if (depPropVal != null || depPropValConfig != null)
								{
									needRuntimeModuleDependents = false;
								}
							}

							var moduledependents = String.Concat(depPropVal, Environment.NewLine, depPropValConfig).ToArray();

							foreach (var depmodule in moduledependents)
							{
								string dep_buildmodules = project.Properties[dependentGroup + ".buildmodules"];

								//Dependent group with modules
								if (dep_buildmodules != null)
								{
									var dep_buildmodules_list = StringExtensions.ToArray(dep_buildmodules);
									if (dep_buildmodules_list.Contains(depmodule))
									{
										var entry = new BuildModuleDef(depmodule, dependentGroup, dependentGroup + "." + depmodule);
										if (!dependencies.Contains(entry))
										{
											stack.Push(entry);
											dependencies.Add(entry);
										}
									}
								}
								else if (project.Properties[dependentGroup + ".buildtype"] != null)
								{
									//Dependent group without modules
									var entry = new BuildModuleDef(packageName, dependentGroup, dependentGroup.ToString());

									if (!dependencies.Contains(entry))
									{
										stack.Push(entry);
										dependencies.Add(entry);
									}
								}
								else
								{
									project.Log.ThrowWarning
									(
										Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Minimal,
										"Module '{0}.{1}' in package '{2}' declares dependency on module '{3}.{4}' that is not defined in this package", moduledef.BuildGroup, moduledef.ModuleName, packageName, dependentGroup, depmodule
									);
								}
							}
						}
					}

					// Now add runtime modules to all groups except tool if needed: 
					if (dependentGroup == BuildGroups.runtime && moduledef.BuildGroup != dependentGroup && moduledef.BuildGroup != BuildGroups.tool && needRuntimeModuleDependents)
					{
						var modskipruntime = String.Format("{0}.{1}.skip-runtime-dependency", moduledef.BuildGroup, moduledef.ModuleName);
						var skipruntime = String.Format("{0}.skip-runtime-dependency", moduledef.BuildGroup);

						if (false == project.Properties.GetBooleanPropertyOrDefault(modskipruntime, project.Properties.GetBooleanPropertyOrDefault(skipruntime, false)))
						{
							var runtime_buildmodules = project.Properties["runtime.buildmodules"];

							string groupname = null;

							if (runtime_buildmodules == null && project.Properties[dependentGroup + ".buildtype"] != null)
							{
								runtime_buildmodules = packageName;
								groupname = dependentGroup.ToString();
							}

							if (runtime_buildmodules != null)
							{

								foreach (var runtime_module in runtime_buildmodules.ToArray())
								{
									var entry = new BuildModuleDef(runtime_module, dependentGroup, groupname ?? dependentGroup + "." + runtime_module);
									if (!dependencies.Contains(entry))
									{
										stack.Push(entry);
										dependencies.Add(entry);
									}
								}
								var propName = String.Format("{0}.{1}.runtime.automoduledependencies", moduledef.BuildGroup, moduledef.ModuleName);
								project.Properties[propName] = runtime_buildmodules.ToArray().ToString(Environment.NewLine);
							}
						}
					}
				}
			}
			return dependencies;
		}

		private static IModule DelayedCreateNewModule(IPackage package, BuildModuleDef module)
		{
			string modules = package.Project.Properties[module.BuildGroup + ".buildmodules"].TrimWhiteSpace();

			if (modules.ToArray().Any(name => name == module.ModuleName))
			{
				string groupsourcedir = Path.Combine(package.Project.Properties["eaconfig." + module.BuildGroup + ".sourcedir"], module.ModuleName);
				return AddModule(package.Project, package, module.BuildGroup, module.ModuleName, module.Groupname, groupsourcedir);
			}
			return null;
		}

		internal static List<string> GetModulesWithDependencies(IPackage package, IEnumerable<string> unprocessedKeys)
		{
			string logPrefix = String.Empty;
			List<string> modulesWithDependencies = new List<string>();
			if (unprocessedKeys != null)
			{
				foreach (IGrouping<string, string> group in unprocessedKeys.GroupBy(um => um.Substring(0, um.IndexOf(':'))))
				{
					bool groupHasModules = !String.IsNullOrEmpty(package.Project.Properties[group.Key + ".buildmodules"].TrimWhiteSpace());

					IEnumerable<BuildModuleDef> buildmodules = GetmodulesWithDependencies(logPrefix, ((Package)package).Project, ConvertUtil.ToEnum<BuildGroups>(group.Key), group.Select(g => g.Substring(g.IndexOf(':') + 1)).ToList(), package.Name, hasmodules: groupHasModules);
					foreach (BuildModuleDef bm in buildmodules)
					{
						if (!package.Modules.Any(m => m.Name == bm.ModuleName && m.BuildGroup == bm.BuildGroup))
						{
							if (null == DelayedCreateNewModule(package, bm))
							{
								continue;
							}
						}
						modulesWithDependencies.Add(bm.BuildGroup + ":" + bm.ModuleName);
					}
				}
			}
			return modulesWithDependencies;
		}

		internal class BuildModuleDef : IEquatable<BuildModuleDef>
		{
			internal BuildModuleDef(string moduleName, BuildGroups buildGroup, String groupname)
			{
				ModuleName = moduleName;
				BuildGroup = buildGroup;
				Groupname = groupname;
			}
			internal readonly string ModuleName;
			internal readonly BuildGroups BuildGroup;
			internal readonly string Groupname;

			public override string ToString()
			{
				return BuildGroup + "." + ModuleName;
			}

			public bool Equals(BuildModuleDef other)
			{
				bool ret = false;

				if (Object.ReferenceEquals(this, other))
				{
					ret = true;
				}
				else if (Object.ReferenceEquals(other, null))
				{
					ret = (ModuleName == null);
				}
				else
				{
					ret = (BuildGroup == other.BuildGroup && ModuleName == other.ModuleName);
				}

				return ret;
			}

			public override int GetHashCode()
			{
				return (BuildGroup + "." + ModuleName).GetHashCode();
			}

		}
	}



}
