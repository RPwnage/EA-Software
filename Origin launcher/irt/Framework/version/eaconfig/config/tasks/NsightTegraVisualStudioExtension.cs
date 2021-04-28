// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;

using Microsoft.Win32;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Writers;

using EA.Eaconfig;
using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

using Model = EA.FrameworkTasks.Model;

namespace EA.AndroidConfig
{
	// contains steps for additional modifications to the solution that aren't possible through an extension
	[TaskName("nsight-tegra-pregenerate")]
	internal class NsightTegraPreGenerate : Task
	{
		protected override void ExecuteTask()
		{
			// registry key fix up
			{
				TaskUtil.Dependent(Project, "AndroidSDK");
				TaskUtil.Dependent(Project, "AndroidNDK");
				TaskUtil.Dependent(Project, "jdk");
				TaskUtil.Dependent(Project, "GradleWrapper");
				Dictionary<string, string> nvidiaRegKeysToExpected = new Dictionary<string, string>
				{
					{ "sdkRoot", PathNormalizer.Normalize(Project.Properties.GetPropertyOrFail("package.AndroidSDK.appdir")) },
					{ "ndkRoot", PathNormalizer.Normalize(Project.Properties.GetPropertyOrFail("package.AndroidNDK.appdir")) },
					{ "jdkRoot", PathNormalizer.Normalize(Project.Properties.GetPropertyOrFail("package.jdk.appdir")) },
					{ "gradleRoot", PathNormalizer.Normalize(Project.Properties.GetPropertyOrFail("package.GradleWrapper.appdir")) }
				};

				const string forcePropertyName = "package.NsightTegra.force-reg-keys";
				const string registryKeyPath = "Software\\NVIDIA Corporation\\Nsight Tegra\\";
				bool forceRegKeys = Project.Properties.GetBooleanPropertyOrDefault(forcePropertyName, false, failOnNonBoolValue: true);
				RegistryView[] registryViews = new RegistryView[] { RegistryView.Registry32, RegistryView.Registry64 }; // we don't know if generated sln will be built under Visual Studio (32bit) or msbuild (could be either) so set both
				foreach (KeyValuePair<string, string> nvidiaKeyToExpected in nvidiaRegKeysToExpected)
				{
					foreach (RegistryView registryView in registryViews)
					{
						string nvidiaValue = null;
						using (RegistryKey localMachineBaseView = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
						{
							using (RegistryKey registryKey = localMachineBaseView.OpenSubKey(registryKeyPath))
							{
								if (registryKey != null)
								{
									object nvidaValueAsObject = registryKey.GetValue(nvidiaKeyToExpected.Key);
									if (nvidaValueAsObject != null)
									{
										nvidiaValue = nvidaValueAsObject.ToString(); // we're expecting strings here, but if we don't get one .ToString() anyway for error reporting
									}
								}
							}

							if (String.IsNullOrWhiteSpace(nvidiaValue) || !PathNormalizer.Normalize(nvidiaValue).Equals(nvidiaKeyToExpected.Value, PathUtil.PathStringComparison))
							{
								string fullReportingPath = "Computer\\HKEY_LOCAL_MACHINE\\" +
									((registryView == RegistryView.Registry64) ? "WOW6432Node\\" : String.Empty) +
									registryKeyPath +
									nvidiaKeyToExpected.Key;

								if (forceRegKeys)
								{
									try
									{
										using (RegistryKey writeableKey = localMachineBaseView.OpenSubKey(registryKeyPath, writable: true) ?? localMachineBaseView.CreateSubKey(registryKeyPath))
										{
											writeableKey.SetValue(nvidiaKeyToExpected.Key, nvidiaKeyToExpected.Value);
											Log.Minimal.WriteLine("Force updating Nvidia Tegra registry key '" + fullReportingPath + "' to '" + nvidiaKeyToExpected.Value + "'.");
										}
									}
									catch (UnauthorizedAccessException e)
									{
										throw new BuildException
										(
											"Failed to update registry key " + fullReportingPath + "' due to insufficient permissions. " +
											"An attempt was made to automatically update this registry key because property '" + forcePropertyName + "' was set to true.",
											innerException: e
										);
									}
								}
								else
								{
									string reportingDescription = nvidiaValue != null ? "has value '" + nvidiaValue + "'" : "has no value";
									Log.Warning.WriteLine
									(
										"Registry key '" + fullReportingPath + "' " + reportingDescription + ". Expected a path equivalent to '" + nvidiaKeyToExpected.Value + "'. " +
										"Nvidia Tegra is dependent on this value as part of it's build process so please make sure it is set correctly. " +
										"You can set the property '" + forcePropertyName + "' to true and these values be update automatically but this is only recommended in the context of automated builds."
									);
								}
							}
						}
					}
				}
			}

			foreach (Module_Native module in AndroidBuildUtil.GetAllAndroidModules(Project))
			{
				if (module.IsKindOf(Model.Module.Program))
				{
					string configBuildDir = module.Package.Project.GetPropertyValue("package.configbuilddir");
					if (!String.IsNullOrEmpty(configBuildDir))
					{
						Model.BuildStep prebuildStep = new Model.BuildStep("prebuild", BuildStep.PreBuild);

						// Add a prebuild step to set the files in the vstmp folder to be writable
						// tegra will copy files from the NDK which are readonly without changing the attributes
						string vstmpPathPattern = configBuildDir;
						if (module.BuildGroup.ToString() != "runtime")
						{
							vstmpPathPattern = Path.Combine(vstmpPathPattern, module.BuildGroup.ToString());
						}
						vstmpPathPattern = Path.Combine(vstmpPathPattern, module.Name, "*");
						prebuildStep.Commands.Add(new Command("attrib -r " + vstmpPathPattern.Quote() + " /S"));

						// Add a prebuild step to copy the keystore into the vstmp folder
						bool debugFlags = module.Package.Project.NamedOptionSets["config-options-common"].GetBooleanOptionOrDefault("debugflags", false);
						if (!debugFlags)
						{
							string vstmpBuildPath = Path.Combine(configBuildDir, module.Name, "vstmp", "build");
							string keystore = module.Package.Project.GetPropertyValue(module.PropGroupName("android.keystore"));
							if (!String.IsNullOrEmpty(keystore))
							{
								prebuildStep.Commands.Add(new Command("copy /Y " + keystore.Quote() + " " + vstmpBuildPath.Quote()));
							}
						}

						module.BuildSteps.Add(prebuildStep);
					}

					BuildStep postbuildStep = new BuildStep("postbuild", BuildStep.PostBuild);

					// Copy the apk file to expected outputdir so that test-run-fast can find it
					// if the module overrides outputdir it will already be in the right spot
					string outputDir = module.PropGroupValue("outputdir");
					if (outputDir.IsNullOrEmpty())
					{
						string apkDir = AndroidBuildUtil.GetApkOutputDir(module);
						postbuildStep.Commands.Add(new Command("@if not exist " + apkDir.Quote() + " mkdir " + apkDir.Quote()));

						string defaultOutputDir = module.Package.Project.Properties["package.configbindir"];
						if (module.BuildGroup.ToString() != "runtime")
						{
							defaultOutputDir = Path.Combine(defaultOutputDir, module.BuildGroup.ToString());
						}
						string defaultApkPath = Path.Combine(defaultOutputDir, module.Name + ".apk");

						string apkFile = Path.Combine(apkDir, AndroidBuildUtil.GetApkName(module) + ".apk");
						postbuildStep.Commands.Add(new Command("copy /Y " + defaultApkPath.Quote() + " " + apkFile.Quote()));
					}

					module.BuildSteps.Add(postbuildStep);
				}
			}
		}
	}

	[TaskName("android-NsightTegra-visualstudio-extension")]
	public class NsightTegraVisualStudioExtension : VisualStudioExtensionBase
	{
		public override void WriteExtensionProjectProperties(IXmlWriter writer)
		{
			writer.WriteStartElement("PropertyGroup");
			writer.WriteAttributeString("Condition", Generator.GetConfigCondition(Module.Configuration));
			writer.WriteAttributeString("Label", "NsightTegraProject");
			writer.WriteElementString("NsightTegraProjectRevisionNumber", "11");
			writer.WriteEndElement();

			// NSight plugin does not collect libraries from dependent projects:
			Generator.ExplicitlyIncludeDependentLibraries = true;
		}

		public override void ProjectConfiguration(IDictionary<string, string> ConfigurationElements)
		{
			TaskUtil.Dependent(Module.Package.Project, "AndroidSDK");

			if (!IsPortable)
			{
				TaskUtil.Dependent(Module.Package.Project, "jdk");
				string jdkPath = PathNormalizer.Normalize(Module.Package.Project.Properties.GetPropertyOrFail("package.jdk.home"), true);

				ConfigurationElements.Add("SdkRootPath", AndroidSdkDir);
				ConfigurationElements.Add("NdkRootPath", AndroidNdkDir);
				ConfigurationElements.Add("JdkRootPath", jdkPath);
				ConfigurationElements.Add("GradleRootPath", GradleRootDir);

				// just to stop visual studio from complaining the path isn't set, it isn't actually used
				ConfigurationElements.Add("AntRootPath", AntDir);
			}

			// Tegra expects architecture to be named differently or else its msbuild tasks fail
			string architecture = Module.Package.Project.GetPropertyOrFail("package.AndroidNDK.architecture");
			if (architecture == "i686")
			{
				ConfigurationElements.Add("AndroidArch", "x86");
			}
			else if (architecture == "armv8-a")
			{
				ConfigurationElements.Add("AndroidArch", "arm64-v8a");
			}
			else
			{
				ConfigurationElements.Add("AndroidArch", architecture);
			}

			// Android STL Type
			ConfigurationElements.Add("AndroidStlType", "llvm-libc++_shared");

			// android target API
			string targetAPI = Module.Package.Project.GetPropertyOrFail("package.AndroidSDK.targetSDKVersion");
			ConfigurationElements.Add("AndroidTargetAPI", String.Format("android-{0}", targetAPI));

			// android minimum API
			string minApi = Module.Package.Project.GetPropertyOrFail("package.AndroidSDK.minimumSDKVersion");
			ConfigurationElements.Add("AndroidMinAPI", String.Format("android-{0}", minApi));

			// android maximum API
			string maxApi = Module.Package.Project.GetPropertyValue("package.AndroidSDK.maximumSDKVersion");
			if (!String.IsNullOrEmpty(maxApi))
			{
				ConfigurationElements.Add("AndroidMaxAPI", String.Format("android-{0}", maxApi));
			}

			// android ndk API
			string ndkApi = Module.Package.Project.GetPropertyOrFail("package.AndroidNDK.apiVersion");
			ConfigurationElements.Add("AndroidNativeAPI", String.Format("android-{0}", ndkApi));

			// Strip Symbols
			// Strips unneeded info from a shared library. Does not affect the debugging process. NDK build always does this.
			// note: We were setting this off for debug builds but started getting java heap space issues with newer versions of Tegra, and according to docs it doesn't actually affect debugging.
			bool strip_symbols = PropertyUtil.GetBooleanPropertyOrDefault(Module.Package.Project, "package.android_config.stripsymbols", true);
			ConfigurationElements.Add("AndroidStripLibrary", strip_symbols.ToString().ToLowerInvariant());

			// NDK Toolchain version
			var ndkToolchainVersion = Module.Package.Project.GetPropertyValue("package.AndroidNDK.toolchain-version");
			if (Module.Configuration.Compiler == "clang")
			{
				ndkToolchainVersion = "DefaultClang"; // GRADLE_TODO
			}
			ConfigurationElements.Add("NdkToolchainVersion", ndkToolchainVersion);

			// Build System
			ConfigurationElements.Add("AndroidBuildSystem", "GradleBuild");

			DoOnce.Execute("NsightTegra64bitToolChainCheck", () => // GRADLE-TODO: what is going on here?
			{
				// NdkPlatformSuffix defines whether 32 or 64 bit tools are used. Nsight targets always automatically select 64 bit tools if they are present.
				if (Module.Package.Project.GetPropertyValue("package.AndroidNDK.toolchain-platform") != "windows-x86_64")
				{
					if (Directory.Exists(String.Format(@"{0}\prebuilt\windows-x86_64", Module.Package.Project.GetPropertyValue("package.AndroidNDK.appdir"))))
					{
						Log.Warning.WriteLine(Environment.NewLine +
							"    !!!! NsightTegra Visual Studio integration will use 64 bit tools when they are present." + Environment.NewLine +
							"    !!!! If you want to use NsightTegra debugger set nant 'global' property  'package.AndroidNDK.toolchain-platform=windows-x86_64' to configure 64 bit Android tools");
					}
				}
			});
		}

		public override void WriteExtensionTool(IXmlWriter writer)
		{
			writer.WriteStartElement("AntBuild");// confusingly the ant settings apply to gradle when AndroidBuildSystem is set as Gradle
			{
				/* there's three cases we need need to consider for interacting with Gradle
				1. Apk OR Program /w custom gradle file - this is a module with a custom gradle file, we need wire up the top level Gradle build 
				to pull this in with it dependents 
				2. Program /wo custom gradle file - this a pure native module, usually a unit test, that doesn't care about the java / apk side
				// of things so we generate the manifest file and our own gradle file
				3. Make / Utility - Tegra doesn't know whether there have Gradle setup or not so it assumes yes, we operate
				on the opposite assumption so shut down Gradle */

				string antBuildPath = Module.IntermediateDir.Path;
				if (Module.IsKindOf(Model.Module.Apk) || (Module.IsKindOf(Model.Module.Program) && Module.Package.Project.Properties.GetBooleanPropertyOrDefault(Module.PropGroupName("has-android-gradle"), false)))
				{
					DependentCollection transitiveBuildDeps = Module.Dependents.FlattenAll(DependencyTypes.Build);

					// write setting.gradle - this configures projects paths
					using (GradleWriter gradleSettingsWriter = new GradleWriter(GradleWriterFormat.Default))
					{
						// GRADLE-TODO: should probably write some comments out in case user stumbles across this file

						string gradleSettingsFilePath = Path.Combine(Module.Package.PackageConfigGenDir.Path, Module.Name, "settings.gradle");

						gradleSettingsWriter.FileName = gradleSettingsFilePath;
						gradleSettingsWriter.CacheFlushed += OnGradleSetupFileUpdate;

						Dictionary<string, IModule> projectsToModules = new Dictionary<string, IModule>()
						{
							{ GetGradleProject(Module), Module }
						};

						// find the transitive java dependencies tree and all set up all project paths
						foreach (Dependency<IModule> dependency in transitiveBuildDeps)
						{
							Module_Java javaDependency = dependency.Dependent as Module_Java;
							if (javaDependency != null)
							{
								IModule duplicatedProjectModule = null;
								if (projectsToModules.TryGetValue(javaDependency.GradleProject, out duplicatedProjectModule))
								{
									throw new BuildException(String.Format("Module '{0}' uses same Gradle project name '{1}' as module '{2}'.", duplicatedProjectModule.Name, javaDependency.GradleProject, javaDependency.Name));
								}
								projectsToModules.Add(javaDependency.GradleProject, javaDependency);
							}
						}
						foreach (KeyValuePair<string, IModule> projectToModule in projectsToModules.OrderBy(kvp => kvp.Key))
						{
							// GRADLE-TODO: let's help user out and check build.gradle exists in this folder
							gradleSettingsWriter.WriteLine(String.Format("include '{0}'", projectToModule.Key));
							gradleSettingsWriter.WriteLine(String.Format("project (':{0}').projectDir = new File('{1}')", projectToModule.Key, PathUtil.RelativePath(GetGradleDir(projectToModule.Value), Path.GetDirectoryName(gradleSettingsFilePath)).PathToUnix())); // ToUnix avoids escaping forward slashes
						}
					}

					// write gradle.properties - this configures some settings from running gradle (and plugins like android plugin)
					WriteGradlePropertiesFile();

					// write framework properties file - this contains a bunch of gradle configuration users can / should reference in their gradle files to configure versions, native lib sources, etc
					string gradleFrameworkPropertiesFile = WriteFrameworkPropertiesGradleFile();

					// write build.gradle - this does some global setup for all projects
					string gradleBuildFilePath = Path.Combine(Module.Package.PackageConfigGenDir.Path, Module.Name, "build.gradle");
					using (GradleWriter gradleBuildWriter = new GradleWriter(GradleWriterFormat.Default))
					{
						// GRADLE-TODO: comments again

						gradleBuildWriter.FileName = gradleBuildFilePath;
						gradleBuildWriter.CacheFlushed += OnGradleSetupFileUpdate;

						gradleBuildWriter.WriteStartBlock("buildscript");
						{
							gradleBuildWriter.WriteStartBlock("repositories");
							{
								gradleBuildWriter.WriteRepository("google");
								gradleBuildWriter.WriteRepository("jcenter");
							}
							gradleBuildWriter.WriteEndBlock(); // repositories

							// imports gradle properties file, the path needs to be passed to gradle via 
							gradleBuildWriter.WriteLine("apply from: '" + PathUtil.RelativePath(gradleFrameworkPropertiesFile, Path.GetDirectoryName(gradleBuildFilePath)).PathToUnix() + "'");

							gradleBuildWriter.WriteStartBlock("dependencies");
							{
								gradleBuildWriter.WriteDependency("classpath", "'com.android.tools.build:gradle:" + Project.Properties.GetPropertyOrFail("package.AndroidSDK.gradle-plugin-version") + "'");
							}
							gradleBuildWriter.WriteEndBlock(); // dependencies
						}
						gradleBuildWriter.WriteEndBlock(); // buildscript

						gradleBuildWriter.WriteStartBlock("subprojects");
						{
							gradleBuildWriter.WriteStartBlock("repositories");
							{
								gradleBuildWriter.WriteRepository("google");
								gradleBuildWriter.WriteRepository("jcenter");
							}
							gradleBuildWriter.WriteEndBlock(); // repositories
						}
						gradleBuildWriter.WriteEndBlock(); // subprojects
					}

					writer.WriteElementString("AssetsDirectories", GetAssetDirectories());

					writer.WriteElementString("SkipAntStep", "IfUpToDate");
					writer.WriteElementString("AntBuildPath", Generator.Interface.GetProjectPath(antBuildPath));

					writer.WriteElementString("EnableGradleBuildScriptOverride", true);
					writer.WriteElementString("GradleBuildScriptOverridePath", Generator.Interface.GetProjectPath(gradleBuildFilePath));

					// tegra needs to know where the manifest is to get some information out
					writer.WriteElementString("AndroidManifestLocation", Generator.Interface.GetProjectPath(GetManifestFile(Module)));

					// let tegra know which additional .so files need to be packaged, it'll pass this along to gradle
					IEnumerable<string> libNames = null;
					IEnumerable<string> libDirs = null;
					GetPackagingLibraryNameAndDirs(transitiveBuildDeps, antBuildPath, out libNames, out libDirs);
					writer.WriteElementString("NativeLibDirectories", String.Join(";", libDirs));
					writer.WriteElementString("NativeLibDependencies", String.Join(";", libNames));
				}
				else if (Module.IsKindOf(Model.Module.Program))
				{
					// write framework properties file - this contains a bunch of gradle configuration users can / should reference in their gradle files to configure versions, native lib sources, etc
					string gradleFrameworkPropertiesFile = WriteFrameworkPropertiesGradleFile();

					// write build.gradle - unlike above case we're not aggregating mutliple projects so write the full android app
					// stragiht into root gradle file (and write no setting.gradle)
					string gradleBuildFilePath = Path.Combine(Module.Package.PackageConfigGenDir.Path, Module.Name, "build.gradle");
					using (GradleWriter gradleBuildWriter = new GradleWriter(GradleWriterFormat.Default))
					{
						// GRADLE-TODO: comments again

						gradleBuildWriter.FileName = gradleBuildFilePath;
						gradleBuildWriter.CacheFlushed += OnGradleSetupFileUpdate;

						gradleBuildWriter.WriteStartBlock("buildscript");
						{
							gradleBuildWriter.WriteStartBlock("repositories");
							{
								gradleBuildWriter.WriteRepository("google");
								gradleBuildWriter.WriteRepository("jcenter");
							}
							gradleBuildWriter.WriteEndBlock(); // repositories

							// imports gradle properties file, the path needs to be passed to gradle via 
							gradleBuildWriter.WriteLine("apply from: '" + PathUtil.RelativePath(gradleFrameworkPropertiesFile, Path.GetDirectoryName(gradleBuildFilePath)).PathToUnix() + "'");

							gradleBuildWriter.WriteStartBlock("dependencies");
							{
								gradleBuildWriter.WriteDependency("classpath", "'com.android.tools.build:gradle:" + Project.Properties.GetPropertyOrFail("package.AndroidSDK.gradle-plugin-version") + "'");
							}
							gradleBuildWriter.WriteEndBlock(); // dependencies
						}
						gradleBuildWriter.WriteEndBlock(); // buildscript

						gradleBuildWriter.WriteLine("apply plugin: \"com.android.application\"");

						// GRADLE-TODO - these aren't clearly documented in android gradle plugin, play with them to find out what is they do
						gradleBuildWriter.WriteLine("buildDir = nsight_tegra_build_dir");
						gradleBuildWriter.WriteLine("archivesBaseName = nsight_tegra_project_name");

						gradleBuildWriter.WriteStartBlock("android");
						{
							gradleBuildWriter.WriteLine("compileSdkVersion nsight_tegra_compile_sdk_version");
							gradleBuildWriter.WriteLine("buildToolsVersion nsight_tegra_build_tools_version");

							gradleBuildWriter.WriteStartBlock("sourceSets.main");
							{
								gradleBuildWriter.WriteLine("setRoot nsight_tegra_project_dir");
								gradleBuildWriter.WriteStartBlock("if (project.hasProperty('nsight_tegra_package_manifest_path'))"); // GRADLE-TODO, this should always exist?
								{
									gradleBuildWriter.WriteLine("manifest.srcFile nsight_tegra_package_manifest_path");
								}
								gradleBuildWriter.WriteEndBlock(); // if
								gradleBuildWriter.WriteLine("jni.srcDirs = nsight_tegra_jni_src_dirs");
								gradleBuildWriter.WriteLine("jniLibs.srcDirs = nsight_tegra_jni_libs_dirs");
								gradleBuildWriter.WriteLine("java.srcDirs = nsight_tegra_java_src_dirs");
								gradleBuildWriter.WriteLine("assets.srcDirs = nsight_tegra_asset_dirs");
							}
							gradleBuildWriter.WriteEndBlock(); // sourceSets.main

							gradleBuildWriter.WriteStartBlock("defaultConfig"); // GRADLE-TODO - if checks here are copied from tegra's template - I don't think they are necesary though?
							{
								gradleBuildWriter.WriteStartBlock("if (project.hasProperty('nsight_tegra_min_sdk_version'))");
								{
									gradleBuildWriter.WriteLine("minSdkVersion nsight_tegra_min_sdk_version");
								}
								gradleBuildWriter.WriteEndBlock(); // if

								gradleBuildWriter.WriteStartBlock("if (project.hasProperty('nsight_tegra_target_sdk_version'))");
								{
									gradleBuildWriter.WriteLine("targetSdkVersion nsight_tegra_target_sdk_version");
								}
								gradleBuildWriter.WriteEndBlock(); // if

								gradleBuildWriter.WriteStartBlock("if (project.hasProperty('nsight_tegra_max_sdk_version'))");
								{
									gradleBuildWriter.WriteLine("maxSdkVersion nsight_tegra_max_sdk_version");
								}
								gradleBuildWriter.WriteEndBlock(); // if
							}
							gradleBuildWriter.WriteEndBlock(); // defaultConfig

							// this block is required because tegra will error if the .aok is not created where it expects
							// however the output location changed between different versions of android gradle plugin, we're
							// adding this on the *assumption* that we're using a newer plugin that what tegra was written against
							gradleBuildWriter.WriteStartBlock("applicationVariants.all");
							{
								gradleBuildWriter.WriteStartBlock("variant -> variant.outputs.all");
								{
									gradleBuildWriter.WriteLine("outputFileName = \"../\" + outputFileName");
								}
								gradleBuildWriter.WriteEndBlock(); // variant -> variant.outputs.all
							}
							gradleBuildWriter.WriteEndBlock(); // applicationVariants.all

							gradleBuildWriter.WriteStartBlock("lintOptions");
							{
								gradleBuildWriter.WriteLine("checkReleaseBuilds false");
							}
							gradleBuildWriter.WriteEndBlock(); // lintOptions

							// always debug sign apks that use our generated .gradle files - if users hasn't provided
							// a gradle file then this is almost guaranteed to be a test app 
							gradleBuildWriter.WriteStartBlock("buildTypes.all");
							{
								gradleBuildWriter.WriteLine("signingConfig signingConfigs.debug");
							}
							gradleBuildWriter.WriteEndBlock(); // buildTypes.all
						}
						gradleBuildWriter.WriteEndBlock(); // android
					}

					writer.WriteElementString("AssetsDirectories", GetAssetDirectories());

					writer.WriteElementString("SkipAntStep", "IfUpToDate");
					writer.WriteElementString("AntBuildPath", Generator.Interface.GetProjectPath(antBuildPath));

					writer.WriteElementString("EnableGradleBuildScriptOverride", true);
					writer.WriteElementString("GradleBuildScriptOverridePath", Generator.Interface.GetProjectPath(gradleBuildFilePath));

					// if there is no gradle file asume there is no manifest either and generte one
					string manifestFile = WriteDefaultNativeActivityManifest(Module as Module_Native);
					writer.WriteElementString("AndroidManifestLocation", PathUtil.RelativePath(manifestFile, antBuildPath));

					// GRADLE-TODO: what does this do?
					if (!DebugFlags)
					{
						string securePropertiesFile = Module.PropGroupValue("android.secure-properties-location", null);
						if (securePropertiesFile != null)
						{
							writer.WriteElementString("AntBuildType", "Release");
							writer.WriteElementString("SecurePropertiesLocation", Generator.Interface.GetProjectPath(securePropertiesFile));
						}
					}

					IEnumerable<string> libNames = null;
					IEnumerable<string> libDirs = null;
					DependentCollection transitiveBuildDeps = Module.Dependents.FlattenAll(DependencyTypes.Build);
					GetPackagingLibraryNameAndDirs(transitiveBuildDeps, antBuildPath, out libNames, out libDirs);

					writer.WriteElementString("NativeLibDirectories", String.Join(";", libDirs));
					writer.WriteElementString("NativeLibDependencies", String.Join(";", libNames));

					// GRADLE TODO - dll program build?
				}
				else if (Module.IsKindOf(Model.Module.Utility) || Module.IsKindOf(Model.Module.MakeStyle))
				{
					writer.WriteElementString("SkipAntStep", "True");
				}
			}
			writer.WriteEndElement(); // Antbuild
		}

		private string WriteFrameworkPropertiesGradleFile(string manifestFilePath = null)
		{
			string gradleFrameworkPropertiesFile = Path.Combine(Module.ModuleConfigGenDir.Path, "framework_properties.gradle");
			using (GradleWriter frameworkGradleWriter = new GradleWriter(GradleWriterFormat.Default))
			{
				// GRADLE-TODO: comments again

				frameworkGradleWriter.FileName = gradleFrameworkPropertiesFile;

				frameworkGradleWriter.WriteStartBlock("allprojects");
				{
					frameworkGradleWriter.WriteLine("apply from: ext['nsight_tegra_build_properties_file_path']");
					frameworkGradleWriter.WriteStartBlock("ext");
					{
						// The microsoft android build expects the apk to be output into build/outputs/apk, so the build_dir needs to be called build
						frameworkGradleWriter.WriteLine(String.Format("framework_build_dir = \"{0}/build/\"", Module.IntermediateDir.Path.PathToUnix()));
						string buildToolsVersion = Module.Package.Project.GetPropertyOrFail("package.AndroidSDK.build-tools-version");
						frameworkGradleWriter.WriteLine(String.Format("framework_build_tools_version = \"{0}\"", buildToolsVersion));
						string targetApi = Module.Package.Project.GetPropertyOrFail("package.AndroidSDK.targetSDKVersion");
						frameworkGradleWriter.WriteLine(String.Format("framework_compile_sdk_version = {0}", targetApi));
						frameworkGradleWriter.WriteLine(String.Format("framework_jni_libs_dirs = [\"{0}/src/main/jniLibs\"]", Module.ModuleConfigBuildDir.Path.PathToUnix()));
						string maxApi = Module.Package.Project.GetPropertyOrFail("package.AndroidSDK.maximumSDKVersion");
						frameworkGradleWriter.WriteLine(String.Format("framework_max_sdk_version = {0}", maxApi));
						string minApi = Module.Package.Project.GetPropertyOrFail("package.AndroidSDK.minimumSDKVersion");
						frameworkGradleWriter.WriteLine(String.Format("framework_min_sdk_version = {0}", minApi));
						frameworkGradleWriter.WriteLine(String.Format("framework_project_name = \"{0}\"", Module.Name));
						frameworkGradleWriter.WriteLine(String.Format("framework_target_sdk_version = {0}", targetApi));
						if (manifestFilePath != null)
						{
							frameworkGradleWriter.WriteLine(String.Format("framework_package_manifest_path = \"{0}\"", manifestFilePath.PathToUnix())); // GRADLE-TODO
						}

						frameworkGradleWriter.WriteLine("framework_compile_sdk_version = nsight_tegra_compile_sdk_version");
						frameworkGradleWriter.WriteLine("framework_build_tools_version = nsight_tegra_build_tools_version");
						frameworkGradleWriter.WriteLine("framework_build_dir = nsight_tegra_build_dir");
						frameworkGradleWriter.WriteLine("framework_jni_libs_dirs = nsight_tegra_jni_libs_dirs ");
					}
					frameworkGradleWriter.WriteEndBlock(); // ext
				}
				frameworkGradleWriter.WriteEndBlock(); // allprojects
			}
			return gradleFrameworkPropertiesFile;
		}

		private void WriteGradlePropertiesFile()
		{
			using (MakeWriter gradlePropertiesWriter = new MakeWriter(writeBOM: false))
			{
				string gradlePropertiesFilePath = Path.Combine(Module.Package.PackageConfigGenDir.Path, Module.Name, "gradle.properties");

				gradlePropertiesWriter.FileName = gradlePropertiesFilePath;
				gradlePropertiesWriter.CacheFlushed += OnGradleSetupFileUpdate;

				// don't auto download SDK components needed for build, use pre-packaged or error - this is important for build determinism
				// and build farms that want to run air gapped
				// Gradle will also ignore out of date version for thing like build-tools and without much obvious output download a more 
				// update to date version which, while a great feature, subverts our uses expecations of what they are using to build
				gradlePropertiesWriter.WriteLine("android.builder.sdkDownload=false");

				string gradleJvmArgs = Module.PropGroupValue("gradle-jvm-args", "");
				if (!string.IsNullOrWhiteSpace(gradleJvmArgs))
				{
					gradlePropertiesWriter.WriteLine("org.gradle.jvmargs=" + gradleJvmArgs);
				}
			}
		}

		private void GetPackagingLibraryNameAndDirs(DependentCollection transitiveBuildDeps, string antBuildPath, out IEnumerable<string> libNames, out IEnumerable<string> libDirs)
		{
			// write out native libs that need packaging // GRADLE-TODO - this probably isn't good enough since it doesn't take into account
			// .so files that aren't the output of the build...
			Dictionary<string, Module_Native> libNameToModules = new Dictionary<string, Module_Native>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);
			foreach (Dependency<IModule> dependency in transitiveBuildDeps)
			{
				if (dependency.Dependent.IsKindOf(Model.Module.DynamicLibrary) && !dependency.Dependent.IsKindOf(Model.Module.Managed)) // shouldn't ever have a managed module here
				{
					Module_Native nativeDynamicLibrary = dependency.Dependent as Module_Native;
					if (nativeDynamicLibrary != null && nativeDynamicLibrary.Link != null)
					{
						string libName = nativeDynamicLibrary.Link.OutputName + nativeDynamicLibrary.Link.OutputExtension;
						libName = libName.StartsWith("lib") ? libName.Substring("lib".Length) : libName;

						Module_Native duplicatedLibNameModule = null;
						if (libNameToModules.TryGetValue(libName, out duplicatedLibNameModule))
						{
							throw new BuildException(String.Format("Module '{0}' uses same lib name '{1}' as module '{2}'.", duplicatedLibNameModule.Name, libName, nativeDynamicLibrary.Name));
						}
						libNameToModules.Add(libName, nativeDynamicLibrary);

						string outputDir = nativeDynamicLibrary.Link.LinkOutputDir.NormalizedPath;

						libNameToModules[libName] = nativeDynamicLibrary;
					}
				}
			}
			libNames = libNameToModules.Keys
				.OrderBy(s => s);
			libDirs = libNameToModules.Values
				.Select(module => PathUtil.RelativePath(module.Link.LinkOutputDir.NormalizedPath, antBuildPath))
				.Distinct()
				.OrderBy(s => s);

			// ndk requires some additional .sos to be packaged
			foreach (string additionalPackageLibrary in Module.Package.Project.Properties["package.AndroidNDK.additional-package-libraries"].ToArray().OrderBy(s => s))
			{
				string additionalDir = Path.GetDirectoryName(additionalPackageLibrary);
				string additionalLibrary = Path.GetFileName(additionalPackageLibrary);
				additionalLibrary = additionalLibrary.StartsWith("lib") ? additionalLibrary.Substring("lib".Length) : additionalLibrary;

				libDirs = libDirs.Concat(new string[] { PathUtil.RelativePath(additionalDir, antBuildPath) });
				libNames = libNames.Concat(new string[] { additionalLibrary });
			}
		}

		// takes a new line separated list and converts it to a semi-colon separated list
		private string FormatListProperty(string contents)
		{
			string[] list = contents.Split(new char[] { '\n', ';' }, StringSplitOptions.RemoveEmptyEntries);
			return String.Join(";", list.Select(p => p.Trim()).Where(x => !string.IsNullOrEmpty(x)));
		}

		// takes a new line separated list of paths, converts to relative paths in portable solutions makes it a semi-colon separated list
		private string FormatPathListProperty(string contents, string relativeTo)
		{
			string[] list = contents.Split(new char[] { '\n', ';' }, StringSplitOptions.RemoveEmptyEntries);
			if (IsPortable)
			{
				for (int i = 0; i < list.Length; ++i)
				{
					list[i] = PathUtil.RelativePath(list[i], relativeTo);
				}
			}
			return String.Join(";", list.Select(p => p.Trim()).Where(x => !string.IsNullOrEmpty(x)));
		}

		/// <summary>
		/// Add data to the .user file
		/// </summary>
		/// <param name="userData">Dictionary with common user data populated by solution generation</param>
		public override void UserData(IDictionary<string, string> userData)
		{
			if (Module.IsKindOf(Model.Module.Program))
			{
				userData["DebuggerFlavor"] = "AndroidDebugger";
				userData["DebuggerLocation"] = Generator.Interface.GetProjectPath(Module.Package.Project.Properties["package.AndroidNDK.android-gdb"]);

				string gdbSetupPath = Path.GetDirectoryName(WriteGdbSetupFile(Module.IntermediateDir));
				if (!String.IsNullOrEmpty(gdbSetupPath))
				{
					userData["GdbSetupPath"] = Generator.Interface.GetProjectPath(gdbSetupPath);
				}

				if (!IsPortable)
				{
					userData["AntRootPath"] = Generator.Interface.GetProjectPath(AntDir);
				}
			}
		}

		public override void WriteMsBuildOverrides(IXmlWriter writer)
		{
			// classic ar rcs bug where stale object files are not removed from the .a file when building, nuke the library before the lib task
			// to force it to be built from scratch
			if (Module.IsKindOf(Model.Module.Library))
			{
				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Condition", Generator.GetConfigCondition(Module.Configuration));
				writer.WriteAttributeString("Name", "_CleanOldLibraryFile_" + Generator.GetVSProjConfigurationName(Module.Configuration).Replace("|", ""));
				writer.WriteAttributeString("BeforeTargets", "Lib;");

				writer.WriteStartElement("Exec");
				var command = String.Format("if exist {0} del /F /Q {0}", Generator.Interface.GetProjectPath(Module.LibraryOutput()).Quote());
				writer.WriteAttributeString("Command", command);
				writer.WriteEndElement(); // Exec
				writer.WriteEndElement(); // Target
			}

			// hack - tegra build targets assumed this is defined for all module types which is incorrect for use and make
			// it's used to gather shared library paths so we can safely return nothing for these modules
			if (Module.IsKindOf(Model.Module.Utility) || Module.IsKindOf(Model.Module.MakeStyle))
			{
				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "BuildSharedLibrary");
				writer.WriteAttributeString("Outputs", "");
				writer.WriteEndElement();

				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "GetResolvedLinkLibsProxy");
				writer.WriteAttributeString("Condition", Generator.GetConfigCondition(Module.Configuration));
				writer.WriteEndElement();
			}

			if (Module.IsKindOf(Model.Module.Native))
			{
				// tegra tries to figured out default stl libraries for us but fucks it up (adds android_support when it isn't needed for example)
				// stomp their setting and let our own library list deal with it
				using (writer.ScopedElement("PropertyGroup"))
				{
					writer.WriteElementString("StlStaticLibs", String.Empty);
					writer.WriteElementString("StlSharedLibs", String.Empty);
				}

				using (writer.ScopedElement("ItemDefinitionGroup").Attribute("Condition", Generator.GetConfigCondition(Module.Configuration)))
				{
					using (writer.ScopedElement("ClCompile"))
					{
						// precompiled header is being set to default value of stdafx.h for tegra, override the value here
						if (Module.GetModuleBuildOptionSet().GetBooleanOptionOrDefault("use-pch", false))
						{
							var native_module = Module as Module_Native;
							writer.WriteElementString("PrecompiledHeaderFile", native_module.PrecompiledHeaderFile);
							writer.WriteElementString("PrecompiledHeaderOutputDirectory", Path.GetDirectoryName(native_module.PrecompiledFile.Path).EnsureTrailingSlash());
						}

						// tegra sets max parallel compilations to 4 (decided by fair dice roll?), use num processors instead - at worst it'll go too wide
						writer.WriteElementString("ProcessMax", "$(NUMBER_OF_PROCESSORS)");
					}
				}
			}
		}

		private IEnumerable<string> GetSourceDirs()
		{
			List<string> sourcedirs = new List<string>();
			/* GRADLE-TODO
			Module_Native native = Module as Module_Native;

			if (native != null)
			{
				foreach (var fi in native.Cc.SourceFiles.FileItems)
				{
					sourcedirs.Add(Path.GetDirectoryName(fi.Path.Path));
				}
			}

			sourcedirs.Add(PathNormalizer.Normalize(Module.Package.Project.GetPropertyValue("package.AndroidNDK.stdcdir"), false));
			sourcedirs.Add(PathNormalizer.Normalize(Module.Package.Project.ExpandProperties("${package.AndroidNDK.platformdir}/arch-${config-processor}/usr/include"), false));
			*/
			return sourcedirs.OrderedDistinct();
		}

		// GRADLE-TODO: find a better home for this
		// write a default manifest for a native activity
		private string WriteDefaultNativeActivityManifest(Module_Native module)
		{
			string outputDir = module.IntermediateDir.Path;
			if (!Directory.Exists(outputDir))
			{
				Directory.CreateDirectory(outputDir);
			}
			string fileName = Path.Combine(outputDir, "AndroidManifest.xml"); ;
			using (IXmlWriter writer = new XmlWriter(XmlFormat.Default))
			{
				writer.FileName = fileName;
				writer.CacheFlushed += (sender, e) => OnManifestFileUpdate(sender, e);

				/*<manifest xmlns:android="http://schemas.android.com/apk/res/android"
					package="com.$(ApplicationName)"
					android:versionCode="1"
					android:versionName="1.0">*/
				writer.WriteStartElement("manifest");
				{
					string safeAppName = module.Name.Replace('-', 'X').Replace('.', '_');

					writer.WriteAttributeString("xmlns", "android", null, "http://schemas.android.com/apk/res/android");
					writer.WriteAttributeString("package", String.Format("com.ea.{0}", safeAppName));
					writer.WriteAttributeString("android", "versionCode", null, "1");
					writer.WriteAttributeString("android", "versionName", null, "1.0");

					writer.WriteStartElement("uses-sdk"); // <uses-sdk android:minSdkVersion="x" android:targetSdkVersion="x"/>
					{
						string targetApi = module.Package.Project.GetPropertyOrFail("package.AndroidSDK.targetSDKVersion");
						string minApi = module.Package.Project.GetPropertyOrFail("package.AndroidSDK.minimumSDKVersion");

						writer.WriteAttributeString("android", "minSdkVersion", null, targetApi);
						writer.WriteAttributeString("android", "targetSdkVersion", null, minApi);
					}
					writer.WriteEndElement(); // uses-sdk

					writer.WriteStartElement("application"); // <application android:label="app_name" android:hasCpde="false">
					{
						writer.WriteAttributeString("android", "label", null, safeAppName); // referencing strings resource file
																							// writer.WriteAttributeString("android", "debuggable", null, "true"); //GRADLE-TODO: do we need this?
						writer.WriteAttributeString("android", "hasCode", null, "false"); // no java code for native activity

						writer.WriteStartElement("activity"); // <activity android:name="activityname" android:label="app_name">
						{
							writer.WriteAttributeString("android", "name", null, "android.app.NativeActivity");
							writer.WriteAttributeString("android", "label", null, safeAppName); // referencing strings resource file

							writer.WriteStartElement("meta-data"); // <meta-data android:name="android.app.lib_name" android:value="package">
							{
								writer.WriteAttributeString("android", "name", null, "android.app.lib_name");

								// deal with some quirks of unix style name resolution - we need to omit the "lib" at the start of the so name
								string libStr = "lib";
								string outputName = module.Link.OutputName;
								string soName = outputName.StartsWith(libStr) ? outputName.Remove(outputName.IndexOf(libStr), libStr.Length) : outputName;

								writer.WriteAttributeString("android", "value", null, soName);
							}
							writer.WriteEndElement(); // meta-data

							writer.WriteStartElement("intent-filter"); // <intent-filter>
							{
								writer.WriteStartElement("action"); // <action android:name="android.intent.action.MAIN"/>
								{
									writer.WriteAttributeString("android", "name", null, "android.intent.action.MAIN");
								}
								writer.WriteEndElement(); // action

								writer.WriteStartElement("category");// <category android:name="android.intent.category.LAUNCHER"/>
								{
									writer.WriteAttributeString("android", "name", null, "android.intent.category.LAUNCHER");
								}
								writer.WriteEndElement(); // category
							}
							writer.WriteEndElement(); // intent-filter
						}
						writer.WriteEndElement(); // activity
					}
					writer.WriteEndElement(); // application

					// default permissions, mainly so unit tests don't have to write a custom manifest for access basic permissions
					writer.WriteStartElement("uses-permission"); // <uses-permission android:name="android.permission.INTERNET"/>
					{
						writer.WriteAttributeString("android", "name", null, "android.permission.INTERNET");
					}
					writer.WriteEndElement(); // uses-permission

					writer.WriteStartElement("uses-permission"); // <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>
					{
						writer.WriteAttributeString("android", "name", null, "android.permission.WRITE_EXTERNAL_STORAGE");
					}
					writer.WriteEndElement(); // uses-permission
				}
				writer.WriteEndElement(); // manifest
			}
			return fileName;
		}

		private string WriteGdbSetupFile(PathString path)
		{
			string gdbSetupPath = Path.Combine(path.Path, "gdb.setup");
			using (IMakeWriter writer = new MakeWriter(writeBOM: false))
			{
				writer.FileName = gdbSetupPath;
				writer.CacheFlushed += OnGdbSetupFileUpdate;
				writer.WriteLine("# GENERATED FILE");
				writer.WriteLine("set solib-search-path {0}", GetSoLibDirs().ToString(";", dir => dir.PathToUnix().TrimQuotes()));
				writer.WriteLine("directory {0}", GetSourceDirs().ToString(" ", dir => dir.PathToUnix().Quote()));
				writer.WriteLine("# BEGIN USER COMMANDS");
				writer.WriteLine("# END USER COMMANDS");
			}
			return gdbSetupPath;
		}

		private string GetGradleDir(IModule module) // GRADLE-TODO, seems a bit pointless to subvert the Module_Java like this
		{
			return module.PropGroupValue("gradle-directory");
		}

		private string GetGradleProject(IModule module) // GRADLE-TODO, seems a bit pointless to subvert the Module_Java like this
		{
			return module.PropGroupValue("gradle-project");
		}

		private string GetManifestFile(IModule module) //GRADLE-TODO, seems a bit pointless to subvert the Module_Java like this
		{
			return module.PropGroupValue("manifest-file");
		}

		private void OnManifestFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = String.Format("{0}{1} manifest file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
				{
					Log.Minimal.WriteLine(message);
				}
				else
				{
					Log.Status.WriteLine(message);
				}
			}
		}

		// GRADLE-TODO: stop repeating this code block
		private void OnGdbSetupFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = String.Format("{0}{1} gdb setup file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
				{
					Log.Minimal.WriteLine(message);
				}
				else
				{
					Log.Status.WriteLine(message);
				}
			}
		}

		// GRADLE-TODO: like seriously
		private void OnGradleSetupFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = String.Format("{0}{1} gradle file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
				{
					Log.Minimal.WriteLine(message);
				}
				else
				{
					Log.Status.WriteLine(message);
				}
			}
		}

		// GRADLE-TODO: there is probably a much better way to do this
		private string GetAssetDirectories()
		{
			var assetDirs = Module.Assets.FileItems.ConvertAll(asset => Path.GetDirectoryName(asset.Path.Path).ToString() + "/..").Distinct();
			return string.Join(";", assetDirs);
		}
	}
}
