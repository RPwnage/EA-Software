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

using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Structured;
using EA.Eaconfig.Modules;
using System.Management.Automation;

namespace EA.Eaconfig.Build
{
	abstract public class ModuleDependencyProcessorBase : ModuleProcessorBase
	{
		protected class PackageDependencyDefinition
		{
			public readonly string Key;
			public readonly string PackageName;
			public readonly string ConfigurationName;

			public PackageDependencyDefinition(string packagename, string configuration)
			{
				PackageName = packagename;
				ConfigurationName = configuration;
				Key = CreateKey(packagename, configuration);
			}

			public override string ToString()
			{
				return Key;
			}

			private static string CreateKey(string packagename, string configuration)
			{
				return packagename + ":" + configuration;
			}			
		}

		protected ModuleDependencyProcessorBase(ProcessableModule module, string logPrefix = "")
			: base(module, logPrefix)
		{
			HasLink = BuildOptionSet.Options["build.tasks"].ToArray().Contains("link") || module.IsKindOf(Module.DotNet);
			AutoBuildUse = Project.Properties.GetBooleanPropertyOrDefault(FrameworkProperties.AutoBuildUseDependencyPropertyName, true);
		}

		protected readonly bool HasLink;
		protected readonly bool AutoBuildUse;

		protected List<Dependency<PackageDependencyDefinition>> GetModuleDependentDefinitions(IModule module, bool excludeThisPackage = false, bool includePublic = false)
		{
			// We need ordered collection here so that dependencies are defined in the same order they are listed in the build file:
			List<Dependency<PackageDependencyDefinition>> depList = new List<Dependency<PackageDependencyDefinition>>();
			Dictionary<string, Dependency<PackageDependencyDefinition>> dependencies = new Dictionary<string, Dependency<PackageDependencyDefinition>>();

			foreach (KeyValuePair<string, uint> propertySuffixToType in Mappings.DependencyPropertyMapping)
			{
				foreach (string dependentPackage in GetModulePropertyValue(propertySuffixToType.Key).ToArray())
				{
					if (excludeThisPackage && dependentPackage == module.Package.Name)
					{
						continue;
					}

					if (Project.UseNugetResolution && PackageMap.Instance.MasterConfigHasNugetPackage(dependentPackage))
					{
						// NUGET-TODO: non-obvious side effect of this function
						// short circuit dependency and instead register this as a nuget reference
						var additionalNugetReferences = module.PropGroupValue("additional-nuget-references").ToArray();
						additionalNugetReferences.Add(dependentPackage);
						module.Package.Project.Properties[module.PropGroupName("additional-nuget-references")] = string.Join("\n", additionalNugetReferences);
						continue;
					}

					if (PackageMap.Instance.TryGetMasterPackage(module.Package.Project, dependentPackage, out MasterConfig.IPackage masterPkg))
					{
						// Looks like we have letter casing discrepancy between masterconfig and user's build script when listing build dependency block.
						// Take masterconfig name as the correct case and warn user.
						if (masterPkg.Name != dependentPackage)
						{
							throw new BuildException(
								string.Format("{0}Build module {1} has dependency to package {2}. " +
									"But this package name is specified in masterconfig using different case: {3}. Please correct your build script. " +
									"The case sensitive package name is being used as 'keys' in too many places including user tasks that is outside Framework's control.",
									LogPrefix, module.Name, dependentPackage, masterPkg.Name)
							);
						}
					}

					if (!GetOrAddPackageDependency(dependencies, dependentPackage, module.Configuration.Name, out Dependency<PackageDependencyDefinition> depDef))
					{
						depList.Add(depDef);
					}

					depDef.SetType(propertySuffixToType.Value);
					if (depDef.IsKindOf(DependencyTypes.AutoBuildUse))
					{
						if (AutoBuildUse)
						{
							if (HasLink)
							{
								depDef.SetType(DependencyTypes.Build);
							}
						}
						else
						{
							depDef.SetType(DependencyTypes.Build);
							depDef.ClearType(DependencyTypes.AutoBuildUse);
						}
					}
				}
			}
			if (includePublic)
			{
				string publicBuildDeps = Project.Properties.GetPropertyOrDefault(String.Format("package.{0}.{1}.{2}", module.Package.Name, module.Name, "publicbuilddependencies"), string.Empty);
				DependencyDeclaration publicDependencies = new DependencyDeclaration(publicBuildDeps);
				foreach (DependencyDeclaration.Package package in publicDependencies.Packages)
				{
					if (excludeThisPackage && package.Name == module.Package.Name)
					{
						continue;
					}

					if (!GetOrAddPackageDependency(dependencies, package.Name, module.Configuration.Name, out Dependency<PackageDependencyDefinition> depDef))
					{
						depList.Add(depDef);
					}
					depDef.SetType(DependencyTypes.Build);
				}
			}
			return depList;
		}

		private bool GetOrAddPackageDependency(Dictionary<string, Dependency<PackageDependencyDefinition>> dependencies, string packageName, string configuration, out Dependency<ModuleDependencyProcessorBase.PackageDependencyDefinition> depDef)
		{
			bool dependencyAlreadyExists = false;

			string key = DependencyDefinition.CreateKey(packageName, configuration);
			if (!dependencies.TryGetValue(key, out depDef))
			{
				depDef = new Dependency<PackageDependencyDefinition>(new PackageDependencyDefinition(packageName, configuration));
				dependencies.Add(key, depDef);
			}
			else
			{
				dependencyAlreadyExists = true;
			}
			return dependencyAlreadyExists;
		}

		internal static void RefineDependencyTypeFromConstraint(BitMask packagedep, BitMask constraint)
		{
			uint newtype = packagedep.Type & constraint.Type;
			packagedep.ClearType(packagedep.Type);
			packagedep.SetType(newtype);
		}

		// Store this outside since it is actually quite expensive
		static BuildGroups[] BuildGroupsArray = (BuildGroups[])Enum.GetValues(typeof(BuildGroups));

		protected List<ModuleDependencyConstraints> GetModuleDependenciesConstraints(Project project, IModule module, string dependentPackage)
		{
			List<ModuleDependencyConstraints> constraints = null;
			// Set moduledependencies for dependent package(constraints). Dependencies between modules in the same package are set elswhere
			foreach (BuildGroups dependentGroup in BuildGroupsArray)
			{
				Dictionary<uint, string> dependencyIndexToConstraints = null;
				var dependentGroupStr = dependentGroup.ToString();

				using (var scope = project.Properties.ReadScope())
				{
					foreach (var mapping in Mappings.ModuleDependencyConstraintsPropertyMapping)
					{
						var value = scope[PropertyKey.Create(module.GroupName, ".", dependentPackage, ".", dependentGroupStr, mapping.Key)];
						if (String.IsNullOrEmpty(value))
							continue;
						if (dependencyIndexToConstraints == null)
							dependencyIndexToConstraints = new Dictionary<uint, string>();
						dependencyIndexToConstraints.Add(mapping.Value, value);
					}
				}

				if (dependencyIndexToConstraints == null)
					continue;

				foreach (KeyValuePair<uint, string> dependencyIndexToConstraint in dependencyIndexToConstraints)
				{
					foreach (string dependentModule in dependencyIndexToConstraint.Value.ToArray())
					{
						if (constraints == null)
						{
							constraints = new List<ModuleDependencyConstraints>();
						}

						var depDef = constraints.FirstOrDefault(c => c.Group == dependentGroup && c.ModuleName == dependentModule);

						if (depDef == null)
						{
							depDef = new ModuleDependencyConstraints(dependentGroup, dependentModule, dependencyIndexToConstraint.Key);
							constraints.Add(depDef);
						}
						else
						{
							depDef.SetType(dependencyIndexToConstraint.Key);
						}

						if (depDef.IsKindOf(DependencyTypes.AutoBuildUse))
						{
							if (AutoBuildUse)
							{
								if (HasLink)
								{
									depDef.SetType(DependencyTypes.Build);
								}
							}
							else
							{
								depDef.SetType(DependencyTypes.Build);
								depDef.ClearType(DependencyTypes.AutoBuildUse);
							}
						}
					}
				}
			}
			return constraints;
		}
	}
}