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
using System.Threading;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Threading;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Structured;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Build
{
	public class ModuleProcessor_ProcessDependencies : ModuleDependencyProcessorBase, IDisposable
	{
		internal class FilteredTypeDependencyComparer : IEqualityComparer<Dependency<IModule>>
		{
			private readonly uint m_typeFilter;

			internal FilteredTypeDependencyComparer(uint typeFilter)
			{
				m_typeFilter = typeFilter;
			}

			public bool Equals(Dependency<IModule> x, Dependency<IModule> y)
			{
				return x.Dependent == y.Dependent && (x.Type & m_typeFilter) == (y.Type & m_typeFilter);
			}

			public int GetHashCode(Dependency<IModule> dep)
			{
				return ((dep.Dependent.GetHashCode() << 5) + dep.Dependent.GetHashCode()) ^ (dep.Type & m_typeFilter).GetHashCode();
			}
		}

		internal ModuleProcessor_ProcessDependencies(ProcessableModule module, string logPrefix)
			: base(module, logPrefix)
		{
		}

		public void Process(ProcessableModule module)
		{
			ProcessPackageLevelDependencies(module);

			ProcessModuleDependents(module, AutoBuildUse, HasLink);

			// need to be after ProcessModuleDependents() otherwise module dependencies within a package's modules will not be considered
			ProcessDependentsPublicDependencies(module, DependencyTypes.Interface, allowUnloaded: true, publicDependencyPropertyNames: new string[] { "publicdependencies", "publicbuilddependencies" });
			ProcessDependentsPublicDependencies(module, DependencyTypes.Build | DependencyTypes.Link | DependencyTypes.AutoBuildUse, publicDependencyPropertyNames: "publicbuilddependencies");

			if (Log.Level >= Log.LogLevel.Detailed)
			{
				var log = new BufferedLog(Log);
				log.Info.WriteLine(LogPrefix + "dependents of {0} - {1}.", module.GetType().Name, module.ModuleIdentityString());
				log.IndentLevel += 6;
				foreach (var dependency in module.Dependents)
				{
					log.Info.WriteLine("{0} : {1} - {2}", DependencyTypes.ToString(dependency.Type), dependency.Dependent.GetType().Name, dependency.Dependent.ModuleIdentityString());
				}
				log.Flush();
			}

			ProcessSpecialDependencyTypes(module);
		}

		private void ProcessPackageLevelDependencies(IModule module)
		{
			try
			{

				int errors = 0;

				IEnumerable<Dependency<PackageDependencyDefinition>> dependencyDefinitions = GetModuleDependentDefinitions(module);
				foreach (Dependency<PackageDependencyDefinition> d in dependencyDefinitions)
				{
					if (0 == Interlocked.CompareExchange(ref errors, 0, 0))
					{
						try
						{
							if (Log.DebugEnabled)
								Log.Debug.WriteLine(LogPrefix + "{0}: processing dependent package '{1}' {2}", module.ModuleIdentityString(), d.Dependent.PackageName, DependencyTypes.ToString(d.Type));

							BuildGraph buildGraph = Project.BuildGraph();
							//Set up Module dependencies:
							string packageVersion = PackageMap.Instance.FindOrInstallCurrentRelease(Project, d.Dependent.PackageName).Version;
							BuildGraph.PackageDependentsProcessingStatus packageDependentProcessingStatus = buildGraph.GetPackageDependentProcessingStatus(module.Package);

							if (!buildGraph.TryGetPackage(d.Dependent.PackageName, packageVersion, d.Dependent.ConfigurationName, out IPackage dependentPackage))
							{
								if (packageDependentProcessingStatus != null && packageDependentProcessingStatus.ProcessedModules.Contains(module.BuildGroup + ":" + module.Name))
								{
									Log.Error.WriteLine(LogPrefix + "Module {0} declares '{1}' dependency on {2} ({3}) and can't find 'Package' for dependent.",
										module.ModuleIdentityString(), DependencyTypes.ToString(d.Type), d.Dependent.PackageName, d.Dependent.ConfigurationName);
								}
							}
							else
							{
								if (packageDependentProcessingStatus != null && !packageDependentProcessingStatus.ProcessedModules.Contains(module.BuildGroup + ":" + module.Name))
								{
									// this module is an active module but was only loaded as an interface dependency from this package when other modules were fully loaded,
									// it's dependencies have not been processed so skip trying to resolve them
									continue;
								}

								if (d.IsKindOf(DependencyTypes.Build | DependencyTypes.AutoBuildUse))
								{
									if (!dependentPackage.IsKindOf(Package.AutoBuildClean))
									{
										if (d.IsKindOf(DependencyTypes.Build))
										{
											Log.Info.WriteLine("{0}  Module {1}/{2} declares build dependency on non buildable package {3}. Dependency reset to use dependency", module.Configuration.Name, module.Package.Name, module.Name, dependentPackage.Name);
										}
										d.ClearType(DependencyTypes.Build | DependencyTypes.AutoBuildUse);
									}
								}

								// NOTE: The GetPackageDeclaredModules() implementation seems to make explicit assumption that 
								// the same module name in a package cannot exist in multiple build group.  This is normally acceptable
								// assumption since we cannot create a Visual Studio solution with multiple identical module names
								// anyways.  But we need to be aware of the legacy use case where package can be in old style build script
								// where there are no module specification and package names are used for modules for all build groups.
								Dictionary<string, BuildGroups> declaredModules = GetPackageDeclaredModules(d.Dependent.PackageName);

								// Explicit subset of modules to depend on
								var moduleConstraints = GetModuleDependenciesConstraints(Project, module, d.Dependent.PackageName);

								// Modules already created in the package
								IEnumerable<IModule> definedModules = null;

								if ((dependentPackage.IsKindOf(Package.AutoBuildClean)))
								{
									definedModules = dependentPackage.Modules;
									if (definedModules.Count() == 0 || (moduleConstraints.IsNullOrEmpty() && definedModules.Where(defmod => defmod.BuildGroup == d.BuildGroup).Count() == 0))
									{
										if (d.IsKindOf(DependencyTypes.Build))
										{
											// We have a build dependency on the package, but there are no defined build modules.
											// Obviously this is not a correct dependency on this package, but we accept it anyway, change it to an interface dependency and warn the user...
											Project.Log.ThrowWarning
											(
												Log.WarningId.InvalidModuleDepReference,
												Log.WarnLevel.Advise,
												"Module [{0}] has a build dependency on package [{1}], but that package has no buildable modules." +
												"Adding[{1}] as if it was an interface dependency.  The build scripts in Package[{2}] should be changed to reflect this.",
												module.Name, dependentPackage.Name, dependentPackage.Name, module.Package.Name
											);
										}

										// We want to clean build type mask
										d.ClearType(DependencyTypes.Build | DependencyTypes.AutoBuildUse);

										definedModules = null;
									}

									if (!dependentPackage.IsKindOf(Package.AutoBuildClean))
									{
										//IM: WE should never get here as Dependent task would take care of this!

										// We have a build dependency on a package which is buildable.  But the autobuildclean attribute is set to false for this package.
										// Hence we treat is as a use dependency.
										definedModules = null;

										// Clear the previous type and set the dependency as a use dependency
										d.ClearType(DependencyTypes.Build | DependencyTypes.AutoBuildUse);
									}
								}

								var useDependencyModules = new List<IModule>();

								if (moduleConstraints == null && !declaredModules.Any(declaredModule => declaredModule.Value == BuildGroups.runtime))
								{
									// if none of the declared modules are runtime modules and we aren't depending on specific modules then assume
									// user wants to depend on the package's moduleless info (i.e. package.MyPackage.includedirs) and so add a 
									// package use dependency
									if (definedModules == null)
									{
										useDependencyModules.Add(GetOrAddModule_UseDependency(Module_UseDependency.PACKAGE_DEPENDENCY_NAME, module.Configuration, d.BuildGroup, dependentPackage));
									}
								}
								else
								{
									// Speed up lookup
									Dictionary<string, IModule> definedModulesLookup = null;
									if (definedModules != null)
									{
										try
										{
											definedModulesLookup = definedModules.ToDictionary((m) => m.Name);
										}
										catch (Exception e)
										{
											// We ran into a situation where user is getting a crash from ToDictionary()
											// complaining about key is already added.  It is possible to be user error creating
											// multiple module of identical name.  But crashing with a message about
											// "An item with the same key has already been added." and nothing else isn't helpful 
											// to user.  So if we got here, we should try to create a better message and tell user
											// which module name is being duplicated.
											Dictionary<string, List<string>> moduleGroups = new Dictionary<string, List<string>>();
											List<string> moduleNamesWithMultipleGroup = new List<string>();
											foreach (IModule mm in definedModules)
											{
												if (!moduleGroups.ContainsKey(mm.Name))
													moduleGroups.Add(mm.Name, new List<string>() { mm.BuildGroup.ToString() });
												else
												{
													moduleGroups[mm.Name].Add(mm.BuildGroup.ToString());
													if (!moduleNamesWithMultipleGroup.Contains(mm.Name))
														moduleNamesWithMultipleGroup.Add(mm.Name);
												}
											}
											if (moduleNamesWithMultipleGroup.Any())
											{
												System.Text.StringBuilder msg = new System.Text.StringBuilder();
												msg.AppendLine(String.Format("The build file for package '{0}' has the following module(s) defined more than once:", dependentPackage.Name));
												foreach (string mn in moduleNamesWithMultipleGroup)
												{
													msg.AppendLine(string.Format("    Module '{0}' is defined in groups: '{1}'", mn, string.Join(",", moduleGroups[mn])));
												}
												throw new BuildException(msg.ToString(), e);
											}
											else
											{
												// Shouldn't really get here.  But rethrow the original exception in case our 
												// assumption that the exception is being throw by ToDictionary() is incorrect.
												throw;
											}
										}
									}
									
									foreach (var depmodule in declaredModules)
									{
										IModule dependentModule = null;

										if (definedModules != null)
										{
											definedModulesLookup.TryGetValue(depmodule.Key, out dependentModule);
										}
										else
										{
											dependentModule = GetOrAddModule_UseDependency(depmodule.Key, module.Configuration, depmodule.Value, dependentPackage);
											useDependencyModules.Add(dependentModule);
										}

										if (dependentModule == null)
										{
											Project.Log.ThrowWarning
											(
												Log.WarningId.InconsistentInitializeXml,
												Log.WarnLevel.Advise,
												"Module '{1}' declared in intialize.xml file of package '{2}' is not defined in the {2}.build file." +
												"{0}\tDeclared modules (Initialize.xml):   " + "{3};{0}\tDefined modules ({2}.build): {4};" +
												"{0}This might be due to modules in {2}.build file are only defined for specific conditions. Those conditions need to be duplicated in initialize.xml file of the package.  If this warning is allowed to continue, this module will be ignored!{0}",
												Environment.NewLine,                        // {0}
												depmodule,                                  // {1}
												dependentPackage.Name,                      // {2}
												declaredModules.ToString(", "),             // {3}
												definedModules.ToString(", ", m => m.Name)  // {4}
											);
										}
									}
								}

								if (moduleConstraints != null && moduleConstraints.Count > 0)
								{
									// We also check that the dependent package is a build dependent package (ie package is not being used as a 
									// prebuilt - use/interface dependent).  If it is a use/interface dependent package, build file is not loaded 
									// and dependentPackage.Modules list will be empty.
									if (dependentPackage.IsKindOf(Package.AutoBuildClean) &&
										dependentPackage.IsKindOf(Package.Buildable) &&
										dependentPackage.IsKindOf(DependencyTypes.Build))
									{
										List<ModuleDependencyConstraints> mismatch = null;

										// Check dependency definitions:
										foreach (var constraint in moduleConstraints)
										{
											if (!dependentPackage.Modules.Any(m => m.BuildGroup == constraint.Group && m.Name == constraint.ModuleName))
											{
												if (mismatch == null)
												{
													mismatch = new List<ModuleDependencyConstraints>();
												}
												mismatch.Add(constraint);
											}
										}
										if (mismatch != null && mismatch.Count > 0)
										{
											Project.Log.ThrowWarning
											(
												Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Minimal,
												"Module '{0}' declares dependency on a module(s) '{1}' of package '{2}' which is not defined in the {2}.build file.", 
												module.ModuleIdentityString(), mismatch.ToString(", ", c => String.Format("{0}.{1}", c.Group, c.ModuleName)), dependentPackage.Name
											);
										}

										if (declaredModules != null && declaredModules.Count > 0)
										{
											List<string> dmismatch = null;

											foreach (var declared in declaredModules)
											{
												if (!dependentPackage.Modules.Any(m => m.BuildGroup == d.BuildGroup && m.Name == declared.Key))
												{
													if (dmismatch == null)
													{
														dmismatch = new List<string>();
													}
													dmismatch.Add(declared.Key);
												}
											}
											if (dmismatch != null && dmismatch.Count > 0)
											{
												Project.Log.ThrowWarning
												(
													Log.WarningId.InconsistentInitializeXml, Log.WarnLevel.Minimal,
													"Package '{0}' has mismatched module definitions in .build and Initialize.xml files.{1}      Build file modules: {2}.{1}      Initialize.xml modules: {3}",
													d.Dependent.PackageName, Environment.NewLine, dependentPackage.Modules.ToString(", ", m => String.Format("{0}.{1}", m.BuildGroup, m.Name)), declaredModules.ToString(", ", dcm => String.Format("{0}.{1}", d.BuildGroup, dcm))
												);
											}

										}
									}
									else
									{
										if (declaredModules != null && declaredModules.Count > 0)
										{
											List<ModuleDependencyConstraints> mismatch = null;
											List<KeyValuePair<string, BuildGroups>> groupMismatch = null;

											// Check dependency definitions:
											foreach (var constraint in moduleConstraints)
											{
												if (!declaredModules.Any(dm => dm.Key == constraint.ModuleName && dm.Value == constraint.Group))
												{
													// Keep track of all of the dependency modules that cannot be found so we can warn about them
													if (mismatch == null)
													{
														mismatch = new List<ModuleDependencyConstraints>();
													}
													mismatch.Add(constraint);

													// Keep track of which of the module dependencies are found but with a different group so we can provide a better warning
													KeyValuePair<string, BuildGroups> matchInDifferentGroup = declaredModules.FirstOrDefault(dm => dm.Key == constraint.ModuleName);
													if (matchInDifferentGroup.Key != null)
													{
														if (groupMismatch == null)
														{
															groupMismatch = new List<KeyValuePair<string, BuildGroups>>();
														}
														groupMismatch.Add(matchInDifferentGroup);
													}
												}
											}

											if (groupMismatch != null && groupMismatch.Count() > 0)
											{
												Project.Log.ThrowWarning
												(
													Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Minimal,
													"Module '{0}' declares dependency on a module(s) '{1}' of package '{2}' which is not declared in the Initialize.xml file of {2}."
													+ " However we found following module(s) with the same name in a different build group '{3}'."
													+ " If these are the correct modules please update the dependencies with the correct build group."
													+ " For example in structured xml you can use the syntax <package>/<group>/<module> when declaring dependencies.",
													module.ModuleIdentityString(), mismatch.ToString(", ", c => String.Format("{0}.{1}", c.Group, c.ModuleName)), dependentPackage.Name,
													groupMismatch.ToString(", ", c => String.Format("{0}.{1}", c.Value.ToString(), c.Key))
												);
											}
											else if (mismatch != null && mismatch.Count > 0)
											{
												Project.Log.ThrowWarning
												(
													Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Minimal,
													"Module '{0}' declares dependency on a module(s) '{1}' of package '{2}' which is not declared in the Initialize.xml file of {2}.",
													module.ModuleIdentityString(), mismatch.ToString(", ", c => String.Format("{0}.{1}", c.Group, c.ModuleName)), dependentPackage.Name
												);
											}
										}
										else
										{
											Project.Log.ThrowWarning
											(
												Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Minimal, 
												"Module '{0}' declares dependency on subset of modules from package '{1}' but modules are not declared in the Initialize.xml file of {1}." + 
												" Attempted Dependencies are: {2}. This module constraint will be ignored and entire package will be depended on!", 
												module.ModuleIdentityString(), dependentPackage.Name, moduleConstraints.ToString(", ", mc => mc.ModuleName)
											);
											// If we get this use case, we won't be able to rely on module constrains as the dependent package's Initialize.xml might have set a package level fileset export including ALL modules.
											// So we need to remove any module constraints and force make it dependent on entire package.
											moduleConstraints = null;
										}
									}
								}

								foreach (var dependentModule in (definedModules ?? useDependencyModules))
								{
									var dependencyType = new BitMask(d.Type);

									if (!moduleConstraints.IsNullOrEmpty())
									{
										var constrainDef = moduleConstraints.FirstOrDefault(c => c.Group == dependentModule.BuildGroup && c.ModuleName == dependentModule.Name);
										if (constrainDef == null)
										{
											//Skip this module because it is not in the constraint.
											if (definedModules == null && declaredModules.IsNullOrEmpty() && dependentModule is Module_UseDependency)
											{
												var packageBuildStatus = dependentPackage.IsKindOf(EA.FrameworkTasks.Model.Package.Buildable)
													? (dependentPackage.IsKindOf(EA.FrameworkTasks.Model.Package.AutoBuildClean) ? String.Empty : "(autobuildclean=false) ") : "(nonbuildable) ";

												var depconstraintstr = moduleConstraints.ToString(Environment.NewLine, c => String.Format("          !!!!!! {0}/{1}.{2}", d.Dependent.PackageName, c.Group, c.ModuleName));

												Project.Log.ThrowWarning
												(
													Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Minimal,
													"           !!!!!! Skipping dependencies defined in module '{1}':{0}" + 
													"{2}{0}" +
													"           !!!!!! Reason: No modules are listed in the Initialize.xml file in package '{3}' {4}, it is impossible to satisfy specific module dependency constraint.{0}" +
													"           !!!!!! See documentation on how to define data for each module in Initialize.xml file.",
													Environment.NewLine, module.ModuleIdentityString(), depconstraintstr, d.Dependent.PackageName, packageBuildStatus
												);
											}
											continue;
										}
										else
										{
											//Refine dependency flags based on constraints 
											RefineDependencyTypeFromConstraint(dependencyType, constrainDef);

											if (dependencyType.Type != d.Type)
											{
												Log.Info.WriteLine("Dependency '{0}' on a '{1}' changed type from '{2}' to '{3}'", module.ModuleIdentityString(), dependentModule.ModuleIdentityString(), DependencyTypes.ToString(d.Type), DependencyTypes.ToString(dependencyType.Type));
											}
										}
									}
									else if (dependentModule.BuildGroup != d.BuildGroup)
									{
										continue;
									}

									// Utility and makefile modules set as auto dependencies will always try to be built.
									// If it has nothing to build there will be a small cost (~0.03s) to determine there is nothing to build.
									// To avoid this set these as use/build dependencies explicitly.
									if (d.IsKindOf(DependencyTypes.AutoBuildUse)
										&& dependentModule.IsKindOf(Module.MakeStyle | Module.Utility)
										&& dependencyType.Type != DependencyTypes.Interface     // If the dependency has been marked specifically as interface only, don't force build dependent or you'll get circular dependency.
										)
									{
										dependencyType.SetType(DependencyTypes.Build);
									}

									module.Dependents.Add(dependentModule, dependencyType.Type);
								}
							}
						}
						catch (Exception e)
						{
							string test = e.ToString();
							Log.Warning.Write(test);
							if (1 == Interlocked.Increment(ref errors))
							{
								throw;
							}
						}
					}
				}
			}
			catch (Exception e)
			{
				ThreadUtil.RethrowThreadingException
				(
					String.Format
					(
						"Error processing dependents (ModuleProcessor_ProcessDependencies.ProcessPackageLevelDependencies) of [{0}-{1}], module '{2}'", 
						module.Package.Name, module.Package.Version, module.Name
					),
					Location.UnknownLocation, e
				);
			}
		}

		private void ProcessDependentsPublicDependencies(IModule module, uint propagationTypes, bool allowUnloaded = false, params string[] publicDependencyPropertyNames)
		{
			FilteredTypeDependencyComparer filteredComparer = new FilteredTypeDependencyComparer(propagationTypes);
			List <Dependency<IModule>> additionalPublicDependenciesToAdd = new List<Dependency<IModule>>();
			List <Dependency<IModule>> dependenciesToProcess = module.Dependents.ToList();
			HashSet<Dependency<IModule>> processedDependencies = new HashSet<Dependency<IModule>>(filteredComparer); 
			while (dependenciesToProcess.Any())
			{
				foreach (Dependency<IModule> dep in dependenciesToProcess)
				{
					uint matchingTypes = propagationTypes & dep.Type;
					if (matchingTypes != 0)
					{
						foreach (string publicDependencyPropertyName in publicDependencyPropertyNames)
						{
							string publicDeps = Project.Properties.GetPropertyOrDefault(String.Format("package.{0}.{1}.{2}", dep.Dependent.Package.Name, dep.Dependent.Name, publicDependencyPropertyName), String.Empty);

							DependencyDeclaration dependencies = new DependencyDeclaration(publicDeps);
							foreach (DependencyDeclaration.Package package in dependencies.Packages)
							{
								foreach (KeyValuePair<BuildGroups, HashSet<string>> group in dependencies.GetPublicModuleNamesForPackage(package, Project))
								{
									Dependency<IPackage> dependentPackage = null;
									if (!module.Package.DependentPackages.Any(p => p.Dependent.Name == package.Name) && package.Name == module.Package.Name)
									{
										dependentPackage = new Dependency<IPackage>(module.Package, group.Key);
									}
									else
									{
										dependentPackage = module.Package.DependentPackages.First(p => p.Dependent.Name == package.Name); // assigned in inner loop, becayse this may not be in DependentPackages until after GetPublicModulesNamesForPackage call
									}
									BuildGraph.PackageDependentsProcessingStatus dependentProcessingStatus = Project.BuildGraph().TryGetPackageDependentProcessingStatus(dependentPackage.Dependent);
									foreach (string mod in group.Value)
									{
										IModule publicDepModule = dependentPackage.Dependent.Modules.Where(m => m.Name == mod && m.BuildGroup == group.Key).FirstOrDefault();
										if (publicDepModule == null)
										{
											// If the build graph for top package is created using "use" target style rather than "build" target style, only the
											// Initialize.xml is loaded and the build file will not be loaded.  In which case, the above dependentPackage.Dependent.Modules
											// list will be empty and dependentProcessingStatus will return null (because the dependent package wasn't loaded using "build"
											// method).  In this case, we just treat this dependent package as use dependent package because during this scenario, 
											// we really just want to get the publicDepModule info and we are not actually doing build.
											if (!(allowUnloaded || dep.Dependent is Module_UseDependency || dependentProcessingStatus == null))
											{
												throw new BuildException(String.Format("Module {0} has a dependency on {1}/{2} via {5} set in package {3} ({4}) but that module is not found in the package {1}!",
													module.ModuleIdentityString(), package.Name, mod, dep.Dependent.Package.Name, module.Configuration.Name, publicDependencyPropertyName));
											}
											else
											{
												// if this is a use dependency, it's likely we never loaded it's dependents so propagated use dependency instead
												publicDepModule = GetOrAddModule_UseDependency(mod, module.Configuration, group.Key, dependentPackage.Dependent);
											}
										}

										if (publicDepModule.Name == module.Name)  // A Module cannot depend on itself!
											continue;

										additionalPublicDependenciesToAdd.Add(new Dependency<IModule>(publicDepModule, publicDepModule.BuildGroup, matchingTypes));
									}
								}
							}
						}
					}
				}

				// cyclic dependency protection
				foreach (Dependency<IModule> depMod in dependenciesToProcess)
				{
					processedDependencies.Add(new Dependency<IModule>(depMod));
				}

				// important note: this loop may modifying existing dependency references,
				// hence the copies above
				foreach (Dependency<IModule> depMod in additionalPublicDependenciesToAdd)
				{
					module.Dependents.Add(depMod.Dependent, depMod.Type);
				}

				dependenciesToProcess = additionalPublicDependenciesToAdd.Except(processedDependencies, filteredComparer).ToList();

				additionalPublicDependenciesToAdd = new List<Dependency<IModule>>();
			}
		}

		private static readonly BuildGroups[] s_buildGroupsArray = (BuildGroups[])Enum.GetValues(typeof(BuildGroups));

		/// <summary>Dependencies between modules in the same package</summary>
		public static void ProcessModuleDependents(IModule module, bool auto_build_use = false, bool has_link = false)
		{
			foreach (BuildGroups dependentGroup in s_buildGroupsArray)
			{
				foreach (var v in Mappings.ModuleDependencyPropertyMapping)
				{
					string depPropName = String.Format("{0}.{1}.{2}{3}", module.BuildGroup, module.Name, dependentGroup, v.Key);
					string depPropNameConfig = depPropName + "." + module.Configuration.System;
					string depPropNameConfigValue = module.Package.Project.Properties[depPropNameConfig];

					var depModuleNames = String.Concat(depPropNameConfigValue, Environment.NewLine, module.Package.Project.Properties[depPropName]).ToArray();

					foreach (string dependentModule in depModuleNames)
					{
						bool found = false;
						bool foundButOfDifferentCasing = false;
						foreach (IModule m in module.Package.Modules)
						{
							if(m.BuildGroup == dependentGroup && string.Equals(m.Name,dependentModule,StringComparison.InvariantCultureIgnoreCase))
							{
								foundButOfDifferentCasing = true;
							}
							
							if (m.BuildGroup == dependentGroup && m.Name == dependentModule)
							{
								uint dependencyType = v.Value;

								if (v.Key == ".moduledependencies")
								{
									// ".moduledependencies" behave as auto. We can change this if needed. But is seems to make sense for efficiency
									if (!module.IsKindOf(Module.Utility | Module.MakeStyle) && !m.IsKindOf(Module.Utility | Module.MakeStyle))
									{
										dependencyType |= DependencyTypes.AutoBuildUse;
										dependencyType &= ~DependencyTypes.Build;
									}
								}

								if (auto_build_use)
								{
									if (has_link && (dependencyType & DependencyTypes.AutoBuildUse) == DependencyTypes.AutoBuildUse)
									{
										dependencyType |= DependencyTypes.Build;
									}
								}
								else if ((dependencyType & DependencyTypes.AutoBuildUse) == DependencyTypes.AutoBuildUse)
								{
									dependencyType |= DependencyTypes.Build;
								}

								if (module != m)
								{
									module.Dependents.Add(m, dependencyType);
								}
								else
								{
									module.Package.Project.Log.ThrowWarning
									(
										Log.WarningId.ModuleSelfDependency,
										Log.WarnLevel.Normal,
										"Module {0} from package {1} declares dependency upon itself.", 
										module.Name, module.Package.Name
									);
								}

								found = true;
								break;
							}
						}
						if (!found)
						{
							string formatting;

							if(foundButOfDifferentCasing)
							{
								formatting = "package {0}-{1} ({2}) {3}: Module '{4}.{5}' specified in property '{6}' is defined in this package, but is of different casing. Did you spell it correctly?";
							}
							else
							{
								formatting = "package {0}-{1} ({2}) {3}: Module '{4}.{5}' specified in property '{6}' is not defined in this package.";
							}

							module.Package.Project.Log.ThrowWarning
							(
								Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Normal,
								formatting,
								module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup + "." + module.Name, dependentGroup, dependentModule, depPropNameConfigValue != null ? depPropNameConfig : depPropName
							);
						}
					}
				}
			}
		}

		private IModule GetOrAddModule_UseDependency(string name, Configuration configuration, BuildGroups buildgroup, IPackage package)
		{
			// This needs to use threadsafe GetOrAdd since it can be called from multiple threads
			return Project.BuildGraph().Modules.GetOrAdd(Module.MakeKey(name, buildgroup, package), new Module_UseDependency(name, configuration, buildgroup, package), true);
		}

		private void ProcessSpecialDependencyTypes(IModule module)
		{
			// DAVE-FUTURE-REFACTOR-TODO: we only have one special dependency type now, this function could be made more specific
			Dictionary<string, uint> dependenciesPropertiesToTypes = new Dictionary<string, uint>()
			{
				{ "copylocal.dependencies", DependencyTypes.CopyLocal }
			};

			foreach (KeyValuePair<string, uint> dependencyMapping in dependenciesPropertiesToTypes)
			{
				string propertyName = dependencyMapping.Key;
				uint dependencyType = dependencyMapping.Value;

				DependencyDeclaration dependencyDeclaration = new DependencyDeclaration(GetModulePropertyValue(propertyName));
				foreach (DependencyDeclaration.Package depPackage in dependencyDeclaration.Packages)
				{
					// explicit modules dependencies
					foreach (DependencyDeclaration.Group depGroup in depPackage.Groups)
					{
						foreach (string depModule in depGroup.Modules)
						{
							// find all dependencies that match this declaration
							IEnumerable<Dependency<IModule>> matchingDependencies = module.Dependents.Where(dep =>
								dep.Dependent.Package.Name == depPackage.Name &&
								dep.Dependent.BuildGroup == depGroup.Type &&
								dep.Dependent.Name == depModule);

							if (matchingDependencies.Any())
							{
								foreach (Dependency<IModule> dependency in matchingDependencies)
								{
									dependency.SetType(dependencyType);
								}
							}
							else
							{
								Project.Log.ThrowWarning
								(
									Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Normal,
									"Invalid dependency in the '{0}' property: '{1}/{2}/{3}'. Referenced entry is not found in the {4} module dependencies. " +
									"This missing dependency '{1}/{2}/{3}' might have been caused by inconsistency between {1}'s build file and its initialize.xml file.",
									PropGroupName(propertyName), depPackage.Name, depGroup.Type.ToString(), depModule, module.Name
								);
							}
						}
					}

					// implicit dependency on all runtime modules, or package level information
					if (depPackage.FullDependency)
					{
						// we don't check for public declared modules here as if this is a build dependency then
						// we will have access to private modules as well
						IEnumerable<Dependency<IModule>> matchingDependencies = module.Dependents.Where(dep =>
							dep.Dependent.Package.Name == depPackage.Name &&
							dep.Dependent.BuildGroup == BuildGroups.runtime);

						foreach (Dependency<IModule> dependency in matchingDependencies)
						{
							dependency.SetType(dependencyType);
						}
					}
				}
			}
		}

		private Dictionary<string, BuildGroups> GetPackageDeclaredModules(string packageName)
		{
			var declaredModules = new Dictionary<string, BuildGroups>();
			// Modules declared in Initialize.Xml iterate over ALL build groups!
			string runtimeModulesString = Project.Properties[PropertyKey.Create("package.", packageName, ".buildmodules")] ??
				Project.Properties[PropertyKey.Create(packageName, ".buildmodules")]; // runtime might be declared without group

			if (!runtimeModulesString.IsNullOrEmpty())
			{
				var tempRuntimeModules = runtimeModulesString.ToArray();
				foreach (var mod in tempRuntimeModules)
				{
					declaredModules[mod] = BuildGroups.runtime;
				}
			}

			foreach (BuildGroups bg in s_buildGroupsArray)
			{
				var bgString = bg.ToString();
				var modulesString = Project.Properties[PropertyKey.Create("package.", packageName, ".", bgString, ".buildmodules")] ??
					Project.Properties[PropertyKey.Create(packageName, ".", bgString, ".buildmodules")];
				if (!modulesString.IsNullOrEmpty())
				{
					var tempModules = modulesString.ToArray();
					foreach (var mod in tempModules)
					{
						declaredModules[mod] = bg;
					}
				}
			}
			return declaredModules;
		}

		#region Dispose interface implementation

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		void Dispose(bool disposing)
		{
			if (!this._disposed)
			{
				if (disposing)
				{
					// Dispose managed resources here
				}
			}
			_disposed = true;
		}
		private bool _disposed = false;

		#endregion
	}
}


