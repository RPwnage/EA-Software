using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Threading;
using System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using NAnt.Core.Reflection;
using NAnt.Core.PackageCore;
using EA.FrameworkTasks.Model;
using NAnt.Shared.Properties;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Build
{
    internal class ModuleProcessor_SetModuleData : ModuleProcessorBase, IModuleProcessor, IDisposable
    {
        internal ModuleProcessor_SetModuleData(ProcessableModule module, string logPrefix)
            : base(module, logPrefix)
        {
            ModuleOutputNameMapping = Project.GetOutputMapping(module.Package.Name);
        }

#if FRAMEWORK_PARALLEL_TRANSITION
        public bool ProcessGenerationData ;
#endif

        private readonly OptionSet ModuleOutputNameMapping = null;

        public void Process(ProcessableModule module)
        {
            BufferedLog log = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
            Chrono timer = (log == null) ? null : new Chrono();

            if(log != null)
            {
                log.Info.WriteLine(LogPrefix + "processing module {0} - {1}", module.GetType().Name, module.ModuleIdentityString());
            }

            module.IntermediateDir = GetIntermediateDir(module);
            module.OutputDir = module.IntermediateDir;

            SetPublicData(module);

            SetModuleReferenceFlags(module);

            module.Apply(this);

            SetModuleBuildSteps(module);

            foreach (Tool t in module.Tools)
            {
                SetToolTemplates(t);
            }

            if (log != null)
            {
                log.IndentLevel += 6;
                log.Info.WriteLine("tools:       {0}", module.Tools.ToString(", ", t=> t.ToolName.Quote()));
                if (module.BuildSteps != null && module.BuildSteps.Count > 0)
                {
                    log.Info.WriteLine("build steps: {0}", module.BuildSteps.ToString(", ", s => s.Name.Quote()));
                }
                log.Info.WriteLine("completed    {0}", timer.ToString());
                log.Flash();
            }
        }

        public void Process(Module_Native module)
        {
            string outputlibdir = null;
            string outputname = module.Name;

            var headerfiles = AddModuleFiles(module, ".headerfiles", "${package.dir}/include/**/*.h", "${package.dir}/" + module.GroupSourceDir + "/**/*.h");

            // Obsolete way of defining custom build rules:
            var fileToCustomBuildToolMap = IntitFileCustomBuildToolMapObsolete();

            var custombuildsources = new FileSet();
            headerfiles = SetupCustomBuildToolFilesObsolete(module, headerfiles, fileToCustomBuildToolMap, ref custombuildsources);

            headerfiles.SetFileDataFlags(FileData.Header);

            module.ExcludedFromBuild_Files.AppendWithBaseDir(headerfiles);

            module.PrecompiledFile = new PathString(String.Empty);
            module.PrecompiledHeaderFile = String.Empty;

            module.CopyLocal = ConvertUtil.ToEnum<CopyLocalType>(PropGroupValue("copylocal", "false"));

            if (module.IsKindOf(Module.Managed))
            {
                module.TargetFrameworkVersion = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".targetframeworkversion"), Project.Properties["package." + module.Package.Name + ".targetframeworkversion"] ?? PackageMap.Instance.GetMasterVersion("DotNet", Project));
            }
            module.RootNamespace = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".rootnamespace"), "");

            // Add custom build tools
            var custombuildfiles_temp = AddModuleFiles(module, ".custombuildfiles");
            FileSet custombuildObjects = null;
            List<PathString> custombuildIncludeDirs = null;
            if (custombuildfiles_temp != null)
            {
                custombuildObjects = new FileSet();
                custombuildIncludeDirs = new List<PathString>();

                var basedir = PathString.MakeNormalized(custombuildfiles_temp.BaseDirectory);

                foreach (var fileitem in custombuildfiles_temp.FileItems)
                {
                    module.AddTool(CreateCustomBuildTools(module, fileitem, basedir, custombuildsources, custombuildObjects, custombuildIncludeDirs));
                }
            }

            module.CustomBuildRuleFiles = PropGroupFileSet("vcproj.custom-build-rules");
            Project.NamedOptionSets.TryGetValue(PropGroupName(".vcproj.custom-build-rules-options"), out module.CustomBuildRuleOptions);

            foreach (string buildtask in StringUtil.ToArray(OptionSetUtil.GetOptionOrFail(Project, BuildOptionSet, "build.tasks")))
            {
                switch (buildtask)
                {
                    case "asm":
                        {
                            module.Asm = CreateAsmTool(BuildOptionSet, module);

                            FileSet asmsources = AddModuleFiles(module, ".asmsourcefiles");

                            //Support for deprecated file custom build rules syntax;
                            asmsources = SetupCustomBuildToolFilesObsolete(module, asmsources, fileToCustomBuildToolMap, ref custombuildsources);

                            if (asmsources != null)
                            {
                                var commonroots = new CommonRoots<FileItem>(asmsources.FileItems.OrderBy(e => e.Path.Path), (fe) => 
#if FRAMEWORK_PARALLEL_TRANSITION
                                {
                                    if (!fe.IsKindOf(FileData.BulkBuild))
                                    {
                                        string dirPath = Path.GetDirectoryName(fe.Path.Path);
                                        // Add one more directory level above files, unless files are just in drive root:

                                        if (dirPath.IndexOf(Path.DirectorySeparatorChar) != dirPath.LastIndexOf(Path.DirectorySeparatorChar)
                                            && !dirPath.Equals(module.Package.Dir.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
                                        {
                                            return dirPath;
                                        }
                                    }
                                    return fe.Path.Path;
                                },
#else
                                fe.Path.Path,
#endif
#if FRAMEWORK_PARALLEL_TRANSITION
                                new string[] { PackageMap.Instance.BuildRoot });
#else
                                new string[] { PackageMap.Instance.BuildRoot, module.Package.Dir.Path });
#endif
                                var duplicateObjFiles = new HashSet<string>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

                                // Set file specific options:
                                foreach (var fileitem in asmsources.FileItems)
                                {
                                    Tool tool = null;
                                    if (!String.IsNullOrEmpty(fileitem.OptionSetName))
                                    {
                                        OptionSet fileOptions;
                                        if (Project.NamedOptionSets.TryGetValue(fileitem.OptionSetName, out fileOptions))
                                        {
                                            tool = CreateAsmTool(fileOptions, module);

                                            SetToolTemplates(tool, fileOptions);

                                            if (custombuildIncludeDirs != null)
                                            {
                                                (tool as AsmCompiler).IncludeDirs.AddUnique(custombuildIncludeDirs);
                                            }

                                            ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["preprocess"].ToArray());
                                            ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["postprocess"].ToArray());
                                        }
                                    }
                                    uint nameConflictFlag = 0;
                                    var objFile = GetObjectFile(fileitem, tool ?? module.Asm, module, commonroots, duplicateObjFiles, out nameConflictFlag);

                                    fileitem.SetFileData(tool, objFile, nameConflictFlag);
                                }
                            }
                            module.Asm.SourceFiles.IncludeWithBaseDir(asmsources);
                            if (custombuildIncludeDirs != null)
                            {
                                module.Asm.IncludeDirs.AddUnique(custombuildIncludeDirs);
                            }
                        }
                        break;
                    case "cc":
                        {
                            module.Cc = CreateCcTool(BuildOptionSet, module);

                            SetDebuglevelObsolete(module, module.Cc);

                            if (module.Cc.PrecompiledHeaders != CcCompiler.PrecompiledHeadersAction.NoPch)
                            {
#if FRAMEWORK_PARALLEL_TRANSITION
                                module.PrecompiledFile = PathString.MakeNormalized(PropGroupValue("pch-file", "%intermediatedir%\\stdPCH.pch")
                                        .Replace("%intermediatedir%", GetLibOutputDir(module).Path)
                                        .Replace("%outputdir%", GetLibOutputDir(module).Path)
                                        , PathString.PathParam.NormalizeOnly);

                                if (!Path.IsPathRooted(module.PrecompiledFile.Path))
                                {
                                    module.PrecompiledFile = PathString.MakeCombinedAndNormalized(GetLibOutputDir(module).Path, module.PrecompiledFile.Path, PathString.PathParam.NormalizeOnly);
                                }
#else
                                module.PrecompiledFile = PathString.MakeNormalized(PropGroupValue("pch-file", "%intermediatedir%\\stdPCH.pch")
                                        .Replace("%intermediatedir%", module.IntermediateDir.Path)
                                        .Replace("%outputdir%", module.OutputDir.Path)
                                        , PathString.PathParam.NormalizeOnly);

                                if (!Path.IsPathRooted(module.PrecompiledFile.Path))
                                {
                                    module.PrecompiledFile = PathString.MakeCombinedAndNormalized(module.IntermediateDir.Path, module.PrecompiledFile.Path, PathString.PathParam.NormalizeOnly);
                                }
#endif
                                module.PrecompiledHeaderFile = PathNormalizer.Normalize(PropGroupValue("pch-header-file", "stdPCH.h").TrimWhiteSpace(), false);
                            }

                            if (custombuildIncludeDirs != null)
                            {
                                module.Cc.IncludeDirs.AddUnique(custombuildIncludeDirs);
                            }

                            //some packages add header files into set of source file. Clean this up:
                            var cc_sources = RemoveHeadersFromSourceFiles(module, AddModuleFiles(module, ".sourcefiles", Path.Combine(module.Package.Dir.Path, module.GroupSourceDir) + "/**/*.cpp"));

                            // Support for deprecated file custom build rules;
                            cc_sources = SetupCustomBuildToolFilesObsolete(module, cc_sources, fileToCustomBuildToolMap, ref custombuildsources);

                            FileSet excludedSourcefiles;

                            FileSet sources = SetupBulkbuild(module, module.Cc, cc_sources, custombuildsources, out excludedSourcefiles);

                            var commonroots = new CommonRoots<FileItem>(sources.FileItems.OrderBy(e => e.Path.Path), (fe) =>
#if FRAMEWORK_PARALLEL_TRANSITION
                                {
                                    if (!fe.IsKindOf(FileData.BulkBuild))
                                    {
                                        string dirPath = Path.GetDirectoryName(fe.Path.Path);
                                        // Add one more directory level above files, unless files are just in drive root:

                                        if (dirPath.IndexOf(Path.DirectorySeparatorChar) != dirPath.LastIndexOf(Path.DirectorySeparatorChar)
                                            && !dirPath.Equals(module.Package.Dir.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
                                        {
                                            return dirPath;
                                        }
                                    }
                                    return fe.Path.Path;
                                },
#else
                                fe.Path.Path,
#endif
#if FRAMEWORK_PARALLEL_TRANSITION
                                new string[] { PackageMap.Instance.BuildRoot });
#else
                                new string[] { PackageMap.Instance.BuildRoot, module.Package.Dir.Path });
#endif



                            var duplicateObjFiles = new HashSet<string>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

                            var tools = new Dictionary<string, Tool>();

                            // Set file specific options:
                            foreach (var fileitem in sources.FileItems)
                            {
                                Tool tool = null;

                                if (!String.IsNullOrEmpty(fileitem.OptionSetName))
                                {
                                    bool isDerivedFromBuildType;
                                    var fileOptions = GetFileBuildOptionSet(fileitem, out isDerivedFromBuildType);

                                    if (fileOptions != null)
                                    {
                                        if (!tools.TryGetValue(fileitem.OptionSetName, out tool))
                                        {
                                            tool = CreateCcTool(fileOptions, module);
                                            SetToolTemplates(tool, fileOptions);
                                            tools.Add(fileitem.OptionSetName, tool);

                                            if (custombuildIncludeDirs != null)
                                            {
                                                (tool as CcCompiler).IncludeDirs.AddUnique(custombuildIncludeDirs);
                                            }
                                        }
                                        if (!isDerivedFromBuildType)
                                        {
                                            ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["preprocess"].ToArray());
                                            ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["postprocess"].ToArray());
                                        }
                                    }
                                }

                                uint nameConflictFlag = 0;
                                var objFile = GetObjectFile(fileitem, tool ?? module.Cc, module, commonroots, duplicateObjFiles, out nameConflictFlag);

                                fileitem.SetFileData(tool, objFile, nameConflictFlag);
                            }

                            module.Cc.SourceFiles.IncludeWithBaseDir(sources);

                            if(excludedSourcefiles != null)
                            {
                                var emptyCc = new CcCompiler(module.Cc.Executable, description: "#global_context_settings");

                                var commonExcludedroots = new CommonRoots<FileItem>(excludedSourcefiles.FileItems.OrderBy(e => e.Path.Path), (fe) => fe.Path.Path, new string[] { PackageMap.Instance.BuildRoot, module.Package.Dir.Path });

                                foreach (var fileitem in excludedSourcefiles.FileItems)
                                {
                                    Tool tool = emptyCc;

                                    if (!String.IsNullOrEmpty(fileitem.OptionSetName))
                                    {
                                        bool isDerivedFromBuildType;
                                        var fileOptions = GetFileBuildOptionSet(fileitem, out isDerivedFromBuildType);

                                        if (fileOptions != null)
                                        {
                                            if (!tools.TryGetValue(fileitem.OptionSetName, out tool))
                                            {
                                                tool = CreateCcTool(fileOptions, module);
                                                SetToolTemplates(tool, fileOptions);
                                                tools.Add(fileitem.OptionSetName, tool);
                                            }
                                        }
                                    }

                                    uint nameConflictFlag = 0;
                                    var objFile = GetObjectFile(fileitem, tool, module, commonExcludedroots, duplicateObjFiles, out nameConflictFlag);

                                    fileitem.SetFileData(tool, objFile, nameConflictFlag);
                                }
                                module.ExcludedFromBuild_Sources.IncludeWithBaseDir(excludedSourcefiles);
                            }
                        }
                        break;
                    case "link":
                        module.Link = CreateLinkTool(BuildOptionSet, module);
                        module.PostLink = CreatePostLinkTool(BuildOptionSet, module, module.Link);

                        outputlibdir = module.Link.ImportLibOutputDir.Path;
                        outputname = module.Link.OutputName;

                        if (custombuildObjects != null)
                        {
                            module.Link.ObjectFiles.Include(custombuildObjects);
                        }
                        break;
                    case "lib":
                        module.Lib = CreateLibTool(BuildOptionSet, module);

                        outputname = module.Lib.OutputName;

                        if (custombuildObjects != null)
                        {
                            module.Lib.ObjectFiles.Include(custombuildObjects);
                        }

                        break;
                    default:
                        Log.Warning.WriteLine(LogPrefix + "Custom tasks can not be processed by Framework 2: BuildType='{0}', build task='{1}' [{2}].", module.BuildType.Name, buildtask, BuildOptionSet.Options["build.tasks"]);
                        break;
                }
            }

            foreach (Tool tool in module.Tools)
            {
                ReplaceTemplates(tool, module, outputlibdir, outputname);
            }


            if (module.Cc != null)
            {
                foreach (var fileitem in module.Cc.SourceFiles.FileItems)
                {
                    ReplaceTemplates(fileitem.GetTool(), module, outputlibdir, outputname);
                }
            }

            if (module.Asm != null)
            {
                foreach (var fileitem in module.Asm.SourceFiles.FileItems)
                {
                    ReplaceTemplates(fileitem.GetTool(), module, outputlibdir, outputname);
                }
            }

            module.ExcludedFromBuild_Files.IncludeWithBaseDir(PropGroupFileSet("vcproj.excludedbuildfiles"));

            //Set module types.
            if (module.Link != null)
            {
                if(!module.IsKindOf(Module.Program | Module.DynamicLibrary))
                {
                    Log.Warning.WriteLine("package {0}-{1} module {2} [{3}] has buildtype {4} ({5}) with 'link' task but module kind is not set to 'Program' or 'DynamicLibrary'.", module.Package.Name, module.Package.Version, module.BuildGroup + "." + module.Name, module.Configuration.Name, BuildType.Name, BuildType.BaseName);
                }
            }
            else if(module.Lib != null)
            {
                if (!module.IsKindOf(Module.Library))
                {
                    Log.Warning.WriteLine("package {0}-{1} module {2} [{3}] has buildtype {4} ({5}) with 'lib' task but module kind is not set to 'Library'.", module.Package.Name, module.Package.Version, module.BuildGroup + "." + module.Name, module.Configuration.Name, BuildType.Name, BuildType.BaseName);
                }
            }

#if !FRAMEWORK_PARALLEL_TRANSITION
            // Create prebuild step to ensure all output directories exist as some toll will fail if the don't
            BuildStep prebuild = new BuildStep("prebuild", BuildStep.PreBuild);
            prebuild.Commands.Add(new Command(String.Empty, String.Empty, createdirectories: new PathString[] {module.OutputDir, module.IntermediateDir, 
                (module.Link != null) ? module.Link.LinkOutputDir : null, (module.Link != null) ? module.Link.ImportLibOutputDir : null }.Where(d => d != null).OrderedDistinct()));

            AddBuildStepToModule(module, prebuild);
#endif
        }

        public void Process(Module_DotNet module)
        {
            if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.manual.sourcefiles")))
            {
                Error.Throw(Project, "SetupBulkBuild", "BulkBuild features aren't available to C# or F# modules, but '{0}.bulkbuild.manual.sourcefiles' was specified", module.GroupName);
            }
            if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.manual.sourcefiles." + module.Configuration.System)))
            {
                Error.Throw(Project, "SetupBulkBuild", "BulkBuild features aren't available to C# or F# modules, but '{0}.bulkbuild.manual.sourcefiles.{1}' was specified", module.GroupName, module.Configuration.System);
            }
            if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
            {
                Error.Throw(Project, "SetupBulkBuild", "BulkBuild features aren't available to C# or F# modules, but '{0}.bulkbuild.filesets' was specified", module.GroupName);
            }

            module.OutputDir = GetBinOutputDir(module);

            module.CopyLocal = ConvertUtil.ToEnum<CopyLocalType>(PropGroupValue("copylocal", "false"));

            module.TargetFrameworkVersion = PropGroupValue(GetFrameworkPrefix(module) + ".targetframeworkversion", // For bacwards compatibility
                                                PropGroupValue(".targetframeworkversion", Project.Properties["package." + module.Package.Name + ".targetframeworkversion"]));
            // If targetframework version was not explicitly defined retrieve it from the dot-net package:
            if (string.IsNullOrEmpty(module.TargetFrameworkVersion))
            {
                module.TargetFrameworkVersion = PackageMap.Instance.GetMasterVersion("DotNet", Project);
            }

            module.RootNamespace = PropGroupValue(GetFrameworkPrefix(module) + ".rootnamespace", module.Name);

            module.ApplicationManifest = PropGroupValue(GetFrameworkPrefix(module) + ".application-manifest", PropGroupValue("application-manifest"));
            module.AppDesignerFolder = PathString.MakeNormalized(PropGroupValue(GetFrameworkPrefix(module) + ".appdesigner-folder", PropGroupValue("appdesigner-folder")), PathString.PathParam.NormalizeOnly);

            module.DisableVSHosting = GetModuleBooleanPropertyValue(GetFrameworkPrefix(module) + ".disablevshosting");

            foreach (DotNetProjectTypes type in Enum.GetValues(typeof(DotNetProjectTypes)))
            {
                if (GetModuleBooleanPropertyValue(GetFrameworkPrefix(module) + "." + type.ToString().ToLowerInvariant()))
                {
                    module.ProjectTypes |= type;
                }
            }

            module.GenerateSerializationAssemblies = ConvertUtil.ToEnum<DotNetGenerateSerializationAssembliesTypes>(GetModulePropertyValue(".generateserializationassemblies", "None"));

            module.ImportMSBuildProjects = GetModulePropertyValue(GetFrameworkPrefix(module) + ".importmsbuildprojects", Project.Properties["eaconfig." + GetFrameworkPrefix(module) + ".importmsbuildprojects"]);

            Project.NamedOptionSets.TryGetValue(PropGroupName(".webreferences"), out module.WebReferences);

            module.RunPostBuildEventCondition = GetModuleSinglePropertyValue(GetFrameworkPrefix(module) + ".runpostbuildevent", "OnOutputUpdated");

            module.ExcludedFromBuild_Files.Include(PropGroupFileSet(GetFrameworkPrefix(module) + ".excludedbuildfiles"));


            foreach (string buildtask in StringUtil.ToArray(OptionSetUtil.GetOptionOrFail(Project, BuildOptionSet, "build.tasks")))
            {
                DotNetCompiler compiler = null;
                string dotnetbinfolder = Project.Properties["package.DotNet.bindir"] ?? Project.Properties["package.DotNet.appdir"];

                switch (buildtask)
                {
                    case "csc":

                        if(!module.IsKindOf(Module.CSharp))
                        {
                            Log.Warning.WriteLine(LogPrefix + "package {0} module {1} is declared as C# module, but does not have csc task in BuildType '{2}", module.Package.Name, module.Name, module.BuildType.Name);
                        }

                        CscCompiler csc = new CscCompiler(PathString.MakeNormalized(Path.Combine(dotnetbinfolder, "csc.exe"), PathString.PathParam.NormalizeOnly),
                                                          ConvertUtil.ToEnum<DotNetCompiler.Targets>(GetOptionString("csc.target", BuildOptionSet.Options)));

                        module.Compiler = compiler = csc;

                        csc.Options.Add("/nowarn:1607");  // Supress Assembly generation - Referenced assembly 'System.Data.dll' targets a different processor 
                        csc.Win32icon = PathString.MakeNormalized(GetModulePropertyValue(".win32icon"));
                        csc.Modules.AppendWithBaseDir(AddModuleFiles(module, ".modules", "${package.dir}/" + module.GroupSourceDir + "/**/*.module"));

                        string cscDoc = PropGroupValue("csc-doc", null);

                        if (cscDoc != null && !(cscDoc == "true" || cscDoc == "false"))
                        {
                            String msg = Error.Format(Project, "ModuleProcessor", "WARNING", "property '{0}.csc-doc'={1}, it should only be either 'true' or 'false', Other string values are considered invalid!", module.GroupName, cscDoc);
                            Log.Warning.WriteLine(LogPrefix+ msg);
                        }

                        compiler.GenerateDocs = cscDoc.ToBoolean();

                        break;

                    case "fsc":

                        if(!module.IsKindOf(Module.FSharp))
                        {
                            Log.Warning.WriteLine(LogPrefix + "package {0} module {1} is declared as F# module, but does not have fsc task in BuildType '{2}", module.Package.Name, module.Name, module.BuildType.Name);
                        }

                        FscCompiler fsc = new FscCompiler(PathString.MakeNormalized(Path.Combine(dotnetbinfolder, "fsc.exe")),
                                                          ConvertUtil.ToEnum<DotNetCompiler.Targets>(GetOptionString("fsc.target", BuildOptionSet.Options)));

                        module.Compiler = compiler = fsc;

                        string fscDoc = PropGroupValue("fsc-doc", null);
                        if (fscDoc != null && !(fscDoc == "true" || fscDoc == "false"))
                        {
                            String msg = Error.Format(Project, "ModuleProcessor", "WARNING", "property '{0}.fsc-doc'={1}, it should only be either 'true' or 'false', Other string values are considered invalid!", module.GroupName, fscDoc);
                            Log.Warning.WriteLine(LogPrefix + msg);
                        }

                        compiler.GenerateDocs = fscDoc != null;

                        break;

                    default:
                        Log.Warning.WriteLine(LogPrefix + "Custom tasks can not be processed by Framework 2: BuildType='{0}', build task='{1}' [{2}].", module.BuildType.Name, buildtask, BuildOptionSet.Options["build.tasks"]);
                        break;
                }

                if (compiler == null)
                {
                    Error.Throw(Project, "Process(Module_DotNet)", "[{0}] module '{1}' does not have valid compiler task defined. Tasks: '{2}'", module.Package.Name, module.Name, module.Tools.Select(tool => tool.ToolName).ToString(" "));
                }

                // ---- Add common csc/fsc settings ---

                string outputname;
                string replacename;
                string outputextension;
                PathString linkoutputdir;
                SetLinkOutput(module, out outputname, out outputextension, out linkoutputdir, out replacename); // Redefine output dir and output name based on linkoutputname property.
                compiler.OutputName = outputname;
                compiler.OutputExtension = outputextension;

                string optimization = GetOptionString("optimization", BuildOptionSet.Options, "optimization", useOptionProperty:false);
                string debugflags = GetOptionString("debugflags", BuildOptionSet.Options, "debugflags", useOptionProperty: false);

                var nodefaultdefines = GetOptionString("nodefaultdefines", BuildOptionSet.Options, "nodefaultdefines", useOptionProperty: false).OptionToBoolean();

                if (optimization == "on" || optimization == "custom")
                {
                    compiler.Options.Add("/optimize");
                }

                if (false == nodefaultdefines)
                {
                    compiler.Defines.Add("EA_DOTNET2");
                }
                
                if(debugflags == "on" || debugflags == "custom")
                {
                    AddOptionOrPropertyValue(compiler, "debug", "/debug:", (val) => "full");
                    if (false == nodefaultdefines)
                    {
                        compiler.Defines.Add("TRACE");
                        compiler.Defines.Add("DEBUG");
                    }
                }

                compiler.Defines.AddRange(GetOptionString(compiler.ToolName + ".defines", BuildOptionSet.Options, ".defines").LinesToArray());

                compiler.SourceFiles.IncludeWithBaseDir(AddModuleFiles(module, ".sourcefiles"));
                compiler.Assemblies.AppendWithBaseDir(AddModuleFiles(module, ".assemblies"));
                compiler.Resources.IncludeWithBaseDir(AddModuleFiles(module, ".resourcefiles", "${package.dir}/" + module.GroupSourceDir + "/**/*.ico"));
                compiler.Resources.SetFileDataFlags(FileData.EmbeddedResource);
                compiler.NonEmbeddedResources.IncludeWithBaseDir(AddModuleFiles(module, ".resourcefiles.notembed"));

                compiler.Resources.BaseDirectory = GetModulePropertyValue(".resourcefiles.basedir", compiler.SourceFiles.BaseDirectory??compiler.Resources.BaseDirectory);
                compiler.Resources.Prefix = GetModulePropertyValue(".resourcefiles.prefix", module.Name);

                compiler.ContentFiles.IncludeWithBaseDir(AddModuleFiles(module, ".contentfiles"));
                compiler.ContentFiles.SetFileDataFlags(FileData.ContentFile);

                compiler.AdditionalArguments = GetModulePropertyValue(compiler.ToolName + "-args");

                if (module.Compiler.AdditionalArguments.Contains("-nowin32manifest"))
                {
                    compiler.Options.Add("-nowin32manifest");
                    module.Compiler.AdditionalArguments = module.Compiler.AdditionalArguments.Replace("-nowin32manifest", String.Empty);
                }
                if (module.Compiler.AdditionalArguments.Contains("/nowin32manifest"))
                {
                    compiler.Options.Add("/nowin32manifest");
                    module.Compiler.AdditionalArguments = module.Compiler.AdditionalArguments.Replace("/nowin32manifest", String.Empty);
                }


                var content_action = GetModulePropertyValue(".contentfiles.action", null).TrimWhiteSpace().ToLowerInvariant();

                if (!String.IsNullOrEmpty(content_action))
                {
                    if (content_action == "copy-always" || content_action == "do-not-copy" || content_action == "copy-if-newer")
                    {
                        foreach (var fsItem in compiler.ContentFiles.Includes)
                        {
                            fsItem.OptionSet = content_action;
                        }
                    }
                    else
                    {
                        Log.Warning.WriteLine("package {0} module {1}: invalid value of '{2}'= '{3}', valid values are: {4}.", module.Package.Name, module.Name, PropGroupValue(".contentfiles.action"), content_action, "copy-always, do-not-copy, copy-if-newer");
                    }
                }

                // ---- options -----

                char[] OPTIONS_DELIM = new char[] { ' ', ';', ',', '\n', '\t' };

                compiler.Defines.AddRange(GetOptionString(compiler.ToolName + ".defines", BuildOptionSet.Options, ".defines").LinesToArray());

                if (!AddOptionOrPropertyValue(compiler, "warningsaserrors.list", "/warnaserror:", (val) => val.ToArray(OPTIONS_DELIM).ToString(",")))
                {
                    //If the list is not defined, we need to see if warningsaserrors is defined
                    AddOptionOrPropertyValue(compiler, "warningsaserrors", "/warnaserror");
                }

                AddOptionOrPropertyValue(compiler, "suppresswarnings", "/nowarn:", val => val.ToArray(OPTIONS_DELIM).ToString(","));

                AddOptionOrPropertyValue(compiler, "allowunsafe", "/unsafe");

                AddOptionOrPropertyValue(compiler, "keyfile", "/keyfile:", val => PathNormalizer.Normalize(val, false).Quote());

                AddOptionOrPropertyValue(compiler, "platform", "/platform:", (val) =>
                {
                    string platform = "anycpu";

                    if (String.IsNullOrWhiteSpace(val))
                    {
                        bool useExactPlatform = Project.Properties.GetBooleanPropertyOrDefault("eaconfig.use-exact-dotnet-platform", GetModuleBooleanPropertyValue("use-exact-dotnet-platform", false))
                            || (compiler.Target == DotNetCompiler.Targets.Exe || compiler.Target == DotNetCompiler.Targets.WinExe);

                        if (useExactPlatform)
                        {
                            var processor = (Project.Properties["config-processor"] ?? String.Empty).TrimWhiteSpace().ToLowerInvariant();
                            switch (processor)
                            {
                                case "x86":
                                    platform = "x86";
                                    break;
                                case "x64":
                                    platform = "x64";
                                    break;
                                case "arm":
                                    platform = "ARM";
                                    break;
                                default:
                                    //In case config processor is not defined, check pc/pc64
                                    switch (module.Configuration.System)
                                    {
                                        case "pc":
                                            platform = "x86";
                                            break;
                                        case "pc64":
                                            platform = "x64";
                                            break;
                                        default:
                                            platform = "anycpu";
                                            break;
                                    }
                                    break;
                            }
                        }
                        else
                        {
                            platform = "anycpu";
                        }
                    }
                    else
                    {
                        platform = val;
                    }
                    return platform;
                });

                // Additional options;

                var additionalargs = GetModulePropertyValue("." + compiler.ToolName + "-args").TrimWhiteSpace();                
                if (!String.IsNullOrEmpty(additionalargs))
                {
                    int index = 0;
                    while (index < additionalargs.Length)
                    {
                        int ind1 = Math.Min(additionalargs.IndexOf(" /", index), additionalargs.IndexOf(" -"));
                        string option = null;
                        if (ind1 < 0)
                        {
                            option = additionalargs.Substring(index);
                            index = additionalargs.Length;
                        }
                        else
                        {
                            option = additionalargs.Substring(index, ind1 - index);
                            index = ind1 + 1;
                        }
                        option = option.TrimWhiteSpace();
                        if (!String.IsNullOrEmpty(option))
                        {
                            compiler.Options.Add(option);
                        }
                    }
                }

                compiler.Options.AddRange(BuildOptionSet.Options[compiler.ToolName + ".options"].LinesToArray());

                if (compiler.GenerateDocs)
                {
                    compiler.DocFile = PathString.MakeNormalized(GetModuleSinglePropertyValue(".doc-file", Path.Combine(module.IntermediateDir.Path, compiler.OutputName + ".xml")));
                }

                compiler.ApplicationIcon = PathString.MakeNormalized(GetModulePropertyValue(".win32icon"));

                SetupComAssembliesAssemblies(compiler, module);

                SetupWebReferences(compiler, module);

                SetDebuglevelObsolete(module, compiler);
            }
        }



        public void Process(Module_Utility module)
        {

            AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".asmsourcefiles");
            AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".sourcefiles");

            var headerfiles = AddModuleFiles(module, ".headerfiles", "${package.dir}/include/**/*.h", "${package.dir}/" + module.GroupSourceDir + "/**/*.h");
            headerfiles.SetFileDataFlags(FileData.Header);
            module.ExcludedFromBuild_Files.AppendWithBaseDir(headerfiles);

            module.ExcludedFromBuild_Files.Include(PropGroupFileSet("vcproj.excludedbuildfiles"));

            // Add custom build tools
            var custombuildfiles = AddModuleFiles(module, ".custombuildfiles");
            if (custombuildfiles != null)
            {
                var basedir = PathString.MakeNormalized(custombuildfiles.BaseDirectory);

                foreach (var fileitem in custombuildfiles.FileItems)
                {
                    module.AddTool(CreateCustomBuildTools(module, fileitem, basedir, module.ExcludedFromBuild_Sources));
                }
            }
        }

        public void Process(Module_MakeStyle module)
        {
            if (module.BuildSteps.Count > 0)
            {
                Error.Throw(Project, "MakeStyle", "MakesStyle module does not support build steps. '{0}' buildsteps were defined.", module.BuildSteps.Count);
            }

            var command = PropGroupValue("MakeBuildCommand").TrimWhiteSpace();

            if (!String.IsNullOrEmpty(command))
            {
                var buildstep = new BuildStep("MakeStyle-Build", BuildStep.Build | BuildStep.ExecuteAlways);
                buildstep.Commands.Add(new Command(command, workingDir: PathString.MakeNormalized(Project.Properties["package.builddir"])));
                module.BuildSteps.Add(buildstep);
            }

            command = PropGroupValue("MakeRebuildCommand").TrimWhiteSpace();

            if (!String.IsNullOrEmpty(command))
            {
                var buildstep = new BuildStep("MakeStyle-ReBuild", BuildStep.ReBuild | BuildStep.ExecuteAlways);
                buildstep.Commands.Add(new Command(command, workingDir: PathString.MakeNormalized(Project.Properties["package.builddir"])));
                module.BuildSteps.Add(buildstep);
            }

            command = PropGroupValue("MakeCleanCommand").TrimWhiteSpace();

            if (!String.IsNullOrEmpty(command))
            {
                var buildstep = new BuildStep("MakeStyle-Clean", BuildStep.Clean | BuildStep.ExecuteAlways);
                buildstep.Commands.Add(new Command(command, workingDir: PathString.MakeNormalized(Project.Properties["package.builddir"])));
                module.BuildSteps.Add(buildstep);
            }

            AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".asmsourcefiles");
            AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".sourcefiles");
            AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".vcproj.excludedbuildfiles");

            var headerfiles = AddModuleFiles(module, ".headerfiles", "${package.dir}/include/**/*.h", "${package.dir}/" + module.GroupSourceDir + "/**/*.h");
            headerfiles.SetFileDataFlags(FileData.Header);
            module.ExcludedFromBuild_Files.AppendWithBaseDir(headerfiles);
        }

        public void Process(Module_Python module)
        {
            module.SetType(Module.Python);
            module.SearchPath = PropGroupValue("searchpaths", "");
            module.StartupFile = PropGroupValue("startupfile", "");
            module.WorkDir = PropGroupValue("workdir", ".");
            module.ProjectHome = PropGroupValue("projecthome", ".");
            module.IsWindowsApp = PropGroupValue("windowsapp", "false");

            // todo: only activepython is checked, should be updated for alternate python interpreters
            // note: python interpreter is currently only used to store file names
            string pythonexe = Project.Properties.GetPropertyOrDefault("package.ActivePython.exe", "");
            PythonInterpreter interpreter = new PythonInterpreter("python", 
                PathString.MakeNormalized(pythonexe, PathString.PathParam.NormalizeOnly));

            interpreter.SourceFiles.IncludeWithBaseDir(AddModuleFiles(module, ".sourcefiles"));
            interpreter.ContentFiles.IncludeWithBaseDir(AddModuleFiles(module, ".contentfiles"));
            module.Interpreter = interpreter;
        }

        public void Process(Module_ExternalVisualStudio module)
        {
            module.VisualStudioProject = PathString.MakeNormalized(PropGroupValue("project-file").TrimWhiteSpace());
            module.VisualStudioProjectConfig = PropGroupValue("project-config").TrimWhiteSpace();
            module.VisualStudioProjectPlatform = PropGroupValue("project-platform").TrimWhiteSpace();

            if (!Guid.TryParse(PropGroupValue("project-guid").TrimWhiteSpace(), out module.VisualStudioProjectGuild))
            {
                var msg = String.Format("Property '{0}={1}' can not be converted to guid", PropGroupName("project-guid"), PropGroupValue("project-guid").TrimWhiteSpace());
                throw new BuildException(msg);
            }

            var ext = Path.GetExtension(module.VisualStudioProject.Path).ToLowerInvariant();
            switch (ext)
            {
                case ".csproj":
                    module.SetType(Module.DotNet | Module.CSharp);
                    break;
                case ".fsproj":
                    module.SetType(Module.DotNet | Module.FSharp);
                    break;
                case ".vcxproj":
                default:
                    if (Project.Properties.GetBooleanPropertyOrDefault(PropGroupName("project-managed"), false))
                    {
                        module.SetType(Module.Managed);
                    }
                    break;
            }

            if(module.IsKindOf(Module.DotNet))
            {
                foreach (DotNetProjectTypes type in Enum.GetValues(typeof(DotNetProjectTypes)))
                {
                    if (GetModuleBooleanPropertyValue(type.ToString().ToLowerInvariant()))
                    {
                        module.DotNetProjectTypes |= type;
                    }
                }
            }

        }

        public void Process(Module_UseDependency module)
        {
            Error.Throw(Project, "ModuleProcessor", "Internal error: trying to process use dependency module [{0}]", module.Name);
        }

        #region all modules

        private void SetModuleReferenceFlags(IModule module)
        {
            var copylocalModulesDef = GetModulePropertyValue("copylocal.dependencies").ToArray();
            if (copylocalModulesDef.Count > 0)
            {
                // Get all modules declared in the property
                foreach (var item in GetModulePropertyValue("copylocal.dependencies").ToArray())
                {
                    var components = item.Trim(new char[] { '"', '\'' }).ToArray(new char[] { '/' });

                    int foundItems = 0;

                    switch (components.Count)
                    {
                        case 1: // "PackageName"

                            foreach (var d in module.Dependents.Where(dep => dep.Dependent.Package.Name == components[0]))
                            {
                                d.SetType(DependencyTypes.CopyLocal);
                                foundItems++;
                            }
                            break;

                        case 2: // "PackageName/ModuleName"

                            foreach (var d in module.Dependents.Where(dep => dep.Dependent.Package.Name == components[0] && dep.Dependent.Name == components[1]))
                            {
                                d.SetType(DependencyTypes.CopyLocal);
                                foundItems++;
                            }
                            break;
                        case 3: // "PackageName/group/ModuleName"

                            foreach (var d in module.Dependents.Where(dep => dep.Dependent.Package.Name == components[0] && dep.Dependent.BuildGroup.ToString() == components[1] && dep.Dependent.Name == components[2]))
                            {
                                d.SetType(DependencyTypes.CopyLocal);
                                foundItems++;
                            }
                            break;
                        default:
                            if (components.Count > 3)
                            {
                                Log.Warning.WriteLine("Invalid entry in the '{0}' property: '{1}'. Following syntax is accepted: 'PackageName', or 'PackageName/ModuleName', or 'PackageName/group/ModuleName'.", PropGroupName("copylocal.dependencies"), item);
                                foundItems = -1;
                            }
                            break;
                    }

                    if (foundItems == 0)
                    {
                        Log.Warning.WriteLine("Invalid entry in the '{0}' property: '{1}'. Referenced entry is not found in the {2} module dependencies.", PropGroupName("copylocal.dependencies"), item, module.Name);
                    }

                }
            }
        }

        private void SetPublicData(IModule module)
        {
            int errors = 0;

#if NANT_CONCURRENT
            // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
            bool parallel = (PlatformUtil.IsMonoRuntime == false) && (Project.NoParallelNant == false);
#else
            bool parallel = false;
#endif
            try
            {
                if(parallel)
                {
                    Parallel.ForEach(module.Dependents, (d) =>
                        {
                            if (0 == Interlocked.CompareExchange(ref errors, 0, 0))
                            {
                                SetDependentPublicData(module, d);
                            }
                            else
                            {
                                return;
                            }
                        });
                }
                else
                {
                    foreach (var d in module.Dependents)
                    {
                            if (0 == Interlocked.CompareExchange(ref errors, 0, 0))
                            {
                                SetDependentPublicData(module, d);
                            }
                            else
                            {
                                break;
                            }
                    }
                }
            }
            catch (System.Exception e)
            {
                if (1 == Interlocked.Increment(ref errors))
                {
                    Log.Error.WriteLine(ThreadUtil.GetStackTrace(e));

                    ThreadUtil.RethrowThreadingException(
                        String.Format(LogPrefix + "Error processing dependents (ModuleProcessor.DoPackageDependents) of [{0}-{1}], module '{2}'", module.Package.Name, module.Package.Version, module.Name),
                        Location.UnknownLocation, e);
                }
            }
        }

        private void SetDependentPublicData(IModule module, Dependency<IModule> d)
        {

            OptionSet outputmapOptionset = Project.GetOutputMapping(module.Configuration, d.Dependent.Package);

            /*IMTODO do I need these data?
            var declaredModules = (Project.Properties["package." + d.Dependent.Package.Name + "." + d.BuildGroup + ".buildmodules" + BuildType.SubSystem] ??
                                   Project.Properties[PropGroupName(d.Dependent.Package.Name + "." + d.BuildGroup + ".buildmodules")]).ToArray();


            var useDependencyModules = new List<IModule>();
            */

            // Populate public include dirs, public libraries and assemblies.                    
            PublicData packagePublicData = new PublicData();

            string propertyname;

            if (d.IsKindOf(DependencyTypes.Interface))
            {
                propertyname = "package." + d.Dependent.Package.Name + ".defines";
                packagePublicData.Defines = Project.Properties.GetPropertyOrDefault(propertyname + BuildType.SubSystem, Project.Properties[propertyname])
                                                    .LinesToArray();

                propertyname = "package." + d.Dependent.Package.Name + ".includedirs";
                packagePublicData.IncludeDirectories = Project.Properties.GetPropertyOrDefault(propertyname + BuildType.SubSystem, Project.Properties[propertyname])
                                                    .LinesToArray()
                                                    .Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>();

                propertyname = "package." + d.Dependent.Package.Name + ".usingdirs";
                packagePublicData.UsingDirectories = MapDependentOutputDir(
                                                     Project.Properties.GetPropertyOrDefault(propertyname + BuildType.SubSystem, Project.Properties[propertyname])
                                                    .LinesToArray()
                                                    .Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>(),
                                                    outputmapOptionset, d.Dependent.Package, "link");
            }
            if (d.IsKindOf(DependencyTypes.Link))
            {
                propertyname = "package." + d.Dependent.Package.Name + ".libdirs";
                packagePublicData.LibraryDirectories = MapDependentOutputDir(
                                                    Project.Properties.GetPropertyOrDefault(propertyname + BuildType.SubSystem, Project.Properties[propertyname])
                                                    .LinesToArray()
                                                    .Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>(),
                                                    outputmapOptionset, d.Dependent.Package, "lib");

                packagePublicData.Libraries = MapDependentOutputFiles(Project.GetFileSet("package." + d.Dependent.Package.Name + ".libs" + BuildType.SubSystem), outputmapOptionset, d.Dependent.Package, "lib", lockInput: true)
                                                    .SafeClone()
                                                    .AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + ".libs" + BuildType.SubSystem + ".external"));

                packagePublicData.DynamicLibraries = MapDependentOutputFiles(Project.GetFileSet("package." + d.Dependent.Package.Name + ".dlls" + BuildType.SubSystem), outputmapOptionset, d.Dependent.Package, "link", lockInput: true)
                                                    .SafeClone()
                                                    .AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + ".dlls" + BuildType.SubSystem + ".external"));
            }
            if (d.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
            {
                packagePublicData.Assemblies = MapDependentOutputFiles(Project.GetFileSet("package." + d.Dependent.Package.Name + ".assemblies" + BuildType.SubSystem), outputmapOptionset, d.Dependent.Package, "link", lockInput: true)
                                                    .SafeClone()
                                                    .AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + ".assemblies" + BuildType.SubSystem + ".external"));
            }

            if (packagePublicData.Defines == null)
            {
                packagePublicData.Defines = new String[0];
            }
            if (packagePublicData.IncludeDirectories == null)
            {
                packagePublicData.IncludeDirectories = new PathString[0];
            }
            if (packagePublicData.UsingDirectories == null)
            {
                packagePublicData.UsingDirectories = new PathString[0];
            }
            if (packagePublicData.LibraryDirectories == null)
            {
                packagePublicData.LibraryDirectories = new PathString[0];
            }
            if (packagePublicData.Libraries == null)
            {
                packagePublicData.Libraries = new FileSet();
            }
            if (packagePublicData.DynamicLibraries == null)
            {
                packagePublicData.DynamicLibraries = new FileSet();
            }
            if (packagePublicData.Assemblies == null)
            {
                packagePublicData.Assemblies = new FileSet();
            }


            packagePublicData.Libraries.FailOnMissingFile = false;
            packagePublicData.DynamicLibraries.FailOnMissingFile = false;
            packagePublicData.Assemblies.FailOnMissingFile = false;

            var dependentModule = d.Dependent;
            {

                PublicData modulePublicData = new PublicData();
                string moduledefines;

                if (d.IsKindOf(DependencyTypes.Interface))
                {
                    moduledefines = Project.Properties["package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".defines"];
                    modulePublicData.Defines = (moduledefines != null) ? moduledefines.LinesToArray() : packagePublicData.Defines;

                    string moduleincludes = Project.Properties["package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".includedirs"];
                    modulePublicData.IncludeDirectories = (moduleincludes != null) ? moduleincludes.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>() : packagePublicData.IncludeDirectories;

                    string moduleusingdirs = Project.Properties["package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".usingdirs"];
                    modulePublicData.UsingDirectories = moduleusingdirs != null ?
                                    MapDependentOutputDir(moduleusingdirs.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>(), outputmapOptionset, d.Dependent.Package, "lib")
                                  : packagePublicData.UsingDirectories;
                }
                if (d.IsKindOf(DependencyTypes.Link))
                {
                    string modulelibdirs = Project.Properties["package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".libdirs"];
                    modulePublicData.LibraryDirectories = modulelibdirs != null ?
                                    MapDependentOutputDir(modulelibdirs.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>(), outputmapOptionset, d.Dependent.Package, "lib")
                                  : packagePublicData.LibraryDirectories;

                    var modulelibraries = MapDependentOutputFiles(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".libs" + BuildType.SubSystem), outputmapOptionset, d.Dependent.Package, "lib");

                    modulePublicData.Libraries = (modulelibraries ?? packagePublicData.Libraries)
                                                      .SafeClone()
                                                      .AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".libs" + ".external"));

                    var moduledynlibraries = MapDependentOutputFiles(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".dlls" + BuildType.SubSystem), outputmapOptionset, d.Dependent.Package, "link");

                    modulePublicData.DynamicLibraries = (moduledynlibraries ?? packagePublicData.DynamicLibraries)
                                                      .SafeClone()
                                                      .AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".dlls" + ".external"));
                }

                if (d.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
                {
                    var moduleassemblies = MapDependentOutputFiles(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".assemblies" + BuildType.SubSystem), outputmapOptionset, d.Dependent.Package, "link");

                    modulePublicData.Assemblies = (moduleassemblies ?? packagePublicData.Assemblies)
                                                      .SafeClone()
                                                      .AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".assemblies" + ".external"));
                }

                if (modulePublicData.Defines == null)
                {
                    modulePublicData.Defines = new String[0];
                }
                if (modulePublicData.IncludeDirectories == null)
                {
                    modulePublicData.IncludeDirectories = new PathString[0];
                }
                if (modulePublicData.UsingDirectories == null)
                {
                    modulePublicData.UsingDirectories = new PathString[0];
                }
                if (modulePublicData.LibraryDirectories == null)
                {
                    modulePublicData.LibraryDirectories = new PathString[0];
                }
                if (modulePublicData.Libraries == null)
                {
                    modulePublicData.Libraries = new FileSet();
                }
                if (modulePublicData.DynamicLibraries == null)
                {
                    modulePublicData.DynamicLibraries = new FileSet();
                }
                if (modulePublicData.Assemblies == null)
                {
                    modulePublicData.Assemblies = new FileSet();
                }

                modulePublicData.Libraries.FailOnMissingFile = false;
                modulePublicData.DynamicLibraries.FailOnMissingFile = false;
                modulePublicData.Assemblies.FailOnMissingFile = false;


                var copyLocal = ConvertUtil.ToEnum<CopyLocalType>(PropGroupValue("copylocal", "false"));

                if (d.IsKindOf(DependencyTypes.CopyLocal) || copyLocal == CopyLocalType.True || copyLocal == CopyLocalType.Slim)
                {
                    foreach (var inc in modulePublicData.DynamicLibraries.Includes)
                    {
                        if (inc.OptionSet == null)
                        {
                            inc.OptionSet = "copylocal";
                        }
                    }
                    foreach (var inc in modulePublicData.Assemblies.Includes)
                    {
                        if (inc.OptionSet == null)
                        {
                            inc.OptionSet = "copylocal";
                        }
                    }
                }

                if (!d.Dependent.Package.TryAddPublicData(modulePublicData, module, dependentModule))
                {
                    Error.Throw(Project, "ModuleProcessor", "module [{0}] {1} is depending on [{2}] {3} but cannot add public data for it, because this dependency was already processed. This may happen when several targets are chained nant build example-build test-build. Currently this is unsupported.", module.Package.Name, module.BuildGroup + "." + module.Name, d.Dependent.Package.Name, dependentModule.BuildGroup + "." + dependentModule.Name);
                }

            }
        }

        private IEnumerable<PathString> MapDependentOutputDir(IEnumerable<PathString> input, OptionSet outputmapOptionset, IPackage package, string type)
        {
            var output = input;
            if (outputmapOptionset != null && input != null && input.Count() > 0)
            {
                string pathmapping;
                string diroption = "config" + ((type == "lib") ? "lib" : "bin") + "dir";
                if (outputmapOptionset.Options.TryGetValue(diroption, out pathmapping))
                {
                    var newpath = PathString.MakeNormalized(pathmapping);
                    var inputbasedir = PackageMap.Instance.GetBuildGroupRoot(package.Name, Project);

                    output = input.Select(path => (path.Path.StartsWith(inputbasedir) || path.Path.StartsWith(newpath.Path)) ? newpath : path);
                }
            }
            return output;
        }

        private FileSet MapDependentOutputFiles(FileSet input, OptionSet outputmapOptionset, IPackage package, string type, bool lockInput = false)
        {
            var output = input;
            if (outputmapOptionset != null && input != null && input.Includes.Count > 0)
            {
                input.FailOnMissingFile = false;
                string diroption = "config" + ((type == "lib") ? "lib" : "bin") + "dir";
                string pathmapping = outputmapOptionset.Options[diroption];
                string namemapping = outputmapOptionset.Options[type + "outputname"];
                if (pathmapping != null || namemapping != null)
                {
                    output = new FileSet();
                    output.BaseDirectory = input.BaseDirectory;

                    var inputbasedir = PackageMap.Instance.GetBuildGroupRoot(package.Name, Project);
                    var newdir = PathNormalizer.Normalize(pathmapping);

                    FileItemList fileitems;
                    if (lockInput)
                    {
                        lock (input)
                        {
                            // Synchronize fileset expansion;
                            fileitems = input.FileItems;

                        }
                    }
                    else
                    {
                        fileitems = input.FileItems;
                    }

                    foreach (var fi in fileitems)
                    {
                        if (fi.Path.Path.StartsWith(inputbasedir) || (newdir != null && fi.Path.Path.StartsWith(newdir)))
                        {
                            var newname = Path.GetFileNameWithoutExtension(fi.Path.Path);
                            if (namemapping != null)
                            {
                                if (type == "lib")
                                {
                                    var prefix = Project.Properties["lib-prefix"] ?? String.Empty;
                                    if (!String.IsNullOrEmpty(prefix))
                                    {
                                        newname = newname.Substring(prefix.Length);
                                    }
                                    newname = prefix + namemapping.Replace("%outputname%", newname);
                                }
                                else
                                {
                                    newname = namemapping.Replace("%outputname%", newname);
                                }
                            }

                            var newpath = Path.Combine(newdir ?? Path.GetDirectoryName(fi.Path.Path), newname + Path.GetExtension(fi.Path.Path));

                            newpath = PathNormalizer.Normalize(newpath);

                            output.Include(newpath, asIs: fi.AsIs, failonmissing: input.FailOnMissingFile, data: fi.Data);
                        }
                        else
                        {
                            output.Include(fi, failonmissing: input.FailOnMissingFile);
                        }
                    }
                }
            }
            return output;
        }

        private void SetModuleBuildSteps(Module module)
        {
            string prefix = !(module is Module_DotNet) ? "vcproj" : GetFrameworkPrefix(module as Module_DotNet);
            // Pre/post build steps
            AddBuildStepToModule(module, GetBuildStep(module, "prebuild", "prebuildtarget", prefix + ".pre-build-step", BuildStep.PreBuild));
            AddBuildStepToModule(module, GetBuildStep(module, "prelink", "prelinktarget", prefix + ".pre-link-step", BuildStep.PreLink));
            AddBuildStepToModule(module, GetBuildStep(module, "postbuild", "postbuildtarget", prefix + ".post-build-step", BuildStep.PostBuild));
            // Custom build steps. VS runs them last. Add as a postbuild step.
            AddBuildStepToModule(module, GetCustomBuildStep(BuildStep.PostBuild));
        }

        private void AddBuildStepToModule(Module module, BuildStep step)
        {
            if (step != null)
            {
                module.BuildSteps.Add(step);
            }
        }

        private BuildStep GetBuildStep(Module module, string stepname, string targetname, string commandname, uint type)
        {
            BuildStep step = new BuildStep(stepname, type);

            // Collect all target names:
            var targets = new List<Tuple<string, string, string>>() 
            { 
                Tuple.Create(Project.Properties["eaconfig." + targetname] ?? "eaconfig." + targetname, "eaconfig." + targetname, "property"),

                Tuple.Create(Project.Properties["eaconfig." + module.Configuration.Platform + "." + targetname] ?? "eaconfig." + module.Configuration.Platform + "." + targetname, "eaconfig." + module.Configuration.Platform + "." + targetname, "property"),

                Tuple.Create(BuildOptionSet.GetOptionOrDefault(targetname, String.Empty), targetname, "option"),

                Tuple.Create(PropGroupValue(targetname, PropGroupName(targetname)), PropGroupName(targetname), "property")
            };


            foreach (var targetdef in targets)
            {
                foreach (var target in targetdef.Item1.ToArray())
                {

                    if (Project.TargetExists(target))
                    {
                        var targetInput = new TargetInput();

                        var nantbuildonly = BuildOptionSet.GetOptionOrDefault(target+".nantbuildonly", Project.Properties[PropGroupName(targetname)+"."+target+".nantbuildonly"]);

                        step.TargetCommands.Add(new TargetCommand(target, targetInput, ConvertUtil.ToBooleanOrDefault(nantbuildonly, true)));
                    }
                    else
                    {
                        if (targetdef.Item3 == "option" || target != targetdef.Item2)
                        {
                            Log.Warning.WriteLine(LogPrefix + "target '{0}' defined in {1} '{2}' does not exist", target, targetdef.Item3,targetdef.Item2);
                        }
                    }
                }
            }

            // Commands:
            var command = targetname.Replace("target", "command");
            var commands = new string[] 
            { 
                Project.Properties["eaconfig." + command],
                Project.Properties["eaconfig." + module.Configuration.Platform + "." + command],
                BuildOptionSet.GetOptionOrDefault(command, String.Empty),
                PropGroupValue(commandname)
            };

            foreach (var commandbody in commands)
            {

                var script = commandbody.TrimWhiteSpace();
                if (!String.IsNullOrEmpty(script))
                {
                    var liboutput = module.LibraryOutput();
                    var primaryoutput = module.PrimaryOutput();
                    script = script
                        .Replace("%linkoutputpdbname%", BuildOptionSet.GetOptionOrDefault("linkoutputpdbname", String.Empty))
                        .Replace("%linkoutputmapname%", BuildOptionSet.GetOptionOrDefault("linkoutputmapname", String.Empty))
                        .Replace("%imgbldoutputname%", BuildOptionSet.GetOptionOrDefault("imgbldoutputname", String.Empty))
                        .Replace("%intermediatedir%", module.IntermediateDir.Path)
                        .Replace("%outputdir%", module.OutputDir.Path)
                        .Replace("%outputname%", module.PrimaryOutputName())
                        .Replace("%liboutputname%", liboutput == null ? String.Empty : liboutput.Path)
                        .Replace("%linkoutputname%", primaryoutput)
                        .Replace("%securelinkoutputname%", primaryoutput);

                    step.Commands.Add(new Command(script));
                }
                
            }

            if (step.TargetCommands.Count == 0 && step.Commands.Count == 0)
            {
                return null;
            }

            return step;
        }

        //some packages add header files into set of source file. Clean this up:
        private FileSet RemoveHeadersFromSourceFiles(Module_Native module, FileSet input)
        {
            if(input != null)
            {
            bool remove = false;
            foreach (var pattern in input.Includes)
            {
                if (pattern.Pattern.Value != null && (pattern.Pattern.Value.EndsWith("*", StringComparison.Ordinal) || HEADER_EXTENSIONS.Any(ext => pattern.Pattern.Value.ToLowerInvariant().EndsWith(ext, StringComparison.Ordinal))))
                {
                    remove = true;
                    break;
                }
            }
            if (remove)
            {
                var cleaned = new FileSet();
                cleaned.BaseDirectory = input.BaseDirectory;

                foreach (var fi in input.FileItems)
                {
                    if (HEADER_EXTENSIONS.Contains(Path.GetExtension(fi.Path.Path)))
                    {
                        fi.SetFileDataFlags(FileData.Header);
                        module.ExcludedFromBuild_Files.Include(fi);
                    }
                    else
                    {
                        cleaned.Include(fi);
                    }
                }
                return cleaned;
            }
            }
            return input;
        }

        private FileSet SetupCustomBuildToolFilesObsolete(ProcessableModule module, FileSet input, IDictionary<string, OptionSet> fileToCustomBuildToolMap, ref FileSet custombuildsources)
        {
            var output = input;

            // Support for deprecated custombuild sources:
            if (fileToCustomBuildToolMap != null && input != null)
            {
                var basedir = PathString.MakeNormalized(input.BaseDirectory);
                FileSet inputupdated = new FileSet();
                inputupdated.BaseDirectory = input.BaseDirectory;

                if (custombuildsources == null)
                {
                    custombuildsources = new FileSet();
                }

                foreach (var fileitem in input.FileItems)
                {
                    OptionSet customBuildOptions;
                    if (fileToCustomBuildToolMap.TryGetValue(Path.GetFileName(fileitem.Path.Path), out customBuildOptions))
                    {
                        module.AddTool(CreateCustomBuildTools(module, fileitem, customBuildOptions, basedir, custombuildsources));
                    }
                    else
                    {
                        inputupdated.Include(fileitem);
                    }
                }
                if (inputupdated.Includes.Count > 0)
                {
                    output = inputupdated;
                }
                else
                {
                    output = null;
                }
            }
            return output;
        }

        private void SetDebuglevelObsolete(IModule module, CcCompiler compiler)
        {
            // Getting package level debug level defines into the build
            if (BuildOptionSet.Options["debugflags"] != "off")
            {
                string debugLevel = Project.Properties.GetPropertyOrDefault("package.debuglevel", "0");
                debugLevel = Project.Properties.GetPropertyOrDefault("package." + module.Package.Name + ".debuglevel", debugLevel).TrimWhiteSpace();

                if (debugLevel != "0")
                {
                    compiler.Defines.Add(String.Format("{0}_DEBUG={1}",  module.Package.Name.ToUpperInvariant(), debugLevel));
                }
            }
        }

        private void SetDebuglevelObsolete(IModule module, DotNetCompiler compiler)
        {
            // Getting package level debug level defines into the build
            if (BuildOptionSet.Options["debugflags"] != "off")
            {
                string debugLevel = Project.Properties.GetPropertyOrDefault("package.debuglevel", "0");
                debugLevel = Project.Properties.GetPropertyOrDefault("package." + module.Package.Name + ".debuglevel", debugLevel).TrimWhiteSpace();

                if (debugLevel != "0")
                {
                    compiler.Defines.Add(String.Format("{0}_DEBUG", module.Package.Name.ToUpperInvariant()));
                }
            }
        }


        private IDictionary<string, OptionSet> IntitFileCustomBuildToolMapObsolete()
        {
            IDictionary<string, OptionSet> fileToCustomBuildToolMap = null;

            var source_custom_build = GetModulePropertyValue(".vcproj.custom-build-source").TrimWhiteSpace();

            if (!String.IsNullOrEmpty(source_custom_build))
            {
                if (Log.WarningLevel >= Log.WarnLevel.Deprecation)
                {
                    Log.Warning.WriteLine("Using property '{0}' to define file custom build rules is deprecated. Use '{1}' fileset to define custom build tools. See documantation for details", PropGroupName(".vcproj.custom-build-source"), PropGroupName(".custombuildfiles")); 
                }

                fileToCustomBuildToolMap = new Dictionary<string, OptionSet>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

                foreach (string fileStepPair in source_custom_build.ToArray())
                {
                    string[] pair = fileStepPair.Split(new char[] { ':' });

                    if (pair.Length != 2)
                    {
                        Log.Error.WriteLine("Property '{0}''='{1}' contains invalid association between file name and custom build step name: '{3}'", PropGroupName(".vcproj.custom-build-source"), source_custom_build, fileStepPair);
                        throw new BuildException("Failed generating custom build step");
                    }
                    if (String.IsNullOrEmpty(pair[0]) || String.IsNullOrEmpty(pair[1]))
                    {
                        Log.Error.WriteLine("Property '{0}''='{1}' contains invalid association between file name and custom build step name: '{3}'", PropGroupName(".vcproj.custom-build-source"), source_custom_build, fileStepPair);
                        throw new BuildException("Failed generating custom build step");
                    }

                    string stepFile = pair[0].TrimWhiteSpace();
                    string stepName = pair[1].TrimWhiteSpace();

                    OptionSet stepInfo;
                    if (!fileToCustomBuildToolMap.TryGetValue(stepFile, out stepInfo))
                    {
                        stepInfo = new OptionSet();
                        fileToCustomBuildToolMap.Add(stepFile, stepInfo);

                        stepInfo.Options["name"] = "custom rule for "+ stepFile;
                        stepInfo.Options["stepname"] = stepName;
                    }

                    OptionSetUtil.AppendOption(stepInfo, "command", GetModulePropertyValue(".vcproj.custom-build-tool." + stepName, String.Empty));
                    OptionSetUtil.AppendOption(stepInfo, "outputs", GetModulePropertyValue(".vcproj.custom-build-outputs." + stepName, String.Empty));
                    OptionSetUtil.AppendOption(stepInfo, "description", GetModulePropertyValue(".vcproj.custom-build-description." + stepName, String.Empty));
                    OptionSetUtil.AppendOption(stepInfo, "input-filesets", GetModulePropertyValue(".vcproj.custom-build-dependencies." + stepName, String.Empty));
                }
            }
            return fileToCustomBuildToolMap;
        }

        private BuildStep GetCustomBuildStep(uint type)
        {
            // Add Custom build steps. No Target exists for these steps at the moment:

            var source_custom_build = GetModulePropertyValue(".vcproj.custom-build-source").TrimWhiteSpace();

            string customcmdline = PropGroupValue("vcproj.custom-build-tool");

            if (!String.IsNullOrWhiteSpace(customcmdline))
            {
                // Deprecated way to define custom build step per file. If sources are defined bailout from here!
                // Data will be processed in IntitFileCustomBuildToolMapObsolete();
                
                if (!String.IsNullOrEmpty(source_custom_build))
                {
                    return null;
                }
                BuildStep step = new BuildStep("custombuild", type);

                step.Before = Project.Properties.GetPropertyOrDefault(PropGroupName("custom-build-before-targets"), String.Empty);
                step.After = Project.Properties.GetPropertyOrDefault(PropGroupName("custom-build-after-targets"), String.Empty);

                // default type is postbuild, but if before or after target are set we clear the postbuild flag
                if (step.Before != String.Empty || step.After != String.Empty)
                {
                    step.ClearType(BuildStep.PostBuild);
                }

                // set build type for native nant builds based on before target
                string beforetarget = step.Before.ToLower();
                if (beforetarget == "link")
                {
                    step.SetType(BuildStep.PreLink);
                }
                else if (beforetarget == "build")
                {
                    step.SetType(BuildStep.PreBuild);
                }
                else if (beforetarget == "run") 
                { 
                    // handled by eaconfig-run
                }
                else if (Log.Level >= Log.LogLevel.Detailed)
                {
                    Log.Warning.WriteLine(String.Format(
                        "Before value '{0}' not supported for native NAnt builds.", step.Before));
                }

                // set build type for native nant builds based on after target
                string aftertarget = step.After.ToLower();
                if (aftertarget == "build")
                {
                    step.SetType(BuildStep.PostBuild);
                }
                else if (aftertarget == "run")
                {
                    // handled by eaconfig-run
                }
                else if (Log.Level >= Log.LogLevel.Detailed)
                {
                    Log.Warning.WriteLine(String.Format(
                        "After value '{0}' not supported for native NAnt builds.", step.After));
                }

                step.Commands.Add(new Command(customcmdline));

                step.OutputDependencies = PropGroupValue("vcproj.custom-build-outputs").LinesToArray().Select(file => PathString.MakeNormalized(file)).OrderedDistinct().ToList();
                step.InputDependencies = PropGroupValue("vcproj.custom-build-dependencies").LinesToArray().Select(file => PathString.MakeNormalized(file)).OrderedDistinct().ToList();
                var inputs = PropGroupFileSet("vcproj.custom-build-dependencies");
                if (inputs != null)
                {
                    step.InputDependencies.AddRange(inputs.FileItems.Select(fi => fi.Path));
                }

                return step;
            }
            return null;
        }

        private bool AddOption(Tool tool, string name, string prefix = "", bool useOptionProperty = false, Func<string, string> convert = null)
        {
            string value = GetOptionString(name, BuildOptionSet.Options, name, useOptionProperty).TrimWhiteSpace();

            if (convert == null)
            {
                if (value.OptionToBoolean())
                {
                    tool.Options.Add(prefix);
                    return true;
                }
            }
            else
            {
                value = convert(value);
                if (!String.IsNullOrWhiteSpace(value))
                {
                    tool.Options.Add(prefix + convert(value));
                    return true;
                }
            }
            return false;
        }

        private bool AddOptionOrPropertyValue(Tool tool, string name, string prefix = "", Func<string, string> convert = null)
        {
            string value = GetModulePropertyValue(name);

            if (String.IsNullOrEmpty(value))
            {
                value = GetOptionString(name, BuildOptionSet.Options, useOptionProperty: false).TrimWhiteSpace();
            }

            if (convert == null)
            {
                if (value.OptionToBoolean())
                {
                    tool.Options.Add(prefix);
                    return true;
                }
            }
            else
            {
                value = convert(value);
                if (!String.IsNullOrWhiteSpace(value))
                {
                    tool.Options.Add(prefix + convert(value));
                    return true;
                }
            }
            return false;
        }

        private void ExecuteProcessSteps(ProcessableModule module, Tool tool, OptionSet buildTypeOptionSet, IEnumerable<string> steps)
        {
            foreach (string step in steps)
            {
                var task = Project.TaskFactory.CreateTask(step, Project);
                if (task != null)
                {
                    IBuildModuleTask bmt = task as IBuildModuleTask;
                    if (bmt != null)
                    {
                        bmt.Module = module;
                        bmt.Tool = tool;
                        bmt.BuildTypeOptionSet = buildTypeOptionSet;
                    }
                    task.Execute();
                }
                else
                {
                    Project.ExecuteTargetIfExists(step, force: true);
                }
            }
        }

        private BuildTool CreateCustomBuildTools(ProcessableModule module, FileItem fileitem, PathString basedir, FileSet sources = null, FileSet objects = null, List<PathString> includedirs = null)
        {
            OptionSet fileOptions = null;
            if (!String.IsNullOrEmpty(fileitem.OptionSetName))
            {
                if (Project.NamedOptionSets.TryGetValue(fileitem.OptionSetName, out fileOptions))
                {
                    return CreateCustomBuildTools(module, fileitem, fileOptions, basedir, sources, objects, includedirs);
                }
            }
            return null;
        }

        private BuildTool CreateCustomBuildTools(ProcessableModule module, FileItem fileitem, OptionSet fileOptions, PathString basedir, FileSet sources = null, FileSet objects = null, List<PathString> includedirs=null)
        {
            if (fileOptions == null)
            {
                return null;
            }

            var filepath = fileitem.Path.Path;
            var filename = Path.GetFileNameWithoutExtension(filepath);
            var fileext = Path.GetExtension(filepath);
            var filedir = Path.GetDirectoryName(filepath);
            var filebasedir = basedir.Path;
            var filereldir = PathUtil.RelativePath(filedir, filebasedir, failOnError: true);

            var execs = new List<Tuple<string, string>>();

            var build_tasks = OptionSetUtil.GetOptionOrDefault(fileOptions, "build.tasks", null).ToArray();
            if (build_tasks.Count == 0)
            {
                build_tasks.Add(String.Empty);
            }

            foreach (string buildtask in build_tasks)
            {
                var buildtask_prefix = String.IsNullOrEmpty(buildtask) ? String.Empty : buildtask + ".";
                var command = OptionSetUtil.GetOptionOrDefault(fileOptions, buildtask_prefix + "command", String.Empty)
                        .Replace("%filepath%", filepath)
                        .Replace("%filename%", filename)
                        .Replace("%fileext%", fileext)
                        .Replace("%filedir%", filedir)
                        .Replace("%filebasedir%", filebasedir)
                        .Replace("%filereldir%", filereldir)
                        .Replace("%intermediatedir%", module.IntermediateDir.Path)
                        .Replace("%outputdir%", module.OutputDir.Path);
                // IM: Dice is using empty commands in VisualStudio just to have files act as input dependency for a project.
                // Do not add command.TrimWhiteSpace()  
                if (!String.IsNullOrEmpty(command))
                {
                    execs.Add(Tuple.Create<string, string>(null, command));
                }

                var exec = OptionSetUtil.GetOptionOrDefault(fileOptions, buildtask_prefix + "executable", String.Empty).TrimWhiteSpace().Quote();
                var opt = OptionSetUtil.GetOptionOrDefault(fileOptions, buildtask_prefix + "options", String.Empty).TrimWhiteSpace();

                if (!String.IsNullOrEmpty(exec))
                {
                    exec = exec.Replace("%filepath%", filepath)
                    .Replace("%filename%", filename)
                    .Replace("%fileext%", fileext)
                    .Replace("%filedir%", filedir)
                    .Replace("%filebasedir%", filebasedir)
                    .Replace("%filereldir%", filereldir)
                    .Replace("%intermediatedir%", module.IntermediateDir.Path)
                    .Replace("%outputdir%", module.OutputDir.Path);

                    if (!String.IsNullOrEmpty(opt))
                    {
                        opt = opt.Replace("%filepath%", filepath)
                                  .Replace("%filename%", filename)
                                  .Replace("%fileext%", fileext)
                                  .Replace("%filedir%", filedir)
                                  .Replace("%filebasedir%", filebasedir)
                                  .Replace("%filereldir%", filereldir)
                                  .Replace("%intermediatedir%", module.IntermediateDir.Path)
                                  .Replace("%outputdir%", module.OutputDir.Path);
                    }
                    else
                    {
                        opt = string.Empty;
                    }

                    execs.Add(Tuple.Create<string, string>(exec, opt));
                }
            }

            var outputs = OptionSetUtil.GetOptionOrDefault(fileOptions, "outputs", "")
                        .Replace("%filepath%", filepath)
                        .Replace("%filename%", filename)
                        .Replace("%fileext%", fileext)
                        .Replace("%filedir%", filedir)
                        .Replace("%filebasedir%", filebasedir)
                        .Replace("%filereldir%", filereldir)
                        .Replace("%intermediatedir%", module.IntermediateDir.Path)
                        .Replace("%outputdir%", module.OutputDir.Path)
                        .LinesToArray().OrderedDistinct().ToList();

            List<PathString> createoutputdirs = null;
            if (outputs.Count > 0 && fileOptions.GetBooleanOptionOrDefault("createoutputdirs", false))
            {
                createoutputdirs = new List<PathString>();
                foreach (var output in outputs)
                {
                    createoutputdirs.Add(new PathString(Path.GetDirectoryName(output), PathString.PathState.Directory));
                }
            }

            var description = OptionSetUtil.GetOptionOrDefault(fileOptions, "description", "")
                            .Replace("%filepath%", filepath)
                            .Replace("%filename%", filename)
                            .Replace("%fileext%", fileext)
                            .Replace("%filedir%", filedir)
                            .Replace("%filebasedir%", filebasedir)
                            .Replace("%filereldir%", filereldir);

            var treatOutputAsContent = fileOptions.GetBooleanOption("treatoutputascontent");

            var toolname = OptionSetUtil.GetOptionOrFail(Project, fileOptions, "name");

            var tooltype = Tool.TypePreCompile;

            BuildTool tool = null;

            if (execs.Count > 1)
            {
                // Convert to script
                var script = new StringBuilder();
                foreach (var entry in execs)
                {
                    if (entry.Item1 == null)
                    {
                        script.AppendLine(entry.Item2);
                    }
                    else
                    {
                        script.Append(entry.Item1);
                        script.Append(" ");
                        script.AppendLine(entry.Item2.LinesToArray().ToString(" "));
                    }
                }

                tool = new BuildTool(Project, toolname, script.ToString(), tooltype, description, outputdir: module.OutputDir, intermediatedir: module.IntermediateDir, createdirectories: createoutputdirs, treatoutputascontent: treatOutputAsContent);
            }
            else if (execs.Count == 1)
            {
                if (execs[0].Item1 == null)
                {
                    // Script
                    tool = new BuildTool(Project, toolname, execs[0].Item2, tooltype, description, outputdir: module.OutputDir, intermediatedir: module.IntermediateDir, createdirectories: createoutputdirs, treatoutputascontent: treatOutputAsContent);
                }
                else
                {
                    // Exec + options
                    tool = new BuildTool(Project, toolname, PathString.MakeNormalized(execs[0].Item1, PathString.PathParam.NormalizeOnly), tooltype, description, outputdir: module.OutputDir, intermediatedir: module.IntermediateDir, createdirectories: createoutputdirs, treatoutputascontent: treatOutputAsContent);
                    tool.Options.AddRange(execs[0].Item2.LinesToArray());
                }
            }
            else
            {
                Log.Warning.WriteLine(LogPrefix + "Custom tool '{0}' has no definition of either command or 'executable + options'", toolname);
                return null;
            }

            fileitem.SetFileData(tool);

            ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["preprocess"].ToArray());

            tool.Files.Include(fileitem, baseDir: basedir.Path, failonmissing: false);


            foreach (var output in outputs)
            {
                tool.OutputDependencies.IncludePatternWithMacro(output, failonmissing: false);
            }

            var inputs = OptionSetUtil.GetOptionOrDefault(fileOptions, "inputs", "")
                        .Replace("%filepath%", filepath)
                        .Replace("%filename%", filename)
                        .Replace("%fileext%", fileext)
                        .Replace("%filedir%", filedir)
                        .Replace("%filebasedir%", filebasedir)
                        .Replace("%filereldir%", filereldir);

            foreach (var input in inputs.LinesToArray().OrderedDistinct())
            {
                tool.InputDependencies.IncludePatternWithMacro(input, failonmissing: false);
            }

            foreach (var filesetName in OptionSetUtil.GetOptionOrDefault(fileOptions, "input-filesets", "").ToArray().OrderedDistinct())
            {
                FileSet fs;
                if(Project.NamedFileSets.TryGetValue(filesetName, out fs))
                {
                    tool.InputDependencies.AppendIfNotNull(fs);
                }
                else
                {
                    Log.Warning.WriteLine("Input dependency fileset '{0}' specified in custom build rule '{1}' does not exist", filesetName, tool.ToolName);
                }
            }


            var native_module = module as Module_Native;

            if (native_module != null)
            {
                var sourcefiles = OptionSetUtil.GetOptionOrDefault(fileOptions, "sourcefiles", "")
                        .Replace("%filepath%", filepath)
                        .Replace("%filename%", filename)
                        .Replace("%fileext%", fileext)
                        .Replace("%filedir%", filedir)
                        .Replace("%filebasedir%", filebasedir)
                        .Replace("%filereldir%", filereldir);

                if (sources != null)
                {
                    foreach (var src in sourcefiles.LinesToArray().OrderedDistinct())
                    {
                        sources.Include(src, failonmissing: false, data: new CustomBuildToolFlags(fileOptions.GetBooleanOption(FrameworkProperties.BulkBuildPropertyName) ? CustomBuildToolFlags.AllowBulkbuild : 0));
                    }
                }
                var headerfiles = OptionSetUtil.GetOptionOrDefault(fileOptions, "headerfiles", "")
                        .Replace("%filepath%", filepath)
                        .Replace("%filename%", filename)
                        .Replace("%fileext%", fileext)
                        .Replace("%filedir%", filedir)
                        .Replace("%filebasedir%", filebasedir)
                        .Replace("%filereldir%", filereldir);

                foreach (var hf in headerfiles.LinesToArray().OrderedDistinct())
                {
                    native_module.ExcludedFromBuild_Files.Include(hf, failonmissing: false, data: new FileData(null, flags: FileData.Header));
                }

                if (objects != null)
                {
                    var objectfiles = OptionSetUtil.GetOptionOrDefault(fileOptions, "objectfiles", "")
                            .Replace("%filepath%", filepath)
                            .Replace("%filename%", filename)
                            .Replace("%fileext%", fileext)
                            .Replace("%filedir%", filedir)
                            .Replace("%filebasedir%", filebasedir)
                            .Replace("%filereldir%", filereldir);

                    foreach (var of in objectfiles.LinesToArray().OrderedDistinct())
                    {
                        objects.Include(of, baseDir: filebasedir, failonmissing: false);
                    }
                }

                if (includedirs != null)
                {
                    var includedirsStr = OptionSetUtil.GetOptionOrDefault(fileOptions, "includedirs", "")
                            .Replace("%filepath%", filepath)
                            .Replace("%filename%", filename)
                            .Replace("%fileext%", fileext)
                            .Replace("%filedir%", filedir)
                            .Replace("%filebasedir%", filebasedir)
                            .Replace("%filereldir%", filereldir).TrimWhiteSpace();

                    includedirs.AddRange(includedirsStr.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct());
                }

            }
            ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["postprocess"].ToArray());

            return tool;
        }


        private bool IsBulkBuild(IModule module)
        {
            bool bulkbuild = Project.Properties.GetBooleanProperty(FrameworkProperties.BulkBuildPropertyName);

            if (bulkbuild)  // Check if current module is specified in excluded list
            {
                var bulkBuildExcluded = Project.Properties[FrameworkProperties.BulkBuildExcludedPropertyName].TrimWhiteSpace();
                if (!String.IsNullOrEmpty(bulkBuildExcluded))
                {
                    foreach (var item in bulkBuildExcluded.ToArray())
                    {
                        var components = item.Trim(new char[] {'"', '\''}).ToArray(new char[]{'/'});

                        if (components.Count == 1 && module.Package.Name == components[0]) // Case: "PackageName". All modules in package excluded
                        {
                            bulkbuild = false;
                        }
                        else if (components.Count == 2 && module.Package.Name == components[0] && module.Name == components[1]) // Case: "PackageName/ModuleName"
                        {
                            bulkbuild = false;
                        }
                        else if (components.Count == 3 && module.Package.Name == components[0] && module.BuildGroup.ToString() == components[1] && module.Name == components[1]) // Case: "PackageName/group/ModuleName"
                        {
                            bulkbuild = false;
                        }
                        else if (components.Count > 3)
                        {
                            Log.Warning.WriteLine("Invalid entry in the '{0}' property: '{1}'. Following syntax is accepted: 'PackageName', or 'PackageName/ModuleName', or 'PackageName/group/ModuleName'.", FrameworkProperties.BulkBuildExcludedPropertyName, item);
                        }
                    }
                }
            }

            return bulkbuild;
        }

        #endregion

        #region Make Module


        #endregion

        #region Utility Module


        #endregion



        #region DotNetModule


        private string GetFrameworkPrefix(Module_DotNet module)
        {
            if (module.IsKindOf(Module.CSharp))
            {
                return "csproj";
            }
            else if (module.IsKindOf(Module.FSharp))
            {
                return "fsproj";
            }

            Error.Throw(Project, "Process(Module_DotNet)", "[{0}] module '{1}' has unknown .Net language setting, supported languages are C3 and F#.", module.Package.Name, module.Name);

            return string.Empty;
        }

        #endregion

        #region NativeModule

        private AsmCompiler CreateAsmTool(OptionSet buildOptionSet, Module_Native module)
        {
            var workingdir = buildOptionSet.Options["as.workingdir"];

            AsmCompiler compiler = new AsmCompiler(PathString.MakeNormalized(GetOptionString("as", BuildOptionSet.Options), PathString.PathParam.NormalizeOnly), workingdir == null ? null : new PathString(workingdir, PathString.PathState.Undefined));
            compiler.Defines.AddRange(GetOptionString("as.defines", BuildOptionSet.Options, ".defines").LinesToArray());
            compiler.Options.AddRange(GetOptionString("as.options", BuildOptionSet.Options, ".warningsuppression").LinesToArray());

            // Add module include directories, system include dirs are added later
            compiler.IncludeDirs.AddRange(GetModulePropertyValue(".includedirs",
                                Path.Combine(Project.Properties["package.dir"], "include"),
                                Path.Combine(Project.Properties["package.dir"], Project.Properties["eaconfig." + module.BuildGroup + ".sourcedir"]))
                                .LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct());

            var removedefines = GetModulePropertyValue("remove.defines").ToArray();
            if (removedefines.Count > 0)
            {
                compiler.Defines.RemoveAll(x => removedefines.Contains(x));
            }
            var removeasmoptions = GetModulePropertyValue("remove.as.options").LinesToArray();
            if (removeasmoptions.Count > 0)
            {
                compiler.Options.RemoveAll(x => removeasmoptions.Contains(x));
            }

            compiler.ObjFileExtension = GetOptionString("as.objfile.extension", buildOptionSet.Options) ?? ".obj";

            return compiler;
        }

        private CcCompiler CreateCcTool(OptionSet buildOptionSet, Module_Native module)
        {
            CcCompiler.PrecompiledHeadersAction pch = CcCompiler.PrecompiledHeadersAction.NoPch;

            if (buildOptionSet.GetBooleanOption("create-pch"))
            {
                pch = CcCompiler.PrecompiledHeadersAction.CreatePch;

                if (buildOptionSet.GetBooleanOption("use-pch"))
                {
                    Log.Warning.WriteLine(LogPrefix + "[{0}] {1} - {2} - both 'use-pch' and 'create-pch' options are specified. 'create-pch' option will be used.", module.Package.Name, module.Name, buildOptionSet.Options["buildset.name"]);
                }
            }
            else if (buildOptionSet.GetBooleanOption("use-pch"))
            {
                pch = CcCompiler.PrecompiledHeadersAction.UsePch;
            }

            var workingdir = buildOptionSet.Options["cc.workingdir"];

            CcCompiler compiler = new CcCompiler(PathString.MakeNormalized(GetOptionString("cc", buildOptionSet.Options), PathString.PathParam.NormalizeOnly), pch, workingdir == null ? null : new PathString(workingdir,PathString.PathState.Undefined));

            compiler.Defines.AddRange(GetOptionString(compiler.ToolName + ".defines", buildOptionSet.Options, ".defines").LinesToArray());

            compiler.CompilerInternalDefines.AddRange(GetOptionString(compiler.ToolName + ".compilerinternaldefines", BuildOptionSet.Options, ".compilerinternaldefines").LinesToArray());

#if FRAMEWORK_PARALLEL_TRANSITION
            if (module.Configuration.System == "ps3" && (module.Configuration.Compiler == "gcc" || module.Configuration.SubSystem == ".spu"))
            {
                compiler.Options.AddRange(GetOptionString(compiler.ToolName + ".options", buildOptionSet.Options).LinesToArray());


                string systemincludedirs;
                if (BuildOptionSet.Options.TryGetValue("cc.systemincludedirs", out systemincludedirs))
                {
                    foreach (var includedir in systemincludedirs.LinesToArray().OrderedDistinct())
                    {
                        compiler.Options.Add(String.Format("-isystem {0}", includedir.Quote()));
                    }
                }

                compiler.Options.AddRange(GetModulePropertyValue(".warningsuppression").LinesToArray());
            }
            else
            {
                compiler.Options.AddRange(GetOptionString(compiler.ToolName + ".options", buildOptionSet.Options, ".warningsuppression").LinesToArray());
            }
#else
            compiler.Options.AddRange(GetOptionString(compiler.ToolName + ".options", buildOptionSet.Options, ".warningsuppression").LinesToArray());
#endif

            compiler.UsingDirs.AddRange(GetOptionString(compiler.ToolName + ".usingdirs", buildOptionSet.Options, ".usingdirs").LinesToArray().Select(dir => PathString.MakeNormalized(dir)).Distinct());

            // Add module include directories, system include dirs are added later
            compiler.IncludeDirs.AddRange(GetModulePropertyValue(".includedirs",
                                Path.Combine(Project.Properties["package.dir"], "include"),
                                Path.Combine(Project.Properties["package.dir"], Project.Properties["eaconfig." + module.BuildGroup + ".sourcedir"]))
                                .LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct());

            AddModuleFiles(compiler.Assemblies, module, ".assemblies");

            SetupComAssembliesAssemblies(compiler, module);

            SetupWebReferences(compiler, module);

            var removedefines = GetModulePropertyValue("remove.defines").ToArray();
            if (removedefines.Count > 0)
            {
                compiler.Defines.RemoveAll(x => removedefines.Contains(x));
            }
            var removeccoptions = GetModulePropertyValue("remove.cc.options").LinesToArray();
            if (removeccoptions.Count > 0)
            {
                compiler.Options.RemoveAll(x => removeccoptions.Contains(x));
            }

            compiler.ObjFileExtension = GetOptionString(compiler.ToolName + ".objfile.extension", buildOptionSet.Options) ?? ".obj";

            return compiler;
        }

        private Linker CreateLinkTool(OptionSet buildOptionSet, Module_Native module)
        {
            var workingdir = buildOptionSet.Options["link.workingdir"];

            Linker linker = new Linker(PathString.MakeNormalized(GetOptionString("link", BuildOptionSet.Options), PathString.PathParam.NormalizeOnly), workingdir == null ? null : new PathString(workingdir, PathString.PathState.Undefined));

            linker.ImportLibOutputDir = GetLibOutputDir(module);
            module.OutputDir = GetBinOutputDir(module);

            string outputname;
            string replacename;
            string outputextension;
            PathString linkoutputdir;
            SetLinkOutput(module, out outputname, out outputextension, out linkoutputdir, out replacename); // Redefine output dir and output name based on linkoutputname property.

            linker.OutputName = outputname;
            linker.OutputExtension = outputextension;
            linker.LinkOutputDir = linkoutputdir;

            linker.Options.AddRange(GetOptionString(linker.ToolName + ".options", BuildOptionSet.Options)
                                    .LinesToArray());


            var removelinkoptions = GetModulePropertyValue("remove.link.options").LinesToArray();
            if (removelinkoptions.Count > 0)
            {
                linker.Options.RemoveAll(x => removelinkoptions.Contains(x));
            }

            string[] OPTION_NAME_LIST = new string[] { 
                "linkoutputname", 
                "linkoutputpdbname", 
                "linkoutputmapname", 
                "imgbldoutputname", 
                "securelinkoutputname" 
            };

            foreach (string option in OPTION_NAME_LIST)
            {
                string value;
                if (buildOptionSet.Options.TryGetValue(option, out value))
                {
                    if (option == "linkoutputname")
                    {
                        // To avoid recursive expansion of linkoutputname template.
                        value = Path.Combine(linker.LinkOutputDir.Path, linker.OutputName + linker.OutputExtension);
                    }
                    else
                    {
                        value = value
                            .Replace("%intermediatedir%", module.IntermediateDir.Path)
                            .Replace("%outputdir%", module.OutputDir.Path)
                            .Replace("%outputname%", replacename);
                    }

                    value = PathNormalizer.Normalize(value, false);

                    for (int i = 0; i < linker.Options.Count; i++)
                    {
                        linker.Options[i] = linker.Options[i].Replace("%" + option + "%", value);
                    }
                }
            }

            string impllibtemplate;
            if (buildOptionSet.Options.TryGetValue("impliboutputname", out impllibtemplate))
            {
                if (!String.IsNullOrWhiteSpace(impllibtemplate))
                {
                    impllibtemplate = impllibtemplate
                        .Replace("%intermediatedir%", module.IntermediateDir.Path)
                        .Replace("%outputdir%", module.OutputDir.Path)
                        .Replace("%outputlibdir%", linker.ImportLibOutputDir.Path)
                        .Replace("%outputname%", replacename);

                    linker.ImportLibFullPath = PathString.MakeNormalized(impllibtemplate);

                    for (int i = 0; i < linker.Options.Count; i++)
                    {
                        linker.Options[i] = linker.Options[i].Replace("%impliboutputname%", linker.ImportLibFullPath.Path);
                    }

                }
            }

            linker.LibraryDirs.AddRange(GetOptionString(linker.ToolName + ".librarydirs", BuildOptionSet.Options, ".librarydirs")
                                        .LinesToArray()
                                        .Select(dir => PathString.MakeNormalized(dir, PathString.PathParam.NormalizeOnly)).OrderedDistinct());

            //Options can contain lib names without a path (system libs, for example), don't normalize with making full path, and add asIs to the fileset:
            foreach (var library in GetOptionString(linker.ToolName + ".libraries", BuildOptionSet.Options, ".libs")
                                        .LinesToArray()
                                        .Select(lib => PathString.MakeNormalized(lib, PathString.PathParam.NormalizeOnly)).OrderedDistinct())
            {
                linker.Libraries.Include(library.Path, asIs:true);
            }

            // Some build scripts explicitly include libraries from other modules in the same package
            if (ModuleOutputNameMapping != null)
            {
                FileSet libstomap = new FileSet();
                AddModuleFiles(libstomap, module, ".libs");
                linker.Libraries.IncludeWithBaseDir(MapDependentOutputFiles(libstomap, ModuleOutputNameMapping, module.Package, "lib"));
            }
            else
            {
                AddModuleFiles(linker.Libraries, module, ".libs");
            }

            AddModuleFiles(linker.DynamicLibraries, module, ".dlls");

            AddModuleFiles(linker.ObjectFiles, module, ".objects");

            linker.UseLibraryDependencyInputs = buildOptionSet.GetBooleanOptionOrDefault("uselibrarydependencyinputs", linker.UseLibraryDependencyInputs);

            return linker;
        }

        private PostLink CreatePostLinkTool(OptionSet buildOptionSet, IModule module, Linker linker)
        {
            string program = GetOptionString("link.postlink.program", BuildOptionSet.Options).TrimWhiteSpace();

            if (!String.IsNullOrEmpty(program))
            {
                program = program
                    .Replace("%outputlibdir%", linker.ImportLibOutputDir.Path)
                    .Replace("%intermediatedir%", module.IntermediateDir.Path)
                    .Replace("%outputdir%", module.OutputDir.Path)
                    .Replace("%outputname%", linker.OutputName);

                PostLink postlink = new PostLink(PathString.MakeNormalized(program, PathString.PathParam.NormalizeOnly));
                var primaryoutput = Path.Combine(linker.LinkOutputDir.Path, linker.OutputName + linker.OutputExtension);
                
                postlink.Options.AddRange((GetOptionString("link.postlink.commandline", BuildOptionSet.Options)??String.Empty)
                    .Replace("%linkoutputpdbname%", BuildOptionSet.GetOptionOrDefault("linkoutputpdbname", String.Empty))
                    .Replace("%linkoutputmapname%", BuildOptionSet.GetOptionOrDefault("linkoutputmapname", String.Empty))
                    .Replace("%imgbldoutputname%", BuildOptionSet.GetOptionOrDefault("imgbldoutputname", String.Empty))
                    .Replace("%outputlibdir%", linker.ImportLibOutputDir.Path)
                    .Replace("%intermediatedir%", module.IntermediateDir.Path)
                    .Replace("%outputdir%", module.OutputDir.Path)
                    .Replace("%outputname%", linker.OutputName)
                    .Replace("%linkoutputname%", primaryoutput)
                    .Replace("%securelinkoutputname%", primaryoutput)
                    .LinesToArray());
                return postlink;
            }
            return null;
        }

        private Librarian CreateLibTool(OptionSet buildOptionSet, IModule module)
        {
            var workingdir = buildOptionSet.Options["lib.workingdir"];

            Librarian librarian = new Librarian(PathString.MakeNormalized(GetOptionString("lib", BuildOptionSet.Options), PathString.PathParam.NormalizeOnly), workingdir == null ? null : new PathString(workingdir, PathString.PathState.Undefined));

            librarian.OutputName =  GetOutputName(module);
            module.OutputDir = GetLibOutputDir(module);

            librarian.Options.AddRange(GetOptionString(librarian.ToolName + ".options", BuildOptionSet.Options).LinesToArray());
            librarian.ObjectFiles.AppendWithBaseDir(PropGroupFileSet(".objects"));
            librarian.ObjectFiles.AppendWithBaseDir(PropGroupFileSet(".objects." + module.Configuration.System));

            string mappedlibname = null;
            if (ModuleOutputNameMapping != null)
            {
                if (!ModuleOutputNameMapping.Options.TryGetValue("liboutputname", out mappedlibname))
                {
                    mappedlibname = null;
                }
            }

            var legacyOutputName = librarian.OutputName;
            string value;
            if (buildOptionSet.Options.TryGetValue("liboutputname", out value))
            {
                if (mappedlibname != null)
                {
                    value = value.Replace("%outputname%", mappedlibname);
                    legacyOutputName = mappedlibname.Replace("%outputname%", librarian.OutputName);
                }

                value = value.Replace("%intermediatedir%", module.IntermediateDir.Path).Replace("%outputdir%", module.OutputDir.Path).Replace("%outputname%", librarian.OutputName);

#if FRAMEWORK_PARALLEL_TRANSITION
                if (module.Configuration.Compiler == "vc")
                {
                    value = PathNormalizer.Normalize(value, true);
                }
#else
                value = PathNormalizer.Normalize(value, true);
#endif

                for (int i = 0; i < librarian.Options.Count; i++)
                {
                    librarian.Options[i] = librarian.Options[i]
                        .Replace("%liboutputname%", value)
                        .Replace("%outputname%", legacyOutputName);
                }

                string finalOutputname;
                string finalExtension;
                GetOutputNameAndExtension(value, librarian.OutputName, out finalOutputname, out finalExtension);

                module.OutputDir = new PathString(Path.GetDirectoryName(value), PathString.PathState.Normalized | PathString.PathState.Normalized | PathString.PathState.Directory);
                librarian.OutputName = finalOutputname;
                librarian.OutputExtension = finalExtension;
            }
            else // Backwards compatibility
            {

                if (mappedlibname != null)
                {
                    librarian.OutputName = mappedlibname.Replace("%outputname%", librarian.OutputName);
                }

                var defaultliboutput = ("%outputdir%/"+Project.Properties["lib-prefix"]+"%outputname%"+ Project.Properties["lib-suffix"]).Replace("%outputdir%", module.OutputDir.Path).Replace("%outputname%", librarian.OutputName);

                for (int i = 0; i < librarian.Options.Count; i++)
                {
                    librarian.Options[i] = librarian.Options[i]
                        .Replace("%intermediatedir%", module.IntermediateDir.Path)
                        .Replace("%outputdir%", module.OutputDir.Path)
                        .Replace("%outputname%", librarian.OutputName)
                        .Replace("%liboutputname%", defaultliboutput);                    
                }

                string outputfile = Project.Properties["lib-prefix"] + librarian.OutputName + Project.Properties["lib-suffix"];
                

                string finalOutputname;
                string finalExtension;
                GetOutputNameAndExtension(outputfile, librarian.OutputName, out finalOutputname, out finalExtension);
                librarian.OutputName = finalOutputname;
                librarian.OutputExtension = finalExtension;
            }

            var removeliboptions = GetModulePropertyValue("remove.lib.options").LinesToArray();
            if (removeliboptions.Count > 0)
            {
                librarian.Options.RemoveAll(x => removeliboptions.Contains(x));
            }

            return librarian;
        }

        private FileSet SetupBulkbuild(Module_Native module, CcCompiler compiler, FileSet cc_sources, FileSet custombuildsources, out FileSet excludedSources)
        {
            excludedSources = null;
            bool bulkbuild = IsBulkBuild(module);

            if (bulkbuild)
            {

                if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.sourcefiles")) &&
                    PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
                {
                    Error.Throw(Project, "VerifyBulkBuildSetup", "You cannot have both '{0}' and '{1}' defined.", PropGroupName("bulkbuild.sourcefiles"), PropGroupName("bulkbuild.filesets"));
                }
                if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
                {
                    foreach (string filesetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
                    {
                        if (!FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild." + filesetName + ".sourcefiles")))
                        {
                            Error.Throw(Project, "VerifyBulkBuildSetup", "FileSet: '{0} was specified in '{1} but it doesn't exits..",
                                        PropGroupName("bulkbuild." + filesetName + ".sourcefiles"),
                                        PropGroupName("bulkbuild.filesets"));
                        }
                    }
                }
            }

            bool isReallyBulkBuild = false;

            string bulkbuildOutputdir = Path.Combine(module.Package.PackageConfigBuildDir.Path,  module.Package.Project.Properties["eaconfig."+module.BuildGroup+".outputfolder"].TrimLeftSlash()??String.Empty,"source");

            FileSet source = new FileSet();
            source.FailOnMissingFile = false;

            if (bulkbuild)
            {
                excludedSources = new FileSet();

                // As of eaconfig-1.21.01, unless explicitly specified, we're now NOT enabling partial
                // BulkBuild.  Metrics gathered on Def Jam and C&C show that it's just TOO slow. 
                bool enablePartialBulkBuild = Project.Properties.GetBooleanPropertyOrDefault("package.enablepartialbulkbuild",
                                                Project.Properties.GetBooleanProperty(PropGroupName("enablepartialbulkbuild")));

                List<FileItem> custombuildLooseSources = null;
                List<FileItem> custombuildBulkBuildSources = null;
                if (custombuildsources != null)
                {
                    custombuildLooseSources = new List<FileItem>();
                    custombuildBulkBuildSources = new List<FileItem>();

                    foreach (var cbFi in custombuildsources.FileItems)
                    {
                        var flags = cbFi.Data as CustomBuildToolFlags;
                        if (flags != null && flags.IsKindOf(CustomBuildToolFlags.AllowBulkbuild))
                        {
                            cbFi.Data = null;
                            custombuildBulkBuildSources.Add(cbFi);
                            enablePartialBulkBuild = true;
                        }
                        else
                        {
                            cbFi.Data = null;
                            custombuildLooseSources.Add(cbFi);
                        }
                    }
                }

                FileSet manualSource = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.manual.sourcefiles"));
                FileSet manualSourceCfg = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.manual.sourcefiles." + module.Configuration.System));


                if (manualSource != null || manualSourceCfg != null)
                {
                    isReallyBulkBuild = true;

                    manualSource.SetFileDataFlags(FileData.BulkBuild);
                    manualSourceCfg.SetFileDataFlags(FileData.BulkBuild);

                    FileSetUtil.FileSetAppendWithBaseDir(source, manualSource);
                    FileSetUtil.FileSetAppendWithBaseDir(source, manualSourceCfg);

                    FileSetUtil.FileSetInclude(excludedSources, cc_sources);
                }

                //This is the simple case where the user wants to bulkbuild the runtime.sourcefiles.
                if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.sourcefiles")))
                {
                    isReallyBulkBuild = true;

                    if (custombuildBulkBuildSources != null && custombuildBulkBuildSources.Count > 0)
                    {
                        FileSet bbSourceFiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles"));

                        foreach (var cbFi in custombuildBulkBuildSources)
                        {
                            bbSourceFiles.Include(cbFi);
                        }
                    }

                    // See if any files have optionset with option "bulkbuild"=off/false. Exclude these files from bulkbuildsources and set partial bulkbuild to true.

                    FileSet bulkbuildfiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles"));

                    foreach (var pattern in bulkbuildfiles.Includes)
                    {
                        if (!String.IsNullOrEmpty(pattern.OptionSet))
                        {
                            OptionSet fileoptions = null;
                            if (Project.NamedOptionSets.TryGetValue(pattern.OptionSet, out fileoptions))
                            {
                                string bulkbuildoption = null;
                                if (fileoptions.Options.TryGetValue("bulkbuild", out bulkbuildoption))
                                {
                                    if (bulkbuildoption != null && (bulkbuildoption.Equals("off", StringComparison.OrdinalIgnoreCase) || bulkbuildoption.Equals("false", StringComparison.OrdinalIgnoreCase)))
                                    {
                                        enablePartialBulkBuild = true;
                                        bulkbuildfiles.Excludes.Add(pattern);
                                    }
                                }
                            }
                        }
                    }

                    // On Mac exclude *.mm files from the bulkbuild
                    // Mono returns Platform Unix instead of MacOSX. Test for Unix as well. 
                    // TODO: find a better way to detect MacOSX
                    // Some versions of Runtime do not have MacOSX definition. Use integer value 6
                    //if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX)
                    if (module.Configuration.System == "iphone" || module.Configuration.System == "iphone-sim" || module.Configuration.System == "osx")
                    {
                        int bbSourceCount = 0;

                        FileSet bbSourceFiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles"));
                        if (!enablePartialBulkBuild)
                        {
                            bbSourceCount = bbSourceFiles.FileItems.Count;
                        }
                        FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.mm");
                        FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.m");
                        if (!enablePartialBulkBuild && (bbSourceCount != bbSourceFiles.FileItems.Count))
                        {
                            enablePartialBulkBuild = true;
                        }
                    }

                    //Loose = common + platform-specific - bulkbuild
                    FileSet looseSourcefiles = null;
                    if (enablePartialBulkBuild)
                    {
                        looseSourcefiles = new FileSet();
                        if (custombuildLooseSources != null)
                        {
                            foreach (var cbFi in custombuildLooseSources)
                            {
                                looseSourcefiles.Include(cbFi);
                            }
                        }
                        looseSourcefiles.IncludeWithBaseDir(cc_sources);
                        looseSourcefiles.Exclude(FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles")));
                    }

                    string bbfiletemplate =   PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("bulkbuild.filename.template"),
                                              PropertyUtil.GetPropertyOrDefault(Project, "bulkbuild.filename.template", "BB_%groupname%.cpp"));


                    string outputFile = bbfiletemplate.Replace("%groupname%", module.GroupName).Replace("%group%", module.BuildGroup.ToString()).Replace("%modulename%", module.Name);

                    // Check it template defines any directories:
                    var subdir = Path.GetDirectoryName(outputFile);
                    if (!String.IsNullOrEmpty(subdir))
                    {
                        bulkbuildOutputdir = Path.Combine(bulkbuildOutputdir, subdir);
                        outputFile = Path.GetFileName(outputFile);
                    }

                    string maxBulkbuildSize = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("max-bulkbuild-size"),
                                              PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.max-bulkbuild-size", String.Empty));

                    // Generate bulkbuild CPP file
                    FileSet bbloosefiles;

                    var header = compiler.PrecompiledHeaders == CcCompiler.PrecompiledHeadersAction.UsePch ? module.PrecompiledHeaderFile : null;

                    FileSet bbFiles = EA.GenerateBulkBuildFiles.generatebulkbuildfiles.Execute(Project, module.GroupName + ".bulkbuild.sourcefiles", outputFile, bulkbuildOutputdir, maxBulkbuildSize, header, out bbloosefiles);

                    if (bbloosefiles != null && bbloosefiles.FileItems.Count > 0)
                    {
                        if (enablePartialBulkBuild)
                        {
                            if (looseSourcefiles.FileItems.Count == 0)
                            {
                                looseSourcefiles.BaseDirectory = bbloosefiles.BaseDirectory;
                            }
                            looseSourcefiles.Include(bbloosefiles, force: true);
                        }
                        else
                        {
                            source.Include(bbloosefiles, force: true);
                        }
                    }

                    //Add bulkbuild CPP to source fileset
                    FileSetUtil.FileSetAppend(source, bbFiles);
                    if (String.IsNullOrEmpty(source.BaseDirectory))
                    {
                        source.BaseDirectory = bbFiles.BaseDirectory;
                    }


                    //Add loose files to source fileset 
                    if (enablePartialBulkBuild)
                    {
                        if (looseSourcefiles.FileItems.Count > 0)
                        {
                            FileSetUtil.FileSetAppendWithBaseDir(source, looseSourcefiles);
                        }

                        if (custombuildBulkBuildSources != null)
                        {
                            foreach (var cbFi in custombuildBulkBuildSources)
                            {
                                excludedSources.Include(cbFi);
                            }
                        }
                        FileSetUtil.FileSetInclude(excludedSources, cc_sources);
                        FileSetUtil.FileSetExclude(excludedSources, looseSourcefiles);
                    }
                    else
                    {
                        //IMTODO: to be more accurate use sourcefiles or not?
                        excludedSources.IncludeWithBaseDir(FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles")));
                    }
                }

                //This is the case where the user supplies bulkbuild filesets. This means that the runtime.bulkbuild.fileset Property exists.
                if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
                {
                    isReallyBulkBuild = true;

                    if (custombuildBulkBuildSources != null && custombuildBulkBuildSources.Count > 0)
                    {
                        var bbfilesets = PropGroupValue("bulkbuild.filesets");

                        FileSet bbCusomBuildSourceFiles = new FileSet();

                        foreach (var cbFi in custombuildBulkBuildSources)
                        {
                            bbCusomBuildSourceFiles.Include(cbFi);
                        }
                        string customBbName = PropGroupName("_auto_custombuild_bb");
                        Project.NamedFileSets[customBbName] = bbCusomBuildSourceFiles;
                        Project.Properties[PropGroupName("bulkbuild.filesets")] = bbfilesets + Environment.NewLine + customBbName;
                    }


                    // On Mac exclude *.mm files from the bulkbuild
                    // Mono returns Platform Unix instead of MacOSX. Test for Unix as well. 
                    // TODO: find a better way to detect MacOSX
                    // Some versions of Runtime do not have MacOSX definition. Use integer value 6
                    if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX)
                    {
                        if (module.Configuration.System == "iphone" || module.Configuration.System == "iphone-sim" || module.Configuration.System == "osx")
                        {
                            int bbSourceCount = 0;

                            foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
                            {
                                FileSet bbSourceFiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles"));
                                if (bbSourceFiles != null)
                                {
                                    if (!enablePartialBulkBuild)
                                    {
                                        bbSourceCount = bbSourceFiles.FileItems.Count;
                                    }
                                    FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.mm");
                                    FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.m");

                                    if (!enablePartialBulkBuild && (bbSourceCount != bbSourceFiles.FileItems.Count))
                                    {
                                        enablePartialBulkBuild = true;
                                    }
                                }
                                else
                                {
                                    Error.Throw(Project, "SetupBulkBuild", "Fileset '{0}' specified in property '{1}'='{2}' does not exist ", PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles"), PropGroupName("bulkbuild.filesets"), Project.Properties[PropGroupName("bulkbuild.filesets")]);
                                }
                            }
                        }
                    }
                    //Loose = common + platform-specific - bulkbuild(multiple ones)
                    FileSet looseSourcefiles = null;
                    if (enablePartialBulkBuild)
                    {
                        looseSourcefiles = new FileSet();
                        if (custombuildLooseSources != null)
                        {
                            foreach (var cbFi in custombuildLooseSources)
                            {
                                looseSourcefiles.Include(cbFi);
                            }
                        }

                        FileSetUtil.FileSetInclude(looseSourcefiles, cc_sources);

                        foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
                        {
                            FileSetUtil.FileSetExclude(looseSourcefiles, FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles")));
                        }
                    }

                    // Generate bulkbuild CPP file
                    foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName(".bulkbuild.filesets")]))
                    {
                        string outputFile = "BB_" + module.GroupName + "_" + bbFileSetName + ".cpp";

                        string maxBulkbuildSize = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(bbFileSetName + ".max-bulkbuild-size"),
                                                             PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("max-bulkbuild-size"),
                                                             PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.max-bulkbuild-size", String.Empty)));

                        FileSet bbloosefiles;

                        var header = compiler.PrecompiledHeaders == CcCompiler.PrecompiledHeadersAction.UsePch ? module.PrecompiledHeaderFile : null;

                        FileSet bbFiles = EA.GenerateBulkBuildFiles.generatebulkbuildfiles.Execute(Project, module.GroupName + ".bulkbuild." + bbFileSetName + ".sourcefiles", outputFile, bulkbuildOutputdir, maxBulkbuildSize, header, out bbloosefiles);

                        //Add bulkbuild CPP to source fileset
                        FileSetUtil.FileSetAppend(source, bbFiles);
                        if (String.IsNullOrEmpty(source.BaseDirectory))
                        {
                            source.BaseDirectory = bbFiles.BaseDirectory;
                        }


                        if (bbloosefiles != null && bbloosefiles.FileItems.Count > 0)
                        {
                            if (enablePartialBulkBuild)
                            {
                                looseSourcefiles.Include(bbloosefiles, force: true);
                            }
                            else
                            {
                                source.Include(bbloosefiles, force: true);
                            }
                        }

                        FileSetUtil.FileSetInclude(excludedSources, FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles")));
                    }

                    //Add loose files to source fileset 
                    if (enablePartialBulkBuild)
                    {
                        FileSetUtil.FileSetAppendWithBaseDir(source, looseSourcefiles);

                        if (custombuildBulkBuildSources != null)
                        {
                            foreach (var cbFi in custombuildBulkBuildSources)
                            {
                                excludedSources.Include(cbFi);
                            }
                        }

                        FileSetUtil.FileSetInclude(excludedSources, cc_sources);
                        FileSetUtil.FileSetExclude(excludedSources, looseSourcefiles);
                    }
                    else
                    {
                        excludedSources.Append(custombuildsources);
                        FileSetUtil.FileSetInclude(excludedSources, cc_sources);
                    }
                }
            }

            if (!isReallyBulkBuild)
            {
                //This is the non-bulkbuild case. Set up build sourcefiles.
                source.AppendWithBaseDir(custombuildsources);
                source.AppendWithBaseDir(cc_sources);
            }

            return source;
        }

        private void ReplaceTemplates(Tool tool, Module_Native module, string outputlibdir, string outputname)
        {
            if (tool != null)
            {
                for (int i = 0; i < tool.Options.Count; i++)
                {
                    tool.Options[i] = tool.Options[i]
                        .Replace("%outputlibdir%", outputlibdir ?? module.OutputDir.Path)
                        .Replace("%outputbindir%", module.OutputDir.Path)
                        .Replace("%intermediatedir%", module.IntermediateDir.Path)
                        .Replace("%outputdir%", module.OutputDir.Path)
                        .Replace("%outputname%", outputname)
                        .Replace("\"%pchheaderfile%\"", module.PrecompiledHeaderFile.Quote())
                        .Replace("%pchheaderfile%", module.PrecompiledHeaderFile.Quote())
                        .Replace("%pchfile%", module.PrecompiledFile.Path);
                }
            }
        }

        private void AddSystemIncludes(CcCompiler tool)
        {
            if (tool != null)
            {
                bool prepend = Project.Properties.GetBooleanPropertyOrDefault(tool.ToolName + ".usepreincludedirs", false);

                var systemincludes = GetOptionString(tool.ToolName + ".includedirs", BuildOptionSet.Options).LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
                if (prepend)
                    tool.IncludeDirs.InsertRange(0, systemincludes);
                else
                    tool.IncludeDirs.AddUnique(systemincludes);

                foreach (var filetool in tool.SourceFiles.FileItems.Select(fi => fi.GetTool() as CcCompiler).Where(t => t != null))
                {
                    if (prepend)
                        filetool.IncludeDirs.InsertRange(0, systemincludes);
                    else
                        filetool.IncludeDirs.AddUnique(systemincludes);
                }
            }
        }

        private void AddSystemIncludes(AsmCompiler tool)
        {
            if (tool != null)
            {
                bool prepend = Project.Properties.GetBooleanPropertyOrDefault("as.usepreincludedirs", false);

                var systemincludes = GetOptionString("as.includedirs", BuildOptionSet.Options).LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
                if (prepend)
                    tool.IncludeDirs.InsertRange(0, systemincludes);
                else
                    tool.IncludeDirs.AddUnique(systemincludes);

                foreach (var filetool in tool.SourceFiles.FileItems.Select(fi => fi.GetTool() as AsmCompiler).Where(t => t != null))
                {
                    if (prepend)
                        filetool.IncludeDirs.InsertRange(0, systemincludes);
                    else
                        filetool.IncludeDirs.AddUnique(systemincludes);
                }
            }
        }

        private void SetupComAssembliesAssemblies(DotNetCompiler compiler, Module_DotNet module)
        {
            if (FileSetUtil.FileSetExists(Project, PropGroupName("comassemblies")))
            {
                    IDictionary<string, string> taskParameters = new Dictionary<string, string>()
                    {
                        {"module", module.Name},
                        {"group", module.BuildGroup.ToString()},
                        {"generated-assemblies-fileset", "__private_com_net_wrapper_references"}
                    };

                    TaskUtil.ExecuteScriptTask(Project, "task-generatemoduleinteropassemblies", taskParameters);

                    compiler.Assemblies.AppendIfNotNull(FileSetUtil.GetFileSet(Project, "__private_com_net_wrapper_references"));
                    NAnt.Core.Functions.FileSetFunctions.FileSetUndefine(Project, "__private_com_net_wrapper_references");
            }
        }



        private void SetupComAssembliesAssemblies(CcCompiler compiler, Module_Native module)
        {
            if (FileSetUtil.FileSetExists(Project, PropGroupName("comassemblies")))
            {
                if (module.IsKindOf(Module.Managed))
                {
                    IDictionary<string, string> taskParameters = new Dictionary<string, string>()
                    {
                        {"module", module.Name},
                        {"group", module.BuildGroup.ToString()},
                        {"generated-assemblies-fileset", "__private_com_net_wrapper_references"}
                    };

                    TaskUtil.ExecuteScriptTask(Project, "task-generatemoduleinteropassemblies", taskParameters);

                    compiler.Assemblies.AppendIfNotNull(FileSetUtil.GetFileSet(Project, "__private_com_net_wrapper_references"));
                    NAnt.Core.Functions.FileSetFunctions.FileSetUndefine(Project, "__private_com_net_wrapper_references");

                }
                else
                {
                    compiler.ComAssemblies.AppendWithBaseDir(FileSetUtil.GetFileSet(Project, PropGroupName("comassemblies")));
                }
            }
        }

        private void SetupWebReferences(DotNetCompiler compiler, Module_DotNet module)
        {
            if (OptionSetUtil.OptionSetExists(Project, PropGroupName("webreferences")))
            {
                IDictionary<string, string> taskParameters = new Dictionary<string, string>()
                {
                    {"module", module.Name},
                    {"group", module.BuildGroup.ToString()},
                    {"output", "__private_generated_sources_fileset"}
                };

                TaskUtil.ExecuteScriptTask(Project, "task-generatemodulewebreferences", taskParameters);

                compiler.SourceFiles.AppendIfNotNull(FileSetUtil.GetFileSet(Project, "__private_generated_sources_fileset"));
                NAnt.Core.Functions.FileSetFunctions.FileSetUndefine(Project, "__private_generated_sources_fileset");

                // The system.web.dll and System.web.services.dll must be included so the code can compile
                //  Dot Net frameworks version 3.0 and higher do not contain macorlib.dll, System.dll in reference directory
                compiler.Assemblies.Include("System.Web.dll", true);
                compiler.Assemblies.Include("System.Web.Services.dll", true);
            }
        }

        private void SetupWebReferences(CcCompiler compiler, Module_Native module)
        {
            if (OptionSetUtil.OptionSetExists(Project, PropGroupName("webreferences")))
            {
                if (module.IsKindOf(Module.Managed))
                {
                    IDictionary<string, string> taskParameters = new Dictionary<string, string>()
                    {
                        {"module", module.Name},
                        {"group", module.BuildGroup.ToString()},
                        {"output", "__private_generated_header_dir"}

                    };

                    TaskUtil.ExecuteScriptTask(Project, "task-generatemodulewebreferences", taskParameters);

                    compiler.IncludeDirs.Add(new NAnt.Core.Util.PathString(Project.Properties["__private_generated_header_dir"],
                        NAnt.Core.Util.PathString.PathState.Directory));
                    NAnt.Core.Functions.PropertyFunctions.PropertyUndefine(Project, "__private_generated_header_dir");
                }
            }
        }

        PathString GetObjectFile(FileItem fileitem, Tool tool, Module module, CommonRoots<FileItem> commonroots, ISet<string> duplicates, out uint nameConflictFlag)
        {
            nameConflictFlag = 0;
            OptionDictionary options = BuildOptionSet.Options;
            OptionSet fileoptions = null;
            if (!String.IsNullOrEmpty(fileitem.OptionSetName))
            {
                if (Project.NamedOptionSets.TryGetValue(fileitem.OptionSetName, out fileoptions))
                {
                    options = fileoptions.Options;
                }
            }
            // Get commonRoot:
            string prefix = commonroots.GetPrefix(fileitem.Path.Path);
            string root = commonroots.Roots[prefix];

            string intermediatedir = module.IntermediateDir.Path + Path.DirectorySeparatorChar;

#if FRAMEWORK_PARALLEL_TRANSITION
            if (ProcessGenerationData)
            {
                intermediatedir += "vcproj" + Path.DirectorySeparatorChar;
            }

#endif
            // To prevent very long obj file paths make sure that part after root is not too long
            string rel = fileitem.Path.Path.Substring(root.Length);
            if (rel.Count(c=> c=='\\' || c == '/') > 4 || Path.GetDirectoryName(rel).Length > 100)
            {
                rel = Path.Combine("src",Path.GetFileName(rel));
            }

            //string basepath = fileitem.Path.Path.Replace(root, intermediatedir);
            string basepath = Path.Combine(intermediatedir, rel);


            if (!duplicates.Add(basepath))
            {
                basepath = EA.CPlusPlusTasks.CompilerBase.GetOutputBasePath(fileitem, intermediatedir, String.Empty);
                nameConflictFlag = FileData.ConflictObjName;
            }

            //IMDODO !!!!
            // Disabled normalization because some paths can get too long (>260 characters)...  I'm looking at you NCAA xenon-vs-dev-opt-gamereflect config...
            //             return PathString.MakeNormalized(basepath + (GetOptionString(tool.ToolName + ".objfile.extension", options) ?? ".obj"));
            return PathString.MakeNormalized(basepath + (GetOptionString(tool.ToolName + ".objfile.extension", options) ?? ".obj"), PathString.PathParam.NormalizeOnly);
        }
        #endregion

        #region Helper functions

        protected void SetToolTemplates(Tool tool, OptionSet options = null)
        {
            if (tool != null)
            {

                string filter = (tool.ToolName == "asm" ? "as" : tool.ToolName) + ".template.";

                foreach (var prop in Project.Properties.Where(p => p.Name.StartsWith(filter)))
                {
                    if (!Project.Properties.IsPropertyLocal(prop.Name))
                    {
                        tool.Templates[prop.Name] = prop.Value;
                    }
                }

                var set = options ?? BuildOptionSet;

                foreach (var entry in set.Options.Where(e => e.Key.StartsWith(filter)))
                {
                    tool.Templates[entry.Key] = entry.Value;
                }
            }
        }

        private string GetOutputName(IModule module)
        {
            return PropGroupValue("outputname", module.Name);
        }

        private void GetOutputNameAndExtension(string path, string name, out string outputname, out string extension)
        {
            //Here account for the case when module name has dots in it.
            var file = Path.GetFileName(path);
            if (file.StartsWith(name, StringComparison.OrdinalIgnoreCase))
            {
                int dotind = file.IndexOf('.',name.Length);
                if (dotind < 0)
                {
                    outputname = file;
                    extension = String.Empty;
                }
                else
                {
                    outputname = file.Substring(0, dotind);
                    extension = file.Substring(dotind);
                }
            }
            else
            {
                outputname = Path.GetFileNameWithoutExtension(file);
                extension = Path.GetExtension(file);
            }
        }

        protected PathString GetIntermediateDir(IModule module)
        {
            PathString intermediatedir = PathString.MakeNormalized(Path.Combine(
                                                                Project.Properties["package.configbuilddir"],
                                                                Project.Properties["eaconfig." + module.BuildGroup + ".outputfolder"].TrimLeftSlash(),
                                                                module.Name));

            return intermediatedir;
        }

        protected PathString GetBinOutputDir(IModule module)
        {
            PathString outputdir;

            // If user defined custom output directory use it:
            string dir = Project.Properties[PropGroupName(".outputdir")];

            if (!String.IsNullOrEmpty(dir))
            {
                outputdir = PathString.MakeNormalized(dir);
            }
            else
            {
                outputdir = PathString.MakeNormalized(Path.Combine(
                                                      Project.Properties["package.configbindir"],
                                                      Project.Properties["eaconfig." + module.BuildGroup + ".outputfolder"].TrimLeftSlash()));
            }

            return outputdir;
        }

        protected PathString GetLibOutputDir(IModule module)
        {
            PathString outputdir;

            // If user defined custom output directory use it:
            string dir = Project.Properties[PropGroupName(".outputdir")];

            if (!String.IsNullOrEmpty(dir))
            {
                outputdir = PathString.MakeNormalized(dir);
            }
            else
            {
                outputdir = PathString.MakeNormalized(Path.Combine(
                                                      Project.Properties["package.configlibdir"],
                                                      Project.Properties["eaconfig." + module.BuildGroup + ".outputfolder"].TrimLeftSlash()));
            }

            return outputdir;
        }

        protected void SetLinkOutput(IModule module, out string outputname, out string outputextension, out PathString linkoutputdir, out string replacename)
        {
            outputname = GetOutputName(module);
            replacename = outputname;

            string mappedlinkname = null;

            if (ModuleOutputNameMapping != null)
            {
                if (ModuleOutputNameMapping.Options.TryGetValue("linkoutputname", out mappedlinkname))
                {
                    replacename = mappedlinkname.Replace("%outputname%", outputname);
                }
                else
                {
                    mappedlinkname = null;
                }
            }

            // Mapping templates can contain outputname
            if (module.OutputDir.Path.Contains("%outputname%"))
            {
                 module.OutputDir = PathString.MakeNormalized(module.OutputDir.Path.Replace("%outputname%", outputname));
            }

            string linkoutputname;
            if (BuildOptionSet.Options.TryGetValue("linkoutputname", out linkoutputname))
            {
                bool isIntermediate = linkoutputname.Contains("%intermediatedir%");

                if (mappedlinkname != null)
                {
                    linkoutputname = linkoutputname.Replace("%outputname%", mappedlinkname);
                }

                linkoutputname = linkoutputname
                    .Replace("%intermediatedir%", module.IntermediateDir.Path)
                    .Replace("%outputdir%", module.OutputDir.Path)
                    .Replace("%outputname%", outputname);

                linkoutputname = PathNormalizer.Normalize(linkoutputname, true);
                // group.module.outputdir property overwrites linkoutputname.
                linkoutputdir = PathString.MakeNormalized(Path.GetDirectoryName(linkoutputname), PathString.PathParam.NormalizeOnly);

                if (!isIntermediate && !Project.Properties.Contains(PropGroupName("outputdir")))
                {
                    module.OutputDir = linkoutputdir;
                }

                GetOutputNameAndExtension(linkoutputname, mappedlinkname ?? outputname, out outputname, out outputextension);
            }
            else
            {
                string suffix = "exe-suffix";

                if (module.IsKindOf(Module.Program))
                {
                    suffix = "exe-suffix";
                }
                else if (module.IsKindOf(Module.DynamicLibrary))
                {
                       suffix = "dll-suffix";
                }
                GetOutputNameAndExtension(replacename + Project.Properties[suffix], replacename, out outputname, out outputextension);
                linkoutputdir = module.OutputDir;
            }
        }

        #endregion

        #region Dispose interface implementation

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing)
                {
                    // Dispose managed resources here
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;

        #endregion

        internal class CustomBuildToolFlags : BitMask
        {
            internal CustomBuildToolFlags(uint type = 0) : base(type) { }

            internal const uint AllowBulkbuild = 1;
        }

        private static readonly HashSet<string> HEADER_EXTENSIONS = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { ".h", ".hpp", ".hxx" };

        


    }
}
