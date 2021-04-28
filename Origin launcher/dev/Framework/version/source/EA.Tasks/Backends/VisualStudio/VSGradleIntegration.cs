// (c) Electronic Arts. All rights reserved.

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Writers;

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal static class VSGradleIntegration
	{
		private const string LogPrefix = " [visualstudio] ";

		internal static string GetGradleBuildPath(IModule module)
		{
			// The path of the gradle output is sometimes too large so we shorten the path of the output by hashing parts 
			// of the path.  If we don't do this, tasks like stripping fail when building.

			var buildRoot = module.Package.Project.Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT].PathToUnix();
			var pathHash = Hash.GetHashString(module.ModuleConfigBuildDir.Path.PathToUnix().Substring(buildRoot.Length));
			pathHash = Regex.Replace(pathHash, "[^a-zA-Z0-9]+", "", RegexOptions.Compiled);
			var gradleBuildPath = Path.Combine(buildRoot, "Gradle", pathHash).PathToWindows();

			return gradleBuildPath;
		}

		internal static string WriteGradleIntegrationFiles(IModule module, out List<string> gradleIntegrationFiles)
		{
			/* 
				there's 4 cases we need need to consider for interacting with Gradle
				1. Apk, Aar OR Program /w custom gradle file - this is a module with a custom gradle file, we need wire up the top level Gradle build 
				to pull this in with it dependents 
				2. Program /wo custom gradle file - this a pure native module, usually a unit test, that doesn't care about the java / apk side
				// of things so we generate the manifest file and our own gradle file
				3. Make / Utility - we don't support gradle wiring with these, so do nothing
				4. libraries, etc - also nothing to do here
			*/

			gradleIntegrationFiles = new List<string>();

			string gradleBuildPath = GetGradleBuildPath(module);
			if (!Directory.Exists(gradleBuildPath))
			{
				Directory.CreateDirectory(gradleBuildPath);
			}

			if (module.IsKindOf(Module.Apk) || module.IsKindOf(Module.Aar) || (module.IsKindOf(Module.Program) && module.Package.Project.Properties.GetBooleanPropertyOrDefault(module.PropGroupName("has-android-gradle"), false)))
			{
				// in this case we expect a build.gradle to be provided by user - check we actually have one
				string gradleDir = GetGradleDir(module);
				string userGradlePath = Path.Combine(gradleDir, "build.gradle");
				if (!File.Exists(userGradlePath))
				{
					throw new BuildException($"Module {module.Name} missing build.gradle file. Expected file to exist at '{userGradlePath}'.");
				}
				gradleIntegrationFiles.Add(userGradlePath);

				string manifestFile = GetManifestFile(module);
				if (manifestFile != null)
				{
					gradleIntegrationFiles.Add(manifestFile); // TODO not necessarily generated, should handle AndroidManifest outside of Gradle integ code
				}

				// write setting.gradle - this configures projects paths and cache settings
				string gradleSettingsFilePath = WriteGradleSettingsFile(module);
				gradleIntegrationFiles.Add(gradleSettingsFilePath);

				// write gradle.properties - this configures some settings from running gradle (and plugins like android plugin)
				string gradlePropsPath = WriteGradlePropertiesFile(module);
				gradleIntegrationFiles.Add(gradlePropsPath);

				// write framework properties file - this contains a bunch of gradle configuration users can / should reference in their gradle files to configure versions, native lib sources, etc
				string gradleFrameworkPropertiesFile = WriteFrameworkPropertiesGradleFile(module, gradleBuildPath, manifestFile);
				gradleIntegrationFiles.Add(gradleFrameworkPropertiesFile);

				// write build.gradle - this does some global setup for all projects
				string gradleBuildFilePath = Path.Combine(module.ModuleConfigGenDir.Path, "build.gradle");
				gradleIntegrationFiles.Add(gradleBuildFilePath);
				using (GradleWriter gradleBuildWriter = new GradleWriter(GradleWriterFormat.Default))
				{
					// GRADLE-TODO: comments again

					gradleBuildWriter.FileName = gradleBuildFilePath;
					gradleBuildWriter.CacheFlushed += (sender, e) => OnFileUpdate(module.Package.Project.Log, "gradle build file", e);

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
							gradleBuildWriter.WriteDependency("classpath", "'com.android.tools.build:gradle:" + module.Package.Project.Properties.GetPropertyOrFail("package.AndroidSDK.gradle-plugin-version") + "'");
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

					gradleBuildWriter.WriteLine("buildDir = framework_build_dir");
				}
			}
			else if (module.IsKindOf(Module.Program))
			{
				string manifestFile = GetManifestFile(module);
				if (manifestFile != null) // paranoia, GetManifestFile should generate a manifest file for this case (but we aren't checking for native module here)
				{
					gradleIntegrationFiles.Add(manifestFile);
				}

				// write setting.gradle - this configures projects paths and cache settings
				string gradleSettingsFilePath = WriteGradleSettingsFile(module);
				gradleIntegrationFiles.Add(gradleSettingsFilePath);

				// write gradle.properties - this configures some settings from running gradle (and plugins like android plugin)
				string gradlePropsPath = WriteGradlePropertiesFile(module);
				gradleIntegrationFiles.Add(gradlePropsPath);

				// write framework properties file - this contains a bunch of gradle configuration users can / should reference in their gradle files to configure versions, native lib sources, etc
				string gradleFrameworkPropertiesFile = WriteFrameworkPropertiesGradleFile(module, gradleBuildPath, manifestFile);
				gradleIntegrationFiles.Add(gradleFrameworkPropertiesFile);

				// write build.gradle - unlike above case we're not aggregating mutliple projects so write the full android app
				// stragiht into root gradle file (and write no setting.gradle)
				string gradleBuildFilePath = Path.Combine(module.ModuleConfigGenDir.Path, "build.gradle");
				gradleIntegrationFiles.Add(gradleBuildFilePath);

				using (GradleWriter gradleBuildWriter = new GradleWriter(GradleWriterFormat.Default))
				{
					// GRADLE-TODO: comments again

					gradleBuildWriter.FileName = gradleBuildFilePath;
					gradleBuildWriter.CacheFlushed += (sender, e) => OnFileUpdate(module.Package.Project.Log, "gradle build file", e);

					gradleBuildWriter.WriteStartBlock("allprojects");
					{
						gradleBuildWriter.WriteStartBlock("repositories");
						{
							gradleBuildWriter.WriteRepository("google");
							gradleBuildWriter.WriteRepository("jcenter");
						}
						gradleBuildWriter.WriteEndBlock(); // repositories
					}
					gradleBuildWriter.WriteEndBlock(); // allprojects
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
							gradleBuildWriter.WriteDependency("classpath", "'com.android.tools.build:gradle:" + module.Package.Project.Properties.GetPropertyOrFail("package.AndroidSDK.gradle-plugin-version") + "'");
						}
						gradleBuildWriter.WriteEndBlock(); // dependencies
					}
					gradleBuildWriter.WriteEndBlock(); // buildscript

					gradleBuildWriter.WriteLine("apply plugin: \"com.android.application\"");

					// GRADLE-TODO
					// this allows us to mess with output name and folder if needed, but ms will copy from standard gradle output dirs
					// to desired destination so doesn't seem to be needed
					gradleBuildWriter.WriteLine("buildDir = framework_build_dir"); 
					//gradleBuildWriter.WriteLine("archivesBaseName = framework_project_name");

					gradleBuildWriter.WriteStartBlock("android");
					{
						gradleBuildWriter.WriteLine("compileSdkVersion framework_compile_sdk_version");
						gradleBuildWriter.WriteLine("buildToolsVersion framework_build_tools_version");

						gradleBuildWriter.WriteStartBlock("sourceSets.main");
						{
							// gradleBuildWriter.WriteLine("setRoot something"); seems like we can move the root of where relative paths look? might be useful later
							gradleBuildWriter.WriteLine("manifest.srcFile framework_package_manifest_path");
							//gradleBuildWriter.WriteLine("jniLibs.srcDirs framework_jni_libs_dirs"); looks like ms integration copies jni libs to the default location for android relative to this file, so need to set this
						}
						gradleBuildWriter.WriteEndBlock(); // sourceSets.main

						gradleBuildWriter.WriteStartBlock("defaultConfig");
						{
							gradleBuildWriter.WriteLine("minSdkVersion framework_min_sdk_version");
							gradleBuildWriter.WriteLine("targetSdkVersion framework_target_sdk_version");
							gradleBuildWriter.WriteLine("maxSdkVersion framework_max_sdk_version");
						}
						gradleBuildWriter.WriteEndBlock(); // defaultConfig

						// create the config/build type name
						string configName = module.Package.Project.Properties[module.GroupName + ".vs-config-name"]
							?? module.Package.Project.Properties["eaconfig.vs-config-name"]
							?? module.Package.Project.Properties["config"];

						// 'Release' as a gradle target conflicts with the inbuilt 'Release' target, but 'release' doesn't
						configName = configName.ToLower();

						// we can use '-' in our buildtype name, but we should change it to '_' for the applicationIdSuffix
						// because the AndroidManifest can not have a dash in the application-id
						var buildTypeName = "'" + configName + "'";
						var applicationIdSuffix = configName.Replace("-", "_");
						
						// GRADLE-TODO: check if we need this block with ms android support
						// this block is required because tegra will error if the .apk is not created where it expects
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

						gradleBuildWriter.WriteStartBlock("buildTypes");
						{
							gradleBuildWriter.WriteStartBlock(buildTypeName);
							{
								gradleBuildWriter.WriteLine("initWith(buildTypes.debug)");
								gradleBuildWriter.WriteLine("debuggable true");
								gradleBuildWriter.WriteLine("applicationIdSuffix \"." + applicationIdSuffix + "\"");
							}
							gradleBuildWriter.WriteEndBlock(); // configName buildType
						}
						gradleBuildWriter.WriteEndBlock(); // buildTypes
					}
					gradleBuildWriter.WriteEndBlock(); // android
				}
			}
			return gradleBuildPath;
		}

		private static string WriteGradleSettingsFile(IModule module)
		{
			string gradleSettingsFilePath = Path.Combine(module.ModuleConfigGenDir.Path, "settings.gradle");
			using (GradleWriter gradleSettingsWriter = new GradleWriter(GradleWriterFormat.Default))
			{
				// GRADLE-TODO: should probably write some comments out in case user stumbles across this file

				gradleSettingsWriter.FileName = gradleSettingsFilePath;
				gradleSettingsWriter.CacheFlushed += (sender, e) => OnFileUpdate(module.Package.Project.Log, "gradle settings file", e);

				// configure buildcache to use our folder, otherwise it'll dump junk in user's folder and sometimes if multiple
				// build are running in parallel one will lock the other out of the cache causing a timeout - we don't expect
				// the cache to ever really give us any significant speed up between projects
				gradleSettingsWriter.WriteStartBlock("buildCache");
				{
					gradleSettingsWriter.WriteStartBlock("local");
					{
						gradleSettingsWriter.WriteLine($"directory = \"{module.ModuleConfigBuildDir.Path.PathToUnix()}\"");
					}
					gradleSettingsWriter.WriteEndBlock();
				}
				gradleSettingsWriter.WriteEndBlock();


				Dictionary<string, IModule> projectsToModules = new Dictionary<string, IModule>()
					{
						{ GetGradleProject(module), module }
					};

				// find the transitive java dependencies tree and all set up all project paths
				DependentCollection transitiveBuildDeps = module.Dependents.FlattenAll(DependencyTypes.Build);
				foreach (Dependency<IModule> dependency in transitiveBuildDeps)
				{
					if (dependency.Dependent is Module_Java javaDependency)
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

			return gradleSettingsFilePath;
		}

		private static string WriteFrameworkPropertiesGradleFile(IModule module, string gradleBuildPath, string manifestFilePath = null)
		{
			string gradleFrameworkPropertiesFile = Path.Combine(module.ModuleConfigGenDir.Path, "framework_properties.gradle");
			using (GradleWriter frameworkGradleWriter = new GradleWriter(GradleWriterFormat.Default))
			{
				// GRADLE-TODO: comments again

				frameworkGradleWriter.FileName = gradleFrameworkPropertiesFile;
				frameworkGradleWriter.CacheFlushed += (sender, e) => OnFileUpdate(module.Package.Project.Log, "gradle build file", e);

				frameworkGradleWriter.WriteStartBlock("allprojects");
				{
					frameworkGradleWriter.WriteStartBlock("ext");
					{
						// The microsoft android build expects the apk to be output into build/outputs/apk, so the build_dir needs to be called build
						frameworkGradleWriter.WriteLine($"framework_build_dir = \"{gradleBuildPath.PathToUnix()}/build/\"");
						string buildToolsVersion = module.Package.Project.GetPropertyOrFail("package.AndroidSDK.build-tools-version");
						frameworkGradleWriter.WriteLine($"framework_build_tools_version = \"{buildToolsVersion}\"");
						string targetApi = module.Package.Project.GetPropertyOrFail("package.AndroidSDK.targetSDKVersion");
						frameworkGradleWriter.WriteLine($"framework_compile_sdk_version = {targetApi}");
						frameworkGradleWriter.WriteLine($"framework_jni_libs_dirs = [\"{Path.Combine(module.IntermediateDir.Path, "src", "main", "jniLibs").PathToUnix()}\"]");
						string maxApi = module.Package.Project.GetPropertyOrFail("package.AndroidSDK.maximumSDKVersion");
						frameworkGradleWriter.WriteLine($"framework_max_sdk_version = {maxApi}");
						string minApi = module.Package.Project.GetPropertyOrFail("package.AndroidSDK.minimumSDKVersion");
						frameworkGradleWriter.WriteLine($"framework_min_sdk_version = {minApi}");
						frameworkGradleWriter.WriteLine($"framework_project_name = \"{module.Name}\"");
						frameworkGradleWriter.WriteLine($"framework_target_sdk_version = {targetApi}");
						if (manifestFilePath != null)
						{
							frameworkGradleWriter.WriteLine($"framework_package_manifest_path = \"{manifestFilePath.PathToUnix()}\""); // GRADLE-TODO
						}

						frameworkGradleWriter.WriteLine($"nsight_tegra_compile_sdk_version = framework_compile_sdk_version");
						frameworkGradleWriter.WriteLine($"nsight_tegra_build_tools_version = framework_build_tools_version");
						frameworkGradleWriter.WriteLine($"nsight_tegra_build_dir = framework_build_dir");
						frameworkGradleWriter.WriteLine($"nsight_tegra_jni_libs_dirs = framework_jni_libs_dirs");
					}
					frameworkGradleWriter.WriteEndBlock(); // ext
				}
				frameworkGradleWriter.WriteEndBlock(); // allprojects
			}
			return gradleFrameworkPropertiesFile;
		}

		private static string WriteGradlePropertiesFile(IModule module)
		{
			using (MakeWriter gradlePropertiesWriter = new MakeWriter(writeBOM: false))
			{
				string gradlePropertiesFilePath = Path.Combine(module.ModuleConfigGenDir.Path, "gradle.properties");

				gradlePropertiesWriter.FileName = gradlePropertiesFilePath;
				gradlePropertiesWriter.CacheFlushed += (sender, e) => OnFileUpdate(module.Package.Project.Log, "gradle properties file", e);

				// don't auto download SDK components needed for build, use pre-packaged or error - this is important for build determinism
				// and build farms that want to run air gapped
				// Gradle will also ignore out of date version for thing like build-tools and without much obvious output download a more 
				// update to date version which, while a great feature, subverts our uses expecations of what they are using to build
				gradlePropertiesWriter.WriteLine("android.builder.sdkDownload=false");

				string gradleJvmArgs = module.PropGroupValue("gradle-jvm-args", "");
				if (!string.IsNullOrWhiteSpace(gradleJvmArgs))
				{
					gradlePropertiesWriter.WriteLine("org.gradle.jvmargs=" + gradleJvmArgs);
				}

				string additionalGradleProperties = module.PropGroupValue("gradle-properties");
				if (additionalGradleProperties != null)
				{
					foreach (string additional in additionalGradleProperties.LinesToArray())
					{
						gradlePropertiesWriter.WriteLine(additional);
					}
				}

				return gradlePropertiesFilePath;
			}
		}

		// write a default manifest file // TODO: move to android project writer
		private static string WriteDefaultNativeActivityManifest(Module_Native module)
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
				writer.CacheFlushed += (sender, e) => OnFileUpdate(module.Package.Project.Log, "manifest file", e);

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

						writer.WriteAttributeString("android", "minSdkVersion", null, minApi);
						writer.WriteAttributeString("android", "targetSdkVersion", null, targetApi);
					}
					writer.WriteEndElement(); // uses-sdk

					writer.WriteStartElement("application"); // <application android:label="app_name" android:hasCpde="false" android:debuggable="true">
					{
						writer.WriteAttributeString("android", "label", null, safeAppName); // referencing strings resource file
						writer.WriteAttributeString("android", "hasCode", null, "false"); // no java code for native activity
						writer.WriteAttributeString("android", "debuggable", null, "true"); // always allow debugging of unit tests

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

		private static string GetGradleDir(IModule module) // GRADLE-TODO, seems a bit pointless to subvert the Module_Java like this
		{
			return module.PropGroupValue("gradle-directory");
		}

		private static string GetGradleProject(IModule module) // GRADLE-TODO, seems a bit pointless to subvert the Module_Java like this
		{
			return module.PropGroupValue("gradle-project");
		}

		// manifest logic:
		// - if user explicitly gives us a manifest file, take that // GRADLE-TODO no nice syntax for this yet
		// - if there is a manifest at the default gradle location, take that
		// - in the special case of Program getting an automatic packaging project, we generate a manifest
		// - finally return null, this case assumes users have a manual manifest file that there build.gradle
		// point to directly without FW needing to know about it (which means it won't show up in sln) // GRADLE-TODO: should this be warning?
		private static string GetManifestFile(IModule module)
		{
			string manifestFile = module.PropGroupValue("manifest-file", null);
			if (manifestFile != null)
			{
				return manifestFile;
			}
			string gradleDir = GetGradleDir(module);
			string defaultManifestFile = Path.Combine(gradleDir, "src\\main\\AndroidManifest.xml");
			if (File.Exists(defaultManifestFile))
			{
				return defaultManifestFile;
			}
			else if (module is Module_Native nativeModule && module.IsKindOf(Module.Program))
			{
				return WriteDefaultNativeActivityManifest(nativeModule);
			}
			return null;
		}

		private static void OnFileUpdate(Log log, string fileDescription, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string updateMessage = e.IsUpdatingFile ? "    Updating" : "NOT Updating";
				string message = $"{LogPrefix}{updateMessage} {fileDescription} '{Path.GetFileName(e.FileName)}'";
				if (e.IsUpdatingFile)
				{
					log.Minimal.WriteLine(message);
				}
				else
				{
					log.Status.WriteLine(message);
				}
			}
		}
	}
}
