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
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Core.Writers;

using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Tasks.Model;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;

using XmlWriter = NAnt.Core.Writers.XmlWriter;

namespace EA.Eaconfig.Backends.VisualStudio
{
	public class VCPPDirectories
	{
		public VCPPDirectories()
		{
			InheritExecutablePath = true;
			InheritIncludePath = true;
			InheritReferencePath = true;
			InheritLibraryPath = true;
			InheritSourcePath = true;
			InheritExcludePath = true;

			// Support for these two are new and therefore they are not used by default.  Setting these to false
			// and not provide any entries to the list will stop the field from being created in the vcxproj file.
			InheritLibraryWPath = false;
			InheritNativeExecutablePath = false;
		}

		// These properties are created in Framework 5.05.00? release.  If you package needs to work with
		// older version of Framework, you may need to use reflection and test for the presence of 
		// these properties before using them.
		// NOTE: If you set these to false, you must provide valid entries to the following corresponding
		// list, otherwise, the field will not be created in vcxproj and Visual Studio will default to 
		// inherit from default values.
		public bool InheritExecutablePath { get; set; }
		public bool InheritIncludePath { get; set; }
		public bool InheritReferencePath { get; set; }
		public bool InheritLibraryPath { get; set; }
		public bool InheritLibraryWPath { get; set; }
		public bool InheritSourcePath { get; set; }
		public bool InheritExcludePath { get; set; }
		public bool InheritNativeExecutablePath { get; set; }

		public readonly List<PathString> ExecutableDirectories = new List<PathString>();
		public readonly List<PathString> IncludeDirectories = new List<PathString>();
		public readonly List<PathString> ReferenceDirectories = new List<PathString>();
		public readonly List<PathString> LibraryDirectories = new List<PathString>();
		public readonly List<PathString> SourceDirectories = new List<PathString>();
		public readonly List<PathString> ExcludeDirectories = new List<PathString>();

		// Note that these data member is available only after Framework 5.05.00 release.  So please make
		// sure you use reflection to test for these properties before you use them.
		public readonly List<PathString> LibraryWinRTDirectories = new List<PathString>();
		// The NativeExecutablePath field is not visible from inside Visual Studio GUI.  And it is only used starting with
		// VS 2012.  So far, only XBox1 build requies us to set this field in order to properly set up the WindowsSDK
		// path to find rc.exe.
		public readonly List<PathString> NativeExecutableDirectories = new List<PathString>();
	}

	internal abstract partial class VSCppMsbuildProject : VSCppProject
	{
		private readonly Dictionary<uint, string> s_moduleTypesToConfigurations = new Dictionary<uint, string>()
		{
			{ Module.DynamicLibrary, "DynamicLibrary" },
			{ Module.Library, "StaticLibrary" },
			{ Module.MakeStyle, "Makefile" },
			{ Module.Program, "Application" },
			{ Module.Utility, "Utility" },
			{ Module.Java, "Utility" } // assumes .apk and .aar have been filtered out to androidproj already
		};

		public override IEnumerable<SolutionEntry> SolutionEntries
		{
			get
			{
				IEnumerable<SolutionEntry> baseEntries = base.SolutionEntries;

				// msvs-android has an extra project for packaging
				Configuration[] androidPackagingConfigs = Modules
					.Where(mod => IsVisualStudioAndroid(mod))
					.Where(mod => mod.IsKindOf(Module.Program))
					.Select(mod => mod.Configuration)
					.ToArray();

				if (!androidPackagingConfigs.Any())
				{
					return baseEntries;
				}

				string packagingName = Name + ".Packaging";
				// return the packaging project first, if this is the default startup module then we want the packaging project to be the default startup project
				return new SolutionEntry[]
				{
					new SolutionEntry
					(
						packagingName,
						VSSolutionBase.ToString(Hash.MakeGUIDfromString(packagingName)),
						ProjectFileNameWithoutExtension + ".Packaging.androidproj",
						RelativeDir,
						ANDROID_PACKAGING_GUID,
						validConfigs: androidPackagingConfigs,
						deployConfigs: androidPackagingConfigs
					)
				}
				.Concat(baseEntries);
			}
		}

		protected VSCppMsbuildProject(VSSolutionBase solution, IEnumerable<IModule> modules)
			: base(solution, modules)
		{
			HashSet<string> nonUnixVSPlatforms = new HashSet<string>();
			HashSet<string> unixVSPlatforms = new HashSet<string>();
			foreach (var module in modules)
			{
				string platform = GetProjectTargetPlatform(module.Configuration);
				if (module.Configuration.System.StartsWith("unix"))
				{
					if (!unixVSPlatforms.Contains(platform))
						unixVSPlatforms.Add(platform);
				}
				else
				{
					if (!nonUnixVSPlatforms.Contains(platform))
						nonUnixVSPlatforms.Add(platform);
				}
			}
			if (nonUnixVSPlatforms.Intersect(unixVSPlatforms).Any() == false)
			{
				// We need to make sure that the resolved "platform" information cannot be the same 
				// between unix configs and non-unix configs.  Because there are settings that need to be
				// setup under project Global section and Visual Studio seems to only check "platform" information
				// (rather than "config|platform") to switch between normal project property dialog box vs Unix Specific
				// project dialog.
				mCanUseVSUnixMakeProject = true;    // This data member is defined in base class VSCppMsBuildProject
			}

			mVSSupportUnityFlag = (StartupModule.Package.Project.Properties["config-vs-full-version"] ?? "0").StrCompareVersions("15.9.0.0") >= 0
					// Give people means to disable it if things goes really wrong.
					&& StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault("enable-vs-unity-build",true);
		}

		protected string DefaultToolset { get => SolutionGenerator.VSToolsetVersion; }
		protected string ToolsVersion { get => SolutionGenerator.VSVersion.Substring(0, 2) + ".0"; }

		protected abstract string CppToolsVersion { get; }

		protected virtual string GetDefaultAndroidPlatformToolset(IModule module)
		{
			if (module.Package.Project.Properties["package.android_config.build-system"].StartsWith("msvs"))
				return "Clang_5_0";
			else if (module.Package.Project.Properties["package.android_config.build-system"].StartsWith("tegra"))
				return "DefaultClang";
			else
				return "";
		}

		protected override string UserFileName
		{
			get
			{
				return ProjectFileName + ".user";
			}
		}

		internal bool VSSupportUnityFlag
		{
			get { return mVSSupportUnityFlag; }
		} protected bool mVSSupportUnityFlag = false;


		internal class UseVSUnityCacheInfo
		{
			public UseVSUnityCacheInfo(bool enable, HashSet<string> realCompileFileList) { Enable = enable; RealCompileFileList = realCompileFileList; }
			public bool Enable = false;
			public HashSet<string> RealCompileFileList = null;
		}
		// mConfigEnabledUnityFlag is setup in WriteConfigurations() which is called pretty early on.  We need to make sure that
		// the rest of the project setup is consistent with unity flag being enabled.
		private Dictionary<Configuration, UseVSUnityCacheInfo> mConfigVSUnityCacheInfo = new Dictionary<Configuration, UseVSUnityCacheInfo>();
		internal bool IsConfigEnabledForVSUnityFlag(Configuration config)
		{
			return mConfigVSUnityCacheInfo.ContainsKey(config) ? mConfigVSUnityCacheInfo[config].Enable : false;
		}
		internal bool IsSrcFileForConfigInCompileList(Configuration config, string fileFullPath)
		{
			return mConfigVSUnityCacheInfo.ContainsKey(config) ? 
					(mConfigVSUnityCacheInfo[config].RealCompileFileList != null ? mConfigVSUnityCacheInfo[config].RealCompileFileList.Contains(fileFullPath) : true) : 
					true;
		}

		internal bool CanUseVSUnixMakeProject
		{
			get { return mCanUseVSUnixMakeProject; }
		} protected bool mCanUseVSUnixMakeProject = false;

		internal bool ModuleShouldUseVSUnixMakeProject(IModule module)
		{
			if (CanUseVSUnixMakeProject
				&& module.IsKindOf(Module.ForceMakeStyleForVcproj)
				&& (module.Configuration.System.StartsWith("unix"))
				&& (module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.unix.use-vs-make-support", true))
				)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		internal bool UnixDebugOnWsl(IModule module)
		{
			return module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.unix.vcxproj-debug-on-wsl", false);
		}

		internal bool ShouldAddVSUnixDeployPostBuildStep(IModule module)
		{
			return
				   ModuleShouldUseVSUnixMakeProject(module)
				&& module.IsKindOf(Module.Program)      // Only need to deploy for program modules.
				&& module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.unix.vcxproj-deploy-during-build", false)
				&& String.IsNullOrEmpty(module.Package.Project.Properties.GetPropertyOrDefault("eaconfig.unix.deploymentroot", null)) == false
				;
		}

		protected override string Keyword
		{
			get
			{
				string keyword = String.Empty;
				foreach (var module in Modules) // Modules are list of same module in all different configs.
				{
					string moduleKeyword = "Win32Proj";
					if (ModuleShouldUseVSUnixMakeProject(module))
					{
						moduleKeyword = "Linux";
					}
					else if (module.IsKindOf(Module.MakeStyle))
					{
						moduleKeyword = "MakeFileProj";
					}
					else if (module.IsKindOf(Module.Managed))
					{
						moduleKeyword = "ManagedCProj";
					}
					else
					{
						moduleKeyword = GetProjectTargetPlatform(module.Configuration) + "Proj";
					}

					if (String.IsNullOrEmpty(keyword))
					{
						keyword = moduleKeyword;
					}
					else if (keyword != moduleKeyword)
					{
						keyword = "MixedTypesProj";
						break;
					}
				}

				return keyword;
			}
		}

		protected override string DefaultTargetFrameworkVersion
		{
			get { return "4.0"; }
		}

		private DotNetFrameworkFamilies TargetFrameworkFamily
		{
			get
			{
				if (StartupModule is Module_Native nativeStartUp && nativeStartUp.IsKindOf(Module.Managed))
				{
					return nativeStartUp.TargetFrameworkFamily;
				}
				return DotNetFrameworkFamilies.Framework;
			}
		}

		protected override string TargetFrameworkVersion
		{
			get
			{
				if (StartupModule.IsKindOf(Module.Managed) && StartupModule is Module_Native)
				{
					string targetframework = (StartupModule as Module_Native).TargetFrameworkVersion;

					string dotNetVersion = String.IsNullOrEmpty(targetframework) ? DefaultTargetFrameworkVersion : targetframework;

					if (dotNetVersion.StartsWith("v") || dotNetVersion.StartsWith("net"))
					{
						return dotNetVersion;
					}

					// For backwards compatibility with Framework 2 where "v" was not prepended or Framework version is derived from .Net package version.
					//Set target framework version according to VS2010 format:

					if (dotNetVersion.StartsWith("2.0"))
					{
						return "v2.0";
					}
					else if (dotNetVersion.StartsWith("3.0"))
					{
						return "v3.0";
					}
					else if (dotNetVersion.StartsWith("3.5"))
					{
						return "v3.5";
					}
					else if (dotNetVersion.StartsWith("4.0"))
					{
						return "v4.0";
					}
					// we accept 4.51 as well because we released the package as version 4.51
					else if (dotNetVersion.StartsWith("4.5.1") || dotNetVersion.StartsWith("4.51"))
					{
						return "v4.5.1";
					}
					// we technically didn't release package for 4.5.2, but to be complete, we should add them as well.
					else if (dotNetVersion.StartsWith("4.5.2") || dotNetVersion.StartsWith("4.52"))
					{
						return "v4.5.2";
					}
					else if (dotNetVersion.StartsWith("4.5"))
					{
						return "v4.5";
					}
					else if (dotNetVersion.StartsWith("4.6"))
					{
						return "v4.6";
					}
					else if (dotNetVersion.StartsWith("5.0"))
					{
						return "v5.0";
					}

					return DefaultTargetFrameworkVersion.StartsWith("v") ? DefaultTargetFrameworkVersion : "v" + DefaultTargetFrameworkVersion;
				}
				else
				{
					return DotNetTargeting.GetNetVersion(StartupModule.Package.Project, DotNetFrameworkFamilies.Framework);
				}
			}
		}

		protected override IEnumerable<string> GetAllDefines(CcCompiler compiler, Configuration configuration)
		{
			return base.GetAllDefines(compiler, configuration).UnescapeDefines();
		}

		protected virtual bool IsWinRTProgram(Module_Native module)
		{
			if (module != null && module.IsKindOf(Module.Program))
			{
				if (module.IsKindOf(Module.WinRT))
				{
					return true;
				}
				if (module.Package.Project.NamedOptionSets[module.BuildType.Name].GetBooleanOptionOrDefault("genappxmanifest", false))
				{
					return true;
				}
			}
			return false;
		}

		protected virtual bool IsGDKEnabledProgram(Module module)
		{
			if (module != null && module.IsKindOf(Module.Program))
			{
				return module.Package.Project.Properties.GetBooleanPropertyOrDefault("gdk.enabled", false);
			}
			return false;
		}

		protected virtual bool IsWinRTLibrary(Module_Native module)
		{
			if (module != null && module.IsKindOf(Module.Library))
			{
				if (module.IsKindOf(Module.WinRT))
				{
					return true;
				}
			}
			return false;
		}

		#region Write methods

		partial void WriteSnowCacheRead(IXmlWriter writer);
		partial void WriteSnowCacheWrite(IXmlWriter writer);

		protected override void WriteProject(IXmlWriter writer)
		{
			SetCustomBuildRulesInfo();

			WriteVisualStudioProject(writer);
			WriteSnowCacheRead(writer);
			WriteStartupConfig(writer);
			WriteMSBuildExtensionsPathOverride(writer);
			WriteExtensionProjectProperties(writer);
			WritePlatforms(writer);
			WriteProjectGlobals(writer);

			WriteMSVCOverride(writer);
			ImportProject(writer, @"$(VCTargetsPath)\Microsoft.Cpp.Default.props");

			WriteConfigurations(writer);

			ImportProject(writer, @"$(VCTargetsPath)\Microsoft.Cpp.props");
			
			WriteCustomBuildPropFiles(writer);

			IEnumerable<ProjectRefEntry> projectReferences;
			IDictionary<PathString, FileEntry> references;
			IDictionary<PathString, FileEntry> comreferences;
			IDictionary<Configuration, ISet<PathString>> referencedAssemblyDirs;

			GetReferences(out projectReferences, out references, out comreferences, out referencedAssemblyDirs);

			WriteBuildTools(writer, referencedAssemblyDirs);

			WriteReferences(writer, projectReferences, references, comreferences, referencedAssemblyDirs);

			WriteAppPackage(writer);

			WriteFiles(writer);
			WriteCopyLocalFiles(writer, isNativeBuild: true);

			WriteMaybePreserveImportLibraryTimestamp(writer);

			WriteExtensions(writer);

			WriteSNDBSBeforeTargets(writer);
			ImportProject(writer, @"$(VCTargetsPath)\Microsoft.Cpp.targets");
			WriteSNDBSAfterTargets(writer);

			WriteImportFrameworkTargets(writer);
			WriteCustomBuildTargetsFiles(writer);
			WriteMsBuildTargetOverrides(writer);
			WriteImportTargets(writer);
			WriteSnowCacheWrite(writer);

			WriteAndroidPackagingProject();
		}

		protected override void WriteVisualStudioProject(IXmlWriter writer)
		{
			writer.WriteStartElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");
			writer.WriteAttributeString("DefaultTargets", "Build");
			writer.WriteAttributeString("ToolsVersion", ToolsVersion);

			// Allow the ability to have SDK packages to provide platform specific MSBuild properties override
			// or teams creating their team specific properties/environment setup.
			List<string> startupPropertyImports = new List<string>();
			startupPropertyImports.Add("visualstudio.startup.property.import");
			HashSet<string> systemAdded = new HashSet<string>();
			foreach (Configuration config in BuildGenerator.Configurations)
			{
				if (!systemAdded.Contains(config.System))
				{
					systemAdded.Add(config.System);
					startupPropertyImports.Add("visualstudio.startup.property.import." + config.System);
				}
			}
			HashSet<string> importFileAdded = new HashSet<string>();
			foreach (string prop in startupPropertyImports)
			{
				// Need to loop through all modules because Property list for each module (from different config) will be different.
				foreach (var module in Modules)
				{
					if (module.Package.Project.Properties.Contains(prop))
					{
						IEnumerable<string> importFileList = module.Package.Project.Properties[prop].Trim().Split().Where(p=>!String.IsNullOrEmpty(p));
						foreach (string impFile in importFileList)
						{
							string importFile = impFile.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
							if (!string.IsNullOrEmpty(writer.FileName) && BuildGenerator.IsPortable)
							{
								importFile = PathUtil.RelativePath(importFile, Path.GetDirectoryName(writer.FileName));
							}
							// To avoid possible user script error with adding the same file in different script,
							// do the following test to make sure the same file is only imported once.
							if (!importFileAdded.Contains(importFile))
							{
								writer.WriteStartElement("Import");
								writer.WriteAttributeString("Project", importFile);
								writer.WriteEndElement();
								importFileAdded.Add(importFile);
							}
						}
						break;
					}
				}
			}
		}

		protected virtual void WriteMSBuildExtensionsPathOverride(IXmlWriter writer)
		{
			foreach (Module module in Modules)
			{
				string msBuildExtensionOverridePath = 
					module.Package.Project.Properties["visualstudio.msbuild-extension-path." + module.Configuration.System] ??
					module.Package.Project.Properties["visualstudio.msbuild-extension-path"];

				if (!String.IsNullOrWhiteSpace(msBuildExtensionOverridePath))
				{
					// overriding these properteries changes where visualstudio looks for all the .targets, .props and .dlls
					// msbuild use to build diffrent platforms
					StartPropertyGroup(writer, condition: GetConfigCondition(module.Configuration), label: "MSBuild Paths Override");
					writer.WriteElementString("MSBuildExtensionsPath", msBuildExtensionOverridePath);
					writer.WriteElementString("VCTargetsPath", String.Format(@"$(MSBuildExtensionsPath)\Microsoft.Cpp\{0}\{1}\", CppToolsVersion, DefaultToolset));
					writer.WriteElementString("VCTargetsPath10", String.Format(@"$(MSBuildExtensionsPath)\Microsoft.Cpp\{0}\", CppToolsVersion));
					writer.WriteElementString("VCTargetsPath11", String.Format(@"$(MSBuildExtensionsPath)\Microsoft.Cpp\{0}\v110\", CppToolsVersion));
					writer.WriteElementString("VCTargetsPath12", String.Format(@"$(MSBuildExtensionsPath)\Microsoft.Cpp\{0}\v120\", CppToolsVersion));
					writer.WriteElementString("VCTargetsPath14", String.Format(@"$(MSBuildExtensionsPath)\Microsoft.Cpp\{0}\v140\", CppToolsVersion));
					writer.WriteEndElement(); // PropertyGroup
				}
			}
		}

		protected virtual void WriteStartupConfig(IXmlWriter writer)
		{
			// Set default initial configuration
			bool vsDefaultIsValid = Modules.Any(m => "Debug" == GetVSProjTargetConfiguration(m.Configuration) && "Win32" == GetProjectTargetPlatform(m.Configuration));
			if (!vsDefaultIsValid)
			{
				var startupConfigName = GetVSProjTargetConfiguration(BuildGenerator.StartupConfiguration);
				var startupPlatform = GetProjectTargetPlatform(BuildGenerator.StartupConfiguration);

				StartPropertyGroup(writer, condition: "'$(Configuration)' == 'Debug' and '$(Platform)' == 'Win32'");
				if (startupConfigName != "Debug")
				{
					writer.WriteElementString("Configuration", startupConfigName);
				}
				if (startupPlatform != "Win32")
				{
					writer.WriteElementString("Platform", startupPlatform);
				}
				writer.WriteEndElement(); // PropertyGroup
			}
		}

		protected override void WritePlatforms(IXmlWriter writer)
		{
			writer.WriteStartElement("ItemGroup");
			writer.WriteAttributeString("Label", "ProjectConfigurations");

			// vcxproj generation requires setting up ProjectConfigurations for all permutations of
			// $(Configuration)|$(Platform) even for invalid combo.  Otherwise, when you open the solution, you will constantly get
			// warning message from Visual Studio asking for permission to fix up the solution.
			// We properly disable that invalid combo by correctly setup the .sln file generation to disable that combo form build
			// (ie not setup the .Build.0 entry).

			IEnumerable<string> configNameList = Modules.Select(module => GetVSProjTargetConfiguration(module.Configuration)).OrderedDistinct();
			IEnumerable<string> platformNameList = Modules.Select(module => GetProjectTargetPlatform(module.Configuration)).OrderedDistinct();

			foreach (string platform in platformNameList)
			{
				foreach (string configName in configNameList)
				{
					writer.WriteStartElement("ProjectConfiguration");
					writer.WriteAttributeString("Include", configName + "|" + platform);
					writer.WriteElementString("Configuration", configName);
					writer.WriteElementString("Platform", platform);
					writer.WriteEndElement(); // ProjectConfiguration
				}
			}

			writer.WriteEndElement(); // ItemGroup - ProjectConfigurations

		}

		protected virtual void AddProjectGlobals(IDictionary<string, string> projectGlobals)
		{
		}

		protected virtual void WriteProjectGlobals(IXmlWriter writer)
		{
			var projectGlobals = new OrderedDictionary<string, string>();

			var unixMakeStyleSpecificGlobals = new OrderedDictionary<string, string>();
			unixMakeStyleSpecificGlobals.Add("ApplicationType", "Linux");
			unixMakeStyleSpecificGlobals.Add("ApplicationTypeRevision", "1.0");
			unixMakeStyleSpecificGlobals.Add("TargetLinuxPlatform", "Generic");
			unixMakeStyleSpecificGlobals.Add("LinuxProjectType", "{FC1A4D80-50E9-41DA-9192-61C0DBAA00D2}");

			var projectProperties = StartupModule.Package.Project.Properties;

			projectGlobals.Add("ProjectGuid", ProjectGuidString);
			projectGlobals.Add("Keyword", Keyword);
			projectGlobals.Add("ProjectName", Name);
			if (TargetFrameworkFamily == DotNetFrameworkFamilies.Framework)
			{
				projectGlobals.Add("TargetFrameworkVersion", TargetFrameworkVersion);
			}
			else
			{
				projectGlobals.Add("TargetFramework", TargetFrameworkVersion);
			}
			projectGlobals.Add("RootNamespace", RootNamespace);
			projectGlobals.Add("UseOfStl", "c++_static");

			// TODO unify with VSDotNetProject
			// -dvaliant 2018/02/28
			/*
			 below chunk of code is *almost* identical to one in DotNet project (because of managed c++ and the fact that
			 makestyle projects try to resolve .net assemblies), in a future sln gen refactor it would be nice tp unify these
			 */
			// Point visual studio at the WindowsSDK for the case where the user is using the non-proxy and has removed the installed dotnet tools 
			if (!BuildGenerator.IsPortable)
			{
				string toolsDir = projectProperties.GetPropertyOrDefault("package.WindowsSDK.dotnet.tools.dir", null);
				if (!String.IsNullOrEmpty(toolsDir))
				{
					if (Environment.Is64BitOperatingSystem)
					{
						toolsDir = Path.Combine(toolsDir,"x64");
					}
					projectGlobals.AddNonEmpty("TargetFrameworkSDKToolsDirectory", toolsDir);
				}

				if (TargetFrameworkFamily == DotNetFrameworkFamilies.Framework)
				{
					// If set, don't let VS try to resolve installed .net sdk and use explicitly root
					string referenceAssemblyDir = projectProperties["package.DotNet.vs-referencedir-override"];
					projectGlobals.AddNonEmpty("TargetFrameworkRootPath", referenceAssemblyDir);
				}

				if (projectProperties.GetBooleanPropertyOrDefault("package.VisualStudio.useCustomMSVC", false))
				{
					if (projectProperties.GetBooleanPropertyOrDefault("package.VisualStudio.use-non-proxy-build-tools", false))
					{
						projectGlobals.AddNonEmpty("VsInstallRoot", projectProperties.GetPropertyOrFail("package.MSBuildTools.appdir"));
					}
				}
			}

			bool onlyUnixSetup = true;
			bool hasSpecialUnixSetup = false;
			HashSet<Configuration> allUnixConfigs = new HashSet<Configuration>();
			foreach (var module in Modules)
			{
				if (module.Configuration.System.StartsWith("unix") == false)
					onlyUnixSetup = false;

				if (ModuleShouldUseVSUnixMakeProject(module))
				{
					hasSpecialUnixSetup = true;
					if (!allUnixConfigs.Contains(module.Configuration))
						allUnixConfigs.Add(module.Configuration);
				}
			}

			AddProjectGlobals(projectGlobals);

			// Invoke Visual Studio extensions
			foreach (var module in Modules)
			{
				ExecuteExtensions(module, extension => extension.ProjectGlobals(projectGlobals));
			}

			writer.WriteStartElement("PropertyGroup");
			writer.WriteAttributeString("Label", "Globals");
			foreach (var entry in projectGlobals)
			{
				var val = entry.Value;
				if (BuildGenerator.IsPortable)
				{
					val = BuildGenerator.PortableData.NormalizeIfPathString(val, OutputDir.Path, quote : false);
				}

				writer.WriteNonEmptyElementString(entry.Key, val);
			}

			if (hasSpecialUnixSetup)
			{
				IEnumerable<string> vsPlatformListForUnix = allUnixConfigs.Select(config => GetProjectTargetPlatform(config)).OrderedDistinct();
				foreach (string vsUnixPlatform in vsPlatformListForUnix)
				{
					string conditions = " '$(Platform)' == '" + vsUnixPlatform + "' ";
					foreach (KeyValuePair<string, string> kvpair in unixMakeStyleSpecificGlobals)
					{
						writer.WriteStartElement(kvpair.Key);
						if (!onlyUnixSetup)
						{
							writer.WriteAttributeString("Condition", conditions);
						}
						writer.WriteString(kvpair.Value);
						writer.WriteEndElement();
					}

					if (onlyUnixSetup)
					{
						// Only need to list it once if all configs are unix configs.
						break;
					}
				}
			}

			writer.WriteEndElement();
		}

		protected override void WriteConfigurations(IXmlWriter writer)
		{
			foreach (Module module in Modules)
			{
				StartPropertyGroup(writer, condition: GetConfigCondition(module.Configuration), label: "Configuration");

				mConfigVSUnityCacheInfo[module.Configuration] = new UseVSUnityCacheInfo(false, new HashSet<string>());

				if (module.IsKindOf(Module.ForceMakeStyleForVcproj))
				{
					writer.WriteElementString("ConfigurationType", "Makefile");
					if (!ModuleShouldUseVSUnixMakeProject(module))
					{
						// Only add the toolset info if we are setting up regular Make Project.  If we are creating
						// the special Linux Make project, don't setup PlatformToolset info.
						writer.WriteElementString("PlatformToolset", GetPlatformToolSet(module));
					}
					else // Setting up Visual Studio's special Linux Make file project.
					{
						Linker linkerTool = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;
						PathString outDir = module.OutputDir;
						if (module.OutputDir == null)
						{
							var msg = String.Format("{0}-{1} ({2}) Module '{3}.{4}' OutputDirectory is not set.", module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup.ToString(), module.Name);
							throw new BuildException(msg);
						}
						if (linkerTool != null && linkerTool.LinkOutputDir != null)
						{
							outDir = linkerTool.LinkOutputDir;
						}
						string normalizedOutDir = outDir.Normalize().Path;

						string outputRootDir = PathString.MakeNormalized(module.PropGroupValue("output_rootdir", "")).Path;
						string outputRelativeDir = null;
						if (!String.IsNullOrEmpty(outputRootDir) && normalizedOutDir.StartsWith(outputRootDir))
						{
							outputRelativeDir = PathUtil.RelativePath(normalizedOutDir, outputRootDir);
						}
						else
						{
							outputRelativeDir = Path.Combine("$(ProjectName)","$(Platform)","$(Configuration)");
						}
						outputRootDir = outputRootDir.Replace('\\', '/').TrimEnd(new char[] { '/' });
						outputRelativeDir = outputRelativeDir.Replace('\\', '/').TrimEnd(new char[] { '/' });
						if (!String.IsNullOrEmpty(outputRelativeDir))
						{
							outputRelativeDir = "/" + outputRelativeDir;
						}

						// Now actually override Visual Studio's default setup of the MSBuild properties override.
						if (UnixDebugOnWsl(module))
						{
							if (String.IsNullOrEmpty(outputRootDir))
							{
								outputRootDir = normalizedOutDir.Replace('\\','/');
								outputRelativeDir = "";
							}
							string drive_letter = outputRootDir.Substring(0, 1);
							string wslRemoteRootDir = outputRootDir.Replace(drive_letter + ":", "/mnt/" + drive_letter.ToLowerInvariant()).Replace(@"\", @"/");
							writer.WriteElementString("RemoteRootDir", wslRemoteRootDir);
							writer.WriteElementString("RemoteProjectDir", "$(RemoteRootDir)/$(ProjectName)");
							writer.WriteElementString("RemoteDeployDir", "$(RemoteRootDir)" + outputRelativeDir);
						}
						else if (ShouldAddVSUnixDeployPostBuildStep(module))
						{
							string deployroot = module.Package.Project.Properties["eaconfig.unix.deploymentroot"].TrimEnd(new char[] { '/' });
							if (!String.IsNullOrEmpty(deployroot))
							{
								writer.WriteElementString("RemoteRootDir", deployroot);
								writer.WriteElementString("RemoteProjectDir", "$(RemoteRootDir)/$(ProjectName)");
								writer.WriteElementString("RemoteDeployDir", "$(RemoteRootDir)" + outputRelativeDir);
							}
						}
					}
				}
				else
				{
					var configtype = GetConfigurationType(module);
					if (configtype == "StaticLibrary")
					{
						// For static library when there is no src files, no obj files or custombuildsteps - change type to utility. Otherwize librarian tool may fail.
						if (module.Tools.Count() <= 3)
						{
							var native = module as Module_Native;
							if ((native.Cc == null || native.Cc.SourceFiles.Includes.Count == 0)
							  && (native.Asm == null || native.Asm.SourceFiles.Includes.Count == 0)
							  && (native.Lib == null || native.Lib.ObjectFiles.Includes.Count == 0))
							{
								configtype = "Utility";
							}
						}
					}
					if (configtype == "StaticLibrary" && (module.Configuration.System == "pc" || module.Configuration.System == "pc64"))
					{
						// DAVE-REFACTOR-TODO: nuke use-versioned-vstmp

						// enables two tasks in Framework MSBuild tasks that run before and after lib step to check if
						// .lib has been built by another means and delete the lib if it is newer than when we last built
						// it, this is an extension of use-versioned-vstmp feature as it allows different vs version
						// to have different intermediate folders but the same output folder and still build correctly
						// only known to be required on pc for now 
						bool useVersionedVstmp = module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualsudio.use-versioned-vstmp", false) || module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.use-versioned-vstmp", false);
						if (useVersionedVstmp && (module.Configuration.System == "pc" || module.Configuration.System == "pc64"))
						{
							writer.WriteElementString("CheckForNewerLibTimeStamp", true);
						}
					}

					writer.WriteElementString("ConfigurationType", configtype);
					writer.WriteElementString("CharacterSet", CharacterSet(module).ToString());
					writer.WriteElementString("PlatformToolset", GetPlatformToolSet(module));
					if (module.Configuration.Compiler == "vc")
					{
						// Use 64 bit tools if possible for vc compiler. Affects only VS2013
						writer.WriteElementString("UseNativeEnvironment", "true");
						// Force using 64 bit toolset.
						if (Environment.Is64BitOperatingSystem && Version.StrCompareVersions("12.00") < 0 && module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.force-64bit-toolset", false))
						{
							writer.WriteElementString("_IsNativeEnvironment", "true");
						}
					}

					Module_Native nativeModule = module as Module_Native;
					if (nativeModule != null && nativeModule.Cc != null && nativeModule.Cc.SourceFiles != null && nativeModule.Cc.SourceFiles.FileItems.Any())
					{
						HashSet<string> srcBulkBuildList = new HashSet<string>();
						FileSet fs = nativeModule.GetPlatformModuleFileSet("bulkbuild.sourcefiles");
						if (fs != null && fs.FileItems.Any())
						{
							srcBulkBuildList = new HashSet<string>(fs.FileItems.Select(fitm => fitm.FileName));
						}

						HashSet<string> realCompilerFileList = new HashSet<string>(nativeModule.Cc.SourceFiles.FileItems.Select(fitm => fitm.FileName));

						// [compiler filelist] - [files marked for bulkbuild] = [loose files] + [auto generated bulkbuild files]
						// We really just wanted to check that there are auto generated bulkbuild files.  But if there are no
						// auto generated files, then [compiler filelist] should be identical to [files marked for bulkbuild]
						HashSet<string> looseFilesAndBulkBuildFiles = new HashSet<string>(realCompilerFileList.Except(srcBulkBuildList));

						if (VSSupportUnityFlag
								&& module.Configuration.Compiler == "vc"
								&& module.IsKindOf(Module.DynamicLibrary | Module.Library | Module.Program)
								// Note that testing IsBulkBuild isn't good enough.  Need to make sure there is a bulkbuild fileset.
								&& module.IsBulkBuild()
								// Make sure we actually have files listed in bulkbuild filelist
								&& srcBulkBuildList.Any()
								// Logic still allow for unlikely possibility that every file listed in srcBulkBuildList has
								// custom build options and framework decides not to generate any bulkbuild files.  In that case
								// the looseFilesAndBulkBuildFiles info contains no bulkbuild files.  With Unity flag enabled,
								// it is still buildable as we mark all loose files with not in unity file.  Just not efficient.
								// But I would blame that inefficiency to build file setup.
								&& looseFilesAndBulkBuildFiles.Any())
						{
							writer.WriteElementString("EnableUnitySupport", "true");
							mConfigVSUnityCacheInfo[module.Configuration].Enable = true;
							mConfigVSUnityCacheInfo[module.Configuration].RealCompileFileList = realCompilerFileList;
						}
					}
				}

				IDictionary<string, string> nameToXMLValue = new SortedDictionary<string, string>();

				CcCompiler cc = module.Tools.SingleOrDefault(t => t.ToolName == "cc") as CcCompiler;
				if (cc != null)
				{
					ProcessSwitches(module.Package.Project,VSConfig.GetParseDirectives(module).General, nameToXMLValue, cc.Options, "general", false);

				}

				Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;
				if (link != null)
				{
					ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).General, nameToXMLValue, link.Options, "general", false);
				}

				if (nameToXMLValue.ContainsKey("CLRSupport") && TargetFrameworkFamily == DotNetFrameworkFamilies.Core)
				{
					nameToXMLValue["CLRSupport"] = "NetCore";
				}

				if (!nameToXMLValue.ContainsKey("UseDebugLibraries"))
				{
					var pmodule = module as ProcessableModule;
					if (pmodule != null && module.Package.Project != null)
					{
						bool usedebuglibs = OptionSetUtil.GetOptionSetOrFail(module.Package.Project, pmodule.BuildType.Name).GetBooleanOptionOrDefault("usedebuglibs", false);

						nameToXMLValue.Add("UseDebugLibraries", usedebuglibs.ToString());
					}
				}

				Module_Native nativeMod = module as Module_Native;
				if (IsWinRTProgram(nativeMod) || IsWinRTLibrary(nativeMod))
				{
					nameToXMLValue["DefaultLanguage"] = "en-US";
					// The "genappxmanifest" is only set for capilano build.
					if (nativeMod.Package.Project.NamedOptionSets[nativeMod.BuildType.Name].GetBooleanOptionOrDefault("genappxmanifest", false))
					{
						nameToXMLValue["MinimumVisualStudioVersion"] = Version;		// Capilano build now supports both VS 2012 and VS 2015.
					}
					else
					{
						nameToXMLValue["MinimumVisualStudioVersion"]= "11.0";
					}
					nameToXMLValue["AppContainerApplication"] = "true";
				}

				bool setDisableUpToDateCheck = module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.native.set-disable-uptodatecheck",
																			module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.set-disable-uptodatecheck", false));

				if (setDisableUpToDateCheck)
				{
					writer.WriteElementString("DisableFastUpToDateCheck", "true");
				}


				ExecuteExtensions(module, extension => extension.ProjectConfiguration(nameToXMLValue));

				WriteNamedElements(writer, nameToXMLValue);

				writer.WriteEndElement(); // End PropertyGroup - Configuration
			}
		}

		protected virtual void WriteImportTargets(IXmlWriter writer)
		{
			if (StartupModule is Module_Native)
			{
				foreach (string importProject in (StartupModule as Module_Native).ImportMSBuildProjects.LinesToArray())
				{
					writer.WriteStartElement("Import");
					writer.WriteAttributeString("Project", importProject.TrimQuotes());
					writer.WriteEndElement(); //Import
				}
			}
		}

		protected virtual void WriteCustomBuildPropFiles(IXmlWriter writer)
		{
			foreach (var group in _custom_rules_per_file_info.GroupBy(info => info.PropFile))
			{
				writer.WriteStartElement("ImportGroup");
				writer.WriteAttributeString("Label", "ExtensionSettings");
				writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(group.Select(info => info.Config)));
				ImportProject(writer, group.Key);
				writer.WriteFullEndElement(); //ImportGroup
			}
		}

		protected virtual void WriteCustomBuildTargetsFiles(IXmlWriter writer)
		{
			foreach (var group in _custom_rules_per_file_info.GroupBy(info => info.TargetsFile))
			{
				writer.WriteStartElement("ImportGroup");
				writer.WriteAttributeString("Label", "ExtensionTargets");
				writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(group.Select(info => info.Config)));
				ImportProject(writer, group.Key);
				writer.WriteFullEndElement(); //ImportGroup
			}
		}

		protected bool DoModulesDependOnPostBuild(IEnumerable<IModule> modules)
		{
			foreach (Module module in Modules)
			{
				string kOptionsSetName = module.GroupName + ".winrt.deployoptions";
				Project project = module.Package.Project;
				if (project.NamedOptionSets.ContainsKey(kOptionsSetName))
				{
					OptionSet deployoptions = project.NamedOptionSets[kOptionsSetName];
					string dependsonpostbuild = deployoptions.GetOptionOrDefault("dependsonpostbuild", "false");

					bool result;
					if (bool.TryParse(dependsonpostbuild, out result))
					{
						if (result == true)
						{
							return true;
						}
					}
				}
			}

			return false;
		}

		protected virtual void WriteMsBuildTargetOverrides(IXmlWriter writer)
		{
			foreach (Module module in Modules)
			{
				ExecuteExtensions(module, extension => extension.WriteMsBuildOverrides(writer));
			}
		}

		protected bool ModuleHasPreserveImportLibraryTimestamp(IModule module)
		{
			return (module.IsKindOf(Module.DynamicLibrary) && !module.IsKindOf(Module.Managed)) // managed dlls do not have an import library and thus we can not cheat its timestamp
			&& module.GetModuleBooleanPropertyValue("enable-preserve-importlibrary-timestamp", module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.enable-preserve-importlibrary-timestamp", false));
		}

        private string GetImportLibraryTaskAssembly(IXmlWriter writer)
        {
            string filePath = (BuildGenerator.IsPortable) ? Path.Combine(BuildGenerator.SlnFrameworkFilesRoot, "bin") : Path.Combine(PackageMap.Instance.GetFrameworkRelease().Path, "bin");
            var msbuild_assembly = Path.GetFullPath(Path.Combine(filePath, "MaybePreserveImportLibraryTimestamp.dll"));
            if (!string.IsNullOrEmpty(writer.FileName) && BuildGenerator.IsPortable)
            {
                msbuild_assembly = PathUtil.RelativePath(msbuild_assembly, Path.GetDirectoryName(writer.FileName));
            }
            return msbuild_assembly;
        }

		// Optimization to avoid rebuilding DLLs.
		protected void WriteMaybePreserveImportLibraryTimestamp(IXmlWriter writer)
		{
			var modules = Modules.Where(m => ModuleHasPreserveImportLibraryTimestamp(m));
            string msbuild_assembly = string.Empty;

            if (!modules.IsNullOrEmpty())
			{
				string condition = GetConfigCondition(modules.Select(m => m.Configuration));

                msbuild_assembly = GetImportLibraryTaskAssembly(writer);

                StartElement(writer, "UsingTask", condition: condition);
				writer.WriteAttributeString("TaskName", "EA.Framework.MsBuild.CppTasks.MaybePreserveImportLibraryTimestamp");
				writer.WriteAttributeString("AssemblyFile", msbuild_assembly);
				writer.WriteEndElement();

				var suppressLogging = modules.First().Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.custom.msbuildtasks.suppress-logging", false);

				StartElement(writer, "Target", condition: condition);
				writer.WriteAttributeString("Name", "MaybePreserveImportLibrary");
				writer.WriteAttributeString("AfterTargets", "Link");
				{
					writer.WriteStartElement("MaybePreserveImportLibraryTimestamp");
					writer.WriteAttributeString("TrackerIntermediateDirectory", "$(IntDir)\\tracker");
					writer.WriteAttributeString("ImportLibrary", "%(Link.ImportLibrary)");
					if (suppressLogging)
					{
						writer.WriteAttributeString("AllLoggingAsLow", "True");
					}
					// This task meant to be performance improvement by preserving old timestamp if detected nothing is really changed.
					// However, if this task failed (happens under incredibuild), we don't really need to fail the build.
					writer.WriteAttributeString("ContinueOnError","True");
					writer.WriteEndElement();
				}
				writer.WriteEndElement();
			}
		}

		protected override void WriteCopyLocalFiles(IXmlWriter writer, bool isNativeBuild)
		{
			base.WriteCopyLocalFiles(writer, isNativeBuild);

			// normally we inject FrameworkExecuteCopyLocal after CopyFilesToOutputDirectory but this doesn't happen for 
			// utility modules - inject target explicitly
			bool utilityCopyLocalInjected = false;
			foreach (Module module in Modules)
			{
				if (module is Module_Utility && !utilityCopyLocalInjected)
				{
					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "UtilityCopyLocal");
					writer.WriteAttributeString("DependsOnTargets", "FrameworkExecuteCopyLocal");
					writer.WriteAttributeString("AfterTargets", "AfterBuild");
					writer.WriteEndElement(); // Target

					utilityCopyLocalInjected = true;
				}
			}

			// utility modules have a special set of post build copy properties
			// handle this using copy local mechanism
			// TODO filecopystep
			// -dvaliant 2016/04/18
			// be nice to get rid of this - a more generic mechanism would be to allow
			// <Utility name="mymodule' outputdir="dir-for-files">
			//	<contentfiles blah="blah"/> 
			// </Utility>
			// 90% of the plumbing needed for this is in place
			Dictionary<string, Tuple<string, Module>> filesToCopy = new Dictionary<string, Tuple<string, Module>>(); // TODO this should be divived by config
			foreach (Module module in Modules)
			{
				if (module is Module_Utility)
				{
					int copystepIndex = 0;
					string propName = String.Format("filecopystep{0}", copystepIndex);
					string tofile = module.PropGroupValue(String.Format("{0}.tofile", propName), null);
					string todir = module.PropGroupValue(String.Format("{0}.todir", propName), null);

					while (tofile != null || todir != null)
					{
						if (tofile != null)
						{
							string srcFile = module.PropGroupValue(String.Format("{0}.file", propName), null);
							if (srcFile == null)
							{
								string modulePropName = module.PropGroupName(String.Format("{0}", propName));
								Log.Error.WriteLine("{0}.tofile property was defined but missing {0}.file property.", modulePropName);
								break;
							}
							filesToCopy[srcFile] = Tuple.Create<string, Module>(tofile, module);
						}
						else if (todir != null)
						{
							FileSet srcFileset = module.PropGroupFileSet(String.Format("{0}.fileset", propName));
							string srcFile = module.PropGroupValue(String.Format("{0}.file", propName), null);
							if (srcFileset != null)
							{
								foreach (FileItem item in srcFileset.FileItems)
								{
									string srcf = item.Path.NormalizedPath;
									string destFile = Path.GetFileName(srcf);

									filesToCopy[srcf] = Tuple.Create<string, Module>(Path.Combine(todir, destFile), module);
								}
							}
							else if (srcFile != null)
							{
								string destFile = Path.GetFileName(srcFile);
								filesToCopy[srcFile] = Tuple.Create<string, Module>(Path.Combine(todir, destFile), module);
							}
							else
							{
								string modulePropName = module.PropGroupName(String.Format("{0}", propName));
								Log.Error.WriteLine("{0}.todir property was defined but missing {0}.fileset or {0}.file.", modulePropName);
								break;
							}
						}
						else
						{
							break;
						}
						copystepIndex++;
						propName = String.Format("filecopystep{0}", copystepIndex);
						tofile = module.PropGroupValue(String.Format("{0}.tofile", propName), null);
						todir = module.PropGroupValue(String.Format("{0}.todir", propName), null);
					}
				}
			}
			if (filesToCopy.Any())
			{
				writer.WriteStartElement("ItemGroup");
				foreach (KeyValuePair<string, Tuple<string, Module>> sourceToDest in filesToCopy)
				{
					string destPath = sourceToDest.Value.Item1;
					Module destModule = sourceToDest.Value.Item2;

					writer.WriteStartElement("FrameworkCopyLocal");
					writer.WriteAttributeString("Include", sourceToDest.Key);
					writer.WriteElementString("IsFrameworkCopyLocal", "true");
					writer.WriteElementString("PrebuildDestinations", GetProjectPath(destPath));
					writer.WriteElementString("NonCritical", "true");
					writer.WriteElementString("SkipUnchanged", "false");
					writer.WriteElementString("Link", Path.Combine("CopyLocalFiles", Path.GetFileName(sourceToDest.Key)));
					writer.WriteEndElement();
				}
				writer.WriteEndElement(); // ItemGroup
			}
		}

		protected void WriteImportFrameworkTargets(IXmlWriter writer)
		{
			string filePath = Path.Combine((BuildGenerator.IsPortable) ? BuildGenerator.SlnFrameworkFilesRoot : PackageMap.Instance.GetFrameworkRelease().Path, "data");
			string propsPath = Path.GetFullPath(Path.Combine(Path.Combine(filePath, @"FrameworkMsbuild.props")));
			if (!string.IsNullOrEmpty(writer.FileName) && BuildGenerator.IsPortable)
			{
				propsPath = PathUtil.RelativePath(propsPath, Path.GetDirectoryName(writer.FileName));
			}
			writer.WriteStartElement("Import");
			writer.WriteAttributeString("Project", GetProjectPath(propsPath));
			writer.WriteEndElement(); //Import

			string tasksPath = Path.GetFullPath(Path.Combine(Path.Combine(filePath, @"FrameworkMsbuild.tasks")));
			if (!string.IsNullOrEmpty(writer.FileName) && BuildGenerator.IsPortable)
			{
				tasksPath = PathUtil.RelativePath(tasksPath, Path.GetDirectoryName(writer.FileName));
			}
			writer.WriteStartElement("Import");
			writer.WriteAttributeString("Project", GetProjectPath(tasksPath));
			writer.WriteEndElement(); //Import
		}

		private void WriteSNDBSBeforeTargets(IXmlWriter writer)
		{
			if (this.SolutionGenerator.SndbsEnabled)
			{
				IModule module = Modules.First();
				if (module.Configuration.System != "ps5" && module.Configuration.System != "kettle")
				{
					Project project = module.Package.Project;
					string vsversion = project.Properties.GetPropertyOrDefault("vsversion." + project.Properties["config-system"], project.Properties.GetPropertyOrDefault("vsversion", "2019"));
					string beforeTargetFile = project.Properties.GetPropertyOrDefault("package.SNDBS_Customizations.beforeTargetsFile_" + vsversion, null);
					if (string.IsNullOrEmpty(beforeTargetFile) == false)
					{
						writer.WriteStartElement("Import");
						writer.WriteAttributeString("Project", GetProjectPath(beforeTargetFile));
						writer.WriteEndElement(); //Import
					}
				}
			}
		}

		private void WriteSNDBSAfterTargets(IXmlWriter writer)
		{
			IModule module = Modules.First();
			if (module.Configuration.System != "ps5" && module.Configuration.System != "kettle")
			{
				Project project = module.Package.Project;
				if (this.SolutionGenerator.SndbsEnabled)
				{
					string vsversion = project.Properties.GetPropertyOrDefault("vsversion." + project.Properties["config-system"], project.Properties.GetPropertyOrDefault("vsversion", "2019"));
					string afterTargetPropsFile = project.Properties.GetPropertyOrDefault("package.SNDBS_Customizations.afterTargetsPropFile_" + vsversion, null);
					string afterTargetStubFile = project.Properties.GetPropertyOrDefault("package.SNDBS_Customizations.afterTargetsStubFile_" + vsversion, null);
					if (string.IsNullOrEmpty(afterTargetPropsFile) == false && string.IsNullOrEmpty(afterTargetStubFile) == false)
					{
						writer.WriteStartElement("Import");
						writer.WriteAttributeString("Project", GetProjectPath(afterTargetPropsFile));
						writer.WriteEndElement(); //Import

						writer.WriteStartElement("Import");
						writer.WriteAttributeString("Project", GetProjectPath(afterTargetStubFile));
						writer.WriteEndElement(); //Import
					}
				}
			}
		}

		protected virtual void WriteBuildTools(IXmlWriter writer, IDictionary<Configuration, ISet<PathString>> referencedAssemblyDirs)
		{
			foreach (Module module in Modules)
			{
				ISet<PathString> moduleRefAssembluDirs = null;
				referencedAssemblyDirs.TryGetValue(module.Configuration, out moduleRefAssembluDirs);

				WriteOneConfigurationBuildTools(writer, module, moduleRefAssembluDirs);
			}
		}

		protected virtual void WriteAppPackage(IXmlWriter writer)
		{
			foreach (Module module in Modules)
			{
				WriteOneConfigurationAppPackage(writer, module);
			}
		}

		protected virtual void WriteOneConfigurationAppPackage(IXmlWriter writer, Module module)
		{
			if (IsWinRTProgram(module as Module_Native))
			{
				var appmanifest = AppXManifest.Generate(Log, module as Module_Native, OutputDir, Name, ProjectFileNameWithoutExtension);
				if (appmanifest != null)
				{
					AddResourceWithTool(module, appmanifest.ManifestFilePath, "appxmanifest-visual-studio-tool", "AppxManifest", excludedFromBuildRespected:false);
				}
			}
			else if (IsGDKEnabledProgram(module as Module_Native))
			{
				var gameConfig = GameConfig.Generate(Log, module as Module_Native, OutputDir, Name, ProjectFileNameWithoutExtension);
				if (gameConfig != null)
				{
					AddResourceWithTool(module, gameConfig.ManifestFilePath, "gameconfig-visual-studio-tool", "CopyFileToFolders", excludedFromBuildRespected: false);
				}
			}
		}

		protected virtual void WriteExtensionProjectProperties(IXmlWriter writer)
		{
			foreach (Module module in Modules)
			{
				ExecuteExtensions(module, extension => extension.WriteExtensionProjectProperties(writer));
			}
		}

		protected virtual void WriteExtensions(IXmlWriter writer)
		{
			foreach (Module module in Modules)
			{
				WriteOneConfigurationExtension(writer, module);
			}
		}

		protected virtual void WriteOneConfigurationExtension(IXmlWriter writer, Module module)
		{
			ExecuteExtensions(module, extension => extension.WriteExtensionItems(writer));
		}

		protected virtual void WriteOneConfigurationBuildTools(IXmlWriter writer, Module module, ISet<PathString> referencedAssemblyDirs)
		{
			string configCondition = GetConfigCondition(module.Configuration);

			// This function result is needed multiple times in vaious places, so evaluate it first and cache it to a variable.
			bool moduleShouldUseVsUnixMakeProject = ModuleShouldUseVSUnixMakeProject(module);

			// Retrieve the C++ directories (system include directories) first. We need that to setup Linux make style project.
			VCPPDirectories vcppDirectories = new VCPPDirectories();
			if (!moduleShouldUseVsUnixMakeProject) // if portable don't add system include path - leave it as default and assume MSBuild will figure it out
			{
				CcCompiler compiler = module.Tools.SingleOrDefault(t => t.ToolName == "cc") as CcCompiler;
				if (compiler != null)
				{
					if (compiler.SystemIncludeDirs != null && compiler.SystemIncludeDirs.Any())
					{
						// If we have explicitly setup system includedirs, disable vcxproj's
						// inherit includepath.  If our sdk or config package set it up wrong, 
						// we should fix our sdk / config package.  We also want to make sure
						// our setup is consistent with plain command line / make file build.
						vcppDirectories.InheritIncludePath = false;
						if (BuildGenerator.IsPortable)
						{
							var generatorKvp = this.BuildGenerator.ModuleGenerators.FirstOrDefault(x => x.Value.ModuleName == module.Name);

							ModuleGenerator generator = generatorKvp.Value;

							var finalIncludeDirs = compiler.SystemIncludeDirs.Select(x => new PathString(generator.GetProjectPath(x),PathString.PathState.Normalized));

							vcppDirectories.IncludeDirectories.AddRange(finalIncludeDirs);
						}
						else 
						{
							vcppDirectories.IncludeDirectories.AddRange(compiler.SystemIncludeDirs); 
						}
					}
				}
			}
			ExecuteExtensions(module, extension => extension.SetVCPPDirectories(vcppDirectories));

			List<Tuple<string, string, string>> propSheetList = new List<Tuple<string, string, string>>();
			propSheetList.Add(new Tuple<string, string, string>(@"$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props", @"exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')", null));
			ExecuteExtensions(module, extension => extension.AddPropertySheetsImportList(propSheetList));
			ImportGroup(writer, "PropertySheets", configCondition, propSheetList);

			// --- Macros ----------------------------------------------------------------------------------
			StartPropertyGroup(writer, label: "UserMacros");
			writer.WriteEndElement();

			module.PrimaryOutput(out string targetDir, out string targetName, out string targetExt);

			// If this '${config-system}.vcproj.use_vsi_targetext' property is provided, it means the MSBuild property TargetExt is always provided by 
			// Visual Studio's platform setup and this value can also change when user change value in project's property page.
			bool useVsiTargetExt = module.Package.Project.GetPropertyOrDefault(module.Configuration.System+".vcproj.use_vsi_targetext", "false").ToLower() == "true";

			// Get the list of output associated with this module for the Linux Make project.  We need this list to properly
			// setup deployment files in the vcxproj.
			List<string> moduleOutputList = new List<string>();
			if (moduleShouldUseVsUnixMakeProject)
			{
				if (!String.IsNullOrEmpty(targetExt) || useVsiTargetExt)
				{
					moduleOutputList.Add("$(TargetName)$(TargetExt)");
				}
				else
				{
					moduleOutputList.Add("$(TargetName)");
				}
				// Add standard copy local files
				foreach (CopyLocalInfo clfile in module.CopyLocalFiles)
				{
					string copylocalFile = PathUtil.RelativePath(clfile.AbsoluteDestPath, targetDir).PathToUnix();
					moduleOutputList.Add(copylocalFile);
				}
				// Add any custom build tools that has output dependency info.
				foreach (var tool in module.Tools.Where(t => t is BuildTool))
				{
					if (tool.OutputDependencies != null)
					{
						foreach (FileItem depFile in tool.OutputDependencies.FileItems)
						{
							if (depFile.FullPath.StartsWith(targetDir))
							{
								string relFilePath = PathUtil.RelativePath(depFile.FullPath, targetDir).PathToUnix();
								moduleOutputList.Add(relFilePath);
							}
						}
					}
				}
			}

			if (module.IsKindOf(Module.MakeStyle))
			{
				WriteMakeTool(writer, "VCNMakeTool", module.Configuration, module);
			}
			else if (module.IsKindOf(Module.ForceMakeStyleForVcproj))
			{
				WriteForcedMakeTool(writer, "VCNMakeTool", module.Configuration, module, moduleShouldUseVsUnixMakeProject ? vcppDirectories.IncludeDirectories : new List<PathString>());
				ExecuteExtensions(module, extension => extension.WriteExtensionTool(writer));

				// Retrieve the -std= compiler option so that we can setup the CppLanguageStandard or CLanguageStandard being used by intellisense!
				string cppLanguageStandard = null;
				string cLanguageStandard = null;
				CcCompiler compiler = module.Tools.SingleOrDefault(t => t.ToolName == "cc") as CcCompiler;
				if (compiler != null && moduleShouldUseVsUnixMakeProject)
				{
					IEnumerable<string> stdOps = compiler.Options.Where(op => op.StartsWith("-std=") || op.StartsWith("/std="));
					if (stdOps.Any())
					{
						foreach (string stdOp in stdOps)
						{
							string stdVer = stdOp.Substring("-std=".Length).Trim();
							if (stdVer.StartsWith("c++") || stdVer.StartsWith("gnu++"))
							{
								cppLanguageStandard = stdVer;
							}
							else if (stdVer.StartsWith("c") || stdVer.StartsWith("gnu"))
							{
								cLanguageStandard = stdVer;
							}
						}
					}
				}

				List<string> linuxPostBuildDeployFiles = new List<string>();
				if (ShouldAddVSUnixDeployPostBuildStep(module) && !UnixDebugOnWsl(module) && moduleOutputList.Any())
				{
					linuxPostBuildDeployFiles.AddRange(moduleOutputList.Select( m => "$(OutDir)" + m + ":=$(RemoteDeployDir)/" + m));
				}
				if (linuxPostBuildDeployFiles.Any() || !String.IsNullOrEmpty(cppLanguageStandard) || !String.IsNullOrEmpty(cLanguageStandard))
				{
					StartElement(writer, "ItemDefinitionGroup", configCondition);
					if (linuxPostBuildDeployFiles.Any())
					{
						writer.WriteStartElement("PostBuildEvent");
						writer.WriteNonEmptyElementString("AdditionalSourcesToCopyMapping", String.Join(";", linuxPostBuildDeployFiles));
						writer.WriteEndElement();
					}
					if (!String.IsNullOrEmpty(cppLanguageStandard) || !String.IsNullOrEmpty(cLanguageStandard))
					{
						writer.WriteStartElement("ClCompile");
						writer.WriteNonEmptyElementString("CppLanguageStandard", cppLanguageStandard);
						writer.WriteNonEmptyElementString("CLanguageStandard", cLanguageStandard);
						writer.WriteEndElement();
					}
					writer.WriteEndElement();
				}
			}
			else
			{
				// Write tools:
				StartElement(writer, "ItemDefinitionGroup", configCondition);
				{
					WriteCustomBuildRuleTools(writer, module as Module_Native);

					var map = GetTools(module.Configuration);
					for (int i = 0; i < map.GetLength(0); i++)
					{
						WriteOneTool(writer, map[i, 0], module.Tools.FirstOrDefault(t => t.ToolName == map[i, 1]), module);
					}

					WriteBuildEvents(writer, "PreBuildEvent", "prebuild", BuildStep.PreBuild, module);
					WriteBuildEvents(writer, "PreLinkEvent", "prelink", BuildStep.PreLink, module);
					WriteBuildEvents(writer, "PostBuildEvent", "postbuild", BuildStep.PostBuild, module);
					WriteBuildEvents(writer, "CustomBuildStep", "custombuild", BuildStep.PostBuild, module);

					ExecuteExtensions(module, extension => extension.WriteExtensionTool(writer));
				}
				writer.WriteEndElement();
			}

			// --- Write 'General' property group ---

			StartElement(writer, "PropertyGroup", configCondition);
			{
				Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

				//MSBuild C++ does not like when OutDir differs from linker output dir. Set it to the linker output:
				var outDir = module.OutputDir;
				if (module.OutputDir == null)
				{
					var msg = String.Format("{0}-{1} ({2}) Module '{3}.{4}' OutputDirectory is not set.", module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup.ToString(), module.Name);
					throw new BuildException(msg);
				}
				if (link != null && link.LinkOutputDir != null)
				{
					outDir = link.LinkOutputDir;
				}
				// Make sure OutDir corresponds to actual linker or librarian setting
				IDictionary<string,string> nameToXMLValue;
				if (_ModuleLinkerNameToXMLValue.TryGetValue(module.Key, out nameToXMLValue))
				{
					string outputfile;
					if (nameToXMLValue != null && nameToXMLValue.TryGetValue("OutputFile", out outputfile))
					{
						if(!String.IsNullOrEmpty(outputfile) && -1 == outputfile.IndexOfAny(Path.GetInvalidPathChars()))
						{
							var outdir_fromOptions = PathString.MakeNormalized(Path.GetDirectoryName(outputfile), PathString.PathParam.NormalizeOnly);
							if (outDir != outdir_fromOptions)
							{
								// If we are doing portable solution, outdir_fromOptions will be a relative path. But outDir is still a full path.
								// We need to make sure we are indeed comparing full paths for both.
								bool skipWarning = false;
								if (BuildGenerator.IsPortable && !outdir_fromOptions.IsFullPath && outDir.IsFullPath)
								{
									// If outdir_fromOptions is a relative path, it should be relative to the vcxproj file itself.
									// Try to get the intended full path.
									PathString optOutDirFullPath = PathString.MakeNormalized(Path.Combine(Path.GetDirectoryName(writer.FileName), outdir_fromOptions.Path));
									// Skip warning if path is actually the same and we will just silently reset outDir to outdir_fromOptions below.
									skipWarning = outDir == optOutDirFullPath;
								}
								if (!skipWarning)
								{
									Log.Warning.WriteLine("{0} linker/librarian options define output directory {1} that differs from module output directory setting {2}. Use [lib/link]outputname options or Framework properties to set output, do not specify it directly in linker or librarian options.", module.ModuleIdentityString(), outdir_fromOptions.Path.Quote(), outDir.Path.Quote());
								}
								outDir = outdir_fromOptions;
							}

							string targetName_fromOptions = Path.GetFileNameWithoutExtension(outputfile);
							if (outputfile.EndsWith("$(TargetExt)"))
							{
								targetName_fromOptions = Path.GetFileName(outputfile.Substring(0, outputfile.Length - "$(TargetExt)".Length));
							}
							// Framework and MSBuild tend to get confused when no extension is set and the file name has a period in it
							// check for this and and don't warn about it if that is the case
							bool falseExtension = targetExt.IsNullOrEmpty() && !Path.GetExtension(targetName).IsNullOrEmpty();
							if (targetName != targetName_fromOptions && !falseExtension)
							{
								Log.Warning.WriteLine("{0} linker/librarian options define output name {1} that differs from module tool output name setting {2}. Use [lib/link]outputname options or Framework properties to set output, do not specify it directly in linker or librarian options.", module.ModuleIdentityString(), targetName_fromOptions.Quote(), targetName.Quote());
							}
						}
					}
				}

				if (IsVisualStudioAndroid(module) || module.Configuration.System == "stadia")
				{
					// Android: The Android build machinery (mainly GenerateApkRecipe and related targets) require that OutDir expand to a fullpath and the default in VS is that OutDir is relative to ProjectDir.
					// Stadia: Stadia requires a full path to debug properly in VS.  If the path isn't the full path, you will see popups from the Stadia VSI saying it can't find the executable.
					string projOutDir = GetProjectPath(outDir);
					if (!Path.IsPathRooted(projOutDir))
						projOutDir = "$(ProjectDir)\\" + projOutDir;
					writer.WriteElementString("OutDir", projOutDir.EnsureTrailingSlash(defaultPath: "."));
				}
				else
				{
					writer.WriteElementString("OutDir", GetProjectPath(outDir).EnsureTrailingSlash(defaultPath: "."));
				}

				string vstmpDir = module.GetVSTmpFolder().Path;
				writer.WriteElementString("IntDir", GetProjectPath(vstmpDir).EnsureTrailingSlash(defaultPath: "."));

				string tlogLoc = module.GetVSTLogLocation().Path;
				writer.WriteElementString("TLogLocation", GetProjectPath(tlogLoc).EnsureTrailingSlash(defaultPath: "."));

				// Stadia's msbuild VSI seems to be using PC's default MSBuild setup which test for linker's $(OutputFile) extension
				// against $(TargetExt).  On PC, we have .exe extension which is fine.  But Stadia build output do not have
				// file extension for program output.  Stadia's VSI/MSBuild should have written their own custom code.  For now,
				// we hack around that by force spliting the output name if a period is found.
				if (module.Configuration.System == "stadia" && link != null && !String.IsNullOrEmpty(link.OutputName) && 
					targetExt.IsNullOrEmpty() && !Path.GetExtension(targetName).IsNullOrEmpty())
				{
					writer.WriteNonEmptyElementString("TargetName", Path.GetFileNameWithoutExtension(targetName));
					writer.WriteNonEmptyElementString("TargetExt", Path.GetExtension(targetName));
				}
				else
				{
					writer.WriteNonEmptyElementString("TargetName", targetName);

					if (targetExt != null && !useVsiTargetExt)
					{
						writer.WriteNonEmptyElementString("TargetExt", targetExt);
					}
				}

				if (link != null)
				{
					if (nameToXMLValue == null)
					{
						nameToXMLValue = new SortedDictionary<string, string>();

						ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).Link, nameToXMLValue, link.Options, "general", false);
					}

					string value;
					if (nameToXMLValue.TryGetValue("LinkIncremental", out value))
					{
						writer.WriteElementString("LinkIncremental", value);
					}

					if (nameToXMLValue.TryGetValue("GenerateManifest", out value))
					{
						writer.WriteElementString("GenerateManifest", value);
					}

					if (module.MiscFlags.TryGetValue("embedmanifest", out value))
					{
						writer.WriteElementString("EmbedManifest", value);
					}
					if (nameToXMLValue.TryGetValue("LinkKeyFile", out value))
					{
						writer.WriteElementString("LinkKeyFile", GetProjectPath(value));
					}
					if (nameToXMLValue.TryGetValue("DelaySign", out value))
					{
						writer.WriteElementString("LinkDelaySign", value);
					}
				}

				AddCustomVsOptions(writer, module);

				ExecuteExtensions(module, extension => extension.WriteExtensionToolProperties(writer));

				if (referencedAssemblyDirs != null)
				{
					// We need to make sure output directory is not listed in the reference path.  Otherwise, we run into situation
					// where Visual Studio accidentally treat copied local assemblies as input file instead of output file.
					PathString moduleOutputDir = PathString.MakeNormalized(Path.GetDirectoryName(module.PrimaryOutput()));
					List<PathString> refDirs = referencedAssemblyDirs.ToList();
					refDirs.Remove(moduleOutputDir);
					vcppDirectories.ReferenceDirectories.AddRange(refDirs);
				}

				if (this.SolutionGenerator.SndbsEnabled)
				{
					TaskUtil.Dependent(module.Package.Project, "SNDBS_Customizations");
					string snDbsOptionsString = module.Package.Project.GetPropertyOrDefault("eaconfig.sndbs-global-options", String.Empty);
					IEnumerable<string> rewriteRulesFiles = module.SnDbsRewriteRulesFiles.FileItems.Select(item => GetProjectPath(item.FullPath).Quote());
					if (rewriteRulesFiles.Any())
					{
						snDbsOptionsString += $" --include-rewrite-rules {String.Join(" --include-rewrite-rules ", rewriteRulesFiles)}";
					}
					string vsversion = module.Package.Project.Properties.GetPropertyOrDefault("vsversion." + module.Package.Project.Properties["config-system"], module.Package.Project.Properties.GetPropertyOrDefault("vsversion", "2019"));
					string toolTemplateDirectory = module.Package.Project.Properties.GetPropertyOrDefault("package.SNDBS_customizations.toolTemplateDirectory_" + vsversion, null);
					if (string.IsNullOrEmpty(toolTemplateDirectory) == false && (module.Configuration.System != "ps5" && module.Configuration.System != "kettle"))
					{
						snDbsOptionsString += " -templates " + toolTemplateDirectory;
					}
					writer.WriteNonEmptyElementString("SnDbsOptions", snDbsOptionsString);
				}

				// Add default values
				if (vcppDirectories.InheritExecutablePath && 
					!vcppDirectories.ExecutableDirectories.Any(p => p.Path == "$(ExecutablePath)" || p.Path.Contains("$(ExecutablePath);")))
					vcppDirectories.ExecutableDirectories.Add(new PathString("$(ExecutablePath)"));
				if (vcppDirectories.InheritIncludePath &&
					!vcppDirectories.IncludeDirectories.Any(p => p.Path == "$(IncludePath)" || p.Path.Contains("$(IncludePath);")))
					vcppDirectories.IncludeDirectories.Add(new PathString("$(IncludePath)"));
				if (vcppDirectories.InheritReferencePath &&
					!vcppDirectories.ReferenceDirectories.Any(p => p.Path == "$(ReferencePath)" || p.Path.Contains("$(ReferencePath);")))
					vcppDirectories.ReferenceDirectories.Add(new PathString("$(ReferencePath)"));
				if (vcppDirectories.InheritLibraryPath &&
					!vcppDirectories.LibraryDirectories.Any(p => p.Path == "$(LibraryPath)" || p.Path.Contains("$(LibraryPath);")))
					vcppDirectories.LibraryDirectories.Add(new PathString("$(LibraryPath)"));
				if (vcppDirectories.InheritLibraryWPath &&
					!vcppDirectories.LibraryWinRTDirectories.Any(p => p.Path == "$(LibraryWPath)" || p.Path.Contains("$(LibraryWPath);")))
					vcppDirectories.LibraryWinRTDirectories.Add(new PathString("$(LibraryWPath)"));
				if (vcppDirectories.InheritSourcePath &&
					!vcppDirectories.SourceDirectories.Any(p => p.Path == "$(SourcePath)" || p.Path.Contains("$(SourcePath);")))
					vcppDirectories.SourceDirectories.Add(new PathString("$(SourcePath)"));
				if (vcppDirectories.InheritExcludePath &&
					!vcppDirectories.ExcludeDirectories.Any(p => p.Path == "$(ExcludePath)" || p.Path.Contains("$(ExcludePath);")))
					vcppDirectories.ExcludeDirectories.Add(new PathString("$(ExcludePath)"));
				if (vcppDirectories.InheritNativeExecutablePath &&
					!vcppDirectories.NativeExecutableDirectories.Any(p => p.Path == "$(NativeExecutablePath)" || p.Path.Contains("$(NativeExecutablePath);")))
					vcppDirectories.NativeExecutableDirectories.Add(new PathString("$(NativeExecutablePath)"));

				writer.WriteNonEmptyElementString("ExecutablePath", vcppDirectories.ExecutableDirectories.ToString(";", p => p.Path));
				writer.WriteNonEmptyElementString("IncludePath", vcppDirectories.IncludeDirectories.ToString(";", p => p.Path));
				writer.WriteNonEmptyElementString("ReferencePath", vcppDirectories.ReferenceDirectories.ToString(";", p => p.Path));
				writer.WriteNonEmptyElementString("LibraryPath", vcppDirectories.LibraryDirectories.ToString(";", p => p.Path));
				writer.WriteNonEmptyElementString("LibraryWPath", vcppDirectories.LibraryWinRTDirectories.ToString(";", p => p.Path));
				writer.WriteNonEmptyElementString("SourcePath", vcppDirectories.SourceDirectories.ToString(";", p => p.Path));
				writer.WriteNonEmptyElementString("ExcludePath", vcppDirectories.ExcludeDirectories.ToString(";", p => p.Path));
				writer.WriteNonEmptyElementString("NativeExecutablePath", vcppDirectories.NativeExecutableDirectories.ToString(";", p => p.Path));
			}

			foreach (var step in module.BuildSteps)
			{
				if (step.Name == "custombuild")
				{
					string before_target = step.Before;
					
					// for visual studio builds we want pre-build to happen before the ClCompile step.
					if (before_target.ToLowerInvariant() == "build")
					{
						before_target = "ClCompile";
					}

					writer.WriteNonEmptyElementString("CustomBuildBeforeTargets", before_target);
					writer.WriteNonEmptyElementString("CustomBuildAfterTargets", step.After);
					break;
				}
			}

			writer.WriteEndElement();	// Closing <PropertyGroup> element
		}

		protected virtual IDictionary<string, IList<Tuple<ProjectRefEntry, uint>>> GroupProjectReferencesByConfigConditions(IEnumerable<ProjectRefEntry> projectReferences)
		{
			var groups = new Dictionary<string, IList<Tuple<ProjectRefEntry, uint>>>();

			foreach (ProjectRefEntry projRef in projectReferences)
			{
				foreach (IGrouping<uint, ProjectRefEntry.ConfigProjectRefEntry> group in projRef.ConfigEntries.GroupBy(pr => pr.Type))
				{
					if (group.Count() > 0)
					{
						string condition = GetConfigCondition(group.Select(ce => ce.Configuration));
						if (!groups.TryGetValue(condition, out IList<Tuple<ProjectRefEntry, uint>> configEntries))
						{
							configEntries = new List<Tuple<ProjectRefEntry, uint>>();
							groups.Add(condition, configEntries);
						}
						configEntries.Add(Tuple.Create(projRef, group.First().Type));
					}
				}
			}
			return groups;
		}

		protected virtual void GetProjectReferenceUnreferencedConditions(
			IEnumerable<Configuration> allConfigs,
			IEnumerable<ProjectRefEntry.ConfigProjectRefEntry> projRefConfigEntries,
			out List<Configuration> unaccountedConfigs)
		{
			unaccountedConfigs = allConfigs.ToHashSet().Except(projRefConfigEntries.Select(ce => ce.Configuration)).ToList();
		}

		protected virtual void GetProjectReferenceConditionsForType(
			IEnumerable<Configuration> allConfigs, 
			IEnumerable<ProjectRefEntry.ConfigProjectRefEntry> projRefConfigEntries, 
			uint requiredType,
			out List<Configuration> trueConfigs, 
			out List<Configuration> falseConfigs, 
			out List<Configuration> unaccountedConfigs)
		{
			trueConfigs = projRefConfigEntries.Where(ce => ce.IsKindOf(requiredType)).Select(ce => ce.Configuration).ToList();
			falseConfigs = projRefConfigEntries.Where(ce => ce.IsKindOf(requiredType) == false).Select(ce => ce.Configuration).ToList();
			unaccountedConfigs = allConfigs.ToHashSet().Except(trueConfigs.AsEnumerable()).Except(falseConfigs.AsEnumerable()).ToList();
		}

		protected virtual void WriteReferences(IXmlWriter writer, IEnumerable<ProjectRefEntry> projectReferences, IDictionary<PathString, FileEntry> references, IDictionary<PathString, FileEntry> comreferences, IDictionary<Configuration, ISet<PathString>> referencedAssemblyDirs)
		{
			if (projectReferences.Count() > 0)
			{
				// Visual Studio no longer support having project references with config conditions.  All platform's project references need to be listed
				// even for incompatible config projects.  It is up to the .sln file itself to set up the project not to build and we setup LinkLibraryDependencies, etc
				// using the config conditions.

				StartElement(writer, "ItemGroup", label: "ProjectReferences");

				foreach (ProjectRefEntry projRef in projectReferences)
				{
					writer.WriteStartElement("ProjectReference");
					writer.WriteAttributeString("Include", GetProjectPath(Path.Combine(projRef.ProjectRef.OutputDir.Path, projRef.ProjectRef.ProjectFileName)));
					writer.WriteElementString("Project", projRef.ProjectRef.ProjectGuidString);

					writer.WriteElementString("Private", false); // disable VS copy local - we use own mechanism for copying

					// Unaccounted configs can show up when a module is only depended on for specific configs and not others.
					GetProjectReferenceUnreferencedConditions(AllConfigurations, projRef.ConfigEntries, out List<Configuration> unaccountedConfigs);
					if (unaccountedConfigs.Any())
					{
						// mainly for android, don't try and package the library if it is not enabled for this config,
						// can't use LinkLibraryDependencies here for cases where we want to build, package, but not link
						string notDependedOnCondition = GetConfigCondition(unaccountedConfigs);
						writer.WriteStartElement("PackageLibrary");
						writer.WriteAttributeString("Condition", notDependedOnCondition);
						writer.WriteString("False");
						writer.WriteEndElement();
					}		

					// Group unaccounted configs to false configs for flags (we don't link if it's not depended on!)
					GetProjectReferenceConditionsForType(AllConfigurations, projRef.ConfigEntries, ProjectRefEntry.LinkLibraryDependencies, out List<Configuration> trueConfigs, out List<Configuration> falseConfigs, out unaccountedConfigs);
					falseConfigs.AddRange(unaccountedConfigs);
					if (trueConfigs.Count() == 0 || falseConfigs.Count() == 0)
					{
						// If all conditions has identical value, we just need to write one.
						writer.WriteElementString("LinkLibraryDependencies", (falseConfigs.Count() == 0).ToString());
					}
					else
					{
						string trueCondition = GetConfigCondition(trueConfigs);
						writer.WriteStartElement("LinkLibraryDependencies");
						writer.WriteAttributeString("Condition", trueCondition);
						writer.WriteString("True");
						writer.WriteEndElement();
						string falseCondition = GetConfigCondition(falseConfigs);
						writer.WriteStartElement("LinkLibraryDependencies");
						writer.WriteAttributeString("Condition", falseCondition);
						writer.WriteString("False");
						writer.WriteEndElement();
					}

					GetProjectReferenceConditionsForType(AllConfigurations, projRef.ConfigEntries, ProjectRefEntry.UseLibraryDependencyInputs, out trueConfigs, out falseConfigs, out unaccountedConfigs);
					falseConfigs.AddRange(unaccountedConfigs);
					if (trueConfigs.Count() == 0 || falseConfigs.Count() == 0)
					{
						// If all conditions has identical value, we just need to write one.
						writer.WriteElementString("UseLibraryDependencyInputs", (falseConfigs.Count() == 0).ToString());
					}
					else
					{
						string trueCondition = GetConfigCondition(trueConfigs);
						writer.WriteStartElement("UseLibraryDependencyInputs");
						writer.WriteAttributeString("Condition", trueCondition);
						writer.WriteString("True");
						writer.WriteEndElement();
						string falseCondition = GetConfigCondition(falseConfigs);
						writer.WriteStartElement("UseLibraryDependencyInputs");
						writer.WriteAttributeString("Condition", falseCondition);
						writer.WriteString("False");
						writer.WriteEndElement();
					}

					GetProjectReferenceConditionsForType(AllConfigurations, projRef.ConfigEntries, ProjectRefEntry.ReferenceOutputAssembly, out trueConfigs, out falseConfigs, out unaccountedConfigs);
					falseConfigs.AddRange(unaccountedConfigs);
					if (trueConfigs.Count() == 0 || falseConfigs.Count() == 0)
					{
						writer.WriteElementString("ReferenceOutputAssembly", (falseConfigs.Count() == 0).ToString());
					}
					else
					{
						// If there are config conditions.  We need to write out both true and false condition of it be switch.
						string trueCondition = GetConfigCondition(trueConfigs);
						writer.WriteStartElement("ReferenceOutputAssembly");
						writer.WriteAttributeString("Condition", trueCondition);
						writer.WriteString("True");
						writer.WriteEndElement();
						string falseCondition = GetConfigCondition(falseConfigs);
						writer.WriteStartElement("ReferenceOutputAssembly");
						writer.WriteAttributeString("Condition", falseCondition);
						writer.WriteString("False");
						writer.WriteEndElement();
					}
					writer.WriteEndElement(); // ProjectReference
				}
				writer.WriteEndElement(); // ItemGroup (ProjectReferences)
			}

			if (references.Count() > 0)
			{
				StartElement(writer, "ItemGroup");

				foreach (FileEntry assemblyRef in references.Values)
				{
					WriteAssemblyReference(writer, assemblyRef);
				}
				writer.WriteFullEndElement(); //ItemGroup (ProjectReferences)
			}

			WriteComReferences(writer, comreferences.Values);
		}

		private void WriteAssemblyReference(IXmlWriter writer, FileEntry assemblyRef)
		{
			StartElement(writer, "Reference", GetConfigCondition(assemblyRef.ConfigEntries.Select(ce => ce.Configuration)));

			string includePath = Path.GetFileName(assemblyRef.Path.Path);
			bool isWinMdRef = ".winmd".Equals(Path.GetExtension(assemblyRef.Path.Path), StringComparison.OrdinalIgnoreCase);
			includePath = isWinMdRef ? includePath.Substring(0, includePath.Length - ".winmd".Length) : includePath;

			FileEntry.ConfigFileEntry firstConfigFileEntry = assemblyRef.ConfigEntries.First();
			OptionSet options = null;
			if (firstConfigFileEntry.FileItem.OptionSetName != null)
			{
				options = firstConfigFileEntry.Project.NamedOptionSets[firstConfigFileEntry.FileItem.OptionSetName];
			}

			if (options == null)
			{
				writer.WriteAttributeString("Include", includePath);

				// reference entries are created based on extension less file name, but actual path
				// can differ for each config - calculate hint path for each configuration
				Dictionary<Configuration, string> hintPathByConfig = assemblyRef.ConfigEntries.ToDictionary
				(
					ce => ce.Configuration,
					ce => ce.FileItem.AsIs ? ce.FileItem.Path.Path : GetProjectPath(ce.FileItem.Path)
				);

				// reverse map each unique hint paths to all the configurations that have that path
				Dictionary<string, IEnumerable<Configuration>> hintsPathsToConfigs = hintPathByConfig.GroupBy(kvp => kvp.Value).ToDictionary(group => group.Key, group => group.Select(kvp => kvp.Key));

				foreach (KeyValuePair<string, IEnumerable<Configuration>> hintToConfigs in hintsPathsToConfigs)
				{
					WriteElementStringWithConfigCondition(writer, "HintPath", hintToConfigs.Key, GetConfigCondition(hintToConfigs.Value));
				}

				if (isWinMdRef)
				{
					writer.WriteElementString("IsWinMDFile", isWinMdRef);
				}
				writer.WriteElementString("Private", false); 
			}
			else
			{
				Dictionary<string, string> assemblyAttributes = options.GetOptionDictionary("visual-studio-assembly-attributes");
				if (!assemblyAttributes.ContainsKey("Include"))
				{
					assemblyAttributes.Add("Include", includePath);
				}

				foreach (var prop in assemblyAttributes)
				{
					writer.WriteAttributeString(prop.Key, prop.Value);
				}

				Dictionary<string, string> assemblyProperties = options.GetOptionDictionary("visual-studio-assembly-properties");

				// reference entries are created based on extension less file name, but actual path
				// can differ for each config - calculate hint path for each configuration
				Dictionary<Configuration, string> hintPathByConfig = assemblyRef.ConfigEntries.ToDictionary
				(
					ce => ce.Configuration,
					ce => ce.FileItem.AsIs ? ce.FileItem.Path.Path : GetProjectPath(ce.FileItem.Path)
				);

				// reverse map each unique hint paths to all the configurations that have that path
				Dictionary<string, IEnumerable<Configuration>> hintsPathsToConfigs = hintPathByConfig.GroupBy(kvp => kvp.Value).ToDictionary(group => group.Key, group => group.Select(kvp => kvp.Key));

				foreach (KeyValuePair<string, IEnumerable<Configuration>> hintToConfigs in hintsPathsToConfigs)
				{
					WriteElementStringWithConfigCondition(writer, "HintPath", hintToConfigs.Key, GetConfigCondition(hintToConfigs.Value));
				}

				if (!assemblyProperties.ContainsKey("IsWinMDFile") && isWinMdRef)
				{
					assemblyProperties.Add("IsWinMDFile", isWinMdRef);
				}
				if (!assemblyProperties.ContainsKey("Private")) 
				{
					assemblyProperties.Add("Private", false.ToString());
				}

				foreach (var prop in assemblyProperties)
				{
					writer.WriteElementString(prop.Key, prop.Value);
				}
			}
			writer.WriteEndElement();
		}

		protected override void WriteComReferences(IXmlWriter writer, ICollection<FileEntry> comreferences)
		{
			if (comreferences.Count > 0)
			{
				foreach (var group in comreferences.GroupBy(f => f.ConfigSignature, f => f))
				{
					if (group.Count() > 0)
					{
						using (writer.ScopedElement("ItemGroup"))
						{
							writer.WriteAttributeString("Condition", GetConfigCondition(group));

							foreach (var comRef in group)
							{
								if (GenerateInteropAssembly(comRef.Path, out Guid libGuid, out var typeLibAttr))
								{
									// If file is declared asis, keep it as a file name. No relative path.
									// In VS2008 we can't put conditions by configuration. If any config declares file asIs, then we use it asis;
									bool isFileAsIs = null != comRef.ConfigEntries.Find(ce => ce.FileItem.AsIs);
									var refpath = isFileAsIs ? comRef.Path.Path : GetProjectPath(comRef.Path.Path, addDot: true);

									writer.WriteStartElement("COMReference");
									writer.WriteAttributeString("Include", refpath);
									writer.WriteElementString("Guid", libGuid.ToString());
									writer.WriteElementString("VersionMajor", typeLibAttr.wMajorVerNum.ToString());
									writer.WriteElementString("VersionMinor", typeLibAttr.wMinorVerNum.ToString());
									writer.WriteElementString("Lcid", typeLibAttr.lcid.ToString());
									writer.WriteElementString("WrapperTool", "tlbimp");
									writer.WriteElementString("Isolated", "false");
									writer.WriteElementString("Private", true);
									writer.WriteElementString("ReferenceOutputAssembly", false);
									writer.WriteElementString("CopyLocalSatelliteAssemblies", true);
									writer.WriteEndElement(); //COMReference
								}
							}
						}
					}
				}
			}
		}

		protected override void WriteFilesWithFilters(IXmlWriter writer, FileFilters filters)
		{
			// tuple meaning: 
			// string - toolName, ie ClCompile, None, CustomBuild, etc
			// PathString - path to file
			// string - name of filter
			// bool - force project path, hack to deal with CustomBuild files
			List<Tuple<string, PathString, string, bool>> toolFileFilter = new List<Tuple<string, PathString, string, bool>>();

			using (writer.ScopedElement("ItemGroup"))
			{
				filters.ForEach(entry =>
				{
					foreach (FileEntry file in entry.Files)
					{
						if (Path.GetExtension(file.Path.Path).ToLowerInvariant() != ".resx")
						{
							WriteFile(writer, file, out string fileToolName);

							toolFileFilter.Add(Tuple.Create<string, PathString, string, bool>(fileToolName, file.Path, entry.FilterPath, false));

							if (fileToolName == "ClInclude" && WriteResXFile(writer, file, out fileToolName, out PathString resXPath))
							{
								toolFileFilter.Add(Tuple.Create<string, PathString, string, bool>(fileToolName, resXPath, entry.FilterPath, false));
							}
						}
					}
				});
			}

			// add copylocal files to filter list
			// TODO cpp filter for copylocal
			// -dvaliant 2016/05/25
			/* this is very dependent on VSProjectBase.cs WriteCopyLocalFiles logic - ideally the code should enforce some coupling between the two */
			IEnumerable<string> copyLocalSourceFiles = Modules.SelectMany(module => module.CopyLocalFiles)
				.Where(copyLocalFile => !copyLocalFile.PostBuildCopyLocal) // ignore post builds, we only make regular copies visible in solution explorer
				.SelectMany(copyLocalFile => new string[] { copyLocalFile.AbsoluteSourcePath, copyLocalFile.AbsoluteDestPath }) // select both source and dest path - we need both to be filtered
				.Distinct(); // VS will not like it if we have multiple entries per file
			foreach (string copyLocalSource in copyLocalSourceFiles)
			{
				toolFileFilter.Add(Tuple.Create<string, PathString, string, bool>("CustomBuild", PathString.MakeNormalized(copyLocalSource), "CopyLocalFiles", true));
			}

			WriteFilters(writer.FileName, filters, toolFileFilter);
		}

		protected virtual void WriteFile(IXmlWriter writer, FileEntry fileEntry, out string outFileToolName)
		{
			// Tool name must be common for all configurations.
			var firstConfigWithTool = fileEntry.ConfigEntries.FirstOrDefault(x => x.Tool != null || x.FileItem.GetTool() != null);

			var fileToolName = outFileToolName = GetToolMapping(firstConfigWithTool ?? fileEntry.ConfigEntries.FirstOrDefault(), out OptionSet toolproperties);
			if (fileToolName == "Masm" && fileEntry.ConfigEntries.Any(e => e.Configuration.Compiler != "vc"))
			{
				fileToolName = "ClCompile";
			}

			using (writer.ScopedElement(fileToolName))
			{
				// Because of bug in visual studio 2010 we need to force relative path, otherwise custom build does not work
				// and property pages for individual ClCompile files do not show up in GUI
				// See https://connect.microsoft.com/VisualStudio/feedback/details/748640 for details

				if (fileToolName == "ClCompile" && Version == "10.00")
				{
					// Use full path when (base + relative) > 260. We loose property pages but at least it builds.
					writer.WriteAttributeString("Include", PathUtil.SafeRelativePath_OrFullPath(fileEntry.Path, OutputDir));
				}
				else if (fileToolName == "CustomBuild")
				{
					// Force relative path regardless the length, otherwise custom build step just won't work
					writer.WriteAttributeString("Include", PathUtil.RelativePath(fileEntry.Path, OutputDir));
				}
				// java files *must* be linked or MSVS just can't
				else if (fileToolName == "JavaCompile")
				{
					writer.WriteAttributeString("Include", GetProjectPath(fileEntry.Path.Path));

					// the startup module should be our packaging project, which is classified as native
					if (StartupModule is Module_Native nativeModule)
					{
						using (writer.ScopedElement("Link"))
						{
							var fileDir = fileEntry.Path.Path;
							var linkPath = PathUtil.RelativePath(fileDir, nativeModule.JavaSourceFiles.BaseDirectory);
							writer.WriteString(linkPath);
						}
					}
				}
				else
				{
					writer.WriteAttributeString("Include", GetProjectPath(fileEntry.Path.Path));
				}

				string configCondition = null;
				if (!IsExcludedFromBuild_Respected(toolproperties))
				{
					configCondition = GetConfigCondition(fileEntry.ConfigEntries.Select(ce => ce.Configuration));
					writer.WriteNonEmptyAttributeString("Condition", configCondition);
				}

				var active = new HashSet<string>();
				foreach (var configentry in fileEntry.ConfigEntries)
				{
					var tool = configentry.FileItem.GetTool();

					bool usingVSUnitySetup = tool != null && tool is CcCompiler && IsConfigEnabledForVSUnityFlag(configentry.Configuration);

					if (fileToolName == "None" && configentry.FileItem.OptionSetName == "deploy")
					{
						WriteElementStringWithConfigCondition(writer, "DeploymentContent", "TRUE", GetConfigCondition(configentry.Configuration));
					}
					if (configentry.IsKindOf(FileEntry.ExcludedFromBuild))
					{
						if (!usingVSUnitySetup)
						{
							using (writer.ScopedElement("ExcludedFromBuild"))
							{
								writer.WriteAttributeString("Condition", GetConfigCondition(configentry.Configuration));
								writer.WriteString("TRUE");
							}
						}

						if (tool != null && tool.Description == "#global_context_settings")
						{
							tool = null;
						}
					}

					var fileConfigItemToolName = GetToolMapping(configentry, out OptionSet filetoolproperties);
					if (fileConfigItemToolName != fileToolName)
					{
						// Error. VC can only handle same tool names.
					}
					var currentModule = Modules.SingleOrDefault(m => m.Configuration == configentry.Configuration) as Module;
					WriteOneTool(writer, fileToolName, tool, currentModule, null, configentry.FileItem, filetoolproperties);

					if (IsVisualStudioAndroid(currentModule)
						&& filetoolproperties != null
						&& filetoolproperties.Options["create-pch"] == "on")
					{
						/* On android, the pch is automatically added to ClCompile; see FixupCLCompileOptions in Microsoft.Cpp.Clang.targets.
						 * We removed it from the build here to avoid duplicate entries into ClCompile which breaks MultiTool */
						WriteElementStringWithConfigCondition(writer, "ExcludedFromBuild", "TRUE", GetConfigCondition(configentry.Configuration));
					}
					else if (IsTegraAndroid(currentModule)
						&& filetoolproperties != null
						&& filetoolproperties.Options["create-pch"] == "on"
						&& currentModule is Module_Native nativeModule)
					{
						// tegra-android ignores the PrecompiledHeaderOutputFile setting so we explicitly change the output file to be what the module has specified
						writer.WriteElementString("PrecompiledHeaderFile", Path.GetFileNameWithoutExtension(nativeModule.PrecompiledFile.Path));
					}


					active.Add(configentry.Configuration.Name);
				}

				// Write excluded from build:
				foreach (var config in AllConfigurations.Where(c => !active.Contains(c.Name)))
				{
					WriteElementStringWithConfigCondition(writer, "ExcludedFromBuild", "TRUE", GetConfigCondition(config));
				}
			}
		}

		// This method is for handling WinForm applications Form headers and their associated resX files
		// It's assumed that formx.h and its resX file be in the same directory.

		protected virtual bool WriteResXFile(IXmlWriter writer, FileEntry fileEntry, out string fileToolName, out PathString filePath)
		{
			fileToolName = "EmbeddedResource";
			var resXfilepath = Path.ChangeExtension(fileEntry.Path.Path, ".resX");
			if (File.Exists(resXfilepath))
			{
				filePath = new PathString(resXfilepath, PathString.PathState.File | fileEntry.Path.State);
				writer.WriteStartElement(fileToolName);
				writer.WriteAttributeString("Include", GetProjectPath(resXfilepath));

				foreach (var configentry in fileEntry.ConfigEntries)
				{
					if (configentry.IsKindOf(FileEntry.ExcludedFromBuild))
					{
						WriteElementStringWithConfigCondition(writer, "ExcludedFromBuild", "TRUE", GetConfigCondition(configentry.Configuration));
					}
					else
					{
						WriteElementStringWithConfigCondition(writer, "DependentUpon", Path.GetFileName(fileEntry.Path.Path), GetConfigCondition(configentry.Configuration));
						WriteElementStringWithConfigCondition(writer, "LogicalName", Name + "." + Path.GetFileNameWithoutExtension(fileEntry.Path.Path) + @".resources", GetConfigCondition(configentry.Configuration));
					}
				}
				writer.WriteEndElement();
				return true;
			}
			filePath = null;
			return false;
		}

		protected virtual void WriteFilters(string baseFileName, FileFilters filters, List<Tuple<string, PathString, string, bool>> toolFileFilter)
		{
			using (IXmlWriter writer = new XmlWriter(FilterFileWriterFormat))
			{
				writer.FileName = baseFileName + ".filters";
				writer.CacheFlushed += new CachedWriterEventHandler(OnProjectFileUpdate);

				using (writer.ScopedElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003"))
				{
					writer.WriteAttributeString("ToolsVersion", ToolsVersion);

					// filter definitions
					using (writer.ScopedElement("ItemGroup"))
					{
						var nonEmptyFilters = filters
							.FilterEntries
							.Where(x => !string.IsNullOrEmpty(x.FilterPath));

						foreach (var filter in nonEmptyFilters)
						{
							using (writer.ScopedElement("Filter"))
							{
								writer.WriteAttributeString("Include", filter.FilterPath);
								writer.WriteElementString("UniqueIdentifier", VSSolutionBase.ToString(Hash.MakeGUIDfromString(filter.FilterPath)));
							}
						}
					}

					var toolGroupsWithAny = toolFileFilter
						.Where(e => e.Item3 != null)
						.GroupBy(e => e.Item1)
						.Where(e => e.Any());

					foreach (var toolGroup in toolGroupsWithAny)
					{
						// filter files
						using (writer.ScopedElement("ItemGroup"))
						{
							IEnumerable<Tuple<string, string>> pathAndFilter = toolGroup.Select(groupEntry => Tuple.Create(GetFilterPath(toolGroup.Key, groupEntry.Item2.Path, groupEntry.Item4), groupEntry.Item3));
							foreach (Tuple<string, string> item in pathAndFilter.OrderBy(paf => paf.Item1))
							{
								using (writer.ScopedElement(toolGroup.Key))
								{
									writer.WriteAttributeString("Include", item.Item1);
									if (!string.IsNullOrEmpty(item.Item2))
										writer.WriteElementString("Filter", item.Item2);
								}
							}
						}
					}
				}
			}
		}


		// this needs to mirror WriteFile code for filters to match however for copy local mechanism 
		// we actually use CustomBuild differently so allow force option for that case
		private string GetFilterPath(string toolType, string path, bool forceProjectPath)
		{
			if (!forceProjectPath && toolType == "ClCompile" && Version == "10.00")
			{
				// because of a bug in visual studio 2010, path must be relative. See https://connect.microsoft.com/VisualStudio/feedback/details/748640 for details
				return PathUtil.RelativePath(path, OutputDir.Path);
			}
			else if (!forceProjectPath && toolType == "CustomBuild")
			{
				// because of a bug in visual studio 2010, path must be relative. See https://connect.microsoft.com/VisualStudio/feedback/details/748640 for details
				return PathUtil.RelativePath(path, OutputDir.Path);
			}
			else
			{
				return GetProjectPath(path);
			}
		}

		protected override void WriteUserFile()
		{

			string userFilePath = Path.Combine(OutputDir.Path, UserFileName);
			var userFileDoc = new XmlDocument();

			bool corrupt_userfile = false;

			try
			{
				if (File.Exists(userFilePath))
				{
					userFileDoc.Load(userFilePath);
				}
			}
			catch (Exception ex)
			{
				Log.Warning.WriteLine("Failed to load existing '.user' file '{0}': {1}. Recreating user file.", UserFileName, ex.Message);
				userFileDoc = new XmlDocument();
				corrupt_userfile = true;
			}

			var userFileEl = userFileDoc.GetOrAddElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");

			userFileEl.SetAttribute("ProjectType", "Visual C++");
			userFileEl.SetAttribute("ToolsVersion", ToolsVersion);

			int userdata_count = 0;

			foreach (var module in Modules)
			{
				userdata_count += SetUserFileConfiguration(userFileEl, module as ProcessableModule);
			}
			if (userdata_count > 0)
			{
				using (MakeWriter writer = new MakeWriter())
				{
					writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnProjectFileUpdate);
					writer.FileName = userFilePath;
					string formatedXml = System.Xml.Linq.XElement.Parse(userFileDoc.OuterXml).ToString();
					writer.Write(formatedXml);
				}
			}
			else if (corrupt_userfile && File.Exists(userFilePath))
			{
				File.Delete(userFilePath);
			}
		}

		protected override int SetUserFileConfiguration(XmlNode rootEl, ProcessableModule module, Func<ProcessableModule, IEnumerable<KeyValuePair<string, string>>> getGeneratedUserData = null)
		{
			getGeneratedUserData = getGeneratedUserData ?? ((m) => GetUserDataMSBuild(m));

			int userdata_count = 0;

			string condition = GetConfigCondition(module.Configuration);

			XmlNode propertyGroupEl = null;

			XmlNamespaceManager nsmgr = new XmlNamespaceManager(rootEl.OwnerDocument.NameTable);
			nsmgr.AddNamespace("msb", rootEl.NamespaceURI);

			foreach (var entry in getGeneratedUserData(module))
			{
				var node = rootEl.SelectSingleNode(String.Format("msb:PropertyGroup[contains(@Condition, '{0}')]/msb:{1}", GetVSProjConfigurationName(module.Configuration), entry.Key), nsmgr);
				if (node == null)
				{
					node = rootEl.SelectSingleNode(String.Format("msb:PropertyGroup[contains(@Condition, '{0}')]", GetVSProjConfigurationName(module.Configuration)), nsmgr);

					if (node == null)
					{
						if (propertyGroupEl == null)
						{
							propertyGroupEl = rootEl.OwnerDocument.CreateElement("PropertyGroup", rootEl.NamespaceURI);
							rootEl.AppendChild(propertyGroupEl);
							propertyGroupEl.SetAttribute("Condition", condition);
						}
						node = propertyGroupEl;
					}
					node = node.GetOrAddElement(entry.Key);
				}

				if (BuildGenerator.IsPortable)
				{
					node.InnerText = BuildGenerator.PortableData.NormalizeIfPathString(entry.Value, OutputDir.Path, quote: false);
				}
				else
				{
					node.InnerText = entry.Value;
				}
				userdata_count++;
			}
			return userdata_count;
		}

		protected virtual IEnumerable<KeyValuePair<string, string>> GetUserDataMSBuild(ProcessableModule module)
		{
			var userdata = new Dictionary<string, string>();
			string defaultWorkingDirectory = "$(OutDir)";

			bool useVSUnixMakeProject = (module != null && ModuleShouldUseVSUnixMakeProject(module));

			if (module != null && useVSUnixMakeProject && module.AdditionalData.DebugData == null && module.IsKindOf(Module.Program))
			{
				// Currently, there are no module.AdditionalData.DebugData setup for Linux module other than the following flag.
				userdata.Add("DebuggerFlavor", "LinuxDebugger");
				userdata.Add("PreLaunchCommand", "chmod 777 $(RemoteDeployDir)/$(TargetName)");
				userdata.Add("RemoteDebuggerCommand", "$(RemoteDeployDir)/$(TargetName)");
				userdata.Add("RemoteDebuggerWorkingDirectory", "$(RemoteDeployDir)");
			}

			if (module != null && module.AdditionalData.DebugData != null)
			{
				var prefixes = new string[] { "Local" };
				var remotemachine = module.AdditionalData.DebugData.RemoteMachine;
				string commandargs = module.AdditionalData.DebugData.Command.Options.ToString(" ");

				var workingDirectory = string.IsNullOrEmpty(module.AdditionalData.DebugData.Command.WorkingDir.Path) ? defaultWorkingDirectory : module.AdditionalData.DebugData.Command.WorkingDir.Path;

				var debuggerFlavor = "Windows"+ prefixes[0] + "Debugger";

				if (IsWinRTProgram(module as Module_Native))
				{
					debuggerFlavor = "AppHost" + prefixes[0] + "Debugger";
				}
				else if (useVSUnixMakeProject)
				{
					debuggerFlavor = "LinuxDebugger";
				}

				if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.RemoteMachine))
				{
					prefixes = new string[] { "Local", "Remote" };
				}

				if (useVSUnixMakeProject && module.IsKindOf(Module.Program))
				{
					userdata.AddNonEmpty("DebuggerFlavor", debuggerFlavor);
					userdata.Add("PreLaunchCommand", "chmod 777 $(RemoteDeployDir)/$(TargetName)");
					userdata.Add("RemoteDebuggerCommand", "$(RemoteDeployDir)/$(TargetName)");
					if (!String.IsNullOrEmpty(workingDirectory) && UnixDebugOnWsl(module) && workingDirectory != defaultWorkingDirectory)
					{
						// We can only run PathToWsl if it is providing full path and not already a reference to other variable like the default $(OutDir)
						userdata.Add("RemoteDebuggerWorkingDirectory", PathUtil.PathToWsl(workingDirectory));
					}
					else
					{
						userdata.Add("RemoteDebuggerWorkingDirectory", "$(RemoteDeployDir)");
					}
				}
				else
				{
					foreach (var prefix in prefixes)
					{
						userdata.AddNonEmpty(prefix + "DebuggerCommand", module.AdditionalData.DebugData.Command.Executable.Path);

						userdata.AddNonEmpty(prefix + "DebuggerWorkingDirectory", workingDirectory);

						userdata.AddNonEmpty(prefix + "DebuggerCommandArguments", commandargs);

						if (prefix == "Remote")
						{
							userdata.AddNonEmpty(prefix + "DebuggerServerName", remotemachine);
						}
						else if (prefix == "Xbox360")
						{
							userdata.AddNonEmpty("RemoteMachine", remotemachine);
						}
					}
					userdata.AddNonEmpty("DebuggerFlavor", debuggerFlavor);
				}
			}
			else if (!useVSUnixMakeProject && module.IsKindOf(Module.Program))
			{
				userdata.AddNonEmpty("LocalDebuggerWorkingDirectory", defaultWorkingDirectory);
				userdata.AddNonEmpty("RemoteDebuggerWorkingDirectory", defaultWorkingDirectory);

			}

			ExecuteExtensions(module, extension => extension.UserData(userdata));

			return userdata;
		}
		#endregion Write methods

		#region Write Tools

		protected override void WriteOneCustomBuildRuleTool(IXmlWriter writer, string name, string toolOptionsName, IModule module)
		{
			if (!String.IsNullOrEmpty(name))
			{
				writer.WriteStartElement(name);
				OptionSet options;
				if (module.Package.Project.NamedOptionSets.TryGetValue(toolOptionsName, out options))
				{
					foreach (var o in options.Options)
					{
						writer.WriteNonEmptyElementString(o.Key, o.Value);
					}
				}
				writer.WriteEndElement();
			}
		}

		protected override void WriteBuildEvents(IXmlWriter writer, string name, string toolname, uint type, Module module)
		{
			StringBuilder cmd = new StringBuilder();
			StringBuilder input = new StringBuilder();
			StringBuilder output = new StringBuilder();

			if (toolname == "custombuild")
			{
				// This is custom build step:
				foreach (var step in module.BuildSteps.Where(step => step.Name == toolname))
				{
					input.Append(step.InputDependencies.ToString(";", dep => GetProjectPath(dep)));
					output.Append(step.OutputDependencies.ToString(";", dep => GetProjectPath(dep)));

					foreach (var command in step.Commands)
					{
						cmd.AppendLine(GetCommandScriptWithCreateDirectories(command).TrimWhiteSpace());
					}

					if (step.Commands.Count == 0 && step.TargetCommands.Count > 0 )
					{
						foreach (var targetCommand in step.TargetCommands)
						{
							if (!targetCommand.NativeNantOnly)
							{
								Func<string, string> normalizeFunc = null;
								if (BuildGenerator.IsPortable)
								{
									normalizeFunc = (X) => BuildGenerator.PortableData.NormalizeIfPathString(X, OutputDir.Path);
								}
								cmd.AppendLine(NantInvocationProperties.TargetToNantCommand(module, targetCommand, addGlobal: true, normalizePathString: normalizeFunc, isPortable: BuildGenerator.IsPortable));
							}
						}
					}
				}
			}
			else
			{
				if ((type & BuildStep.PreBuild) == BuildStep.PreBuild)
				{
					cmd.AppendLine(GetCreateDirectoriesCommand(module.Tools));
				}

				foreach (BuildStep step in module.BuildSteps)
				{
					if (!step.IsKindOf(type) || step.Name == "custombuild")
						continue;

					foreach (var command in step.Commands)
					{
						cmd.AppendLine(GetCommandScriptWithCreateDirectories(command).TrimWhiteSpace());
					}

					if (step.Commands.Count == 0 && step.TargetCommands.Count > 0)
					{
						foreach (var targetCommand in step.TargetCommands)
						{
							if (!targetCommand.NativeNantOnly)
							{
								Func<string, string> normalizeFunc = null;
								if (BuildGenerator.IsPortable)
								{
									normalizeFunc = (X) => BuildGenerator.PortableData.NormalizeIfPathString(X, OutputDir.Path);
								}
								cmd.AppendLine(NantInvocationProperties.TargetToNantCommand(module, targetCommand, addGlobal: true, normalizePathString: normalizeFunc, isPortable : BuildGenerator.IsPortable));
							}
						}
					}
				}
			}

			if (cmd.Length > 0)
			{
				writer.WriteStartElement(name);
				writer.WriteNonEmptyElementString("Command", cmd.ToString().TrimWhiteSpace());
				writer.WriteNonEmptyElementString("Inputs", input.ToString().TrimWhiteSpace());
				writer.WriteNonEmptyElementString("Outputs", output.ToString().TrimWhiteSpace());
				writer.WriteEndElement();
			}
		}



		protected override void WriteMakeTool(IXmlWriter writer, string name, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (module.IsKindOf(Module.MakeStyle))
			{
				StartPropertyGroup(writer, GetConfigCondition(config));

				string build;
				string rebuild;
				string clean;
				string output;

				GetMakeToolCommands(module as Module, out build, out rebuild, out clean, out output);

				writer.WriteNonEmptyElementString("NMakeBuildCommandLine", build);
				writer.WriteNonEmptyElementString("NMakeReBuildCommandLine", rebuild);
				writer.WriteNonEmptyElementString("NMakeCleanCommandLine", clean);
				writer.WriteNonEmptyElementString("NMakeOutput", output);

				writer.WriteEndElement();
			}
		}

		protected void WriteForcedMakeTool(IXmlWriter writer, string name, Configuration config, Module module, IEnumerable<PathString> additionalSysInclude)
		{
			if (!module.IsKindOf(Module.ForceMakeStyleForVcproj))
			{
				return;
			}

			bool errorDetected = false;
			MacroMap makeCommandMacros = new MacroMap(module.Macros)
			{
				{ "makefile_dir", module.IntermediateDir.NormalizedPath },
				{ "makefile_name", $"{module.Name}.nant.mak" },
				{ "project.name", Name }
			};

			// get make exe
			string makeExeProperty = $"eaconfig.build.sln.{module.Configuration.System}.make.exe";
			string makeExe = module.Package.Project.GetPropertyValue(makeExeProperty);
			if (String.IsNullOrEmpty(makeExe))
			{
				Log.Error.WriteLine("Property {0} appears to be missing!", makeExeProperty);
				errorDetected = true;
			}

			// build command
			string build = null;
			{
				string makeBuildCmdProperty = $"eaconfig.build.sln.{module.Configuration.System}.make.buildcmd";
				string makeBuildCmd = module.Package.Project.GetPropertyValue(makeBuildCmdProperty);
				if (String.IsNullOrEmpty(makeBuildCmd))
				{
					Log.Error.WriteLine("Property {0} appears to be missing!", makeBuildCmdProperty);
					errorDetected = true;
				}
				else
				{
					makeBuildCmd = MacroMap.Replace(makeBuildCmd, makeCommandMacros, additionalErrorContext: $" from property {makeBuildCmdProperty}");
					build = $"\"{makeExe}\" {makeBuildCmd}";
				}
			}

			// rebuild command (optional)
			string rebuild = null;
			{
				string makeRebuildCmdProperty = String.Format("eaconfig.build.sln.{0}.make.rebuildcmd", module.Configuration.System);
				string makeRebuildCmd = module.Package.Project.GetPropertyValue(makeRebuildCmdProperty);

				if (!String.IsNullOrEmpty(makeRebuildCmd))
				{
					makeRebuildCmd = MacroMap.Replace(makeRebuildCmd, makeCommandMacros, additionalErrorContext: $" from property {makeRebuildCmdProperty}");
					rebuild = $"\"{makeExe}\" {makeRebuildCmd}";
				}
			}

			// clean command
			string clean = null;
			{
				string makeCleanCmdProperty = String.Format("eaconfig.build.sln.{0}.make.cleancmd", module.Configuration.System);
				string makeCleanCmd = module.Package.Project.GetPropertyValue(makeCleanCmdProperty);
				if (String.IsNullOrEmpty(makeCleanCmd))
				{
					Log.Error.WriteLine("ERROR: Property {0} appears to be missing!", makeCleanCmdProperty);
					errorDetected = true;
				}
				else
				{
					makeCleanCmd = MacroMap.Replace(makeCleanCmd, makeCommandMacros, additionalErrorContext: $" from property {makeCleanCmdProperty}");
					clean = $"\"{makeExe}\" {makeCleanCmd}";
				}
			}

			// output
			string output = null;
			string makeOutputProperty = String.Format("eaconfig.build.sln.{0}.make.output", module.Configuration.System);
			string makeOutput = module.Package.Project.GetPropertyValue(makeOutputProperty);
			if (String.IsNullOrEmpty(makeOutput))
			{
				Log.Error.WriteLine("ERROR: Property {0} appears to be missing!", makeOutputProperty);
				errorDetected = true;
			}
			else
			{
				output = MacroMap.Replace
				(
					makeOutput,
					new MacroMap()
					{
						{ "output", module.PrimaryOutput() },
						{ "module_vcproj_name", ProjectFileNameWithoutExtension },
						{ "vcproj_config", GetVSProjTargetConfiguration(module.Configuration) }
					},
					additionalErrorContext: $" from property {makeOutputProperty}"
				);
			}

			if (errorDetected)
			{
				throw new BuildException(String.Format("ERROR: Required properties to setup makestyle build for module {0} in config {1} appears to be missing!", module.Name, module.Configuration.Name));
			}

			StartPropertyGroup(writer, GetConfigCondition(config));
			if (ModuleShouldUseVSUnixMakeProject(module))
			{
				writer.WriteNonEmptyElementString("BuildCommandLine", build);
				writer.WriteNonEmptyElementString("ReBuildCommandLine", rebuild);
				writer.WriteNonEmptyElementString("CleanCommandLine", clean);

				// If there are no files to copy to remote unix machine, we need to set this to false or Visual Studio will
				// fail the build.  Should only set to true for local developers doing debugging.  Build farm or regular
				// user just doing build should always set this to false.
				writer.WriteElementString("LocalRemoteCopySources", "false");
			}
			else
			{
				writer.WriteNonEmptyElementString("NMakeBuildCommandLine", build);
				writer.WriteNonEmptyElementString("NMakeReBuildCommandLine", rebuild);
				writer.WriteNonEmptyElementString("NMakeCleanCommandLine", clean);
				writer.WriteNonEmptyElementString("NMakeOutput", output);
			}

			// Set module defines, include paths, and the compiler's system include paths so that Visual Studio's intellisense 
			// will still work under this unix cross build.  We get these info from the compiler tool's current setting.
			List<string> includeDirList = new List<string>();
			List<string> sysIncludeDirList = new List<string>(additionalSysInclude.Select(p => p.Path));
			List<string> libDirList = new List<string>();
			List<string> exePathList = new List<string>();

			CcCompiler cc = module.Tools.SingleOrDefault(t => t.ToolName == "cc") as CcCompiler;
			if (cc != null)
			{
				foreach(PathString includeDir in cc.IncludeDirs)
				{
					string includePath = includeDir.NormalizedPath.TrimQuotes();
					if (!includeDirList.Contains(includePath))
						includeDirList.Add(includePath);
				}

				foreach (string opt in cc.Options)
				{
					if (opt.StartsWith("-isystem"))
					{
						string path = new PathString(opt.Substring("-isystem".Length).Trim()).NormalizedPath.TrimQuotes();
						if (!sysIncludeDirList.Contains(path))
							sysIncludeDirList.Add(path);
					}
					else if (opt.StartsWith("-B"))
					{
						string path = new PathString(opt.Substring("-B".Length).Trim()).NormalizedPath.TrimQuotes();
						if (!exePathList.Contains(path))
							exePathList.Add(path);
					}
				}

				if (cc.SystemIncludeDirs.Any())
				{
					foreach (PathString sysIncDir in cc.SystemIncludeDirs)
					{
						string path = sysIncDir.NormalizedPath;
						if (!sysIncludeDirList.Contains(path))
							sysIncludeDirList.Add(path);
					}
				}
			}

			if (module.Tools.SingleOrDefault(t => t.ToolName == "link") is Linker linker)
			{
				foreach (string opt in linker.Options)
				{
					if (opt.StartsWith("-B"))
					{
						string path = new PathString(opt.Substring("-B".Length).Trim()).NormalizedPath.TrimQuotes();
						if (!exePathList.Contains(path))
							exePathList.Add(path);
					}
					else if (opt.StartsWith("-L"))
					{
						string path = new PathString(opt.Substring("-L".Length).Trim()).NormalizedPath.TrimQuotes();
						if (!libDirList.Contains(path))
							libDirList.Add(path);
					}
				}
			}

			Librarian lib = module.Tools.SingleOrDefault(t => t.ToolName == "lib") as Librarian;
			if (lib != null)
			{
				foreach (string opt in lib.Options)
				{
					if (opt.StartsWith("-B"))
					{
						string path = new PathString(opt.Substring("-B".Length).Trim()).NormalizedPath.TrimQuotes();
						if (!exePathList.Contains(path))
							exePathList.Add(path);
					}
				}
			}

			if (sysIncludeDirList.Count > 0)
			{
				if (ModuleShouldUseVSUnixMakeProject(module))
				{
					includeDirList.InsertRange(0, sysIncludeDirList);
				}
				else
				{
					StringBuilder includePaths = new StringBuilder();
					foreach (string path in sysIncludeDirList)
					{
						includePaths.Append(String.Format("{0};",path));
					}
					writer.WriteElementString("IncludePath", includePaths.ToString());
				}
			}
			if (libDirList.Count > 0)
			{
				StringBuilder libPaths = new StringBuilder();
				foreach (string path in libDirList)
				{
					libPaths.Append(String.Format("{0};",path));
				}
				writer.WriteElementString("LibraryPath", libPaths.ToString());
			}
			// Just in case, we're adding the cross-tool's bin folder.
			if (exePathList.Count > 0)
			{
				StringBuilder exePaths = new StringBuilder();
				foreach (string path in exePathList)
				{
					exePaths.Append(String.Format("{0};",path));
				}
				exePaths.Append("$(PATH)");
				writer.WriteElementString("ExecutablePath", exePaths.ToString());
			}

			// We're separating the module's include path from the system include paths with different element.
			if (includeDirList.Count > 0)
			{
				StringBuilder includePaths = new StringBuilder();
				foreach (string path in includeDirList)
				{
					includePaths.Append(String.Format("{0};", path));
				}
				writer.WriteElementString("NMakeIncludeSearchPath", includePaths.ToString());
			}

			// Set up the defines for intellisense to properly function. (We can only do this for native "CC" compiler's define.
			// If this is being used for C# project, we will ignore it because makestyle project is a vcxproj file and not a csproj
			// file.  And C# project's define doesn't behave the same way as C++ define.
			if (cc != null)
			{
				// TODO: ProDG VSI provide their own custom intellisense includes.  If we every need to force PS4 build under
				// make style project, we make need to design a generic mechanism to allow Framework to detect platform 
				// specific intellisense force include files.

				List<string> allDefineList = new List<string>();
				if (!config.Compiler.StartsWith("vc"))
				{
					// Provide a force include file to undefine VC compiler's windows define

					string filePath = Path.Combine((BuildGenerator.IsPortable) ? BuildGenerator.SlnFrameworkFilesRoot : PackageMap.Instance.GetFrameworkRelease().Path, "data");
					string undef_win_include = Path.GetFullPath(Path.Combine(filePath, "intellisense_undef_win.h"));
					if (!string.IsNullOrEmpty(writer.FileName) && BuildGenerator.IsPortable)
					{
						undef_win_include = PathUtil.RelativePath(undef_win_include, Path.GetDirectoryName(writer.FileName));
					}
					writer.WriteElementString("NMakeForcedIncludes", undef_win_include);
					// Then add proper compiler's internal defines.
					allDefineList.AddRange(cc.CompilerInternalDefines);
				}
				allDefineList.AddRange(GetAllDefines(cc, config));
				string allDefines = allDefineList.ToString(";");
				writer.WriteElementString("NMakePreprocessorDefinitions", allDefines);
			}

			writer.WriteEndElement();
		}

		protected override void WriteXMLDataGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{

		}

		protected override void WriteWebServiceProxyGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{

		}

		private Dictionary<string, string> FxCompileSwitchDefaults = new Dictionary<string, string>()
		{
			{ "DisableOptimizations", "FALSE" },
			{ "EnableDebuggingInformation", "FALSE" },
		};

		protected override void WriteFxCompileTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool is BuildTool fxcompile)
			{
				if (file == null)
					writer.WriteStartElement("FxCompile");

				// The profile option is tricky because it corresponds to two different options within visual studio (shader type and model)
				// to complicate matters, visual studio uses the shader model version to select whether to use fxc.exe or dxc.exe
				// and telling fxc.exe to use shader model > 6 will fail.
				// So this code essentially splits the argument into the two settings that visual studio needs.
				int profileIndex = 0;
				string shaderModel = null;
				string shaderType = null;
				bool profileMatched = false;
				foreach (string option in fxcompile.Options)
				{
					Match match = Regex.Match(option, "^[-/]T \"?([^\"]+)\"?$");
					if (match.Success)
					{
						string[] tokens = match.Groups[1].Value.Split(new char[] { '_' }, 2);
						shaderType = tokens[0];
						shaderModel = tokens[1];
						profileMatched = true;
						break;
					}
					profileIndex++;
				}
				if (profileMatched) fxcompile.Options.RemoveAt(profileIndex);
				if (shaderModel != null)
				{
					fxcompile.Options.Add("/" + shaderModel);
				}
				if (shaderType != null) fxcompile.Options.Add("/" + shaderType);

				List<string> buildToolOptions = new List<string>();
				buildToolOptions.AddRange(fxcompile.Options);
				if (file != null && file.Data is FileData)
				{
					FileData fileData = file.Data as FileData;
					List<string> options = fileData.Data as List<string>;
					buildToolOptions.AddRange(options as List<string>);
				}

				var nameToXMLValue = new SortedDictionary<string, string>();
				nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

				ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).FxCompile, nameToXMLValue, buildToolOptions, "FxCompile", true, nameToDefaultXMLValue);

				// For some options when the command line flag is present visual studio considers it the default
				// And only when the flag is not present does visual studio add an element to the project file.
				foreach (var item in FxCompileSwitchDefaults)
				{
					if (!nameToXMLValue.ContainsKey(item.Key))
					{
						writer.WriteElementString(item.Key, item.Value);
					}
				}
				// Library type cannot set an entry point, and if we don't set the entrypoint
				// as an empty string visual studio will give it the default value "main"
				if (!nameToXMLValue.ContainsKey("EntryPointName") &&
					nameToXMLValue.ContainsKey("ShaderType") &&
					nameToXMLValue["ShaderType"] == "Library")
				{
					nameToXMLValue.Add("EntryPointName", "");
				}

				foreach (var item in nameToXMLValue)
				{
					writer.WriteElementString(item.Key, item.Value);
				}

				if (file == null)
					writer.WriteEndElement(); //Tool
			}
		}

		private Dictionary<string, string> WavePsslcSwitchDefaults = new Dictionary<string, string>()
		{
		};

		protected override void WriteWavePsslcTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool is BuildTool psslc)
			{
				if (file == null)
				{
					// since we are only using file specific settings, 
					// skip the general block because apparently it causes problems
					// related to the "too many input files" error and setting the embed setting
					return; 
					//writer.WriteStartElement("WavePsslc");
				}

				List<string> buildToolOptions = new List<string>();
				buildToolOptions.AddRange(psslc.Options);
				if (file != null && file.Data is FileData)
				{
					FileData fileData = file.Data as FileData;
					List<string> options = fileData.Data as List<string>;
					buildToolOptions.AddRange(options as List<string>);
				}

				var nameToXMLValue = new SortedDictionary<string, string>();
				nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

				ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).WavePsslc, nameToXMLValue, buildToolOptions, "psslc", true, nameToDefaultXMLValue);

				// For some options when the command line flag is present visual studio considers it the default
				// And only when the flag is not present does visual studio add an element to the project file.
				foreach (var item in WavePsslcSwitchDefaults)
				{
					if (!nameToXMLValue.ContainsKey(item.Key))
					{
						writer.WriteElementString(item.Key, item.Value);
					}
				}

				foreach (var item in nameToXMLValue)
				{
					writer.WriteElementString(item.Key, item.Value);
				}

				//if (file == null)
				//	writer.WriteEndElement(); //Tool
			}
		}

		protected override void WriteMIDLTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool is BuildTool midl)
			{
				if (file == null)
					writer.WriteStartElement("Midl");

				var nameToXMLValue = new SortedDictionary<string, string>();
				nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

				ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).Midl, nameToXMLValue, midl.Options, "midl", true, nameToDefaultXMLValue);

				foreach (var item in nameToXMLValue)
				{
					writer.WriteElementString(item.Key, item.Value);
				}

				if (file == null)
					writer.WriteEndElement(); //Tool
			}
		}

		protected override void WriteAsmCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			string condition = file == null ? null : GetConfigCondition(config);

			if (tool is AsmCompiler asm)
			{
				if (file == null)
				{
					writer.WriteStartElement(name);
				}
				else if (config.Compiler != "vc")
				{
					WriteElementStringWithConfigCondition(writer, "CompileAs", "CompileAsAssembler", condition);
				}

				IDictionary<string, string> nameToXMLValue = new SortedDictionary<string, string>();
				nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

				// For AsmCompiler, we're not going to bother doing a similar InitCompilerToolProperties(), there are actually limitation
				// from VisualStudio itself for only allowing max of 10 /I for example.  Since we never set this one up
				// previously, not sure if the existing as.includedirs setting is correct.  If we start having using needing to set those
				// we investigate at that time what we should do with a real use case.

				ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).Masm, nameToXMLValue, asm.Options, "as", true, nameToDefaultXMLValue);

				foreach (var item in nameToXMLValue)
				{
					WriteElementStringWithConfigCondition(writer, item.Key, item.Value, condition);
				}

				if (file != null)
				{
					if (file.Data is FileData filedata && filedata.ObjectFile != null)
					{
						WriteElementStringWithConfigCondition(writer, "ObjectFileName", GetProjectPath(filedata.ObjectFile), condition);
					}
				}
				else
				{
					writer.WriteEndElement(); //Tool
				}

			}
			else if (file != null)  // File references common tool settings:
			{
				if (config.Compiler != "vc")
				{
					WriteElementStringWithConfigCondition(writer, "CompileAs", "CompileAsAssembler", condition);
				}

				FileData filedata = file.Data as FileData;
				if (filedata != null && filedata.ObjectFile != null)
				{
					WriteElementStringWithConfigCondition(writer, "ObjectFileName", GetProjectPath(filedata.ObjectFile), condition);
				}
			}
		}

		Dictionary<Tool, IDictionary<string, string>> _processSwitchesCache = new Dictionary<Tool, IDictionary<string, string>>();


		protected override void WriteCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			string condition = file == null ? null : GetConfigCondition(config);

			if (tool is CcCompiler cc)
			{
				if (file == null)
				{
					writer.WriteStartElement(name);
				}

				IDictionary<string, string> projectLevelXmlValues = null;

				if (!_processSwitchesCache.TryGetValue(tool, out IDictionary<string, string> nameToXMLValue))
				{
					Tool projectLevelCc = module.Tools.FirstOrDefault(t => t is CcCompiler);
					if (file != null && projectLevelCc != null)
					{
						_processSwitchesCache.TryGetValue(projectLevelCc, out projectLevelXmlValues);
					}

					nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

					bool includeDirQuotes = module.Package.Project.GetPropertyOrDefault("cc.includedirs." + config.System + ".quotes", "true") == "true";
					InitCompilerToolProperties(module, config, GetAllIncludeDirectories(cc, includeDirQuotes), GetAllUsingDirectories(cc), GetAllDefines(cc, config).ToString(";"), out nameToXMLValue, nameToDefaultXMLValue);

					// GRADLE-TODO: gross tegra hack
					//   - has it's own special metadata for system include dirs
					//   - if portable don't add system include path
					//   - leave it as default and assume MSBuild will figure it out
					if (IsTegraAndroid(module) && !BuildGenerator.IsPortable && (cc.SystemIncludeDirs?.Any() ?? false))
					{
						nameToXMLValue.Add("SystemIncludeDirectories", cc.SystemIncludeDirs.ToString(";", p => p.Path));
					}

					var usemultiproc = GetBultTypeOption(module, file, "multiprocessorcompilation");

					if (usemultiproc.IsOptionBoolean() && false == usemultiproc.OptionToBoolean())
					{
						nameToDefaultXMLValue["MultiProcessorCompilation"] = "FALSE";
					}

					ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).Cc, nameToXMLValue, cc.Options, "cc", true, nameToDefaultXMLValue);

					if (nameToXMLValue.ContainsKey("CompileAsManaged") && TargetFrameworkFamily == DotNetFrameworkFamilies.Core)
					{
						nameToXMLValue["CompileAsManaged"] = "NetCore";
					}

					if (module.IsKindOf(Module.Managed))
					{
						FilterForcedUsingFiles(nameToXMLValue, module, cc);
					}

					ExecuteExtensions(module, extension => extension.WriteExtensionCompilerToolProperties(cc, file, nameToXMLValue));

					_processSwitchesCache.Add(tool, nameToXMLValue);
				}

				// Now remove file level elements that already defined in project level (for performance - reduce project file size)
				// Also insert empty element that exist in project level but not desirable in file level.
				if (file != null && projectLevelXmlValues != null && projectLevelXmlValues.Any())
				{
					foreach (KeyValuePair<string, string> itm in projectLevelXmlValues)
					{
						if (nameToXMLValue.ContainsKey(itm.Key) && nameToXMLValue[itm.Key] == itm.Value)
						{
							// If this is already defined in project level, no need to re-define it.  Remove this from list and just let project inheritance get the value.
							nameToXMLValue.Remove(itm.Key);
						}
						else if (itm.Key == "ForcedIncludeFiles" && !nameToXMLValue.ContainsKey(itm.Key))
						{
							// We seems to have a lot of files with optionset assigned that is not a buildtype.  As a result, we actually have a lot of potential
							// missing fields that we rely on inheriting from project level.  So for now, we explicitly check fields that should not be inherited and
							// empty those out.
							nameToXMLValue.Add(itm.Key, "");
						}
					}
				}

				// mad hacks to send all the include dirs to a response file so we don't go over the 32k-character
				// command-line length limit in CreateProcess.
				if (IsTegraAndroid(module))
				{
					var filename = "includes.rsp";

					// make a copy of the properties if we're doing a particular file
					if (file != null)
					{
						var fileXMLValues = nameToXMLValue.ToDictionary(e => e.Key, e => (string)e.Value.Clone());
						nameToXMLValue = fileXMLValues;
						filename = Path.GetFileName(file.FileName).TrimExtension() + ".includes.rsp";
					}

					// write response file
					var responseFilepath = Path.Combine(module.IntermediateDir.Path, filename);

					try
					{
						var lines = cc.IncludeDirs
							.Concat(cc.SystemIncludeDirs)
							.Select(x => "-I\"" + x.Path.Replace('\\', '/') + "\"");

						Directory.CreateDirectory(Path.GetDirectoryName(responseFilepath));
						File.WriteAllLines(responseFilepath, lines);
						Log.Debug.WriteLine("[{0}] File.WriteAllLines wrote successfully!", module.Name);
					}
					catch (Exception e)
					{
						Log.Warning.WriteLine("[{0}] File.WriteAllLines encountered:", module.Name);
						Log.Warning.WriteLine(e.Message);

						// bail early - maybe a response file wasn't needed anyhow
						return;
					}

					// add response file to AdditionalOptions
					if (nameToXMLValue.ContainsKey("AdditionalOptions"))
						nameToXMLValue["AdditionalOptions"] += " @" + responseFilepath;
					else
						nameToXMLValue.Add("AdditionalOptions", "@" + responseFilepath);

					// write nothing to (different from deleting) AdditionalIncludeDirectories & SystemIncludeDirectories
					if (nameToXMLValue.ContainsKey("AdditionalIncludeDirectories"))
						nameToXMLValue["AdditionalIncludeDirectories"] = "";
					else
						nameToXMLValue.Add("AdditionalIncludeDirectories", "");

					if (nameToXMLValue.ContainsKey("SystemIncludeDirectories"))
						nameToXMLValue["SystemIncludeDirectories"] = "";
					else
						nameToXMLValue.Add("SystemIncludeDirectories", "");
				}

				//Apply default attributes
				foreach (var item in nameToXMLValue)
				{
					WriteElementStringWithConfigCondition(writer, item.Key, item.Value, condition);
				}

				if (file != null)
				{
					if (file.Data is FileData filedata && filedata.ObjectFile != null)
					{
						string objFile = filedata.ObjectFile.Path;

						if (config.Compiler == "vc")
						{
							if (!filedata.IsKindOf(FileData.ConflictObjName) && (cc.Options.Any(op=> op.StartsWith("-MP") || op.StartsWith("/MP"))))
							{
								if (_duplicateMPObjFiles.Add(config.Name + "/" + Path.GetFileNameWithoutExtension(file.Path.Path)))
								{
									// No global conflict, don't write anything
									objFile = null;
								}
							}
						}
						if(objFile != null)
						{
							WriteElementStringWithConfigCondition(writer, "ObjectFileName", GetProjectPath(objFile), condition);
						}
					}
					if (IsConfigEnabledForVSUnityFlag(module.Configuration) && IsSrcFileForConfigInCompileList(module.Configuration, file.FileName))
					{
						// If we actually need to compile this file, mark this file as not being in unity file.
						WriteElementStringWithConfigCondition(writer, "IncludeInUnityFile", "false", condition);
					}
				}
				else //  file == null (ClCompile tool define)
				{
					if (IsConfigEnabledForVSUnityFlag(module.Configuration))
					{
						// Setting up default for all files (unless otherwise stated) will be in bulk build files and setting
						// CustomUnitFile to true meaning we generate the bulkbuild file ourselves instead of letting VisualStudio do it.
						WriteElementStringWithConfigCondition(writer, "CustomUnityFile", "true", condition);
						WriteElementStringWithConfigCondition(writer, "IncludeInUnityFile", "true", condition);
					}
					writer.WriteEndElement(); //Tool
				}
			}
			else if (file != null)  // File references common tool settings:
			{
				if (file.Data is FileData filedata && filedata.ObjectFile != null)
				{
					string objFile = filedata.ObjectFile.Path;

					if (config.Compiler == "vc")
					{
						if (module is Module_Native native && native.Cc != null && (!filedata.IsKindOf(FileData.ConflictObjName) && (native.Cc.Options.Any(op => op.StartsWith("-MP") || op.StartsWith("/MP")))))
						{
							if (_duplicateMPObjFiles.Add(config.Name + "/" + Path.GetFileNameWithoutExtension(file.Path.Path)))
							{
								// No global conflict, don't write anything
								objFile = null;
							}
						}
					}
					if (objFile != null)
					{
						WriteElementStringWithConfigCondition(writer, "ObjectFileName", GetProjectPath(objFile), condition);
					}
				}

				if (IsConfigEnabledForVSUnityFlag(module.Configuration) && IsSrcFileForConfigInCompileList(module.Configuration, file.FileName))
				{
					// If we actually need to compile this file, mark this file as not being in unity file.
					WriteElementStringWithConfigCondition(writer, "IncludeInUnityFile", "false", condition);
				}
			}
		}

		private void FilterForcedUsingFiles(IDictionary<string, string> nameToXMLValue, Module module, CcCompiler cc)
		{
			// Assemblies that are referenced in project file should not be added as -FU.  o
			string forcedusingfiles;
			if (nameToXMLValue.TryGetValue("ForcedUsingFiles", out forcedusingfiles))
			{
				var forcedusingassemblies = forcedusingfiles.ToArray(";");
				var finalFUList = new List<string>();

				//Only assemblies added through forceusing-assemblies fileset:
				var fuAssemblies = module.Package.Project.GetFileSet(module.GroupName + ".forceusing-assemblies");
				if (fuAssemblies != null)
				{
					finalFUList.AddRange(forcedusingassemblies.Where(path => fuAssemblies.FileItems.Any(fi => fi.Path.Path.Equals(path, StringComparison.OrdinalIgnoreCase))));
				}
				// Or assemblies directly added through compiler options but not present in cc.Assemblies;
				if (cc.Assemblies != null)
				{
					finalFUList.AddRange(forcedusingassemblies.Where(path => !cc.Assemblies.FileItems.Any(fi => fi.Path.Path.Equals(path, StringComparison.OrdinalIgnoreCase))));
				}

				if (finalFUList.Count == 0)
				{
					nameToXMLValue.Remove("ForcedUsingFiles");
				}
				else
				{
					nameToXMLValue["ForcedUsingFiles"] = finalFUList.ToString(";");
				}
			}
		}

		protected override void WriteLibrarianTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			Librarian lib = tool as Librarian;

			if (lib != null)
			{
				writer.WriteStartElement(name);

				IDictionary<string, string> nameToXMLValue;
				nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

				module.PrimaryOutput(out string outputdir, out string targetname, out string targetext);
				string primparyOutput = Path.Combine(outputdir, targetname + targetext);

				InitLibrarianToolProperties(config, GetLibrarianObjFiles(module, lib), primparyOutput, out nameToXMLValue, nameToDefaultXMLValue);

				ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).Lib, nameToXMLValue, lib.Options, "lib", true, nameToDefaultXMLValue);

				if (module.Package.Project.Properties.GetBooleanPropertyOrDefault("package." + module.Package.Name + "." + module.Name + ".issharedpchmodule", false) &&
					module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.use-shared-pch-binary", true) &&
					module.Package.Project.Properties.GetBooleanPropertyOrDefault(config.System + ".support-shared-pch" , false) &&
					!module.Package.Project.Properties.GetBooleanPropertyOrDefault(config.System + ".shared-pch-generate-lib", true) &&
					nameToXMLValue.ContainsKey("OutputFile"))
				{
					// For most (but not all) clang platforms, if the "library" module contains only a single cpp file that
					// create the .pch, no obj files will be created and therefor no library file will be generated.  If we still
					// set the OutputFile library path, we will get build error in such case.  Empty out this field allow the
					// VSI to ignore the expected library output.
					nameToXMLValue["OutputFile"] = "";
				}

				// If this '${config-system}.vcproj.use_vsi_targetext' property is provided, it means the MSBuild property TargetExt is always provided by 
				// Visual Studio's platform setup and this value can also change when user change value in project's property page.
				bool useVsiTargetExt = module.Package.Project.GetPropertyOrDefault(module.Configuration.System + ".vcproj.use_vsi_targetext", "false").ToLower() == "true";
				if (useVsiTargetExt)
				{
					if (nameToXMLValue.TryGetValue("OutputFile", out string outputfile))
					{
						if (!outputfile.EndsWith("$(TargetExt)"))
						{
							if (string.IsNullOrEmpty(targetext))
							{
								nameToXMLValue["OutputFile"] = outputfile + "$(TargetExt)";
							}
							else
							{
								nameToXMLValue["OutputFile"] = Path.ChangeExtension(outputfile, "$(TargetExt)").Replace(".$(TargetExt)", "$(TargetExt)");
							}
						}
					}
				}

				_ModuleLinkerNameToXMLValue[module.Key] = nameToXMLValue; // Store to use in project level properties;

				//Apply default attributes
				foreach (var item in nameToXMLValue)
				{
					writer.WriteElementString(item.Key, item.Value);
				}

				writer.WriteEndElement(); //Tool
			}
		}

		protected override void WriteManagedResourceCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool != null || file != null)
			{
				//var condition = file == null ? null : GetConfigCondition(config);

				if (file == null)
				{
					writer.WriteStartElement(name);
				}


				//string resourceName = intermediateDir + OutputName + @"." + Path.GetFileNameWithoutExtension(filePath) + @".resources";
				//writer.WriteAttributeString("ResourceFileName", resourceName);

				if (file == null)
				{
					writer.WriteEndElement(); //Tool
				}
			}
		}

		protected override void WriteResourceCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool != null || file != null)
			{
				if (file == null)
				{
					writer.WriteStartElement(name);
				}

				if (tool != null)
				{
					if (module is Module_Native native)
					{
						if (file == null)
						{
							var includedirs = tool.Options.Where(o => o.StartsWith("/i ") || o.StartsWith("-i ")).Select(o => o.Substring(3).TrimWhiteSpace());
							var defines = tool.Options.Where(o => o.StartsWith("/d ") || o.StartsWith("-d ")).Select(o => o.Substring(3).TrimWhiteSpace());
							// Add include directories only in generic tool.
							writer.WriteElementString("AdditionalIncludeDirectories", includedirs.ToString(";", s => GetProjectPath(s).Quote()));
							writer.WriteElementString("PreprocessorDefinitions", defines.ToString(";"));
						}
						else
						{
							var outputresourcefile = tool.OutputDependencies.FileItems.SingleOrDefault().Path.Path;
							WriteElementStringWithConfigCondition(writer, "ResourceOutputFileName", GetProjectPath(outputresourcefile), GetConfigCondition(config));
						}
					}
				}
				if (file == null)
				{
					writer.WriteEndElement(); //Tool
				}
			}

		}

		protected override void WriteLinkerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool is Linker linker)
			{
				writer.WriteStartElement("Link");

				IDictionary<string, string> nameToXMLValue;
				nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

				module.PrimaryOutput(out string outputdir, out string targetname, out string targetext);
				string primparyOutput = Path.Combine(outputdir,targetname + targetext);

				InitLinkerToolProperties(config, linker, GetAllLibraries(linker, module), GetAllLibraryDirectories(linker), GetLinkerObjFiles(module,linker), primparyOutput, out nameToXMLValue, nameToDefaultXMLValue);

				ProcessSwitches(module.Package.Project, VSConfig.GetParseDirectives(module).Link, nameToXMLValue, linker.Options, "link", true, nameToDefaultXMLValue);

				// If this '${config-system}.vcproj.use_vsi_targetext' property is provided, it means the MSBuild property TargetExt is always provided by 
				// Visual Studio's platform setup and this value can also change when user change value in project's property page.
				bool useVsiTargetExt = module.Package.Project.GetPropertyOrDefault(module.Configuration.System + ".vcproj.use_vsi_targetext", "false").ToLower() == "true";
				if (useVsiTargetExt)
				{
					if (nameToXMLValue.TryGetValue("OutputFile", out string outputfile))
					{
						if (!outputfile.EndsWith("$(TargetExt)"))
						{
							if (string.IsNullOrEmpty(targetext))
							{
								nameToXMLValue["OutputFile"] = outputfile + "$(TargetExt)";
							}
							else
							{
								nameToXMLValue["OutputFile"] = Path.ChangeExtension(outputfile, "$(TargetExt)").Replace(".$(TargetExt)", "$(TargetExt)");
							}
						}
					}
				}

				_ModuleLinkerNameToXMLValue[module.Key] = nameToXMLValue; // Store to use in project level properties;

				//Because of how VC handles incremental link with manifest we need to change replace manifest file name
				if (nameToXMLValue.ContainsKey("GenerateManifest") && ConvertUtil.ToBoolean(nameToXMLValue["GenerateManifest"]))
				{
					if (nameToXMLValue.ContainsKey("ManifestFile"))
					{
						nameToXMLValue["ManifestFile"] = GetProjectPath(Path.Combine(module.IntermediateDir.Path, Path.GetFileName(module.PrimaryOutput()) + @".intermediate.manifest"));
					}
				}
				else
				{
					nameToXMLValue.Remove("ManifestFile");
				}

				ExecuteExtensions(module, extension => extension.WriteExtensionLinkerToolProperties(nameToXMLValue));

				foreach (KeyValuePair<string, string> item in nameToXMLValue)
				{
					writer.WriteElementString(item.Key, item.Value);
				}

				ExecuteExtensions(module, extension => extension.WriteExtensionLinkerTool(writer));

				writer.WriteEndElement(); //Tool
			}
			else if (module.IsKindOf(Module.Utility) && GetConfigurationType(module) != "Utility")
			{
				writer.WriteStartElement("Link");
				writer.WriteElementString("IgnoreLibraryLibrary", "true");
				writer.WriteElementString("LinkLibraryDependencies", "false");
				writer.WriteElementString("GenerateManifest", "false");
				writer.WriteEndElement(); //Tool
			}
		}

		protected override void WriteALinkTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool != null)
			{
				writer.WriteStartElement(name);
				writer.WriteEndElement(); //Tool
			}

		}

		protected override void WriteManifestTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (module.IsKindOf(Module.Utility))
			{
				return;
			}
			var manifest = tool as BuildTool;
			if (manifest != null)
			{
				string manifestResourceFile = null;

				using (writer.ScopedElement(name))
				{
					if (manifest.Files.FileItems.Count > 0)
					{
						writer.WriteElementString("AdditionalManifestFiles", manifest.Files.FileItems.ToString(";", f => f.Path.Path).TrimWhiteSpace());
					}
					if (manifest.InputDependencies.FileItems.Count > 0)
					{
						writer.WriteElementString("InputResourceManifests", manifest.InputDependencies.FileItems.ToString(";", f => f.Path.Path).TrimWhiteSpace());
					}
					foreach (var fi in manifest.OutputDependencies.FileItems)
					{
						if (fi.Path.Path.EndsWith(".res"))
						{
							manifestResourceFile = GetProjectPath(fi.Path.Path);
						}
						else
						{
							writer.WriteElementString("OutputManifestFile", GetProjectPath(fi.Path));
						}
					}
				}

				if (manifestResourceFile != null)
				{
					writer.WriteStartElement("ManifestResourceCompile");
					{
						writer.WriteNonEmptyElementString("ManifestResourceFile", manifestResourceFile);
					}
					writer.WriteEndElement();
				}
			}
		}

		protected override void WriteXDCMakeTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool != null)
			{
				writer.WriteStartElement(name);
				writer.WriteEndElement(); //Tool
			}

		}

		protected override void WriteBscMakeTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool != null)
			{
				writer.WriteStartElement(name);
				writer.WriteEndElement(); //Tool
			}

		}

		protected override void WriteFxCopTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool != null)
			{
				writer.WriteStartElement(name);
				writer.WriteEndElement(); //Tool
			}

		}

		protected override void WriteAppVerifierTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool != null)
			{
				writer.WriteStartElement(name);
				writer.WriteEndElement(); //Tool
			}

		}

		protected override void WriteWebDeploymentTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			if (tool != null)
			{
				writer.WriteStartElement(name);
				writer.WriteEndElement(); //Tool
			}

		}

		protected override void WriteVCCustomBuildTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			var filetool = tool as BuildTool;
			if (filetool != null && file != null)
			{
				var command = GetCommandScriptWithCreateDirectories(filetool).TrimWhiteSpace();
				if (!String.IsNullOrEmpty(command))
				{
					string condition = GetConfigCondition(config);
					string description = filetool.Description.TrimWhiteSpace();
					if (BuildGenerator.IsPortable)
					{
						description = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(description, OutputDir.Path); // normalize the description as if it was command line in case it contains paths in the message
					}

					WriteElementStringWithConfigCondition(writer, "Command", command, condition);
					WriteElementStringWithConfigConditionNonEmpty(writer, "Message", description, condition);
					WriteElementStringWithConfigConditionNonEmpty(writer, "AdditionalInputs", filetool.InputDependencies.FileItems.Select(fi => GetProjectPath(fi.Path)).OrderedDistinct().ToString(";", path => path).TrimWhiteSpace(), condition);
					WriteElementStringWithConfigConditionNonEmpty(writer, "Outputs", filetool.OutputDependencies.ThreadSafeFileItems().Select(fi => GetProjectPath(fi.Path)).ToString(";", path => path).TrimWhiteSpace(), condition);
					WriteElementStringWithConfigCondition(writer, "TreatOutputAsContent", filetool.TreatOutputAsContent.ToString(), condition);
					if (filetool.IsKindOf(BuildTool.ExcludedFromBuild))
					{
						WriteElementStringWithConfigCondition(writer, "ExcludedFromBuild", "TRUE", condition);
					}
				}
				else
				{
					string outputList = filetool.OutputDependencies.ThreadSafeFileItems().Select(fi => fi.Path.Path).ToString(";", path => path).TrimWhiteSpace();
					if (!String.IsNullOrEmpty(outputList) && !filetool.IsKindOf(BuildTool.ExcludedFromBuild))
					{
						string condition = GetConfigCondition(config);
						WriteElementStringWithConfigCondition(writer, "Outputs", outputList, condition);
					}
				}
			}
		}

		protected virtual void WriteEmptyTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
		{
			//Do nothing
		}

		#endregion

		#region Data methods

		protected override void InitCompilerToolProperties(Module module, Configuration configuration, string includedirs, string usingdirs, string defines, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
		{
			base.InitCompilerToolProperties(module, configuration, includedirs, usingdirs, defines, out nameToXMLValue, nameToDefaultXMLValue);

			if (nameToXMLValue.ContainsKey("AdditionalUsingDirectories") && TargetFrameworkFamily == DotNetFrameworkFamilies.Framework)
			{
				string dotnetReferenceDir = module.Package.Project.Properties["package.DotNet.referencedir"];

				if (dotnetReferenceDir == null)
				{
					throw new BuildException("DotNet package is required but is not listed in the masterconfig.");
				}

				string dotnetFacadesDir = Path.Combine(dotnetReferenceDir, "Facades");
				nameToXMLValue["AdditionalUsingDirectories"] = string.Join(";", new string[] { nameToXMLValue["AdditionalUsingDirectories"], dotnetReferenceDir, dotnetFacadesDir });
			}

			// Remove some 2008 settings:
			nameToDefaultXMLValue.Remove("DebugInformationFormat");
			nameToDefaultXMLValue.Remove("UsePrecompiledHeader");

			nameToDefaultXMLValue["DebugInformationFormat"] = "None";

			if (Version == "10.00")
			{
				// Visual Studio 2010 uses an empty string to indicate that no PDB file should be generated.
				nameToDefaultXMLValue["DebugInformationFormat"] = "";
			}

			nameToDefaultXMLValue["CompileAs"] = "Default";
			nameToDefaultXMLValue["WarningLevel"] = "TurnOffAllWarnings";
			nameToDefaultXMLValue["PrecompiledHeader"] = "NotUsing";
			nameToDefaultXMLValue["CompileAsWinRT"] = "false";

			var useMpThreads = module.Package.Project.Properties["eaconfig.build-MP"];
			if (configuration.Compiler != "vc")
			{
				if (useMpThreads != null)
				{
					if (String.IsNullOrEmpty(useMpThreads) || (useMpThreads.IsOptionBoolean() && useMpThreads.OptionToBoolean()))
					{
						nameToXMLValue["MultiProcessorCompilation"] = "TRUE";
					}
					else if (!useMpThreads.IsOptionBoolean())
					{
						int threads;
						if (Int32.TryParse(useMpThreads, out threads))
						{
							nameToXMLValue["MultiProcessorCompilation"] = "TRUE";
							nameToXMLValue["ProcessorNumber"] = threads.ToString();
						}
					}
				}
			}
			else
			{
				nameToDefaultXMLValue["BasicRuntimeChecks"] = "Default";
			}
		}

		protected override void InitLinkerToolProperties(Configuration configuration, Linker linker, IEnumerable<PathString> libraries, IEnumerable<PathString> libraryDirs, IEnumerable<PathString> objects, string defaultOutput, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
		{
			nameToXMLValue = new SortedDictionary<string, string>();

			if (linker.UseLibraryDependencyInputs)
			{
				nameToXMLValue.Add("UseLibraryDependencyInputs", "TRUE");
			}

			//**********************************
			//* Set defaults that should exist before the switch parsing
			//* These are either something that a switch can add to (if switch setOnce==false)
			//* or something that ever can't be changed (if switch setOnce==true)
			//**********************************
			// GRADLE-TODO: android link order BS hack
			if (libraries.Any() || objects.Any())
			{
				// Sorted to make sure generation is deterministic
				var additionalDependencies = new SortedSet<String>(libraries.Concat(objects).Select(p => GetProjectPath(p).Quote()));
				string linkObjs = additionalDependencies.ToString(";");
				if (configuration.System == "android")
				{
					linkObjs = "-Wl,--start-group;" + linkObjs + ";-Wl,--end-group;";
				}
				nameToXMLValue.AddNonEmpty("AdditionalDependencies", linkObjs);
			}
			nameToXMLValue.AddNonEmpty("AdditionalLibraryDirectories", libraryDirs.ToString(";", p => GetProjectPath(p).Quote()));

			//*****************************
			//* Set the defaults that are used if a particular attribute has no value after the parsing
			//*****************************

			nameToDefaultXMLValue.Add("GenerateDebugInformation", "FALSE");
			nameToDefaultXMLValue.Add("OutputFile", defaultOutput);

			if (linker.ImportLibFullPath != null && !String.IsNullOrEmpty(linker.ImportLibFullPath.Path))
			{
				nameToDefaultXMLValue.Add("ImportLibrary", GetProjectPath(linker.ImportLibFullPath));
			}
		}

		protected override void InitLibrarianToolProperties(Configuration configuration, IEnumerable<PathString> objects, string defaultOutput, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
		{
			nameToXMLValue = new SortedDictionary<string, string>();
			var additionalDependencies = new SortedSet<String>(objects.Select(p => GetProjectPath(p).Quote()));
			nameToXMLValue.AddNonEmpty("AdditionalDependencies", additionalDependencies.ToString(";"));
			nameToDefaultXMLValue.Add("OutputFile", defaultOutput);
		}

		protected virtual string GetConfigurationType(IModule module)
		{

			foreach (KeyValuePair<uint, string> moduleTypeToConfig in s_moduleTypesToConfigurations)
			{
				if (module.IsKindOf(moduleTypeToConfig.Key))
				{
					return moduleTypeToConfig.Value;
				}
			}

			throw new BuildException($"Couldn't detected correct MSBuild configuration type for module {module.Name}.");
		}

		protected virtual string GetPlatformToolSet(IModule module)
		{
			using (var scope = module.Package.Project.Properties.ReadScope())
			{
				if (module.Configuration.System == "android")
				{
					var platformToolset =
						scope["visualstudio.platform-toolset"] ??
						scope["package.VisualStudio.platformtoolset"] ??
						GetDefaultAndroidPlatformToolset(module);

					return platformToolset;
				}
				else
				{
					var platformToolset =
						scope["visualstudio.platform-toolset"] ??
						scope["package.VisualStudio.platformtoolset"] ??
						DefaultToolset;

					return platformToolset;
				}
			}
		}

		virtual protected HashSet<string> GetSkipProjectReferences(IModule module)
		{
			HashSet<string> skipModules = null;

			if(module.Package.Project != null)
			{
				var skiprefsproperty = (module.Package.Project.Properties[module.GroupName + ".skip-visualstudio-references"] + Environment.NewLine + module.Package.Project.Properties[module.GroupName + ".skip-visualstudio-references." + module.Configuration.System]).TrimWhiteSpace();

				skipModules = new HashSet<string>();

				foreach (string dep in StringUtil.ToArray(skiprefsproperty))
				{
					string package = null;
					string group = "runtime";
					string moduleName = null;

					IList<string> depDetails = StringUtil.ToArray(dep, "/");
					switch (depDetails.Count)
					{
						case 1: // Package name
							package = depDetails[0];
							break;
						case 2: // Package name + module name
							package = depDetails[0];
							moduleName = depDetails[1];
							break;
						case 3: //Package name + group name + module name;
							package = depDetails[0];
							group = depDetails[1];
							moduleName = depDetails[2];
							break;
						default:
							throw new BuildException($"{module.LogPrefix}Invalid 'skip-visualstudio-references' value: {dep}, valid format are 'package_name' or 'package_name/module_name' or 'package_name/group/module_name', where group is one of 'runtime, test, example, tool'.");
					}

					package = package.TrimWhiteSpace();
					var buildGroup = ConvertUtil.ToEnum<BuildGroups>(group.TrimWhiteSpace());
					moduleName = moduleName.TrimWhiteSpace();


					if (!String.IsNullOrEmpty(moduleName))
					{
						foreach (var d in module.Dependents.Where(d => d.Dependent.Package.Name == package && d.Dependent.BuildGroup == buildGroup && d.Dependent.Name == moduleName))
						{
							skipModules.Add(d.Dependent.Key);
						}
					}
					else
					{
						foreach (var d in module.Dependents.Where(d => d.Dependent.Package.Name == package && d.Dependent.BuildGroup == buildGroup))
						{
							skipModules.Add(d.Dependent.Key);
						}
					}
				}


			}
			return skipModules;
		}

		protected virtual void GetReferences(out IEnumerable<ProjectRefEntry> projectReferences, out IDictionary<PathString, FileEntry> references, out IDictionary<PathString, FileEntry> comreferences, out IDictionary<Configuration, ISet<PathString>> referencedAssemblyDirs)
		{
			var projectRefs = new List<ProjectRefEntry>();
			projectReferences = projectRefs;

			// Collect project referenced assemblies by configuration:
			var projectReferencedAssemblies = new Dictionary<Configuration, ISet<PathString>>();

			foreach (VSProjectBase depProject in Dependents)
			{
				var refEntry = new ProjectRefEntry(depProject);
				projectRefs.Add(refEntry);

				foreach (var module in Modules)
				{
					uint referenceFlags = 0;
					bool moduleHasLink = false;

					if (module is Module_Native)
					{
						var module_native = module as Module_Native;

						if (module_native.Link != null)
						{
							moduleHasLink = true;

							if (module_native.Link.UseLibraryDependencyInputs)
							{
								referenceFlags |= ProjectRefEntry.UseLibraryDependencyInputs;
							}
						}
					}

					// Find dependent project configuration that corresponds to this project config:
					Configuration dependentConfig;

					var mapEntry = ConfigurationMap.Where(e => e.Value == module.Configuration);
					if (mapEntry.Count() > 0 && ConfigurationMap.TryGetValue(mapEntry.First().Key, out dependentConfig))
					{
						var depModule = depProject.Modules.SingleOrDefault(m => m.Configuration == dependentConfig);
						if (depModule != null)
						{
							var skipReferences = GetSkipProjectReferences(module);

							if (skipReferences != null && skipReferences.Contains(depModule.Key))
							{
								continue;
							}

							var moduleDependency = module.Dependents.FirstOrDefault(d => d.Dependent.Key == depModule.Key);

							if (moduleDependency != null)
							{
								if (moduleHasLink)
								{
									if (moduleDependency.IsKindOf(DependencyTypes.Link) &&
										moduleDependency.Dependent.IsKindOf(Module.DotNet | Module.Library | Module.DynamicLibrary))
									{
										referenceFlags |= ProjectRefEntry.LinkLibraryDependencies;
									}
									if (moduleDependency.IsKindOf(DependencyTypes.Link) &&
										moduleDependency.Dependent.IsKindOf(Module.DotNet | Module.Managed))
									{
										referenceFlags |= ProjectRefEntry.ReferenceOutputAssembly;
									}
								}

								if (module.IsKindOf(Module.WinRT))
								{
									if (moduleDependency.IsKindOf(DependencyTypes.Link) &&
										moduleDependency.Dependent.IsKindOf(Module.WinRT) &&
										moduleDependency.Dependent.IsKindOf(Module.DotNet | Module.Managed | Module.DynamicLibrary))
									{
										referenceFlags |= ProjectRefEntry.ReferenceOutputAssembly;
									}
								}
							}

							PathString assembly = null;

							if (depModule.IsKindOf(Module.DotNet))
							{
								var dotnetmod = depModule as Module_DotNet;
								if (dotnetmod != null)
								{
									assembly = PathString.MakeCombinedAndNormalized(dotnetmod.OutputDir.Path, dotnetmod.Compiler.OutputName + dotnetmod.Compiler.OutputExtension);
								}
							}
							else if (depModule.IsKindOf(Module.Managed))
							{
								var nativetmod = depModule as Module_Native;
								if (nativetmod != null && nativetmod.Link != null)
								{
									if (nativetmod.Link != null)
									{
										assembly = PathString.MakeCombinedAndNormalized(nativetmod.Link.LinkOutputDir.Path, nativetmod.Link.OutputName + nativetmod.Link.OutputExtension);
									}
								}
							}

							if (assembly != null)
							{
								ISet<PathString> assemblies;
								if (!projectReferencedAssemblies.TryGetValue(module.Configuration, out assemblies))
								{
									assemblies = new HashSet<PathString>();
									projectReferencedAssemblies.Add(module.Configuration, assemblies);
								}

								assemblies.Add(assembly);
							}

							refEntry.TryAddConfigEntry(module.Configuration, referenceFlags);
						}
					}
				}
			}

			references = new Dictionary<PathString, FileEntry>();
			comreferences = new Dictionary<PathString, FileEntry>();

			referencedAssemblyDirs = new Dictionary<Configuration, ISet<PathString>>();

			foreach (var entry in projectReferencedAssemblies)
			{
				referencedAssemblyDirs[entry.Key] = new HashSet<PathString>(entry.Value.Select(p=> PathString.MakeNormalized(Path.GetDirectoryName(p.Path))));
			}

			foreach (var module in Modules.Where(m => m is Module_Native).Cast<Module_Native>())
			{
				if (module.Cc != null)
				{
					if (module.IsKindOf(Module.Managed | Module.WinRT))
					{
						ISet<PathString> assemblies = null;
						projectReferencedAssemblies.TryGetValue(module.Configuration, out assemblies);

						foreach (var assembly in module.Cc.Assemblies.FileItems)
						{
							// Exclude any assembly that is already included through project references:
							if (assemblies != null && assemblies.Contains(assembly.Path))
							{
								continue;
							}

							// Exclude any assembly that comes from an SDK reference, MSBuild will handle this for us
							if (assembly.OptionSetName == "sdk-reference")
							{
								continue;
							}

							uint copyLocal = 0;
							if (module.CopyLocal == CopyLocalType.True)
							{
								copyLocal = FileEntry.CopyLocal;
							}

							// Check if assembly has optionset with copylocal flag attached.
							var assemblyCopyLocal = assembly.GetCopyLocal(module);
							if (assemblyCopyLocal == CopyLocalType.True)
							{
								copyLocal = FileEntry.CopyLocal;
							}
							else if (assemblyCopyLocal == CopyLocalType.False)
							{
								copyLocal = 0;
							}

							var key = new PathString(Path.GetFileName(assembly.Path.Path));
							UpdateFileEntry(module,references, key, assembly, module.Cc.Assemblies.BaseDirectory, module.Configuration, flags: copyLocal);

							if(!assembly.AsIs && PathUtil.IsValidPathString(assembly.Path.Path))
							{
								var assemblyPath = Path.GetDirectoryName(assembly.Path.Path);

								ISet<PathString> assemblydirs;
								if (!referencedAssemblyDirs.TryGetValue(module.Configuration, out assemblydirs))
								{
									assemblydirs = new HashSet<PathString>();
									referencedAssemblyDirs.Add(module.Configuration, assemblydirs);
								}

								assemblydirs.Add(PathString.MakeNormalized(Path.GetDirectoryName(assembly.Path.Path)));

							}
						}
					}
					else
					{
						ISet<PathString> assemblies = null;
						projectReferencedAssemblies.TryGetValue(module.Configuration, out assemblies);

						foreach (var assembly in module.Cc.ComAssemblies.FileItems)
						{
							// Exclude any assembly that is already included through project references:
							if (assemblies != null && assemblies.Contains(assembly.Path))
							{
								continue;
							}

							// Exclude any assembly that comes from an SDK reference, MSBuild will handle this for us
							if (assembly.OptionSetName == "sdk-reference")
							{
								continue;
							}

							uint copyLocal = 0;
							if (module.CopyLocal == CopyLocalType.True)
							{
								copyLocal = FileEntry.CopyLocal;
							}

							// Check if assembly has optionset with copylocal flag attached.
							var assemblyCopyLocal = assembly.GetCopyLocal(module);
							if (assemblyCopyLocal == CopyLocalType.True)
							{
								copyLocal = FileEntry.CopyLocal;
							}
							else if (assemblyCopyLocal == CopyLocalType.False)
							{
								copyLocal = 0;
							}

							var key = new PathString(Path.GetFileName(assembly.Path.Path));
							UpdateFileEntry(module, comreferences, key, assembly, module.Cc.ComAssemblies.BaseDirectory, module.Configuration, flags: copyLocal);
						}
					}
				}
			}
		}

		protected OptionSet GetBultTypeOptions(Module module, FileItem file)
		{
			var setname = (file != null && !String.IsNullOrEmpty(file.OptionSetName)) ? file.OptionSetName : ((file != null) ? (module as ProcessableModule).BuildType.Name : null);

			OptionSet buildTypeSet = null;

			if (!String.IsNullOrEmpty(setname))
			{
				module.Package.Project.NamedOptionSets.TryGetValue(setname, out buildTypeSet);
			}

			return buildTypeSet;
		}

		protected string GetBultTypeOption(Module module, FileItem file, string option_name)
		{
			string option_value = null;

			var setname = (file != null && !String.IsNullOrEmpty(file.OptionSetName)) ? file.OptionSetName : ((file != null) ? (module as ProcessableModule).BuildType.Name : null);

			if (!String.IsNullOrEmpty(setname))
			{
				OptionSet buildTypeSet;
				if (module.Package.Project.NamedOptionSets.TryGetValue(setname, out buildTypeSet))
				{
					option_value = buildTypeSet.Options[option_name];
				}
			}

			return option_value;
		}

		#endregion

		#region utility methods

		protected virtual IEnumerable<PathString> GetLinkerObjFiles(Module module, Linker linker)
		{
			// Resources are linked by VisualStudio. Exclude from object files
			var resources = module.Tools.Where(t => t.ToolName == "rc" && !t.IsKindOf(Tool.ExcludedFromBuild)).Select(t => t.OutputDependencies.FileItems.SingleOrDefault().Path);

			var objectfiles = linker.ObjectFiles.FileItems.Select(item => item.Path).Except(resources);

			if (objectfiles.FirstOrDefault() != null)
			{
				// When custombuildfiles Outputs contain obj files they are sent to linker
				// by MSBuild targets. An explicit exclude definition is required
				// to remove them.
				// Note: similar code exists below for library modules.  Possible refactor:
				// Have the Linker tool derive from Librarian.  That would provide access
				// to the Object File list and could eliminate the similar function below.

				var excludetools = new string[] { "rc", "midl", "resx" }; 

				var validExtensions = GetPlatformObjLibExt(module);

				var custombuildtool = module.Tools
					.Where(t => t.IsKindOf(Tool.TypePreCompile) && !t.IsKindOf(Tool.ExcludedFromBuild) && !excludetools.Contains(t.ToolName))
					.SelectMany(t => t.OutputDependencies.ThreadSafeFileItems(), (t, fi) => fi.Path).Where(p => validExtensions.Contains(Path.GetExtension(p.Path)));

				objectfiles = objectfiles.Except(custombuildtool);

			}
			
			if (linker.UseLibraryDependencyInputs)
			{
				//    A bug in Visual Studio 2010 & VS 2012.0 causes the linker to ignore
				//    object files that are not built with the compiler when the option Use
				//    Library Dependency Inputs is set to Yes, which is a pre-requisite for
				//    enabling incremental linking.

				var objects = GetObjFilesFromDependentProjects(module);
				if (objects.Count > 0)
				{
					objects.AddRange(objectfiles);
					objectfiles = objects;
				}
			}
			 
			return objectfiles.OrderedDistinct();
		}

		protected virtual List<PathString> GetObjFilesFromDependentProjects(Module module)
		{
			var depObjFiles = new List<PathString>();

			var excludetools = new string[] { "rc", "midl", "resx" }; 

			foreach (VSProjectBase depProject in Dependents)
			{
					// Find dependent project configuration that corresponds to this project config:
					Configuration dependentConfig;

					var mapEntry = ConfigurationMap.Where(e => e.Value == module.Configuration);
					if (mapEntry.Count() > 0 && ConfigurationMap.TryGetValue(mapEntry.First().Key, out dependentConfig))
					{
						var depModule = depProject.Modules.SingleOrDefault(m => m.Configuration == dependentConfig) as Module;
						Dependency<IModule> dep;
						if (depModule != null && module.Dependents.TryGet(depModule, out dep))
						{
							if (dep.IsKindOf(DependencyTypes.Link))
							{
								var validExtensions = GetPlatformObjLibExt(depModule);

								var toolObjFiles = depModule.Tools
												.Where(t => t.IsKindOf(Tool.TypePreCompile) && !t.IsKindOf(Tool.ExcludedFromBuild) && !excludetools.Contains(t.ToolName))
												.SelectMany(t => t.OutputDependencies.ThreadSafeFileItems(), (t, fi) => fi.Path).Where(p => validExtensions.Contains(Path.GetExtension(p.Path)));

								depObjFiles.AddRange(toolObjFiles);

								// vs 2010, 2012 won't pass .objs from asm source files to incremental linking so it is done here
								Module_Native nativeModule = depModule as Module_Native;
								if (nativeModule != null)
								{
									if (nativeModule.Asm != null)
									{
										foreach (FileItem fileItem in nativeModule.Asm.SourceFiles.FileItems)
										{
											FileData fileData = fileItem.Data as FileData;
											if (fileData != null && !fileData.ObjectFile.IsNullOrEmpty())
											{
												depObjFiles.Add(fileData.ObjectFile);
											}
										}
									}
								}
							}
						}
					}
			}

			return depObjFiles;
		}


		private HashSet<string> GetPlatformObjLibExt(Module module)
		{
			var valid_extensions = new HashSet<string>() { ".obj", ".lib" };

			if (module.Configuration.Compiler != "vc")
			{
				if (module.Package.Project != null)
				{
					var platform_obj_ext = module.Package.Project.Properties["eaconfig.visual-studio.ComputeCustomBuildOutput.ext"];

					if (!string.IsNullOrEmpty(platform_obj_ext))
					{
						foreach (var ext in platform_obj_ext.ToArray())
						{
							valid_extensions.Add(ext);
						}
					}
				}
			}

			return valid_extensions;
		}

		protected virtual IEnumerable<PathString> GetLibrarianObjFiles(Module module, Librarian librarian)
		{
			var objectfiles = librarian.ObjectFiles.FileItems.Select(item => item.Path);

			if (objectfiles.FirstOrDefault() != null)
			{
				// When custombuildfiles outputs contain obj files they are sent to the
				// librarian by MSBuild targets. An explicit exclude definition is required
				// to remove them.
				// Note: this code is quite similar to the GetLinkerObjFiles function, but
				// simplified for librarian tool requirements, which can only have object
				// files that could be linked.

				var validExtensions = GetPlatformObjLibExt(module);

				var custombuildtool = module.Tools
					.Where(t => t.IsKindOf(Tool.TypePreCompile) && !t.IsKindOf(Tool.ExcludedFromBuild))
					.SelectMany(t => t.OutputDependencies.FileItems, (t, fi) => fi.Path).Where(p => validExtensions.Contains(Path.GetExtension(p.Path)));

				objectfiles = objectfiles.Except(custombuildtool);
			}

			return objectfiles;
		}

		private void WriteMSVCOverride(IXmlWriter writer)
		{
			if (StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.useCustomMSVC", false))
			{
				if (StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.use-non-proxy-build-tools", false))
				{
					if (!PackageMap.Instance.MasterConfigHasPackage("MSBuildTools"))
					{
						throw new BuildException("MSBuildTools package is not listed in masterconfig, but package.VisualStudio.use-non-proxy-build-tools was set to true which requires the MSBuildTools package.");
					}

					bool success = PackageMap.Instance.TryGetMasterPackage(StartupModule.Package.Project, "MSBuildTools", out MasterConfig.IPackage package);

					if (success && package.Version.StrCompareVersions("16.0") >= 0)
					{
						TaskUtil.Dependent(StartupModule.Package.Project, "MSBuildTools");
						string versionPropsLocation = StartupModule.Package.Project.Properties.GetPropertyOrDefault("package.MSBuildTools.vc-version-props", string.Empty);
						if (string.IsNullOrEmpty(versionPropsLocation))
						{
							Log.Warning.WriteLine($"Microsoft.VCToolsVersion.default.props file not found in MSBuildTools: enabling the package.VisualStudio.useCustomMSVC property requires that " +
							                      $"the MSBuildTools package being used contains a Microsoft.VCToolsVersion.default.props file. useCustomMSVC will not be enabled for this project.");
						}
						else
						{
							ImportProject(writer, versionPropsLocation);
						}
					}
				}
			}
		}

		protected void ImportProject(IXmlWriter writer, string project, string condition = null, string label = null)
		{
			writer.WriteStartElement("Import");
			writer.WriteAttributeString("Project", project);
			if (!String.IsNullOrEmpty(condition))
			{
				writer.WriteAttributeString("Condition", condition);
			}
			if (!String.IsNullOrEmpty(label))
			{
				writer.WriteAttributeString("Label", label);
			}

			writer.WriteEndElement();
		}

		private void ImportGroup(IXmlWriter writer, string label = null, string condition = null, List<Tuple<string,string,string>> projects=null)
		{
			if (projects == null || !projects.Any())
				return;

			writer.WriteStartElement("ImportGroup");
			if (!String.IsNullOrEmpty(condition))
			{
				writer.WriteAttributeString("Condition", condition);
			}
			if (!String.IsNullOrEmpty(label))
			{
				writer.WriteAttributeString("Label", label);
			}
			foreach (Tuple<string,string,string> proj in projects)
			{
				ImportProject(writer, proj.Item1, proj.Item2, proj.Item3);
			}
			writer.WriteEndElement();
		}

		private void StartPropertyGroup(IXmlWriter writer, string condition = null, string label = null)
		{
			writer.WriteStartElement("PropertyGroup");
			if (!string.IsNullOrEmpty(condition))
			{
				writer.WriteAttributeString("Condition", condition);
			}
			if (!string.IsNullOrEmpty(label))
			{
				writer.WriteAttributeString("Label", label);
			}
		}

		protected void StartElement(IXmlWriter writer, string name, string condition = null, string label = null)
		{
			writer.WriteStartElement(name);
			if (!String.IsNullOrEmpty(condition))
			{
				writer.WriteAttributeString("Condition", condition);
			}
			if (!string.IsNullOrEmpty(label))
			{
				writer.WriteAttributeString("Label", label);
			}
		}

		protected virtual void WriteNamedElements(IXmlWriter writer, IDictionary<string, string> attributes)
		{
			foreach (KeyValuePair<string, string> item in attributes)
			{
				writer.WriteNonEmptyElementString(item.Key, item.Value);
			}
		}

		protected override void WriteOneTool(IXmlWriter writer, string vcToolName, Tool tool, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null, OptionSet customtoolproperties=null)
		{
			WriteToolDelegate writetoolmethod;

			if (_toolMethods.TryGetValue(vcToolName, out writetoolmethod))
			{
				writetoolmethod(writer, vcToolName, tool, module.Configuration, module, nameToDefaultXMLValue, file);
			}

			var toolelements = customtoolproperties.GetOptionDictionary("visual-studio-tool-properties");
			if (toolelements != null)
			{
				foreach (var prop in toolelements)
				{
					writer.WriteElementString(prop.Key, prop.Value);
				}
			}
		}

		protected string GetToolMapping(FileEntry.ConfigFileEntry configentry, out OptionSet toolproperties)
		{
			var toolname = "None";
			toolproperties = null;
			if (configentry != null)
			{
				if (configentry.IsKindOf(FileEntry.Headers))
				{
					toolname = "ClInclude";
				}
				else if (configentry.IsKindOf(FileEntry.NonBuildable))
				{
					toolname = "None";
				}
				else
				{
					OptionSet options = null;
					string vstool = null;
					if (!String.IsNullOrEmpty(configentry.FileItem.OptionSetName) && configentry.Project != null && configentry.Project.NamedOptionSets.TryGetValue(configentry.FileItem.OptionSetName, out options))
					{
						vstool = options.Options["visual-studio-tool"].TrimWhiteSpace();
						toolproperties = options;
					}
					if (!String.IsNullOrEmpty(vstool))
					{
						toolname = vstool;
					}
					else
					{
						var tool = configentry.FileItem.GetTool() ?? configentry.Tool;
						if (tool != null)
						{
							bool found = false;
							var map = GetTools(configentry.Configuration);
							for (int i = 0; i < map.GetLength(0); i++)
							{
								if (tool.ToolName == map[i, 1])
								{
									toolname = map[i, 0];
									found = true;
									break;
								}
							}
							if (!found)
							{
								// Unmapped tools are executed as custom build tools:
								var customtool = tool as BuildTool;
								if (customtool != null)
								{
									toolname = "CustomBuild";
								}
							}
						}
						else
						{
							var ext = Path.GetExtension(configentry.FileItem.Path.Path);
							CustomRulesInfo info;
							if (_custom_rules_extension_mapping.TryGetValue(ext, out info))
							{
								toolname = info.ToolName;
							}
							else if (configentry.IsKindOf(FileEntry.Resources))
							{
								toolname = "None";
							}
							else if (configentry.IsKindOf(FileEntry.Buildable))
							{
								toolname = "ClCompile";
							}
						}
					}
				}
			}
			return toolname;
		}

		private bool IsExcludedFromBuild_Respected(OptionSet options)
		{
			return options.GetBooleanOptionOrDefault("visual-studio-tool-excludedfrombuild-respected", true);
		}

		protected virtual void SetCustomBuildRulesInfo()
		{
			foreach (var module in Modules.Where(m => m is Module_Native).Cast<Module_Native>())
			{
				if (module.Configuration.Compiler == "vc")
				{
					var info = new CustomRulesInfo();
					info.PropFile = @"$(VCTargetsPath)\BuildCustomizations\masm.props";
					info.TargetsFile = @"$(VCTargetsPath)\BuildCustomizations\masm.targets";
					info.Config = module.Configuration;
					_custom_rules_per_file_info.Add(info);
				}
				if (module.CustomBuildRuleFiles != null)
				{
					foreach (var fi in module.CustomBuildRuleFiles.FileItems)
					{
						var cleanRule = CreateRuleFileFromTemplate(fi, module);
						ConvertToMSBuildRule(cleanRule, module);
					}
				}
				if (module.NatvisFiles != null)
				{
					foreach (FileItem item in module.NatvisFiles.FileItems)
					{
						AddResourceWithTool(module, item.Path, "natvis-visual-studio-tool", "Natvis", excludedFromBuildRespected: false, basedir: item.BaseDirectory ?? module.NatvisFiles.BaseDirectory);
					}

					// Visual Studio will automatically add the natvis files to the linker
					// so we need to remove the linker argument so they don't get added twice.
					if (module.Link != null)
					{
						module.Link.Options.RemoveAll(s => s.StartsWith("/NATVIS:", StringComparison.OrdinalIgnoreCase));
					}
				}
			}
		}

		private CustomRulesInfo ConvertToMSBuildRule(string rule_file, IModule module)
		{
			CustomRulesInfo info = null;

			if (String.IsNullOrEmpty(rule_file))
			{
				return info;
			}


			StringBuilder extensions = new StringBuilder();

			if (Path.GetExtension(rule_file) == ".rules")
			{
				//autoconvert
				var fullIntermediateDir = Path.Combine(OutputDir.Path, module.IntermediateDir.Path);
				var outputFileName = Path.Combine(fullIntermediateDir, Path.GetFileName(rule_file));

				Directory.CreateDirectory(fullIntermediateDir);

				var doc = new XmlDocument();
				doc.Load(rule_file);

				using (IXmlWriter propsWriter = new NAnt.Core.Writers.XmlWriter(FilterFileWriterFormat))
				using (IXmlWriter targetsWriter = new NAnt.Core.Writers.XmlWriter(FilterFileWriterFormat))
				using (IXmlWriter schemaWriter = new NAnt.Core.Writers.XmlWriter(FilterFileWriterFormat))
				{
					propsWriter.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnProjectFileUpdate);
					targetsWriter.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnProjectFileUpdate);
					schemaWriter.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnProjectFileUpdate);

					propsWriter.FileName = Path.ChangeExtension(outputFileName, ".props");
					targetsWriter.FileName = Path.ChangeExtension(outputFileName, ".targets");
					schemaWriter.FileName = Path.ChangeExtension(outputFileName, ".xml");

					foreach (XmlNode node in doc.DocumentElement.SelectNodes("./Rules/CustomBuildRule"))
					{
						info = new CustomRulesInfo();

						info.Config = module.Configuration;
						info.ToolName = node.GetAttributeValue("Name");
						info.Extensions = node.GetAttributeValue("FileExtensions");
						info.DisplayName = node.GetAttributeValue("DisplayName");

						info.PropFile = propsWriter.FileName;
						info.TargetsFile = targetsWriter.FileName;
						info.SchemaFile = schemaWriter.FileName;

						GeneratePropFile(node, propsWriter, info);
						GenerateTargetFile(node, targetsWriter, info);
						GenerateSchemaFile(node, schemaWriter, info);

						if (!String.IsNullOrEmpty(info.Extensions))
						{
							extensions.Append(info.Extensions);
							extensions.Append(" ");
						}
					}
				}
			}
			else
			{
				info = new CustomRulesInfo();
				info.Config = module.Configuration;
				info.PropFile = rule_file;
				info.TargetsFile = Path.ChangeExtension(rule_file, ".targets");
				info.SchemaFile = Path.ChangeExtension(rule_file, ".xml");
			}

			_custom_rules_per_file_info.Add(info);

			foreach (string extension in extensions.ToString().ToArray())
			{
				if (!_custom_rules_extension_mapping.ContainsKey(extension))
				{
					_custom_rules_extension_mapping.Add(extension, info);
				}
			}

			return info;
		}

		protected virtual void GeneratePropFile(XmlNode node, IXmlWriter writer, CustomRulesInfo info)
		{
			if (writer != null)
			{
				writer.WriteStartDocument();
				writer.WriteStartElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");

				writer.WriteStartElement("PropertyGroup");
				writer.WriteAttributeString("Condition", String.Format("'$({0}BeforeTargets)' == '' and '$({0}AfterTargets)' == '' and '$(ConfigurationType)' != 'Makefile'", info.ToolName));
				writer.WriteElementString(info.ToolName + "BeforeTargets", "Midl");
				writer.WriteElementString(info.ToolName + "AfterTargets", "CustomBuild");
				writer.WriteEndElement();

				writer.WriteStartElement("PropertyGroup");
				writer.WriteStartElement(info.ToolName + "DependsOn");
				writer.WriteAttributeString("Condition", "'$(ConfigurationType)' != 'Makefile'");
				writer.WriteString("_SelectedFiles;$(" + info.ToolName + "DependsOn)");
				writer.WriteEndElement();
				writer.WriteEndElement();

				writer.WriteStartElement("ItemDefinitionGroup");
				writer.WriteStartElement(info.ToolName);

				foreach (XmlNode properties in node.ChildNodes)
				{
					if (properties.Name == "Properties")
					{
						foreach (XmlNode property in properties.ChildNodes)
						{
							string propertyName = property.GetAttributeValue("Name");
							if (String.IsNullOrEmpty(propertyName)) continue;

							string defaultValue = property.GetAttributeValue("DefaultValue", "NoDefaultValue");
							if (String.Equals(defaultValue, "NoDefaultValue")) continue;

							writer.WriteElementString(propertyName, defaultValue);
						}
					}
				}

				foreach (XmlAttribute attr in node.Attributes)
				{
					if (attr.Name == "Name" ||
						attr.Name == "DisplayName" ||
						attr.Name == "FileExtensions" ||
						attr.Name == "BatchingSeparator")
					{
						continue;
					}

					string newName = attr.Name;
					if (attr.Name == "CommandLine")
					{
						newName = "CommandLineTemplate";
					}
					writer.WriteElementString(newName, attr.Value);
				}

				writer.WriteEndElement();
				writer.WriteEndElement();
				writer.WriteEndElement(); //Project
			}
		}

		protected virtual void GenerateTargetFile(XmlNode node, IXmlWriter writer, CustomRulesInfo info)
		{
			if (writer != null)
			{
				writer.WriteStartDocument();
				using (writer.ScopedElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003"))
				{
					using (writer.ScopedElement("ItemGroup"))
					{
						using (writer.ScopedElement("PropertyPageSchema"))
						{
							writer.WriteAttributeString("Include", info.SchemaFile);
						}

						using (writer.ScopedElement("AvailableItemName"))
						{
							writer.WriteAttributeString("Include", info.ToolName);
							writer.WriteElementString("Targets", "_" + info.ToolName);
						}
					}

					using (writer.ScopedElement("UsingTask"))
					{
						writer.WriteAttributeString("TaskName", info.ToolName);
						writer.WriteAttributeString("TaskFactory", "XamlTaskFactory");
						writer.WriteAttributeString("AssemblyName", "Microsoft.Build.Tasks.v4.0");
						writer.WriteElementString("Task", info.SchemaFile);
					}

					using (writer.ScopedElement("Target"))
					{
						writer.WriteAttributeString("Name", "_" + info.ToolName);
						writer.WriteAttributeString("BeforeTargets", "$(" + info.ToolName + "BeforeTargets)");
						writer.WriteAttributeString("AfterTargets", "$(" + info.ToolName + "AfterTargets)");
						writer.WriteAttributeString("Condition", "'@(" + info.ToolName + ")' != ''");
						writer.WriteAttributeString("DependsOnTargets", "$(" + info.ToolName + "DependsOn);Compute" + info.ToolName + "Output");
						writer.WriteAttributeString("Outputs", "@(" + info.ToolName + "-&gt;Metadata('Outputs')-&gt;Distinct())");
						writer.WriteAttributeString("Inputs", "@(" + info.ToolName + ");%(" + info.ToolName + ".AdditionalDependencies);$(MSBuildProjectFile)");

						using (writer.ScopedElement("ItemGroup"))
						{
							writer.WriteAttributeString("Condition", "'@(SelectedFiles)' != ''");

							using (writer.ScopedElement(info.ToolName))
							{
								writer.WriteAttributeString("Remove", "@(" + info.ToolName + ")");
								writer.WriteAttributeString("Condition", "'%(Identity)' != '@(SelectedFiles)'");
							}
						}

						using (writer.ScopedElement("ItemGroup"))
						{
							using (writer.ScopedElement(info.ToolName + "_tlog"))
							{
								writer.WriteAttributeString("Include", "%(" + info.ToolName + ".Outputs)");
								writer.WriteAttributeString("Condition", "'%(" + info.ToolName + ".Outputs)' != '' and '%(" + info.ToolName + ".ExcludedFromBuild)' != 'true'");
								writer.WriteElementString("Source", "@(" + info.ToolName + ", '|')");
							}
						}

						using (writer.ScopedElement("Message"))
						{
							writer.WriteAttributeString("Importance", "High");
							writer.WriteAttributeString("Text", "%(" + info.ToolName + ".ExecutionDescription)");
						}

						using (writer.ScopedElement("WriteLinesToFile"))
						{
							writer.WriteAttributeString("Condition", "'@(" + info.ToolName + "_tlog)' != '' and '%(" + info.ToolName + "_tlog.ExcludedFromBuild)' != 'true'");
							writer.WriteAttributeString("File", "$(IntDir)$(ProjectName).write.1.tlog");
							writer.WriteAttributeString("Lines", "^%(" + info.ToolName + "_tlog.Source);@(" + info.ToolName + "_tlog-&gt;'%(Fullpath)')");
						}

						using (writer.ScopedElement(info.ToolName))
						{
							writer.WriteAttributeString("Condition", "'@(" + info.ToolName + ")' != '' and '%(" + info.ToolName + ".ExcludedFromBuild)' != 'true'");
							writer.WriteAttributeString("CommandLineTemplate", "%(" + info.ToolName + ".CommandLineTemplate)");
							writer.WriteAttributeString("AdditionalOptions", "%(" + info.ToolName + ".AdditionalOptions)");
							writer.WriteAttributeString("Inputs", "@(" + info.ToolName + ")");

							//Add all custom properties
							foreach (XmlNode properties in node.ChildNodes)
							{
								if (properties.Name == "Properties")
								{
									foreach (XmlNode property in properties.ChildNodes)
									{
										XmlAttribute propertyName = property.Attributes["Name"];
										if (propertyName != null && !String.IsNullOrEmpty(propertyName.Value))
										{
											writer.WriteAttributeString(propertyName.Value, "%(" + info.ToolName + "." + propertyName.Value + ")");
										}
									}
								}
							}
						}
					}

					using (writer.ScopedElement("PropertyGroup"))
					{
						writer.WriteElementString("ComputeLinkInputsTargets", "$(ComputeLinkInputsTargets);" + Environment.NewLine + "Compute" + info.ToolName + "Output;");
						writer.WriteElementString("ComputeLibInputsTargets", "$(ComputeLibInputsTargets);" + Environment.NewLine + "Compute" + info.ToolName + "Output;");
					}

					using (writer.ScopedElement("Target"))
					{
						writer.WriteAttributeString("Name", "Compute" + info.ToolName + "Output");
						writer.WriteAttributeString("Condition", "'@(" + info.ToolName + ")' != ''");
					}

					using (writer.ScopedElement("ItemGroup"))
					{
						using (writer.ScopedElement(info.ToolName + "DirsToMake"))
						{
							writer.WriteAttributeString("Condition", "'@(" + info.ToolName + ")' != '' and '%(" + info.ToolName + ".ExcludedFromBuild)' != 'true'");
							writer.WriteAttributeString("Include", "%(" + info.ToolName + ".Outputs)");
						}

						using (writer.ScopedElement("Link"))
						{
							writer.WriteAttributeString("Include", "%(" + info.ToolName + "DirsToMake.Identity)");
							writer.WriteAttributeString("Condition", "'%(Extension)'=='.obj' or '%(Extension)'=='.res' or '%(Extension)'=='.rsc' or '%(Extension)'=='.lib'");
						}

						using (writer.ScopedElement("Lib"))
						{
							writer.WriteAttributeString("Include", "%(" + info.ToolName + "DirsToMake.Identity)");
							writer.WriteAttributeString("Condition", "'%(Extension)'=='.obj' or '%(Extension)'=='.res' or '%(Extension)'=='.rsc' or '%(Extension)'=='.lib'");
						}

						using (writer.ScopedElement("ImpLib"))
						{
							writer.WriteAttributeString("Include", "%(" + info.ToolName + "DirsToMake.Identity)");
							writer.WriteAttributeString("Condition", "'%(Extension)'=='.obj' or '%(Extension)'=='.res' or '%(Extension)'=='.rsc' or '%(Extension)'=='.lib'");
						}
					}

					using (writer.ScopedElement("MakeDir"))
					{
						writer.WriteAttributeString("Directories", "@(" + info.ToolName + "DirsToMake-&gt;'%(RootDir)%(Directory)')");
					}
				}
			}
		}

		protected virtual void GenerateSchemaFile(XmlNode node, IXmlWriter writer, CustomRulesInfo info)
		{
			if (writer != null)
			{
				writer.WriteStartElement("ProjectSchemaDefinitions", "clr-namespace:Microsoft.Build.Framework.XamlTypes;assembly=Microsoft.Build.Framework");
				writer.WriteAttributeString("xmlns", "x", null, "http://schemas.microsoft.com/winfx/2006/xaml");
				writer.WriteAttributeString("xmlns", "sys", null, "clr-namespace:System;assembly=mscorlib");
				writer.WriteAttributeString("xmlns", "transformCallback", null, "Microsoft.Cpp.Dev10.ConvertPropertyCallback");

				writer.WriteStartElement("Rule");
				writer.WriteAttributeString("Name", info.ToolName);
				writer.WriteAttributeString("PageTemplate", "tool");
				writer.WriteAttributeString("DisplayName", info.DisplayName);
				writer.WriteAttributeString("Order", "200");

				using (writer.ScopedElement("Rule.DataSource"))
				{
					using (writer.ScopedElement("DateSource"))
					{
						writer.WriteAttributeString("Persistence", "ProjectFile");
						writer.WriteAttributeString("ItemType", info.ToolName);
					}
				}

				writer.WriteStartElement("Rule.Categories");
				writer.WriteStartElement("Category");
				writer.WriteAttributeString("Name", "General");
				writer.WriteStartElement("Category.DisplayName");
				writer.WriteElementString("sys", "String", null, "Command Line");
				writer.WriteEndElement(); //Category.DisplayName
				writer.WriteEndElement(); //Category
				writer.WriteEndElement(); //Rule.Categories

				writer.WriteStartElement("StringListProperty");
				writer.WriteAttributeString("Name", "Inputs");
				writer.WriteAttributeString("Category", "Command Line");
				writer.WriteAttributeString("IsRequired", "true");
				writer.WriteAttributeString("Switch", " ");
				writer.WriteStartElement("StringListProperty.DataSource");
				writer.WriteStartElement("DataSource");
				writer.WriteAttributeString("Persistence", "ProjectFile");
				writer.WriteAttributeString("ItemType", info.ToolName);
				writer.WriteAttributeString("SourceType", "Item");
				writer.WriteEndElement(); //DataSource
				writer.WriteEndElement(); //StringListProperty.DataSource
				writer.WriteEndElement(); //StringListProperty

				//traverse properties
				foreach (XmlNode properties in node.ChildNodes)
				{
					if (properties.Name == "Properties")
					{
						foreach (XmlNode property in properties.ChildNodes)
						{
							string propertyName = property.GetAttributeValue("Name");
							if (String.IsNullOrEmpty(propertyName)) continue;

							string readOnly = property.GetAttributeValue("IsReadOnly", "false");
							string delimited = property.GetAttributeValue("Delimited", "false");
							string switchstr = property.GetAttributeValue("Switch");
							string displayName = property.GetAttributeValue("DisplayName", propertyName);
							string separator = ";";

							if (property.Name == "StringProperty")
							{
								if (delimited == "true")
									writer.WriteStartElement("StringListProperty");
								else
									writer.WriteStartElement("StringProperty");

								writer.WriteAttributeString("Name", propertyName);
								writer.WriteAttributeString("ReadOnly", readOnly);
								writer.WriteAttributeString("HelpContext", "0");
								writer.WriteAttributeString("DisplayName", displayName);
								if (delimited == "true")
									writer.WriteAttributeString("Separator", separator);
								writer.WriteAttributeString("Switch", switchstr);
								writer.WriteEndElement();//StringProperty or StringListProperty
							}
						}
					}
				}

				writer.WriteStartElement("StringProperty");
				writer.WriteAttributeString("Name", "CommandLineTemplate");
				writer.WriteAttributeString("DisplayName", "Command Line");
				writer.WriteAttributeString("Visible", "False");
				writer.WriteEndElement();//StringProperty

				writer.WriteStartElement("DynamicEnumProperty");
				writer.WriteAttributeString("Name", String.Format("{0}BeforeTargets", info.ToolName));
				writer.WriteAttributeString("Category", "General");
				writer.WriteAttributeString("EnumProvider", "Targets");
				writer.WriteAttributeString("IncludeInCommandLine", "False");
				writer.WriteStartElement("DynamicEnumProperty.DisplayName");
				writer.WriteElementString("sys", "String", null, "Execute Before");
				writer.WriteEndElement(); //DynamicEnumProperty.DisplayNames
				writer.WriteStartElement("DynamicEnumProperty.Description");
				writer.WriteElementString("sys", "String", null, "Specifies the targets for the build customization to run before.");
				writer.WriteEndElement(); //DynamicEnumProperty.Description
				writer.WriteStartElement("DynamicEnumProperty.ProviderSettings");
				writer.WriteStartElement("NameValuePair");
				writer.WriteAttributeString("Name", "Exclude");
				writer.WriteAttributeString("Value", String.Format("^{0}BeforeTargets|^Compute", info.ToolName));
				writer.WriteEndElement(); //NameValuePair
				writer.WriteEndElement(); //DynamicEnumProperty.ProviderSettings
				writer.WriteStartElement("DynamicEnumProperty.DataSource");
				writer.WriteStartElement("DataSource");
				writer.WriteAttributeString("Persistence", "ProjectFile");
				writer.WriteAttributeString("HasConfigurationCondition", "true");
				writer.WriteEndElement(); //DataSource
				writer.WriteEndElement(); //DynamicEnumProperty.DataSource
				writer.WriteEndElement(); //DynamicEnumProperty

				writer.WriteStartElement("DynamicEnumProperty");
				writer.WriteAttributeString("Name", String.Format("{0}AfterTargets", info.ToolName));
				writer.WriteAttributeString("Category", "General");
				writer.WriteAttributeString("EnumProvider", "Targets");
				writer.WriteAttributeString("IncludeInCommandLine", "False");
				writer.WriteStartElement("DynamicEnumProperty.DisplayName");
				writer.WriteElementString("sys", "String", null, "Execute After");
				writer.WriteEndElement(); //DynamicEnumProperty.DisplayNames
				writer.WriteStartElement("DynamicEnumProperty.Description");
				writer.WriteElementString("sys", "String", null, "Specifies the targets for the build customization to run after.");
				writer.WriteEndElement(); //DynamicEnumProperty.Description
				writer.WriteStartElement("DynamicEnumProperty.ProviderSettings");
				writer.WriteStartElement("NameValuePair");
				writer.WriteAttributeString("Name", "Exclude");
				writer.WriteAttributeString("Value", String.Format("^{0}AfterTargets|^Compute", info.ToolName));
				writer.WriteEndElement(); //NameValuePair
				writer.WriteEndElement(); //DynamicEnumProperty.ProviderSettings
				writer.WriteStartElement("DynamicEnumProperty.DataSource");
				writer.WriteStartElement("DataSource");
				writer.WriteAttributeString("Persistence", "ProjectFile");
				writer.WriteAttributeString("HasConfigurationCondition", "true");
				writer.WriteEndElement(); //DataSource
				writer.WriteEndElement(); //DynamicEnumProperty.DataSource
				writer.WriteEndElement(); //DynamicEnumProperty

				writer.WriteStartElement("StringListProperty");
				writer.WriteAttributeString("Name", "Outputs");
				writer.WriteAttributeString("DisplayName", "Outputs");
				writer.WriteAttributeString("Visible", "False");
				writer.WriteAttributeString("IncludeInCommandLine", "False");
				writer.WriteEndElement();//StringListProperty

				writer.WriteStartElement("StringProperty");
				writer.WriteAttributeString("Name", "ExecutionDescription");
				writer.WriteAttributeString("DisplayName", "Execution Description");
				writer.WriteAttributeString("Visible", "False");
				writer.WriteAttributeString("IncludeInCommandLine", "False");
				writer.WriteEndElement();//StringProperty

				writer.WriteStartElement("StringListProperty");
				writer.WriteAttributeString("Name", "AdditionalDependencies");
				writer.WriteAttributeString("DisplayName", "Additional Dependencies");
				writer.WriteAttributeString("Visible", "False");
				writer.WriteAttributeString("IncludeInCommandLine", "False");
				writer.WriteEndElement();//StringListProperty

				writer.WriteStartElement("StringProperty");
				writer.WriteAttributeString("Subtype", "AdditonalOptions");
				writer.WriteAttributeString("Name", "AdditonalOptions");
				writer.WriteAttributeString("Category", "Command Line");
				writer.WriteStartElement("StringProperty.DisplayName");
				writer.WriteElementString("sys", "String", null, "Additional Options");
				writer.WriteEndElement(); //StringProperty.DisplayName
				writer.WriteStartElement("StringProperty.Description");
				writer.WriteElementString("sys", "String", null, "Additional Options");
				writer.WriteEndElement(); //StringProperty.Description
				writer.WriteEndElement();//StringProperty

				writer.WriteEndElement(); //Rule

				writer.WriteStartElement("ItemType");
				writer.WriteAttributeString("Name", info.ToolName);
				writer.WriteAttributeString("DisplayName", info.DisplayName);
				writer.WriteEndElement(); //ItemType


				writer.WriteStartElement("FileExtension");
				writer.WriteAttributeString("Name", info.Extensions);
				writer.WriteAttributeString("ContentType", info.ToolName);
				writer.WriteEndElement(); //FileExtensions

				writer.WriteStartElement("ContentType");
				writer.WriteAttributeString("Name", info.ToolName);
				writer.WriteAttributeString("DisplayName", info.DisplayName);
				writer.WriteAttributeString("ItemType", info.ToolName);
				writer.WriteEndElement(); //ContentType
				writer.WriteStartElement("ProjectSchemaDefinitions");
			}
		}


		#endregion

		protected void ExecuteExtensions(IModule module, Action<IMCPPVisualStudioExtension> action)
		{
			foreach(var ext in GetExtensionTasks(module as ProcessableModule))
			{
				action(ext);
			}
		}

		protected IEnumerable<IMCPPVisualStudioExtension> GetExtensionTasks(ProcessableModule module)
		{
			if (module == null)
				return Enumerable.Empty<IMCPPVisualStudioExtension>();

			List<string> extensionTaskNames;
			using (var scope = module.Package.Project.Properties.ReadScope())
			{
				// Dynamically created packages may use exising projects from different packages. Skip such projects
				if (module.Configuration.Name != scope["config"])
				{
					return Enumerable.Empty<IMCPPVisualStudioExtension>();
				}

				extensionTaskNames = new List<string>() { module.Configuration.Platform + "-visualstudio-extension" };
				extensionTaskNames.AddRange(scope[module.Configuration.Platform + "-visualstudio-extension"].ToArray());
				extensionTaskNames.AddRange(scope[module.GroupName + ".visualstudio-extension"].ToArray());
			}
			List<IMCPPVisualStudioExtension> extensions = new List<IMCPPVisualStudioExtension>();

			foreach (var extensionTaskName in extensionTaskNames)
			{
				var task = module.Package.Project.TaskFactory.CreateTask(extensionTaskName, module.Package.Project);
				var extension = task as IMCPPVisualStudioExtension;
				if (extension != null)
				{
					extension.Module = module;
					extension.Generator = new IMCPPVisualStudioExtension.MCPPProjectGenerator(this);
					extensions.Add(extension);
				}
				else if (task != null && (task as IDotNetVisualStudioExtension) == null)
				{
					Log.Warning.WriteLine("Visual Studio generator extension Task '{0}' does not implement IMCPPVisualStudioExtension. Task is ignored.", extensionTaskName);
				}
			}
			return extensions;
		}


		#region tool to VCTools mapping

		protected override string[,] GetTools(Configuration config)
		{
			return VCTools;
		}

		protected override void InitFunctionTables()
		{
			//--- Win
			_toolMethods.Add("XMLDataGenerator", WriteXMLDataGeneratorTool);
			_toolMethods.Add("WebServiceProxyGenerator", WriteWebServiceProxyGeneratorTool);
			_toolMethods.Add("FxCompile", WriteFxCompileTool);
			_toolMethods.Add("WavePsslc", WriteWavePsslcTool);
			_toolMethods.Add("Midl", WriteMIDLTool);
			_toolMethods.Add("Masm", WriteAsmCompilerTool);
			_toolMethods.Add("ClCompile", WriteCompilerTool);
			_toolMethods.Add("Lib", WriteLibrarianTool);
			_toolMethods.Add("EmbeddedResource", WriteManagedResourceCompilerTool);
			_toolMethods.Add("ResourceCompile", WriteResourceCompilerTool);
			_toolMethods.Add("Linker", WriteLinkerTool);
			_toolMethods.Add("ALink", WriteALinkTool);
			_toolMethods.Add("Manifest", WriteManifestTool);
			_toolMethods.Add("XDCMake", WriteXDCMakeTool);
			_toolMethods.Add("BscMake", WriteBscMakeTool);
			_toolMethods.Add("FxCop", WriteFxCopTool);
			_toolMethods.Add("AppVerifier", WriteAppVerifierTool);
			_toolMethods.Add("WebDeployment", WriteWebDeploymentTool);
			_toolMethods.Add("CustomBuild", WriteVCCustomBuildTool);
			//--- File no action tools
			_toolMethods.Add("ClInclude", WriteEmptyTool);
			_toolMethods.Add("None", WriteEmptyTool);
		}

		protected static readonly string[,] VCTools = new string[,]
		{ 
			 { "XMLDataGenerator"                 ,"***"  }
			,{ "WebServiceProxyGenerator"         ,"***"  }
			,{ "Midl"                             ,"midl" }
			,{ "Masm"                             ,"asm"  }
			,{ "ClCompile"                        ,"cc"   }
			,{ "EmbeddedResource"                 ,"resx" }
			,{ "ResourceCompile"                  ,"rc"   }
			,{ "Lib"                              ,"lib"  }
			,{ "Linker"                           ,"link" }
			,{ "ALink"                            ,"***"  }
			,{ "Manifest"                         ,"vcmanifest" }
			,{ "XDCMake"                          ,"***"  }
			,{ "BscMake"                          ,"***"  }
			,{ "FxCop"                            ,"***"  }
			,{ "AppVerifier"                      ,"***"  }
			,{ "WebDeployment"                    ,"***"  }
			,{ "Deployment"                       ,"deploy"  }
			,{ "FxCompile"                        ,"fxc"  }
			,{ "WavePsslc"                        ,"psslc"  }
		};

		#endregion

		#region Xml format
		protected override IXmlWriterFormat ProjectFileWriterFormat
		{
			get { return _xmlWriterFormat; }
		}

		protected virtual IXmlWriterFormat FilterFileWriterFormat
		{
			get { return _xmlWriterFormat; }
		}

		private static readonly IXmlWriterFormat _xmlWriterFormat = new XmlFormat(
			identChar: ' ',
			identation: 2,
			newLineOnAttributes: false,
			encoding: new UTF8Encoding(true) // byte order mask!
			);

		#endregion

		private bool IsTegraAndroid(IModule m)
		{
			return m.Configuration.System == "android" && StartupModule.Package.Project.Properties["package.android_config.build-system"] == "tegra-android";
		}
		
		private bool IsVisualStudioAndroid(IModule m)
		{
			return m.Configuration.System == "android" && StartupModule.Package.Project.Properties["package.android_config.build-system"] == "msvs-android";
		}

		private void WriteJavaFiles(IXmlWriter writer)
		{
			var nativeModules = Modules
				.Where(m => m is Module_Native)
				.Cast<Module_Native>();

			foreach (var module in nativeModules)
			{
				if (module.JavaSourceFiles != null)
				{
					var files = new Dictionary<PathString, FileEntry>();

					foreach (var fileitem in module.JavaSourceFiles.FileItems)
					{
						UpdateFileEntry(module, files, fileitem.Path, fileitem, module.JavaSourceFiles.BaseDirectory, module.Configuration);
					}

					var filelist = files
						.Values
						.OrderBy(e => e.Path);

					WriteFilesWithFilters(writer, FileFilters.ComputeFilters(filelist, AllConfigurations, Modules)); // DAVE-FUTURE-REFACTOR-TODO, do we need the "WithFilters" for .androidproj?
				}
			}
		}

		private void WriteAndroidPackagingProject()
		{
			// only do this for msvs-android configurations - GRADLE TODO: this doesn't handle multi platform slns properly
			if (!IsVisualStudioAndroid(StartupModule))
			{
				return;
			}

			// quickly jam in the understanding of java files for our XMLWriter
			_custom_rules_extension_mapping[".java"] = new CustomRulesInfo()
			{
				ToolName = "JavaCompile",
			};

			IModule[] androidPackagingModules = Modules
				.Where
				(
					mod => mod.Configuration.System == "android" &&
					mod.IsKindOf(Module.Program)
				)
				.ToArray();

			if (!androidPackagingModules.Any())
			{
				return;
			}

			// project file
			string packagingProjectFileName = ProjectFileNameWithoutExtension + ".Packaging.androidproj";
			using (IXmlWriter writer = new XmlWriter(ProjectFileWriterFormat))
			{
				writer.CacheFlushed += new CachedWriterEventHandler(OnProjectFileUpdate);
				writer.FileName = Path.Combine(OutputDir.Path, packagingProjectFileName);
				writer.WriteStartDocument();

				AndroidProjectWriter.WriteProject(writer, androidPackagingModules, this, WriteJavaFiles, ".Packaging");
			}

			// android packaging project user files	-- GRADLE-TODO: refactor with regular user file gen
			string userFilePath = Path.Combine(OutputDir.Path, packagingProjectFileName + ".user");
			XmlDocument userFileDoc = new XmlDocument();
			bool corruptUserFule = false;

			try
			{
				if (File.Exists(userFilePath))
				{
					userFileDoc.Load(userFilePath);
				}
			}
			catch (Exception ex)
			{
				Log.Warning.WriteLine("Failed to load existing '.user' file '{0}': {1}. Recreating user file.", UserFileName, ex.Message);
				userFileDoc = new XmlDocument();
				corruptUserFule = true;
			}

			XmlNode userFileEl = userFileDoc.GetOrAddElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");

			userFileEl.SetAttribute("ToolsVersion", ToolsVersion);

			int userdata_count = 0;

			foreach (var module in Modules)
			{
				userdata_count += SetUserFileConfiguration(userFileEl, module as ProcessableModule, (m) => GetAndroidUserData(m));
			}
			if (userdata_count > 0)
			{
				using (MakeWriter writer = new MakeWriter())
				{
					writer.CacheFlushed += new CachedWriterEventHandler(OnProjectFileUpdate);
					writer.FileName = userFilePath;
					writer.Write(userFileDoc.OuterXml);
				}
			}
			else if (corruptUserFule && File.Exists(userFilePath))
			{
				File.Delete(userFilePath);
			}
		}

		private IEnumerable<KeyValuePair<string, string>> GetAndroidUserData(ProcessableModule module)
		{
			// GRADLE-TODO - need to look into multi-so build - we may need to add paths of whole dependency tree
			return new KeyValuePair<string, string>[] { };
		}

		protected class CustomRulesInfo
		{
			public string DisplayName = String.Empty;
			public string ToolName = String.Empty;
			public string PropFile = String.Empty;
			public string TargetsFile = String.Empty;
			public string SchemaFile = String.Empty;
			public string Extensions = String.Empty;
			public Configuration Config = null;
		}

		private readonly List<CustomRulesInfo> _custom_rules_per_file_info = new List<CustomRulesInfo>();
		private readonly IDictionary<string, CustomRulesInfo> _custom_rules_extension_mapping = new Dictionary<string, CustomRulesInfo>(StringComparer.OrdinalIgnoreCase);

		private readonly HashSet<string> _duplicateMPObjFiles = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

		private readonly IDictionary<string, IDictionary<string, string>> _ModuleLinkerNameToXMLValue = new Dictionary<string, IDictionary<string, string>>();
	}
}
