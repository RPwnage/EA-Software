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
using NAnt.Core.Util;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;

namespace EA.Eaconfig.Build
{
	public sealed class ProcessModuleAssetFiles : ModuleProcessorBase
	{
		private readonly Module Module;

		public ProcessModuleAssetFiles(Module module, string logPrefix)
			: base(module as ProcessableModule, logPrefix)
		{
			Module = module;
		}

		public bool IsDeployAssets
		{
			get
			{
				var defaultVal = BuildOptionSet.GetBooleanOptionOrDefault("deployassets", Module.IsKindOf(Module.Program));

				Module.DeployAssets = Module.GetModuleBooleanPropertyValue("deployassets", defaultVal);

				return Module.DeployAssets;
			}
		}


		public List<string> GetModuleAssetFileSetNames()
		{
			var assetDependencyDefinitions = new List<DependencyDef>();

			AddDependencyDefinitions_FromAssetDependencies(assetDependencyDefinitions);

			AddDependencyDefinitions_FromModuleDependencies(assetDependencyDefinitions);

			var filesetNames = GetAssetFilesetNames_FromDependents(assetDependencyDefinitions.OrderedDistinct().ToList());

			GetAssetFilesetNames_FromThisModule(filesetNames);

			return filesetNames;
		}

		public void SetModuleAssetFiles()
		{
			foreach (var fsname in GetModuleAssetFileSetNames())
			{
				FileSet fs;
				if (Project.NamedFileSets.TryGetValue(fsname, out fs))
				{
					Module.Assets = Module.Assets.MergeWithBaseDir(fs);
				}
			}
		}

		private void GetAssetFilesetNames_FromThisModule(List<string> filesetNames)
		{
			var assetFileSetsProp = Module.Package.Project.GetPropertyValue(PropGroupName(".assetfiles-set." + Module.Configuration.System))
									?? Module.Package.Project.GetPropertyValue(PropGroupName(".assetfiles-set"));

			FileSet assets;
			var assetFileSets = assetFileSetsProp.ToArray();

			if (assetFileSets.Count > 0)
			{
				Log.Status.WriteLine(LogPrefix + "+ {0}", Module.GroupName + ".assetfiles-set");

				foreach (var fsName in assetFileSets)
				{
					if (Module.Package.Project.NamedFileSets.TryGetValue(fsName, out assets))
					{
						if (!filesetNames.Contains(fsName))
						{
							filesetNames.Add(fsName);
							Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsName, assets.FileItems.Count);
						}
					}
				}
			}
			var fsname = PropGroupName(".assetfiles." + Module.Configuration.System);
			if (Module.Package.Project.NamedFileSets.TryGetValue(fsname, out assets))
			{
				if (!filesetNames.Contains(fsname))
				{
					filesetNames.Add(fsname);
					Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsname, assets.FileItems.Count);
				}
			}
			else
			{
				fsname = PropGroupName(".assetfiles");

				if (Module.Package.Project.NamedFileSets.TryGetValue(fsname, out assets))
				{
					if (!filesetNames.Contains(fsname))
					{
						filesetNames.Add(fsname);
						Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsname, assets.FileItems.Count);
					}
				}
			}
		}


		private void AddDependencyDefinitions_FromAssetDependencies(List<DependencyDef> assetDependencyDefinitions)
		{
			var assetDependencies = Module.GetPlatformModuleProperty(".assetdependencies").ToArray();

			int startDependenciesCount = assetDependencyDefinitions.Count;

			foreach (var dependency in assetDependencies)
			{
				var components = dependency.Trim(new char[] { '"', '\'' }).ToArray(new char[] { '/' });

				switch (components.Count)
				{
					case 1: // "PackageName"
						assetDependencyDefinitions.Add(new DependencyDef(components[0], null, BuildGroups.runtime));
						break;
					case 2: // "PackageName/ModuleName"
						assetDependencyDefinitions.Add(new DependencyDef(components[0], components[1], BuildGroups.runtime));
						break;
					case 3: // "PackageName/group/ModuleName"
						assetDependencyDefinitions.Add(new DependencyDef(components[0], components[2], ConvertUtil.ToEnum<BuildGroups>(components[1])));
						break;
					default:
						if (components.Count > 3)
						{
							Project.Log.ThrowWarning
							(
								Log.WarningId.SyntaxError,
								Log.WarnLevel.Normal,
								"Invalid entry in the property '{0}': '{1}'. Following syntax is accepted: 'PackageName', or 'PackageName/ModuleName', or 'PackageName/group/ModuleName'.", 
								Module.GroupName + ".assetdependencies", dependency
							);
						}
						break;
				}
			}
			if (assetDependencyDefinitions.Count - startDependenciesCount > 0)
			{
				Log.Status.WriteLine(LogPrefix + "Found {0} assetdependencies", assetDependencyDefinitions.Count - startDependenciesCount);
			}
			else
			{
				Log.Info.WriteLine(LogPrefix + "Found {0} assetdependencies", assetDependencyDefinitions.Count - startDependenciesCount);
			}
		}

		private void AddDependencyDefinitions_FromModuleDependencies(List<DependencyDef> assetDependencyDefinitions)
		{
			foreach (var dep in Module.Dependents)
			{
				if (dep.IsKindOf(DependencyTypes.Interface | DependencyTypes.Link))
				{
					//IMTODO: How do we deal with use dependencies?
					assetDependencyDefinitions.Add(new DependencyDef(dep.Dependent.Package.Name, dep.Dependent.Name, dep.Dependent.BuildGroup));
				}
			}
		}


		private List<string> GetAssetFilesetNames_FromDependents(List<DependencyDef> assetDependencyDefinitions)
		{
			var filesetNames = new List<string>();

			foreach (var depPackage in assetDependencyDefinitions.GroupBy(dep => dep.PackageName))
			{
				if (depPackage.Any(depDef => String.IsNullOrEmpty(depDef.ModuleName)))
				{
					int declaredModulesCount = 0;

					foreach(var buildGroup in depPackage.GroupBy(dDef => dDef.BuildGroup))
					{
						// Dependency on all modules in the package
						var declaredModules = Project.Properties["package." + depPackage.Key + "." + buildGroup.Key + ".buildmodules"].ToArray();

						declaredModulesCount += declaredModules.Count;

						AddModuleLevelAssetFileSets(depPackage.Key, declaredModules, filesetNames);
					}

					if (declaredModulesCount == 0)
					{
						// Check package level definitions
						AddPackageLevelAssetFilesets(depPackage.Key, filesetNames);
					}
				}
				else
				{
					//Dependencies on subset of modules.
					int foundModuleAssetDefinitions = AddModuleLevelAssetFileSets(depPackage.Key, depPackage.Select(dDef=>dDef.ModuleName), filesetNames);

					if (foundModuleAssetDefinitions == 0)
					{
						//Dependent package must have assetfile definitions on the package level only
						// Check package level definitions
						AddPackageLevelAssetFilesets(depPackage.Key, filesetNames);
					}

				}
			}

			return filesetNames;
		}

		private int AddModuleLevelAssetFileSets(string depPackageName, IEnumerable<string> depModuleNames, List<string> filesetNames)
		{
			int foundModuleLevelAssets = 0;
			foreach (var depModule in depModuleNames)
			{
				if (String.IsNullOrEmpty(depModule))
				{
					// Package level dependency
					return 0;
				}
				else
				{
					var assetFileSetsProp = Module.Package.Project.GetPropertyValue("package." + depPackageName + "." + depModule + ".assetfiles-set." + Module.Configuration.System)
											?? Module.Package.Project.GetPropertyValue("package." + depPackageName + "." + depModule + ".assetfiles-set");

					FileSet assets;
					var assetFileSets = assetFileSetsProp.ToArray();

					if (assetFileSets.Count > 0)
					{
						Log.Status.WriteLine(LogPrefix + "+ {0}", "package." + depPackageName + "." + depModule + ".assetfiles-set");

						foreach (var fsName in assetFileSets)
						{

							if (Module.Package.Project.NamedFileSets.TryGetValue(fsName, out assets))
							{
								if (!filesetNames.Contains(fsName))
								{
									filesetNames.Add(fsName);
									foundModuleLevelAssets++;
									Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsName, assets.FileItems.Count);
								}
							}
						}
					}

					var fsname = "package." + depPackageName + "." + depModule + ".assetfiles." + Module.Configuration.System;
					if (Module.Package.Project.NamedFileSets.TryGetValue(fsname, out assets))
					{
						if (!filesetNames.Contains(fsname))
						{
							filesetNames.Add(fsname);
							foundModuleLevelAssets++;
							Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsname, assets.FileItems.Count);
						}
					}
					else
					{
						fsname = "package." + depPackageName + "." + depModule + ".assetfiles";
						if (Module.Package.Project.NamedFileSets.TryGetValue(fsname, out assets))
						{
							if (!filesetNames.Contains(fsname))
							{
								filesetNames.Add(fsname);
								foundModuleLevelAssets++;
								Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsname, assets.FileItems.Count);
							}
						}
					}
				}
			}

			return foundModuleLevelAssets;
		}


		private void AddPackageLevelAssetFilesets(string packageName, List<string> filesetNames)
		{
			var assetFileSetsProp = Module.Package.Project.GetPropertyValue("package." + packageName + ".assetfiles-set." + Module.Configuration.System)
									?? Module.Package.Project.GetPropertyValue("package." + packageName + ".assetfiles-set");

			FileSet assets;
			var assetFileSets = assetFileSetsProp.ToArray();

			if (assetFileSets.Count > 0)
			{
				Log.Status.WriteLine(LogPrefix + "+ {0}", "package." + packageName + ".assetfiles-set");

				foreach (var fsName in assetFileSets)
				{
					if (Module.Package.Project.NamedFileSets.TryGetValue(fsName, out assets))
					{
						if (!filesetNames.Contains(fsName))
						{
							filesetNames.Add(fsName);
							Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsName, assets.FileItems.Count);
						}
					}
				}
			}
			var fsname = "package." + packageName + ".assetfiles." + Module.Configuration.System;
			if (Module.Package.Project.NamedFileSets.TryGetValue(fsname, out assets))
			{
				if (!filesetNames.Contains(fsname))
				{
					filesetNames.Add(fsname);
					Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsname, assets.FileItems.Count);
				}
			}
			else
			{
				fsname = "package." + packageName + ".assetfiles";
				if (Module.Package.Project.NamedFileSets.TryGetValue(fsname, out assets))
				{
					if (!filesetNames.Contains(fsname))
					{
						filesetNames.Add(fsname);
						Log.Status.WriteLine(LogPrefix + "{0}: {1} file(s)", fsname, assets.FileItems.Count);
					}
				}
			}
		}

		internal class DependencyDef : IEquatable<DependencyDef>
		{
			internal DependencyDef(string packageName, string moduleName, BuildGroups buildGroup)
			{
				PackageName = packageName;
				ModuleName = moduleName;
				BuildGroup = buildGroup;
			}
			public readonly string PackageName;
			public readonly string ModuleName;
			public readonly BuildGroups BuildGroup;

			public override string ToString()
			{
				return BuildGroup + "." + ModuleName;
			}

			public bool Equals(DependencyDef other)
			{
				bool ret = false;

				if (Object.ReferenceEquals(this, other))
				{
					ret = true;
				}
				else if (Object.ReferenceEquals(other, null))
				{
					ret = (PackageName == null && ModuleName == null);
				}
				else
				{
					ret = (BuildGroup == other.BuildGroup && ModuleName == other.ModuleName && PackageName == other.PackageName);
				}

				return ret;
			}
		}
	}
}

