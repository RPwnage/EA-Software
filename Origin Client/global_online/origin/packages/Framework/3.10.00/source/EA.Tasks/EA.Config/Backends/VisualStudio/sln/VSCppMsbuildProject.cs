using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Globalization;
using System.Security.Principal;

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
using EA.Eaconfig.Backends.Text;

namespace EA.Eaconfig.Backends.VisualStudio
{
    public class VCPPDirectories
    {
        public readonly List<PathString> ExecutableDirectories = new List<PathString>();
        public readonly List<PathString> IncludeDirectories = new List<PathString>();
        public readonly List<PathString> ReferenceDirectories = new List<PathString>();
        public readonly List<PathString> LibraryDirectories = new List<PathString>();
        public readonly List<PathString> SourceDirectories = new List<PathString>();
        public readonly List<PathString> ExcludeDirectories = new List<PathString>();
    }

    internal abstract class VSCppMsbuildProject : VSCppProject
    {
        protected VSCppMsbuildProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules)
        {
        }

        protected abstract string ToolsVersion { get; }

        protected abstract string DefaultToolset { get; }

        protected override string UserFileName
        {
            get
            {
                return ProjectFileName + ".user";
            }
        }

        protected override string Keyword
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

        protected override string TargetFrameworkVersion
        {
            get
            {
                if (StartupModule.IsKindOf(Module.Managed) && StartupModule is Module_Native)
                {
                    string targetframework = (StartupModule as Module_Native).TargetFrameworkVersion;

                    string dotNetVersion = String.IsNullOrEmpty(targetframework) ? DefaultTargetFrameworkVersion : targetframework;

                    if (dotNetVersion.StartsWith("v"))
                    {
                        return dotNetVersion;
                    }

                    // For backwards compativility with Framework 2 where "v" was not prepended or Framewrk version is derived from .Net package version.
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
                    else if (dotNetVersion.StartsWith("4.5"))
                    {
                        return "v4.5";
                    }
                    else if (dotNetVersion.StartsWith("5.0"))
                    {
                        return "v5.0";
                    }

                    return DefaultTargetFrameworkVersion.StartsWith("v") ? DefaultTargetFrameworkVersion : "v"+DefaultTargetFrameworkVersion;
                }
                return String.Empty;
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

        protected override void WriteProject(IXmlWriter writer)
        {
            SetCustomBuildRulesInfo();

            WriteVisualStudioProject(writer);
            WritePlatforms(writer);
            WriteProjectGlobals(writer);

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

            WriteFiles(writer);

            WriteExtensions(writer);

            WriteAppPackage(writer);

            ImportProject(writer, @"$(VCTargetsPath)\Microsoft.Cpp.targets");
            foreach (Module module in Modules)
            {
                if (module.Configuration.System == "winprt")
                {
                    var windowsPhoneTargets = @"$(MSBuildExtensionsPath)\Microsoft\WindowsPhone\v$(TargetPlatformVersion)\Microsoft.Cpp.WindowsPhone.$(TargetPlatformVersion).targets";
                    ImportProject(writer, windowsPhoneTargets, GetConfigCondition(module.Configuration));
                }
                else if (module.Configuration.System == "psp2")
                {
                    var psp2Targets = @"$(VCTargetsPath)\Platforms\$(Platform)\SCE.Makefile.$(Platform).targets";
                    var configCondition = GetConfigCondition(module.Configuration);
                    var condition = @"'$(ConfigurationType)' == 'Makefile' and Exists('$(VCTargetsPath)\Platforms\$(Platform)\SCE.Makefile.$(Platform).targets')";
                    if (!String.IsNullOrEmpty(configCondition))
                    {
                        condition += " and " + configCondition;
                    }
                    ImportProject(writer, psp2Targets, condition);
                }

            }
            WriteCustomBuildTargetsFiles(writer);

            WriteMsBuildTargetOverrides(writer);

            writer.WriteStartElement("ProjectExtensions");
            writer.WriteStartElement("VisualStudio");
            {
                WriteGlobals(writer);
            }
            writer.WriteEndElement();
            writer.WriteEndElement();
        }

        protected override void WriteVisualStudioProject(IXmlWriter writer)
        {
            writer.WriteStartElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");
            writer.WriteAttributeString("DefaultTargets", "Build");
            writer.WriteAttributeString("ToolsVersion", ToolsVersion);
        }

        protected override void WritePlatforms(IXmlWriter writer)
        {
            writer.WriteStartElement("ItemGroup");
            writer.WriteAttributeString("Label", "ProjectConfigurations");

            foreach (Module module in Modules)
            {
                var configName = GetVSProjTargetConfiguration(module.Configuration);
                var platform = GetProjectTargetPlatform(module.Configuration);
                writer.WriteStartElement("ProjectConfiguration");
                writer.WriteAttributeString("Include", configName + "|" + platform);
                writer.WriteElementString("Configuration", configName);
                writer.WriteElementString("Platform", platform);
                writer.WriteEndElement(); // ProjectConfiguration
            }
            writer.WriteEndElement(); // ItemGroup - ProjectConfigurations

        }

        protected virtual void WriteProjectGlobals(IXmlWriter writer)
        {
            var projectGlobals = new OrderedDictionary<string, string>();

            projectGlobals.Add("ProjectGuid", ProjectGuidString);
            projectGlobals.Add("Keyword", Keyword);
            projectGlobals.Add("ProjectName", Name);
            projectGlobals.Add("TargetFrameworkVersion", TargetFrameworkVersion);
            projectGlobals.Add("RootNamespace", RootNamespace);

            // Windows Phone 8 and Windows 8 Store apps unfortunately require special elements in the project globals.
            // This means it is not possible to have a valid project that contains both Windows Phone 8 and 
            // Windows 8 Store configurations (winrt and winprt configurations).
            foreach (var module in Modules)
            {
                if (module.Configuration.System == "winprt")
                {
                    // Windows Phone 8 - Do these have to be in Globals? Can they be conditional on config? 
                    projectGlobals.Add("MinimumVisualStudioVersion", "11.0");
                    projectGlobals.Add("XapOutputs", "true");
                    projectGlobals.Add("XapFilename", Name + ".xap"); // $(Name)_$(Configuration)_$(Platform).xap
                    projectGlobals.Add("TargetPlatformIdentifier", "Windows Phone");
                    projectGlobals.Add("TargetPlatformVersion", "8.0");
                    projectGlobals.Add("WinMDAssembly", "true");
                    // Only add these globals once regardless of how many winprt configurations there are
                    break;
                }
                else if (module.Configuration.System == "winrt" 
                    && (module.IsKindOf(Module.Program) || module.IsKindOf(Module.Library)))
                {
                    // Windows 8 - This seems to be required to be present in the Project Globals or the app won't
                    // be deployable and Visual Studio will try to run it as a regular Win32 executable.
                    projectGlobals.Add("AppContainerApplication", "true");
                    // Only add these globals once regardless of how many winrt configurations there are
                    break;
                }
            }

            // Invoke Visual Studio extensions
            foreach (var module in Modules)
            {
                var extension = GetExtensionTask(module as ProcessableModule);
                if (extension != null)
                {
                    extension.ProjectGlobals(projectGlobals);
                }
            }

            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Label", "Globals");
            foreach (var entry in projectGlobals)
            {
                writer.WriteNonEmptyElementString(entry.Key, entry.Value);
            }

            WriteSourceControlIntegration(writer);

            writer.WriteEndElement();
        }

        protected override void WriteConfigurations(IXmlWriter writer)
        {
            foreach (Module module in Modules)
            {
                StartPropertyGroup(writer, condition: GetConfigCondition(module.Configuration), label: "Configuration");
                writer.WriteElementString("ConfigurationType", GetConfigurationType(module));
                writer.WriteElementString("CharacterSet", CharacterSet(module).ToString());
                writer.WriteElementString("PlatformToolset", GetPlatformToolSet(module));


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

                if (IsWinRTProgram(module as Module_Native) || IsWinRTLibrary(module as Module_Native))
                {
                    nameToXMLValue["DefaultLanguage"] = "en-US";
                    nameToXMLValue["MinimumVisualStudioVersion"]= "11.0";
                    nameToXMLValue["AppContainerApplication"] = "true";
                }

                var extension = GetExtensionTask(module as ProcessableModule);
                if (extension != null)
                {
                    extension.ProjectConfiguration(nameToXMLValue);
                }

                WriteNamedElements(writer, nameToXMLValue);

                writer.WriteEndElement(); // End PropertyGroup - Configuration
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

        protected bool IsWinRTDeployTargetNeeded()
        {
            foreach (Module module in Modules)
            {
                if ((module.Configuration.System == "winrt" || module.Configuration.System == "winprt") && IsWinRTProgram(module as Module_Native))
                {
                    return true;
                }
            }
            return false;
        }

        protected virtual void WriteMsBuildTargetOverrides(IXmlWriter writer)
        {
            if (IsWinRTDeployTargetNeeded())
            {
                WriteWinrtDeployTarget(writer);
            }

            WriteCopyLocalNativeDependentTarget(writer);

            foreach (Module module in Modules)
            {
                var extension = GetExtensionTask(module as ProcessableModule);
                if (extension != null)
                {
                    extension.WriteMsBuildOverrides(writer);

                }
            }
        }

        protected virtual void WriteCopyLocalNativeDependentTarget(IXmlWriter writer)
        {
            int index = 1;
            foreach (Module_Native module in Modules.Where(m => m is Module_Native))
            {
                var filestocopy = new Dictionary<string, string>();

                foreach (var pair in module.Dependents.FlattenParentChildPair())
                {
                    bool copy = (module.CopyLocal == CopyLocalType.True || module.CopyLocal == CopyLocalType.Slim || pair.Dependency.Dependent.IsKindOf(DependencyTypes.CopyLocal));

                    // Copy Dependent build output:
                    if (copy && pair.Dependency.Dependent.IsKindOf(EA.FrameworkTasks.Model.Module.DynamicLibrary | EA.FrameworkTasks.Model.Module.Program))
                    {
                        var from = pair.Dependency.Dependent.PrimaryOutput();
                        if (!String.IsNullOrEmpty(from))
                        {
                            var to = Path.Combine(module.OutputDir.Path, Path.GetFileName(from));
                            filestocopy[from] = to;
                        }
                    }
                }

                if (filestocopy.Count > 0)
                {
                    writer.WriteStartElement("Target");
                    writer.WriteAttributeString("Name", "CopylocalNativeModules_"+index++);
                    writer.WriteAttributeString("BeforeTargets", "Build");
                    writer.WriteAttributeString("Condition", GetConfigCondition(module.Configuration));

                    foreach (var entry in filestocopy)
                    {
                        writer.WriteStartElement("Copy");
                        writer.WriteAttributeString("SourceFiles", GetProjectPath(entry.Key));
                        writer.WriteAttributeString("DestinationFiles", GetProjectPath(entry.Value));
                        writer.WriteAttributeString("SkipUnchangedFiles", "true");
                        writer.WriteAttributeString("UseHardlinksIfPossible", "true");
                        writer.WriteEndElement(); //Copy
                    }

                    writer.WriteEndElement();

                }
            }

        }

        protected virtual void WriteWinrtDeployTarget(IXmlWriter writer)
        {
            string kCustomDeployTargetName = "GatherEACustomDeployFiles";

            writer.WriteStartElement("Target");
            writer.WriteAttributeString("Name", kCustomDeployTargetName);
            if (DoModulesDependOnPostBuild(Modules))
            {
                // This changes relative ordering of the AfterLink targets run.
                // however, it's a common use case for ea packages to use the post-build
                // step to gather/generate a set of files to be included in the build.
                // this makes sure that our new targets runs the PostBuildEvent first.
                writer.WriteAttributeString("DependsOnTargets", "PostBuildEvent");
            }

            // clears the EACustomDeployFilesOutExpanded
            writer.WriteStartElement("ItemGroup");
            writer.WriteElementString("EACustomDeployFilesOutExpanded", "");
            writer.WriteEndElement(); // ItemGroup

            foreach (Module module in Modules)
            {
                WriteOneConfigurationWinrtDeploy(writer, module);
            }

            writer.WriteStartElement("ItemGroup");
            writer.WriteStartElement("PackagingOutputs");
            writer.WriteAttributeString("Include", "@(EACustomDeployFilesOutExpanded)");
            writer.WriteEndElement(); // PackagingOutputs
            writer.WriteEndElement(); // ItemGroup

            writer.WriteEndElement(); // Target

            foreach (Module module in Modules)
            {
                if (module.Configuration.System == "winrt" && IsWinRTProgram(module as Module_Native))
                {
                    string condition = GetConfigCondition(module.Configuration);
                    writer.WriteStartElement("PropertyGroup");
                    writer.WriteAttributeString("Condition", condition);
                    writer.WriteElementString("GetPackagingOutputsDependsOn", "$(GetPackagingOutputsDependsOn);" + kCustomDeployTargetName);
                    writer.WriteEndElement(); // PropertyGroup
                }
            }
        }

        protected virtual void WriteOneConfigurationWinrtDeploy(IXmlWriter writer, Module module)
        {
            // only winrt programs for winrt config-system need this
            if ((module.Configuration.System == "winrt" || module.Configuration.System == "winprt") && IsWinRTProgram(module as Module_Native))
            {
                string condition = GetConfigCondition(module.Configuration);

                string kOptionsSetName = module.GroupName + ".winrt.deployoptions";
                Project project = module.Package.Project;
                if (project.NamedOptionSets.ContainsKey(kOptionsSetName))
                {
                    OptionSet deployoptions = project.NamedOptionSets[kOptionsSetName];
                    string deploysetraw = deployoptions.GetOptionOrDefault("deploysets", "");
                    if (!string.IsNullOrWhiteSpace(deploysetraw))
                    {
                        string[] deploysets = deploysetraw.Split( new char[] {' ', ';', ':'}, StringSplitOptions.RemoveEmptyEntries);
                        foreach (string deploysetname in deploysets)
                        {
                            OptionSet deploysetoptions = project.NamedOptionSets.ContainsKey(deploysetname) ? project.NamedOptionSets[deploysetname] : null;
                            if (deploysetoptions != null)
                            {
                                string basepath = deploysetoptions.GetOptionOrDefault("basepath", "$(OutDir)");

                                if (!string.IsNullOrWhiteSpace(basepath))
                                {
                                    string includesraw = deploysetoptions.GetOptionOrDefault("include", "**");
                                    string excludesraw = deploysetoptions.GetOptionOrDefault("exclude", "AppX\\**;*.pdb;*.ilk;*.exp;*.lib;*.winmd;*.appxrecipe;*.pri");

                                    string[] includeslist = includesraw.Split(new char[] { ';' }, StringSplitOptions.RemoveEmptyEntries);
                                    string[] excludeslist = excludesraw.Split(new char[] { ';' }, StringSplitOptions.RemoveEmptyEntries);

                                    string includeattrib = "";
                                    foreach (string includeitem in includeslist)
                                    {
                                        includeattrib += string.Format("{0}\\{1};", basepath, includeitem);
                                    }

                                    string excludeattrib = "";
                                    foreach (string excludeitem in excludeslist)
                                    {
                                        excludeattrib += string.Format("{0}\\{1};", basepath, excludeitem);
                                    }

                                    // clears the EACustomDeployFiles
                                    writer.WriteStartElement("ItemGroup");
                                    writer.WriteAttributeString("Condition", condition);
                                    writer.WriteElementString("EACustomDeployFiles", "");
                                    writer.WriteEndElement(); // ItemGroup
                                    // clears the EACustomDeployFilesOut
                                    writer.WriteStartElement("ItemGroup");
                                    writer.WriteAttributeString("Condition", condition);
                                    writer.WriteElementString("EACustomDeployFilesOut", "");
                                    writer.WriteEndElement(); // ItemGroup

                                    writer.WriteStartElement("ItemGroup");
                                    writer.WriteAttributeString("Condition", condition);
                                    writer.WriteStartElement("EACustomDeployFiles");
                                    writer.WriteAttributeString("Include", includeattrib);
                                    writer.WriteAttributeString("Exclude", excludeattrib);
                                    writer.WriteEndElement(); // EACustomDeployFiles
                                    writer.WriteEndElement(); // ItemGroup

                                    writer.WriteStartElement("AssignTargetPath");
                                    writer.WriteAttributeString("Condition", condition);
                                    writer.WriteAttributeString("RootFolder", basepath);
                                    writer.WriteAttributeString("Files", "@(EACustomDeployFiles)");
                                    writer.WriteStartElement("Output");
                                    writer.WriteAttributeString("TaskParameter", "AssignedFiles");
                                    writer.WriteAttributeString("ItemName", "EACustomDeployFilesOut");
                                    writer.WriteEndElement(); // "Output"
                                    writer.WriteEndElement(); // AssignTargetPath

                                    writer.WriteStartElement("ItemGroup");
                                    writer.WriteAttributeString("Condition", condition);
                                    writer.WriteStartElement("EACustomDeployFilesOutExpanded");
                                    writer.WriteAttributeString("Include", "@(EACustomDeployFilesOut)");
                                    writer.WriteElementString("OutputGroup", "EACustomDeployFilesGroup");
                                    writer.WriteElementString("ProjectName", "$(ProjectName)");
                                    writer.WriteEndElement(); // EACustomDeployFilesOutExpanded
                                    writer.WriteEndElement(); // ItemGroup
                                }
                            }
                        }

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
                if (module.IsKindOf(Module.Program) && module.Configuration.System == "winprt")
                {
                    // Add reference to WMAppManifest.xml for Windows Phone
                    var wmAppManifest = WMAppManifest.Generate(Log, module as Module_Native, OutputDir, OutputDir, Name);
                    PathString wmAppManifestPath = wmAppManifest.ManifestFilePath;

                    StartElement(writer, "ItemGroup", condition: GetConfigCondition(module.Configuration));
                    foreach (var image in wmAppManifest.ImageFiles)
                    {
                        writer.WriteStartElement("Image");
                        writer.WriteAttributeString("Include", image.Path);
                        writer.WriteEndElement();
                    }
                    writer.WriteEndElement();

                    StartElement(writer, "ItemGroup", condition: GetConfigCondition(module.Configuration));

                    writer.WriteStartElement("Xml");
                    writer.WriteAttributeString("Include", Path.GetFileName(wmAppManifestPath.Path));                    
                    writer.WriteEndElement();

                    writer.WriteEndElement();
                }
                else
                {
                    WriteOneConfigurationAppPackage(writer, module);
                }
            }
        }

        protected virtual void WriteOneConfigurationAppPackage(IXmlWriter writer, Module module)
        {
            if (IsWinRTProgram(module as Module_Native))
            {                
                var appmanifest = AppXManifest.Generate(Log, module as Module_Native, OutputDir, Name, ProjectFileNameWithoutExtension);
                if (appmanifest != null)
                {
                    StartElement(writer, "ItemGroup", condition: GetConfigCondition(module.Configuration));
                    {
                        writer.WriteStartElement("AppxManifest");
                        writer.WriteAttributeString("Include", GetProjectPath(appmanifest.ManifestFilePath.Path));
                        writer.WriteElementString("SubType", "Designer");
                        writer.WriteEndElement();

                        foreach (var image in appmanifest.ImageFiles)
                        {
                            writer.WriteStartElement("Image");
                            writer.WriteAttributeString("Include", GetProjectPath(image.Path));
                            writer.WriteEndElement();
                        }
                    }
                    writer.WriteEndElement();
                }
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
            var extension = GetExtensionTask(module as ProcessableModule);
            if (extension != null)
            {
                extension.WriteExtensionItems(writer);
            }
        }


        protected virtual void WriteOneConfigurationBuildTools(IXmlWriter writer, Module module, ISet<PathString> referencedAssemblyDirs)
        {
            string configCondition = GetConfigCondition(module.Configuration);

            ImportGroup(writer, "PropertySheets", configCondition, @"$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props", @"exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')", "LocalAppDataPlatform");

            // --- Macros ----------------------------------------------------------------------------------
            StartPropertyGroup(writer, label: "UserMacros");
            writer.WriteEndElement();


            if (module.IsKindOf(Module.MakeStyle))
            {
                WriteMakeTool(writer, "VCNMakeTool", module.Configuration, module);
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
                }
                writer.WriteEndElement();
            }


            // --- Write 'General' property group ---
            string targetName;
            string targetExt;
            PrimaryOutput(module, out targetName, out targetExt);

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
                                Log.Warning.WriteLine("{0} linker/librarian options define output directory {1} that differs from module oputput directory setting {2}. Use [lib/link]outputname options or Framework properties to set output, do not specify it directly in linker or librarian options.", module.ModuleIdentityString(), outdir_fromOptions.Path.Quote(), outDir.Path.Quote());
                                outDir = outdir_fromOptions;
                            }
                            var targetName_fromOptions = Path.GetFileNameWithoutExtension(outputfile);
                            if (targetName != targetName_fromOptions)
                            {
                                Log.Warning.WriteLine("{0} linker/librarian options define output name {1} that differs from module tool output name setting {2}. Use [lib/link]outputname options or Framework properties to set output, do not specify it directly in linker or librarian options.", module.ModuleIdentityString(), targetName_fromOptions.Quote(), targetName.Quote());
                            }
                        }
                    }
                }
                //IM bb10 targets have error, require these settings:
                if ("qcc" == GetPlatformToolSet(module))
                {
                    if (module.Package.Project.Properties["eaconfig.visual-studio.platform-name"] == "BlackBerrySimulator")
                    {
                        writer.WriteElementString("OutDir", @"Simulator\");
                        writer.WriteElementString("IntDir", @"Simulator\");
                    }
                    else
                    {
                        writer.WriteElementString("OutDir", @"Device-$(Configuration)\");
                        writer.WriteElementString("IntDir", @"Device-$(Configuration)\");
                    }
                }
                else
                {
                    writer.WriteElementString("OutDir", GetProjectPath(outDir).EnsureTrailingSlash(defaultPath: "."));
                    writer.WriteElementString("IntDir", GetProjectPath(Path.Combine(module.IntermediateDir.Path, "vstmp")).EnsureTrailingSlash(defaultPath: "."));
                }
                writer.WriteNonEmptyElementString("TargetName", targetName);
                if (targetExt != null)
                {
                    writer.WriteNonEmptyElementString("TargetExt", targetExt);
                }

                if (link != null)
                {
                    if (nameToXMLValue == null)
                    {
                        nameToXMLValue = new SortedDictionary<string, string>();

                        ProcessSwitches(VSConfig.GetParseDirectives(module).Link, nameToXMLValue, link.Options, "general", false);
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

                if (module.Configuration.System == "xenon")
                {
                    string outputfile = PrimaryOutput(module);

                    writer.WriteElementString("OutputFile", GetProjectPath(outputfile));

                    if (link != null)
                    {
                        string value = null;
                        Tool imagebuilder = module.Tools.SingleOrDefault(t => t.ToolName == "xenonimage");
                        if (imagebuilder != null)
                        {
                            nameToXMLValue = new SortedDictionary<string, string>();

                            ProcessSwitches(VSConfig.GetParseDirectives(module).Ximg, nameToXMLValue, imagebuilder.Options, "Image Builder", true);

                            nameToXMLValue.TryGetValue("OutputFileName", out value);
                        }
                        if (string.IsNullOrEmpty(value))
                        {
                            value = Path.ChangeExtension(outputfile, ".xex");
                        }
                        writer.WriteElementString("ImageXexOutput", value);
                        // Start image path workaround
                        //There seem to be a bug in msbuild targets. Following properties supposed to be recomputed from 'ImageXexOutput' but they aren't
                        // writing them explicitly:
                        var imagePath = Path.Combine(OutputDir.Path, value);
                        writer.WriteElementString("ImagePath", GetProjectPath(imagePath));
                        writer.WriteElementString("ImageDir", GetProjectPath(Path.GetDirectoryName(imagePath)));
                        writer.WriteElementString("ImageExt", Path.GetExtension(imagePath));
                        writer.WriteElementString("ImageFileName", Path.GetFileName(imagePath));
                        writer.WriteElementString("ImageName", Path.GetFileNameWithoutExtension(imagePath));
                        // End image path workaround

                        var deploy = module.Tools.SingleOrDefault(t => t.ToolName == "deploy") as BuildTool;
                        if (deploy != null)
                        {
                            writer.WriteNonEmptyElementString("RemoteRoot", deploy.OutputDir.Path);
                        }
                    }
                }

                if (module.Configuration.System == "ps3")
                {
                    if (module.Configuration.SubSystem == ".spu")
                    {
                        writer.WriteNonEmptyElementString("SpuElfConversionUseInBuild", "false");
                    }
                    if (module.IsKindOf(Module.DynamicLibrary) && link != null)
                    {
                        if (link.ImportLibFullPath != null && !String.IsNullOrEmpty(link.ImportLibFullPath.Path))
                        {
                            writer.WriteNonEmptyElementString("PrxModuleName", Path.GetFileName(link.ImportLibFullPath.Path).Replace("_stub.a", String.Empty));
                        }
                    }
                }

                var extension = GetExtensionTask(module as ProcessableModule);
                if (extension != null)
                {
                    extension.WriteExtensionToolProperties(writer);
                }

                var vcppDirectories = GetVCPPDirectories(module);

                if (extension != null)
                {
                    extension.SetVCPPDirectories(vcppDirectories);
                }

                if (referencedAssemblyDirs != null)
                {
                    vcppDirectories.ReferenceDirectories.AddRange(referencedAssemblyDirs);
                }

                // Add default values
                if (!vcppDirectories.ExecutableDirectories.Any(p => p.Path == "$(ExecutablePath)" || p.Path.Contains("$(ExecutablePath);")))
                    vcppDirectories.ExecutableDirectories.Add(new PathString("$(ExecutablePath)"));
                if (!vcppDirectories.IncludeDirectories.Any(p => p.Path == "$(IncludePath)" || p.Path.Contains("$(IncludePath);")))
                    vcppDirectories.IncludeDirectories.Add(new PathString("$(IncludePath)"));
                if (!vcppDirectories.ReferenceDirectories.Any(p => p.Path == "$(ReferencePath)" || p.Path.Contains("$(ReferencePath);")))
                    vcppDirectories.ReferenceDirectories.Add(new PathString("$(ReferencePath)"));
                if (!vcppDirectories.LibraryDirectories.Any(p => p.Path == "$(LibraryPath)" || p.Path.Contains("$(LibraryPath);")))
                    vcppDirectories.LibraryDirectories.Add(new PathString("$(LibraryPath)"));
                if (!vcppDirectories.SourceDirectories.Any(p => p.Path == "$(SourcePath)" || p.Path.Contains("$(SourcePath);")))
                    vcppDirectories.SourceDirectories.Add(new PathString("$(SourcePath)"));
                if (!vcppDirectories.ExcludeDirectories.Any(p => p.Path == "$(ExcludePath)" || p.Path.Contains("$(ExcludePath);")))
                    vcppDirectories.ExcludeDirectories.Add(new PathString("$(ExcludePath)"));

                writer.WriteNonEmptyElementString("ExecutablePath", vcppDirectories.ExecutableDirectories.ToString(";", p => p.Path));
                writer.WriteNonEmptyElementString("IncludePath", vcppDirectories.IncludeDirectories.ToString(";", p => p.Path));
                writer.WriteNonEmptyElementString("ReferencePath", vcppDirectories.ReferenceDirectories.ToString(";", p => p.Path));
                writer.WriteNonEmptyElementString("LibraryPath", vcppDirectories.LibraryDirectories.ToString(";", p => p.Path));
                writer.WriteNonEmptyElementString("SourcePath", vcppDirectories.SourceDirectories.ToString(";", p => p.Path));
                writer.WriteNonEmptyElementString("ExcludePath", vcppDirectories.ExcludeDirectories.ToString(";", p => p.Path));
            }

            foreach (var step in module.BuildSteps)
            {
                if (step.Name == "custombuild")
                {
                    writer.WriteNonEmptyElementString("CustomBuildBeforeTargets", step.Before);
                    writer.WriteNonEmptyElementString("CustomBuildAfterTargets", step.After);
                    break;
                }
            }

            writer.WriteEndElement();
        }

        protected virtual IDictionary<string, IList<Tuple<ProjectRefEntry, uint>>> GroupProjectReferencesByConfigConditions(IEnumerable<ProjectRefEntry> projectReferences)
        {
            var groups = new Dictionary<string, IList<Tuple<ProjectRefEntry, uint>>>();

            foreach (var projRef in projectReferences)
            {
                foreach (var group in projRef.ConfigEntries.GroupBy(pr => pr.Type))
                {
                    if (group.Count() > 0)
                    {
                        var condition = GetConfigCondition(group.Select(ce => ce.Configuration));

                        IList<Tuple<ProjectRefEntry, uint>> configEntries;

                        if (!groups.TryGetValue(condition, out configEntries))
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

        protected virtual void WriteReferences(IXmlWriter writer, IEnumerable<ProjectRefEntry> projectReferences, IDictionary<PathString, FileEntry> references, IDictionary<PathString, FileEntry> comreferences, IDictionary<Configuration, ISet<PathString>> referencedAssemblyDirs)
        {
            if (projectReferences.Count() > 0)
            {
                var projrefGroups = GroupProjectReferencesByConfigConditions(projectReferences);

                foreach (var entry in projrefGroups)
                {
                    if (entry.Value.Count() > 0)
                    {
                        StartElement(writer, "ItemGroup", label: "ProjectReferences", condition: entry.Key);

                        foreach (var projRefWithType in entry.Value)
                        {
                            var projRefEntry = projRefWithType.Item1;
                            var projRefType = new BitMask(projRefWithType.Item2);

                            writer.WriteStartElement("ProjectReference");
                            {
                                writer.WriteAttributeString("Include", GetProjectPath(Path.Combine(projRefEntry.ProjectRef.OutputDir.Path, projRefEntry.ProjectRef.ProjectFileName)));
                                writer.WriteElementString("Project", projRefEntry.ProjectRef.ProjectGuidString);
                                writer.WriteElementString("Private", projRefType.IsKindOf(ProjectRefEntry.CopyLocal).ToString());
                                writer.WriteElementString("CopyLocalSatelliteAssemblies", projRefType.IsKindOf(ProjectRefEntry.CopyLocalSatelliteAssemblies).ToString());
                                writer.WriteElementString("LinkLibraryDependencies", projRefType.IsKindOf(ProjectRefEntry.LinkLibraryDependencies).ToString());
                                writer.WriteElementString("UseLibraryDependencyInputs", projRefType.IsKindOf(ProjectRefEntry.UseLibraryDependencyInputs).ToString());
                                writer.WriteElementString("ReferenceOutputAssembly", projRefType.IsKindOf(ProjectRefEntry.ReferenceOutputAssembly).ToString());
                            }
                            writer.WriteEndElement(); // ProjectReference
                        }
                        writer.WriteEndElement(); // ItemGroup (ProjectReferences)
                    }
                }
            }

            if (references.Count() > 0)
            {
                foreach (var group in references.Values.GroupBy(f => f.ConfigSignature, f => f))
                {
                    if (group.Count() > 0)
                    {
                        StartElement(writer, "ItemGroup", condition: GetConfigCondition(group));

                        foreach (var assemblyRef in group)
                        {
                            writer.WriteStartElement("Reference");
                            {
                                // If file is declared asis, keep it as a file name. No relative path.
                                // In VS2008 we can't put conditions by configuration. If any config declares file asIs, then we use it asis;
                                bool isFileAsIs = null != assemblyRef.ConfigEntries.Find(ce => ce.FileItem.AsIs);
                                var refpath = isFileAsIs ? assemblyRef.Path.Path : GetProjectPath(assemblyRef.Path.Path, addDot: true);

                                //In VS2008 we can't put conditions by configuration. If any config declares copy local, then we use copy local;
                                bool isCopyLocal = null != assemblyRef.ConfigEntries.Find(ce => ce.IsKindOf(FileEntry.CopyLocal));

                                bool isWinMdRef = ".winmd".Equals(Path.GetExtension(assemblyRef.Path.Path), StringComparison.OrdinalIgnoreCase);

                                var includepath = isWinMdRef ?
                                    refpath.Substring(0, refpath.Length - ".winmd".Length)
                                    :refpath;

                                writer.WriteAttributeString("Include", includepath);
                                if (!isFileAsIs)
                                {
                                    writer.WriteElementString("HintPath", refpath);
                                }
                                if (isWinMdRef)
                                {
                                    writer.WriteElementString("IsWinMDFile", isWinMdRef);
                                }
                                writer.WriteElementString("Private", isCopyLocal.ToString());
                                // Other possible settings:
                                // <ReferenceOutputAssembly>false</ReferenceOutputAssembly>
                                // <CopyLocalSatelliteAssemblies>false</CopyLocalSatelliteAssemblies>

                            }
                            writer.WriteEndElement();

                        }
                        writer.WriteFullEndElement(); //ItemGroup (ProjectReferences)
                    }
                }
            }

            WriteComReferences(writer, comreferences.Values);
        }

        protected override void WriteComReferences(IXmlWriter writer, ICollection<FileEntry> comreferences)
        {
            if (comreferences.Count > 0)
            {
                foreach (var group in comreferences.GroupBy(f => f.ConfigSignature, f => f))
                {
                    if (group.Count() > 0)
                    {
                        StartElement(writer, "ItemGroup", condition: GetConfigCondition(group));

                        foreach (var comRef in group)
                        {
                            Guid libGuid;
                            System.Runtime.InteropServices.ComTypes.TYPELIBATTR typeLibAttr;

                            if (GenerateInteropAssembly(comRef.Path, out libGuid, out typeLibAttr))
                            {
                                // If file is declared asis, keep it as a file name. No relative path.
                                // In VS2008 we can't put conditions by configuration. If any config declares file asIs, then we use it asis;
                                bool isFileAsIs = null != comRef.ConfigEntries.Find(ce => ce.FileItem.AsIs);
                                var refpath = isFileAsIs ? comRef.Path.Path : GetProjectPath(comRef.Path.Path, addDot: true);

                                //In VS2008 we can't put conditions by configuration. If any config declares copy local, then we use copy local;
                                bool isCopyLocal = null != comRef.ConfigEntries.Find(ce => ce.IsKindOf(FileEntry.CopyLocal));

                                writer.WriteStartElement("COMReference");
                                writer.WriteAttributeString("Include", refpath);
                                writer.WriteElementString("Guid", libGuid.ToString());
                                writer.WriteElementString("VersionMajor", typeLibAttr.wMajorVerNum.ToString());
                                writer.WriteElementString("VersionMinor", typeLibAttr.wMinorVerNum.ToString());
                                writer.WriteElementString("Lcid", typeLibAttr.lcid.ToString());
                                writer.WriteElementString("WrapperTool", "tlbimp");
                                writer.WriteElementString("Isolated", "false");
                                writer.WriteElementString("Private", isCopyLocal.ToString());
                                // Other possible settings:
                                // <ReferenceOutputAssembly>false</ReferenceOutputAssembly>
                                // <CopyLocalSatelliteAssemblies>false</CopyLocalSatelliteAssemblies>
                                writer.WriteEndElement(); //COMReference
                            }
                        }
                    }
                    writer.WriteEndElement(); //ItemGroup
                }
            }
        }

        protected override void WriteFilesWithFilters(IXmlWriter writer, FileFilters filters)
        {
            var toolFileFilter = new List<Tuple<string, PathString, string>>();

            writer.WriteStartElement("ItemGroup");

            filters.ForEach(
                (entry) =>
                {
                    foreach (var file in entry.Files)
                    {
                        if (Path.GetExtension(file.Path.Path).ToLowerInvariant() != ".resx")
                        {
                            string fileToolName;

                            WriteFile(writer, file, out fileToolName);

                            toolFileFilter.Add(Tuple.Create<string, PathString, string>(fileToolName, file.Path, entry.FilterPath));

                            if (fileToolName == "ClInclude")
                            {
                                PathString resXPath;
                                if (WriteResXFile(writer, file, out fileToolName, out resXPath))
                                {
                                    toolFileFilter.Add(Tuple.Create<string, PathString, string>(fileToolName, resXPath, entry.FilterPath));
                                }
                            }
                        }
                    }
                }
            );

            writer.WriteEndElement();

            WriteFilters(filters, toolFileFilter);
        }

        protected virtual void WriteFile(IXmlWriter writer, FileEntry fileentry, out string fileToolName)
        {
            // Tool name must be common for all configurations.
            var firstConfigWithTool = fileentry.ConfigEntries.FirstOrDefault(x => x.Tool != null || x.FileItem.GetTool() != null);
            fileToolName = GetToolMapping(firstConfigWithTool ?? fileentry.ConfigEntries.FirstOrDefault());

            if (fileToolName == "Masm" && fileentry.ConfigEntries.Any(e => e.Configuration.Compiler != "vc"))
            {
                fileToolName = "ClCompile";
            }

            writer.WriteStartElement(fileToolName);
            if (fileToolName == "CustomBuild" || fileToolName == "ClCompile")
            {
                // Because of bug in visual studio 2010 we need to force relative path, otherwise custom build does not work
                // and property pages for individual ClCompile files do not show up in GUI
                // See https://connect.microsoft.com/VisualStudio/feedback/details/748640 for details
                writer.WriteAttributeString("Include", PathUtil.RelativePath(fileentry.Path, OutputDir));
            }
            else
            {
                writer.WriteAttributeString("Include", GetProjectPath(fileentry.Path.Path));
            }

            var active = new HashSet<string>();
            foreach (var configentry in fileentry.ConfigEntries)
            {
                var tool = configentry.FileItem.GetTool();

                if (configentry.IsKindOf(FileEntry.ExcludedFromBuild))
                {
                    WriteElementStringWithConfigCondition(writer, "ExcludedFromBuild", "TRUE", GetConfigCondition(configentry.Configuration));

                    if (tool != null && tool.Description == "#global_context_settings")
                    {
                        tool = null;
                    }
                }
                var fileConfigItemToolName = GetToolMapping(configentry);
                if (fileConfigItemToolName != fileToolName)
                {
                    // Error. VC can only handle same tool names.
                }
                WriteOneTool(writer, fileToolName, tool, Modules.SingleOrDefault(m => m.Configuration == configentry.Configuration) as Module, null, configentry.FileItem);

                active.Add(configentry.Configuration.Name);
            }

            // Write excluded from build:
            foreach (var config in AllConfigurations.Where(c => !active.Contains(c.Name)))
            {
                WriteElementStringWithConfigCondition(writer, "ExcludedFromBuild", "TRUE", GetConfigCondition(config));
            }
            writer.WriteEndElement();//tool


        }

        // This method is for handling WinForm applications Form headers and their associated resX files
        // It's assumed that formx.h and its resX file be in the same directory.

        protected virtual bool WriteResXFile(IXmlWriter writer, FileEntry fileentry, out string fileToolName, out PathString filePath)
        {
            fileToolName = "EmbeddedResource";
            var resXfilepath = Path.ChangeExtension(fileentry.Path.Path, ".resX");
            if (File.Exists(resXfilepath))
            {
                filePath = new PathString(resXfilepath, PathString.PathState.File | fileentry.Path.State);
                writer.WriteStartElement(fileToolName);
                writer.WriteAttributeString("Include", GetProjectPath(resXfilepath));

                foreach (var configentry in fileentry.ConfigEntries)
                {
                    if (configentry.IsKindOf(FileEntry.ExcludedFromBuild))
                    {
                        WriteElementStringWithConfigCondition(writer, "ExcludedFromBuild", "TRUE", GetConfigCondition(configentry.Configuration));
                    }
                    else
                    {
                        WriteElementStringWithConfigCondition(writer, "DependentUpon", Path.GetFileName(fileentry.Path.Path), GetConfigCondition(configentry.Configuration));
                        WriteElementStringWithConfigCondition(writer, "LogicalName", Name + "." + Path.GetFileNameWithoutExtension(fileentry.Path.Path) + @".resources", GetConfigCondition(configentry.Configuration));
                    }
                }
                writer.WriteEndElement();
                return true;
            }
            filePath = null;
            return false;
        }

        protected virtual void WriteFilters(FileFilters filters, List<Tuple<string, PathString, string>> toolFileFilter)
        {
            using (IXmlWriter writer = new NAnt.Core.Writers.XmlWriter(FilterFileWriterFormat))
            {
                writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnProjectFileUpdate);

                writer.FileName = Path.Combine(OutputDir.Path, ProjectFileName + ".filters");
                // writer.WriteStartDocument();
                writer.WriteStartElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");
                writer.WriteAttributeString("ToolsVersion", ToolsVersion);

                //--- Write Filter definitions
                writer.WriteStartElement("ItemGroup");
                {
                    filters.ForEach(filter =>
                    {
                        if (!String.IsNullOrEmpty(filter.FilterPath))
                        {
                            writer.WriteStartElement("Filter");
                            writer.WriteAttributeString("Include", filter.FilterPath);
                            writer.WriteElementString("UniqueIdentifier", ((VSSolutionBase)BuildGenerator).ToString(Hash.MakeGUIDfromString(filter.FilterPath)));
                            writer.WriteEndElement();
                        }
                    });
                }
                writer.WriteEndElement();

                foreach (var toolgroup in toolFileFilter.GroupBy(e => e.Item1))
                {
                    //--- Write Filter files
                    writer.WriteStartElement("ItemGroup");
                    {
                        foreach (var item in toolgroup)
                        {
                            if (!String.IsNullOrEmpty(item.Item3))
                            {
                                writer.WriteStartElement(toolgroup.Key);
                                if (item.Item1 == "CustomBuild" || item.Item1 == "ClCompile")
                                {
                                    // because of a bug in visual studio 2010, path must be relative. See https://connect.microsoft.com/VisualStudio/feedback/details/748640 for details
                                    writer.WriteAttributeString("Include", PathUtil.RelativePath(item.Item2.Path, OutputDir.Path));
                                } else {
                                    writer.WriteAttributeString("Include", GetProjectPath(item.Item2.Path));
                                }
                                writer.WriteElementString("Filter", item.Item3);
                                writer.WriteEndElement();
                            }
                        }
                    }
                    writer.WriteEndElement();
                }

                writer.WriteEndElement(); // Project
            }
        }

        protected override void WriteUserFile()
        {
            if (null != Modules.Cast<ProcessableModule>().FirstOrDefault(m => m.AdditionalData != null && m.AdditionalData.DebugData != null))
            {
                string userFilePath = Path.Combine(OutputDir.Path, UserFileName);
                var userFileDoc = new XmlDocument();

                if (File.Exists(userFilePath))
                {
                    userFileDoc.Load(userFilePath);
                }

                var userFileEl = userFileDoc.GetOrAddElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");

                userFileEl.SetAttribute("ProjectType", "Visual C++");
                userFileEl.SetAttribute("ToolsVersion", ToolsVersion);

                foreach (var module in Modules)
                {
                    SetUserFileConfiguration(userFileEl, module as ProcessableModule);
                }

                userFileDoc.Save(userFilePath);
            }
        }

        protected override void SetUserFileConfiguration(XmlNode rootEl, ProcessableModule module)
        {
            if (module != null && module.AdditionalData.DebugData != null)
            {
                string condition = GetConfigCondition(module.Configuration);

                XmlNode propertyGroupEl = null;

                XmlNamespaceManager nsmgr = new XmlNamespaceManager(rootEl.OwnerDocument.NameTable);
                nsmgr.AddNamespace("msb", rootEl.NamespaceURI);

                foreach (var entry in GetUserDataMSBuild(module))
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
                        node.InnerText = BuildGenerator.PortableData.NormalizeIfPathString(entry.Value, OutputDir.Path);
                    }
                    else
                    {
                        node.InnerText = entry.Value;
                    }
                }
            }
        }

        protected virtual IEnumerable<KeyValuePair<string, string>> GetUserDataMSBuild(ProcessableModule module)
        {
            var userdata = new Dictionary<string, string>();

            if (module != null && module.AdditionalData.DebugData != null)
            {
                var prefixes = new string[] { "Local" };
                var remotemachine = module.AdditionalData.DebugData.RemoteMachine;
                string commandargs = module.AdditionalData.DebugData.Command.Options.ToString(" ");

                var workingDirectory = module.AdditionalData.DebugData.Command.WorkingDir.Path;


                var debuggerFlavor = "Windows"+ prefixes[0] + "Debugger";

                if(IsWinRTProgram(module as Module_Native))
                {
                    debuggerFlavor = "AppHost" + prefixes[0] + "Debugger";
                }

                if (module.Configuration.System == "xenon")
                {
                    prefixes = new string[] { "Xbox360" };
                    if (string.IsNullOrEmpty(remotemachine))
                    {
                        remotemachine = "$(DefaultConsole)";
                    }
                    if (String.IsNullOrEmpty(workingDirectory))
                    {
                        workingDirectory = "$(RemotePath)";
                    }
                    debuggerFlavor = prefixes[0] + "Debugger";
                }
                else if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.RemoteMachine))
                {
                    prefixes = new string[] { "Local", "Remote" };
                }

                if (module.Configuration.Platform.StartsWith("ps3"))
                {
                    userdata.AddNonEmpty("DebuggerFlavor", "PS3Debugger");

                    if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.Command.WorkingDir.Path))
                    {
                        string[] options = { "HomeDir", "FileServDir" };
                        var workingDirs = ParsePS3UserFileOption(module.AdditionalData.DebugData.Command.WorkingDir.Path, options);
                        foreach (string entry in workingDirs)
                        {
                            if (entry.StartsWith("HomeDir="))
                            {
                                userdata.AddNonEmpty("LocalDebuggerHomeDirectory", entry.Remove(0, 8));
                            }
                            else if (entry.StartsWith("FileServDir="))
                            {
                                userdata.AddNonEmpty("LocalDebuggerFileServingDirectory", entry.Remove(0, 12));
                            }
                        }
                    }

                    if (!String.IsNullOrEmpty(commandargs))
                    {
                        string[] options = { "DbgCmdLine", "DbgElfArgs", "RunCmdLine", "RunElfArgs" };
                        var cmdArgs = ParsePS3UserFileOption(commandargs, options);
                        foreach (string entry in cmdArgs)
                        {
                            if (entry.StartsWith("DbgCmdLine="))
                            {
                                userdata.AddNonEmpty("LocalDebuggerCommandLine", entry.Remove(0, 11));
                            }
                            else if (entry.StartsWith("DbgElfArgs="))
                            {
                                userdata.AddNonEmpty("LocalDebuggerCommandArguments", entry.Remove(0, 11));
                            }
                            else if (entry.StartsWith("RunCmdLine="))
                            {
                                userdata.AddNonEmpty("LocalRunCommandLine", entry.Remove(0, 11));
                            }
                        }
                    }

                    userdata.AddNonEmpty("LocalDebuggerCommand", module.AdditionalData.DebugData.Command.Executable.Path);
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

            var extension = GetExtensionTask(module as ProcessableModule);
            if (extension != null)
            {
                extension.UserData(userdata);
            }

            return userdata;
        }

        protected override IEnumerable<KeyValuePair<string, string>> Globals
        {
            get
            {
                var globals = new Dictionary<string, string>();
                foreach (var entry in base.Globals)
                {
                    globals.Add(entry.Key, entry.Value);
                }

                const string OPTIONS_VSTOMAKE = "options_vstomake.xml";

                foreach (var module in Modules)
                {
                    var platform = GetProjectTargetPlatform(module.Configuration);
                    var toolset = GetPlatformToolSet(module);

                    var name = String.Format("{0}-{1}-options_vstomake_map", platform, toolset);

                    if (!globals.ContainsKey(name))
                    {
                        string dataPath = String.Empty;

                        var platformConfigPackage = module.Package.Project.Properties["nant.platform-config-package-name"].TrimWhiteSpace();

                        if (!string.IsNullOrEmpty(platformConfigPackage))
                        {
                            dataPath = PathNormalizer.Normalize(Path.Combine(module.Package.Project.Properties["package." + platformConfigPackage + ".dir"], @"config\data", OPTIONS_VSTOMAKE));
                        }
                        else
                        {
                            dataPath = PathNormalizer.Normalize(Path.Combine(PackageMap.Instance.GetFrameworkRelease().Path, "data", OPTIONS_VSTOMAKE));
                        }

                        if (!String.IsNullOrEmpty(dataPath) && File.Exists(dataPath))
                        {
                            globals.Add(name, dataPath);
                        }
                    }
                }

                return globals;
            }
        }


        protected override void WriteGlobals(IXmlWriter writer)
        {
            writer.WriteStartElement("UserProperties");
            foreach (var global in Globals)
            {
                var globalVal = global.Value;

                if (BuildGenerator.IsPortable)
                {
                    bool isSdk = false;
                    globalVal = BuildGenerator.PortableData.FixPath(global.Value, out isSdk);
                }


                writer.WriteAttributeString(global.Key, globalVal);
            }


            writer.WriteFullEndElement(); //Globals
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
                if (module.Configuration.System == "xenon" && GetConfigurationType(module) == "Utility")
                {
                    Log.Warning.WriteLine("Custom build steps are not supported for Utility module '{0}' on XENON configurations." + Environment.NewLine +
                        "If the module is of 'Utility' type, it's assigned the 'Xenon Utility' type in its Visual Studio project." +
                        "However VS Xenon Utility type only has pre/post build steps and it does not support custom build steps.", module.BuildGroup + "." + module.Name);
                    return;
                }

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
                if ((type & BuildStep.PostBuild) == BuildStep.PostBuild)
                {
                     if (module.IsKindOf(Module.DynamicLibrary) && module.Configuration.System == "ps3" && module.Configuration.Compiler=="gcc")
                     {
                         var native = module as Module_Native;
                         if (native != null && native.PostLink != null)
                         {
                             cmd.AppendLine(GetCommandScriptWithCreateDirectories(native.PostLink).TrimWhiteSpace());
                         }
                     }
                }

                foreach (var step in module.BuildSteps.Where(step => step.IsKindOf(type) && step.Name == toolname))
                {
                    if (module.IsKindOf(Module.DynamicLibrary) && module.Configuration.System == "ps3")
                    {
                        if(step.Name.Contains("copy *_sub.a"))
                        {
                            continue;
                        }
                    }
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

                GetMakeToolCommands(module as Module, out build, out rebuild, out clean);

                writer.WriteElementString("NMakeBuildCommandLine", build);
                writer.WriteElementString("NMakeReBuildCommandLine", rebuild);
                writer.WriteElementString("NMakeCleanCommandLine", clean);

                writer.WriteEndElement();

            }



        }

        protected override void WriteXMLDataGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {

        }

        protected override void WriteWebServiceProxyGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {

        }

        protected override void WriteMIDLTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if (file == null)
                writer.WriteStartElement("Midl");

            BuildTool midl = tool as BuildTool;
            if (midl != null)
            {
                var nameToXMLValue = new SortedDictionary<string, string>();
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

                ProcessSwitches(VSConfig.GetParseDirectives(module).Midl, nameToXMLValue, midl.Options, "midl", true, nameToDefaultXMLValue);

                foreach (var item in nameToXMLValue)
                {
                    writer.WriteElementString(item.Key, item.Value);
                }
            }

            writer.WriteEndElement(); //Tool
        }

        protected override void WriteAsmCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {

            if (file != null)  // File references common tool settings:
            {
                var condition = GetConfigCondition(config);
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

        protected override void WriteCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            CcCompiler cc = tool as CcCompiler;

            var condition = file == null ? null : GetConfigCondition(config);

            if (cc != null)
            {
                if (file == null)
                {
                    writer.WriteStartElement(name);
                }

                IDictionary<string, string> nameToXMLValue;
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

                InitCompilerToolProperties(module, config, GetAllIncludeDirectories(cc), GetAllUsingDirectories(cc), GetAllDefines(cc, config).ToString(";"), out nameToXMLValue, nameToDefaultXMLValue);

                ProcessSwitches(VSConfig.GetParseDirectives(module).Cc, nameToXMLValue, cc.Options, "cc", true, nameToDefaultXMLValue);

                if (module.IsKindOf(Module.Managed))
                {
                    FilterForcedUsingFiles(nameToXMLValue, module, cc);
                }

                //Apply default attributes
                foreach (var item in nameToXMLValue)
                {
                    WriteElementStringWithConfigCondition(writer, item.Key, item.Value, condition);
                }

                if (file != null)
                {
                    FileData filedata = file.Data as FileData;
                    if (filedata != null && filedata.ObjectFile != null)
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
                }
                else
                {
                    writer.WriteEndElement(); //Tool
                }
            }
            else if (file != null)  // File references common tool settings:
            {
                FileData filedata = file.Data as FileData;
                if (filedata != null && filedata.ObjectFile != null)
                {
                    string objFile = filedata.ObjectFile.Path;

                    if (config.Compiler == "vc")
                    {
                        var native = module as Module_Native;
                        if (native != null && native.Cc != null && (!filedata.IsKindOf(FileData.ConflictObjName) && (native.Cc.Options.Any(op=> op.StartsWith("-MP") || op.StartsWith("/MP")))))
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

                InitLibrarianToolProperties(config, GetLibrarianObjFiles(module, lib), PrimaryOutput(module), out nameToXMLValue, nameToDefaultXMLValue);

                ProcessSwitches(VSConfig.GetParseDirectives(module).Lib, nameToXMLValue, lib.Options, "lib", true, nameToDefaultXMLValue);

                _ModuleLinkerNameToXMLValue[module.Key] = nameToXMLValue; // Store to use in project level properties;


                //IM bb10 targets have error, requires empty setting for output file
                if ("qcc" == GetPlatformToolSet(module) && nameToXMLValue.ContainsKey("OutputFile"))
                {
                    nameToXMLValue.Remove("OutputFile");
                }

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
                    Module_Native native = module as Module_Native;
                    if (native != null)
                    {
                        if (file == null)
                        {
                            var includedirs = tool.Options.Where(o => o.StartsWith("/i ") || o.StartsWith("-i ")).Select(o => o.Substring(3).TrimWhiteSpace());
                            // Add include directories only in generic tool.
                            writer.WriteElementString("AdditionalIncludeDirectories", includedirs.ToString(";", s => GetProjectPath(s).Quote()));
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
            Linker linker = tool as Linker;

            if (linker != null)
            {
                writer.WriteStartElement("Link");

                IDictionary<string, string> nameToXMLValue;
                nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();


                InitLinkerToolProperties(config, linker, GetAllLibraries(linker, module), GetAllLibraryDirectories(linker), GetLinkerObjFiles(module,linker), PrimaryOutput(module), out nameToXMLValue, nameToDefaultXMLValue);

                ProcessSwitches(VSConfig.GetParseDirectives(module).Link, nameToXMLValue, linker.Options, "link", true, nameToDefaultXMLValue);

                _ModuleLinkerNameToXMLValue[module.Key] = nameToXMLValue; // Store to use in project level properties;

                //Because of how VC handles incremental link with manifest we need to change replace manifest file name
                if (nameToXMLValue.ContainsKey("GenerateManifest") && ConvertUtil.ToBoolean(nameToXMLValue["GenerateManifest"]))
                {
                    if (nameToXMLValue.ContainsKey("ManifestFile"))
                    {
                        nameToXMLValue["ManifestFile"] = GetProjectPath(Path.Combine(module.IntermediateDir.Path, Path.GetFileName(PrimaryOutput(module)) + @".intermediate.manifest"));
                    }
                }
                else
                {
                    nameToXMLValue.Remove("ManifestFile");
                }

                if (nameToXMLValue.ContainsKey("LinkIncremental") && ConvertUtil.ToBoolean(nameToXMLValue["LinkIncremental"]))
                {
                    // Bug in VisualStudio Map File generation does not work with incremental link. Disable MapFile generation
                    if (nameToXMLValue.ContainsKey("GenerateMapFile") && ConvertUtil.ToBoolean(nameToXMLValue["GenerateMapFile"]))
                    {
                        Log.Warning.WriteLine("Visual Studio project '{0}': Because of a bug in Visual Studio map file generation does not work (on clean build) when incremental linking is enabled. GenerateMapFile element reset to 'FALSE'.", Name);
                        nameToXMLValue["GenerateMapFile"] = "false";
                    }
                }

                //IM bb10 targets have error, requires empty setting for output file
                if ("qcc" == GetPlatformToolSet(module) && nameToXMLValue.ContainsKey("OutputFile"))
                {
                    nameToXMLValue.Remove("OutputFile");
                }

                foreach (KeyValuePair<string, string> item in nameToXMLValue)
                {
                    writer.WriteElementString(item.Key, item.Value);
                }


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

                writer.WriteStartElement(name);
                {
                    if (manifest.Files.FileItems.Count > 0)
                    {
                        writer.WriteElementString("AdditionalManifestFiles", manifest.Files.FileItems.ToString(";", f => f.Path.Path).TrimWhiteSpace());
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
                writer.WriteEndElement(); //Tool

                writer.WriteStartElement("ManifestResourceCompile");
                {

                    if (manifest.InputDependencies.FileItems.Count > 0)
                    {
                        writer.WriteElementString("InputResourceManifests", manifest.InputDependencies.FileItems.ToString(";", f => f.Path.Path).TrimWhiteSpace());
                    }
                    writer.WriteNonEmptyElementString("ManifestResourceFile", manifestResourceFile);
                }
                writer.WriteEndElement();
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
                    WriteElementStringWithConfigCondition(writer, "Command", command, condition);
                    WriteElementStringWithConfigCondition(writer, "Message", filetool.Description.TrimWhiteSpace(), condition);
                    WriteElementStringWithConfigCondition(writer, "AdditionalInputs", filetool.InputDependencies.FileItems.Select(fi => fi.Path.Path).OrderedDistinct().ToString(";", path => path).TrimWhiteSpace(), condition);
                    WriteElementStringWithConfigCondition(writer, "Outputs", filetool.OutputDependencies.FileItems.Select(fi => fi.Path.Path).ToString(";", path => path).TrimWhiteSpace(), condition);
                    WriteElementStringWithConfigCondition(writer, "TreatOutputAsContent", filetool.TreatOutputAsContent.ToString(), condition);
                    if (filetool.IsKindOf(BuildTool.ExcludedFromBuild))
                    {
                        WriteElementStringWithConfigCondition(writer, "ExcludedFromBuild", "TRUE", condition);
                    }
                }
            }
        }

        protected override void WriteX360ImageTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if (module.Configuration.System == "xenon")
            {
                if (module.IsKindOf(Module.Utility))
                {
                    return;
                }

                if (module.IsKindOf(Module.Program | Module.DynamicLibrary))
                {
                    writer.WriteStartElement("ImageXex");

                    if (tool != null)
                    {
                        var nameToXMLValue = new SortedDictionary<string, string>();
                        nameToDefaultXMLValue = nameToDefaultXMLValue ?? new SortedDictionary<string, string>();

                        ProcessSwitches(VSConfig.GetParseDirectives(module).Ximg, nameToXMLValue, tool.Options, "Image Builder", true, nameToDefaultXMLValue);

                        //Apply default attributes
                        foreach (KeyValuePair<string, string> item in nameToXMLValue)
                        {
                            writer.WriteElementString(item.Key, item.Value);
                        }
                    }

                    writer.WriteEndElement(); //Tool
                }
            }

        }

        protected override void WriteX360DeploymentTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            if (module.Configuration.System == "xenon")
            {
                if (module.IsKindOf(Module.Utility))
                {
                    return;
                }

                if (module.IsKindOf(Module.Program | Module.DynamicLibrary))
                {
                    writer.WriteStartElement("Deploy");

                    BuildTool deploy = tool as BuildTool;

                    if (deploy == null)
                    {
                        writer.WriteElementString("DeploymentType", "CopyToHardDrive");
                        writer.WriteElementString("DeploymentFiles", "");
                        writer.WriteElementString("ExcludedFromBuild", true);
                    }
                    else
                    {
                        if (deploy.Options.Contains("-dvd"))
                        {
                            if (deploy.OutputDir.Path.EndsWith(".xgd"))
                            {
                                writer.WriteElementString("DeploymentFiles", "");
                                writer.WriteElementString("LayoutFile", deploy.OutputDir.Path);
                            }
                            else
                            {
                                writer.WriteElementString("DeploymentFiles", deploy.OutputDir.Path);
                            }
                        }
                        else
                        {
                            writer.WriteElementString("DeploymentType", "CopyToHardDrive");
                            writer.WriteNonEmptyElementString("RemoteRoot", deploy.OutputDir.Path);

                            var depfiles = deploy.InputDependencies.FileItems.ToString(";", item => item.Path.Path);

                            if (String.IsNullOrEmpty(depfiles))
                            {
                                var depcontentfiles = module.Package.Project.GetFileSet(module.GroupName + ".deploy-contents-fileset");
                                if (depcontentfiles != null)
                                {
                                    depfiles = depcontentfiles.FileItems.ToString(";", item => "$(RemoteRoot)="+item.Path.Path.Quote());
                                }
                            }
                            if (String.IsNullOrEmpty(depfiles))
                            {
                                depfiles = "$(RemoteRoot)=$(ImagePath)";
                            }

                            writer.WriteElementString("DeploymentFiles", depfiles);

                            if (deploy.IsKindOf(BuildTool.ExcludedFromBuild))
                            {
                                writer.WriteElementString("ExcludedFromBuild", true);
                            }
                        }
                    }

                    writer.WriteEndElement(); //Tool
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
            // Remove some 2008 settings:
            nameToDefaultXMLValue.Remove("DebugInformationFormat");
            nameToDefaultXMLValue.Remove("UsePrecompiledHeader");

            nameToDefaultXMLValue["ExceptionHandling"] = "FALSE";
            nameToDefaultXMLValue["CompileAs"] = "Default";
            nameToDefaultXMLValue["WarningLevel"] = "TurnOffAllWarnings";
            nameToDefaultXMLValue["PrecompiledHeader"] = "NotUsing";
            nameToDefaultXMLValue["CompileAsWinRT"] = "false";

            var useMpThreads = module.Package.Project.Properties["eaconfig.build-MP"];
            if (configuration.Compiler != "vc")
            {
                if (useMpThreads != null && (!useMpThreads.IsOptionBoolean() || (useMpThreads.IsOptionBoolean() && useMpThreads.OptionToBoolean())))
                {
                    nameToXMLValue["MultiProcessorCompilation"] = "TRUE";
                }
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
            if (linker.Executable != null && linker.Executable.Path != null && linker.Executable.Path.Contains("qcc.exe"))
            {
                // Looks like additional dependencies can only be system libs, or libs in the library dirs.
                nameToXMLValue.AddNonEmpty("AdditionalDependencies", libraries.Where(p=>!(p.Path.Contains('\\') || p.Path.Contains('\\'))).ToString(";", p => p.Path.TrimQuotes()));
                nameToXMLValue.AddNonEmpty("AdditionalLibraryDirectories", libraryDirs.ToString(";", p => GetProjectPath(p).TrimQuotes()));
                nameToXMLValue.AddNonEmpty("AdditionalOptions", libraries.Where(p => (p.Path.Contains('\\') || p.Path.Contains('\\'))).ToString(" ", p => GetProjectPath(p).Quote()) + " " + objects.ToString(" ", p => GetProjectPath(p).Quote()));
            }
            else
            {
                nameToXMLValue.AddNonEmpty("AdditionalDependencies", libraries.ToString(";", p => GetProjectPath(p).Quote()) + ";" + objects.ToString(";", p => GetProjectPath(p).Quote()));
                nameToXMLValue.AddNonEmpty("AdditionalLibraryDirectories", libraryDirs.ToString(";", p => GetProjectPath(p).Quote()));
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
                nameToDefaultXMLValue["LinkIncremental"] = "FALSE";
                nameToDefaultXMLValue.Add("GenerateManifest", "FALSE");
                // Yes!  This attribute naming is inconsistent with that used by the CompilerTool
                // which uses ProgramDataBaseFileName.  This inconsistency is correct.
                nameToDefaultXMLValue.Add("ProgramDatabaseFile", "$(OutDir)\\$(TargetName).pdb");
            }

            if (linker.ImportLibFullPath != null && !String.IsNullOrEmpty(linker.ImportLibFullPath.Path))
            {
                nameToDefaultXMLValue.Add("ImportLibrary", GetProjectPath(linker.ImportLibFullPath));
            }
        }

        protected override void InitLibrarianToolProperties(Configuration configuration, IEnumerable<PathString> objects, string defaultOutput, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
        {
            nameToXMLValue = new SortedDictionary<string, string>();
            nameToXMLValue.AddNonEmpty("AdditionalDependencies", objects.ToString(";", p => GetProjectPath(p).Quote()));
            nameToDefaultXMLValue.Add("OutputFile", defaultOutput);
        }


        protected virtual string GetConfigurationType(IModule module)
        {
            var typemask = new uint[] { Module.Library, Module.DynamicLibrary, Module.Program, Module.Utility, Module.MakeStyle };
            var typename = new string[] { "StaticLibrary", "DynamicLibrary", "Application", "Utility", "Makefile" };

            if (module.Configuration.System == "winrt" && module.Package.Project.Properties["config-processor"] == "arm")
            {
                typename[3] = "Makefile";
            }

            for (int i = 0; i < typemask.Length; i++)
            {
                if (module.IsKindOf(typemask[i]))
                {
                    return typename[i];
                }
            }
            return "Unknown";
        }

        protected virtual string GetPlatformToolSet(IModule module)
        {
            var properties = module.Package.Project.Properties;

            var platformToolset = properties["visualstudio.platform-toolset" + module.Configuration.SubSystem] ?? 
                                  properties["visualstudio.platform-toolset"] ?? 
                                  properties["package.VisualStudio.platformtoolset"] ?? 
                                  DefaultToolset;

            return platformToolset;
        }

        protected VCPPDirectories GetVCPPDirectories(Module module)
        {
            var dirs = new VCPPDirectories();

            if (module.Configuration.System == "ps3")
            {
                string ps3_sdk_path = "$(SCE_PS3_ROOT)";
                if (!BuildGenerator.IsPortable)
                {
                    var sdk_path_property = module.Package.Project.Properties["package.ps3sdk.appdir"];
                    ps3_sdk_path = sdk_path_property == null ? "$(SCE_PS3_ROOT)" : PathNormalizer.Normalize(module.Package.Project.Properties["package.ps3sdk.appdir"], false);
                }

                //NOTE. Do not quote paths. Plugin does not understand quoted paths.
                dirs.ExecutableDirectories.Add(new PathString(@"$(ExecutablePath);$(DevEnvDir)..\..\VC\bin", PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString(@"$(VCTargetsPath)\Platforms\PS3", PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString(String.Format(@"{0}\host-win32\sn\bin", ps3_sdk_path), PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString(String.Format(@"{0}\host-win32\ppu\bin", ps3_sdk_path), PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString(String.Format(@"{0}\host-win32\spu\bin", ps3_sdk_path), PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString(String.Format(@"{0}\host-win32\bin", ps3_sdk_path), PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString(String.Format(@"{0}\host-win32\Cg\bin", ps3_sdk_path), PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString(String.Format(@"{0}\bin", ps3_sdk_path), PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString(@"$(SN_COMMON_PATH)\bin", PathString.PathState.Directory));
                dirs.ExecutableDirectories.Add(new PathString("$(PATH)", PathString.PathState.Directory));

                if (module.Configuration.SubSystem == ".spu")
                {
                    dirs.IncludeDirectories.Add(new PathString(String.Format(@"{0}\target\spu\include", ps3_sdk_path), PathString.PathState.Directory));
                    dirs.IncludeDirectories.Add(new PathString(String.Format(@"{0}\target\common\include", ps3_sdk_path), PathString.PathState.Directory));
                    dirs.IncludeDirectories.Add(new PathString(String.Format(@"{0}\host-win32\spu\lib\gcc\spu-lv2\4.1.1\include", ps3_sdk_path), PathString.PathState.Directory));
                }
                else
                {
                    dirs.IncludeDirectories.Add(new PathString(String.Format(@"{0}\target\ppu\include", ps3_sdk_path), PathString.PathState.Directory));
                    dirs.IncludeDirectories.Add(new PathString(String.Format(@"{0}\target\common\include", ps3_sdk_path), PathString.PathState.Directory));
                    if (module.Configuration.Compiler == "gcc")
                    {
                        dirs.IncludeDirectories.Add(new PathString(String.Format(@"{0}\host-win32\ppu\lib\gcc\ppu-lv2\4.1.1\include", ps3_sdk_path), PathString.PathState.Directory));
                    }
                    else
                    {
                        dirs.IncludeDirectories.Add(new PathString(String.Format(@"{0}\host-win32\sn\ppu\include", ps3_sdk_path), PathString.PathState.Directory));
                    }
                }
            }
            return dirs;
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
                            Error.Throw(module.Package.Project, "VSCppMSbuildProject", "Ivalid 'skip-visualstudio-references' value: {0}, valid values are 'package_name' or 'package_name/module_name' or 'package_name/group/module_name', where group is one of 'runtime, test, example, tool'", dep);
                            break;
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

                        if (module_native.CopyLocal == CopyLocalType.True)
                        {
                            referenceFlags |= ProjectRefEntry.CopyLocal | ProjectRefEntry.CopyLocalSatelliteAssemblies;
                        }
                        else if (module_native.CopyLocal == CopyLocalType.Slim)
                        {
                            referenceFlags |= ProjectRefEntry.CopyLocal;
                        }

                        if (module_native.Link != null)
                        {
                            moduleHasLink = true;

                            if (module_native.Link.UseLibraryDependencyInputs)
                            {
                                referenceFlags |= ProjectRefEntry.UseLibraryDependencyInputs;
                            }
                        }
                    }
                    //IMTODO: I think Module_DotNet can not appear here
                    else if (module is Module_DotNet)
                    {
                        
                        var module_dotnet = module as Module_DotNet;

                        if (module_dotnet.CopyLocal == CopyLocalType.True)
                        {
                            referenceFlags |= ProjectRefEntry.CopyLocal | ProjectRefEntry.CopyLocalSatelliteAssemblies;
                        }
                        else if (module_dotnet.CopyLocal == CopyLocalType.Slim)
                        {
                            referenceFlags |= ProjectRefEntry.CopyLocal;
                        }

                        moduleHasLink = true;
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
                            
                            if ((referenceFlags & ProjectRefEntry.CopyLocal) != ProjectRefEntry.CopyLocal)
                            {
                                //Check if copylocal is defined in module dependents.
                                var dep = module.Dependents.FirstOrDefault(d => d.Dependent.Name == depProject.ModuleName && d.Dependent.Package.Name == depProject.Package.Name && d.Dependent.BuildGroup == depProject.Modules.FirstOrDefault().BuildGroup);
                                if (dep != null && dep.IsKindOf(DependencyTypes.CopyLocal))
                                {
                                    referenceFlags |= ProjectRefEntry.CopyLocal;
                                }
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

                            if(!assembly.AsIs && PathUtil.IsValidPathString(assembly.Path.Path))
                            {
                                var assemblyPath = Path.GetDirectoryName(assembly.Path.Path);

                                ISet<PathString> assemblydirs;
                                if (!referencedAssemblyDirs.TryGetValue(module.Configuration, out assemblydirs))
                                {
                                    assemblydirs = new HashSet<PathString>();
                                    projectReferencedAssemblies.Add(module.Configuration, assemblydirs);
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
                            UpdateFileEntry(comreferences, key, assembly, module.Cc.ComAssemblies.BaseDirectory, module.Configuration, flags: copyLocal);
                        }
                    }
                }
            }
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
                var excludetools = new string[] { "rc", "midl", "resx" }; // "rc" already done above.

                var validExtensions = GetPlatformObjLibExt(module);

                var custombuildtool = module.Tools
                    .Where(t => t.IsKindOf(Tool.TypePreCompile) && !t.IsKindOf(Tool.ExcludedFromBuild) && !excludetools.Contains(t.ToolName))
                    .SelectMany(t => t.OutputDependencies.FileItems, (t, fi) => fi.Path).Where(p=>validExtensions.Contains(Path.GetExtension(p.Path)));

                objectfiles = objectfiles.Except(custombuildtool);

            }
            return objectfiles;
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

        private void ImportGroup(IXmlWriter writer, string label = null, string condition = null, string project = null, string projectCondition = null, string projectLabel = null)
        {
            writer.WriteStartElement("ImportGroup");
            if (!String.IsNullOrEmpty(condition))
            {
                writer.WriteAttributeString("Condition", condition);
            }
            if (!String.IsNullOrEmpty(label))
            {
                writer.WriteAttributeString("Label", label);
            }
            if (!String.IsNullOrEmpty(project))
            {
                ImportProject(writer, project, projectCondition);
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

        private void StartElement(IXmlWriter writer, string name, string condition = null, string label = null)
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

        private void WriteElementStringWithConfigCondition(IXmlWriter writer, string name, string value, string condition = null)
        {

            if (!String.IsNullOrEmpty(condition))
            {
                writer.WriteStartElement(name);
                writer.WriteAttributeString("Condition", condition);
                writer.WriteString(value);
                writer.WriteEndElement();
            }
            else
            {
                writer.WriteElementString(name, value);
            }
        }

        internal string GetConfigCondition(Configuration config)
        {
            return " '$(Configuration)|$(Platform)' == '" + GetVSProjConfigurationName(config) + "' ";
        }

        internal string GetConfigCondition(IEnumerable<Configuration> configs)
        {
            var condition = MSBuildConfigConditions.GetCondition(configs, AllConfigurations);

            return MSBuildConfigConditions.FormatCondition("$(Configuration)|$(Platform)", condition, GetVSProjConfigurationName);
        }

        internal string GetConfigCondition(IEnumerable<FileEntry> files)
        {
            var fe = files.FirstOrDefault();
            if (fe != null)
            {
                return GetConfigCondition(fe.ConfigEntries.Select(ce => ce.Configuration));
            }
            return String.Empty;
        }

        internal string GetConfigCondition(IEnumerable<ProjectRefEntry> projectRefs)
        {
            var pre = projectRefs.FirstOrDefault();
            if (pre != null)
            {
                return GetConfigCondition(pre.ConfigEntries.Select(ce => ce.Configuration));
            }
            return String.Empty;
        }

        protected virtual void WriteNamedElements(IXmlWriter writer, IDictionary<string, string> attributes)
        {
            foreach (KeyValuePair<string, string> item in attributes)
            {
                writer.WriteNonEmptyElementString(item.Key, item.Value);
            }
        }

        protected override void WriteOneTool(IXmlWriter writer, string vcToolName, Tool tool, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            WriteToolDelegate writetoolmethod;

            if (_toolMethods.TryGetValue(vcToolName, out writetoolmethod))
            {
                writetoolmethod(writer, vcToolName, tool, module.Configuration, module, nameToDefaultXMLValue, file);
            }
            else
            {
                // This must be a custom tool. No extra data required
            }
        }


        protected string GetToolMapping(FileEntry.ConfigFileEntry configentry)
        {
            var toolname = "None";
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
            return toolname;
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
                writer.WriteStartElement("Project", "http://schemas.microsoft.com/developer/msbuild/2003");

                writer.WriteStartElement("ItemGroup");
                writer.WriteStartElement("PropertyPageSchema");
                writer.WriteAttributeString("Include", info.SchemaFile);
                writer.WriteEndElement();
                writer.WriteStartElement("AvailableItemName");
                writer.WriteAttributeString("Include", info.ToolName);
                writer.WriteElementString("Targets", "_" + info.ToolName);
                writer.WriteEndElement();
                writer.WriteEndElement();

                writer.WriteStartElement("UsingTask");
                writer.WriteAttributeString("TaskName", info.ToolName);
                writer.WriteAttributeString("TaskFactory", "XamlTaskFactory");
                writer.WriteAttributeString("AssemblyName", "Microsoft.Build.Tasks.v4.0");
                writer.WriteElementString("Task", info.SchemaFile);
                writer.WriteEndElement();

                writer.WriteStartElement("Target");
                writer.WriteAttributeString("Name", "_" + info.ToolName);
                writer.WriteAttributeString("BeforeTargets", "$(" + info.ToolName + "BeforeTargets)");
                writer.WriteAttributeString("AfterTargets", "$(" + info.ToolName + "AfterTargets)");
                writer.WriteAttributeString("Condition", "'@(" + info.ToolName + ")' != ''");
                writer.WriteAttributeString("DependsOnTargets", "$(" + info.ToolName + "DependsOn);Compute" + info.ToolName + "Output");
                writer.WriteAttributeString("Outputs", "@(" + info.ToolName + "-&gt;Metadata('Outputs')-&gt;Distinct())");
                writer.WriteAttributeString("Inputs", "@(" + info.ToolName + ");%(" + info.ToolName + ".AdditionalDependencies);$(MSBuildProjectFile)");

                writer.WriteStartElement("ItemGroup");
                writer.WriteAttributeString("Condition", "'@(SelectedFiles)' != ''");
                writer.WriteStartElement(info.ToolName);
                writer.WriteAttributeString("Remove", "@(" + info.ToolName + ")");
                writer.WriteAttributeString("Condition", "'%(Identity)' != '@(SelectedFiles)'");
                writer.WriteEndElement();
                writer.WriteEndElement();

                writer.WriteStartElement("ItemGroup");
                writer.WriteStartElement(info.ToolName + "_tlog");
                writer.WriteAttributeString("Include", "%(" + info.ToolName + ".Outputs)");
                writer.WriteAttributeString("Condition", "'%(" + info.ToolName + ".Outputs)' != '' and '%(" + info.ToolName + ".ExcludedFromBuild)' != 'true'");
                writer.WriteElementString("Source", "@(" + info.ToolName + ", '|')");
                writer.WriteEndElement();
                writer.WriteEndElement();

                writer.WriteStartElement("Message");
                writer.WriteAttributeString("Importance", "High");
                writer.WriteAttributeString("Text", "%(" + info.ToolName + ".ExecutionDescription)");
                writer.WriteEndElement();

                writer.WriteStartElement("WriteLinesToFile");
                writer.WriteAttributeString("Condition", "'@(" + info.ToolName + "_tlog)' != '' and '%(" + info.ToolName + "_tlog.ExcludedFromBuild)' != 'true'");
                writer.WriteAttributeString("File", "$(IntDir)$(ProjectName).write.1.tlog");
                writer.WriteAttributeString("Lines", "^%(" + info.ToolName + "_tlog.Source);@(" + info.ToolName + "_tlog-&gt;'%(Fullpath)')");
                writer.WriteEndElement();

                writer.WriteStartElement(info.ToolName);
                writer.WriteAttributeString("Condition", "'@(" + info.ToolName + ")' != '' and '%(" + info.ToolName + ".ExcludedFromBuild)' != 'true'");
                writer.WriteAttributeString("CommandLineTemplate", "%(" + info.ToolName + ".CommandLineTemplate)");
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

                writer.WriteAttributeString("AdditionalOptions", "%(" + info.ToolName + ".AdditionalOptions)");
                writer.WriteAttributeString("Inputs", "@(" + info.ToolName + ")");
                writer.WriteEndElement();

                writer.WriteEndElement(); // Target

                writer.WriteStartElement("PropertyGroup");
                writer.WriteElementString("ComputeLinkInputsTargets", "$(ComputeLinkInputsTargets);" + Environment.NewLine + "Compute" + info.ToolName + "Output;");
                writer.WriteElementString("ComputeLibInputsTargets", "$(ComputeLibInputsTargets);" + Environment.NewLine + "Compute" + info.ToolName + "Output;");
                writer.WriteEndElement();

                writer.WriteStartElement("Target");
                writer.WriteAttributeString("Name", "Compute" + info.ToolName + "Output");
                writer.WriteAttributeString("Condition", "'@(" + info.ToolName + ")' != ''");

                writer.WriteStartElement("ItemGroup");
                writer.WriteStartElement(info.ToolName + "DirsToMake");
                writer.WriteAttributeString("Condition", "'@(" + info.ToolName + ")' != '' and '%(" + info.ToolName + ".ExcludedFromBuild)' != 'true'");
                writer.WriteAttributeString("Include", "%(" + info.ToolName + ".Outputs)");
                writer.WriteEndElement();
                writer.WriteStartElement("Link");
                writer.WriteAttributeString("Include", "%(" + info.ToolName + "DirsToMake.Identity)");
                writer.WriteAttributeString("Condition", "'%(Extension)'=='.obj' or '%(Extension)'=='.res' or '%(Extension)'=='.rsc' or '%(Extension)'=='.lib'");
                writer.WriteEndElement();
                writer.WriteStartElement("Lib");
                writer.WriteAttributeString("Include", "%(" + info.ToolName + "DirsToMake.Identity)");
                writer.WriteAttributeString("Condition", "'%(Extension)'=='.obj' or '%(Extension)'=='.res' or '%(Extension)'=='.rsc' or '%(Extension)'=='.lib'");
                writer.WriteEndElement();
                writer.WriteStartElement("ImpLib");
                writer.WriteAttributeString("Include", "%(" + info.ToolName + "DirsToMake.Identity)");
                writer.WriteAttributeString("Condition", "'%(Extension)'=='.obj' or '%(Extension)'=='.res' or '%(Extension)'=='.rsc' or '%(Extension)'=='.lib'");
                writer.WriteEndElement();
                writer.WriteEndElement();
                writer.WriteStartElement("MakeDir");
                writer.WriteAttributeString("Directories", "@(" + info.ToolName + "DirsToMake-&gt;'%(RootDir)%(Directory)')");
                writer.WriteEndElement();
                writer.WriteEndElement(); // Target
                writer.WriteEndElement(); //Project
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

                writer.WriteStartElement("Rule.DataSource");
                writer.WriteStartElement("DateSource");
                writer.WriteAttributeString("Persistence", "ProjectFile");
                writer.WriteAttributeString("ItemType", info.ToolName);
                writer.WriteEndElement(); //DataSource
                writer.WriteEndElement(); //Rule.DataSource

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

        protected IMCPPVisualStudioExtension GetExtensionTask(ProcessableModule module)
        {
            if (module == null)
            {
                return null;
            }
            // Dynamically created packages may use exising projects from different packages. Skip such projects
            if (module.Configuration.Name != module.Package.Project.Properties["config"])
            {
                return null;
            }

            var extensionTaskName = module.Configuration.Platform + "-visualstudio-extension";

            var task = module.Package.Project.TaskFactory.CreateTask(extensionTaskName, module.Package.Project);
            var extension = task as IMCPPVisualStudioExtension;
            if (extension != null)
            {
                extension.Module = module;
                extension.Generator = new IMCPPVisualStudioExtension.MCPPProjectGenerator(this);
            }
            else if(task != null)
            {
                Log.Warning.WriteLine("Visual Studio generator extension Task '{0}' does not implement IVisualStudioExtension. Task is ignored.", extensionTaskName);
            }
            return extension;
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
            _toolMethods.Add("Midl", WriteMIDLTool);
            _toolMethods.Add("Masm", WriteAsmCompilerTool);
            _toolMethods.Add("ClCompile", WriteCompilerTool);
            _toolMethods.Add("Lib", WriteLibrarianTool);
            _toolMethods.Add("ManagedResourceCompile", WriteManagedResourceCompilerTool);
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
            //--- Xbox 360
            _toolMethods.Add("ImageXex", WriteX360ImageTool);
            _toolMethods.Add("Deployment", WriteX360DeploymentTool);
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
            ,{ "ManagedResourceCompile"           ,"resx" }
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
            ,{ "ImageXex"                         ,"xenonimage"}
            ,{ "Deployment"                       ,"deploy"  }
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
            encoding: new UTF8Encoding(true) // no byte order mask!
            );

        #endregion


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
