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
using System.Collections.Generic;
using System.Collections.Concurrent;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Structured;

namespace EA.Eaconfig.Build
{
	internal class ModuleProcessor_AddModuleDependentsPublicData : ModuleProcessorBase, IModuleProcessor, IDisposable
	{
		private readonly ConcurrentDictionary<string, DependentCollection> m_transitiveCollection;

		internal ModuleProcessor_AddModuleDependentsPublicData(ProcessableModule module, ref ConcurrentDictionary<string, DependentCollection> transitiveCollection, string logPrefix)
			: base(module, logPrefix)
		{
			m_transitiveCollection = transitiveCollection;
		}

		public static void VerifyPackagePublicData(Package package, string logPrefix)
		{
			var verification_supression_property = "package." + package.Name + ".supress-public-data-verification";

			if (package != null && !package.Project.Properties.GetBooleanPropertyOrDefault(verification_supression_property, false))
			{
				var packageBuildLibs = new List<PathString>();
				var packageBuildDlls = new List<PathString>();

				foreach (var module in package.Modules)
				{
					if (module is Module_UseDependency)
						continue;

					var libOut = module.LibraryOutput();
					if (libOut != null)
						packageBuildLibs.Add(libOut);
					var asmOut = module.DynamicLibOrAssemblyOutput();
					if (asmOut != null)
						packageBuildDlls.Add(asmOut);
				}

				foreach (var module in package.Modules.Where(m => (m is Module_Native || m is Module_DotNet)))
				{
					var outputlib = module.LibraryOutput();
					var outputdll = module.DynamicLibOrAssemblyOutput();

					var removedlibraries = new HashSet<PathString>(); // Collect all removed libs to print warning.
					var removeddlls = new HashSet<PathString>(); // Collect all removed assemblies to print warning.

					var proceccedLibs = new HashSet<PathString>();
					bool foundlib = false;
					string libList = String.Empty;

					foreach (var data in package.GetModuleAllPublicData(module))
					{
						// ------------ Test libraries ------------------------------------------------------------
						if (data.Libraries != null && data.Libraries.Includes.Count > 0)
						{
							var suspectlibs = (outputlib == null) ? packageBuildLibs : packageBuildLibs.Where(lib => outputlib != lib);
							if (suspectlibs.Count() > 0)
							{
								var cleanedPublicLib = new FileSet();
								foreach (var fileItem in data.Libraries.FileItems)
								{
									if (!suspectlibs.Contains(fileItem.Path))
									{
										cleanedPublicLib.Include(fileItem);
									}
									else
									{
										if (package.Project.Log.WarningLevel >= Log.WarnLevel.Advise)
										{
											removedlibraries.Add(fileItem.Path);
										}
									}
								}

								cleanedPublicLib.FailOnMissingFile = false;
								((PublicData)data).Libraries = cleanedPublicLib;
							}

							// Check consistency of the exported library files in initialize.xml files. 
							// This check should only be applied to modules in packages different from module.Package
							if (!foundlib && outputlib != null && !data.Libraries.FileItems.Any(fi => fi.Path == outputlib))
							{
								if (proceccedLibs.Add(outputlib))
								{
									libList += Environment.NewLine;
									var padding = String.Empty.PadLeft(Log.IndentLength * package.Project.Log.IndentLevel + 18);
									if (data.Libraries.FileItems.Count > 0)
									{
										libList += String.Format(String.Empty.PadLeft(Log.IndentLength * package.Project.Log.IndentLevel + 12) + "Library file(s) defined in the Initialize.xml:{0}{1}", Environment.NewLine, data.Libraries.FileItems.ToString(Environment.NewLine, fi => padding + fi.Path.Path));
									}
									else if (data.Libraries.Includes.Count > 0)
									{
										libList += String.Format(String.Empty.PadLeft(Log.IndentLength * package.Project.Log.IndentLevel + 12) + "Library fileset pattern(s) defined in the Initialize.xml:{0}{1}", Environment.NewLine, data.Libraries.Includes.ToString(Environment.NewLine, inc => padding + inc.Pattern.Value));
									}
								}
							}
							else
							{
								foundlib = true;
							}
						}
						else if (outputlib == null)
						{
							foundlib = true;
						}
							
						else
						{
							//IMTODO: I need to remove this if. But to do proper check we need to see if any other module has a Link dependency on this one. Otherwise public data are empty.
							// Probably need to store suspects in some array and verify later when dependencies are processed.
							foundlib = true;
						}

						// ------------ Test assemblies------------------------------------------------------------
						if (data.Assemblies != null && data.Assemblies.Includes.Count > 0)
						{
							var suspectdlls = (outputdll == null) ? packageBuildDlls : packageBuildDlls.Where(dll => outputdll != dll);
							if (suspectdlls.Count() > 0)
							{
								var cleanedPublicDlls = new FileSet();
								foreach (var fileItem in data.Assemblies.FileItems)
								{
									if (!suspectdlls.Contains(fileItem.Path))
									{
										cleanedPublicDlls.Include(fileItem);
									}
									else
									{
										removeddlls.Add(fileItem.Path);
									}
								}

								cleanedPublicDlls.FailOnMissingFile = false;
								((PublicData)data).Assemblies = cleanedPublicDlls;
							}
						}
					}
					if (!foundlib)
					{
						if (package.Project.Log.WarningLevel >= Log.WarnLevel.Advise)
						{
							package.Project.Log.Warning.WriteLine("Module {0} builds library '{1}', but it looks like it does not export this library in the initialize.xml script.{2}", module.ModuleIdentityString(), outputlib, libList);
						}
					}

					if (package.Project.Log.WarningLevel >= Log.WarnLevel.Advise)
					{
						if (removedlibraries.Count > 0)
						{
							package.Project.Log.Warning.WriteLine(logPrefix + "package {0}-{1} ({2}) {3}:{4}\t\t\tFollowing public libraries defined in Initialize.xml file {4}{4}{5}{4}{4}\t\t\tare removed from the module '{3}' public data because these libraries are built by other modules in the same package.{4}\t\t\tFix Initialize.xml file by defining libs per module, or, if this behavior is intended - suppress autoremoval functionality for the package - define property '{6}=true' in the package build script.", package.Name, package.Version, package.ConfigurationName, module.BuildGroup + "." + module.Name, Environment.NewLine, removedlibraries.ToString(Environment.NewLine, ps => "\t\t\t\t\t" + ps.Path), verification_supression_property);
						}
						if (removeddlls.Count > 0)
						{
							package.Project.Log.Warning.WriteLine(logPrefix + "package {0}-{1} ({2}) {3}:{4}\t\t\tFollowing public assemblies defined in Initialize.xml file {4}{4}{5}{4}{4}\t\t\tare removed from the module '{3}' public data because these assemblies are built by other modules in the same package.{4}\t\t\tFix Initialize.xml file by defining assemblies per module, or, if this behavior is intended - suppress autoremoval functionality for the package - define property '{6}=true' in the package build script.", package.Name, package.Version, package.ConfigurationName, module.BuildGroup + "." + module.Name, Environment.NewLine, removeddlls.ToString(Environment.NewLine, ps => "\t\t\t\t\t" + ps.Path), verification_supression_property);
						}
					}
				}
			}
		}


		public void Process(ProcessableModule module)
		{
			module.Apply(this);

			// ValidatePublicDependencies(module, "publicdependencies", DependencyTypes.Undefined, "interface");
			// ValidatePublicDependencies(module, "publicbuilddependencies", DependencyTypes.Build | DependencyTypes.Link | DependencyTypes.AutoBuildUse, "auto/link/build");
		}

		private void ValidatePublicDependencies(ProcessableModule module, string dependencyPropertyName, uint dependencyType, string dependencyTypeDescription)
		{
			string fullPropertyName = String.Format("package.{0}.{1}.{2}", module.Package.Name, module.Name, dependencyPropertyName);
			string propertyValue = Project.Properties[fullPropertyName];
			if (!String.IsNullOrWhiteSpace(propertyValue)) // if this is not set it could mean that is was never set OR that this package does not initialize it's own public data - we can only validate if latter us true
			{
				DependencyDeclaration dependencyDeclaration = new DependencyDeclaration(propertyValue);
				foreach (DependencyDeclaration.Package package in dependencyDeclaration.Packages)
				{
					if (package.FullDependency && !module.Dependents.Any(dep => dep.Dependent.Package.Name.ToLowerInvariant() == package.Name.ToLowerInvariant()))
					{
						Project.Log.ThrowWarning
						(
							Log.WarningId.InvalidPublicDependencies, Log.WarnLevel.Normal,
							"Module {0} in package {1} declares public dependency on package {2} in it's Initialize.xml but does not actually depend on this package!",
							module.Name, module.Package.Name, package.Name
						);
					}
					foreach (DependencyDeclaration.Group group in package.Groups)
					{
						foreach (string publicDepModule in group.Modules)
						{
							Dependency<IModule> dependency = module.Dependents.FirstOrDefault
							(
								dep => 
								dep.Dependent.Package.Name == package.Name && 
								dep.Dependent.BuildGroup == group.Type &&
								dep.Dependent.Name == publicDepModule
							);
							bool dependencyFound = dependency != null;
							if (!dependencyFound)
							{
								Log.ThrowWarning
								(
									Log.WarningId.InvalidPublicDependencies, Log.WarnLevel.Normal,
									$"Module {module.Name} in package {module.Package.Name} declares {dependencyPropertyName} on module {publicDepModule} from package {package.Name} in it's Initialize.xml but does not actually depend on this module!"
								);
							}
							else if (dependencyType != DependencyTypes.Undefined && !dependency.IsKindOf(dependencyType))
							{
								Log.ThrowWarning
								(
									Log.WarningId.InvalidPublicDependencies, Log.WarnLevel.Normal,
									$"Module {module.Name} in package {module.Package.Name} declares {dependencyPropertyName} on module {publicDepModule} from package {package.Name} in it's Initialize.xml but it has no dependency of the correct type to make public. " +
									$"You should only declare {dependencyPropertyName} for modules on which you have a {dependencyTypeDescription} dependency."
								);
							}
						}
					}
				}
			}
		}

		public void Process(Module_Native module)
		{
			ProcessDependents(module);

			AddSystemIncludes(module.Cc);
			AddSystemIncludes(module.Asm);
		}

		public void Process(Module_DotNet module)
		{
			if (module.Compiler != null)
			{
				module.TraverseDependencies((stack, parent, dependency) =>
				{
					var propagatedDependency = dependency;

					if (m_transitiveCollection != null && !(dependency.Dependent is Module_UseDependency) && parent.Key != module.Key)
					{
						propagatedDependency = new Dependency<IModule>(dependency);

						if (dependency.IsKindOf(DependencyTypes.AutoBuildUse) && dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
						{
							propagatedDependency.SetType(DependencyTypes.Build);
						}

						if (!dependency.IsKindOf(DependencyTypes.Build) && dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
						{
							propagatedDependency.SetType(DependencyTypes.Build);
						}

						//We only need to propagate build dependencies here
						if (propagatedDependency.IsKindOf(DependencyTypes.Build))
						{
							propagatedDependency.ClearType(DependencyTypes.Interface); // We don't propagate interface

							m_transitiveCollection.AddOrUpdate(module.Key,
								(mkey) => { var collection = new DependentCollection(module); collection.Add(propagatedDependency); return collection; },
								(mkey, mcollection) => { mcollection.Add(propagatedDependency); return mcollection; });
						}
					}

					if (dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
					{
						module.Compiler.DependentAssemblies.AppendIfNotNull(dependency.Dependent.Public(parent).Assemblies);
					}

					// Do we need to process dependents of 'dependency' (true/false):
					bool transitiveLinkUtilModule = false;
					if (dependency.Dependent is Module_Utility)
					{
						transitiveLinkUtilModule = (dependency.Dependent as Module_Utility).TransitiveLink;
					}
					else if (dependency.Dependent is Module_MakeStyle)
					{
						transitiveLinkUtilModule = (dependency.Dependent as Module_MakeStyle).TransitiveLink;
					}
					bool ret = (!dependency.Dependent.IsKindOf(Module.CSharp | Module.DynamicLibrary | Module.Program | Module.Utility | Module.MakeStyle)
								 || transitiveLinkUtilModule)
							&& propagatedDependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Build);

					return ret;

				},
				isTransitive: m_transitiveCollection != null);

				// For modules in the same package public data may not be defined. Add dependent assemblies
				foreach (var d in module.Dependents.Where(dep => dep.Dependent.Package.Key == module.Package.Key))
				{
					if (d.IsKindOf(DependencyTypes.Link))  // Add Libraries
					{
						if (d.Dependent.IsKindOf(Module.DotNet | Module.Managed))
						{
							var assembly = d.Dependent.DynamicLibOrAssemblyOutput();
							if (assembly != null)
							{
								module.Compiler.DependentAssemblies.Include(assembly.Path, failonmissing: false);
							}
						}
					}
				}

			}

		}

		public void Process(Module_Utility module)
		{
			AddTransitiveDependencies(module);
		}

		public void Process(Module_MakeStyle module)
		{
			AddTransitiveDependencies(module);
		}

		public void Process(Module_Python module)
		{
			AddTransitiveDependencies(module);
		}

		public void Process(Module_ExternalVisualStudio module)
		{
			AddTransitiveDependencies(module);
		}

		public void Process(Module_Java module)
		{
			if (module.IsKindOf(Module.Apk))
			{
				AddTransitiveDependencies(module);
			}
		}

		private void AddTransitiveDependencies(ProcessableModule module)
		{
			if (m_transitiveCollection != null)
			{
				foreach (var dep in module.Dependents.Flatten(DependencyTypes.Build))
				{
					if (!(dep.Dependent is Module_UseDependency) && dep.Dependent.Key != module.Key)
					{
						m_transitiveCollection.AddOrUpdate(module.Key,
							(mkey) => { var collection = new DependentCollection(module); collection.Add(dep); return collection; },
							(mkey, mcollection) => { mcollection.Add(dep); return mcollection; });
					}
				}
			}
		}

		public void Process(Module_UseDependency module)
		{
			throw new BuildException($"{LogPrefix}Trying to process use dependency module. This is most likely a bug.");
		}

		#region NativeModule

		private void ProcessDependents(Module_Native module)
		{
			List<PathString> dependentIncludeDirs = new List<PathString>();
			List<PathString> dependentPchIncludeDirs = new List<PathString>();
			List<PathString> dependentLibraryDirs = new List<PathString>();
			List<PathString> dependentUsingDirs = new List<PathString>();
			List<string> dependentDefines = new List<string>();
			FileSet dependentAssemblies = new FileSet();
			FileSet dependentNatvisFiles = new FileSet();

			List<PathString> includeDirsPtr;
			// Dependents include directories are not transitive
			foreach (var d in module.Dependents.Where(dep => dep.Dependent.Public(module) != null).OrderBy(x => (x.Dependent as ProcessableModule).GraphOrder))
			{
				includeDirsPtr = dependentIncludeDirs;
				if (d.IsKindOf(DependencyTypes.Interface))  // Add IncludeDirectories
				{
					if (module.Cc != null || module.Asm != null)
					{
						if (module.AdditionalData.SharedPchData != null)
						{
							if (d.Dependent.Name == module.AdditionalData.SharedPchData.SharedPchModule
								&& d.Dependent.Package.Name == module.AdditionalData.SharedPchData.SharedPchPackage)
							{
								includeDirsPtr = dependentPchIncludeDirs;
							}
						}
						IPublicData modulePublicData = d.Dependent.Public(module);

						if (modulePublicData.IncludeDirectories != null)
						{
							includeDirsPtr.AddRange(modulePublicData.IncludeDirectories);
						}
					}
				}
			}

			module.TraverseDependencies((stack, parent, dependency) =>
			{
				//module.Package.Project.Log.Status.WriteLine("+++ {0}", stack.FormatDependencyStack(parent, dependency));

				var propagated = PropagateDependency(module, stack, parent, dependency);

				if (propagated == null)
				{
					return false;
				}
				
				// TODO dangerous public data traversal
				// -dvaliant 2016/04/04
				/* 
					accessing public data inside of the TraverseDependencies functions is potentially
					going to cause a bug:
					*	if A depends on B and C
					*	and B and C depends on D
					*	B and C's "view" of D's public data can be different (though *usually* isn't)
					*	only one of these "views" will be evaluated base on which dependency paths we
						traverse
				*/
				IPublicData publicData = propagated.Dependent.Public(parent); 

				if (publicData != null)
				{
					if (propagated.IsKindOf(DependencyTypes.Interface)) // note only first level dependencies will have propagated interface dependency
					{
						if (module.Cc != null)
						{
							if (publicData.Defines != null)
							{
								dependentDefines.AddRange(publicData.Defines);
							}
							if (publicData.UsingDirectories != null)
							{
								dependentUsingDirs.AddRange(publicData.UsingDirectories);
							}
						}
						module.SnDbsRewriteRulesFiles.AppendIfNotNull(publicData.SnDbsRewriteRulesFiles);
					}

					if (propagated.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface) && module.IsKindOf(Module.Managed))
					{
						if (module.Cc != null)
						{
							dependentAssemblies.AppendIfNotNull(publicData.Assemblies);
						}
					}

					if (propagated.IsKindOf(DependencyTypes.Link))  // Add Libraries
					{
						if (module.Link != null)
						{
							module.Link.Libraries.AppendIfNotNull(publicData.Libraries);

							// Check for incorrect usage of ScriptInit Task:
							if (propagated.Dependent.Package.IsKindOf(Package.AutoBuildClean) && propagated.Dependent.Package.Modules.Any(m => !(m is Module_UseDependency)))
							{
								var defaultScriptInitLib = module.Package.Project.Properties["package." + propagated.Dependent.Package.Name + ".default.scriptinit.lib"];
								if (defaultScriptInitLib != null && !propagated.Dependent.Package.Modules.Any(m => m.Name == propagated.Dependent.Package.Name))
								{
									module.Link.Libraries.Exclude(defaultScriptInitLib);

									if (Log.WarningLevel >= Log.WarnLevel.Advise)
									{
										Log.Warning.WriteLine("Processing '{0}':  Initialize.xml from package '{1}' invokes task ScriptInit without 'Libs' parameter. ScriptInit in this case exports library '{2}', but package does not have any modules with this name. Fix your Initialize.xml file.",
												module.GroupName + "." + module.Name, propagated.Dependent.Package.Name, defaultScriptInitLib);
									}
								}
							}

							if (publicData.LibraryDirectories != null)
							{
								dependentLibraryDirs.AddRange(publicData.LibraryDirectories);
							}

							dependentNatvisFiles.AppendWithBaseDir(publicData.NatvisFiles);
						}
					}
				}

				// Do we need to process dependents of 'dependency' (true/false):
				bool transitiveLinkUtilModule = false;
				if (dependency.Dependent is Module_Utility)
				{
					transitiveLinkUtilModule = (dependency.Dependent as Module_Utility).TransitiveLink;
				}
				else if (dependency.Dependent is Module_MakeStyle)
				{
					transitiveLinkUtilModule = (dependency.Dependent as Module_MakeStyle).TransitiveLink;
				}
				bool ret = (!dependency.Dependent.IsKindOf(Module.CSharp | Module.DynamicLibrary | Module.Program | Module.Utility | Module.MakeStyle)
							 || transitiveLinkUtilModule)
						&& propagated.IsKindOf(DependencyTypes.Link | DependencyTypes.Build);

				return ret;
			},
			isTransitive: m_transitiveCollection != null
			);


			foreach (var d in module.Dependents.Where(dep => dep.Dependent.Package.Key == module.Package.Key))
			{
				if (d.IsKindOf(DependencyTypes.Link))  // Add Libraries
				{
					if (module.Link != null)
					{
						// Note that this is executing in parallel so Dependent can actually add tools while LibraryOutput is called!
						var library = d.Dependent.LibraryOutput();
						if (library != null)
						{
							module.Link.Libraries.Include(library.Path, failonmissing: false);
						}
					}

					if (module.IsKindOf(Module.Managed) && d.Dependent.IsKindOf(Module.DotNet | Module.Managed))
					{
						// Note that this is executing in parallel so Dependent can actually add tools while LibraryOutput is called!
						var assembly = d.Dependent.DynamicLibOrAssemblyOutput();
						if (assembly != null)
						{
							dependentAssemblies.Include(assembly.Path, failonmissing: false);
						}
					}
				}
			}

			// Add module Cc and Asm data:

			var includeDirs = dependentIncludeDirs.OrderedDistinct();
			var pchIncludeDirs = dependentPchIncludeDirs.OrderedDistinct();
			var defines = dependentDefines.Distinct();
			var usingDirs = dependentUsingDirs.OrderedDistinct();
			if (module.Cc != null)
			{
				module.Cc.IncludeDirs.AddUnique(includeDirs);
				module.Cc.IncludeDirs.InsertRange(0, pchIncludeDirs);
				module.Cc.Defines.AddUnique(defines);
				module.Cc.UsingDirs.AddUnique(usingDirs);
				module.Cc.Assemblies.Append(dependentAssemblies);

				foreach (var cc in module.Cc.SourceFiles.FileItems.Select(fi => fi.GetTool() as CcCompiler).Where(tool => tool != null))
				{
					cc.IncludeDirs.AddUnique(includeDirs);		
					cc.IncludeDirs.InsertRange(0, pchIncludeDirs);					
					cc.Defines.AddUnique(defines);
					cc.UsingDirs.AddUnique(usingDirs);
					cc.Assemblies.Append(dependentAssemblies);
				}
			}

			if (module.Asm != null)
			{
				module.Asm.IncludeDirs.AddUnique(includeDirs);
				module.Asm.Defines.AddUnique(defines);

				foreach (var asm in module.Asm.SourceFiles.FileItems.Select(fi => fi.GetTool() as AsmCompiler).Where(tool => tool != null))
				{
					asm.IncludeDirs.AddUnique(includeDirs);
					asm.Defines.AddUnique(defines);
				}
			}
			if (module.Link != null)
			{
				module.Link.LibraryDirs.AddUnique(dependentLibraryDirs.OrderedDistinct());
				module.NatvisFiles.AppendIfNotNull(dependentNatvisFiles);
			}
		}

		private List<string> RemoveDuplicateDefines(Module_Native module, IEnumerable<string> defines)
		{
			List<string> filteredDefines = new List<string>();
			Dictionary<string, string> moduleDefineLookup = new Dictionary<string, string>();
			foreach (var moduleDefine in module.Cc.Defines)
			{
				var moduleDefineSplit = moduleDefine.Split('=');
				if (moduleDefineSplit.Length < 2)
					continue;
				string name = moduleDefineSplit[0].Trim().ToUpper();
				if (moduleDefineLookup.ContainsKey(name) == false)
				{
					moduleDefineLookup.Add(name, moduleDefineSplit[1].Trim().ToUpper());
				}
			}

			foreach (var define in defines)
			{
				var defineSplit = define.Split('=');
				if (defineSplit.Length < 2)
				{
					filteredDefines.Add(define);
					continue;
				}

				string name = defineSplit[0].Trim().ToUpper();
				string value = defineSplit[1].Trim().ToUpper();
				if (moduleDefineLookup.ContainsKey(name))
				{
					if (value != moduleDefineLookup[name])
					{
						if (Log.WarningLevel >= Log.WarnLevel.Advise)
						{
							module.Package.Project.Log.Warning.WriteLine(LogPrefix + "Duplicate define removed: \r\n{0}", define);
						}
						continue;
					}
				}
				filteredDefines.Add(define);
			}
			return filteredDefines;
		}

		private Dependency<IModule> PropagateDependency(Module_Native module, Stack<Dependency<IModule>> stack, IModule parent, Dependency<IModule> dep)
		{
			var propagated = dep;

			bool dependentBuildFileIsLoaded = !(dep.Dependent is Module_UseDependency);

			if(module.Key == dep.Dependent.Key)
			{
				if (Log.WarningLevel >= Log.WarnLevel.Advise)
				{
					module.Package.Project.Log.Warning.WriteLine(LogPrefix + "Skipped transitive propagation '{0}' module '{1}' shouldn't depend on itself.",
						stack.FormatDependencyStack(module, parent, dep), module.Name);
				}
				return null;

			}

			if (parent.Key != module.Key)  // If this is immediate dependent - do not ad any manipulation with dependency
			{
				if (!dependentBuildFileIsLoaded && dep.Dependent.Package.IsKindOf(EA.FrameworkTasks.Model.Package.AutoBuildClean) && dep.IsKindOf(DependencyTypes.Link))
				{
					if (Log.WarningLevel >= Log.WarnLevel.Advise)
					{
						DoOnce.Execute(String.Format("skipped.transitive.propagation.usedependency.warning.{0},{1},{2}", parent.BuildGroup, parent.Name, dep.Dependent.Package.Name), () =>
						{
							module.Package.Project.Log.Warning.WriteLine(LogPrefix + "Skipped transitive propagation '{0}' (or one of its modules). Looks like module '{1}.{2}' declares use dependency on buildable package '{3}'. Use dependency implies linking but package '{3} is not built because no build dependency was defined. Correct build scripts to use interfacedependency (or builddependency) instead of usedependency",
								stack.FormatDependencyStack(module, parent, dep), parent.BuildGroup, parent.Name, dep.Dependent.Package.Name);
						});
					}
					return null;
				}
			
				//I only need to propagate build dependencies here when I have link or managed module needs Assembly for compilation:
				if (m_transitiveCollection != null && ( (module.Link != null && dep.IsKindOf(DependencyTypes.Link)) || (dep.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface) && module.IsKindOf(Module.Managed))) )
				{
					propagated = new Dependency<IModule>(dep);

					if (module.Link != null && dep.IsKindOf(DependencyTypes.AutoBuildUse) && dep.IsKindOf(DependencyTypes.Link))
					{
						propagated.SetType(DependencyTypes.Build);
					}

					if (module.Link != null && !dep.IsKindOf(DependencyTypes.Build) && dep.IsKindOf(DependencyTypes.Link))
					{
						propagated.SetType(DependencyTypes.Build);
					}

					if ((dep.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface) && module.IsKindOf(Module.Managed)) && dep.IsKindOf(DependencyTypes.AutoBuildUse))
					{
						propagated.SetType(DependencyTypes.Build);
					}

					if (module.IsKindOf(Module.Managed | Module.DotNet) && parent.IsKindOf(Module.Library) && (dep.Dependent.IsKindOf(Module.Managed | Module.DotNet) || !dependentBuildFileIsLoaded))
					{
						// Link dependency on Managed assembly should not propagate through native library
						propagated.ClearType(DependencyTypes.Link);

						if (!dependentBuildFileIsLoaded)
						{
							if (!module.Dependents.Any(md => md.Dependent.Key == dep.Dependent.Key && dep.IsKindOf(DependencyTypes.Link)))
							{
								module.Package.Project.Log.Warning.WriteLine(LogPrefix + "Skipped transitive propagation '{0}''. Package '{1}' is non buildable (or autobuildclean=false), can't determine if it is appropriate to propagate (managed) <= (native) <= (unknown type)", stack.FormatDependencyStack(module, parent, dep), dep.Dependent.Package.Name);
							}
						}
					}

					if (module.Package.Project.Log.Level >= Log.LogLevel.Detailed)
					{
						Dependency<IModule> existingDep;

						if (module.Dependents.TryGet(dep.Dependent, out existingDep))
						{
							if (existingDep.Type != (existingDep.Type | propagated.Type))
							{
								module.Package.Project.Log.Status.WriteLine(LogPrefix + "Transitive propagation '{0}', dependency type changed from '{1}' to '{2}'.", stack.FormatDependencyStack(module, parent, dep), DependencyTypes.ToString(existingDep.Type), DependencyTypes.ToString(existingDep.Type|propagated.Type));
							}
						}
						else
						{
							module.Package.Project.Log.Status.WriteLine(LogPrefix + "Transitive propagation '{0}', new dependency '{1}'.", stack.FormatDependencyStack(module, parent, dep), DependencyTypes.ToString(propagated.Type));
						}
					}

					//We only need to propagate build dependencies here
					if (propagated.IsKindOf(DependencyTypes.Build))
					{
						propagated.ClearType(DependencyTypes.Interface); // We don't propagate interface

						m_transitiveCollection.AddOrUpdate(module.Key,
							(mkey) => { var collection = new DependentCollection(module); collection.Add(propagated); return collection; },
							(mkey, mcollection) => { mcollection.Add(propagated); return mcollection; });
					}
				}
			}

			return propagated;
		}

		private void AddSystemIncludes(CcCompiler tool)
		{
			if (tool != null)
			{
				bool prepend = Project.Properties.GetBooleanPropertyOrDefault(tool.ToolName + ".usepreincludedirs", false);

				var systemincludes = GetOptionString(tool.ToolName + ".includedirs", BuildOptionSet.Options).LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
				if (prepend)
					tool.IncludeDirs.InsertRange(0, systemincludes);
				else
					tool.IncludeDirs.AddUnique(systemincludes);

				foreach (var filetool in tool.SourceFiles.FileItems.Select(fi => fi.GetTool() as CcCompiler).Where(t => t != null))
				{
					if (prepend)
						filetool.IncludeDirs.InsertRange(0, systemincludes);
					else
						filetool.IncludeDirs.AddUnique(systemincludes);
				}
			}
		}

		private void AddSystemIncludes(AsmCompiler tool)
		{
			if (tool != null)
			{
				bool prepend = Project.Properties.GetBooleanPropertyOrDefault("as.usepreincludedirs", false);

				var systemincludes = GetOptionString("as.includedirs", BuildOptionSet.Options).LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
				if (prepend)
					tool.IncludeDirs.InsertRange(0, systemincludes);
				else
					tool.IncludeDirs.AddUnique(systemincludes);

				foreach (var filetool in tool.SourceFiles.FileItems.Select(fi => fi.GetTool() as AsmCompiler).Where(t => t != null))
				{
					if (prepend)
						filetool.IncludeDirs.InsertRange(0, systemincludes);
					else
						filetool.IncludeDirs.AddUnique(systemincludes);
				}
			}
		}


		#endregion

		#region Helper functions


		#endregion

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
