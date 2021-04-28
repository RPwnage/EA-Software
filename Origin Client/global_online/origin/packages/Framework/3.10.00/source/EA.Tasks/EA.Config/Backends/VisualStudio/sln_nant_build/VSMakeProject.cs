using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Globalization;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.PackageCore;
using NAnt.Core.Writers;
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
    internal abstract class VSMakeProject : VSProjectBase
    {
        protected VSMakeProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            :
            base(solution, modules, VCPROJ_GUID)
        {

            InitFunctionTables();
        }

        private void InitFunctionTables()
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
            //--- Xbox 360
            _toolMethods.Add("VCCLX360CompilerTool", WriteCompilerTool);
            _toolMethods.Add("VCX360LinkerTool", WriteLinkerTool);
            _toolMethods.Add("VCX360ImageTool", WriteX360ImageTool);
            _toolMethods.Add("VCX360DeploymentTool", WriteX360DeploymentTool);
        }



        #region Implementation of Inherited Abstract Methods

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

        protected override void WriteUserFile()
        {
        }

        #endregion Implementation of Inherited Abstract Methods

        #region Virtual Protected Methods

        #region Utility Methods

        protected virtual string PrimaryOutput(Module module)
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

            return Path.Combine(module.OutputDir.Path, module.Name);
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

            foreach (var e in map)
            {
                if (module.IsKindOf(e.Key))
                    return e.Value;
            }

            Log.Warning.WriteLine("Can't map module '{0}'-{1} to Visual Studio configuration type. Assume Utility project", module.Name, module.Configuration.Name);

            return "10"; // Utility by default
        }

        protected virtual string GetAllIncludeDirectories(CcCompiler compiler)
        {

            return compiler.IncludeDirs.ToString("; ", PathString.Quote);
        }

        protected virtual string GetAllUsingDirectories(CcCompiler compiler)
        {
            return compiler.UsingDirs.ToString("; ", PathString.Quote);
        }

        protected virtual IEnumerable<PathString> GetAllLibraries(Linker link)
        {
            return link.Libraries.FileItems.Select(item => item.Path);
        }

        protected virtual IEnumerable<PathString> GetAllLibraryDirectories(Linker link)
        {
            return link.LibraryDirs;
        }

        protected virtual void InitCompilerToolProperties(Configuration configuration, string includedirs, string usingdirs, string defines, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
        {
            nameToXMLValue = new Dictionary<string, string>();

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

            // SN_TARGET_ is required by VSI.NET to recognize the project as PSx and invoke its tools
            // instead of Microsoft ones. The defines were found by looking at a PSx project generated
            // with the VSI.NET wizard
            if (configuration.System == "ps3")
            {
                // There is no simple method of determining whether we're building PPU or SPU code
                // based on the current configuration, now that the PPU and SPU configurations
                // have been folded into a single config.  For the time being we're having to
                // assume PPU unless SN_TARGET_PS3* is already defined.  This means that all SPU
                // modules need to explicitly define SN_TARGET_PS3_SPU to work correctly with VSI,
                // which isn't ideal.  Maybe we should add some these defines to eaconfig so this
                // works out of the box?  Issue reported by Gilbert Wong.
                if (defines == null || defines.IndexOf("SN_TARGET_PS3") == -1)
                {
                    // Supposedly PPU:

                    defines += ";SN_TARGET_PS3";
                    if (configuration.Compiler == "sn")
                    {
                        defines += ";__SNC__";
                    }
                    else // assume gcc compiler
                    {
                        defines += ";__GCC__";
                    }

                }
                else  // SPU or GCC compiler
                {
                    defines += ";__GCC__";
                }
            }
            else if (configuration.System == "ps2")
            {
                defines += ";SN_TARGET_PS2";
            }

            //Turn the defines from '\n' separated to ; separated
            if (!String.IsNullOrEmpty(defines))
            {
                nameToXMLValue.Add("PreprocessorDefinitions", VSConfig.CleanString(defines, false, true, false));
            }

            //*****************************
            //* Set the defaults that are used if a particular attribute has no value after the parsing
            //*****************************
            /* IMTODO
            if (Version == "10.0")
            {
                nameToDefaultXMLValue.Add("ExceptionHandling", "FALSE");
                nameToDefaultXMLValue.Add("CompileAs", "Default");
                nameToDefaultXMLValue.Add("WarningLevel", "TurnOffAllWarnings");
                nameToDefaultXMLValue.Add("UsePrecompiledHeader", "0");
            }

            else
             */
            {
                nameToDefaultXMLValue.Add("DebugInformationFormat", "0");
                nameToDefaultXMLValue.Add("ExceptionHandling", "0");
                nameToDefaultXMLValue.Add("CompileAs", "0");
                nameToDefaultXMLValue.Add("WarningLevel", "0");
                nameToDefaultXMLValue.Add("UsePrecompiledHeader", "0");
            }

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

        protected virtual void InitLinkerToolProperties(Configuration configuration, IEnumerable<PathString> libraries, IEnumerable<PathString> libraryDirs, IEnumerable<PathString> objects, string defaultOutput, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
        {
            nameToXMLValue = new Dictionary<string, string>();

            //**********************************
            //* Set defaults that should exist before the switch parsing
            //* These are either something that a switch can add to (if switch setOnce==false)
            //* or something that ever can't be changed (if switch setOnce==true)
            //**********************************
            nameToXMLValue.Add("AdditionalDependencies", libraries.ToString(" ", PathString.Quote));

            nameToXMLValue.Add("AdditionalLibraryDirectories", libraryDirs.ToString("; ", PathString.Quote));

            if (nameToXMLValue.ContainsKey("AdditionalDependencies"))
            {
                string existingVal = (string)nameToXMLValue["AdditionalDependencies"];
                nameToXMLValue["AdditionalDependencies"] = existingVal + " " + objects.ToString(" ", PathString.Quote);
            }
            else
            {
                nameToXMLValue.AddNonEmpty("AdditionalDependencies", objects.ToString(" ", PathString.Quote));
            }
            //*****************************
            //* Set the defaults that are used if a particular attribute has no value after the parsing
            //*****************************

            nameToDefaultXMLValue.Add("GenerateDebugInformation", "FALSE");
            nameToDefaultXMLValue.Add("OutputFile", defaultOutput);

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
            nameToXMLValue = new Dictionary<string, string>();

            //**********************************
            //* Set defaults that should exist before the switch parsing
            //* These are either something that a switch can add to (if switch setOnce==false)
            //* or something that ever can't be changed (if switch setOnce==true)
            //**********************************
            nameToXMLValue.AddNonEmpty("AdditionalDependencies", objects.ToString(" ", PathString.Quote));

            nameToDefaultXMLValue.Add("OutputFile", defaultOutput);
        }

        #endregion Utility Methods

        #region Write Methods

        protected void WriteVisualStudioProject(IXmlWriter writer)
        {
            string Keywod = "Win32Proj";
            string Version = "9,00";
            string projectType = "Visual C++";


            writer.WriteStartElement("VisualStudioProject");
            writer.WriteAttributeString("Name", Name);
            writer.WriteAttributeString("Keywod", Keywod);
            writer.WriteAttributeString("ProjectGUID", ProjectGuidString);
            writer.WriteAttributeString("ProjectType", projectType);
            //writer.WriteAttributeString("RootNamespace", ????); //IMTODO 
            writer.WriteAttributeString("Version", Version);
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
            writer.WriteStartElement("ToolFiles");
            writer.WriteStartElement("DefaultToolFile");
            writer.WriteAttributeString("FileName", "masm.rules");
            writer.WriteFullEndElement(); //DefaultToolFile
            writer.WriteFullEndElement(); //ToolFiles
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
            writer.WriteAttributeString("OutputDirectory", module.OutputDir.Path);
            writer.WriteAttributeString("IntermediateDirectory", module.IntermediateDir.Path);
            writer.WriteAttributeString("CharacterSet", "2");//TODO
            writer.WriteAttributeString("ConfigurationType", GetVCConfigurationType(module));
            

            //Process following attributes:
            //      "ManagedExtensions"
            //      "UseOfMFC"
            //      "UseOfATL"
            //      "WholeProgramOptimization"

            IDictionary<string, string> nameToXMLValue = new Dictionary<string, string>();

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

            var map = GetTools(module.Configuration);
            for (int i = 0; i < map.GetLength(0); i++)
            {
                WriteOneTool(writer, map[i, 0], module.Tools.SingleOrDefault(t => t.ToolName == map[i, 1]), module);
            }


            WriteBuildEvents(writer, "VCPreBuildEventTool", BuildStep.PreBuild, module);
            WriteBuildEvents(writer, "VCPreLinkEventTool", BuildStep.PreLink, module);
            WriteBuildEvents(writer, "VCPostBuildEventTool", BuildStep.PostBuild, module);
            WriteBuildEvents(writer, "VCCustomBuildTool", BuildStep.Build, module);


            writer.WriteEndElement(); //Configuration
        }

        protected virtual void WriteOneTool(IXmlWriter writer, string vcToolName, Tool tool, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
             WriteToolDelegate writetoolmethod;

            if (_toolMethods.TryGetValue(vcToolName, out writetoolmethod))
            {
                writetoolmethod(writer, vcToolName, tool, module.Configuration, module, nameToDefaultXMLValue);
            }
            else
            {
                Log.Warning.WriteLine("Don't know how to write tool '{0}'", vcToolName);
            }

        }

        protected virtual void WriteReferences(IXmlWriter writer)
        {
            writer.WriteStartElement("References");
            writer.WriteFullEndElement(); //References
        }

        protected virtual void WriteFiles(IXmlWriter writer)
        {
            var files = GetAllFiles(tool =>
            {
                var filesets = new List<Tuple<FileSet, uint, Tool>>();
                if (tool is CcCompiler) filesets.Add(Tuple.Create((tool as CcCompiler).SourceFiles, FileEntry.Buildable, tool));
                if (tool is AsmCompiler) filesets.Add(Tuple.Create((tool as AsmCompiler).SourceFiles, FileEntry.Buildable, tool));
                if (tool is BuildTool) filesets.Add(Tuple.Create((tool as BuildTool).Files, FileEntry.Buildable, tool));
                return filesets;
            }).Select(e => e.Value).OrderBy(e => e.Path.Path);

            var customFilters = new Dictionary<string, string>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase) 
            { 
               // { BuildDir.Path, "generated" }, 
                { PackageMap.Instance.BuildRoot, "__Generated__" } 
            };

            writer.WriteStartElement("Files");

            var commonroots = new CommonRoots<FileEntry>(files, (fe) =>                    
                {
                    if (!fe.IsKindOf(fe.ConfigEntries.FirstOrDefault().Configuration, FileEntry.BulkBuild))
                    {
                        string dirPath = Path.GetDirectoryName(fe.Path.Path);
                        // Add one more directory level above files, unless files are just in drive root:

                        if (dirPath.IndexOf(Path.DirectorySeparatorChar) != dirPath.LastIndexOf(Path.DirectorySeparatorChar)
                            && !dirPath.Equals(Package.Dir.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
                        {
                            return dirPath;
                        }
                    }
                    return fe.Path.Path;
                },
                customFilters.Keys);

            var duplicatefilters = commonroots.FilterMap(files, customFilters);

            var dirStack = new Stack<string>();
            string prefix = null;
            string currentPathLevel = null;
            string customfilter = null;
            string customfilterConfig = null;

            foreach (var item in files)
            {
                string currentprefix = commonroots.GetPrefix(item.Path.Path);

                if (prefix == null || !currentprefix.Equals(prefix, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
                {
                    prefix = currentprefix;
                    currentPathLevel = commonroots.Roots[prefix];

                    // write out any remaining filter elements closing tags
                    while (dirStack.Count > 0)
                    {
                        dirStack.Pop();
                        writer.WriteEndElement();
                    }

                    if (!customFilters.TryGetValue(prefix, out customfilter))
                    {
                        customfilter = null;
                    }
                    else
                    {
                        // Push the currentPathLevel on the stack so that we can close all the filters off later
                        dirStack.Push(currentPathLevel);

                        writer.WriteStartElement("Filter");
                        writer.WriteAttributeString("Name", customfilter);
                        writer.WriteAttributeString("Filter", string.Empty);
                    }
                    customfilterConfig = null;
                }

                if (customfilter != null)
                {
                    // For config specific files add configuration filter:
                    if (item.ConfigEntries.Count == 1 && item.ConfigEntries[0].Configuration.Name != customfilterConfig)
                    {
                        if (customfilterConfig != null)
                        {
                            dirStack.Pop();
                            writer.WriteEndElement();
                        }
                        customfilterConfig = item.ConfigEntries[0].Configuration.Name;
                        writer.WriteStartElement("Filter");
                        writer.WriteAttributeString("Name", customfilterConfig);
                        writer.WriteAttributeString("Filter", string.Empty);
                        dirStack.Push(currentPathLevel);
                    }
                }
                else
                {
                    // this file is on the current drive
                    string baseDir = item.Path.Path.Substring(0, item.Path.Path.LastIndexOf(Path.DirectorySeparatorChar) + 1);

                    // while the path doesn't start with the currentPathLevel we need to pop directories off the stack.
                    //   as we have a common prefix we know that we will eventually have get back to the original currentPathLevel which was set to the common root.
                    while (String.Compare(baseDir, 0, currentPathLevel, 0, currentPathLevel.Length, !PathUtil.IsCaseSensitive) != 0)
                    {
                        // Move up the directory tree and pop queue entries, making sure filter tags are terminated
                        writer.WriteEndElement();
                        currentPathLevel = dirStack.Pop();
                    }

                    // Now check to see if we need to create filters to move down to the directory that we are looking for
                    //  ( we only compare up to the penultimate character because currentPathLevel contains a trailing \ )
                    while (String.Compare(baseDir, 0, currentPathLevel, 0, baseDir.Length, true) != 0)
                    {
                        // work out what the name of the next directory down should be
                        int endIdx = baseDir.IndexOf(Path.DirectorySeparatorChar, currentPathLevel.Length);
                        string dirName;
                        if (endIdx >= 0)
                        {
                            dirName = baseDir.Substring(currentPathLevel.Length, endIdx - currentPathLevel.Length);
                        }
                        else
                        {
                            dirName = baseDir.Substring(currentPathLevel.Length);
                        }

                        string filterprefix = String.Empty;
                        if (dirStack.Count == 0)
                        {
                            bool isDuplicate = false;
                            if (duplicatefilters.TryGetValue(dirName, out isDuplicate))
                            {
                                if (isDuplicate)
                                    filterprefix = currentprefix + ":\\";
                            }
                        }

                        // Push the currentPathLevel on the stack so that we can close all the filters off later
                        dirStack.Push(currentPathLevel);

                        writer.WriteStartElement("Filter");
                        writer.WriteAttributeString("Name", filterprefix + dirName);
                        writer.WriteAttributeString("Filter", string.Empty);
                        currentPathLevel = currentPathLevel + dirName + Path.DirectorySeparatorChar;
                    }
                }

                WriteFile(writer, item);
            }
            // write out any remaining filter elements closing tags
            while (dirStack.Count > 0)
            {
                writer.WriteEndElement();
                currentPathLevel = (string)dirStack.Pop();
            }

            writer.WriteFullEndElement(); //Files
        }

        protected virtual void WriteFile(IXmlWriter writer, FileEntry fileentry)
        {
            writer.WriteStartElement("File");
            writer.WriteAttributeString("RelativePath", PathUtil.RelativePath(fileentry.Path.Path, OutputDir.Path));

            var active = new HashSet<string>();
            foreach (var configentry in fileentry.ConfigEntries)
            {
                writer.WriteStartElement("FileConfiguration");
                writer.WriteAttributeString("Name", GetVSProjConfigurationName(configentry.Configuration));
                writer.WriteAttributeString("ExcludedFromBuild", configentry.IsKindOf(FileEntry.ExcludedFromBuild) ? "true" : "false");
                if (!configentry.IsKindOf(FileEntry.ExcludedFromBuild))
                {
                    Tool tool = configentry.FileItem.GetTool() ?? configentry.Tool;

                    FileData filedata = configentry.FileItem.Data as FileData;

                    if (tool != null)
                    {
                        bool processedtool = false;
                        var map = GetTools(configentry.Configuration);
                        for (int i = 0; i < map.GetLength(0); i++)
                        {
                            if (configentry.Tool.ToolName == map[i, 1])
                            {
                                if (configentry.FileItem.GetTool() != null)
                                {
                                    IDictionary<string, string> additionaloptions = null;
                                    if (filedata != null && filedata.ObjectFile != null)
                                    {
                                        additionaloptions = new Dictionary<string, string>() { { "ObjectFile", filedata.ObjectFile.Path.Quote() } };
                                    }

                                    WriteOneTool(writer, map[i, 0], tool, Modules.SingleOrDefault(m => m.Configuration == configentry.Configuration) as Module, additionaloptions);
                                }
                                else
                                {
                                    writer.WriteStartElement("Tool");
                                    writer.WriteAttributeString("Name", map[i, 0]);

                                    if (filedata != null && filedata.ObjectFile != null)
                                    {
                                        writer.WriteAttributeString("ObjectFile", filedata.ObjectFile.Path.Quote());
                                    }

                                    writer.WriteEndElement(); //Tool
                                }
                                processedtool = true;
                                break;
                            }
                        }

                        if (!processedtool)
                        {
                            var filetool = tool as BuildTool;
                            if (filetool != null)
                            {
                                writer.WriteStartElement("Tool");
                                writer.WriteAttributeString("Name", "VCCustomBuildTool");

                                var input = new StringBuilder();

                                if (filetool.IsKindOf(Tool.ShellScript))
                                {
                                    writer.WriteAttributeString("CommandLine", filetool.Executable.ToString().TrimWhiteSpace());
                                    input.AppendFormat("{0}; ", filetool.Executable.Path.Quote());
                                }
                                else
                                {
                                    writer.WriteAttributeString("CommandLine", filetool.Executable.Path.Quote() + " " + filetool.Options.ToString(" "));
                                }

                                if (!string.IsNullOrWhiteSpace(filetool.Description))
                                {
                                    writer.WriteAttributeString("Description", filetool.Description.TrimWhiteSpace());
                                }

                                input.Append(filetool.InputDependencies.FileItems.Select(fi => fi.Path.Path).ToString(";", file => file.Quote()).TrimWhiteSpace());

                                writer.WriteAttributeString("AdditionalDependencies", filetool.InputDependencies.FileItems.Select(fi => fi.Path.Path).ToString(";", file => file.Quote()).TrimWhiteSpace());
                                writer.WriteAttributeString("Outputs", filetool.OutputDependencies.FileItems.Select(fi => fi.Path.Path).ToString(";", file => file.Quote()).TrimWhiteSpace());

                                writer.WriteEndElement();
                            }
                        }
                    }
                }
                writer.WriteEndElement();

                active.Add(configentry.Configuration.Name);
            }
            // Write excluded from build:
            foreach (var config in AllConfigurations.Where(c => !active.Contains(c.Name)))
            {
                writer.WriteStartElement("FileConfiguration");
                writer.WriteAttributeString("Name", GetVSProjConfigurationName(config));
                {
                    bool isExcluded = !fileentry.IsKindOf(config, FileEntry.Headers | FileEntry.Resources) || fileentry.IsKindOf(config, FileEntry.ExcludedFromBuild);

                    writer.WriteAttributeString("ExcludedFromBuild", isExcluded.ToString());
                    if (!isExcluded)
                    {
                        writer.WriteStartElement("Tool");
                        writer.WriteAttributeString("Name", "VCCustomBuildTool");
                        writer.WriteEndElement();
                    }
                }
                writer.WriteEndElement();
            }

            writer.WriteEndElement();//File

        }

        protected virtual void WriteGlobals(IXmlWriter writer)
        {
            writer.WriteStartElement("Globals");
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

        protected virtual void WriteBuildEvents(IXmlWriter writer, string name, uint type, Module module)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            StringBuilder cmd = new StringBuilder();

            if ((type & BuildStep.Build) == BuildStep.Build)
            {
                // This is custom build step:
                StringBuilder input = new StringBuilder();
                StringBuilder output = new StringBuilder();

                foreach (var step in module.BuildSteps.Where(step => step.IsKindOf(type)))
                {
                    input.Append(step.InputDependencies.ToString(";", dep => dep.Path.Quote()));
                    output.Append(step.OutputDependencies.ToString(";", dep => dep.Path.Quote()));

                    foreach (var command in step.Commands)
                    {
                        cmd.AppendLine(command.CommandLine);
                    }
                }

                writer.WriteNonEmptyAttributeString("AdditionalDependencies", input.ToString().TrimWhiteSpace());
                writer.WriteNonEmptyAttributeString("Outputs", output.ToString().TrimWhiteSpace());
            }
            else
            {
                foreach (var command in module.BuildSteps.Where(step => step.IsKindOf(type)).SelectMany(step => step.Commands))
                {
                    cmd.AppendLine(command.CommandLine);
                }
            }

            if (cmd.Length > 0)
            {
                writer.WriteNonEmptyAttributeString("CommandLine", cmd.ToString().TrimWhiteSpace());
            }

            writer.WriteEndElement(); //Tool
        }

        protected virtual void WriteXMLDataGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteWebServiceProxyGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteMIDLTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            BuildTool midl = tool as BuildTool;
            if (midl != null)
            {
                var nameToXMLValue = new Dictionary<string, string>();
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new Dictionary<string, string>();

                ProcessSwitches(VSConfig.GetParseDirectives(module).Midl, nameToXMLValue, midl.Options, "midl", true, nameToDefaultXMLValue);

                foreach (var item in nameToXMLValue)
                {
                    writer.WriteAttributeString(item.Key, item.Value);
                }
            }

            writer.WriteEndElement(); //Tool
        }

        protected virtual void WriteAsmCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool
        }

        protected virtual void WriteCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            CcCompiler cc = tool as CcCompiler;

            if (cc != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                IDictionary<string, string> nameToXMLValue;
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new Dictionary<string, string>();

                InitCompilerToolProperties(config, GetAllIncludeDirectories(cc), GetAllUsingDirectories(cc), cc.Defines.ToString("; "), out nameToXMLValue, nameToDefaultXMLValue);

                ProcessSwitches(VSConfig.GetParseDirectives(module).Cc, nameToXMLValue, cc.Options, "cc", true, nameToDefaultXMLValue);

                //Apply default attributes
                foreach (var item in nameToXMLValue)
                {
                    writer.WriteAttributeString(item.Key, item.Value);
                }

                writer.WriteEndElement(); //Tool
            }
        }

        protected virtual void WriteLibrarianTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            Librarian lib = tool as Librarian;

            if (lib != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                IDictionary<string, string> nameToXMLValue;
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new Dictionary<string, string>();

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

        protected virtual void WriteManagedResourceCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteResourceCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);
            Module_Native native = module as Module_Native;
            if (native != null)
            {
                writer.WriteAttributeString("AdditionalIncludeDirectories", GetAllIncludeDirectories(native.Cc));
            }
            /*
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", "VCManagedResourceCompilerTool");
            string resourceName = intermediateDir + OutputName + @"." + Path.GetFileNameWithoutExtension(filePath) + @".resources";
            writer.WriteAttributeString("ResourceFileName", resourceName);
             */


            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteLinkerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {

            Linker linker = tool as Linker;

            if (linker != null)
            {
                writer.WriteStartElement("Tool");
                writer.WriteAttributeString("Name", name);

                IDictionary<string, string> nameToXMLValue;
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new Dictionary<string, string>();

                InitLinkerToolProperties(config, GetAllLibraries(linker), GetAllLibraryDirectories(linker), linker.ObjectFiles.FileItems.Select(item => item.Path), PrimaryOutput(module), out nameToXMLValue, nameToDefaultXMLValue);

                ProcessSwitches(VSConfig.GetParseDirectives(module).Link, nameToXMLValue, linker.Options, "link", true, nameToDefaultXMLValue);

                //Apply default attributes
                foreach (KeyValuePair<string, string> item in nameToXMLValue)
                {
                    writer.WriteAttributeString(item.Key, item.Value);
                }

                writer.WriteEndElement(); //Tool
            }

        }

        protected virtual void WriteALinkTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteManifestTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteXDCMakeTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteBscMakeTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteFxCopTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteAppVerifierTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteWebDeploymentTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteX360ImageTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
        {
            writer.WriteStartElement("Tool");
            writer.WriteAttributeString("Name", name);

            if (tool != null)
            {
                var nameToXMLValue = new Dictionary<string, string>();
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new Dictionary<string, string>();

                ProcessSwitches(VSConfig.GetParseDirectives(module).Ximg, nameToXMLValue, tool.Options, "Image Builder", true, nameToDefaultXMLValue);

                //Apply default attributes
                foreach (KeyValuePair<string, string> item in nameToXMLValue)
                {
                    writer.WriteAttributeString(item.Key, item.Value);
                }
            }

            writer.WriteEndElement(); //Tool

        }

        protected virtual void WriteX360DeploymentTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null)
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

        #endregion Write Tools

        #endregion Write Methods

        private string[,] GetTools(Configuration config)
        {
            if (config.System == "xenon")
                return VCTools360;

            return VCToolsWin;
        }

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
                            myArray.SetValue(match.Groups[cnt].ToString(), cnt);
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
                                        myArray.SetValue(match.Groups[cnt].ToString(), cnt);
                                    break;

                                case "string": //it's a string
                                    myArray.SetValue(match.Groups[cnt].ToString(), cnt);
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

        }


        #endregion

        #region Private Fields


        private delegate void  WriteToolDelegate(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null);


        #endregion Private Fields

        #region Static Const Data

        #region tool to VCTools mapping

        protected static readonly string[,] VCTools360 = new string[,]
        { 
             { "VCXMLDataGeneratorTool"                 ,"***"  }
            ,{ "VCWebServiceProxyGeneratorTool"         ,"***"  }
            ,{ "VCMIDLTool"                             ,"midl"  }
            ,{ "VCCLX360CompilerTool"                   ,"cc"   }
            ,{ "VCManagedResourceCompilerTool"          ,"resx" }
            ,{ "VCResourceCompilerTool"                 ,"rc"   }
            ,{ "VCLibrarianTool"                        ,"lib"  }
            ,{ "VCX360LinkerTool"                       ,"link" }
            ,{ "VCALinkTool"                            ,"***"  }
            ,{ "VCX360ImageTool"                        ,"xenonimage"}
            ,{ "VCBscMakeTool"                          ,"***"  }
            ,{ "VCX360DeploymentTool"                   ,"deploy"  }
        };

        protected static readonly string[,] VCToolsWin = new string[,]
        { 
             { "VCXMLDataGeneratorTool"                 ,"***"  }
            ,{ "VCWebServiceProxyGeneratorTool"         ,"***"  }
            ,{ "VCMIDLTool"                             ,"midl"  }
            ,{ "MASM"                                   ,"asm"  }
            ,{ "VCCLCompilerTool"                       ,"cc"   }
            ,{ "VCManagedResourceCompilerTool"          ,"resx" }
            ,{ "VCResourceCompilerTool"                 ,"rc"   }
            ,{ "VCLibrarianTool"                        ,"lib"  }
            ,{ "VCLinkerTool"                           ,"link" }
            ,{ "VCALinkTool"                            ,"***"  }
            ,{ "VCManifestTool"                         ,"***"  }
            ,{ "VCXDCMakeTool"                          ,"***"  }
            ,{ "VCBscMakeTool"                          ,"***"  }
            ,{ "VCFxCopTool"                            ,"***"  }
            ,{ "VCAppVerifierTool"                      ,"***"  }
            ,{ "VCWebDeploymentTool"                    ,"***"  }
            
        };

        private readonly IDictionary<string,  WriteToolDelegate> _toolMethods = new Dictionary<string,  WriteToolDelegate>();


        #endregion VCTools to Configuration Tools Mapping

        #endregion Static Const Data

    }
}
