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
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Build
{
	internal class ModuleProcessor_LoadPackage : ModuleDependencyProcessorBase, IDisposable
	{
		public readonly string AutobuildTarget;
		public readonly Project.TargetStyleType CurrentTargetStyle;

		private int errors = 0;

		private readonly bool HasLinkInDependencyChain;

		internal ModuleProcessor_LoadPackage(ProcessableModule module, string autobuildtarget, Project.TargetStyleType currentTargetStyle, string logPrefix)
			: base(module, logPrefix)
		{
			AutobuildTarget = autobuildtarget;
			CurrentTargetStyle = currentTargetStyle;

			HasLinkInDependencyChain = false;

			if (Project.Properties.GetBooleanProperty(Project.NANT_PROPERTY_TRANSITIVE))
			{
				HasLinkInDependencyChain = Project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_LINK_DEPENDENCY_IN_CHAIN, false);
			}
		}

		public void Dispose()
		{
			// DAVE-FUTURE-REFACTOR-TODO: there is no reason for this class to implement IDisposable, but it does and is using()'d in a few places that would need cleaning up before it was removed
		}

		public void Process(ProcessableModule module)
		{
			List<Dependency<PackageDependencyDefinition>> packageDependenciesRequiringLoad = GetModuleDependentDefinitions(module, excludeThisPackage: true, includePublic: true);
			{
				foreach (Dependency<PackageDependencyDefinition> dep in packageDependenciesRequiringLoad)
				{
					LoadOneDependentPackage(module, dep);
				}
				DoDelayedInit(module);
			}
		}

		private void LoadOneDependentPackage(ProcessableModule module, Dependency<PackageDependencyDefinition> d)
		{
			try
			{
				if (0 == Interlocked.CompareExchange(ref errors, 0, 0))
				{
					if (module.Package.Name == d.Dependent.PackageName)
					{
						throw new BuildException($"Package '{module.Package.Name}' cannot depend on itself as '{DependencyTypes.ToString(d.Type)}'.");
					}

					var targetStyleValue = d.IsKindOf(DependencyTypes.Build | DependencyTypes.AutoBuildUse) ? Project.TargetStyleType.Build : Project.TargetStyleType.Use;

					if (Project.Properties.GetBooleanProperty(Project.NANT_PROPERTY_TRANSITIVE))
					{
						// To enable transitive propagation in case of "usedependencies".
						// Example: 
						// Program -- (build)--> Library A -- (use) --> Library B. 
						// To let transitive propagation of Library B make sure build file is loaded whenever we have link dependency.
						bool utilModuleStaticLibWrapper = (module.IsKindOf(Module.Utility) && (module as Module_Utility).TransitiveLink) 
							|| (module.IsKindOf(Module.MakeStyle) && (module as Module_MakeStyle).TransitiveLink);
						if (HasLinkInDependencyChain && (module.IsKindOf(Module.Library) || utilModuleStaticLibWrapper) && d.IsKindOf(DependencyTypes.Link))
						{
							targetStyleValue = Project.TargetStyleType.Build;
						}
					}

					// If task is called from within Use target = set target style to use:
					if (CurrentTargetStyle == Project.TargetStyleType.Use)
					{
						targetStyleValue = Project.TargetStyleType.Use;
					}

					if (Log.InfoEnabled)
						Log.Info.WriteLine(LogPrefix + "{0}: loading dependent package '{1}', target '{2}' (targetstyle={3}).", module.ModuleIdentityString(), d.Dependent.PackageName, AutobuildTarget, Enum.GetName(typeof(Project.TargetStyleType), targetStyleValue).ToLowerInvariant());

					var dependent_constraints = GetModuleDependenciesConstraints(Project, module, d.Dependent.PackageName);

					bool shouldPropagateLinkDependency = HasLinkInDependencyChain || HasLink;
					string packageVersion = TaskUtil.Dependent
					(
						Project, d.Dependent.PackageName, d.Dependent.ConfigurationName,
						target: AutobuildTarget,
						targetStyle: targetStyleValue,
						dropCircular: true,
						constraints: dependent_constraints,
						propagatedLinkDependency: shouldPropagateLinkDependency,
						nonBlockingAutoBuildClean: true
					);

					// Add Dependent modules. Get dependent package, and add modules. Filter by moduledependency.
					IPackage dependentPackage = null;
					if (!Project.BuildGraph().TryGetPackage(d.Dependent.PackageName, packageVersion, d.Dependent.ConfigurationName, out dependentPackage))
					{
						throw new BuildException(String.Format("Can't find instance of Package after dependent task was executed for '{0}-{1}'. Verify that package has a .build file with <package> task.", d.Dependent.PackageName, packageVersion));
					}

					if (targetStyleValue != Project.TargetStyleType.Use)
					{
						AddSkippedDependentDependents(dependentPackage, dependent_constraints);
					}
				}
			}
			catch (Exception)
			{
				// Throw the first error occurred in any thread. Ignore others as they are likely consequences of the first one.
				if (1 == Interlocked.Increment(ref errors))
				{
					throw;
				}
			}
		}

		private void AddSkippedDependentDependents(IPackage dependentPackage, List<ModuleDependencyConstraints> dependent_constraints)
		{
			// DependentTask with build style was called on the dependent package, but in some cases because of circular dependencies on the package level 
			// Dependent task will drop dependency and won't wait until dependent package is loaded. In this case we can not check skipped dependencies.

			BuildGraph.PackageDependentsProcessingStatus packageDependentProcessingStatus = Project.BuildGraph().GetPackageDependentProcessingStatus(dependentPackage);

			IEnumerable<string> moduleToProcess = packageDependentProcessingStatus.SetUnProcessedModules(dependent_constraints != null ? dependent_constraints.Select(c => c.Group + ":" + c.ModuleName) : null, LoadPackageTask.GetModulesWithDependencies);
			ReadOnlyCollection<IModule> dependentPackageModules = dependentPackage.ModulesNonLazy;

			if (moduleToProcess != null && moduleToProcess.Count() > 0)
			{
				foreach (string key in moduleToProcess)
				{
					ProcessableModule depMod = null;

					// This is thread un-safe. It is possible to have the dependent package modules
					// added to while iterating over the modules. I don't know if this is harmful,
					// I suspect it may not, but it is a cause of concern. Previously this code used
					// FirstOrDefault() to find the first matching module but this was throwing an
					// exception when the collection of modules was updated.
					// -mburke 2016-02-22
					for (int i = 0; i < dependentPackageModules.Count; ++i)
					{
						IModule currentModule = dependentPackageModules[i];
						if (currentModule.BuildGroup + ":" + currentModule.Name == key)
						{
							depMod = currentModule as ProcessableModule; 
							break;
						}
					}

					if (depMod != null)
					{
						using (var moduleProcessor = new ModuleProcessor_LoadPackage(depMod, packageDependentProcessingStatus.AutobuildTarget, packageDependentProcessingStatus.CurrentTargetStyle, LogPrefix))
						{
							moduleProcessor.Process(depMod);
						}
					}
				}
			}
			else if ((HasLink || HasLinkInDependencyChain) && dependentPackage.Project != null)
			{
				if (!dependentPackage.Project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_LINK_DEPENDENCY_IN_CHAIN, false))
				{
					dependentPackage.Project.Properties[Project.NANT_PROPERTY_LINK_DEPENDENCY_IN_CHAIN] = "true";

					// This is thread un-safe. It is possible to have the dependent package modules
					// added to while iterating over the modules. I don't know if this is harmful,
					// I suspect it may not, but it is a cause of concern.
					// -mburke 2016-01-27
					for (int i = 0; i < dependentPackageModules.Count; ++i)
					{
						if (!(dependentPackageModules[i] is ProcessableModule depMod))
						{
							continue;
						}

						bool needDelayedLoad = false;
						bool utilModuleStaticLibWrapper = (depMod.IsKindOf(Module.Utility) && (depMod as Module_Utility).TransitiveLink) 
										|| (depMod.IsKindOf(Module.MakeStyle) && (depMod as Module_MakeStyle).TransitiveLink);
						if ((depMod.IsKindOf(Module.Library) || utilModuleStaticLibWrapper) && (dependent_constraints==null || (dependent_constraints != null && dependent_constraints.Any(c => c.Group == depMod.BuildGroup && c.ModuleName == depMod.Name))))
						{
							using (var moduleProcessor = new ModuleProcessor_LoadPackage(depMod, packageDependentProcessingStatus.AutobuildTarget, packageDependentProcessingStatus.CurrentTargetStyle, LogPrefix))
							{
								foreach (var d in moduleProcessor.GetModuleDependentDefinitions(depMod))
								{
									Release release = PackageMap.Instance.FindOrInstallCurrentRelease(dependentPackage.Project, d.Dependent.PackageName);
									if (release != null && PackageMap.Instance.IsPackageAutoBuildClean(dependentPackage.Project, d.Dependent.PackageName) && release.Manifest.Buildable)
									{
										if (Project.BuildGraph().TryGetPackage(d.Dependent.PackageName, release.Version, d.Dependent.ConfigurationName, out IPackage pkg))
										{
											if (pkg.IsKindOf(FrameworkTasks.Model.Package.AutoBuildClean) && pkg.IsKindOf(FrameworkTasks.Model.Package.Buildable) && pkg.Project == null)
											{
												needDelayedLoad = true;
												break;
											}
										}
										else
										{
											needDelayedLoad = true;
										}
									}

								}
								if (needDelayedLoad)
								{
									Log.Info.WriteLine(LogPrefix + "Loading dependents of {0} because it is a library and we arrived here through dependency path with link and package {1} was previously loaded through dependency path without link", depMod.ModuleIdentityString(), depMod.Package.Name);
									moduleProcessor.Process(depMod);
								}
							}
						}
					}
				}
			}
		}

		private void DoDelayedInit(ProcessableModule module)
		{
			if (BuildOptionSet.Options.TryGetValue("delayedinit", out string delayedInitTargetName))
			{
				Log.Info.WriteLine(LogPrefix + "'{0}': executing delayedinit target '{1}'", module.ModuleIdentityString(), delayedInitTargetName);

				module.SetModuleBuildProperties();

				Project.ExecuteTargetIfExists(delayedInitTargetName, true);
			}
		}
	}
}

