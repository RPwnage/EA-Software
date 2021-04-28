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
    [TaskName("vs2015-android-extension")]
	public class VS2015AndroidExtension : VisualStudioExtensionBase
    {
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

			// Android.NDK.props defaults these to 3.4 and 4.8, we need to overide if not using this version
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
            // gather all dependent modules of same configuration from dependent projects
            IEnumerable<Module_Native> nativeProjectModules = Generator.Interface.Dependents
                .Select(generator => generator.Modules.FirstOrDefault(genModule => genModule.Configuration == Module.Configuration))
                .OfType<Module_Native>();

            // build up a list of dlls produced by other projects
            List<string> pathsFromDependentProjects = new List<string>();
            foreach (Module_Native nativeModule in nativeProjectModules)
            {
                if (nativeModule.Tools != null)
                {
                    Linker link = nativeModule.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;
                    if (link != null)
                    {
                        pathsFromDependentProjects.Add(PathNormalizer.Normalize(Path.Combine(link.LinkOutputDir.Path, link.OutputName + link.OutputExtension)));
                    }
                }
            }

            // gather all dependent modules
            IEnumerable<IModule> dependencyModules = Module.Dependents
                .Where(dep => dep.IsKindOf(DependencyTypes.Link))
                .Select(dep => dep.Dependent)
                .Where(module => module is Module_Native || module is Module_UseDependency);

            // build up list of publically export dll files from all depends
            IEnumerable<string> publicDllsFromDependents = dependencyModules
                .Select(module => module.Public(Module))
                .OfType<IPublicData>()
                .Select(pd => pd.DynamicLibraries)
                .SelectMany((FileSet fs) => fs.FileItems)
                .Select((FileItem fi) => PathNormalizer.Normalize(fi.FullPath));

            // finally make a list of dlls that are exported by dependents but not produced by other VS projects
            IEnumerable<string> unhandledDlls = publicDllsFromDependents.Except(pathsFromDependentProjects);

            // add Library item group to ensure these files are added to apk
            if (unhandledDlls.Any())
            {
                writer.WriteStartElement("ItemGroup");
                writer.WriteAttributeString("Condition", Generator.GetConfigCondition(Module.Configuration));
                foreach (string item in unhandledDlls)
                {
                    writer.WriteStartElement("Library");
                    writer.WriteAttributeString("Include", item);
                    writer.WriteEndElement(); // Library
                }
                writer.WriteEndElement(); // ItemGroup
            }

			// GRADLE-TODO
            // msbuild for android contains various parameters for stl settings but we have already deduced the path we want
            // and handled the stl include dirs so just stomp everything msbuild tries to do automatically
            /*char[] whiteSpace = null;
            IEnumerable<string> androidStdCLibraryPaths = Module.Package.Project.Properties["package.AndroidNDK.stdclibs"]
				.Split(whiteSpace, StringSplitOptions.RemoveEmptyEntries) // null splits on whitespace
				.Select(lib => lib.Replace("\"", "").Replace("/","\\"));
			writer.WriteStartElement("PropertyGroup");
			writer.WriteElementString("StlIncludeDirectories", ""); // stomp include directories, we handle this more generically
			writer.WriteElementString("StlAdditionalDependencies", String.Join(";", androidStdCLibraryPaths));
			writer.WriteElementString("StlLibraryPath", String.Join(";", androidStdCLibraryPaths.Select(p => Path.GetDirectoryName(p))));
			writer.WriteElementString("StlLibraryName", String.Join(";", androidStdCLibraryPaths.Select(p => Path.GetFileName(p))));
			writer.WriteEndElement();*/
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
        }


		public override void WriteExtensionToolProperties(IXmlWriter writer)
		{
 			base.WriteExtensionToolProperties(writer);
            bool packageSymbols = !(Module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.android_config.stripsymbols", true)); // GRADLE-TODO: there's a optionset thing for stripping
            writer.WriteElementString("PackageDebugSymbols", packageSymbols.ToString().ToLowerInvariant());
            // TODO: Fix this
			//writer.WriteElementString("UseMultiToolTask", "true"); // enables task parallelism in MS clang targets           
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

                // not stricly necessary, but stops Utility projects creating recipe files.
                // Utility project recipe files are never referenced so it doesn't matter
                // if they are created/invalid but it makes validating the build easier if
                // they aren't around
                writer.WriteEndElement();
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
                writer.WriteAttributeString("Condition", "'%(_MSBuildProjectReferenceExistent.Extension)' == '.vcxproj' and '@(ProjectReferenceWithConfiguration)' != '' and '@(_MSBuildProjectReferenceExistent)' != '' and %(_MSBuildProjectReferenceExistent.LinkLibraryDependencies) != 'False'");
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
            writer.WriteStartElement("Target");
            writer.WriteAttributeString("Name", "GetAdditionalLibraries");
            {
                writer.WriteStartElement("ItemGroup");            
                {
                    if (Module.IsKindOf(FrameworkTasks.Model.Module.Program) && Module.Package.Project.Properties.GetBooleanPropertyOrDefault(Module.PropGroupName("include-gdb-server"), true)) // TODO cannot be per config defined - targets cannot be conditional
                    {
                        writer.WriteStartElement("AdditionalLibrary");
                        writer.WriteAttributeString("Include", "$(GdbServerPath)");
                        writer.WriteEndElement(); // AdditionalLibrary
                    }

                    writer.WriteStartElement("AdditionalLibrary");
                    writer.WriteAttributeString("Include", "@(Library)");
                    writer.WriteAttributeString("Condition", "'%(Library.ExcludedFromBuild)'!='true' and '%(Library.Extension)' == '.so'");
                    writer.WriteEndElement(); // AdditionalLibrary

                    writer.WriteStartElement("AdditionalLibrary");
                    writer.WriteAttributeString("Include", "$(StlAdditionalDependencies)");
                    writer.WriteEndElement(); // AdditionalLibrary
                }
                writer.WriteEndElement();
            }
            writer.WriteEndElement();

			/* GRADLE-TODO:
			if (Module.Package.Project.Properties["config-compiler"] == "clang")
			{
				// by default vs2015 uses clang as compiler and linker, however
				// for some reason when it calls ld to do link it passes all arguments 
				// on command line - for solution with many projects this bursts command
				// line limit so here we re-write some properties thank hijack the default
				// link task to use the linker we use for nant builds which doesn't have
				// this isssue
				if (Module.Tools.Any(tool => tool is Linker))
				{
					string fullLinkerPath = Module.Package.Project.Properties["link"];
					string linkerExe = Path.GetFileName(fullLinkerPath);
					string linkerDir = Path.GetDirectoryName(fullLinkerPath);

					writer.WriteStartElement("Target"); // <Target Name="PreLinkToolChange"/>
					writer.WriteAttributeString("Name", "PreLinkToolChange");
					writer.WriteAttributeString("BeforeTargets", "Link");

					writer.WriteStartElement("PropertyGroup"); // <PropertyGroup>

					writer.WriteElementString("GNUMode", true); // without this VS will write out a response file with a BOM that linker cannot process properly

					// override linker path
					writer.WriteElementString("ClangToolExe", linkerExe);
					writer.WriteElementString("ClangToolPath", linkerDir);

					// null out some properties that correspond to linker options our linker doesn't understand (or need)
					writer.WriteElementString("MSVCErrorReport", false);
					writer.WriteElementString("Sysroot", String.Empty);
					writer.WriteElementString("GccToolchainPrebuiltPath", String.Empty);
					writer.WriteElementString("GccToolchain", String.Empty);
					writer.WriteElementString("ClangTarget", String.Empty);

					writer.WriteEndElement(); // PropertyGroup

					writer.WriteEndElement(); // Target
				}
			}*/

            if (Module.Tools.Any(tool => tool is Linker))
            {
                // more hacks! when linking a shared object with other shared objects, the typical msbuild
                // cpp build machinery resolved link inputs to full paths, unforuntely this cause the full
                // path to the .so to be embedded in the linked output which cause runtime failures because
                // the path can't be resolved on android
                
                // so...here we override the Link target in msbuild to do some itemgroup surgery. we strip
                // anything in the Link itemgroup which has extension .so and instead -lSharedObjectFileName
                // -Lpath/to/shared/object by passing them via the appropriate attributes to ClangLink
                writer.WriteStartElement("Target");
                writer.WriteAttributeString("Name", "Link");
                writer.WriteAttributeString("Condition", "'@(Link)' != ''");
                {
                    writer.WriteStartElement("ItemGroup");
                    {
                        writer.WriteStartElement("Link");
                        {
                            writer.WriteStartElement("MinimalRebuildFromTracking");
                            writer.WriteAttributeString("Condition", "'$(_BuildActionType)' != 'Build' or '$(ForceRebuild)' == 'true'");
                            writer.WriteString("false");
                            writer.WriteEndElement(); // MinimalRebuildFromTracking

                            writer.WriteElementString("WholeArchiveEnd", "%(Link.WholeArchiveBegin)");
                        }
                        writer.WriteEndElement(); // Link
                    }
                    writer.WriteEndElement(); // ItemGroup

                    writer.WriteStartElement("PropertyGroup");
                    {
                        writer.WriteStartElement("LinkToolArchitecture");
                        writer.WriteAttributeString("Condition", "'$(LinkToolArchitecture)' == ''");
                        writer.WriteString("$(VCToolArchitecture)");
                        writer.WriteEndElement(); // LinkToolArchitecture

                        writer.WriteStartElement("LinkOutputFile");
                        writer.WriteAttributeString("Condition", "'$(LinkOutputFile)' == ''");
                        writer.WriteString("$(IntDir)$(TargetName)$(TargetExt)");
                        writer.WriteEndElement(); // LinkOutputFile
                    }
                    writer.WriteEndElement(); // PropertyGroup

                    // EA HACKS START HERE
                    // capture all inputs with with extension .so
                    writer.WriteStartElement("ItemGroup");
                    {
                        writer.WriteStartElement("LinkSOs");
                        writer.WriteAttributeString("Include", "@(Link)");
                        writer.WriteAttributeString("Condition", "%(Extension) == '.so'");
                        {
                            writer.WriteElementString("LinkDirectiveName", "-l$([System.String]::Copy('%(Filename)').Substring(3))");
                        }
                        writer.WriteEndElement(); // LinkSOs
                    }
                    writer.WriteEndElement(); // ItemGroup

                    // remove all .so inputs from Link item
                    writer.WriteStartElement("ItemGroup");
                    {
                        writer.WriteStartElement("Link");
                        writer.WriteAttributeString("Remove", "@(Link)");
                        writer.WriteAttributeString("Condition", "%(Extension) == '.so'");
                        writer.WriteEndElement(); // Link
                    }
                    writer.WriteEndElement(); // ItemGroup

                    // transform .so paths to be passed with -Lpath and -lSoFileName
                    writer.WriteStartElement("PropertyGroup");
                    {
                        writer.WriteElementString("SOLinkDirectives", "@(LinkSOs->'%(LinkDirectiveName)')");
                        writer.WriteElementString("SOLibraryDirectories", "@(LinkSOs->'%(RootDir)%(Directory)')");
                    }
					
                    writer.WriteEndElement();

                    // call ClangLink, but prepend our transformed .so information to AdditionalLibraryDirectories and AdditionalDependencies inputs
                    writer.WriteStartElement("ClangLink");
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
                        writer.WriteAttributeString("Sysroot", "$(Sysroot)");
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

                        writer.WriteStartElement("Output");
                        writer.WriteAttributeString("TaskParameter", "SkippedExecution");
                        writer.WriteAttributeString("PropertyName", "LinkSkippedExecution");
                        writer.WriteEndElement(); // Output
                    }
                    writer.WriteEndElement(); // ClangLink
                    // EA HACKS END HERE

                    writer.WriteStartElement("PropertyGroup");
                    {
                        writer.WriteStartElement("StripToolExe");
                        writer.WriteAttributeString("Condition", "'$(StripToolExe)' == '' and '$(ToolchainPrefix)' != ''");
                        writer.WriteString("$(ToolchainPrefix)strip.exe");
                        writer.WriteEndElement(); // StripToolExe
                    }
                    writer.WriteEndElement(); // PropertyGroup

                    // Strip symbols to reduce size for remote debugging 
                    writer.WriteStartElement("Exec");
                    writer.WriteAttributeString("Condition", "'$(PackageDebugSymbols)' != 'true' and '$(LinkSkippedExecution)' != 'true' and '$(TargetPath)' != $([System.IO.Path]::GetFullPath('$(LinkOutputFile)'))");
                    writer.WriteAttributeString("Command", "$(StripToolExe) $(StripOptions) -o \"$(TargetPath)\" \"$(LinkOutputFile)\"");
                    writer.WriteEndElement(); // Exec

                    // If not stripping, then copy to output instead.
                    writer.WriteStartElement("Copy");
                    writer.WriteAttributeString("Condition", "'$(PackageDebugSymbols)' == 'true' and '$(LinkSkippedExecution)' != 'true'");
                    writer.WriteAttributeString("SourceFiles", "$(LinkOutputFile)");
                    writer.WriteAttributeString("DestinationFolder", "$(TargetDir)");
                    writer.WriteEndElement();

                    writer.WriteStartElement("Message");
                    writer.WriteAttributeString("Text", "$(MSBuildProjectFile) -> %(Link.OutputFile)");
                    writer.WriteAttributeString("Importance", "High");
                    writer.WriteEndElement(); // Message;
                }
                writer.WriteEndElement(); // Target
            }

            // yet more hacks! linux style toolchain uses ar which as librarian tool. This doesn't have a command line switch
            // to overwrite the library and will always update it in place. This can lead to problems with function being moved from
            // files which are then delete because the archive will still contain the old file. To fix this we write out our own
            // override of the MS Lib task for android but delete the lib before excuting
            if (Module.Tools.Any(tool => tool is Librarian))
            {
                writer.WriteStartElement("Target");
                writer.WriteAttributeString("Name", "Lib");
                writer.WriteAttributeString("Condition", "'@(Lib)' != ''");
                {
                    writer.WriteStartElement("PropertyGroup");
                    {
                        writer.WriteStartElement("ArchiveToolExe");
                        writer.WriteAttributeString("Condition", "'$(ArchiveToolExe)' == '' and '$(ToolchainPrefix)' != ''");
                        {
                            writer.WriteString("$(ToolchainPrefix)ar.exe");
                        }
                        writer.WriteEndElement(); // ArchiveToolExe

                        writer.WriteStartElement("LibToolArchitecture");
                        writer.WriteAttributeString("Condition", "'$(LibToolArchitecture)' == ''");
                        {
                            writer.WriteString("$(VCToolArchitecture)");
                        }
                        writer.WriteEndElement(); // LibToolArchitecture
                    }
                    writer.WriteEndElement(); // PropertyGroup

                    writer.WriteStartElement("ItemGroup");
                    {
                        writer.WriteStartElement("Lib");
                        {
                            writer.WriteStartElement("MinimalRebuildFromTracking");
                            writer.WriteAttributeString("Condition", "'$(_BuildActionType)' != 'Build' or '$(ForceRebuild)' == 'true'");
                            {
                                writer.WriteString("false");
                            }
                            writer.WriteEndElement(); // MinimalRebuildFromTracking
                        }
                        writer.WriteEndElement(); // Lib
                    }
                    writer.WriteEndElement(); // ItemGroup

                    writer.WriteStartElement("Delete");
                    writer.WriteAttributeString("Files", "%(Lib.OutputFile)");
                    writer.WriteEndElement(); // Delete

                    writer.WriteStartElement("Archive");
                    writer.WriteAttributeString("Sources", "@(Lib)");
                    writer.WriteAttributeString("Command", "%(Lib.Command)");
                    writer.WriteAttributeString("CreateIndex", "%(Lib.CreateIndex)");
                    writer.WriteAttributeString("CreateThinArchive", "%(Lib.CreateThinArchive)");
                    writer.WriteAttributeString("NoWarnOnCreate", "%(Lib.NoWarnOnCreate)");
                    writer.WriteAttributeString("TruncateTimestamp", "%(Lib.TruncateTimestamp)");
                    writer.WriteAttributeString("SuppressStartupBanner", "%(Lib.SuppressStartupBanner)");
                    writer.WriteAttributeString("Verbose", "%(Lib.Verbose)");
                    writer.WriteAttributeString("OutputFile", "%(Lib.OutputFile)");
                    writer.WriteAttributeString("TrackFileAccess", "$(TrackFileAccess)");
                    writer.WriteAttributeString("TrackerLogDirectory", "$(TLogLocation)");
                    writer.WriteAttributeString("MinimalRebuildFromTracking", "%(Lib.MinimalRebuildFromTracking)");
                    writer.WriteAttributeString("TLogReadFiles", "@(LibTLogReadFiles)");
                    writer.WriteAttributeString("TLogWriteFiles", "@(LibTLogWriteFiles)");
                    writer.WriteAttributeString("ToolExe", "$(ArchiveToolExe)");
                    writer.WriteAttributeString("ToolPath", "$(ArchiveToolPath)");
                    writer.WriteAttributeString("ToolArchitecture", "$(LibToolArchitecture)");
                    writer.WriteAttributeString("TrackerFrameworkPath", "$(LibTrackerFrameworkPath)");
                    writer.WriteAttributeString("TrackerSdkPath", "$(LibTrackerSdkPath)");
                    writer.WriteAttributeString("EnableExecuteTool", "$(LibEnableExecuteTool)");
                    writer.WriteEndElement(); // Archive

                    writer.WriteStartElement("Message");
                    writer.WriteAttributeString("Text", "$(MSBuildProjectFile) -> %(Lib.OutputFile)");
                    writer.WriteAttributeString("Importance", "High");
                    writer.WriteEndElement(); // Message
                }
                writer.WriteEndElement(); // Target
            }

            if (Module.Package.Project.Properties["config-compiler"] == "gcc")
			{
				// for gcc msbuild stomps our compiler path override
				// so we add pre compile target to stomp their stomp				
				
				string fullCompilerPath = Module.Package.Project.Properties["cc"];
				string compilerExe = Path.GetFileName(fullCompilerPath);
				string compilerDir = Path.GetDirectoryName(fullCompilerPath);

				writer.WriteStartElement("Target"); // <Target Name="PreCompileToolChange"/>
				writer.WriteAttributeString("Name", "PreCompileToolChange");
                writer.WriteAttributeString("BeforeTargets", "ClCompile");
                {

                    writer.WriteStartElement("PropertyGroup"); // <PropertyGroup>
                    {
                        writer.WriteElementString("ClangToolExe", compilerExe);
                        writer.WriteElementString("ClangToolPath", compilerDir);

                        writer.WriteElementString("Sysroot", String.Empty);
                    }
                    writer.WriteEndElement(); // PropertyGroup
                }
				writer.WriteEndElement(); // Target
			}
		}
    }
}
