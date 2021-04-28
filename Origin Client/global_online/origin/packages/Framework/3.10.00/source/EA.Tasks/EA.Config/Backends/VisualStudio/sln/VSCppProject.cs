using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Globalization;
using System.Security.Principal;
using System.Runtime.InteropServices;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.PackageCore;
using NAnt.Core.Writers;
using NAnt.Core.Events;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal abstract class VSCppProject : VSProjectBase
    {
        protected VSCppProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            :
            base(solution, modules, VCPROJ_GUID)
        {
            InitFunctionTables();

            // Get project configuration that equals solution startup config:
            Configuration startupconfig;
            if (ConfigurationMap.TryGetValue(solution.StartupConfiguration, out startupconfig))
            {
                StartupModule = Modules.Single(m => m.Configuration == startupconfig) as Module;
            }
            else
            {
                StartupModule = Modules.First() as Module;
            }
        }

        protected virtual void InitFunctionTables()
        {
            //--- Win
            _toolMethods.Add("VCXMLDataGeneratorTool", WriteXMLDataGeneratorTool);
            _toolMethods.Add("VCWebServiceProxyGeneratorTool", WriteWebServiceProxyGeneratorTool);
            _toolMethods.Add("VCMIDLTool", WriteMIDLTool);
            _toolMethods.Add("MASM", WriteAsmCompilerTool);
            _toolMethods.Add("VCCLCompilerTool", WriteCompilerTool);
            _toolMethods.Add("VCLibrarianTool", WriteLibrarianTool);
            _toolMethods.Add("VCManagedResourceCompilerTool", WriteManagedResourceCompilerTool);
            _toolMethods.Add("VCResourceCompilerTool", WriteResourceCompilerTool);
            _toolMethods.Add("VCLinkerTool", WriteLinkerTool);
            _toolMethods.Add("VCALinkTool", WriteALinkTool);
            _toolMethods.Add("VCManifestTool", WriteManifestTool);
            _toolMethods.Add("VCXDCMakeTool", WriteXDCMakeTool);
            _toolMethods.Add("VCBscMakeTool", WriteBscMakeTool);
            _toolMethods.Add("VCFxCopTool", WriteFxCopTool);
            _toolMethods.Add("VCAppVerifierTool", WriteAppVerifierTool);
            _toolMethods.Add("VCWebDeploymentTool", WriteWebDeploymentTool);
            _toolMethods.Add("VCCustomBuildTool", WriteVCCustomBuildTool);            
            //--- Xbox 360
            _toolMethods.Add("VCCLX360CompilerTool", WriteCompilerTool);
            _toolMethods.Add("VCX360LinkerTool", WriteLinkerTool);
            _toolMethods.Add("VCX360ImageTool", WriteX360ImageTool);
            _toolMethods.Add("VCX360DeploymentTool", WriteX360DeploymentTool);
        }

        protected readonly Module StartupModule;

        #region Implementation of Inherited Abstract Methods

        protected override string UserFileName
        {
            get 
            {
                string userfilename = ProjectFileName + ".user";

                WindowsPrincipal wp = new WindowsPrincipal(WindowsIdentity.GetCurrent());

                if (wp != null && wp.Identity != null)
                {
                    var currentUser = wp.Identity.Name
                        .Replace(@"\", ".")
                        .Replace(@"/", ".");
                    userfilename = ProjectFileName + "." + currentUser + ".user";
                }

                return userfilename; 
            }
        }


        protected override void WriteProject(IXmlWriter writer)
            {
                WriteVisualStudioProject(writer);
                WriteSourceControlIntegration(writer);
                WritePlatforms(writer);

                WriteToolFiles(writer);
                WriteConfigurations(writer);
                WriteReferences(writer);
                WriteFiles(writer);
                WriteGlobals(writer);
                writer.WriteEndElement(); //VisualStudioProject
            }


        #endregion Implementation of Inherited Abstract Methods

        #region Virtual Protected Methods

        #region Utility Methods

        protected virtual string ProjectType
        {
            get { return "Visual C++"; }
        }

        protected virtual string DefaultTargetFrameworkVersion
        {
            get { return "3.5"; }
        }

        protected virtual string TargetFrameworkVersion
        {
            get
            {
#if FRAMEWORK_PARALLEL_TRANSITION
                const string DEFAULT_TARGET_FRAMEWORK_VERSION = "131072";  // Default to framework 2.0
#else
                const string DEFAULT_TARGET_FRAMEWORK_VERSION = "196613";  // Default to framework 3.5
                if (StartupModule.IsKindOf(Module.Managed))
#endif
                {
                    string targetframework = String.Empty;

                    if (StartupModule is Module_Native)
                    {
                        targetframework = (StartupModule as Module_Native).TargetFrameworkVersion;
                    }

                    string dotNetVersion = String.IsNullOrEmpty(targetframework) ? DefaultTargetFrameworkVersion : targetframework;

                    //Set target framework version according to VS2008 format:

                    if (dotNetVersion.StartsWith("2.0"))
                    {
                        return "131072";
                    }
                    else if (dotNetVersion.StartsWith("3.0"))
                    {
                        return "196608";
                    }
                    return DEFAULT_TARGET_FRAMEWORK_VERSION;
                }
                return string.Empty;
            }
        }

        protected virtual string RootNamespace
        {
            get
            {
                return (StartupModule is Module_Native) ? (StartupModule as Module_Native).RootNamespace : String.Empty;
            }
        }

        protected virtual string Keyword
        {
            get
            {
                string keyword = String.Empty;
                foreach (var module in Modules)
                {
                    string moduleKeyword = "Win32Proj";
                    if (module.IsKindOf(Module.MakeStyle))
                    {
                        moduleKeyword = "MakeFileProj";
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
                        keyword = String.Empty;
                        break;
                    }
                }
                return keyword;
            }
        }

        protected virtual string PrimaryOutput(Module module)
        {
            if (module.Tools != null)
            {
                Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

                if (link != null)
                {
                    return Path.Combine(link.LinkOutputDir.Path, link.OutputName + link.OutputExtension);
                }

                Librarian lib = module.Tools.SingleOrDefault(t => t.ToolName == "lib") as Librarian;

                if (lib != null)
                {
                    return Path.Combine(module.OutputDir.Path, lib.OutputName + lib.OutputExtension);
                }

                Module_DotNet dotNetMod = module as Module_DotNet;

                if (dotNetMod != null && dotNetMod.Compiler != null)
                {
                    return Path.Combine(module.OutputDir.Path, dotNetMod.Compiler.OutputName + dotNetMod.Compiler.OutputExtension);
                }
            }

            return Path.Combine(module.OutputDir.Path, module.Name);
        }

        protected virtual void PrimaryOutput(Module module, out string fileName, out string extension)
        {
            fileName = null;
            extension = null;
            if (module.Tools != null)
            {
                Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

                if (link != null)
                {
                    fileName = link.OutputName;
                    extension = link.OutputExtension;
                }

                Librarian lib = module.Tools.SingleOrDefault(t => t.ToolName == "lib") as Librarian;

                if (lib != null)
                {
                    fileName = lib.OutputName;
                    extension = lib.OutputExtension;
                }
            }
            if (fileName == null)
            {
                fileName = module.Name;
            }
        }


        protected virtual string GetVCConfigurationType(Module module)
        {
            var map = new Dictionary<uint, string>() {

                {Module.Program, "1"},
                {Module.DynamicLibrary, "2"},
                {Module.Library, "4"},
                {Module.Utility, "10"},
                {Module.MakeStyle, "0"}
            };

            // VS2008 Utility type does not have custom build steps. Use type 1 to enable them
            if (module.IsKindOf(Module.Utility) && module.Configuration.System != "xenon")
            {
                    return "1";
            }

            foreach(var e in map)
            {
                if(module.IsKindOf(e.Key))
                    return e.Value;
            }

            Log.Warning.WriteLine("Can't map module [{0} '{1}.{2}' ({3})] to Visual Studio configuration type. Assume Utility project", module.Package.Name, module.BuildGroup, module.Name, module.Configuration.Name);

            return "10"; // Utility by default
        }

        protected virtual VSConfig.CharacterSetTypes CharacterSet(Module module)
        {
            VSConfig.CharacterSetTypes type = VSConfig.CharacterSetTypes.MultiByte;

            if (module.Tools != null)
            {
                CcCompiler compiler = module.Tools.SingleOrDefault(t => t.ToolName == "cc") as CcCompiler;

                if (compiler != null)
                {
                    type = VSConfig.GetCharacterSetFromDefines(compiler.Defines);
                }
            }
            //IMTODO: Makestyle projects allow for set of defines

            return type;

        }

        protected virtual IEnumerable<string> GetAllDefines(CcCompiler compiler, Configuration configuration)
        {
            var defines = new SortedSet<string>(compiler.Defines);

            //IMTODO. MOVE this to eaconfig.
            // SN_TARGET_ is required by VSI.NET to recognize the project as PSx and invoke its tools
            // instead of Microsoft ones. The defines were found by looking at a PSx project generated
            // with the VSI.NET wizard
            if (configuration.System == "ps3")
            {
                if (configuration.SubSystem == ".spu")
                {
                    defines.Add("SN_TARGET_PS3_SPU");
                }
                else
                {
                    defines.Add("SN_TARGET_PS3");
                }

                if (configuration.Compiler == "sn")
                {
                    if (configuration.SubSystem == ".spu")
                    {
                        defines.Add("__GCC__");
                    }
                    else
                    {
                        defines.Add("__SNC__");
                    }
                }
                else  // default to SPU or GCC compiler
                {
                    defines.Add("__GCC__");
                }
            }
            else if (configuration.System == "ps2")
            {
                defines.Add("SN_TARGET_PS2");
            }

            return defines;
        }

        protected virtual string GetAllIncludeDirectories(CcCompiler compiler)
        {
            //Because of a bug in qcc targets quotes are not supportd
            if (compiler.Executable != null && compiler.Executable.Path != null && compiler.Executable.Path.Contains("qcc.exe"))
            {
                return compiler.IncludeDirs.ToString(";", p => GetProjectPath(p).TrimQuotes());
            }
            return compiler.IncludeDirs.ToString(";", p => GetProjectPath(p).Quote());
        }

        protected virtual string GetAllUsingDirectories(CcCompiler compiler)
        {
            //IM Because of a bug in bb10 MSBuild targets quotes are not supportd
            if (compiler.Executable != null && compiler.Executable.Path != null && compiler.Executable.Path.Contains("qcc.exe"))
            {
                return compiler.UsingDirs.ToString(";", p => GetProjectPath(p).TrimQuotes());
            }
            return compiler.UsingDirs.ToString(";", p => GetProjectPath(p).Quote());
        }

        protected virtual IEnumerable<PathString> GetAllLibraries(Linker link, Module module)
        {
            var dependents = module.Package.Project.Properties.GetBooleanProperty(Project.NANT_PROPERTY_TRANSITIVE) 
                ? module.Dependents.Flatten(DependencyTypes.Build).Where(d => d.IsKindOf(DependencyTypes.Link))
                : module.Dependents.Where(d => d.IsKindOf(DependencyTypes.Build) && d.IsKindOf(DependencyTypes.Link));

            var deplibraries = dependents.Select(d => d.Dependent.LibraryOutput()).Where(s => s != null);

            return link.Libraries.FileItems.Select(item => item.Path).Except(deplibraries);
        }

        protected virtual IEnumerable<PathString> GetAllLibraryDirectories(Linker link)
        {
            return link.LibraryDirs;
        }


        protected virtual void InitCompilerToolProperties(Module module, Configuration configuration, string includedirs, string usingdirs, string defines, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
        {
            nameToXMLValue = new SortedDictionary<string, string>();

            //**********************************
            //* Set defaults that should exist before the switch parsing
            //* These are either something that a switch can add to (if switch setOnce==false)
            //* or something that ever can't be changed (if switch setOnce==true)
            //**********************************

            //Turn the include dirs from '\n' separated to ; separated
            if (!String.IsNullOrEmpty(includedirs))
                nameToXMLValue.Add("AdditionalIncludeDirectories", VSConfig.CleanString(includedirs, true, true, true));

            //Turn the include dirs from '\n' separated to ; separated
            if (!String.IsNullOrEmpty(usingdirs))
                nameToXMLValue.Add("AdditionalUsingDirectories", VSConfig.CleanString(usingdirs, true, true, true));

            //Turn the defines from '\n' separated to ; separated
            if (!String.IsNullOrEmpty(defines))
            {
                nameToXMLValue.Add("PreprocessorDefinitions", VSConfig.CleanString(defines, false, true, false));
            }

            //*****************************
            //* Set the defaults that are used if a particular attribute has no value after the parsing
            //*****************************
                nameToDefaultXMLValue.Add("DebugInformationFormat", "0");
                nameToDefaultXMLValue.Add("ExceptionHandling", "0");
                nameToDefaultXMLValue.Add("CompileAs", "0");
                nameToDefaultXMLValue.Add("WarningLevel", "0");
                nameToDefaultXMLValue.Add("UsePrecompiledHeader", "0");

            nameToDefaultXMLValue.Add("MinimalRebuild", "FALSE");
            nameToDefaultXMLValue.Add("BufferSecurityCheck", "FALSE");

            // When building PS3 configurations with Visual Studio .NET, it appears that VSI requires
            // a PDB file setting, or a dialog box appears when you open the project:
            //
            // "A change in how VS2005 handles project building requires the project "a" to be modified
            // for build dependencies to work correctly.  Do you wish VSI to automatically make these
            // changes to your project"

            if (configuration.System == "ps3")
            {
                nameToDefaultXMLValue.Add("ProgramDataBaseFileName", "$(IntDir)\\");
            }
        }
         
        protected virtual void InitLinkerToolProperties(Configuration configuration, Linker linker, IEnumerable<PathString> libraries, IEnumerable<PathString> libraryDirs, IEnumerable<PathString> objects, string defaultOutput, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
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
#if FRAMEWORK_PARALLEL_TRANSITION
            //IMTODO: REMOVE SORTING, TEMP FOR COMPARISON
            nameToXMLValue.AddNonEmpty("AdditionalDependencies", libraries.OrderBy(p => p.Path.Quote(), StringComparer.Ordinal).ToString(" ", PathString.Quote) + " " + objects.OrderBy(p => p.Path.Quote(), StringComparer.Ordinal).ToString(" ", PathString.Quote));
#else
            nameToXMLValue.AddNonEmpty("AdditionalDependencies", libraries.ToString(" ", p => GetProjectPath(p).Quote()) + " " + objects.ToString(" ", p => GetProjectPath(p).Quote()));
#endif

            nameToXMLValue.AddNonEmpty("AdditionalLibraryDirectories", libraryDirs.ToString("; ", p => GetProjectPath(p).Quote()));

            //*****************************
            //* Set the defaults that are used if a particular attribute has no value after the parsing
            //*****************************

            nameToDefaultXMLValue.Add("GenerateDebugInformation", "FALSE");
            nameToDefaultXMLValue.Add("OutputFile", GetProjectPath(defaultOutput));

            // When building PS3 configurations with Visual Studio .NET, it appears that VSI cannot cope
            // with incremental linking or manifest generation.  If the settings are anything other than
            // the settings below then a dialog box appears when you open the project:
            //
            // "A change in how VS2005 handles project building requires the project "a" to be modified
            // for build dependencies to work correctly.  Do you wish VSI to automatically make these
            // changes to your project"
            //
            // We might need to be stricter that this current solution, and actually make it an error
            // to set anything other than these settings.  Maybe set these defaults, then check that
            // they're still the same at the end of the run?
            //
            // The linker tool also needs a PDB output file specified, or the same dialog appears.

            if (configuration.System == "ps3")
            {
                nameToDefaultXMLValue.Add("LinkIncremental", "0");
                nameToDefaultXMLValue.Add("GenerateManifest", "false");
                // Yes!  This attribute naming is inconsistent with that used by the CompilerTool
                // which uses ProgramDataBaseFileName.  This inconsistency is correct.
                nameToDefaultXMLValue.Add("ProgramDatabaseFile", "$(OutDir)\\$(TargetName).pdb");
            }

        }

        protected virtual void InitLibrarianToolProperties(Configuration configuration, IEnumerable<PathString> objects, string defaultOutput, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
        {
            nameToXMLValue = new SortedDictionary<string, string>();

            //**********************************
            //* Set defaults that should exist before the switch parsing
            //* These are either something that a switch can add to (if switch setOnce==false)
            //* or something that ever can't be changed (if switch setOnce==true)
            //**********************************
#if FRAMEWORK_PARALLEL_TRANSITION
            nameToXMLValue.Add("AdditionalDependencies", objects.ToString(" ", PathString.Quote));
#else
            nameToXMLValue.AddNonEmpty("AdditionalDependencies", objects.ToString(" ", p => GetProjectPath(p).Quote()));
#endif
            nameToDefaultXMLValue.Add("OutputFile", GetProjectPath(defaultOutput));
        }


        protected virtual IEnumerable<KeyValuePair<string, string>> Globals
        {
            get
            {
                var globals = new Dictionary<string, string>();

                foreach (var module in Modules)
                {
                    string toolPrefix = String.Empty;
                    //translate the ConfigPlatform string into a tool prefix vstomake understands
                    switch (module.Configuration.System)
                    {
                        case "pc":
                            {
                                toolPrefix = "vc";
                                break;
                            }
                        case "pc64":
                            {
                                toolPrefix = "vc64";
                                break;
                            }
                        case "xenon":
                            {
                                toolPrefix = "xenon";
                                break;
                            }
                        case "ps3":
                            {
                                toolPrefix = "ps3";
                                break;
                            }
                        case "rev":
                            {
                                toolPrefix = "rev";
                                break;
                            }
                        case "psp2":
                            {
                                toolPrefix = "psp2";
                                break;
                            }
                        default:
                            {
                                toolPrefix = module.Configuration.Compiler;
                                break;
                            }
                    }

                    var platform_config_key = ("platform:" + GetVSProjConfigurationName(module.Configuration)).Replace(':', '_').Replace('|', '_').Replace(' ', '_');

                    if (String.IsNullOrEmpty(toolPrefix))
                    {
                        continue;
                    }

#if FRAMEWORK_PARALLEL_TRANSITION

                    if (!globals.ContainsKey("platform"))
                    {
                        globals.Add("platform", toolPrefix);
                    }


                    if (toolPrefix != "ps3")
                    {

                        if(!globals.ContainsKey(toolPrefix + "-compiler"))
                        {
                            globals.Add(toolPrefix + "-compiler", GetToolPathFromProject(module, "cc", "config-options-library"));
                            globals.Add(toolPrefix + "-compiler_clang", GetToolPathFromProject(module, "cc-clanguage", "config-options-library"));
                            globals.Add(toolPrefix + "-linker", GetToolPathFromProject(module, "link", "config-options-program"));
                            globals.Add(toolPrefix + "-asm", GetToolPathFromProject(module, "as", "config-options-library"));
                            globals.Add(toolPrefix + "-librarian", GetToolPathFromProject(module, "lib", "config-options-library"));
                            globals.Add(toolPrefix + "-postlink.program", GetToolPathFromProject(module, "link.postlink.program", "config-options-program"));
                        }
                    }
                    else
                    {
                        bool isSpu = module.Configuration.SubSystem == ".spu";

                        var compiler = (module.Configuration.Compiler == "sn") ? "snc" : "gcc";
                        toolPrefix = compiler + (isSpu ? "-spu" : "-ppu");

                        if (isSpu)
                        {
                            globals.Add(toolPrefix + "-compiler", GetToolPathFromProject(module, "cc", "ps3spu", useProperty:false));
                            globals.Add(toolPrefix + "-linker", GetToolPathFromProject(module, "link", "ps3spu", useProperty: false));
                            globals.Add(toolPrefix + "-asm", GetToolPathFromProject(module, "as", "ps3spu", useProperty: false));
                            globals.Add(toolPrefix + "-librarian", GetToolPathFromProject(module, "lib", "ps3spu", useProperty: false));
                        }
                        else
                        {
                            globals.Add(toolPrefix + "-compiler", GetToolPathFromProject(module, "cc", "config-options-library"));
                            globals.Add(toolPrefix + "-linker", GetToolPathFromProject(module, "link", "config-options-program"));
                            globals.Add(toolPrefix + "-asm", GetToolPathFromProject(module, "as", "config-options-library"));
                            globals.Add(toolPrefix + "-librarian", GetToolPathFromProject(module, "lib", "config-options-library"));
                        }
                    }
#else
                    if (!globals.ContainsKey(platform_config_key))
                    {
                        globals.Add(platform_config_key, toolPrefix);
                    }

                    if (toolPrefix == "ps3")
                    {
                        var compiler = (module.Configuration.Compiler == "sn") ? "snc" : "gcc";
                        toolPrefix = compiler + (module.Configuration.SubSystem == ".spu" ? "-spu" : "-ppu");
                    }

                    if (globals.ContainsKey(toolPrefix + "-compiler"))
                    {
                        continue;
                    }

                    if (module is Module_Native)
                    {
                        var native_module = module as Module_Native;

                        // I need to have both CC compiler and CLANG compilers here. native_module.Cc maybe either one of them. Use optionsets to retrieve compiler:
                        string compiler = String.Empty;
                        string clangcompiler = String.Empty;
                        if (module.Configuration.System == "ps3" && module.Configuration.SubSystem == ".spu")
                        {
                            compiler = GetToolPathFromProject(module, "cc", "ps3spu", useProperty: false);
                            clangcompiler = GetToolPathFromProject(module, "cc-clanguage", "ps3spu", useProperty: false);
                        }
                        else
                        {
                            compiler = GetToolPathFromProject(module, "cc", "config-options-library");
                            clangcompiler = GetToolPathFromProject(module, "cc-clanguage", "config-options-library");
                        }

                        if (!string.IsNullOrEmpty(compiler))
                        {
                            compiler = PathNormalizer.Normalize(compiler, false);
                        }
                        if (!string.IsNullOrEmpty(compiler))
                        {
                            clangcompiler = PathNormalizer.Normalize(clangcompiler, false);
                        }

                        globals.Add(toolPrefix + "-compiler", compiler);
                        globals.Add(toolPrefix + "-compiler_clang", clangcompiler);

                        if (native_module.Link != null)
                            globals.Add(toolPrefix + "-linker", native_module.Link.Executable.Path);

                        if (native_module.Asm != null)
                            globals.Add(toolPrefix + "-asm", native_module.Asm.Executable.Path);

                        if (native_module.Lib != null)
                            globals.Add(toolPrefix + "-librarian", native_module.Lib.Executable.Path);

                        if (native_module.PostLink != null)
                            globals.Add(toolPrefix + "-postlink.program", native_module.PostLink.Executable.Path);

                        if (native_module.IsKindOf(Module.Managed))
                        {
                            // Managed C++ modules may have managed resources.  This is here
                            // so vstomaketools knows what the executable is for processing them
                            // Options and input & output files will be by looking at the list
                            // of buildable files for this module and filtering them by file
                            // extension, so there's no need to include anything else here.
                            string resgenTool = null;

                            foreach (Tool tool in native_module.Tools)
                            {
                                if (tool.ToolName == "resx")
                                {
                                    resgenTool = tool.Executable.Path;
                                    break;
                                }
                            }

                            if (resgenTool != null)
                            {
                                globals.Add(toolPrefix + "-resgen", resgenTool);
                            }
                        }
                    }

#endif
                }
                
                globals.Add("build.env.PATH", Package.Project.Properties["build.env.PATH"] ?? String.Empty);

                var ghsmodule = Modules.FirstOrDefault(m => m.Configuration.Compiler == "ghs");
                if(ghsmodule!=null)
                {
                    globals.Add("build.env.LICENSE_FILE_DIR", ghsmodule.Package.Project.Properties["build.env.LICENSE_FILE_DIR"] ?? String.Empty);
                }
                return globals;
            }
        }

        private string GetToolPathFromProject(IModule module, string toolName, string toolOptionSet, bool useProperty = true)
        {
            string toolpath = module.Package.Project.Properties[toolName];

            if (!useProperty || String.IsNullOrEmpty(toolpath))
            {
                OptionSet programOptionSet; 
                if(module.Package.Project.NamedOptionSets.TryGetValue(toolOptionSet, out programOptionSet))                
                {
                    toolpath = programOptionSet.Options[toolName];
                }
            }
            return toolpath;
        }

        protected override IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>> GetUserData(ProcessableModule module)
        {
            var userdata = new List<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>>();

            if (module != null && module.AdditionalData.DebugData != null)
            {
                var debugSettings = new List<KeyValuePair<string, string>>();

                userdata.Add(new KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>("DebugSettings", debugSettings));


                string httpUrlString = "";

                if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.Command.Executable.Path))
                {
                    debugSettings.Add(new KeyValuePair<string, string>("Command", (module.Configuration.Platform == "xenon") ? "$(TargetPath)" : module.AdditionalData.DebugData.Command.Executable.Path));
                }

                if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.Command.WorkingDir.Path))
                {
                    if (module.Configuration.Platform == "ps3")
                    {
                        string[] options = { "HomeDir", "FileServDir" };
                        var workingDirs = ParsePS3UserFileOption(module.AdditionalData.DebugData.Command.WorkingDir.Path, options);
                        foreach (string entry in workingDirs)
                        {
                            httpUrlString = httpUrlString + "?" + entry + "\r";
                        }
                        httpUrlString = httpUrlString.Trim();
                    }
                    debugSettings.Add(new KeyValuePair<string, string>("WorkingDirectory", (module.Configuration.Platform == "xenon") ? "" : module.AdditionalData.DebugData.Command.WorkingDir.Path));
                }

                string commandargs = module.AdditionalData.DebugData.Command.Options.ToString(" ");
                if (!String.IsNullOrEmpty(commandargs))
                {
                    if (module.Configuration.Platform == "ps3")
                    {
                        string[] options = { "DbgCmdLine", "DbgElfArgs", "RunCmdLine", "RunElfArgs" };
                        var cmdArgs = ParsePS3UserFileOption(commandargs, options);
                        foreach (string entry in cmdArgs)
                        {
                            httpUrlString = "?" + entry + "\r" + httpUrlString;
                        }
                        httpUrlString = httpUrlString.Trim();
                    }

                    debugSettings.Add(new KeyValuePair<string, string>("CommandArguments", (module.Configuration.Platform == "xenon") ? "" : commandargs));
                }

                if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.RemoteMachine))
                {
                    debugSettings.Add(new KeyValuePair<string, string>("RemoteMachine", (module.Configuration.Platform == "xenon") ? "$(DefaultConsole)" : module.AdditionalData.DebugData.RemoteMachine));
                }

                if (!String.IsNullOrEmpty(httpUrlString))
                {
                    debugSettings.Add(new KeyValuePair<string, string>("HttpUrl", (module.Configuration.Platform == "xenon") ? "$(DefaultConsole)" : httpUrlString));
                }

                if (module.Configuration.Platform == "xenon")
                {
                    var debugerTool = new List<KeyValuePair<string, string>>();

                    userdata.Add(new KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>("DebuggerTool", debugerTool));

                    debugSettings.Add(new KeyValuePair<string, string>("Command", module.AdditionalData.DebugData.Command.Executable.Path));
                    debugSettings.Add(new KeyValuePair<string, string>("CommandArguments", module.AdditionalData.DebugData.Command.Options.ToString(" ")));
                    debugSettings.Add(new KeyValuePair<string, string>("RemoteMachine", module.AdditionalData.DebugData.RemoteMachine));
                }
            }

            return userdata;
        }

        protected IEnumerable<string> ParsePS3UserFileOption(string userFileOptions, string[] options)
        {
            var indexSorter = new List<int>();
            // check for duplicate options
            foreach (string option in options)
            {
                MatchCollection mColl = Regex.Matches(userFileOptions, option + "[\\s|\n|\t|\r\n]*=");
                if (mColl.Count > 1)
                {
                    if (options.Length == 2)
                        throw new BuildException("\n\nError: Detected duplicate PS3 working directory options '" + option + "'!\n\n");
                    else
                        throw new BuildException("\n\nError: Detected duplicate PS3 command arguments options '" + option + "'!\n\n");
                }
                else if (mColl.Count == 1)
                {
                    int index = userFileOptions.IndexOf(option);
                    indexSorter.Add(index);
                }
            }
            indexSorter.Sort();
            var argsArray = new List<string>();
            userFileOptions = userFileOptions.TrimEnd(new char[] { '\n', '\r', '\t', ' ' });
            for (int i = 0; i < indexSorter.Count; i++)
            {
                int startIndex = (int)indexSorter[i];
                int endIndex = userFileOptions.Length;
                if (i < indexSorter.Count - 1)
                    endIndex = (int)indexSorter[i + 1];
                string userFileOption = userFileOptions.Substring(startIndex, endIndex - startIndex);
                MatchCollection mc = Regex.Matches(userFileOption, "=");
                if (mc.Count > 1 || mc.Count == 0)
                {
                    if (options.Length == 2)
                        Log.Warning.WriteLine("\n\nWARNING: Invalid working directory found for PS3.\n\n" +
                            "'" + userFileOption + "'\n\n" +
                            "Only 'HomeDir' or 'FileServDir' are valid options for setting PS3 working direcotry, and they are case sensitive!\n" +
                            "Example:\n<property name=\"runtime.module.workingdir\">\n\tHomeDir=c:\\packages\n\tFileServDir=c:\\packages\n</property>\n");
                    else
                        Log.Warning.WriteLine("\n\nWARNING: Invalid command arguments found for PS3.\n\n" +
                            "'" + userFileOption + "'\n\n" +
                            "Only 'DbgCmdLine', 'DbgElfArgs', 'RunCmdLin', and 'RunElfArgs' are valid options for setting PS3 command line args, and they are case sensitive!\n" +
                            "Example:\n<property name=\"runtime.module.commandargs\">\n\tDbgCmdLine=-r\n\tDbgElfArgs=-e\n</property>\n");
                }
                userFileOption = Regex.Replace(userFileOption, "[\\s|\n|\t|\r\n]*=", "=");
                argsArray.Add(userFileOption.Trim());
            }
            return argsArray;
        }

        protected virtual void GetReferences(out IEnumerable<ProjectRefEntry> projectReferences, out IDictionary<PathString, FileEntry> references, out IDictionary<PathString, FileEntry> comreferences)
        {
            var projectRefs = new List<ProjectRefEntry>();
            projectReferences = projectRefs;

            // Collect project referenced assemblies by configuration:
            var projectReferencedAssemblies = new Dictionary<Configuration, ISet<PathString>>();

            foreach (var depProject in Dependents.Where(d => d.Modules.Any(m => m.IsKindOf(Module.DotNet | Module.Managed))).Cast<VSProjectBase>())
            {
                var refEntry = new ProjectRefEntry(depProject);
                projectRefs.Add(refEntry);

                foreach (var module in Modules)
                {
                    uint copyLocal = 0;
                    if (module is Module_Native && ((module as Module_Native).CopyLocal == CopyLocalType.True || (module as Module_Native).CopyLocal == CopyLocalType.Slim))
                    {
                        copyLocal = ProjectRefEntry.CopyLocal;
                    }
                    else if (module is Module_DotNet && ((module as Module_DotNet).CopyLocal == CopyLocalType.True || (module as Module_DotNet).CopyLocal == CopyLocalType.Slim))
                    {
                        copyLocal = ProjectRefEntry.CopyLocal;
                    }

                    // Find dependent project configuration that corresponds to this project config:
                    Configuration dependentConfig;

                    var mapEntry = ConfigurationMap.Where(e => e.Value == module.Configuration);
                    if (mapEntry.Count() > 0 && ConfigurationMap.TryGetValue(mapEntry.First().Key, out dependentConfig))
                    {
                        var depModule = depProject.Modules.SingleOrDefault(m => m.Configuration == dependentConfig);
                        if (depModule != null)
                        {
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

                            if ((copyLocal & ProjectRefEntry.CopyLocal) != ProjectRefEntry.CopyLocal)
                            {
                                //Check if copylocal is defined in module dependents.
                                var dep = module.Dependents.FirstOrDefault(d => d.Dependent.Name == depProject.ModuleName && d.Dependent.Package.Name == depProject.Package.Name && d.Dependent.BuildGroup == depProject.Modules.FirstOrDefault().BuildGroup);
                                if (dep != null && dep.IsKindOf(DependencyTypes.CopyLocal))
                                {
                                    copyLocal |= ProjectRefEntry.CopyLocal;
                                }
                            }


                            refEntry.TryAddConfigEntry(module.Configuration, copyLocal);
                        }
                    }
                }
            }

            references = new Dictionary<PathString, FileEntry>();
            comreferences = new Dictionary<PathString, FileEntry>();

            foreach (var module in Modules.Where(m => m is Module_Native).Cast<Module_Native>())
            {
                if (module.Cc != null)
                {
                    if (module.IsKindOf(Module.Managed))
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
                            UpdateFileEntry(references, key, assembly, module.Cc.Assemblies.BaseDirectory, module.Configuration, flags: copyLocal);
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

                            uint copyLocal = 0;
                            if (module.CopyLocal == CopyLocalType.True)
                            {
                                copyLocal = FileEntry.CopyLocal;
                            }

                            var key = new PathString(Path.GetFileName(assembly.Path.Path));
                            UpdateFileEntry(comreferences, key, assembly, module.Cc.ComAssemblies.BaseDirectory, module.Configuration, flags: copyLocal);
                        }
                    }
                }
            }
        }

        #endregion Utility Methods

        #region Write Methods

        protected virtual void WriteVisualStudioProject(IXmlWriter writer)
        {
            writer.WriteStartElement("VisualStudioProject");
            writer.WriteAttributeString("ProjectType", ProjectType);
            writer.WriteAttributeString("Version", Version);
            writer.WriteAttributeString("Name", Name);
            writer.WriteAttributeString("ProjectGUID", ProjectGuidString);
            writer.WriteNonEmptyAttributeString("Keyword", Keyword);
            writer.WriteNonEmptyAttributeString("TargetFrameworkVersion", TargetFrameworkVersion);  
            writer.WriteNonEmptyAttributeString("RootNamespace", RootNamespace);
        }

        protected virtual void WritePlatforms(IXmlWriter writer)
        {
            // Compute app possible different platforms

            writer.WriteStartElement("Platforms");

            foreach (string platformName in Platforms)
            {
                writer.WriteStartElement("Platform");
                writer.WriteAttributeString("Name", platformName);
                writer.WriteEndElement(); //Platform
            }

            writer.WriteFullEndElement(); //Platforms
        }

        protected virtual void WriteToolFiles(IXmlWriter writer)
        {
            if(Modules.All(m=>m.IsKindOf(Module.MakeStyle)))
            {
                return;
            }
            
            writer.WriteStartElement("ToolFiles");

#if FRAMEWORK_PARALLEL_TRANSITION
            if(!BuildGenerator.GenerateSingleConfig)
            {
                writer.WriteStartElement("DefaultToolFile");
                writer.WriteAttributeString("FileName", "masm.rules");
                writer.WriteEndElement(); //DefaultToolFile
            }
#else
            writer.WriteStartElement("DefaultToolFile");
            writer.WriteAttributeString("FileName", "masm.rules");
            writer.WriteEndElement(); //DefaultToolFile
#endif
            foreach (var mod in Modules)
            {
                var native = mod as Module_Native;
                if(native != null && native.CustomBuildRuleFiles != null)
                {
                    foreach(var rule in native.CustomBuildRuleFiles.FileItems)
                    {
                    string cleanRule = CreateRuleFileFromTemplate(rule, mod);
                    
                    if (cleanRule.Length > 0)
                    {
                        writer.WriteStartElement("ToolFile");
                        writer.WriteAttributeString("RelativePath", PathUtil.RelativePath(cleanRule, OutputDir.Path));
                        writer.WriteEndElement();
                    }

                    }
                }
            }

            writer.WriteEndElement(); //ToolFiles
        }

        protected virtual void WriteConfigurations(IXmlWriter writer)
        {
            writer.WriteStartElement("Configurations");

            foreach (Module module in Modules)
            {
                WriteOneConfiguration(writer, module);
            }
            writer.WriteEndElement(); //Configurations
        }

        protected virtual void WriteOneConfiguration(IXmlWriter writer, Module module)
        {
            writer.WriteStartElement("Configuration");
            writer.WriteAttributeString("Name", GetVSProjConfigurationName(module.Configuration));
            
#if FRAMEWORK_PARALLEL_TRANSITION
            writer.WriteAttributeString("OutputDirectory", module.IntermediateDir.Path);
            writer.WriteAttributeString("IntermediateDirectory", Path.Combine(module.IntermediateDir.Path, "vcproj"));
#else
            writer.WriteAttributeString("OutputDirectory", module.OutputDir.Path);
            writer.WriteAttributeString("IntermediateDirectory", module.IntermediateDir.Path);
#endif
            writer.WriteAttributeString("CharacterSet", ((int)CharacterSet(module)).ToString());
            writer.WriteAttributeString("ConfigurationType", GetVCConfigurationType(module));
            

            //Process following attributes:
            //      "ManagedExtensions"
            //      "UseOfMFC"
            //      "UseOfATL"
            //      "WholeProgramOptimization"

            if (module.IsKindOf(Module.MakeStyle))
            {
                WriteMakeTool(writer, "VCNMakeTool", module.Configuration, module);
            }
            else
            {
                IDictionary<string, string> nameToXMLValue = new SortedDictionary<string, string>();

                CcCompiler cc = module.Tools.SingleOrDefault(t => t.ToolName == "cc") as CcCompiler;

                if (cc != null)
                {
                    ProcessSwitches(VSConfig.GetParseDirectives(module).General, nameToXMLValue, cc.Options, "general", false);
                }

                Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;
                if (link != null)
                {
                    ProcessSwitches(VSConfig.GetParseDirectives(module).General, nameToXMLValue, link.Options, "general", false);
                }

                WriteNamedAttributes(writer, nameToXMLValue);

                // Write tools:

#if FRAMEWORK_PARALLEL_TRANSITION
                WriteBuildEvents(writer, "VCPreBuildEventTool", "prebuild", BuildStep.PreBuild, module);
                WriteBuildEvents(writer, "VCCustomBuildTool", "custombuild", BuildStep.PostBuild, module);
#endif
                WriteCustomBuildRuleTools(writer, module as Module_Native);

                var map = GetTools(module.Configuration);
                for (int i = 0; i < map.GetLength(0); i++)
                {

#if FRAMEWORK_PARALLEL_TRANSITION
                    if (map[i, 0] == "VCALinkTool")
                    {
                        WriteBuildEvents(writer, "VCPreLinkEventTool", "prelink", BuildStep.PreLink, module);
                    }
#endif
                    WriteOneTool(writer, map[i, 0], module.Tools.FirstOrDefault(t => t.ToolName == map[i, 1]), module);
                }

#if FRAMEWORK_PARALLEL_TRANSITION
                WriteBuildEvents(writer, "VCPostBuildEventTool", "postbuild", BuildStep.PostBuild, module);
#else
                WriteBuildEvents(writer, "VCPreBuildEventTool", "prebuild", BuildStep.PreBuild, module);
                WriteBuildEvents(writer, "VCPreLinkEventTool", "prelink", BuildStep.PreLink, module);
                WriteBuildEvents(writer, "VCPostBuildEventTool", "postbuild", BuildStep.PostBuild, module);
                WriteBuildEvents(writer, "VCCustomBuildTool", "custombuild", BuildStep.PostBuild, module);
#endif
            }

             
            writer.WriteEndElement(); //Configuration
        }

        protected virtual void WriteOneTool(IXmlWriter writer, string vcToolName, Tool tool, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
             WriteToolDelegate writetoolmethod;

            if(_toolMethods.TryGetValue(vcToolName, out writetoolmethod))
            {
                writetoolmethod(writer, vcToolName, tool, module.Configuration, module, nameToDefaultXMLValue, file);
            }
            else
            {
               Log.Warning.WriteLine("Don't know how to write tool '{0}'", vcToolName);
            }

        }

        protected virtual void WriteReferences(IXmlWriter writer)
        {
            IEnumerable<ProjectRefEntry> projectReferences;
            IDictionary<PathString, FileEntry> references;
            IDictionary<PathString, FileEntry> comreferences;

            GetReferences(out projectReferences, out references, out comreferences);

            //IMTODO: add COM references
#if FRAMEWORK_PARALLEL_TRANSITION
#else
            if (projectReferences.Count() > 0 || references.Count() > 0)
#endif
            {
                writer.WriteStartElement("References");

                foreach (var projectRefEntry in projectReferences)
                {
                    writer.WriteStartElement("ProjectReference");
                    {
                        //In VS2008 we can't put conditions by configuration. If any config declares copy local, then we use copy local;
                        bool isCopyLocal = null != projectRefEntry.ConfigEntries.Find(ce => ce.IsKindOf(ProjectRefEntry.CopyLocal));

                        writer.WriteAttributeString("ReferencedProjectIdentifier", projectRefEntry.ProjectRef.ProjectGuidString);
                        writer.WriteAttributeString("CopyLocal", isCopyLocal.ToString());
                        writer.WriteAttributeString("RelativePathToProject", Path.Combine(projectRefEntry.ProjectRef.RelativeDir, projectRefEntry.ProjectRef.ProjectFileName));
                    }
                    writer.WriteEndElement(); //
                }

                foreach (var assemblyRef in references.Values)
                {

                    writer.WriteStartElement("AssemblyReference");
                    {
                        // If file is declared asis, keep it as a file name. No relative path.
                        // In VS2008 we can't put conditions by configuration. If any config declares file asIs, then we use it asis;
                        bool isFileAsIs = null != assemblyRef.ConfigEntries.Find(ce => ce.FileItem.AsIs);
                        var refpath = isFileAsIs ? assemblyRef.Path.Path : PathUtil.RelativePath(assemblyRef.Path.Path, OutputDir.Path, addDot: true);

                        //In VS2008 we can't put conditions by configuration. If any config declares copy local, then we use copy local;
                        bool isCopyLocal = null != assemblyRef.ConfigEntries.Find(ce => ce.IsKindOf(FileEntry.CopyLocal));

                        writer.WriteAttributeString("RelativePath", refpath);
                        writer.WriteAttributeString("CopyLocal", isCopyLocal.ToString());
                    }
                    writer.WriteEndElement();
                }

                WriteComReferences(writer, comreferences.Values);

                writer.WriteFullEndElement(); //References
            }
        }

        private enum RegKind
        {
            RegKind_Default = 0,
            RegKind_Register = 1,
            RegKind_None = 2
        }

        [DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern void LoadTypeLibEx(string strTypeLibName, RegKind regKind, out System.Runtime.InteropServices.ComTypes.ITypeLib typeLib);

        protected bool GenerateInteropAssembly(PathString comLib, out Guid libGuid, out System.Runtime.InteropServices.ComTypes.TYPELIBATTR typeLibAttr)
        {
            if (!File.Exists(comLib.Path))
            {
                string msg = String.Format("Unable to generate interop assembly for COM reference, file '{0}' not found", comLib.Path);
                throw new BuildException(msg);
            }

            System.Runtime.InteropServices.ComTypes.ITypeLib typeLib = null;
            try
            {
                LoadTypeLibEx(comLib.Path, RegKind.RegKind_None, out typeLib);
            }
            catch (Exception ex)
            {
                string msg = String.Format("Unable to generate interop assembly for COM reference '{0}'.", comLib.Path);
                new BuildException(msg, ex);
            }
            if (typeLib == null)
            {
                string msg = String.Format("Unable to generate interop assembly for COM reference '{0}'.", comLib.Path);
                new BuildException(msg);
            }

            libGuid = Marshal.GetTypeLibGuid(typeLib);

            IntPtr pTypeLibAttr = IntPtr.Zero;
            typeLib.GetLibAttr(out pTypeLibAttr);

            typeLibAttr = (System.Runtime.InteropServices.ComTypes.TYPELIBATTR)Marshal.PtrToStructure(pTypeLibAttr, typeof(System.Runtime.InteropServices.ComTypes.TYPELIBATTR));

            return true;
        }

        protected virtual void WriteComReferences(IXmlWriter writer, ICollection<FileEntry> comreferences)
        {
            if (comreferences.Count > 0)
            {
                foreach (var fe in comreferences)
                {
                    Guid libGuid;
                    System.Runtime.InteropServices.ComTypes.TYPELIBATTR typeLibAttr;

                    if(GenerateInteropAssembly(fe.Path, out libGuid, out typeLibAttr))
                    {
                        writer.WriteStartElement("ActiveXReference");
                        writer.WriteAttributeString("ControlGUID", "{" + libGuid.ToString() + "}");
                        writer.WriteAttributeString("ControlVersion", typeLibAttr.wMajorVerNum.ToString() + "." + typeLibAttr.wMinorVerNum.ToString());
                        writer.WriteAttributeString("LocaleID", typeLibAttr.lcid.ToString());
                        writer.WriteAttributeString("WrapperTool", "tlbimp");
                        writer.WriteEndElement(); //ActiveXReference
                    }
                }
            }
        }

        protected virtual void WriteFiles(IXmlWriter writer)
        {

            var files = GetAllFiles(tool =>
            {
                var filesets = new List<Tuple<FileSet, uint, Tool>>();

                if (tool is CcCompiler) filesets.Add(Tuple.Create((tool as CcCompiler).SourceFiles, FileEntry.Buildable, tool));                
                if (tool is AsmCompiler) filesets.Add(Tuple.Create((tool as AsmCompiler).SourceFiles, FileEntry.Buildable, tool));
                if (tool is BuildTool) filesets.Add(Tuple.Create((tool as BuildTool).Files, FileEntry.Buildable, tool));
                if (tool is BuildTool && tool.ToolName=="rc") filesets.Add(Tuple.Create((tool as BuildTool).InputDependencies, FileEntry.Buildable|FileEntry.Resources, null as Tool));
                if (tool is BuildTool && tool.ToolName == "resx") filesets.Add(Tuple.Create((tool as BuildTool).InputDependencies, FileEntry.Buildable | FileEntry.Resources, null as Tool));

                return filesets;
            }).Select(e => e.Value).OrderBy(e => e.Path.Path);

            WriteFilesWithFilters(writer, FileFilters.ComputeFilters(files, AllConfigurations, Modules));
        }

        protected virtual void WriteFilesWithFilters(IXmlWriter writer, FileFilters filters)
        {
            writer.WriteStartElement("Files");

            filters.ForEach(
                (entry) =>
                {
                    if (!String.IsNullOrEmpty(entry.FilterName))
                    {
                        writer.WriteStartElement("Filter");
                        writer.WriteAttributeString("Name", entry.FilterName);
                        writer.WriteAttributeString("Filter", String.Empty);
                    }
                    foreach (var file in entry.Files)
                    {
                        if (Path.GetExtension(file.Path.Path).ToLowerInvariant() != ".resx")
                        {
                            WriteFile(writer, file);
                        }
                    }
                },
                (entry) =>
                {
                    if (!String.IsNullOrEmpty(entry.FilterName))
                    {
                        writer.WriteEndElement();
                    }
                }
            );
            writer.WriteFullEndElement(); //Files
        }

        protected virtual void WriteFile(IXmlWriter writer, FileEntry fileentry)
        {
            writer.WriteStartElement("File");
            writer.WriteAttributeString("RelativePath", PathUtil.RelativePath(fileentry.Path.Path, OutputDir.Path));

            var extension = Path.GetExtension(fileentry.Path.Path).ToLower();

            var active = new HashSet<string>();
            foreach (var configentry in fileentry.ConfigEntries)
            {
                if (configentry.IsKindOf(FileEntry.NonBuildable))
                {
                    writer.WriteStartElement("FileConfiguration");
                    writer.WriteAttributeString("Name", GetVSProjConfigurationName(configentry.Configuration));
                    writer.WriteStartElement("Tool");
                    writer.WriteAttributeString("Name", "VCCustomBuildTool");
                    writer.WriteEndElement();
                    writer.WriteEndElement();
                }
                else if (configentry.IsKindOf(FileEntry.CustomBuildTool) || (!configentry.IsKindOf(FileEntry.Headers | FileEntry.Assets|FileEntry.Resources) && !HEADER_EXTENSIONS.Contains(extension)))
                {
                    writer.WriteStartElement("FileConfiguration");
                    writer.WriteAttributeString("Name", GetVSProjConfigurationName(configentry.Configuration));
                    if (configentry.IsKindOf(FileEntry.ExcludedFromBuild) && !configentry.IsKindOf(FileEntry.Headers))
                    {
                        writer.WriteAttributeString("ExcludedFromBuild", "TRUE");
                    }
                    Tool tool = configentry.FileItem.GetTool() ?? configentry.Tool;

                    string vctoolName = GetToolMapping(configentry.Configuration, tool);
                    if (vctoolName != null)
                    {

#if FRAMEWORK_PARALLEL_TRANSITION
                        //This is a bug in nanttovstools, but for backwards compatibility I reproduce it in transition mode:
                        if (configentry.Configuration.System=="xenon" && vctoolName == "VCCLX360CompilerTool")
                        {
                            vctoolName = "VCCLCompilerTool";
                        }
#endif
                        if (!configentry.IsKindOf(FileEntry.ExcludedFromBuild))
                        {
                            WriteOneTool(writer, vctoolName, configentry.FileItem.GetTool(), Modules.SingleOrDefault(m => m.Configuration == configentry.Configuration) as Module, null, configentry.FileItem);
                        }
                        else
                        {
                            writer.WriteStartElement("Tool");
                            writer.WriteAttributeString("Name", vctoolName);
                            writer.WriteEndElement();
                        }
                    }
                    else
                    {
                        var ext = Path.GetExtension(fileentry.Path.Path).ToLower().ToLower();

                        if (SOURCE_EXTENSIONS.Contains(ext))
                        {

                            //IMTODO: Why do we have buildable files with with TOOL == null? Files can be assets, etc. Need to check it here?
                            writer.WriteStartElement("Tool");
#if FRAMEWORK_PARALLEL_TRANSITION
                            writer.WriteAttributeString("Name", "VCCLCompilerTool");
#else
                            writer.WriteAttributeString("Name", GetToolMapping(configentry.Configuration, "cc"));
#endif
                            writer.WriteAttributeString("ObjectFile", Path.GetFileName(fileentry.Path.Path) + ".obj");
                            writer.WriteEndElement();
                        }

                    }
                    writer.WriteEndElement(); // FileConfiguration
                }
                else if(!configentry.IsKindOf(FileEntry.Headers) && !HEADER_EXTENSIONS.Contains(extension))
                {
                    if (configentry.IsKindOf(FileEntry.ExcludedFromBuild))
                    {
                        writer.WriteStartElement("FileConfiguration");
                        writer.WriteAttributeString("Name", GetVSProjConfigurationName(configentry.Configuration));
                        writer.WriteAttributeString("ExcludedFromBuild", "true".ToUpper());
                        writer.WriteEndElement();
                    }
                    else
                    {
                        if (configentry.IsKindOf(FileEntry.Assets))
                        {
                            writer.WriteStartElement("FileConfiguration");
                            writer.WriteAttributeString("Name", GetVSProjConfigurationName(configentry.Configuration));
                            writer.WriteStartElement("Tool");
                            writer.WriteAttributeString("Name", "VCCustomBuildTool");
                            writer.WriteEndElement();
                            writer.WriteEndElement();
                        }
                    }
                }

                active.Add(configentry.Configuration.Name);
            }
            // Write excluded from build files (there are no tools for these):
            foreach (var config in AllConfigurations.Where(c => !active.Contains(c.Name)))
            {
                writer.WriteStartElement("FileConfiguration");
                writer.WriteAttributeString("Name", GetVSProjConfigurationName(config));
                {
                    bool isExcluded = !fileentry.IsKindOf(config, FileEntry.Headers | FileEntry.Resources) || fileentry.IsKindOf(config, FileEntry.ExcludedFromBuild);

                    writer.WriteAttributeString("ExcludedFromBuild", isExcluded.ToString().ToUpper());
                    if (!isExcluded)
                    {
                        writer.WriteStartElement("Tool");
                        writer.WriteAttributeString("Name", "VCCustomBuildTool");
                        writer.WriteEndElement();
                    }
                }
                writer.WriteEndElement();
            }

            // This section is for handling WinForm applications Form headers and their associated resX files
            // It's assumed that formx.h and its resX file be in the same directory.
            if (HEADER_EXTENSIONS.Contains(extension))
            {
                string resXfilepath = Path.ChangeExtension(fileentry.Path.Path, ".resX");
                if (File.Exists(resXfilepath))
                {
                    writer.WriteAttributeString("FileType", "3"); // filetype = 3 for form file. (4 for user control file, but not supported)
                    writer.WriteStartElement("File");
                    writer.WriteAttributeString("RelativePath", PathUtil.RelativePath(resXfilepath, OutputDir.Path));
                    foreach (var cfggentry in fileentry.ConfigEntries)
                    {
                        writer.WriteStartElement("FileConfiguration");
                        writer.WriteAttributeString("Name", GetVSProjConfigurationName(cfggentry.Configuration));
                        if (cfggentry.IsKindOf(FileEntry.ExcludedFromBuild))
                        {
                            writer.WriteAttributeString("ExcludedFromBuild", "TRUE");
                        }
                        writer.WriteStartElement("Tool");
                        writer.WriteAttributeString("Name", "VCManagedResourceCompilerTool");
                        string resourceName = @"$(IntDir)\" + Name + "." + Path.GetFileNameWithoutExtension(resXfilepath) + @".resources";
                        writer.WriteAttributeString("ResourceFileName", resourceName);
                        writer.WriteEndElement(); // Tool
                        writer.WriteEndElement(); //Confifuration
                    }
                    writer.WriteEndElement(); // File
                }
            }


#if FRAMEWORK_PARALLEL_TRANSITION
            writer.WriteFullEndElement();//File
#else
            writer.WriteEndElement();//File
#endif
        }

        protected virtual void WriteGlobals(IXmlWriter writer)
        {            
            writer.WriteStartElement("Globals");
            foreach (var global in Globals)
            {
                writer.WriteStartElement("Global");
                writer.WriteAttributeString("Name", global.Key);
                writer.WriteAttributeString("Value", global.Value);
                writer.WriteEndElement();
            }
            writer.WriteFullEndElement(); //Globals
        }

        protected virtual void WriteNamedAttributes(IXmlWriter writer, IDictionary<string, string> attributes)
        {
            foreach (KeyValuePair<string, string> item in attributes)
            {
                writer.WriteAttributeString(item.Key, item.Value);
            }
        }

        #region Write Tools

        protected virtual void WriteCustomBuildRuleTools(IXmlWriter writer, Module_Native module)
        {
            if (module != null && module.CustomBuildRuleOptions != null)
            {
                foreach (var o in module.CustomBuildRuleOptions.Options)
                {
                    WriteOneCustomBuildRuleTool(writer, o.Key, o.Value, module);
                }
            }
        }

        protected virtual void WriteOneCustomBuildRuleTool(IXmlWriter writer, string name, string toolOptionsName, IModule module)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            OptionSet options;
            if(module.Package.Project.NamedOptionSets.TryGetValue(toolOptionsName, out options))
            {
                foreach (var o in options.Options)
                {
                    writer.WriteAttributeString(o.Key, o.Value);
                }
            }

            writer.WriteEndElement();
        }

        protected virtual void WriteBuildEvents(IXmlWriter writer, string name, string toolname, uint type, Module module)
        {

            StringBuilder cmd = new StringBuilder();
            StringBuilder input = new StringBuilder();
            StringBuilder output = new StringBuilder();

            

            if (toolname == "custombuild")
            {
                if (module.Configuration.System == "xenon" && GetVCConfigurationType(module) == "10")
                {
                        Log.Warning.WriteLine("Custom build steps are not supported for Utility module '{0}' on XENON configurations." + Environment.NewLine +
                            "If the module is of 'Utility' type, it's assigned the 'Xenon Utility' type in its Visual Studio project." +
                            "However VS Xenon Utility type only has pre/post build steps and it does not support custom build steps.", module.BuildGroup + "." + module.Name);
                        return;
                }
                // This is custom build step:
                foreach (var step in module.BuildSteps.Where(step => step.Name == toolname))
                {
                    input.Append(step.InputDependencies.ToString(";", dep => dep.Path.Quote()));
                    output.Append(step.OutputDependencies.ToString(";", dep => dep.Path.Quote()));

                    foreach (var command in step.Commands)
                    {
                        cmd.AppendLine(command.CommandLine);
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
                                cmd.AppendLine(NantInvocationProperties.TargetToNantCommand(module, targetCommand, addGlobal: true, normalizePathString: normalizeFunc));
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

                foreach (var step in module.BuildSteps.Where(step => step.IsKindOf(type) && step.Name == toolname))
                {
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

                                cmd.AppendLine(NantInvocationProperties.TargetToNantCommand(module, targetCommand, addGlobal: true, normalizePathString: normalizeFunc));
                            }

                        }
                    }
                }
            }

            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            if (cmd.Length > 0 )
            {
                var cmd_str = cmd.ToString().TrimWhiteSpace();
                if (!String.IsNullOrEmpty(cmd_str))
                {

#if FRAMEWORK_PARALLEL_TRANSITION
                    if (BuildGenerator.GenerateSingleConfig)
                    {
                        writer.WriteNonEmptyAttributeString("AdditionalDependencies", input.ToString().TrimWhiteSpace());
                        writer.WriteNonEmptyAttributeString("Outputs", output.ToString().TrimWhiteSpace());
                    }
                    else
                    {
                        writer.WriteNonEmptyAttributeString("AdditionalDependencies", input.ToString().TrimWhiteSpace());
                        writer.WriteNonEmptyAttributeString("Outputs", output.ToString().TrimWhiteSpace());
                    }
#else
                    writer.WriteNonEmptyAttributeString("AdditionalDependencies", input.ToString().TrimWhiteSpace());
                    writer.WriteNonEmptyAttributeString("Outputs", output.ToString().TrimWhiteSpace());
                    
#endif
                    writer.WriteAttributeString("CommandLine", cmd_str);
                    
                }
            }
            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteXMLDataGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteWebServiceProxyGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteMIDLTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            BuildTool midl = tool as BuildTool;
            if (midl != null)
            {
                var nameToXMLValue = new SortedDictionary<string, string>();
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

                ProcessSwitches(VSConfig.GetParseDirectives(module).Midl, nameToXMLValue, midl.Options, "midl", true, nameToDefaultXMLValue);

                foreach (var item in nameToXMLValue)
                {
                    writer.WriteAttributeString(item.Key, item.Value);
                }
            }

            writer.WriteEndElement(); //Tool
        }

        protected virtual void WriteAsmCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if (file != null)  // File references common tool settings:
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                FileData filedata = file.Data as FileData;
                if (filedata != null && filedata.ObjectFile != null)
                {
                    writer.WriteAttributeString("ObjectFile", filedata.ObjectFile.Path);
                }

                writer.WriteEndElement(); //Tool
            }
#if FRAMEWORK_PARALLEL_TRANSITION
            else
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);
                writer.WriteEndElement(); //Tool

            }
#endif
        }

        protected virtual void WriteCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            CcCompiler cc = tool as CcCompiler;

            if (cc != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                IDictionary<string, string> nameToXMLValue;
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string,string>();

                InitCompilerToolProperties(module, config, GetAllIncludeDirectories(cc), GetAllUsingDirectories(cc), GetAllDefines(cc, config).ToString(";"), out nameToXMLValue, nameToDefaultXMLValue);

                ProcessSwitches(VSConfig.GetParseDirectives(module).Cc, nameToXMLValue, cc.Options, "cc", true, nameToDefaultXMLValue);

                //Apply default attributes
                foreach (var item in nameToXMLValue)
                {
                    writer.WriteAttributeString(item.Key, item.Value);
                }

                if (file != null)
                {
                    FileData filedata = file.Data as FileData;
                    if (filedata != null && filedata.ObjectFile != null)
                    {
                        writer.WriteAttributeString("ObjectFile", filedata.ObjectFile.Path);
                    }
                }

                writer.WriteEndElement(); //Tool
            }
            else if (file != null)  // File references common tool settings:
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                FileData filedata = file.Data as FileData;
                if (filedata != null && filedata.ObjectFile != null)
                {
                    writer.WriteAttributeString("ObjectFile", filedata.ObjectFile.Path);
                }

                writer.WriteEndElement(); //Tool

            }
        }

        protected virtual void WriteLibrarianTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            Librarian lib = tool as Librarian;

            if (lib != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                IDictionary<string, string> nameToXMLValue;
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string,string>();

                InitLibrarianToolProperties(config, lib.ObjectFiles.FileItems.Select(item => item.Path), PrimaryOutput(module), out nameToXMLValue, nameToDefaultXMLValue);

                ProcessSwitches(VSConfig.GetParseDirectives(module).Lib, nameToXMLValue, lib.Options, "lib", true, nameToDefaultXMLValue);

                //Apply default attributes
                foreach (var item in nameToXMLValue)
                {
                    writer.WriteAttributeString(item.Key, item.Value);
                }

                writer.WriteEndElement(); //Tool
            }
        }

        protected virtual void WriteManagedResourceCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            if (file != null && tool != null)
            {
                // Output should have only one file:
                var fi = tool.OutputDependencies.FileItems.FirstOrDefault();
                if (fi != null)
                {
                    writer.WriteAttributeString("ResourceFileName", PathUtil.RelativePath(fi.Path.Path, OutputDir.Path));
                }
            }

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteResourceCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);
            if (tool != null)
            {
                Module_Native native = module as Module_Native;
                if (native != null)
                {
                    if (file == null)
                    {
                        // Add include directories only in generic tool.
                        var includedirs = tool.Options.Where(o=>o.StartsWith("/i ") || o.StartsWith("-i ")).Select(o=>o.Substring(3).TrimWhiteSpace());
                        // Add include directories only in generic tool.
                        writer.WriteAttributeString("AdditionalIncludeDirectories", includedirs.ToString(";", s=>s.Quote()));
                    }
                    else
                    {
                        var outputresourcefile = tool.OutputDependencies.FileItems.SingleOrDefault().Path.Path;
                        writer.WriteAttributeString("ResourceOutputFileName", outputresourcefile);
                    }
                }
            }
#if FRAMEWORK_PARALLEL_TRANSITION
            else
            {
                Module_Native native = module as Module_Native;
                if (native != null)
                {
                    writer.WriteAttributeString("AdditionalIncludeDirectories", GetAllIncludeDirectories(native.Cc));
                }
                else 
                {
                    Module_Utility utility = module as Module_Utility;
                    if (utility != null)
                    {
                        var includedirs = (module.Package.Project.Properties[module.GroupName + ".includedirs"] + Environment.NewLine 
                            + module.Package.Project.Properties[module.GroupName + ".includedirs." + module.Configuration.System]).TrimWhiteSpace(); 

                        if (String.IsNullOrEmpty(includedirs))
                        {
                           includedirs = (Path.Combine(module.Package.Project.Properties["package.dir"], "include") +  Environment.NewLine 
                               + Path.Combine(module.Package.Project.Properties["package.dir"], module.Package.Project.Properties["eaconfig." + module.BuildGroup + ".sourcedir"]));
                        }
                        writer.WriteAttributeString("AdditionalIncludeDirectories", includedirs.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct().ToString(";", PathString.Quote));
                    }

                }
            }
#endif
            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteLinkerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            
            Linker linker = tool as Linker;

            if (linker != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                IDictionary<string, string> nameToXMLValue;
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string,string>();

                // Resources are linked by VisualStudio. Exclude from object files
                var resources = module.Tools.Where(t => t.ToolName == "rc").Select(t => t.OutputDependencies.FileItems.SingleOrDefault().Path);

                InitLinkerToolProperties(config, linker, GetAllLibraries(linker, module), GetAllLibraryDirectories(linker), linker.ObjectFiles.FileItems.Select(item => item.Path).Except(resources), PrimaryOutput(module), out nameToXMLValue, nameToDefaultXMLValue);

                ProcessSwitches(VSConfig.GetParseDirectives(module).Link, nameToXMLValue, linker.Options, "link", true, nameToDefaultXMLValue);


                if (nameToXMLValue.ContainsKey("GenerateManifest") && ConvertUtil.ToBoolean(nameToXMLValue["GenerateManifest"]))
                {
                    if (nameToXMLValue.ContainsKey("ManifestFile"))
                    {
                        //Because of how VC handles incremental link with manifest we need to change replace manifest file name
#if FRAMEWORK_PARALLEL_TRANSITION
                        nameToXMLValue["ManifestFile"] = PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, module.Configuration.Name, Path.GetFileName(PrimaryOutput(module)) + @".intermediate.manifest"), OutputDir.Path);
#else
                        nameToXMLValue["ManifestFile"] = PathUtil.RelativePath(Path.Combine(module.IntermediateDir.Path, Path.GetFileName(PrimaryOutput(module)) + @".intermediate.manifest"), OutputDir.Path);
#endif
                    }
                }
                else
                {
                    nameToXMLValue.Remove("ManifestFile");
                }

                //Apply default attributes
                foreach (KeyValuePair<string, string> item in nameToXMLValue)
                {
                    writer.WriteAttributeString(item.Key, item.Value);
                }
                writer.WriteEndElement(); //Tool
            }
            else if(module.IsKindOf(Module.Program | Module.DynamicLibrary | Module.Managed))
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);
                writer.WriteEndElement(); //Tool
            }
            else if (module.IsKindOf(Module.Utility) && GetVCConfigurationType(module) != "10")
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);
                writer.WriteAttributeString("IgnoreLibraryLibrary", "true");
                writer.WriteAttributeString("LinkLibraryDependencies", "false");
                writer.WriteAttributeString("GenerateManifest", "false");
                writer.WriteEndElement(); //Tool
            }
        }

        protected virtual void WriteALinkTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteManifestTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if(module.IsKindOf(Module.Utility))
            {
                return;
            }

            var manifest = tool as BuildTool;
            if (manifest != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                if (manifest.Files.FileItems.Count > 0)
                {
                    writer.WriteAttributeString("AdditionalManifestFiles", manifest.Files.FileItems.ToString(";", f => f.Path.Path.Quote()).TrimWhiteSpace());
                }
                if (manifest.InputDependencies.FileItems.Count > 0)
                {
                    writer.WriteAttributeString("InputResourceManifests", manifest.InputDependencies.FileItems.ToString(";", f => f.Path.Path.Quote()).TrimWhiteSpace());
                }

                foreach (var fi in manifest.OutputDependencies.FileItems)
                {
#if FRAMEWORK_PARALLEL_TRANSITION
                    if (fi.Path.Path.EndsWith(".res"))
                    {
                        if (BuildGenerator.GenerateSingleConfig)
                        {
                            //IM: IT IS BUG in NANTTOVSTOOLS to add config subfolder here(module.Configuration.Name) because vcproj is already under configbuilddir.
                            // but for compatibility with old projects we do it here:                            
                            writer.WriteAttributeString("ManifestResourceFile", PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, module.Configuration.Name, Path.GetFileName(PrimaryOutput(module)) + @".embed.manifest.res"), OutputDir.Path));
                        }
                        else
                        {
                            writer.WriteAttributeString("ManifestResourceFile", PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, Path.GetFileName(PrimaryOutput(module)) + @".embed.manifest.res"), OutputDir.Path));
                        }

                    }
                    else
                    {
                        if (BuildGenerator.GenerateSingleConfig)
                        {
                            //IM: IT IS BUG in NANTTOVSTOOLS to add config subfolder here(module.Configuration.Name) because vcproj is already under configbuilddir.
                            // but for compatibility with old projects we do it here:
                            writer.WriteAttributeString("OutputManifestFile", PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, module.Configuration.Name, Path.GetFileName(PrimaryOutput(module)) + @".embed.manifest"), OutputDir.Path));
                        }
                        else
                        {
                            writer.WriteAttributeString("OutputManifestFile", PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, Path.GetFileName(PrimaryOutput(module)) + @".embed.manifest"), OutputDir.Path));
                        }
                    }
#else
                    if (fi.Path.Path.EndsWith(".res"))
                    {
                        writer.WriteAttributeString("ManifestResourceFile", PathUtil.RelativePath(fi.Path.Path, OutputDir.Path));
                    }
                    else
                    {

                        writer.WriteAttributeString("OutputManifestFile", PathUtil.RelativePath(fi.Path.Path, OutputDir.Path));
                    }
#endif
                }
                writer.WriteEndElement(); //Tool
            }
#if FRAMEWORK_PARALLEL_TRANSITION
            else
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                var outputName = module.Tools.Any(t => t.ToolName == "link") ? Path.GetFileName(PrimaryOutput(module)) : Name;
                if (BuildGenerator.GenerateSingleConfig)
                {
                    //IM: IT IS BUG in NANTTOVSTOOLS to add config subfolder here(module.Configuration.Name) because vcproj is already under configbuilddir.
                    // but for compatibility with old projects we do it here:
                    writer.WriteAttributeString("OutputManifestFile", PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, module.Configuration.Name, outputName + @".embed.manifest"), OutputDir.Path));
                    writer.WriteAttributeString("ManifestResourceFile", PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, module.Configuration.Name, outputName + @".embed.manifest.res"), OutputDir.Path));
                }
                else
                {
                    writer.WriteAttributeString("OutputManifestFile", PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, outputName + @".embed.manifest"), OutputDir.Path));
                    writer.WriteAttributeString("ManifestResourceFile", PathUtil.RelativePath(Path.Combine(module.Package.PackageConfigBuildDir.Path, outputName + @".embed.manifest.res"), OutputDir.Path));
                }
                writer.WriteEndElement(); //Tool
            }
#endif
        }

        protected virtual void WriteXDCMakeTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteBscMakeTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteFxCopTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteMakeTool(IXmlWriter writer, string name, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            string build;
            string rebuild;
            string clean;

            GetMakeToolCommands(module, out build, out rebuild, out clean);

            writer.WriteAttributeString("BuildCommandLine", build);
            writer.WriteAttributeString("ReBuildCommandLine", rebuild);
            writer.WriteAttributeString("CleanCommandLine", clean);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void GetMakeToolCommands(Module module, out string build, out string rebuild, out string clean)
        {
            var buildSb = new StringBuilder();
            var rebuildSb = new StringBuilder();
            var cleanSb = new StringBuilder();

            foreach (var step in module.BuildSteps)
            {
                if (step.IsKindOf(BuildStep.Build))
                {
                    foreach (var command in step.Commands)
                    {
                        buildSb.AppendLine(command.CommandLine);
                    }
                }
                else if (step.IsKindOf(BuildStep.Clean))
                {
                    foreach (var command in step.Commands)
                    {
                        cleanSb.AppendLine(command.CommandLine);
                    }
                }
                else if (step.IsKindOf(BuildStep.ReBuild))
                {
                    foreach (var command in step.Commands)
                    {
                        rebuildSb.AppendLine(command.CommandLine);
                    }
                }
            }

            build = buildSb.ToString().TrimWhiteSpace();
            rebuild = rebuildSb.ToString().TrimWhiteSpace();
            clean = cleanSb.ToString().TrimWhiteSpace();

            if (BuildGenerator.IsPortable)
            {
                build = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(build, OutputDir.Path);
                rebuild = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(rebuild, OutputDir.Path);
                clean = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(clean, OutputDir.Path);
            }
        }


        protected virtual void WriteAppVerifierTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if (tool != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);
                writer.WriteEndElement(); //Tool
            }
        }

        protected virtual void WriteWebDeploymentTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if (tool != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                writer.WriteEndElement(); //Tool
            }

        }

        protected virtual void WriteVCCustomBuildTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            var filetool = tool as BuildTool;
            if (filetool != null && file != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", "VCCustomBuildTool");

                writer.WriteAttributeString("CommandLine", GetCommandScriptWithCreateDirectories(filetool));

                writer.WriteNonEmptyAttributeString("Description", filetool.Description.TrimWhiteSpace());

                writer.WriteNonEmptyAttributeString("AdditionalDependencies", filetool.InputDependencies.FileItems.Select(fi => fi.Path.Path).OrderedDistinct().ToString(";", path => path.Quote()).TrimWhiteSpace());
                writer.WriteNonEmptyAttributeString("Outputs", filetool.OutputDependencies.FileItems.Select(fi => fi.Path.Path).ToString(";", path => path.Quote()).TrimWhiteSpace());

                writer.WriteEndElement();
            }
        }

        protected virtual void WriteX360ImageTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if(module.IsKindOf(Module.Utility))
            {
                return;
            }

            if (module.IsKindOf(Module.Program | Module.DynamicLibrary))
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                if (tool != null)
                {
                    var nameToXMLValue = new SortedDictionary<string, string>();
                    nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

                    ProcessSwitches(VSConfig.GetParseDirectives(module).Ximg, nameToXMLValue, tool.Options, "Image Builder", true, nameToDefaultXMLValue);

                    //Apply default attributes
                    foreach (KeyValuePair<string, string> item in nameToXMLValue)
                    {
                        writer.WriteAttributeString(item.Key, item.Value);
                    }
                }

                writer.WriteEndElement(); //Tool
            }

        }

        protected virtual void WriteX360DeploymentTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if(module.IsKindOf(Module.Utility))
            {
                return;
            }

            if (module.IsKindOf(Module.Program | Module.DynamicLibrary))
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                BuildTool deploy = tool as BuildTool;

                if (deploy == null)
                {
                    writer.WriteAttributeString("DeploymentType", "0");
                    writer.WriteAttributeString("DeploymentFiles", "");
                    writer.WriteAttributeString("ExcludedFromBuild", "TRUE");
                }
                else
                {
                    if (deploy.Options.Contains("-dvd"))
                    {
                        writer.WriteAttributeString("DeploymentType", "1");
                        writer.WriteAttributeString("DeploymentFiles", deploy.OutputDir.Path);
                    }
                    else
                    {
                        writer.WriteAttributeString("DeploymentType", "0");
                        if (!string.IsNullOrWhiteSpace(deploy.OutputDir.Path))
                        {
                            writer.WriteAttributeString("RemoteRoot", deploy.OutputDir.Path);
                        }
                        writer.WriteAttributeString("DeploymentFiles", deploy.InputDependencies.FileItems.ToString(";", item => item.Path.Path));
                    }
                }

                writer.WriteEndElement(); //Tool
            }

        }


        #endregion Write Tools

        protected override void WriteUserFile()
        {
            if(null != Modules.Cast<ProcessableModule>().FirstOrDefault(m=> m.AdditionalData.DebugData != null))
            {
                string userFilePath = Path.Combine(OutputDir.Path, UserFileName);
                var userFileDoc = new XmlDocument();

                if(File.Exists(userFilePath))
                {
                    userFileDoc.Load(userFilePath);
                }


                var userFileEl = userFileDoc.GetOrAddElement("VisualStudioUserFile");
                
                userFileEl.SetAttribute("ProjectType", "Visual C++");
                userFileEl.SetAttribute("Version", Version);
                userFileEl.SetAttributeIfMissing("ShowAllFiles", "false");

                var configurationsEl = userFileEl.GetOrAddElement("Configurations");

                foreach (var module in Modules)
                {
                    SetUserFileConfiguration(configurationsEl, module as ProcessableModule);
                }

                userFileDoc.Save(userFilePath);
            }
        }

        protected virtual void SetUserFileConfiguration(XmlNode configurationsEl, ProcessableModule module)
        {
            if (module != null && module.AdditionalData.DebugData != null)
            {
                var configEl = configurationsEl.GetOrAddElementWithAttributes("Configuration", "Name", GetVSProjConfigurationName(module.Configuration));

                foreach (var group in GetUserData(module))
                {
                    var groupEl = configEl.GetOrAddElement(group.Key);

                    foreach (var attrib in group.Value)
                    {
                        var value = attrib.Value;

                        if (BuildGenerator.IsPortable)
                        {
                            value = BuildGenerator.PortableData.NormalizeIfPathString(attrib.Value, OutputDir.Path);
                        }

                        groupEl.SetAttributeIfValueNonEmpty(attrib.Key, value);
                    }
                }
            }
        }

        #endregion Write Methods

        #endregion Virtual Protected Methods

        #region Protected Methods

        /// <summary>
        /// ProcessSwitches -	Takes a newline separated collection of command line switches; parses them according to
        ///						the parsing directives and then puts the translated result (XML attributes) into a hash
        ///						table.
        /// </summary>
        /// <param name="parseDirectives">An List of SwitchInfo's dictating the translation from command line to XML</param>
        /// <param name="nameToXMLValue">The hash table that contains values of the XML attributes (string->string map) that were obtained by the parsing</param>
        /// <param name="CMLString">The newline separated string of command line switches</param>
        /// <param name="taskNameClean">The name of the current task (used for errors)</param>
        /// <param name="errorIfUnrecognized"></param>
        /// <param name="nameToDefaultXMLValue"></param>
        /// <param name="options"></param>
        /// <param name="taskName"></param>
        protected void ProcessSwitches(List<VSConfig.SwitchInfo> parseDirectives, IDictionary<string, string> nameToXMLValue, IEnumerable<string> options, string taskName, bool errorIfUnrecognized = true, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            //Go through all the switches
            foreach (string option in options)
            {
                //Got through all the switchInfo's that we have and see if one matches
                bool matchFound = false; //marks if we've found a match for the current switch
                foreach (VSConfig.SwitchInfo si in parseDirectives)
                {
                    Regex curRegEx = si.Switch;							//Get the current reg exp of the switch
                    Match match = curRegEx.Match(option);	//see if it matches

                    //if it doesn't, go on
                    if (!match.Success)
                        continue;

                    //if the attribute name corresponding the the switch is "", we don't need to do anything else
                    if (si.XMLName.Length == 0)
                    {
                        matchFound = true;
                        break;
                    }


                    bool checkForSetOnce = false;							//Check if setOnce condition has been violated
                    if (nameToXMLValue.ContainsKey(si.XMLName) == false)
                    {
                        nameToXMLValue.Add(si.XMLName, string.Empty);  //The attribute is not set yet, add it
                    }
                    else
                    {
                        //the attribute is already set, see if this switch can only be set once
                        if (si.SetOnce)
                            checkForSetOnce = true;
                    }

                    //Prepare the type specs (split the string along ','; kill whitespace)
                    string[] typeSpecs = si.TypeSpec.Split(new char[] { ',' });
                    for (int cnt = 0; cnt < typeSpecs.Length; cnt++)
                    {
                        typeSpecs[cnt] = typeSpecs[cnt].TrimWhiteSpace();
                    }

                    string lastValue = nameToXMLValue[si.XMLName];							//record the previous setting
                    Array myArray = Array.CreateInstance(typeof(Object), match.Groups.Count + 1);	//Make an array for passing to String.Format

                    //{0} is the previous attribute setting
                    //{1} and up are the regexp matches corresponding to $1...$n
                    myArray.SetValue(lastValue, 0);
                    for (int cnt = 1; cnt < match.Groups.Count; cnt++)
                    {
                        //If we have a corresponding type specifier, parse it; otherwise use the string
                        if (cnt - 1 >= typeSpecs.Length)
                        {
                            myArray.SetValue(GetPortableOption(si.XMLName, match.Groups[cnt].ToString()), cnt);
                        }
                        else
                        {
                            switch (typeSpecs[cnt - 1])
                            {
                                case "": //The type is empty
                                    if (typeSpecs.Length > 1 || cnt - 1 != 0) //type is empty because type specifier string is malformed
                                    {
                                        //IMTODO
                                        //Error.SetFormat(Log, ErrorCodes.INTERNAL_ERROR, "{0}: Empty format specifier found.", taskName);
                                    }
                                    else //type is empty because the type specifier string is empty
                                        myArray.SetValue(GetPortableOption(si.XMLName, match.Groups[cnt].ToString()), cnt);
                                    break;

                                case "string": //it's a string
                                    myArray.SetValue(GetPortableOption(si.XMLName, match.Groups[cnt].ToString()), cnt);
                                    break;

                                case "int": //it's an int
                                    myArray.SetValue(int.Parse(match.Groups[cnt].ToString()), cnt);
                                    break;

                                case "hex": //it's a hex number
                                    string number = match.Groups[cnt].ToString();

                                    if (number.StartsWith("0x"))
                                        number = number.Substring(2, number.Length - 2);

                                    myArray.SetValue(int.Parse(number, NumberStyles.HexNumber), cnt);
                                    break;

                                case "int-hex": //it's a hex number if "0x", integer otherwise
                                    string ihnumber = match.Groups[cnt].ToString();

                                    if (ihnumber.StartsWith("0x"))
                                    {
                                        ihnumber = ihnumber.Substring(2, ihnumber.Length - 2);
                                        myArray.SetValue(int.Parse(ihnumber, NumberStyles.HexNumber), cnt);
                                    }
                                    else
                                    {
                                        myArray.SetValue(int.Parse(ihnumber), cnt);
                                    }
                                    break;

                                default:
                                    //IMTODO
                                    //Error.SetFormat(Log, ErrorCodes.INTERNAL_ERROR, "{0}: Unknown format type", typeSpecs[cnt - 1], taskName);
                                    break;
                            }
                        }
                    }

                    //Make the new value by evaluating the format string with the parameter array
                    String newValue = String.Format(si.XMLResultString, (Object[])myArray);

                    //If we're checking for set once and it's been violated, complain
                    if (checkForSetOnce && newValue != lastValue)
                    {
                        //Error.SetFormat(Log, ErrorCodes.INTERNAL_ERROR, "{3}: {0}: Compiler XML name {1} already has a conflicting setting {2}.", CMLSwitches[index], si.XMLName, lastValue, taskName);
                    }

                    nameToXMLValue[si.XMLName] = newValue; //set the new value
                    matchFound = true; //got our match

                    if (si.MatchOnce)
                        break;

                    //go on to the next parsing spec, because there may be more than 1 spec for 1 switch
                }

                //Unrecognized switch; complain
                if (errorIfUnrecognized && !matchFound)
                {
                    //Error.SetFormat(Log, ErrorCodes.INTERNAL_ERROR, "{0}: option {1} has not been recognized.", taskName, CMLSwitches[index]);
                }
            }

            if (nameToDefaultXMLValue != null)
            {
                //Apply default attributes
                foreach (KeyValuePair<string, string> item in nameToDefaultXMLValue)
                {
                    //If the attribute is not set yet, set it to the default
                    if (nameToXMLValue.ContainsKey(item.Key) == false)
                    {
                        nameToXMLValue.Add(item.Key, item.Value);
                    }
                }
            }

#if FRAMEWORK_PARALLEL_TRANSITION
            string transitionval;
            if (nameToXMLValue.TryGetValue("OutputFile", out transitionval))
            {
                nameToXMLValue["OutputFile"] = PathNormalizer.Normalize(transitionval, false);
            }
#endif

            foreach (var entry in _cleanupParameters)
            {
                string value;
                if (nameToXMLValue.TryGetValue(entry.Key, out value))
                {
                    if ((entry.Value & ParameterCleanupFlags.RemoveQuotes) == ParameterCleanupFlags.RemoveQuotes)
                    {
                        value = value.TrimQuotes();

                        if ((entry.Value & ParameterCleanupFlags.EnsureTrailingPathSeparator) == ParameterCleanupFlags.EnsureTrailingPathSeparator)
                        {
                            value = value.EnsureTrailingSlash();
                        }
                    }
                    else if ((entry.Value & ParameterCleanupFlags.AddQuotes) == ParameterCleanupFlags.AddQuotes)
                    {
                        if ((entry.Value & ParameterCleanupFlags.EnsureTrailingPathSeparator) == ParameterCleanupFlags.EnsureTrailingPathSeparator)
                        {
                            value = value.TrimQuotes();
                            value = value.EnsureTrailingSlash();
                        }
                        value = value.Quote();
                    }

                    nameToXMLValue[entry.Key] = value;
                }
            }

            if (BuildGenerator.IsPortable)
            {
                string additionalOptions;

                if (nameToXMLValue.TryGetValue("AdditionalOptions", out additionalOptions))
                {
                    nameToXMLValue["AdditionalOptions"] = BuildGenerator.PortableData.NormalizeOptionsWithPathStrings(additionalOptions, OutputDir.Path, "\\s*(\"(?:[^\"]*)\"|(?:[^\\s]+))\\s*");
                }
                

            }
        }

        private string GetPortableOption(string name, string value)
        {
            if (BuildGenerator.IsPortable && PARAMETERS_WITH_PATH.Contains(name))
            {
                value = GetProjectPath(value);
            }
            return value;
        }

        protected virtual string[,] GetTools(Configuration config)
        {
            if (config.System == "xenon")
                return VCTools360;

            return VCToolsWin;
        }

        protected string GetToolMapping(Configuration config, Tool tool)
        {
            string mapping = null;
            if (tool != null)
            {
                mapping = GetToolMapping(config, tool.ToolName);

                if (mapping == null)
                {
                    // Unmapped tools are executed as custom build tools:
                    var customtool = tool as BuildTool;
                    if (customtool != null)
                    {
                        mapping = "VCCustomBuildTool";
                    }
                }
            }
            return mapping;
        }

        protected string GetToolMapping(Configuration config, string toolName)
        {
            var map = GetTools(config);
            for (int i = 0; i < map.GetLength(0); i++)
            {
                if (toolName == map[i, 1])
                {
                    return map[i, 0];
                }
            }

            return null;
        }

        // If rule file has optionset attached to it, it is considered a template.
        // Apply Regex.Replace() and put resulting file into build directory:
        protected string CreateRuleFileFromTemplate(FileItem rule, IModule module)
        {
            string inputFileName = rule.FileName.Trim();
            string outputFileName = inputFileName;

            if (String.IsNullOrEmpty(rule.OptionSetName))
            {
                return outputFileName;
            }

            // The rule file is a template and we need to run Regex.Replace() using specified optionset as parameters.
            OptionSet templateOptions;

            if (module.Package.Project.NamedOptionSets.TryGetValue(rule.OptionSetName, out templateOptions))
            {
                // Read file, 
                if (File.Exists(inputFileName))
                {
                    string fullIntermediateDir = Path.Combine(OutputDir.Path, module.IntermediateDir.Path);
                    Directory.CreateDirectory(fullIntermediateDir);

                    outputFileName = Path.Combine(fullIntermediateDir, Path.GetFileName(inputFileName));
                    
                    var  templateFileContent = File.ReadAllText(inputFileName);
                    foreach (var option in templateOptions.Options)
                    {
                        templateFileContent = Regex.Replace(templateFileContent, option.Key, option.Value);
                    }

                    using (IMakeWriter writer = new MakeWriter(writeBOM:false))
                    {
                        writer.FileName =Path.Combine(fullIntermediateDir, Path.GetFileName(inputFileName));

                        writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnCustomRuleFileUpdate);

                        writer.Write(templateFileContent);
                    

                        //Handle vs2010 rules as well...
                        if (outputFileName.EndsWith(".props"))
                        {
                            //ensure requisite build rules files are also there.
                            string inputTargetsFile = Path.ChangeExtension(inputFileName, ".targets");
                            string inputSchemaFile = Path.ChangeExtension(inputFileName, ".xml");

                            var inputTargetsFI = new FileItem(PathString.MakeNormalized(inputTargetsFile), rule.OptionSetName);
                            var inputSchemaFI = new FileItem(PathString.MakeNormalized(inputSchemaFile), rule.OptionSetName);

                            CreateRuleFileFromTemplate(inputTargetsFI, module);
                            CreateRuleFileFromTemplate(inputSchemaFI, module);
                        }
                    }
                }
                else
                {
                    String msg = String.Format("Custom rules template file '{0}' does not exist", inputFileName);
                    throw new BuildException(msg);
                }
            }
            else
            {
                String msg = String.Format("Rules template file Optionset '{0}' with replacement patterns does not exist. Can not apply Regex.Replace()", rule.OptionSetName);
                throw new BuildException(msg);
            }
            return outputFileName;
        }

        protected string GetCreateDirectoriesCommand(IEnumerable<Command> commands)
        {
            var commandstring = new StringBuilder();
            if (commands != null)
            {
                        string format = "@if not exist \"{0}\" mkdir \"{0}\" & SET ERRORLEVEL=0";

                        switch (Environment.OSVersion.Platform)
                        {
                            case PlatformID.MacOSX:
                            case PlatformID.Unix:
                                format = "NOT IMPLEMENTED";
                                break;
                        }


                foreach(var dir in commands.Where(cmd=>cmd.CreateDirectories != null).SelectMany(cmd => cmd.CreateDirectories).Distinct())
                {

                            commandstring.AppendFormat(format, GetProjectPath(dir.Normalize(PathString.PathParam.NormalizeOnly)));
                            commandstring.AppendLine();
                }
            }
            return commandstring.ToString();

        }

        protected string GetCommandScriptWithCreateDirectories(Command command)
        {
            var commandstring = new StringBuilder();

            if (command.CreateDirectories != null && command.CreateDirectories.Count() > 0)
            {

                string format = "@if not exist \"{0}\" mkdir \"{0}\" & SET ERRORLEVEL=0";

                switch (Environment.OSVersion.Platform)
                {
                    case PlatformID.MacOSX:
                    case PlatformID.Unix:
                        format = "NOT IMPLEMENTED";
                        break;
                }

                foreach (var createdir in command.CreateDirectories)
                {
                    commandstring.AppendFormat(format, GetProjectPath(createdir.Normalize(PathString.PathParam.NormalizeOnly)));
                    commandstring.AppendLine();
                }
            }

            if (BuildGenerator.IsPortable)
            {
                commandstring.AppendLine(BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(command.CommandLine, OutputDir.Path));
            }
            else
            {
                commandstring.AppendLine(command.CommandLine);
            }
            return commandstring.ToString();
        }

        protected void OnCustomRuleFileUpdate(object sender, CachedWriterEventArgs e)
        {
            if (e != null)
            {
                Log.Status.WriteLine("{0}{1} } Custom rule file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
            }
        }


        #endregion

        #region Private Fields


        protected delegate void  WriteToolDelegate(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

        #endregion Private Fields

        #region Static Const Data

        #region tool to VCTools mapping 

        protected static readonly string[,] VCTools360 = new string[,]
        { 
             { "MASM"                                   ,"asm"  }
            ,{ "VCXMLDataGeneratorTool"                 ,"***"  }
            ,{ "VCWebServiceProxyGeneratorTool"         ,"***"  }
            ,{ "VCMIDLTool"                             ,"midl" }
            ,{ "VCCLX360CompilerTool"                   ,"cc"   }
            ,{ "VCLibrarianTool"                        ,"lib"  }
#if FRAMEWORK_PARALLEL_TRANSITION
            ,{ "VCManifestTool"                         ,"vcmanifest"  }
#endif
            ,{ "VCManagedResourceCompilerTool"          ,"resx" }
            ,{ "VCResourceCompilerTool"                 ,"rc"   }
            ,{ "VCX360LinkerTool"                       ,"link" }
            ,{ "VCALinkTool"                            ,"***"  }
#if FRAMEWORK_PARALLEL_TRANSITION
            ,{ "VCXDCMakeTool"                          ,"***"  }
#endif
            ,{ "VCX360ImageTool"                        ,"xenonimage" }
            ,{ "VCBscMakeTool"                          ,"***"  }
#if FRAMEWORK_PARALLEL_TRANSITION
            ,{ "VCFxCopTool"                            ,"***"  }
#endif
            ,{ "VCX360DeploymentTool"                   ,"deploy"  }
        };

        protected static readonly string[,] VCToolsWin = new string[,]
        { 
             { "MASM"                                   ,"asm"  }
            ,{ "VCXMLDataGeneratorTool"                 ,"***"  }
            ,{ "VCWebServiceProxyGeneratorTool"         ,"***"  }
            ,{ "VCMIDLTool"                             ,"midl"  }
            ,{ "VCCLCompilerTool"                       ,"cc"   }
            ,{ "VCLinkerTool"                           ,"link" }
            ,{ "VCLibrarianTool"                        ,"lib"  }
            ,{ "VCManifestTool"                         ,"vcmanifest"  }
            ,{ "VCManagedResourceCompilerTool"          ,"resx" }
            ,{ "VCResourceCompilerTool"                 ,"rc"   }
            ,{ "VCALinkTool"                            ,"***"  }
            ,{ "VCXDCMakeTool"                          ,"***"  }
            ,{ "VCBscMakeTool"                          ,"***"  }
            ,{ "VCFxCopTool"                            ,"***"  }
            ,{ "VCAppVerifierTool"                      ,"***"  }
            ,{ "VCWebDeploymentTool"                    ,"***"  }
        };

        protected readonly IDictionary<string,  WriteToolDelegate> _toolMethods = new Dictionary<string,  WriteToolDelegate>();

        #endregion VCTools to Configuration Tools Mapping

        protected enum ParameterCleanupFlags { RemoveQuotes = 1, AddQuotes = 2, EnsureTrailingPathSeparator = 4 }

        protected readonly IDictionary<string, ParameterCleanupFlags> _cleanupParameters = new Dictionary<string, ParameterCleanupFlags>()
        {
              {"OutputDirectory",               ParameterCleanupFlags.RemoveQuotes|ParameterCleanupFlags.EnsureTrailingPathSeparator}
             ,{"IntermediateDirectory",         ParameterCleanupFlags.RemoveQuotes|ParameterCleanupFlags.EnsureTrailingPathSeparator}
             ,{"OutputManifestFile",            ParameterCleanupFlags.RemoveQuotes}
             ,{"OutputFile",                    ParameterCleanupFlags.RemoveQuotes}
             ,{"MapFileName",                   ParameterCleanupFlags.RemoveQuotes}
             ,{"ModuleDefinitionFile",          ParameterCleanupFlags.RemoveQuotes}
             ,{"ImportLibrary",                 ParameterCleanupFlags.RemoveQuotes}
             ,{"ProgramDatabaseFile",           ParameterCleanupFlags.RemoveQuotes}
             ,{"ProgramDatabaseFileName",       ParameterCleanupFlags.RemoveQuotes}
        };

        private static readonly HashSet<string> SOURCE_EXTENSIONS = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { ".cpp", ".c", ".cxx" };
        private static readonly HashSet<string> HEADER_EXTENSIONS = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { ".h", ".hpp", ".hxx" };


        private static readonly HashSet<string> PARAMETERS_WITH_PATH = new HashSet<string> { 
            "OutputDirectory", 
            "IntermediateDirectory", 
            "AdditionalIncludeDirectories", 
            "AdditionalLibraryDirectories",
            "EmbedManagedResourceFile",
            "AdditionalDependencies",
            "OutputManifestFile",
            "OutputFile",
            "MapFileName",
            "ModuleDefinitionFile",
            "ImportLibrary",
            "ProgramDatabaseFile",
            "ProgramDataBaseFileName",
            "VCPostBuildEventTool",
            "OutputFileName"
        };

        #endregion Static Const Data


    }
}
