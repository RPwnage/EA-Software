// Copyright (C) 2003-2015 Electronic Arts Inc.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	[TaskName("android-visualstudio-extension")]
	public class AndroidVisualStudioExtension : VisualStudioExtensionBase
	{
		public static string DefaultPlatformToolset = "Clang_5_0";

		public override void ProjectConfiguration(IDictionary<string, string> configurationElements)
		{
			base.ProjectConfiguration(configurationElements);

			// vs doesn't set these at all and so gets default values, we have to explicit
			string fullCompilerPath = Module.Package.Project.Properties["cc"];
			string compilerExe = Path.GetFileName(fullCompilerPath);
			string compilerDir = Path.GetDirectoryName(fullCompilerPath);
			configurationElements.Add("ClangToolExe", compilerExe); // see note about gcc in WriteMsBuildOverrides
			configurationElements.Add("ClangToolPath", compilerDir);

			// Encountered issue with NDK r11c update where VS2015 could not find librarian...
			string fullArPath = Module.Package.Project.Properties["lib"];
			string arExe = Path.GetFileName(fullArPath);
			string arDir = Path.GetDirectoryName(fullArPath);
			configurationElements.Add("ArchiveToolExe", arExe);
			configurationElements.Add("ArchiveToolPath", arDir);

			// ... or strip ...
			string stripDir = arDir;
			string stripPrefix = arExe.Substring(0, arExe.IndexOf("ar.exe"));
			configurationElements.Add("StripToolExe", Path.Combine(stripDir, stripPrefix + "strip.exe"));

			// Android.NDK.props defaults these to 3.4 and 4.8, we need to overide if not using this version // GRADLE-TODO left-over from previous ms vs android integration, probably not needed
			string llvmVersion = Module.Package.Project.Properties["android_llvm_version"];
			string toolchainVersion = Module.Package.Project.Properties["package.AndroidNDK.toolchain-version"];
			configurationElements.Add("LLVMVersion", llvmVersion);
			configurationElements.Add("ToolchainVersion", toolchainVersion);

			// somewhere before vs2015 release these properties were converted to VS_ names - possibly to reduce conflicts?
			// for now set both to be safe
			configurationElements.Add("Android_Home", AndroidSdkDir);
			configurationElements.Add("VS_AndroidHome", AndroidSdkDir);

			string ndkApiLevel = Module.Package.Project.GetPropertyOrFail("package.AndroidNDK.apiVersion");
			configurationElements.Add("AndroidAPILevel", String.Format("android-{0}", ndkApiLevel)); // need api "android-x" string here, not api level "x"    
			configurationElements.Add("Java_Home", JavaHomeDir);
			configurationElements.Add("VS_JavaHome", JavaHomeDir);
			configurationElements.Add("NDKRoot", AndroidNdkDir);
			configurationElements.Add("VS_NdkRoot", AndroidNdkDir);
			configurationElements.Add("ShowAndroidPathsVerbosity", "low"); // avoid some msbuild spam

			configurationElements.Add("UseOfStl", "c++_shared"); // force llvm shared for now, we can change if teams request but it's unlikely
		}

		public override void WriteExtensionItems(IXmlWriter writer)
		{
			// blast away the stl-library names as msbuild seems to be including android_support unilaterally
			using (writer.ScopedElement("PropertyGroup"))
			{
				writer.WriteElementString("StlLibraryName", "");
			}
		}

		public override void ProjectGlobals(IDictionary<string, string> projectGlobals)
		{
			base.ProjectGlobals(projectGlobals);

			// these properties are used to find msbuild targets/properties i.e
			//  $(VCTargetsPath)\Application Type\$(ApplicationType)\$(ApplicationTypeRevision)\Default.props
			projectGlobals["ApplicationType"] = "Android";
			projectGlobals["Keyword"] = "Android";
			projectGlobals["MinimumVisualStudioVersion"] = "14.0";
			projectGlobals["ApplicationTypeRevision"] = "3.0";
			projectGlobals["RootNamespace"] = projectGlobals["ProjectName"];
		}


		public override void WriteExtensionToolProperties(IXmlWriter writer)
		{
			base.WriteExtensionToolProperties(writer);

			// enables task parallelism in MS clang targets
			writer.WriteElementString("UseMultiToolTask", "true");

			// this is based off of FlagEquals behaviour
			bool stripSymbols = false;
			string stripOption = Module.Package.Project.Properties["eaconfig.stripallsymbols"] ?? Module.GetModuleBuildOptionSet().Options["stripallsymbols"];
			if (!String.IsNullOrEmpty(stripOption))
			{
				if (stripOption.Equals("true", StringComparison.OrdinalIgnoreCase) || stripOption.Equals("on", StringComparison.OrdinalIgnoreCase))
				{
					stripSymbols = true;
				}
				else if (stripOption.Equals("false", StringComparison.OrdinalIgnoreCase) || stripOption.Equals("off", StringComparison.OrdinalIgnoreCase))
				{
					stripSymbols = false;
				}
			}
			writer.WriteElementString("PackageDebugSymbols", (!stripSymbols).ToString().ToLowerInvariant()); // GRADLE-TODO: looks like we can always skip this? seems like gradle strips native for us anyway
		}

		// HACK: we need to wrap our linker sources in start and end group
		// for proper linking dependency order unfortunately VS2015 doesn't
		// have a way to do this, so we add these options to linker command
		// line then they have special handing in 
		// config\data\vs_2015_android_options_msbuild.xml
		public override void WriteExtensionProjectProperties(IXmlWriter writer)
		{
			Module_Native nativeModule = Module as Module_Native;
			if (nativeModule != null && nativeModule.Link != null)
			{
				nativeModule.Link.Options.Add("-Wl,--start-group");
				nativeModule.Link.Options.Add("-Wl,--end-group");
			}
		}
		// END HACK

		public override void WriteMsBuildOverrides(IXmlWriter writer)
		{
			// GRADLE-TODO: most of the hacks we're related to older versions of FW or older versions of MS VS Android (we used to use 1.0, now we use 3.0)
			// need to go through each and see if it is actually still needed

			base.WriteMsBuildOverrides(writer);

			// android msbuild targets try to generate recipe file
			// for uility projects which generates a mangled path,
			// adding this blocks msbuild from trying to add it to
			// merged recipe file used in build
			if (Module.IsKindOf(EA.FrameworkTasks.Model.Module.Utility) || Module.IsKindOf(EA.FrameworkTasks.Model.Module.MakeStyle))
			{
				// legacy target (might be able to remove this once vs2015 is released)
				writer.WriteStartElement("Target"); // <Target Name="GetApkRecipeFile"/>
				writer.WriteAttributeString("Name", "GetApkRecipeFile");
				writer.WriteEndElement();

				// new target (pre-RTM)
				writer.WriteStartElement("Target"); // <Target Name="GetRecipeFile"/>
				writer.WriteAttributeString("Name", "GetRecipeFile");
				writer.WriteEndElement();

				// not stricly necessary, but stops Utility projects creating recipe files.
				// Utility project recipe files are never referenced so it doesn't matter
				// if they are created/invalid but it makes validating the build easier if
				// they aren't around
				writer.WriteStartElement("Target"); // <Target Name="GetRecipeFile"/>
				writer.WriteAttributeString("Name", "_CreateApkRecipeFile");
				writer.WriteEndElement();
			}
			else
			{
				// write our own _CreateApkRecipeFile, two differences from MS one 
				//  - DirectDependenciesRecipelistFile expanded to full path - fixes issues with relative paths
				//  - filtering SoPath to files with .so extensions, prevents unnecessary copying of .a files to packaging directory and
				//    allows to get away with library projects that build nothing (side effect of Framework's model and how we handle
				//    "java only" libraries, with better architecture we wouldn't even create these projects)
				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "_CreateApkRecipeFile");
				writer.WriteAttributeString("DependsOnTargets", "$(CommonBuildOnlyTargets);GetNativeTargetPath;_GetObjIntermediatePaths;GetAdditionalLibraries");

				// msbuild target gathers recipe paths from dependent project types
				writer.WriteStartElement("MSBuild");
				writer.WriteAttributeString("Projects", "@(_MSBuildProjectReferenceExistent->WithMetadataValue('ProjectApplicationType', 'Android'))");
				writer.WriteAttributeString("Targets", "GetRecipeFile");
				writer.WriteAttributeString("BuildInParallel", "$(BuildInParallel)");
				writer.WriteAttributeString("Properties", "%(_MSBuildProjectReferenceExistent.SetConfiguration); %(_MSBuildProjectReferenceExistent.SetPlatform)");
				writer.WriteAttributeString("Condition", "'%(_MSBuildProjectReferenceExistent.Extension)' == '.vcxproj' and '@(ProjectReferenceWithConfiguration)' != '' and '@(_MSBuildProjectReferenceExistent)' != '' and %(_MSBuildProjectReferenceExistent.PackageLibrary) != 'False'");
				writer.WriteAttributeString("RebaseOutputs", "true");
				{
					writer.WriteStartElement("Output");
					writer.WriteAttributeString("TaskParameter", "TargetOutputs");
					writer.WriteAttributeString("ItemName", "DirectDependenciesRecipelistFile");
					writer.WriteEndElement();
				}
				writer.WriteEndElement();

				// filter to only .so files
				writer.WriteStartElement("ItemGroup");
				{
					writer.WriteStartElement("FilteredSOs");
					writer.WriteAttributeString("Include", "@(NativeTargetPath)");
					writer.WriteAttributeString("Condition", "'%(Extension)' == '.so'");
					writer.WriteEndElement();

					writer.WriteStartElement("FilteredSOs");
					writer.WriteAttributeString("Include", "@(AdditionalLibrary->'%(Fullpath)')");
					writer.WriteAttributeString("Condition", "'%(Extension)' == '.so'");
					writer.WriteEndElement();
				}
				writer.WriteEndElement();

				// generate recipe file with references to dependent recipes
				writer.WriteStartElement("GenerateApkRecipe");
				writer.WriteAttributeString("SoPaths", "@(FilteredSOs)");
				writer.WriteAttributeString("IntermediateDirs", "@(ObjDirectories)");
				writer.WriteAttributeString("Configuration", "$(Configuration)");
				writer.WriteAttributeString("Platform", "$(Platform)");
				writer.WriteAttributeString("Abi", "$(TargetArchAbi)");
				writer.WriteAttributeString("RecipeFiles", "@(DirectDependenciesRecipelistFile->'%(Fullpath)')");
				writer.WriteAttributeString("OutputFile", "$(_ApkRecipeFile)");
				writer.WriteEndElement();

				// update tlog file with recipe timestamp
				writer.WriteStartElement("WriteLinesToFile");
				writer.WriteAttributeString("File", "$(TLogLocation)$(ProjectName).write.1u.tlog");
				writer.WriteAttributeString("Lines", "^$(ProjectPath);$(_ApkRecipeFile)");
				writer.WriteAttributeString("Encoding", "Unicode");
				writer.WriteEndElement();

				writer.WriteEndElement();
			}


			// write our own GetAdditionalLibraries so we can control if gdbserver.so is added
			using (writer.ScopedElement("Target")
				.Attribute("Name", "GetAdditionalLibraries"))
			{
				using (writer.ScopedElement("ItemGroup"))
				{
					if (Module.IsKindOf(FrameworkTasks.Model.Module.Program) && Module.Package.Project.Properties.GetBooleanPropertyOrDefault(Module.PropGroupName("include-gdb-server"), true)) // TODO cannot be per config defined - targets cannot be conditional
					{
						using (writer.ScopedElement("AdditionalLibrary")
							.Attribute("Include", "$(GdbServerPath)"))
						{ }
					}

					using (writer.ScopedElement("AdditionalLibrary")
						.Attribute("Include", "@(Library)")
						.Attribute("Condition", "'%(Library.ExcludedFromBuild)'!='true' and '%(Library.Extension)' == '.so'"))
					{ }

					using (writer.ScopedElement("AdditionalLibrary")
						.Attribute("Include", "$(StlAdditionalDependencies)"))
					{ }
				}
			}

			if (Module.Tools.Any(tool => tool is Linker))
			{
				// more hacks! when linking a shared object with other shared objects, the typical msbuild
				// cpp build machinery resolved link inputs to full paths, unforuntely this cause the full
				// path to the .so to be embedded in the linked output which cause runtime failures because
				// the path can't be resolved on android

				// so...here we override the Link target in msbuild to do some itemgroup surgery. we strip
				// anything in the Link itemgroup which has extension .so and instead -lSharedObjectFileName
				// -Lpath/to/shared/object by passing them via the appropriate attributes to ClangLink
				using (writer.ScopedElement("Target")
					.Attribute("Name", "Link")
					.Attribute("Condition", "'@(Link)' != ''"))
				{
					using (writer.ScopedElement("ItemGroup"))
					{
						using (writer.ScopedElement("Link"))
						{
							using (writer.ScopedElement("MinimalRebuildFromTracking")
								.Attribute("Condition", "'$(_BuildActionType)' != 'Build' or '$(ForceRebuild)' == 'true'")
								.Value("false"))
							{ }

							writer.WriteElementString("WholeArchiveEnd", "%(Link.WholeArchiveBegin)");
						}
					}

					using (writer.ScopedElement("PropertyGroup"))
					{
						using (writer.ScopedElement("LinkToolArchitecture")
							.Attribute("Condition", "'$(LinkToolArchitecture)' == ''")
							.Value("$(VCToolArchitecture)"))
						{ }

						using (writer.ScopedElement("LinkOutputFile")
							.Attribute("Condition", "'$(LinkOutputFile)' == ''")
							.Value("$(IntDir)$(TargetName)$(TargetExt)"))
						{ }
					}

					// capture all inputs with with extension .so
					using (writer.ScopedElement("ItemGroup"))
					{
						using (writer.ScopedElement("LinkSOs")
							.Attribute("Include", "@(Link)")
							.Attribute("Condition", "%(Extension) == '.so' and '$(TargetPath)' != $([System.IO.Path]::GetFullPath('$(LinkOutputFile)'))"))
						{
							writer.WriteElementString("LinkDirectiveName", "-l$([System.String]::Copy('%(Filename)').Substring(3))");
						}
					}

					// remove all .so inputs from Link item
					using (writer.ScopedElement("ItemGroup"))
					{
						using (writer.ScopedElement("Link")
							.Attribute("Remove", "@(Link)")
							.Attribute("Condition", "%(Extension) == '.so'"))
						{ }
					}

					// transform .so paths to be passed with -Lpath and -lSoFileName
					using (writer.ScopedElement("PropertyGroup"))
					{
						writer.WriteElementString("SOLinkDirectives", "@(LinkSOs->'%(LinkDirectiveName)')");
						writer.WriteElementString("SOLibraryDirectories", "@(LinkSOs->'%(RootDir)%(Directory)')");
					}

					// call ClangLink, but prepend our transformed .so information to AdditionalLibraryDirectories and AdditionalDependencies inputs
					// also we wrap Sources in start group and end group to avoid link order issues
					using (writer.ScopedElement("ClangLink"))
					{
						writer.WriteAttributeString("BuildingInIDE", "$(BuildingInsideVisualStudio)");
						writer.WriteAttributeString("GNUMode", "$(GNUMode)");
						writer.WriteAttributeString("MSVCErrorReport", "$(MSVCErrorReport)");
						writer.WriteAttributeString("Sources", "-Wl,--start-group;@(Link);-Wl,--end-group");
						writer.WriteAttributeString("AdditionalLibraryDirectories", "$(SOLibraryDirectories);%(Link.AdditionalLibraryDirectories)");
						writer.WriteAttributeString("AdditionalOptions", "%(Link.AdditionalOptions)");
						writer.WriteAttributeString("AdditionalDependencies", "$(SOLinkDirectives);%(Link.AdditionalDependencies)");
						writer.WriteAttributeString("FunctionBinding", "%(Link.FunctionBinding)");
						writer.WriteAttributeString("ForceSymbolReferences", "%(Link.ForceSymbolReferences)");
						writer.WriteAttributeString("GenerateMapFile", "%(Link.GenerateMapFile)");
						writer.WriteAttributeString("GccToolChain", "$(GccToolchainPrebuiltPath)");
						writer.WriteAttributeString("IncrementalLink", "%(Link.IncrementalLink)");
						writer.WriteAttributeString("IgnoreSpecificDefaultLibraries", "%(Link.IgnoreSpecificDefaultLibraries)");
						writer.WriteAttributeString("LibraryDependencies", "%(Link.LibraryDependencies)");
						writer.WriteAttributeString("LinkDLL", "%(Link.LinkDLL)");
						writer.WriteAttributeString("NoExecStackRequired", "%(Link.NoExecStackRequired)");
						writer.WriteAttributeString("DebuggerSymbolInformation", "%(Link.DebuggerSymbolInformation)");
						writer.WriteAttributeString("OptimizeForMemory", "%(Link.OptimizeForMemory)");
						writer.WriteAttributeString("OutputFile", "$(LinkOutputFile)");
						writer.WriteAttributeString("Relocation", "%(Link.Relocation)");
						writer.WriteAttributeString("SharedLibrarySearchPath", "%(Link.SharedLibrarySearchPath)");
						writer.WriteAttributeString("ShowProgress", "%(Link.ShowProgress)");
						writer.WriteAttributeString("Sysroot", "$(SysrootLink)");
						writer.WriteAttributeString("TargetArch", "$(ClangTarget)");
						writer.WriteAttributeString("UnresolvedSymbolReferences", "%(Link.UnresolvedSymbolReferences)");
						writer.WriteAttributeString("Version", "%(Link.Version)");
						writer.WriteAttributeString("VerboseOutput", "%(Link.VerboseOutput)");
						writer.WriteAttributeString("WholeArchiveBegin", "%(Link.WholeArchiveBegin)");
						writer.WriteAttributeString("WholeArchiveEnd", "%(Link.WholeArchiveEnd)");
						writer.WriteAttributeString("MinimalRebuildFromTracking", "%(Link.MinimalRebuildFromTracking)");
						writer.WriteAttributeString("TrackFileAccess", "$(TrackFileAccess)");
						writer.WriteAttributeString("TrackerLogDirectory", "$(TLogLocation)");
						writer.WriteAttributeString("TLogReadFiles", "@(LinkTLogReadFiles)");
						writer.WriteAttributeString("TLogWriteFiles", "@(LinkTLogWriteFiles)");
						writer.WriteAttributeString("ToolExe", "$(ClangToolExe)");
						writer.WriteAttributeString("ToolPath", "$(ClangToolPath)");
						writer.WriteAttributeString("ToolArchitecture", "$(LinkToolArchitecture)");
						writer.WriteAttributeString("TrackerFrameworkPath", "$(LinkTrackerFrameworkPath)");
						writer.WriteAttributeString("TrackerSdkPath", "$(LinkTrackerSdkPath)");
						writer.WriteAttributeString("EnableExecuteTool", "$(ClangEnableExecuteTool)");

						using (writer.ScopedElement("Output")
							.Attribute("TaskParameter", "SkippedExecution")
							.Attribute("PropertyName", "LinkSkippedExecution"))
						{ }
					}

					// copy output
					using (writer.ScopedElement("Copy")
						.Attribute("Condition", "'$(LinkSkippedExecution)' != 'true'")
						.Attribute("SourceFiles", "$(LinkOutputFile)")
						.Attribute("DestinationFolder", "$(TargetDir)"))
					{ }

					using (writer.ScopedElement("Message")
						.Attribute("Text", "$(MSBuildProjectFile) -> %(Link.OutputFile)")
						.Attribute("Importance", "High"))
					{ }
				}
			}

			// yet more hacks! linux style toolchain uses ar which as librarian tool. This doesn't have a command line switch
			// to overwrite the library and will always update it in place. This can lead to problems with function being moved from
			// files which are then delete because the archive will still contain the old file. To fix this we write out our own
			// override of the MS Lib task for android but delete the lib before excuting
			if (Module.Tools.Any(tool => tool is Librarian))
			{
				using (writer.ScopedElement("Target")
					.Attribute("Name", "Lib")
					.Attribute("Condition", "'@(Lib)' != ''"))
				{
					using (writer.ScopedElement("PropertyGroup"))
					{
						using (writer.ScopedElement("ArchiveToolExe")
							.Attribute("Condition", "'$(ArchiveToolExe)' == '' and '$(ToolchainPrefix)' != ''")
							.Value("$(ToolchainPrefix)ar.exe"))
						{ }

						using (writer.ScopedElement("LibToolArchitecture")
							.Attribute("Condition", "'$(LibToolArchitecture)' == ''")
							.Value("$(VCToolArchitecture)"))
						{ }
					}

					using (writer.ScopedElement("ItemGroup"))
					{
						using (writer.ScopedElement("Lib"))
						{
							using (writer.ScopedElement("MinimalRebuildFromTracking")
								.Attribute("Condition", "'$(_BuildActionType)' != 'Build' or '$(ForceRebuild)' == 'true'")
								.Value("false"))
							{ }
						}
					}

					using (writer.ScopedElement("Delete")
						.Attribute("Files", "%(Lib.OutputFile)"))
					{ }

					using (writer.ScopedElement("Archive")
						.Attribute("Sources", "@(Lib)")
						.Attribute("Command", "%(Lib.Command)")
						.Attribute("CreateIndex", "%(Lib.CreateIndex)")
						.Attribute("CreateThinArchive", "%(Lib.CreateThinArchive)")
						.Attribute("NoWarnOnCreate", "%(Lib.NoWarnOnCreate)")
						.Attribute("TruncateTimestamp", "%(Lib.TruncateTimestamp)")
						.Attribute("SuppressStartupBanner", "%(Lib.SuppressStartupBanner)")
						.Attribute("Verbose", "%(Lib.Verbose)")
						.Attribute("OutputFile", "%(Lib.OutputFile)")
						.Attribute("TrackFileAccess", "$(TrackFileAccess)")
						.Attribute("TrackerLogDirectory", "$(TLogLocation)")
						.Attribute("MinimalRebuildFromTracking", "%(Lib.MinimalRebuildFromTracking)")
						.Attribute("TLogReadFiles", "@(LibTLogReadFiles)")
						.Attribute("TLogWriteFiles", "@(LibTLogWriteFiles)")
						.Attribute("ToolExe", "$(ArchiveToolExe)")
						.Attribute("ToolPath", "$(ArchiveToolPath)")
						.Attribute("ToolArchitecture", "$(LibToolArchitecture)")
						.Attribute("TrackerFrameworkPath", "$(LibTrackerFrameworkPath)")
						.Attribute("TrackerSdkPath", "$(LibTrackerSdkPath)")
						.Attribute("EnableExecuteTool", "$(LibEnableExecuteTool)"))
					{ }

					using (writer.ScopedElement("Message")
						.Attribute("Text", "$(MSBuildProjectFile) -> %(Lib.OutputFile)")
						.Attribute("Importance", "High"))
					{ }
				}
			}

			if (Module.Package.Project.Properties["config-compiler"] == "gcc")
			{
				// for gcc msbuild stomps our compiler path override
				// so we add pre compile target to stomp their stomp				

				string fullCompilerPath = Module.Package.Project.Properties["cc"];
				string compilerExe = Path.GetFileName(fullCompilerPath);
				string compilerDir = Path.GetDirectoryName(fullCompilerPath);

				using (writer.ScopedElement("Target")
					.Attribute("Name", "PreCompileToolChange")
					.Attribute("BeforeTargets", "ClCompile"))
				{
					using (writer.ScopedElement("PropertyGroup"))
					{
						writer.WriteElementString("ClangToolExe", compilerExe);
						writer.WriteElementString("ClangToolPath", compilerDir);
						writer.WriteElementString("Sysroot", String.Empty);
					}
				}
			}
		}
	}
}
