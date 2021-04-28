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
using NAnt.Core.Util;
using NAnt.Core.PackageCore;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Build
{
	// helper class for calculating the output dirs for modules, functions are divided between "public" and "private"
	// contexts, all assignments and derivations of library and binary paths should ideally go through these functions
	public class ModulePath
	{
		public string FullPath { get { return Path.Combine(OutputDir, OutputName + Extension); } }

		public readonly string Extension;		// extension of primary build output, lib, dll, exe, elf, prx, etc
		public readonly string OutputName;		// file name without extension
		public readonly string OutputDir;		// directory to ouput file too
		public readonly string OutputNameToken; // output name to use when replacing %outputname% in template strings, can be different from output name

		private const string LinkMappingOverrideName = "linkoutputname";
		private const string LibMappingOverrideName = "liboutputname";
		private const string ImportLibMappingOverrideName = "impliboutputname";

		// private constructor, should only be created by static functions in this class
		private ModulePath(string outputDirectory, string outputName, string extension, string replaceToken)
		{
			OutputDir = outputDirectory;
			OutputName = outputName;
			Extension = extension;
			OutputNameToken = replaceToken;
		}

		// public context implies the Project being used is for a different package and we are loading an initialize.xml
		// to get the context information
		public static class Public
		{
			/// <summary>
			/// Gets information about a module's binary output path. This is the path to a native shared library, a .NET assembly or native executable produced by this module.
			/// </summary>
			/// <param name="project">The project in which the path is trying to be resolved. This function is in the Public class so it assumed the Project passed is not the
			/// Project that owns the package the module is defined in.</param>
			/// <param name="packageName">Name of the package the module is from.</param>
			/// <param name="moduleName">The name of the module.</param>
			/// <param name="moduleGroup">The group the module is in e.g. "runtime", "test", etc.</param>
			/// <param name="defaultSuffix">If buildType is not provided a default suffix must be used for the binary suffix e.g. ".dll", ".exe", etc.</param>
			/// <param name="buildType">Build type object if known, used to get output path templates - function may return incorrect information with this being provided but it is not required.</param>
			/// <param name="outputMappingOptionSet">Module can have private output name mapping options e.g. "runtime.MyModule.outputname-map-options" that will not be accessible in public context.
			/// This optionset should be provided here if it exists. Package level e.g. "package.MyPackage.outputname-map-options" are assumed to be public and will be pcked up automatically.</param>
			/// <param name="outputNameOverride">Moduke nay override it's output name in private context e.g. "runtime.MyModule.outputname". If this is case the override name must be provided here.</param>
			/// <param name="outputDirOverride">Moduke nay override it's output name in private context e.g. "runtime.MyModule.outputdir". If this is case the override directory must be provided here.</param>
			/// <param name="configDirOverride">Package may override the "package.configbindir" property in private context. If this is the case new value must be provided here.</param>
			/// <param name="buildDirOverride">Package may override the "package.configbuilddir" property in private context. This is used to determine moudle intermediate dir which is a possible 
			/// replacement token in output mapping. If "package.configbuilddir" is overridden new value must be provided here.</param>
			/// <param name="ignoreOutputMapping">Set to true to ignore all output mapping name optionsets.</param>
			/// <returns>A ModulePath object coontain information about the binary output path for the module.</returns>
			public static ModulePath GetModuleBinPath(Project project, string packageName, string moduleName, string moduleGroup, string defaultSuffix, BuildType buildType = null, OptionSet outputMappingOptionSet = null, string outputNameOverride = null, string outputDirOverride = null, string configDirOverride = null, string buildDirOverride = null, bool ignoreOutputMapping = false)
			{
				string defaultOutputPrefix = GetBinaryPrefix(project);
				string defaultOutputSuffix = buildType != null ? GetBinarySuffix(project, buildType) : defaultSuffix; // if caller doesn't provide buildtype we rely on them to give us suffix to use

				OptionSet outputMapping = ignoreOutputMapping ? null : outputMappingOptionSet ?? PackageOutputMapping(project, packageName);
				OptionSet buildOptionSet = buildType != null ? project.NamedOptionSets[buildType.Name] : null;

				string intermediateDir = GetModuleIntermedidateDir(project, packageName, moduleName, moduleGroup, buildDirOverride);

                string defaultOutputDir = GetRemappedConfigDir("configbindir", outputMapping, configDirOverride ?? Defaults.GetDefaultConfigBinDir(project, packageName));
				return GetMappedModuleOutputPath
				(
					project,
					packageName,
					moduleName,
					moduleGroup,
					intermediateDir,
					defaultOutputPrefix,
					defaultOutputSuffix,
					outputMapping,
					buildOptionSet,
					LinkMappingOverrideName,
					defaultOutputDir,
					outputNameOverride,
					outputDirOverride
				);
			}

            /// <summary>
            /// Gets information about a module's library output path. This is the path to a native static library, or a shared library import library file produced by this module.
            /// </summary>
            /// <param name="project">The project in which the path is trying to be resolved. This function is in the Public class so it assumed the Project passed is not the
            /// Project that owns the package the module is defined in.</param>
            /// <param name="packageName">Name of the package the module is from.</param>
            /// <param name="moduleName">The name of the module.</param>
            /// <param name="moduleGroup">The group the moduke is in e.g. "runtime", "test", etc.</param>
            /// <param name="buildType">Build type object if known, used to get output path templates - function may return incorrect information with this being provided but it is not required.</param>
            /// <param name="outputMappingOptionSet">Module can have private output name mapping options e.g. "runtime.MyModule.outputname-map-options" that will not be accessible in public context.
            /// This optionset should be provided here if it exists. Package level e.g. "package.MyPackage.outputname-map-options" are assumed to be public and will be pcked up automatically.</param>
            /// <param name="outputNameOverride">Moduke nay override it's output name in private context e.g. "runtime.MyModule.outputname". If this is case the override name must be provided here.</param>
            /// <param name="outputDirOverride">Moduke nay override it's output name in private context e.g. "runtime.MyModule.outputdir". If this is case the override directory must be provided here.</param>
            /// <param name="configBinDirOverride">Package may override the "package.configbindir" property in private context. Binary directory is potentiall used as a replacement token if output 
            /// library is an import library. If "package.configbindir" is overridden new value must be provided here.</param>
            /// <param name="configLibDirOverride">Package may override the "package.configlibdir" property in private context. If this is the case new value must be provided here.</param>
            /// <param name="buildDirOverride">Package may override the "package.configbuilddir" property in private context. This is used to determine module intermediate dir which is a possible 
            /// replacement token in output mapping. If "package.configbuilddir" is overridden new value must be provided here.</param>
            /// <param name="forceNoImportLib">Used to force the function to return path as if module was building a library rather than an import library. Mainly used internally to deduce
            /// normal lib path as it can be a template replacement token in import lib path.</param>
			/// <param name="ignoreOutputMapping">Set to true to ignore all output mapping name optionsets.</param>
            /// <returns>A ModulePath object coontain information about the library output path for the module.</returns>
			public static ModulePath GetModuleLibPath(Project project, string packageName, string moduleName, string moduleGroup, BuildType buildType = null, OptionSet outputMappingOptionSet = null, string outputNameOverride = null, string outputDirOverride = null, string configBinDirOverride = null, string configLibDirOverride = null, string buildDirOverride = null, bool forceNoImportLib = false, bool ignoreOutputMapping = false)
			{
				string defaultOutputPrefix = GetLibraryPrefix(project);
				string defaultOutputSuffix = GetLibrarySuffix(project);

				string intermediateDir = GetModuleIntermedidateDir(project, packageName, moduleName, moduleGroup, buildDirOverride);

				OptionSet outputMapping = ignoreOutputMapping ? null : outputMappingOptionSet ?? PackageOutputMapping(project, packageName);
				OptionSet buildOptionSet = buildType != null ? project.NamedOptionSets[buildType.Name] : null;

                string defaultOutputDir = GetRemappedConfigDir("configlibdir", outputMapping, configLibDirOverride ?? Defaults.GetDefaultConfigLibDir(project, packageName));

                // if optionset contains import library option then the lib path for the module will be the import library path
                if (!forceNoImportLib && buildOptionSet != null && buildOptionSet.Options[ImportLibMappingOverrideName] != null)
				{
					// get lib dir as if this is not an import lib as this token is used in optionset replacement	
					string moduleLibPath = GetModuleLibPath(project, packageName, moduleName, moduleGroup, buildType, outputMappingOptionSet, outputNameOverride, outputDirOverride, configBinDirOverride, configLibDirOverride, buildDirOverride, forceNoImportLib: true).OutputDir;

					// output dir token can also be used in import lib case
					string defaultBinOutputSuffix = String.Empty; // suffix is only used in determining output name and suffix, we're discarding all information except directory so we don't need to have a correct value for this
					string moduleBinPath = GetModuleBinPath(project, packageName, moduleName, moduleGroup, defaultBinOutputSuffix, buildType, outputMappingOptionSet, outputNameOverride, outputDirOverride, configBinDirOverride, buildDirOverride).OutputDir;
		
					return GetMappedModuleOutputPath
					(
						project,
						packageName,
						moduleName,
						moduleGroup,
						intermediateDir,
						defaultOutputPrefix,
						defaultOutputSuffix,
						outputMapping,
						buildOptionSet,
						ImportLibMappingOverrideName,
						defaultOutputDir,
						outputNameOverride,
						outputDirOverride,
						moduleLibPath,
						moduleBinPath
					);
				}
				
				// return library output path
				return GetMappedModuleOutputPath
				(
					project,
					packageName,
					moduleName,
					moduleGroup,
					intermediateDir,
					defaultOutputPrefix,
					defaultOutputSuffix,
					outputMapping,
					buildOptionSet,
					LibMappingOverrideName,
					defaultOutputDir,
					outputNameOverride,
					outputDirOverride
				);
			}

			/// <summary>
			/// Returns the intermedaite folder path for a module.
			/// </summary>
			/// <param name="project">The project in which the path is trying to be resolved. This function is in the Public class so it assumed the Project passed is not the
			/// Project that owns the package the module is defined in.</param>
			/// <param name="packageName">Name of the package the module is from.</param>
			/// <param name="moduleName">The name of the module.</param>
			/// <param name="moduleGroup">The group the moduke is in e.g. "runtime", "test", etc.</param>
			/// <param name="packageBuildDir">Package may override the "package.configbuilddir" property in private context. This is used to determine module intermediate dir 
			/// which is a possible replacement token in output mapping. If "package.configbuilddir" is overridden new value must be provided here.</param>
			/// <returns>A string containing normalized path to intermediate directory.</returns>
			public static string GetModuleIntermedidateDir(Project project, string packageName, string moduleName, string moduleGroup, string packageBuildDir = null)
			{
				return PathNormalizer.Normalize(Path.Combine(
					packageBuildDir ?? Defaults.GetDefaultConfigBuildDir(project, packageName),
					project.Properties["eaconfig." + moduleGroup + ".outputfolder"].TrimLeftSlash(),
					moduleName));
			}

			// get the public output mapping for a package
			internal static OptionSet PackageOutputMapping(Project project, string packageName)
			{
				// masterconfig mapping trumps all others, always take this first
				string optionSetName = null;
				Release release = PackageMap.Instance.FindOrInstallCurrentRelease(project, packageName);
				if (release.Manifest.Buildable)
				{
					optionSetName = PackageMap.Instance.GetMasterGroup(project, packageName)?.OutputMapOptionSet;
				}

				// fall back to private then public property if needed - relying on Private property here is dangerous since we have no
				// guarantee that we can access it in public context but legacy code assumed it would be globally available (i.e would 
				// apply to all packages) so we access it here for backwards compatibility

				string packageDir;
				using (PropertyDictionary.PropertyReadScope scope = project.Properties.ReadScope())
				{
					optionSetName = optionSetName ??
						scope[Private.PackageProperty("outputname-map-options")] ??
						scope[PackageProperty(packageName, "outputname-map-options")];

					packageDir = scope[PackageProperty(packageName, "dir")];
				}

				return ResolveMappingOptionSet(project, optionSetName, packageName, packageDir);
			}

			// package public properties are of the form
			// package.<package>.<property>
			// e.g.
			// package.MyPackage.libs
			internal static string PackageProperty(string packageName, string propertyName)
			{
				return String.Format("package.{0}.{1}", packageName, propertyName);
			}

            // module public properties are of the form
            // package.<package>.<module>.<property>
            // e.g.
            // package.MyPackage.MyModule.libs
            internal static string ModuleProperty(string moduleName, string packageName, string propertyName)
			{
				return String.Format("package.{0}.{1}.{2}", packageName, moduleName, propertyName);
			}
		}

		// private context implies the Project being used is the project for the package the owns the module in question
		// and we have total access to it's internal data
		public static class Private
		{
			public static ModulePath GetModuleBinPath(ProcessableModule module, bool ignoreOutputMapping = false, string binaryPrefix = null, string binarySuffix = null)
			{
				return GetModuleBinPath(module.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), module.BuildType, ignoreOutputMapping, binaryPrefix, binarySuffix);
			}

			/// <summary>	
			/// Gets information about a module's binary output path. This is the path to a native shared library, a .NET assembly or native executable produced by this module.
			/// </summary>
			/// <param name="project">The project in which the path is trying to be resolved. This function is in the Private class so it assumed the Project passed contains
			/// all information for module and its package.</param>
			/// <param name="packageName">Name of the package the module is from.</param>
			/// <param name="moduleName">The name of the module.</param>
			/// <param name="moduleGroup">The group the moduke is in e.g. "runtime", "test", etc.</param>
			/// <param name="buildType">Build type for the module.</param>
			/// <param name="ignoreOutputMapping">Set to true to ignore all output mapping name optionsets.</param>
			/// <param name="binaryPrefix"></param>
			/// <param name="binarySuffix"></param>
			/// <returns>A ModulePath object coontain information about the binary output path for the module.</returns>
			public static ModulePath GetModuleBinPath(Project project, string packageName, string moduleName, string moduleGroup, BuildType buildType, bool ignoreOutputMapping = false, string binaryPrefix = null, string binarySuffix = null)
			{
				string defaultOutputPrefix = binaryPrefix ?? GetBinaryPrefix(project);
				string defaultOutputSuffix = binarySuffix ?? GetBinarySuffix(project, buildType);

				string intermediateDir = GetModuleIntermedidateDir(project, moduleName, moduleGroup);	
			
				OptionSet buildOptionSet = project.NamedOptionSets[buildType.Name];
				OptionSet outputMapping = ignoreOutputMapping ? null : ModuleOutputMapping(project, packageName, moduleName, moduleGroup);

				string defaultOutputDir = GetRemappedConfigDir("configbindir", outputMapping, project.Properties[PackageProperty("configbindir")]);

				return GetMappedModuleOutputPath
				(
					project,
					packageName,
					moduleName,
					moduleGroup,
					intermediateDir,
					defaultOutputPrefix,
					defaultOutputSuffix,
					outputMapping,
					buildOptionSet,
					LinkMappingOverrideName,
					defaultOutputDir,
					// outputname override defines in Initialize.xml uses format package.[pkg].[module].outputname format.
					// while .build file uses [group].[module].outputname.  If this function get called by use dependency
					// .build file is not loaded, only the Initialize.xml.  So test the Initialize.xml version first and 
					project.Properties["package."+packageName+"."+moduleName+".outputname"] ?? project.Properties[ModuleProperty(moduleName, moduleGroup, "outputname")],
					project.Properties["package."+packageName+"."+moduleName+".outputdir"] ?? project.Properties[ModuleProperty(moduleName, moduleGroup, "outputdir")]
				);
			}

			/// <summary>
			/// Gets information about a module's library output path. This is the path to a native static library, or a shared library import library file produced by this module.
			/// </summary>
			/// <param name="module">Module to get library output path from.</param>
			/// <param name="ignoreOutputMapping">Set to true to ignore all output mapping name optionsets.</param>
			/// <param name="libraryPrefix"></param>
			/// <param name="librarySuffix"></param>
			/// <returns>A ModulePath object coontain information about the library output path for the module.</returns>
			public static ModulePath GetModuleLibPath(ProcessableModule module, bool ignoreOutputMapping = false, string libraryPrefix = null, string librarySuffix = null)
			{
                return GetModuleLibPath(module.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), module.BuildType, forceNoImportLib: false, ignoreOutputMapping, libraryPrefix, librarySuffix);
            }

			/// <summary>
			/// Gets information about a module's library output path. This is the path to a native static library, or a shared library import library file produced by this module.
			/// </summary>
			/// <param name="project">The project in which the path is trying to be resolved. This function is in the Private class so it assumed the Project passed contains
			/// all information for module and its package.</param>
			/// <param name="packageName">Name of the package the module is from.</param>
			/// <param name="moduleName">The name of the module.</param>
			/// <param name="moduleGroup">The group the moduke is in e.g. "runtime", "test", etc.</param>
			/// <param name="buildType">Build type for the module.</param>
			/// <param name="forceNoImportLib">Used to force the function to return path as if module was building a library rather than an import library. Mainly used internally to deduce
			/// normal lib path as it can be a template replacement token in import lib path.</param>
			/// <param name="ignoreOutputMapping">Set to true to ignore all output mapping name optionsets.</param>
			/// <param name="libraryPrefix"></param>
			/// <param name="librarySuffix"></param>
			/// <returns>A ModulePath object coontain information about the library output path for the module.</returns>
			public static ModulePath GetModuleLibPath(Project project, string packageName, string moduleName, string moduleGroup, BuildType buildType, bool forceNoImportLib = false, bool ignoreOutputMapping = false, string libraryPrefix = null, string librarySuffix = null)
			{
				string defaultOutputPrefix = libraryPrefix ?? GetLibraryPrefix(project);
				string defaultOutputSuffix = librarySuffix ?? GetLibrarySuffix(project);

				string intermediateDir = GetModuleIntermedidateDir(project, moduleName, moduleGroup);

				OptionSet buildOptionSet = project.NamedOptionSets[buildType.Name];
				OptionSet outputMapping = ignoreOutputMapping ? null : ModuleOutputMapping(project, packageName, moduleName, moduleGroup);

                string defaultOutputDir = GetRemappedConfigDir("configlibdir", outputMapping, project.Properties[PackageProperty("configlibdir")]);

                // if optionset contains import library option then the lib path for the module will be the import library path
                if (!forceNoImportLib && buildOptionSet.Options[ImportLibMappingOverrideName] != null)
				{
					// get lib dir as if this is not an import lib as this token is used in optionset replacement, suffix and prefix not passed because 
					// we don't need them if just want the directory
					OptionSet noImportLibOptionSet = new OptionSet(buildOptionSet);
					noImportLibOptionSet.Options.Remove(ImportLibMappingOverrideName);
					string moduleLibPath = GetModuleLibPath(project, packageName, moduleName, moduleGroup, buildType, forceNoImportLib: true).OutputDir;

					// output dir token can also be used, in import lib case, suffix and prefix not passed because we don't need them if just want the 
					// directory
					string moduleBinPath = GetModuleBinPath(project, packageName, moduleName, moduleGroup, buildType).OutputDir;
                    
					return GetMappedModuleOutputPath
					(
						project,
						packageName,
						moduleName,
						moduleGroup,
						intermediateDir,
						defaultOutputPrefix,
						defaultOutputSuffix,
						outputMapping,
						buildOptionSet,
						ImportLibMappingOverrideName,
						defaultOutputDir,
						project.Properties["package." + packageName + "." + moduleName + ".outputname"] ?? project.Properties[ModuleProperty(moduleName, moduleGroup, "outputname")],
						project.Properties["package." + packageName + "." + moduleName + ".outputdir"] ?? project.Properties[ModuleProperty(moduleName, moduleGroup, "outputdir")],
						moduleLibPath,
						moduleBinPath
					);
				}

				// return regular lib path
				return GetMappedModuleOutputPath
				(
					project,
					packageName,
					moduleName,
					moduleGroup,
					intermediateDir,
					defaultOutputPrefix,
					defaultOutputSuffix,
					outputMapping,
					buildOptionSet,
					LibMappingOverrideName,
					defaultOutputDir,
					project.Properties["package." + packageName + "." + moduleName + ".outputname"] ?? project.Properties[ModuleProperty(moduleName, moduleGroup, "outputname")],
					project.Properties["package." + packageName + "." + moduleName + ".outputdir"] ?? project.Properties[ModuleProperty(moduleName, moduleGroup, "outputdir")]
				);
			}

			/// <summary>
			/// Returns the intermedaite folder path for a module.
			/// </summary>
			/// <param name="project">The project in which the path is trying to be resolved. This function is in the Private class so it assumed the Project passed contains
			/// all information for module and its package.</param>
			/// <param name="moduleName">The name of the module.</param>
			/// <param name="moduleGroup">The group the module is in e.g. "runtime", "test", etc.</param>
			/// <returns>A string containing normalized path to intermediate directory.</returns>
			public static string GetModuleIntermedidateDir(Project project, string moduleName, string moduleGroup)
			{
				return PathNormalizer.Normalize(Path.Combine(
					project.Properties[PackageProperty("configbuilddir")],
					project.Properties["eaconfig." + moduleGroup + ".outputfolder"].TrimLeftSlash(),
					moduleName));
			}

			/// <summary>
			/// Returns the generation folder path for a module.
			/// </summary>
			/// <param name="project">The project in which the path is trying to be resolved. This function is in the Private class so it assumed the Project passed contains
			/// all information for module and its package.</param>
			/// <param name="moduleName">The name of the module.</param>
			/// <param name="moduleGroup">The group the module is in e.g. "runtime", "test", etc.</param>
			/// <returns>A string containing normalized path to generation directory.</returns>
			public static string GetModuleGenerationDir(Project project, string moduleName, string moduleGroup)
			{
				return PathNormalizer.Normalize(Path.Combine(
					project.Properties[PackageProperty("configgendir")],
					project.Properties["eaconfig." + moduleGroup + ".outputfolder"].TrimLeftSlash(),
					moduleName));
			}

			// get the private outputmapping for a package, falls back to public
			public static OptionSet PackageOutputMapping(Project project, string packageName)
			{ 
				// masterconfig mapping trumps all others, always take this first
				string optionSetName = null;
				Release release = PackageMap.Instance.FindOrInstallCurrentRelease(project, packageName);
				if (release.Manifest.Buildable)
				{
					optionSetName = PackageMap.Instance.GetMasterGroup(project, packageName)?.OutputMapOptionSet;
				}

				// fallback to private and then finally public properties if needed
				optionSetName = optionSetName ??
					project.Properties[PackageProperty("outputname-map-options")] ??
					project.Properties[Public.PackageProperty(packageName, "outputname-map-options")];

				string packageDir = project.Properties[PackageProperty("dir")];
				return ResolveMappingOptionSet(project, optionSetName, packageName, packageDir);
			}

			// gets module's private output mapping if applicable, falls back to package level (no "public" module level mapping exists)
			public static OptionSet ModuleOutputMapping(Project project, string packageName, string moduleName, string moduleGroup)
			{
				// for module level, outputname-map-options is name of optionset - not the name of a property the names the optionset
				// like it is at package level
				string mappingOptionSetName = ModuleProperty(moduleName, moduleGroup, "outputname-map-options");
				return project.NamedOptionSets[mappingOptionSetName] ??
					PackageOutputMapping(project, packageName); // this falls back to package public
			}

            // package private properties are of the form
            // package.<propertyname>
            // e.g.
            // package.configbindir
            internal static string PackageProperty(string propertyName)
			{
				return "package." + propertyName;
			}

            // module private properties are of the form
            // <group>.<module>.<property>
            // e.g.
            // runtime.MyModule.outputdir
            internal static string ModuleProperty(string moduleName, string groupName, string propertyName)
			{
				return groupName + "." + moduleName + "." + propertyName;
			}
		}

		// public functions to be used to set default path properties
		public static class Defaults
		{
			public static string GetDefaultConfigBinDir(Project project, string packageName)
			{
				return GetDefaultConfigDir(project, packageName, "bin");
			}

			public static string GetDefaultConfigLibDir(Project project, string packageName)
			{			
				return GetDefaultConfigDir(project, packageName, "lib");
			}

			public static string GetDefaultConfigBuildDir(Project project, string packageName)
			{
				return GetDefaultConfigDir(project, packageName, "build");
			}

			private static string GetDefaultConfigDir(Project project, string packageName, string type)
			{
				// package.<packagename>.builddir is set by both PackageTask and DependentTask
				// so we can rely on it both private and public context
				string buildDir;
				string config;
				using (PropertyDictionary.PropertyReadScope scope = project.Properties.ReadScope())
				{
					buildDir = scope["package." + packageName + ".builddir"];
					config = scope["config"];
				}
				return Path.Combine(buildDir, config, type);
			}
		}

		// all primary build output oaths are ultimately determined by this function
		// libOutputDir and binOutputDir parameters are only used for import lib case where path token for lib dir can be used and outputdir token should reolve to bin dir
		private static ModulePath GetMappedModuleOutputPath(Project project, string packageName, string moduleName, string moduleGroup, string intermediateDir, string defaultOutputPrefix, string defaultOutputSuffix, OptionSet outputMapping, OptionSet buildOptionSet, string outputNameOverrideOption, string defaultOutputDir, string outputNameOverride, string outputDirOverride, string libOutputDir = null, string binOutputDir = null)
		{
			// We have situation where the default is not set and we just get a crash.  
			string outputPrefix = defaultOutputPrefix == null ? String.Empty : defaultOutputPrefix;
			string outputSuffix = defaultOutputSuffix == null ? String.Empty : defaultOutputSuffix;

			// get output file name, can be overridden but normally is module name
			string defaultOutputName = outputNameOverride ?? moduleName;
			string outputName = defaultOutputName;
			if (outputMapping != null)
			{
				// output mapping options can override outputName, however override can contain
				// the %outputname% token which should be replaced by the default output name
				string mappingOutputName = null;
				if (outputMapping.Options.TryGetValue(outputNameOverrideOption, out mappingOutputName))
				{
					outputName = MacroMap.Replace(mappingOutputName, "outputname", defaultOutputName);
				}
			}

			bool useEaconfigGroupFolder = GetRemappedConfigDir("addgroupfolder", outputMapping, "true").ToBooleanOrDefault(true);

			// get output directory, can also be overidden
			bool userSpecifiedValidOutputDirOverride = outputDirOverride != null;  // TODO: should this be an IsNullOrEmpty() check instead of just a null check?
			string outputDir = userSpecifiedValidOutputDirOverride ?
				PathNormalizer.Normalize(outputDirOverride) : 
				PathNormalizer.Normalize(Path.Combine(
					defaultOutputDir,
					useEaconfigGroupFolder ? project.Properties["eaconfig." + moduleGroup + ".outputfolder"].TrimLeftSlash() : ""));

			// output dir specification can contain %outputname% also, replace this with *default* outputname
			// not remapped output name
			outputDir = PathNormalizer.Normalize(MacroMap.Replace(outputDir, "outputname", defaultOutputName));

			// if no build option set was provided or the option set doesn't specify override template
			// we can combine and return the information we have
			string buildOptionSetOutputName = null;
			bool optionSetHasOutputNameOverride = buildOptionSet != null && buildOptionSet.Options.TryGetValue(outputNameOverrideOption, out buildOptionSetOutputName);
			if (!optionSetHasOutputNameOverride)
			{
				// in this case replace token is outputName without suffix
				return new ModulePath
				(
					outputDirectory: outputDir,
					outputName: outputPrefix + outputName,
					extension: outputSuffix,
					replaceToken: outputName
				);
			}

			// if we get here then optionset overrides output name, replace tokens in optionset option	
			MacroMap buildOptionReplaceMap = new MacroMap()
			{
				{ "intermediatedir", intermediateDir },
				{ "outputdir", binOutputDir ?? outputDir },
				{ "outputname", outputName }
			};
			if (libOutputDir != null)
			{
				// special case for import libs, import lib path can use %outputlibdir% token
				buildOptionReplaceMap.Update("outputlibdir", libOutputDir);
			}
			buildOptionSetOutputName = MacroMap.Replace(buildOptionSetOutputName, buildOptionReplaceMap);
			buildOptionSetOutputName = PathNormalizer.Normalize(buildOptionSetOutputName, true);

			// some build flows requires us to split out extension, this is tricky to do because
			// we can't be sure if a . in the name is supposed to be part of the file name or 
			// the start of the extension

			// first validate suffix
			if (outputSuffix != String.Empty && (outputSuffix[0] != '.' || outputSuffix.Count(c => c == '.') != 1))
			{
				throw new BuildException(String.Format("Invalid output suffix '{0}' must be empty or an extension starting with '.' and containing only one '.'.", outputSuffix));
			}

			// see if resolved name ends with suffix
			if (buildOptionSetOutputName.EndsWith(outputSuffix)) // also returns false if outputSuffix is empty
			{
				// strip suffix
				buildOptionSetOutputName = buildOptionSetOutputName.Substring(0, buildOptionSetOutputName.Length - outputSuffix.Length);	
			}
			else
			{
				// PATHS-TODO this is a bad place to be - warning that suffix and prefix are wrong might be justified

				// now things get complicated - if user mapped to custom_%outputname%.ext did they intend
				// a file named 'custom_%outputname%' wth extension '.ext' or a file named 'custom_%outputname%.ext'
				// with no extension? safest to assume that on platforms with no default suffix* then no suffix was
				// intended and on platforms with suffix new suffix was intended
				// * note these will have been filtered already by above if
				outputSuffix = Path.GetExtension(buildOptionSetOutputName);
				buildOptionSetOutputName = buildOptionSetOutputName.Substring(0, buildOptionSetOutputName.Length - outputSuffix.Length);				
			}

			// if output dir was overriden by user take it as the directory no matter what, but 
			// take file name from remapped path
			return new ModulePath
			(
				outputDirectory: userSpecifiedValidOutputDirOverride ? outputDir : Path.GetDirectoryName(buildOptionSetOutputName),
				outputName: Path.GetFileName(buildOptionSetOutputName), // extension has already been removed
				extension: outputSuffix,
				replaceToken: outputName // replace token is output name as remapped by name mapping (if applicable) options but *NOT* as remapped by optionset
			);
		}

		// given a mapping optionset name (can be null) for a given package resolve the optionset including option tokens
		private static OptionSet ResolveMappingOptionSet(Project project, string optionSetName, string packageName, string packageDir)
		{
			OptionSet mappingOptionSet = null;
			if (!String.IsNullOrEmpty(optionSetName))
			{
				if (!project.NamedOptionSets.TryGetValue(optionSetName, out mappingOptionSet))
				{
					throw new Exception($"OutputMapping optionset '{optionSetName}' specified for package {packageName} does not exist.");
				}

				// if any options use package dir token, create a copy of the option set and resolve the token
				if (mappingOptionSet != null)
				{
					OptionSet newOptions = mappingOptionSet;
					MacroMap packageDirMacro = new MacroMap()
					{
						{ "packageDir", packageDir },
						{ "outputname", "%outputname%" } // allow output name to remain unresolved, it can be used in options that aren't resolved until later
					};

					foreach (KeyValuePair<string, string> e in mappingOptionSet.Options)
					{
						string macroReplace = MacroMap.Replace(e.Value, packageDirMacro);
						if (!ReferenceEquals(macroReplace, e.Value)) // if reference is not equal then macromap replaced the string
						{
							if (ReferenceEquals(newOptions, mappingOptionSet)) // if this is the original optionset create a copy to return
							{
								newOptions = new OptionSet(newOptions);
							}

							newOptions.Options[e.Key] = macroReplace;
						}
					}

					mappingOptionSet = newOptions;
				}
			}
			return mappingOptionSet;
		}

        // output mapping optionset can override config bin dir and config lib dir (but doesn't support tokens) - see
        // SetRemappedLibBinDirectories() in Combine.cs
        private static string GetRemappedConfigDir(string propertyName, OptionSet outputMapping, string defaultValue)
        {
            if (outputMapping != null)
            {
                string remappedDir = null;
                if (outputMapping.Options.TryGetValue(propertyName, out remappedDir))
                {
                    return remappedDir;
                }
            }
            return defaultValue;
        }

        private static string GetLibraryPrefix(Project project)
		{
			return project.Properties["lib-prefix"];
		}

		private static string GetLibrarySuffix(Project project)
		{
			return project.Properties["lib-suffix"];
		}

		private static string GetBinaryPrefix(Project project)
		{
			return String.Empty;
		}

		private static string GetBinarySuffix(Project project, BuildType buildType)
		{
			// csharp needs special handling, we can define csharp module in any config
			// but we don't want them to use the dll-suffix property for that platform
			// so we hardcode them here
			if (buildType.IsCSharp)
			{
				OptionSet buildOptionSet = project.NamedOptionSets[buildType.Name];
				DotNetCompiler.Targets target = ConvertUtil.ToEnum<DotNetCompiler.Targets>(buildOptionSet.Options["csc.target"]);
				if (target == DotNetCompiler.Targets.Library)
				{
					return ".dll";
				}
				else if (target == DotNetCompiler.Targets.Exe || target == DotNetCompiler.Targets.WinExe)
				{
					return ".exe";
				}
			}
			else
			{
				// assuming anything left is native type, this ignores other types like python types
				if (buildType.IsProgram)
				{
					return project.Properties["exe-suffix"];
				}
				else if (buildType.IsDynamic)
				{
					return project.Properties["dll-suffix"];
				}
			}
			return project.Properties["exe-suffix"];
		}
	}
}
