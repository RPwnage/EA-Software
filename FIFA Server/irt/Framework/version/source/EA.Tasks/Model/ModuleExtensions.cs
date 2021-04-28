// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Linq;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using NAnt.Core.PackageCore;
using NAnt.Core.Logging;
using EA.Tasks.Model;

namespace EA.FrameworkTasks.Model
{
	public static class ModuleExtensions
	{
		public static bool Contains(this IEnumerable<IModule> modules, string modulename)
		{
			foreach (var m in modules)
			{
				if (m.Name == modulename) return true;
			}
			return false;
		}

		public static string PropGroupName(this IModule module, string name)
		{
			if (module == null)
			{
				return null;
			}

			if (name.StartsWith("."))
			{
				return module.GroupName + name;
			}
			return module.GroupName + "." + name;
		}

		public static string GetPlatformModulePropGroupName(this IModule module, string name)
		{
			return module.PropGroupName(name + "." + module.Configuration.System);
		}

		public static string PropGroupValue(this IModule module, string name, string defaultVal = "")
		{
			if (module == null || module.Package.Project == null || String.IsNullOrEmpty(name))
			{
				return null;
			}

			return module.Package.Project.Properties[module.PropGroupName(name)] ?? defaultVal;
		}

		public static bool GetModuleBooleanPropertyValue(this IModule module, string name, bool defaultVal = false, bool failOnNonBoolValue = false)
		{
			if (module == null || String.IsNullOrEmpty(name))
			{
				return defaultVal;
			}
			string value = module.GetPlatformModuleProperty(name);
			return value.ToBooleanOrDefault(defaultVal);
		}

		public static string GetPlatformModuleProperty(this IModule module, string name, string defaultVal = "")
		{
			if (module == null || String.IsNullOrEmpty(name))
			{
				return defaultVal;
			}
			string val = module.PropGroupValue(name + "." + module.Configuration.System, null) ?? module.PropGroupValue(name, null);
			return val ?? defaultVal;
		}

		public static FileSet PropGroupFileSet(this IModule module, string name)
		{
			if (module == null || String.IsNullOrEmpty(name))
			{
				return null;
			}
			return module.Package.Project.NamedFileSets[module.PropGroupName(name)];
		}

		public static FileSet GetPlatformModuleFileSet(this IModule module, string name, bool combined = true)
		{
			if (module == null || String.IsNullOrEmpty(name))
			{
				return null;
			}

			FileSet platformFileSet = module.PropGroupFileSet(name + "." + module.Configuration.System);
			FileSet genericFileSet = module.PropGroupFileSet(name);

			if (combined && platformFileSet != null && genericFileSet != null)
			{
				FileSet fs = new FileSet();
				fs.IncludeWithBaseDir(platformFileSet);
				fs.IncludeWithBaseDir(genericFileSet);
				return fs;
			}
			else
			{
				return platformFileSet ?? genericFileSet;
			}
		}
		
		// common framework pattern, a module needs to get a publically declared fileset from a dependent module (package.<Package>.<Module>.<fileset>),
		// if module does not declare this fileset a package level fileset is searched for instead (package.<Package>.<fileset>)
		// module : looks for public declarations in this module's project
		// dependent : module that might publically declare fileset
		// fileSetName: name of fileset without prefix
		public static FileSet GetPublicFileSet(this IModule module, IModule dependent, string fileSetName)
		{
			fileSetName = fileSetName.StartsWith(".") ? fileSetName : "." + fileSetName;
			string packageLevelPrefix  = "package." + dependent.Package.Name;
			string moduleLevelFileSetName =  packageLevelPrefix + "." + dependent.Name + fileSetName;
			if (module.Package.Project.NamedFileSets.TryGetValue(moduleLevelFileSetName, out FileSet fileSet))
			{
				return fileSet;
			}
			string packageLevelFileSetName = packageLevelPrefix + fileSetName;
			return module.Package.Project.NamedFileSets[packageLevelFileSetName];
		}

		public static OptionSet GetModuleOptions(this IModule module)
		{
			OptionSet buildoptionset = null;
			if (
				module is ProcessableModule processableModule && 
				(processableModule?.Package?.Project?.NamedOptionSets.TryGetValue(processableModule.BuildType.Name, out buildoptionset) ?? false)
			)
			{
				return buildoptionset;
			}
			return null;
		}

		public static void TraverseDependencies(this IModule module, Func<Stack<Dependency<IModule>>, IModule, Dependency<IModule>, bool> function, bool isTransitive = true, HashSet<string> reprocessDependentList = null)
		{
			if (module != null)
			{
				var stack = new Stack<Dependency<IModule>>();
				var processed = new HashSet<string>();

				foreach (var d in module.Dependents.OrderBy(x => (x.Dependent as ProcessableModule).GraphOrder))
				{
					if (isTransitive)
					{
						TraverseDependenciesImpl(processed, stack, module, d, function, reprocessDependentList);
					}
					else
					{
						function(stack, module, d);
					}
				}
			}
		}

		private static void TraverseDependenciesImpl(HashSet<string> processed, Stack<Dependency<IModule>> stack, IModule parent, Dependency<IModule> d, Func<Stack<Dependency<IModule>>, IModule, Dependency<IModule>, bool> function, HashSet<string> reprocessDependentList)
		{
			bool keepTraversing = function(stack, parent, d);
			if (keepTraversing)
			{
				// The "reprocessDependentList" is a way for the above "function" to update and pass back to this recursive function indicating that
				// we want to reprocess certain module's dependent due to new information presented (usually different usage of the previously
				// processed module).  It is up to the above "function" to keep track whether the module has already been reprocessed.  This
				// function will use "reprocessDependentList" as a consumer.  Once consumed, that module info will be removed from this list.
				if (reprocessDependentList != null)
				{
					if (reprocessDependentList.Contains(d.Dependent.Key))
					{
						processed.Remove(d.Dependent.Key);
						reprocessDependentList.Remove(d.Dependent.Key);
					}
				}

				if (processed.Add(d.Dependent.Key))
				{
					stack.Push(d);

					foreach (var dd in d.Dependent.Dependents.OrderBy(x => (x.Dependent as ProcessableModule).GraphOrder))
					{
						TraverseDependenciesImpl(processed, stack, d.Dependent, dd, function, reprocessDependentList);
					}

					stack.Pop();
				}
			}
		}

		public static string FormatModuleName(IModule module)
		{
			if(module != null)
			{
				return (module.Name == EA.Eaconfig.Modules.Module_UseDependency.PACKAGE_DEPENDENCY_NAME) ? module.Package.Name : module.Name;
			}
			return "unknown";
		}

		public static string FormatDependencyStack(this Stack<Dependency<IModule>> stack, IModule rootmodule, IModule parent, Dependency<IModule> d)
		{
			if (rootmodule.Key != parent.Key)
			{
				return String.Format("{0} <== {1} <== {2}", FormatModuleName(rootmodule), (stack.Count > 0) ? stack.Reverse().ToString(" <== ", dd => FormatModuleName(dd.Dependent)) : FormatModuleName(parent), FormatModuleName(d.Dependent));
			}
			else
			{
				if (stack.Count > 0)
				{
					return String.Format("{0} <== {1} <== {2}", FormatModuleName(rootmodule), stack.Reverse().ToString(" <== ", dd => FormatModuleName(dd.Dependent)), FormatModuleName(d.Dependent));
				}
				else
				{
					return String.Format("{0} <== {1}", FormatModuleName(rootmodule), FormatModuleName(d.Dependent));
				}
			}
		}

		public static bool ShouldForceDisableOptimizationForModule(Project proj, string packageName, string moduleGroup, string moduleName)
		{
			bool disableOpt = false;

			string disableOptModules = proj.Properties.GetPropertyOrDefault(
				"eaconfig." + proj.Properties["config-name"] + ".modulesForceDisableOptimization",
				proj.Properties.GetPropertyOrDefault("eaconfig.modulesForceDisableOptimization", "")).Trim();

			if (String.IsNullOrEmpty(disableOptModules))
				return false;

			HashSet<string> inputModuleList =
				disableOptModules.Split(new char[] { '\n', '\t', ' ', ';', ',' }).Select(m => m.Trim().ToLower()).Where(m => !String.IsNullOrEmpty(m)).ToHashSet();

			HashSet<string> currModuleSpec = new HashSet<string>();
			currModuleSpec.Add(packageName.ToLower());  // Need to test if entire package is marked disable optimization.
			if (moduleGroup == "runtime")
			{
				currModuleSpec.Add((packageName + "/" + moduleName).ToLower());
			}
			currModuleSpec.Add((packageName + "/" + moduleGroup + "/" + moduleName).ToLower());

			disableOpt = inputModuleList.Intersect(currModuleSpec).Any();

			return disableOpt;
		}

		public static OptionSet GetModuleBuildOptionSet(this ProcessableModule module)
		{
			string newOptionSetNamePrefix = "";
			OptionDictionary forceAppliedOptions = new OptionDictionary();
			if (module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true) && module.Package.Project.Properties.GetBooleanProperty(module.PropGroupName("use.pch")))
			{
				newOptionSetNamePrefix = "_UsePch_" + newOptionSetNamePrefix;
				forceAppliedOptions.Add("use-pch", "on");
			}
			if (ShouldForceDisableOptimizationForModule(module.Package.Project, module.Package.Name, module.BuildGroup.ToString(), module.Name))
			{
				newOptionSetNamePrefix = "_DisableOptm_" + newOptionSetNamePrefix;
				forceAppliedOptions.Add("optimization", "off");
			}

			if (!String.IsNullOrEmpty(newOptionSetNamePrefix))
			{
				string newOptName = newOptionSetNamePrefix + module.BuildType.Name;
				return module.Package.Project.NamedOptionSets.GetOrAdd(newOptName, oname =>
				{
					OptionSet os = new OptionSet();
					foreach (KeyValuePair<string, string> opt in forceAppliedOptions)
					{
						os.Options[opt.Key] = opt.Value;
					}
					return Eaconfig.Structured.BuildTypeTask.ExecuteTask(module.Package.Project, oname, module.BuildType.Name, os, saveFinalToproject: false);
				});
			}
			else
			{
				return OptionSetUtil.GetOptionSetOrFail(module.Package.Project, module.BuildType.Name);
			}
		}

		public static string GetTargetFrameworkVersion(this IModule module)
		{
			// error if any of the old properties are set
			List<string> legacyPropertyNames = new List<string>
			{
				module.PropGroupName(".targetframeworkversion"),
				"eaconfig.targetframeworkversion",
				"eaconfig.targetframeworkversion.core",
				"eaconfig.targetframeworkversion.standard"	
			};
			if (module.IsKindOf(Module.CSharp) || module.IsKindOf(Module.FSharp))
			{
				legacyPropertyNames.Add(module.PropGroupName(module.GetFrameworkPrefix() + ".targetframeworkversion"));
			}

			foreach (string legacyPropertyName in legacyPropertyNames)
			{
				if (module.Package.Project.Properties.Contains(legacyPropertyName))
				{
					throw new BuildException($"Module '{module}' sets property '{legacyPropertyName}'. Please remove this property from your build scripts." +
						$"The following override properties for target .NET version are no longer supported: {String.Join(", ", legacyPropertyName)}. " +
						$"The .NET version is automatically derived from target .NET family and the versions of the relevant SDK package in your masterconfig (DotNet or DotNetCoreSdk).");
				}
			}

			if (module is Module_DotNet dotNetModule)
			{
				return dotNetModule.TargetFrameworkVersion;
			}
			else if (module is Module_Native nativeModule && nativeModule.IsKindOf(Module.Managed))
			{
				return nativeModule.TargetFrameworkVersion;
			}
			else
			{
				return DotNetTargeting.GetNetVersion(module.Package.Project, DotNetFrameworkFamilies.Framework);
			}
		}

		public static DotNetFrameworkFamilies GetDotNetFamily(this IModule module)
		{
			if (module is Module_DotNet dotNetModule)
			{
				return dotNetModule.TargetFrameworkFamily;
			}
			else if (module is Module_Native nativeModule && nativeModule.IsKindOf(Module.Managed))
			{

				return nativeModule.TargetFrameworkFamily;
			}
			else
			{
				return DotNetFrameworkFamilies.Framework;
			}
		}

		public static string GetFrameworkPrefix(this IModule module)
		{
			if (module.IsKindOf(Module.CSharp))
			{
				return "csproj";
			}
			else if (module.IsKindOf(Module.FSharp))
			{
				return "fsproj";
			}
			throw new BuildException($"{module.LogPrefix}Module has unknown .Net language setting, supported languages are C# and F#.");
		}
	}
}
