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
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Build
{
	/// <summary>
	/// Collection of utility functions usually used in build, run and other targets.
	/// </summary>
	[FunctionClass("Build Functions")]
	public class BuildFunctions : FunctionClassBase
	{
		/// <summary>
		/// Returns output directory for a given module. For programs and DLLs it is usually 'bin' directory, for libraries 'lib'.
		/// This function takes into account output mapping.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="type">type can be either "lib" or "bin"</param>
		/// <param name="packageName">name of the package</param>
		/// <returns>module output directory</returns>
		[Function()]
		public static string GetModuleOutputDir(Project project, string type, string packageName)
		{
			// TODO GetModuleOutputDir should be deprecated
			// -dvaliant 2016/06/01
			/* relying on build.module here is dangerous we have no idea where users might call this function and if build.module will be
			correct or even set. Also the doc string reads "Returns output directory for a given module" but the module is not given, it 
			is implied. The *package* name is given. Should maybe replace this with GetPackageDefaultOutputDir or to be honest just 
			replace it with GetModuleOutputDirEx and force user to provide a module name */
			string buildModule = project.Properties["build.module"];
			if (buildModule != null)
			{
				return GetModuleOutputDirEx(project, type, packageName, buildModule);
			}

			// rest of this function is legacy behaviour if build module is null, this might return the right value but loses any information 
			// about module specific overrides
			string dirOption = null;
			if (type == "bin")
			{
				dirOption = "configbindir";
			}
			else if (type == "lib")
			{
				dirOption = "configlibdir";
			}
			else
			{
				throw new BuildException(String.Format("Unknown module path type '{0}'! Valid values are 'lib' or 'bin'.", type));
			}

			// try to deduce group from context
			string group = project.Properties["eaconfig.build.group"] ?? "runtime";
			
			// if the package.name properties matches target package name we assume that we're 
			//in the package's .build file context and can rely on private context information, otherwise use public
			if (project.Properties["package.name"] == packageName)
			{
				// if option set override configbindir or configlibdir return that
				OptionSet outputMapping = ModulePath.Private.PackageOutputMapping(project, packageName);
				if (outputMapping != null)
				{
					string dirRemapping = outputMapping.Options[dirOption];
					if (dirRemapping != null)
					{
						return dirRemapping;
					}
				}
				
				// otherwise just return config bin/lib dir property plus group folder if they are valid
				return Path.Combine(project.Properties[ModulePath.Private.PackageProperty(dirOption)] ?? String.Empty, project.Properties["eaconfig." + group + ".outputfolder"] ?? String.Empty);
			}
			else
			{
				// if option set override configvbindir or configlibdir return that
				OptionSet outputMapping = ModulePath.Public.PackageOutputMapping(project, packageName);
				if (outputMapping != null)
				{
					string dirRemapping = outputMapping.Options[dirOption];
					if (dirRemapping != null)
					{
						return dirRemapping;
					}
				}

				// check we have enough information to call default functions
				if (project.Properties["package." + packageName + ".builddir"] == null || project.Properties["config"] == null)
				{
					throw new BuildException(String.Format("Insufficient context information. Make sure dependent task is called for package '{0}' before invoking 'GetModuleOutputDir({1}, {0}).", packageName, type));
				}

				// public context with no output mapping information, we have to assume package is using default directories
				if (type == "bin")
				{
					return ModulePath.Defaults.GetDefaultConfigBinDir(project, packageName);
				}
				else if (type == "lib")
				{
					return ModulePath.Defaults.GetDefaultConfigLibDir(project, packageName);
				}
				else
				{
					throw new BuildException(String.Format("Unknown module path type '{0}'! Valid values are 'lib' or 'bin'.", type));
				}
				
			}	
		}

		/// <summary>
		/// Returns output directory for a given module. For programs and DLLs it is usually 'bin' directory, for libraries 'lib'.
		/// This function takes into account output mapping. NOTE. In the context of a .build file set property 'eaconfig.build.group' 
		/// to use this function for groups other than runtime.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="type">type can be either "lib" or "bin"</param>
		/// <param name="packageName">name of the package</param>
		/// <param name="moduleName">name of the module</param>
		/// <param name="moduleGroup">Optional - group of the module e.g. runtime, test. If not provided Framework will attempt to deduce it from context.</param>
		/// <param name="buildType">Optional - build type of the module e.g. Program, Library.</param>
		/// <returns>module output directory</returns>
		[Function()]
		public static string GetModuleOutputDirEx(Project project, string type, string packageName, string moduleName, string moduleGroup = null, string buildType = null)
		{
			return GetModulePath(project, type, packageName, moduleName, moduleGroup, buildType).OutputDir;
		}

		/// <summary>
		/// Returns outputname for a module. Takes into account output mapping. 
		/// </summary>
		/// <param name="project"></param>
		/// <param name="type">Can be either "lib" or "bin"</param>
		/// <param name="packageName">name of a package</param>
		/// <param name="moduleName">name of a module in the package</param>
		/// <param name="moduleGroup">Optional - group of the module e.g. runtime, test. If not provided Framework will attempt to deduce it from context.</param>
		/// <param name="buildType">Optional - build type of the module e.g. Program, Library.</param>
		/// <returns>outputname</returns>
		[Function()]
		public static string GetModuleOutputName(Project project, string type, string packageName, string moduleName, string moduleGroup = null, string buildType = null)
		{
			return GetModulePath(project, type, packageName, moduleName, moduleGroup, buildType).OutputName;
		}

		/// <summary>
		/// Returns list of module groupmanes for modules defined in package loaded into the current Project.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="groups">list of groups to examine. I.e. 'runtime', 'test', ...</param>
		/// <returns>module groupnames, new line separated.</returns>
		[Function()]
		public static string GetModuleGroupnames(Project project, string groups)
		{
			IPackage p;
			var groupnames = new StringBuilder();
			if(project.TryGetPackage(out p))
			{
				var buildgroups = new HashSet<BuildGroups>(groups.ToArray().Select(g=> ConvertUtil.ToEnum<BuildGroups>(g)));

				foreach(var module in p.Modules.Where(m=>buildgroups.Contains(m.BuildGroup)))
				{
					groupnames.AppendLine(module.GroupName);
				}
			}

			return groupnames.ToString();
		}

		/// <summary>
		/// Returns module name based on the group name
		/// </summary>
		/// <param name="project"></param>
		/// <param name="groupname">I.e. '${module}, runtime.${module}', 'test.${module}', ...</param>
		/// <returns>module name.</returns>
		[Function()]
		public static string ModuleNameFromGroupname(Project project, string groupname)
		{
			var module = groupname;

			foreach (BuildGroups group in Enum.GetValues(typeof(BuildGroups)))
			{
				var prefix = Enum.GetName(typeof(BuildGroups), group) + ".";
				if (groupname.StartsWith(prefix) && groupname.Length > prefix.Length)
				{
					module = groupname.Substring(prefix.Length);
					break;
				}
			}
			return module;
		}

		/// <summary>
		/// Returns true if there is usable build graph
		/// </summary>
		/// <param name="project"></param>
		/// <param name="configurations">list of configurations</param>
		/// <param name="groups">list of group names</param>
		/// <returns>Returns true if there is usable build graph.</returns>
		[Function()]
		public static string HasUsableBuildGraph(Project project, string configurations, string groups)
		{
			if (project.HasBuildGraph() && !project.BuildGraph().IsEmpty)
			{
				if (project.BuildGraph().CanReuse(configurations.ToArray(), groups.ToArray().Select(gn => ConvertUtil.ToEnum<BuildGroups>(gn)), project.Log, project.LogPrefix))
				{
					return "true";
				}
			}
			return "false";
		}

		/// <summary>
		/// Return a specific config component for a specifc full config name from top level build graph.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="configuration">The full config name (such as pc64-vc-debug)</param>
		/// <param name="config_component">The config component (supported values are: config-system, config-compiler, config-platform, config-name, config-processor)</param>
		/// <returns>Return a specific config component for a specifc full config name or an empty string if not found.  NOTE that this function assumes that build graph is already constructed, otherwise, it will just return empty string.</returns>
		[Function()]
		public static string RetrieveConfigInfo(Project project, string configuration, string config_component)
		{
			if (!project.BuildGraph().IsBuildGraphComplete || !project.BuildGraph().Modules.Any())
			{
				return String.Empty;
			}

			string returnValue = String.Empty;
			IEnumerable<Configuration> foundconfigs = project.BuildGraph().ConfigurationList.Where(cfg => cfg.Name == configuration);
			if (foundconfigs.Any())
			{
				Configuration config = foundconfigs.First();	// There should be only one.
				switch (config_component)
				{
					case "config-system":
						returnValue = config.System;
						break;
					case "config-compiler":
						returnValue = config.Compiler;
						break;
					case "config-platform":
						returnValue = config.Platform;
						break;
					case "config-name": // The real property name is "config-name" but we store it as "Configuration.Type" because "Configuration.Name" got used as "config" property.
						returnValue = config.Type;
						break;
					case "config-processor":
						returnValue = config.Processor;
						break;
					default:
						project.Log.Warning.WriteLine("Unrecognized config_component input '{0}' in RetrieveConfigInfo() function!", config_component);
						break;
				}
			}
			return returnValue;
		}

		// this function tries really hard to take into account all the information available in current Project context to return the right
		// answer but is not guaranteed to be be accurate depending on which override options the user has specified
		private static ModulePath GetModulePath(Project project, string type, string packageName, string moduleName, string moduleGroup = null, string buildType = null)
		{
			// if the package.name properties matches target package name we assume that we're in the package's .build file context
			// and can rely on private context information
			if (project.Properties["package.name"] == packageName)
			{
				// we really don't know the context in which this function is being called so we can't really be sure
				// what the group is, however old functions defaulted to current build group so we take that if 
				// available and group was not provided by caller
				moduleGroup = moduleGroup ?? project.Properties["eaconfig.build.group"] ?? "runtime";

				// resolved build type - we always SHOULD be able to resolve this in private context but it's very dependent 
				// on order of declaration so handle undefined case with clear error
				buildType = buildType ?? project.Properties[moduleGroup + "." + moduleName + ".buildtype"];
				// some older build scripts may define a single module and the property may not include the module name
				buildType = buildType ?? project.Properties[moduleGroup + ".buildtype"];
				if (buildType == null)
				{
					throw new BuildException(String.Format("Cannot determine build type for module '{0}.{1}'. Make sure module has been correctly defined or specify explicit build type.", moduleGroup, moduleName));
				}
				BuildType resolvedBuildType = GetModuleBaseType.Execute(project, buildType);

				// call correct function based on type
				if (type == "bin")
				{
					return ModulePath.Private.GetModuleBinPath(project, packageName, moduleName, moduleGroup, resolvedBuildType);
				}
				else if (type == "lib")
				{
					return ModulePath.Private.GetModuleLibPath(project, packageName, moduleName, moduleGroup, resolvedBuildType);
				}
				else
				{
					throw new BuildException(String.Format("Unknown module path type '{0}'! Valid values are 'lib' or 'bin'.", type));
				}
			}
			else
			{
				moduleGroup = moduleGroup ?? "runtime";

				// reolved build type - we can only resolve this if caller has provided build type information
				BuildType resolvedBuildType = null;
				if (buildType != null)
				{
					resolvedBuildType = GetModuleBaseType.Execute(project, buildType);
				}

				// call correct function based on type
				if (type == "bin")
				{
					string defaultSuffix = String.Empty;	// if resolvedBuildType is not null, below function can derived suffix, if not we have no way to know so pass empty string
															// - we expect this function to be mainly used to resolve output dir and output name - not full path with extension
					return ModulePath.Public.GetModuleBinPath(project, packageName, moduleName, moduleGroup, defaultSuffix, resolvedBuildType);
				}
				else if (type == "lib")
				{
					return ModulePath.Public.GetModuleLibPath(project, packageName, moduleName, moduleGroup, resolvedBuildType);
				}
				else
				{
					throw new BuildException("Unknown module path type '{0}'! Valid values are 'lib' or 'bin'.");
				}
			}
		}
	}
}
