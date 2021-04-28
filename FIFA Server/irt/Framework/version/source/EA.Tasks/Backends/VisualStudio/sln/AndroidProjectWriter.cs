using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Backends.VisualStudio
{
	// this is a standalone helper class class for writing android project files, it is it's own class to deal
	// with the fact that sometimes we want to write a single project file for a single apk or arr module (which
	// fits the visual studio generator architecture) but also when we want to write an additonal project on
	// a native program module (for msvc android, this requires a native so proj and an android project)

	internal class AndroidProjectWriter
	{
		internal delegate void WriteFiles(IXmlWriter writer);

		internal static void WriteProject(IXmlWriter writer, IEnumerable<IModule> modules, VSProjectBase project, WriteFiles writeFiles = null, string nameSuffix = "")
		{
			IModule startupModule;
			if (project.ConfigurationMap.TryGetValue(project.BuildGenerator.StartupConfiguration, out Configuration startupconfig))
			{
				startupModule = modules.Single(m => m.Configuration == startupconfig) as Module;
			}
			else
			{
				startupModule = modules.First() as Module;
			}

			using (writer.ScopedElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003"))
			{
				// project configurations
				using (writer.ScopedElement("ItemGroup"))
				{
					writer.WriteAttributeString("Label", "ProjectConfigurations");

					foreach (Module module in project.Modules)
					{
						string configName = project.GetVSProjTargetConfiguration(module.Configuration);
						string platform = project.GetProjectTargetPlatform(module.Configuration);
						using (writer.ScopedElement("ProjectConfiguration"))
						{
							writer.WriteAttributeString("Include", configName + "|" + platform);
							writer.WriteElementString("Configuration", configName);
							writer.WriteElementString("Platform", platform);
						}
					}
				}

				// project globals property group
				OrderedDictionary<string, string> projectGlobals = new OrderedDictionary<string, string>
				{
					{ "MinimumVisualStudioVersion", "14.0" },
					{ "ProjectGuid", VSSolutionBase.ToString(Hash.MakeGUIDfromString(project.Name + nameSuffix)) },
					{ "RootNamespace", project.RootNamespace },
					{ "TargetFrameworkVersion", DotNetTargeting.GetNetVersion(startupModule.Package.Project, DotNetFrameworkFamilies.Framework) }, // only used to prevent resolution errors for .net in msbuild, we don't actually need .net but it still gets resolved so point to point the package version (for non proxy)
					{ "AndroidBuildType", "Gradle" },
					{ "_PackagingProjectWithoutNativeComponent", "true" }
				};

				// If package.DotNet.vs-referencedir-override is set, don't let VS try to resolve installed .net sdk and use explicit root
				string referenceAssemblyDir = startupModule.Package.Project.Properties["package.DotNet.vs-referencedir-override"];
				projectGlobals.AddNonEmpty("TargetFrameworkRootPath", referenceAssemblyDir);

				TaskUtil.Dependent(startupModule.Package.Project, "jdk");
				TaskUtil.Dependent(startupModule.Package.Project, "AndroidNDK");
				TaskUtil.Dependent(startupModule.Package.Project, "AndroidSDK");
				string javaDir = PathNormalizer.Normalize(startupModule.Package.Project.Properties.GetPropertyOrFail("package.jdk.home"), true);
				string ndkDir = PathNormalizer.Normalize(startupModule.Package.Project.Properties.GetPropertyOrFail("package.AndroidNDK.appdir"), true);
				string sdkDir = PathNormalizer.Normalize(startupModule.Package.Project.Properties.GetPropertyOrFail("package.AndroidSDK.appdir"), true);

				projectGlobals.Add("Android_Home", sdkDir);
				projectGlobals.Add("VS_AndroidHome", sdkDir);
				projectGlobals.Add("Java_Home", javaDir);
				projectGlobals.Add("VS_JavaHome", javaDir);
				projectGlobals.Add("NDKRoot", ndkDir);
				projectGlobals.Add("VS_NdkRoot", ndkDir);
				projectGlobals.Add("ShowAndroidPathsVerbosity", "low"); // avoid some msbuild spam

				using (writer.ScopedElement("PropertyGroup"))
				{
					writer.WriteAttributeString("Label", "Globals");

					foreach (var entry in projectGlobals)
					{
						string val = entry.Value;
						if (project.BuildGenerator.IsPortable)
							val = project.BuildGenerator.PortableData.NormalizeIfPathString(val, project.OutputDir.Path, quote: false);

						writer.WriteNonEmptyElementString(entry.Key, val);
					}
				}

				// android default props import - msbuild file installed by vs2015
				writer.WriteStartElement("Import");
				writer.WriteAttributeString("Project", "$(AndroidTargetsPath)\\Android.Default.props");
				writer.WriteEndElement(); // Import

				//directories and output
				foreach (IModule module in modules)
				{
					string vsIntDir = project.GetProjectPath(module.GetVSTmpFolder().Path).EnsureTrailingSlash(defaultPath: ".");

					using (writer.ScopedElement("PropertyGroup"))
					{
						writer.WriteAttributeString("Condition", project.GetConfigCondition(module.Configuration));

						// we need the relative paths to include use the "ProjectDir" macro or VS fails to deploy the apk to emulators
						string outDir = project.GetProjectPath(module.OutputDir);
						if (!Path.IsPathRooted(outDir))
							outDir = "$(ProjectDir)\\" + outDir;
						string intDir = project.GetProjectPath(vsIntDir);
						if (!Path.IsPathRooted(intDir))
							intDir = "$(ProjectDir)\\" + intDir;

						writer.WriteElementString("OutDir", outDir);
						writer.WriteElementString("IntDir", intDir);

						string apkSuffix = null;
						if (!module.IsKindOf(Module.Aar) && // Do this only for Apk
							(apkSuffix = module.Package.Project.Properties.GetPropertyOrDefault(module.PropGroupName("android-apk-suffix"), null)) != null)
                            writer.WriteElementString("TargetName", module.Name + apkSuffix);
						else
							writer.WriteElementString("TargetName", module.Name);

						writer.WriteElementString("ConfigurationType", module.IsKindOf(Module.Aar) ? "Library" : "Application");
						writer.WriteElementString("TargetExt", module.IsKindOf(Module.Aar) ? ".aar" : ".apk");

						bool packageNative = module.Package.Project.Properties.GetBooleanPropertyOrDefault(module.PropGroupName("android-package-transitive-native-dependencies"), true, failOnNonBoolValue: true);
						writer.WriteElementString("PackageTransitiveNativeDependencies", packageNative ? "True" : "False");

						// write TargetAndroidABI property, this property is used when writing out recipe files.
						// Normally it is not required (being blank in recipe file) for android project files
						// but because we have dependencies between android project files these recipe files 
						// get merged into APK recipe files. Merging recipe file with a blank ABI property
						// can cause the final recipe file to have a blank ABI. This leads to an unhelpful
						// error when launching from Visual Studio about invalid ABI "".
						string platform = project.GetProjectTargetPlatform(module.Configuration);
						string abi = module.Package.Project.Properties.GetPropertyOrFail("package.AndroidSDK.android_abi");
						writer.WriteElementString("TargetAndroidABI", abi);

						// write AdditionalSymbolSearchPaths so we can load our libraries for debugging
						writer.WriteElementString("AdditionalSymbolSearchPaths", module.OutputDir.Path + ";%(AdditionalSymbolSearchPaths)");
					}
				}

				// configuration settings
				string targetApi = startupModule.Package.Project.GetPropertyOrFail("package.AndroidSDK.targetSDKVersion");
				using (writer.ScopedElement("PropertyGroup"))
				{
					writer.WriteAttributeString("Label", "Configuration");
					writer.WriteElementString("UseDebugLibraries", true); // TODO - figure out exactly what this does
					writer.WriteElementString("AndroidAPILevel", targetApi);
				}

				// android props import - msbuild file installed by vs
				using (writer.ScopedElement("Import"))
				{
					writer.WriteAttributeString("Project", "$(AndroidTargetsPath)\\Android.props");
				}

				// native activty project reference, if we're wrapping a program project
				// then we want to reference
				if (startupModule.IsKindOf(Module.Program))
				{
					using (writer.ScopedElement("ItemGroup"))
					using (writer.ScopedElement("ProjectReference"))
					{
						writer.WriteAttributeString("Include", project.GetProjectPath(Path.Combine(project.OutputDir.Path, project.ProjectFileName)));
						writer.WriteElementString("Project", project.ProjectGuidString);
					}
				}
				else if (project.Dependents.Any())
				{
					using (writer.ScopedElement("ItemGroup"))
					{
						foreach (VSProjectBase reference in project.Dependents)
						{
							using (writer.ScopedElement("ProjectReference"))
							{
								writer.WriteAttributeString("Include", project.GetProjectPath(Path.Combine(reference.OutputDir.Path, reference.ProjectFileName)));
								writer.WriteElementString("Project", reference.ProjectGuidString);
							}
						}
					}
				}

				// generate gradle files if needed
				foreach (IModule module in modules)
				{
					string gradleBuildPath = VSGradleIntegration.WriteGradleIntegrationFiles(module, out List<string> gradleFiles);

					// add generated files to solution explorer
					using (writer.ScopedElement("ItemGroup"))
					{
						foreach (string gradlePath in gradleFiles.Select(s => PathNormalizer.Normalize(s)).OrderBy(s => s))
						{
							string linkPath = null;
							if (PathUtil.IsPathInDirectory(gradlePath, module.ModuleConfigGenDir.NormalizedPath))
							{
								linkPath = PathUtil.RelativePath(gradlePath, module.ModuleConfigGenDir.NormalizedPath);
								linkPath = "GradleGenerated" + Path.DirectorySeparatorChar + module.Configuration.Name + Path.DirectorySeparatorChar + linkPath;
							}
							else if (PathUtil.IsPathInDirectory(gradlePath, module.Package.Dir.NormalizedPath))
							{
								linkPath = PathUtil.RelativePath(gradlePath, module.Package.Dir.NormalizedPath);
							}

							using (writer.ScopedElement("None"))
							{
								writer.WriteAttributeString("Include", project.GetProjectPath(gradlePath));
								writer.WriteAttributeString("Condition", project.GetConfigCondition(module.Configuration));
								if (linkPath != null)
								{
									writer.WriteElementString("Link", linkPath);
								}
							}
						}
					};

					// write any file item group here, implementation comes from caller
					writeFiles?.Invoke(writer);

					// write out additional package libraries (see _CreateApkRecipeFile override below)
					List<string> packageSos = new List<string>();
					foreach (Dependency<IModule> transitiveDep in module.Dependents.FlattenAll()) // we have to take the whole dependency set, regardless of type - we can't be certain when libraries are needed
					{
						TaskUtil.Dependent(module.Package.Project, transitiveDep.Dependent.Package.Name);

						// have to do this because we haven't computed public data for full transitive dependencies,
						// but it's the same logic for finding dlls
						FileSet moduleSharedLibraries = MapDependentBuildOutputs.RemapModuleBinaryPaths
						(
							transitiveDep.Dependent as ProcessableModule,
							module as ProcessableModule, 
							module.Package.Project.GetFileSet("package." + transitiveDep.Dependent.Package.Name + "." + transitiveDep.Dependent.Name + ".dlls"), 
							module.Package.Project.Properties["dll-suffix"]
						).
						SafeClone()
						.AppendIfNotNull(module.Package.Project.GetFileSet("package." + transitiveDep.Dependent.Package.Name + "." + transitiveDep.Dependent.Name + ".dlls" + ".external"));
						moduleSharedLibraries.FailOnMissingFile = false; // includes files created by build, which may not have run

						string dependencyPrimaryOutput = transitiveDep.Dependent.PrimaryOutput();
						IEnumerable<FileItem> secondaryLibraries = moduleSharedLibraries.FileItems.Where(item => !String.Equals(item.Path.NormalizedPath, dependencyPrimaryOutput, PathUtil.PathStringComparison));
						foreach (FileItem item in secondaryLibraries)
						{
							packageSos.Add(project.GetProjectPath(item.Path));
						}
					}
					if (packageSos.Any())
					{
						using (writer.ScopedElement("ItemGroup")
							.Attribute("Condition", project.GetConfigCondition(module.Configuration)))
						{
							foreach (string soProjetPath in packageSos)
							{
								using (writer.ScopedElement("AddtionalPackageSOs")
									.Attribute("Include", soProjetPath))
								{ }
							}
						}
					}

					using (writer.ScopedElement("ItemDefinitionGroup"))
					{
						using (writer.ScopedElement("GradlePackage"))
						{
							writer.WriteAttributeString("Condition", project.GetConfigCondition(module.Configuration));
							writer.WriteElementString("ProjectDirectory", project.GetProjectPath(module.ModuleConfigGenDir.Path));

							TaskUtil.Dependent(module.Package.Project, "GradleWrapper");
							string gradleWPath = Path.Combine(module.Package.Project.Properties.GetPropertyOrFail("package.GradleWrapper.appdir"), "gradlew.bat").PathToWindows();
							writer.WriteElementString("ToolName", project.GetProjectPath(gradleWPath));

							string gradlePlugin = module.Package.Project.Properties.GetPropertyOrFail("package.AndroidSDK.gradle-plugin-version");
							writer.WriteElementString("GradlePlugin", $"gradle:{gradlePlugin}");

							string gradleVersion = module.Package.Project.Properties.GetPropertyOrDefault("package.GradleWrapper.gradle-version", "4.6");
							writer.WriteElementString("GradleVersion", gradleVersion);

							// gradle assemble name
							string configName = module.Package.Project.Properties[module.GroupName + ".vs-config-name"]
								?? module.Package.Project.Properties["eaconfig.vs-config-name"]
								?? module.Package.Project.Properties["config"];

							writer.WriteElementString("AssembleParameter", "assemble" + configName);
							writer.WriteElementString("ProjectName", module.Name);
							writer.WriteElementString("ApkSuffix", "-" + configName);
							writer.WriteElementString("BuildDir", gradleBuildPath);
							writer.WriteElementString("ApkFileName", "%(ProjectName)%(ApkSuffix)$(TargetExt)");
						}
					}
				}

				// android targets import - msbuild file installed by vs
				using (writer.ScopedElement("Import"))
				{
					writer.WriteAttributeString("Project", "$(AndroidTargetsPath)\\Android.targets");
				}

				// overwrite GradlePackage target - MS makes too many assumptions for us to work around cleanly
				// (and slightly different assumptions depending on VS version, 16.7 vs 16.8 had important 
				// differences for exmaple) so we just stomp with our own target
				using (writer.ScopedElement("Target")
					.Attribute("Name", "GradlePackage")
					.Attribute("DependsOnTargets", "$(GradlePackageDependsOn)"))
				{
					using (writer.ScopedElement("PropertyGroup"))
					{
						using (writer.ScopedElement("GradleOutputDir")
							.Attribute("Condition", "'$(TargetExt)' == '.apk'")
							.Value("%(GradlePackage.BuildDir)\\build\\outputs\\apk\\"))
						{ }

						using (writer.ScopedElement("GradleOutputDir")
							.Attribute("Condition", "'$(TargetExt)' == '.aar'")
							.Value("%(GradlePackage.BuildDir)\\build\\outputs\\aar\\"))
						{ }
						writer.WriteElementString("ProjectParameter", "-p \"$(GradleProjectDirectory)\"");
						writer.WriteElementString("WorkingDirectory", "$([MSBuild]::GetDirectoryNameOfFileAbove($(ProjectDir), settings.gradle))");
						writer.WriteElementString("GradleParameters", "$(ProjectParameter) %(GradlePackage.AdditionalOptions) %(GradlePackage.AssembleParameter)");

					}

					using (writer.ScopedElement("Exec")
						.Attribute("Command", "%(GradlePackage.ToolName) $(GradleParameters)")
						.Attribute("WorkingDirectory", "$(WorkingDirectory)"))
					{ }

					using (writer.ScopedElement("Copy")
						.Attribute("SourceFiles", "$(GradleOutputDir)%(GradlePackage.ApkFileName)")
						.Attribute("DestinationFiles", "$(OutDir)$(TargetName)$(TargetExt)")
						.Attribute("SkipUnchangedFiles", "True"))
					{
						using (writer.ScopedElement("Output")
							.Attribute("TaskParameter", "CopiedFiles")
							.Attribute("ItemName", "_copiedFiles"))
						{ }
					}
				}

				// few changes from vanilla MS version:
				// * give ourselves the option to not package dependent transitive .so files - this is useful
				// if we are building .aar hierarchy and we don't want transitive .so files
				// to get packaged - overwrite _CreateApkRecipe and then change how it sets .so files
				// from dependenices when $(PackageTransitiveNativeDependencies) == False
				// * when creating recipe also add in @(AddtionalPackageSOs) to .so recipe
				// this is a non-standard itemgroup created by Framework
				// for packaging prebuilt .so files (particluarly important for dependency chain
				// such as program.apk => library.so =(link='false')=> prebuilt.so, MSBuild has no
				// mechanism for this kind of dependency chain)
				using (writer.ScopedElement("Target")
					.Attribute("Name", "_CreateApkRecipeFile")
					.Attribute("DependsOnTargets", "$(CommonBuildOnlyTargets);_AssignProjectReferencesPlatformType"))
				{
					using (writer.ScopedElement("MSBuild")
						.Attribute("Projects", "@(_MSBuildProjectReferenceExistent->WithMetadataValue('ProjectApplicationType', 'Android'))")
						.Attribute("Targets", "GetRecipeFile")
						.Attribute("BuildInParallel", "$(BuildInParallel)")
						.Attribute("Properties", "%(_MSBuildProjectReferenceExistent.SetConfiguration); %(_MSBuildProjectReferenceExistent.SetPlatform);")
						.Attribute("Condition", "('%(_MSBuildProjectReferenceExistent.Extension)' == '.vcxproj' or '%(_MSBuildProjectReferenceExistent.Extension)' == '.androidproj') and '@(ProjectReferenceWithConfiguration)' != '' and '@(_MSBuildProjectReferenceExistent)' != '' and '$(PackageTransitiveNativeDependencies)' != 'False' and %(_MSBuildProjectReferenceExistent.LinkLibraryDependencies) != 'False'"))
					{
						using (writer.ScopedElement("Output")
							.Attribute("TaskParameter", "TargetOutputs")
							.Attribute("ItemName", "DirectDependenciesRecipelistFile"))
						{ }
					}

					using (writer.ScopedElement("MSBuild")
						.Attribute("Projects", "@(_MSBuildProjectReferenceExistent->WithMetadataValue('ProjectApplicationType', 'Android'))")
						.Attribute("Targets", "GetNativeTargetPath")
						.Attribute("BuildInParallel", "$(BuildInParallel)")
						.Attribute("Properties", "%(_MSBuildProjectReferenceExistent.SetConfiguration); %(_MSBuildProjectReferenceExistent.SetPlatform);")
						.Attribute("Condition", "'%(_MSBuildProjectReferenceExistent.Extension)' == '.vcxproj' and '@(ProjectReferenceWithConfiguration)' != '' and '@(_MSBuildProjectReferenceExistent)' != '' and '$(PackageTransitiveNativeDependencies)' == 'False' and %(_MSBuildProjectReferenceExistent.LinkLibraryDependencies) != 'False'"))
					{
						using (writer.ScopedElement("Output")
							.Attribute("TaskParameter", "TargetOutputs")
							.Attribute("ItemName", "NativeTargetPath"))
						{ }
					}

					using (writer.ScopedElement("PropertyGroup"))
					{
						using (writer.ScopedElement("AndroidLibPath")
							.Attribute("Condition", "'$(TargetExt)' == '.aar'"))
						{
							writer.WriteString("$(OutDir)$(TargetName)$(TargetExt);$([System.IO.Path]::GetFullPath('%(GradlePackage.ProjectDirectory)'))");
						}
						using (writer.ScopedElement("AndroidLibPath")
							.Attribute("Condition", "'$(TargetExt)' == '.jar'"))
						{
							writer.WriteString("$(OutDir)$(TargetName)$(TargetExt);%(AntPackage.WorkingDirectory)");
						}
					}

					using (writer.ScopedElement("GenerateApkRecipe")
						.Attribute("SoPaths", "@(NativeTargetPath);@(AddtionalPackageSOs)")
						.Attribute("LibPaths", "$(AndroidLibPath)")
						.Attribute("IntermediateDirs", "@(ObjDirectories)")
						.Attribute("Configuration", "")
						.Attribute("Platform", "")
						.Attribute("Abi", "$(TargetAndroidABI)")
						.Attribute("RecipeFiles", "@(DirectDependenciesRecipelistFile)")
						.Attribute("OutputFile", "$(_ApkRecipeFile)"))
					{ }

					using (writer.ScopedElement("ItemGroup"))
					{
						using (writer.ScopedElement("FileWrites")
							.Attribute("Include", "$(_ApkRecipeFile)"))
						{ }
					}
				}

				// overwrite clean task to clean external directory
				using (writer.ScopedElement("Target")
					.Attribute("Name", "GradlePackageClean")
					.Attribute("DependsOnTargets", "SetBuildDefaultEnvironmentVariables;SetupGradlePaths"))
				{
					using (writer.ScopedElement("Exec")
						.Attribute("Command", "%(GradlePackage.ToolName) -p $(GradleProjectDirectory) %(GradlePackage.AdditionalOptions) clean"))
					{ }
				}
			}
		}
	}
}
