using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Runtime.InteropServices;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using NAnt.Core.Writers;
using NAnt.Core.Events;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Backends;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal abstract class VSDotNetProject : VSProjectBase
    {
        protected VSDotNetProject(VSSolutionBase solution, IEnumerable<IModule> modules, Guid projectTypeGuid)
            : base(solution, modules, projectTypeGuid)
        {
            // Get project configuration that equals solution startup config:
            Configuration startupconfig;
            if (ConfigurationMap.TryGetValue(solution.StartupConfiguration, out startupconfig))
            {
                StartupModule = Modules.Single(m => m.Configuration == startupconfig) as Module_DotNet;
            }
            else
            {
                StartupModule = Modules.First() as Module_DotNet;
            }

            ConfigPlatformMap = new Dictionary<Configuration, string>();

            foreach (var module in modules)
            {
                ConfigPlatformMap.Add(module.Configuration, GetConfigPlatformMapping(module as Module_DotNet));
            }
        }

        protected readonly Module_DotNet StartupModule;
        protected readonly Dictionary <Configuration, string> ConfigPlatformMap;

        #region Abstract  Methods

        protected abstract string ToolsVersion { get; }

        protected abstract string DefaultTargetFrameworkVersion { get; }

        protected abstract string ProductVersion  { get; }

        #endregion Abstract Methods

        #region Visual Studio version info

        protected virtual string Xmlns 
        { 
            get { return "http://schemas.microsoft.com/developer/msbuild/2003"; } 
        }

        protected virtual string SchemaVersion 
        { 
            get { return "2.0"; }
        }

        #endregion Visual Studio version info

        protected virtual string DefaultTarget
        {
            get { return "Build"; }
        }

        protected virtual string DefaultTargetSchema
        {
            get { return "IE50"; }
        }

        
        protected virtual string StartupConfiguration 
        {
            get { return StartupModule.Configuration.Name; }
        }

        protected virtual string StartupPlatform 
        {
            get { return GetProjectTargetPlatform(StartupModule.Configuration); }
        }

        protected virtual string TargetFrameworkVersion 
        {
            get
            {
                var frameworkversions = Modules.Cast<Module_DotNet>().Select(m => m.TargetFrameworkVersion).Where(tv=>!String.IsNullOrEmpty(tv)).OrderedDistinct();

                if (frameworkversions.Count() > 1)
                {
                    throw new BuildException(String.Format("DotNet Module '{0}' has different TargetFramework versions ({1}) defined in different configurations. VisualStudio does not support this feature.", ModuleName, frameworkversions.ToString(", ", t => t.ToString())));
                }
                
                var targetversion = frameworkversions.FirstOrDefault();

                if (String.IsNullOrEmpty(targetversion))
                {
                    targetversion = DefaultTargetFrameworkVersion;
                }

                if (!targetversion.StartsWith("v") && targetversion.Length >= 3)
                {
                    targetversion = "v" + targetversion.Substring(0, 3);
                }

                return targetversion;
            }
        }

        protected virtual ICollection<Guid> ProjectTypeGuids 
        {
            get
            {
                var guids = new List<Guid>();

                if (StartupModule.IsProjectType(DotNetProjectTypes.UnitTest))
                {
                    guids.Add(TEST_GUID);
                }
                else if (StartupModule.IsProjectType(DotNetProjectTypes.WebApp))
                {
                    guids.Add(WEBAPP_GUID);
                }
                else if (StartupModule.IsProjectType(DotNetProjectTypes.Workflow))
                {
                    guids.Add(CSPROJ__WORKFLOW_GUID);
                }
                return guids;
            }
        }
        

        protected virtual string RootNamespace
        {
            get
            {
                return StartupModule.RootNamespace ?? StartupModule.Compiler.OutputName.Replace('-', '_');
            }
        }

        #region Implementation of Inherited Abstract Methods

        protected override PathString GetDefaultOutputDir()
        {
            var module = Modules.First() as Module_DotNet;
            if (Package.Project.Properties.GetBooleanPropertyOrDefault("package." + Package.Name + ".designermode", false))
            {
                var customProjectDir = module.Compiler.Resources.BaseDirectory;
                if (String.IsNullOrEmpty(customProjectDir))
                {
                    customProjectDir = module.Compiler.SourceFiles.BaseDirectory;
                }
                if (!String.IsNullOrEmpty(customProjectDir))
                {
                    return PathString.MakeNormalized(customProjectDir);
                }
            }

            return PathString.MakeNormalized(Path.Combine(Package.PackageBuildDir.Path, Name));
        }

        protected override string UserFileName 
        {
            get { return ProjectFileName + ".user";  } 
        }

        internal override string GetProjectTargetPlatform(Configuration configuration)
        {
            string platform;
            if (!ConfigPlatformMap.TryGetValue(configuration, out platform))
            {
                platform = "AnyCPU";
            }
            return platform;
        }

        protected override void WriteProject(IXmlWriter writer)
        {
                writer.WriteStartElement("Project", Xmlns);
                writer.WriteAttributeString("DefaultTargets", DefaultTarget);
                writer.WriteAttributeString("ToolsVersion", ToolsVersion);

                WriteProjectProperties(writer);

                WriteConfigurations(writer);

                WriteImportTargets(writer); // Need to have it before build steps, otherwise some properties like $(TargetPath) may not be defined when steps are evaluated.

                WriteBuildSteps(writer);

                WriteReferences(writer);

                WriteFiles(writer);

                // Invoke Visual Studio extensions
                foreach (var module in Modules)
                {
                    foreach(var extension in GetExtensionTasks(module as ProcessableModule))
                    {
                        extension.WriteExtensionItems(writer);
                    }
                }


                WriteResourceMappingTargets(writer);

                WriteBootstrapperPackageInfo(writer);

                writer.WriteEndElement(); //Project
        }

        protected override IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>> GetUserData(ProcessableModule module)
        {
            var userdata = new List<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>>();
            return userdata;
        }

        protected override void PopulateConfigurationNameOverrides()
        {
            foreach (var module in Modules)
            {
                if (module.Package.Project != null)
                {
                    var configName = module.Package.Project.Properties[module.GroupName + ".vs-config-name"] ?? module.Package.Project.Properties["eaconfig.dotnet.vs-config-name"];
                    if (!String.IsNullOrEmpty(configName))
                    {
                        ProjectConfigurationNameOverrides[module.Configuration] = configName;
                    }
                }
            }
        }

        #endregion Implementation of Inherited Abstract Methods

        #region Set Properties Methods
        protected virtual IDictionary<string,string> GetConfigurationProperties(Module_DotNet module)
        {
            var data = new OrderedDictionary<string, string>();

            var debugtype = module.Compiler.GetOptionValue("debug");
            var debug = module.Compiler.HasOption("debug") && (debugtype != "-");

            data.Add("DefineConstants", module.Compiler.Defines.ToString(";"));
            data.Add("DebugSymbols", debug);
            data.Add("Optimize", module.Compiler.HasOption("optimize") && module.Compiler.GetOptionValue("optimize") != "-");
            data.Add("OutputPath", GetProjectPath(module.OutputDir, addDot: true));
            data.Add("AllowUnsafeBlocks", module.Compiler.HasOption("unsafe"));
            data.Add("BaseAddress", "285212672");
            data.Add("CheckForOverflowUnderflow", "false");
            data.Add("ConfigurationOverrideFile", "");
            if (!module.Compiler.DocFile.IsNullOrEmpty())
            {
                data.Add("DocumentationFile", GetProjectPath(module.Compiler.DocFile));
            }
            data.Add("FileAlignment", "4096");
            data.Add("NoStdLib", module.Compiler.GetBoolenOptionValue("nostdlib"));
            data.AddIfTrue("NoConfig", module.Compiler.GetBoolenOptionValue("noconfig"));
            data.AddNonEmpty("NoWarn", module.Compiler.GetOptionValues("nowarn", ","));
            data.Add("RegisterForComInterop", "false");
            data.Add("RemoveIntegerChecks", "false");

            var warnaserrorlist = module.Compiler.GetOptionValue("warningsaserrors.list");
            if (!String.IsNullOrEmpty(warnaserrorlist))
            {

                data.Add("TreatWarningsAsErrors", false);
                data.Add("WarningsAsErrors", warnaserrorlist);
            }
            else
            {
                data.Add("TreatWarningsAsErrors", module.Compiler.HasOption("warnaserror"));
            }
            data.Add("WarningLevel", "4");

            var debugtypeString = "none";
            if(debug)
            {
                if (debugtypeString == "+" || String.IsNullOrEmpty(debugtypeString))
                {
                    debugtypeString = "full";
                }
                else
                {
                    debugtypeString = debugtype;
                }
            }

            data.Add("DebugType", debugtypeString);
            data.AddNonEmpty("PlatformTarget", module.Compiler.GetOptionValue("platform"));

            if (module.DisableVSHosting)
            {
                data.Add("UseVSHostingProcess", (!module.DisableVSHosting));
            }

            var rspFile = CreateResponseFileIfNeeded(module);
            if(!String.IsNullOrEmpty(rspFile))
            {
                data.AddNonEmpty("CompilerResponseFile", GetProjectPath(rspFile));
            }

            if (module.GenerateSerializationAssemblies != DotNetGenerateSerializationAssembliesTypes.None)
            {
                data.Add("GenerateSerializationAssemblies", module.GenerateSerializationAssemblies.ToString());
            }

            if (EnableFxCop(module))
            {
                data.AddNonEmpty("CodeAnalysisRuleSet", Package.Project.Properties.GetPropertyOrDefault(module.GroupName + ".fxcop.ruleset",
                                                                         Package.Project.Properties.GetPropertyOrDefault("package.fxcop.ruleset", String.Empty)));

                data.Add("RunCodeAnalysis", true);
            }

            if (module.Compiler.HasOption("nowin32manifest"))
            {
                data.Add("NoWin32Manifest", true);
            }
            
            return data;
        }

        protected virtual IDictionary<string,string> GetProjectProperties()
        {
            var data = new OrderedDictionary<string, string>();


            data.AddNonEmpty("ProductVersion", ProductVersion);
            data.AddNonEmpty("SchemaVersion", SchemaVersion);
            data.Add("ProjectGuid", ProjectGuidString);


            data.AddNonEmpty("ApplicationManifest", StartupModule.ApplicationManifest);
            data.AddNonEmpty("ApplicationIcon", GetProjectPath(StartupModule.Compiler.ApplicationIcon));
            data.AddNonEmpty("AssemblyKeyContainerName", String.Empty);
            data.Add("AssemblyName", StartupModule.Compiler.OutputName);

            var keyfile = StartupModule.Compiler.GetOptionValue("keyfile");
            data.Add("SignAssembly", !String.IsNullOrEmpty(keyfile));
            data.AddNonEmpty("AssemblyOriginatorKeyFile", GetProjectPath(keyfile));

            data.AddNonEmpty("DefaultClientScript", "JScript");
            data.AddNonEmpty("DefaultHTMLPageLayout", "Grid");
            data.AddNonEmpty("DefaultTargetSchema", DefaultTargetSchema);
            data.AddNonEmpty("DelaySign", "false");

            data.Add("OutputType", OutputType.ToString());

            data.AddNonEmpty("TargetFrameworkVersion", TargetFrameworkVersion);
            data.AddNonEmpty("RootNamespace", RootNamespace);
            data.AddNonEmpty("RunPostBuildEvent", StartupModule.RunPostBuildEventCondition);
            data.AddNonEmpty("StartupObject", String.Empty);
            data.AddNonEmpty("FileUpgradeFlags", String.Empty);
            data.AddNonEmpty("UpgradeBackupLocation", String.Empty);
            data.AddNonEmpty("AppDesignerFolder", StartupModule.AppDesignerFolder.IsNullOrEmpty()? "Properties" : StartupModule.AppDesignerFolder.Path);

            var projectTypeGuids = ProjectTypeGuids.OrderedDistinct();
            if (projectTypeGuids.Count() > 0)
            {
                data.Add("ProjectTypeGuids", String.Format("{0};{1}", projectTypeGuids.ToString(";", g => SolutionGenerator.ToString(g)), SolutionGenerator.ToString(ProjectTypeGuid)));
            }

            return data;
        }

        #endregion

        #region Write Methods

        protected void WriteLinkPath(IXmlWriter writer, FileEntry fe)
        {
            if (!fe.Path.IsPathInDirectory(OutputDir))
            {
                writer.WriteNonEmptyElementString("Link", GetLinkPath(fe));
            }
        }

        protected void WriteNonEmptyElementString(IXmlWriter writer, string name, string value)
        {
            if (!String.IsNullOrWhiteSpace(value)) 
            {
                writer.WriteElementString(name, value);
            }
        }

        protected virtual void WriteProjectProperties(IXmlWriter writer)
        {
            writer.WriteStartElement("PropertyGroup");

            writer.WriteStartElement("Configuration");
            writer.WriteAttributeString("Condition", " '$(Configuration)' == '' ");
            writer.WriteString(StartupConfiguration);
            writer.WriteEndElement();

            writer.WriteStartElement("Platform");
            writer.WriteAttributeString("Condition", " '$(Platform)' == '' ");
            writer.WriteString(StartupPlatform);
            writer.WriteEndElement();

            foreach (var entry in GetProjectProperties())
            {
                writer.WriteElementString(entry.Key, entry.Value);
            }


            WriteSourceControlIntegration(writer);

            writer.WriteEndElement(); //PropertyGroup
        }

        protected virtual void WriteConfigurations(IXmlWriter writer)
        {
            foreach (Module_DotNet module in Modules)
            {
                WriteOneConfiguration(writer, module);
            }
        }

        protected virtual void WriteOneConfiguration(IXmlWriter writer, Module_DotNet module)
        {
            WriteConfigurationProperties(writer, module);
            
        }

        protected virtual void WriteConfigurationProperties(IXmlWriter writer, Module_DotNet module)
        {
            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", String.Format(" '$(Configuration)|$(Platform)' == '{0}' ", GetVSProjConfigurationName(module.Configuration)));

            foreach (var entry in GetConfigurationProperties(module))
            {
                writer.WriteElementString(entry.Key, entry.Value);
            }

            writer.WriteEndElement(); //PropertyGroup
        }


        protected virtual void WriteBuildSteps(IXmlWriter writer)
        {
            var buildstepgroups = GroupBuildSteps();

            foreach (var group in buildstepgroups)
            {
                writer.WriteStartElement("PropertyGroup");

                WriteConfigCondition(writer, group.Configurations);

                foreach (var buildstep in group.BuildSteps)
                {
                    var cmd = buildstep.Commands.Aggregate(new StringBuilder(), (sb, c) => sb.AppendLine(c.CommandLine)).ToString();
                    
                    if (BuildGenerator.IsPortable)
                    {
                        cmd = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(cmd, OutputDir.Path);
                    }
                    
                    if (buildstep.IsKindOf(BuildStep.PreBuild))
                    {
                        writer.WriteElementString("PreBuildEvent", cmd);
                    }
                    if (buildstep.IsKindOf(BuildStep.PostBuild))
                    {
                        writer.WriteElementString("PostBuildEvent", cmd);
                    }
                }

                writer.WriteEndElement(); //PropertyGroup
            }

        }

        protected virtual void WriteReferences(IXmlWriter writer)
        {
            IEnumerable<ProjectRefEntry> projectReferences;
            IDictionary<PathString, FileEntry> references;

            GetReferences(out projectReferences, out references);

            if(projectReferences.FirstOrDefault() != null || references.Count > 0)
            {
                // Supress warning
                // MSB3270: There was a mismatch between the processor architecture of the project being built "MSIL" and the processor architecture of the reference
                foreach(var group in ConfigPlatformMap.GroupBy(e=>e.Value))
                {
                   if(group.Key == "AnyCPU")
                   {
                       writer.WriteStartElement("PropertyGroup");
                       writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(group.Select(e=>e.Key)));
                       writer.WriteElementString("ResolveAssemblyWarnOrErrorOnTargetArchitectureMismatch", "None");
                       writer.WriteEndElement(); 
                   }
                }
            }

            WriteAssemblyReferences(writer, references.Values);
            WriteProjectRefernces(writer, projectReferences);
            WriteComReferences(writer);
            WriteWebReferences(writer);

            
        }

        protected virtual void WriteAssemblyReferences(IXmlWriter writer, IEnumerable<FileEntry> references)
        {
            foreach (var group in references.GroupBy(f => f.ConfigSignature, f => f))
            {
                if (group.Count() > 0)
                {
                    writer.WriteStartElement("ItemGroup");

                     var first = group.FirstOrDefault();

                     var condition_string = (first != null) ? GetConfigCondition(first.ConfigEntries.Select(ce => ce.Configuration)) : String.Empty;
                    
                    WriteConfigCondition(writer, group);

                    foreach (var reference in group.OrderBy(fe=>fe.Path))
                    {
                        string refNamespace = Path.GetFileNameWithoutExtension(reference.Path.Path);
                        string refName = Path.GetFileNameWithoutExtension(reference.Path.Path);
                        string refHintPath = GetProjectPath(reference.Path);

                        bool isSystem = String.IsNullOrEmpty(Path.GetDirectoryName(reference.Path.Path)) || reference.Path.Path.Contains(@"\GAC_MSIL\");
                        if (isSystem)
                        {
                            refHintPath = Path.GetFileName(reference.Path.Path);
                        }

                        bool isCopyLocal = null != reference.ConfigEntries.Find(ce => ce.IsKindOf(FileEntry.CopyLocal));

                        writer.WriteStartElement("Reference");

                        writer.WriteNonEmptyAttributeString("Condition", condition_string);

                        writer.WriteAttributeString("Include", refNamespace);
                        {
                            writer.WriteElementString("Name", refName);
                            writer.WriteElementString("HintPath", refHintPath);
                            writer.WriteElementString("Private", isCopyLocal.ToString());
                        }
                        writer.WriteEndElement(); //Reference
                    }
                    writer.WriteEndElement(); //ItemGroup
                }
            }
        }

        protected virtual void WriteProjectRefernces(IXmlWriter writer, IEnumerable<ProjectRefEntry> projectReferences)
        {
            if (projectReferences.Count() > 0)
            {
                var projrefGroups = GroupProjectReferencesByConfigConditions(projectReferences);

                foreach (var entry in projrefGroups)
                {
                    if (entry.Value.Count() > 0)
                    {
                         writer.WriteStartElement("ItemGroup");
                         writer.WriteNonEmptyAttributeString("Condition", entry.Key);

                        foreach (var projRefWithType in entry.Value)
                        {
                            var projRefEntry = projRefWithType.Item1;
                            var projRefType = new BitMask(projRefWithType.Item2);

                            writer.WriteStartElement("ProjectReference");
                            {
                                writer.WriteAttributeString("Include", PathUtil.RelativePath(Path.Combine(projRefEntry.ProjectRef.OutputDir.Path, projRefEntry.ProjectRef.ProjectFileName), OutputDir.Path));
                                writer.WriteElementString("Project", projRefEntry.ProjectRef.ProjectGuidString);
                                writer.WriteElementString("Name", projRefEntry.ProjectRef.Name);
                                writer.WriteElementString("Private", projRefType.IsKindOf(ProjectRefEntry.CopyLocal));
                            }
                            writer.WriteEndElement(); // ProjectReference
                        }
                        writer.WriteEndElement(); // ItemGroup (ProjectReferences)
                    }
                }
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

        protected virtual void WriteComReferences(IXmlWriter writer)
        {

            var comreferences = new Dictionary<PathString, FileEntry>();
            foreach (var module in Modules.Cast<Module_DotNet>())
            {
                foreach (var comref in module.Compiler.ComAssemblies.FileItems)
                {
                    var key = new PathString(Path.GetFileName(comref.Path.Path));
                    UpdateFileEntry(comreferences, key, comref, module.Compiler.ComAssemblies.BaseDirectory, module.Configuration);
                }
            }

            foreach (var group in comreferences.Values.GroupBy(f => f.ConfigSignature, f => f))
            {
                if (group.Count() > 0)
                {
                    writer.WriteStartElement("ItemGroup");

                    WriteConfigCondition(writer, group);

                    foreach (var fe in group.OrderBy(fe => fe.Path))
                    {

                        if (!File.Exists(fe.Path.Path))
                        {
                            string msg = String.Format("Unable to generate interop assembly for COM reference, file '{0}' not found", fe.Path.Path);
                            throw new BuildException(msg);
                        }

                        System.Runtime.InteropServices.ComTypes.ITypeLib typeLib = null;
                        try
                        {
                            LoadTypeLibEx(fe.Path.Path, RegKind.RegKind_None, out typeLib);
                        }
                        catch (Exception ex)
                        {
                            string msg = String.Format("Unable to generate interop assembly for COM reference '{0}'.", fe.Path.Path);
                            new BuildException(msg, ex);
                        }
                        if (typeLib == null)
                        {
                            string msg = String.Format("Unable to generate interop assembly for COM reference '{0}'.", fe.Path.Path);
                            new BuildException(msg);
                        }

                        string libName = Marshal.GetTypeLibName(typeLib);
                        Guid libGuid = Marshal.GetTypeLibGuid(typeLib);

                        IntPtr pTypeLibAttr = IntPtr.Zero;
                        typeLib.GetLibAttr(out pTypeLibAttr);

                        System.Runtime.InteropServices.ComTypes.TYPELIBATTR typeLibAttr =
                            (System.Runtime.InteropServices.ComTypes.TYPELIBATTR)Marshal.PtrToStructure(pTypeLibAttr, typeof(System.Runtime.InteropServices.ComTypes.TYPELIBATTR));

                        writer.WriteStartElement("COMReference");
                        writer.WriteAttributeString("Include", GetProjectPath(PathNormalizer.Normalize(libName)));
                        writer.WriteElementString("Guid", "{" + libGuid.ToString() + "}");
                        writer.WriteElementString("VersionMajor", typeLibAttr.wMajorVerNum.ToString());
                        writer.WriteElementString("VersionMinor", typeLibAttr.wMinorVerNum.ToString());
                        writer.WriteElementString("Lcid", typeLibAttr.lcid.ToString());
                        writer.WriteElementString("Isolated", "False");
                        writer.WriteElementString("WrapperTool", "tlbimp");
                        writer.WriteEndElement(); //COMReference
                    }
                }

                writer.WriteFullEndElement(); //References
            }


        }

        protected virtual void WriteWebReferences(IXmlWriter writer)
        {
            if (StartupModule.WebReferences != null && StartupModule.WebReferences.Options.Count > 0)
            {
                var webrefs = StartupModule.WebReferences;
                if (webrefs.Options.Count > 0)
                {
                    string refOutput = "webreferences";
                    writer.WriteStartElement("ItemGroup");
                    writer.WriteStartElement("WebReferences");
                    writer.WriteAttributeString("Include", PathNormalizer.Normalize(refOutput, false));
                    writer.WriteEndElement(); //WebReferences
                    writer.WriteEndElement(); //ItemGroup

                    writer.WriteStartElement("ItemGroup");
                    foreach (var webref in webrefs.Options)
                    {
                        var refName = webref.Key;
                        var refUrl = webref.Value;

                        writer.WriteStartElement("WebReferenceUrl");
                        writer.WriteAttributeString("Include", refUrl);
                        writer.WriteElementString("UrlBehavior", "Dynamic");
                        writer.WriteElementString("RelPath", Path.Combine(refOutput, refName));
                        writer.WriteElementString("UpdateFromURL", refUrl);
                        writer.WriteEndElement(); //WebReferenceUrl
                    }
                    writer.WriteEndElement(); //ItemGroup

                    writer.WriteStartElement("ItemGroup");

                    foreach (var webref in StartupModule.WebReferences.Options)
                    {
                        var refName = webref.Key.ToString();
                        var refUrl = webref.Value;

                        string path = System.IO.Path.Combine(refOutput, refName);
                        string map = System.IO.Path.Combine(path, "Reference.map");
                        string disco = System.IO.Path.Combine(path, Path.GetFileNameWithoutExtension(refUrl));

                        writer.WriteStartElement("None");
                        writer.WriteAttributeString("Include", map);
                        writer.WriteElementString("Generator", "MSDiscoCodeGenerator");
                        writer.WriteElementString("LastGenOutput", "Reference.cs");
                        writer.WriteEndElement(); //None
                        writer.WriteStartElement("None");
                        writer.WriteAttributeString("Include", disco);
                        writer.WriteEndElement(); //None
                    }
                    writer.WriteEndElement(); //ItemGroup
                }
            }
        }

        private const uint Resource = 8388608;
        private const uint Source   = 16777216;
        private const uint Forms    = 33554432;
        private const uint Xaml     = 67108864;
        private const uint Xoml     = 134217728;

        protected virtual void WriteFiles(IXmlWriter writer)
        {

            var files = GetAllFiles(tool =>
            {
                var filesets = new List<Tuple<FileSet, uint, Tool>>();
                filesets.Add(Tuple.Create((tool as DotNetCompiler).Resources as FileSet, Resource, tool));
                filesets.Add(Tuple.Create((tool as DotNetCompiler).NonEmbeddedResources, Resource, tool));
                filesets.Add(Tuple.Create((tool as DotNetCompiler).ContentFiles, Resource, tool));
                filesets.Add(Tuple.Create((tool as DotNetCompiler).SourceFiles, Source, tool));
                
                return filesets;
            }).Select(e => e.Value);


            PrepareSubTypeMappings(writer);

            //Config signature includes file type to print different types in separate item groups. Here group strictly by configuration names because of resource and Xaml files need to be evaluated together.
            foreach (var group in files.GroupBy(f => f.ConfigEntries.OrderBy(ce => ce.Configuration.Name).Aggregate(new StringBuilder(), (result, ce) => result.AppendLine(ce.Configuration.Name)).ToString(), f => f))
            {
                if (group.Count() > 0)
                {
                    // Compute these hashes to use in code behind:
                    IDictionary<string, string> srcHash;
                    IDictionary<string, string> resHash;
                    IDictionary<string, string> xamlHash;
                    IDictionary<string, string> xomlHash;

                    ComputeCodeBehindCache(group, out srcHash, out resHash, out xamlHash, out xomlHash);

                    var enableResourceMapping = Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.sln.enable-resource-mapping", true);

                    if (enableResourceMapping && (xamlHash.Count + xomlHash.Count > 0))
                    {
                        _resourceMapping = new Dictionary<string, ResourceMappingData>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);
                    }

                    // add web config files to resource mapping
                    foreach (var fe in group)
                    {
                        if (fe.Path.Path.EndsWith("Web.config"))
                        {
                            if (_resourceMapping == null)
                            {
                                _resourceMapping = new Dictionary<string, ResourceMappingData>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);
                            }
                            ResourceMappingData data = new ResourceMappingData(fe, "Web.config", null);
                            _resourceMapping.Add(fe.Path.Path, data);
                        }
                    }

                    // Add Item File Items
                    writer.WriteStartElement("ItemGroup");
                    WriteConfigCondition(writer, group);

                    foreach (var fe in group)
                    {
                        var relPath = PathUtil.RelativePath(fe.Path.Path, OutputDir.Path);
                        var relPathKey = relPath.TrimExtension();

                        var firstFe = fe.ConfigEntries.First();

                        if (!firstFe.IsKindOf(FileEntry.ExcludedFromBuild))
                        {
                            if (firstFe.IsKindOf(Resource))
                            {
                                WriteResourceFile(writer, fe, relPath, relPathKey, resHash, srcHash, xomlHash);
                            }
                            else if (firstFe.IsKindOf(Xaml))
                            {
                                WriteXamlFile(writer, fe, relPath);
                            }
                            else if (firstFe.IsKindOf(Xoml))
                            {
                                WriteXomlFile(writer, fe, relPath);
                            }
                            else if (firstFe.IsKindOf(Source))
                            {
                                WriteSourceFile(writer, fe, relPath, relPathKey, resHash, srcHash, xomlHash, xamlHash);
                            }
                            else
                            {
                                writer.WriteStartElement("None");
                                writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                                WriteLinkPath(writer, fe);
                                writer.WriteEndElement(); 
                            }
                        }
                        else
                        {
                            writer.WriteStartElement("None");
                            writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                            WriteLinkPath(writer, fe);
                            writer.WriteEndElement(); //Content
                        }

                    }
                    writer.WriteEndElement(); //ItemGroup
                }
            }
        }

        protected virtual void WriteXamlFile(IXmlWriter writer, FileEntry fe, string relPath)
        {
            string filename = fe.Path.Path;

            if (File.Exists(filename))
            {
                string elementName = "Page";
                using (var reader = new XmlTextReader(filename))
                {
                    while (reader.Read())
                    {
                        // Get first element:
                        if (reader.NodeType == XmlNodeType.Element)
                        {
                            if (0 == String.Compare(reader.LocalName, "App") || 0 == String.Compare(reader.LocalName, "Application"))
                            {
                                elementName = "ApplicationDefinition";
                            }
                            else if (0 == String.Compare(reader.LocalName, "Activity"))
                            {
                                elementName = "XamlAppDef";
                            }

                            break;
                        }
                    }
                }

                var mappingData = PrepareResourceMapping(fe, ref elementName);

                var subType = _savedCsprojData.GetSubTypeMapping(elementName, relPath, "Designer");

                writer.WriteStartElement(elementName);
                writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                writer.WriteElementString("Generator", "MSBuild:Compile");
                writer.WriteElementString("SubType", subType);
                if (mappingData != null)
                {
                    mappingData.Elements.Add(Tuple.Create("Generator", "MSBuild:Compile"));
                    mappingData.Elements.Add(Tuple.Create("SubType", subType));
                }
                WriteLinkPath(writer, fe);

                writer.WriteEndElement(); //ElementName
            }

        }

        protected void WriteXomlFile(IXmlWriter writer, FileEntry fe, string relPath)
        {
            string filename = fe.Path.Path;
            if (File.Exists(filename))
            {
                var elementName = "Content";
                var mappingData = PrepareResourceMapping(fe, ref elementName);
                var subType = _savedCsprojData.GetSubTypeMapping(elementName, relPath, "Component");
                writer.WriteStartElement(elementName);
                writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                writer.WriteElementString("SubType", subType);
                if (mappingData != null)
                {
                    mappingData.Elements.Add(Tuple.Create("SubType", subType));
                }
                WriteLinkPath(writer, fe);

                writer.WriteEndElement();
            }
        }

        protected void WriteResourceFile(IXmlWriter writer, FileEntry fe, string relPath, string relPathKey, IDictionary<string, string> resHash, IDictionary<string, string> srcHash, IDictionary<string, string> xomlHash)
        {
            string filename = fe.Path.Path;

            bool isGlobalResx = false;
            bool isWebConfig = false;
            string linkedFileName = null;
            string langExt = String.Empty;
            switch (Path.GetExtension(filename))
            {
                case ".asax":
                case ".asmx":
                case ".aspx":
                case ".css":
                    writer.WriteStartElement("Content");
                    writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                    break;
                case ".cs":
                    {
                        writer.WriteStartElement("Compile");
                        writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                        if (srcHash.ContainsKey(relPathKey) || resHash.ContainsKey(relPathKey))
                        {
                            string baseName = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(filename));
                            writer.WriteElementString("DependentUpon", baseName + ".resx");
                        }
                    }
                    break;
                case ".resx":
                    writer.WriteStartElement("EmbeddedResource");
                    writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                    if (srcHash.ContainsKey(relPathKey) && srcHash[relPathKey] == ".cs")
                    {
                        writer.WriteElementString("DependentUpon", Path.GetFileNameWithoutExtension(filename) + ".cs");
                    }
                    else // very likely it's a global resx file
                    {
                        string designerFileName = filename.Replace("resx", "Designer.cs");
                        if (File.Exists(designerFileName))
                        {
                            isGlobalResx = true;
                            linkedFileName = designerFileName;
                        }
                        else
                        {
                            // Check for language indexes (like Resources.de.resx):
                            designerFileName = Path.GetFileNameWithoutExtension(filename);
                            langExt = Path.GetExtension(designerFileName);
                            // Not very clean. I don't know if this is language extension or just file name with dot.
                            if (!String.IsNullOrEmpty(langExt))
                            {
                                designerFileName = Path.GetFileNameWithoutExtension(designerFileName) + ".Designer.cs";
                                if (File.Exists(designerFileName))
                                {
                                    isGlobalResx = true;
                                    linkedFileName = designerFileName;
                                }
                            }
                        }
                        if (isGlobalResx && Version.StrCompareVersions("10.00") >= 0)
                        {
                            writer.WriteElementString("Generator", "ResXFileCodeGenerator");
                            writer.WriteElementString("LastGenOutput", Path.GetFileName(designerFileName));
                            writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("EmbeddedResource", relPath, "Designer"));
                        }
                    }
                    break;
                case ".layout":
                case ".rules":
                    writer.WriteStartElement("EmbeddedResource");
                    writer.WriteAttributeString("Include", GetProjectPath(fe.Path));

                    string hashedExt;
                    if (srcHash.TryGetValue(relPathKey, out hashedExt))
                    {
                        if (hashedExt == ".cs")
                            writer.WriteElementString("DependentUpon", Path.GetFileNameWithoutExtension(filename) + hashedExt);
                    }
                    else if (xomlHash.TryGetValue(relPathKey, out hashedExt))
                    {
                        writer.WriteElementString("DependentUpon", Path.ChangeExtension(GetProjectPath(fe.Path), ".xoml"));
                    }
                    break;

                case ".settings":

                    var flags = GetFileEntryFlags(fe);
                    if (flags != null && flags.IsKindOf(FileData.ContentFile))
                    {
                        writer.WriteStartElement("Content");
                    }
                    else
                    {
                        writer.WriteStartElement("None");
                    }
                    
                    writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                    writer.WriteElementString("Generator", "SettingsSingleFileGenerator");
                    writer.WriteElementString("LastGenOutput", Path.GetFileNameWithoutExtension(filename) + ".Designer.cs");
                    break;

                case ".config":
                    
                    var flags1 = GetFileEntryFlags(fe);
                    if (flags1 != null && flags1.IsKindOf(FileData.ContentFile))
                    {
                        writer.WriteStartElement("Content");
                    }
                    else
                    {
                        writer.WriteStartElement("None");
                    }

                    if (fe.Path.Path.EndsWith("Web.config"))
                    {
                        isWebConfig = true;
                        writer.WriteAttributeString("Include", OutputDir + "/Web.config");
                    }
                    else
                    {
                        writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                    }

                    break;

                default:

                    var flags2 = GetFileEntryFlags(fe);

                    if (flags2 != null && flags2.IsKindOf(FileData.EmbeddedResource))
                    {
                        writer.WriteStartElement("EmbeddedResource");
                        writer.WriteAttributeString("Include", GetProjectPath(fe.Path));

                    }
                    else if (flags2 != null && flags2.IsKindOf(FileData.ContentFile))
                    {
                        writer.WriteStartElement("Content");
                        writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                    }
                    else
                    {
                        writer.WriteStartElement("Resource");
                        writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                    }

                    break;
            }


            SetCopyToOutputDirectory(writer, fe.ConfigEntries);

            bool linkResources = !String.IsNullOrEmpty(fe.FileSetBaseDir) && fe.Path.Path.StartsWith(fe.FileSetBaseDir) && !isWebConfig;
            if (linkResources)
            {
                if (!isGlobalResx)
                {
                    WriteLinkPath(writer, fe);
                }
                else if (fe.Path.IsPathInDirectory(PathString.MakeNormalized(fe.FileSetBaseDir)))
                {
                    if (Path.GetExtension(filename) == ".resx")
                    {
                        if (linkedFileName == null)
                            linkedFileName = filename.Replace("resx", "Designer.cs");

                        string linkPath = ConvertNamespaceToLinkPath(linkedFileName);
                        if (linkPath != null)
                            writer.WriteElementString("Link", linkPath + langExt + ".resx");
                    }
                }
            }

            writer.WriteEndElement(); 	// None/Content/EmbeddedResource
        }

        protected BitMask GetFileEntryFlags(FileEntry fe)
        {
            var fileitem = fe.ConfigEntries.First().FileItem;
            return fileitem.Data as BitMask;
        }

        protected void SetCopyToOutputDirectory(IXmlWriter writer, List<FileEntry.ConfigFileEntry> configEntries)
        {
            var fe = configEntries.FirstOrDefault();
            if (fe == null)
            {
                return;
            }

            if (fe.FileItem != null && !String.IsNullOrEmpty(fe.FileItem.OptionSetName))
            {
                var content_action = fe.FileItem.OptionSetName.TrimWhiteSpace().ToLowerInvariant();

                if (content_action == "copy-always")
                {
                    writer.WriteElementString("CopyToOutputDirectory", "Always");
                }
                else if (content_action == "do-not-copy")
                {
                    // Leave empty
                }
                else if (content_action == "copy-if-newer")
                {
                    writer.WriteElementString("CopyToOutputDirectory", "PreserveNewest");
                }
                else if (!String.IsNullOrEmpty(content_action))
                {
                    Log.Warning.WriteLine("csproj {0}, file '{1}' : invalid value of CopyToOutputDirectory action '{2}', valid values are: {3}.", Name, GetProjectPath(fe.FileItem.Path), content_action, "copy-always, do-not-copy, copy-if-newer");
                }
            }
        }

        //IMTODO: complete functionality for link.resourcefiles.nonembed
        private bool LinkResourcesNonEmbed(IModule module)
        {
            return ConvertUtil.ToBooleanOrDefault(module.Package.Project.Properties[module.GroupName + ".csproj.link.resourcefiles.nonembed"], true);
        }

        protected void WriteSourceFile(IXmlWriter writer, FileEntry fe, string relPath, string relPathKey, IDictionary<string, string> resHash, IDictionary<string, string> srcHash, IDictionary<string, string> xomlHash, IDictionary<string, string> xamlHash)
        {
            string filename = fe.Path.Path;

            writer.WriteStartElement("Compile");
            writer.WriteAttributeString("Include", GetProjectPath(fe.Path));

            var codeBehind = Path.GetFileNameWithoutExtension(filename);
            var codeBehindKey = relPathKey.TrimExtension();

            bool isGlobalResource = false;

            if (xamlHash.ContainsKey(codeBehindKey))
            {
                writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "Code"));
                writer.WriteElementString("DependentUpon", codeBehind);
            }
            else if (xomlHash.ContainsKey(codeBehindKey))
            {
                writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "Component"));
                writer.WriteElementString("DependentUpon", codeBehind);
            }
            else if (resHash.ContainsKey(codeBehindKey.TrimExtension()))
            {
                if (Path.GetExtension(codeBehind) == ".asax")
                {
                    writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "Code"));
                    writer.WriteElementString("DependentUpon", codeBehind);
                }
                else if (Path.GetExtension(codeBehind) == ".asmx")
                {
                    writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "Component"));
                    writer.WriteElementString("DependentUpon", codeBehind);
                }
                else if (Path.GetExtension(codeBehind) == ".aspx")
                {
                    writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "ASPXCodeBehind"));
                    writer.WriteElementString("DependentUpon", codeBehind);
                }
                else
                {
                    writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "Code"));
                }
            }
            else if (resHash.ContainsKey(relPathKey))
            {
                string resourceExts;
                if (resHash.TryGetValue(relPathKey, out resourceExts))
                {
                    //IM: I make this code after nantToVSTools, but I'm not sure if subtype=Designer is applicable to .cs files here?
                    writer.WriteNonEmptyElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "Designer", allowRemove:true));
                }
                else
                {
                    writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "Code"));
                }
            }
            else
            {
                writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("Compile", relPath, "Code"));
            }

            if (filename.ToLower().EndsWith(".designer.cs"))
            {
                int index = codeBehind.ToLower().LastIndexOf(".designer");

                string partial = codeBehind.Substring(0, index);
                string partialKey = relPathKey.Substring(0, relPathKey.ToLower().LastIndexOf(".designer"));
                string ext;

                if (srcHash.TryGetValue(partialKey, out ext) && ext == ".cs")
                {
                    if (partial.EndsWith(".aspx", StringComparison.OrdinalIgnoreCase))
                    {
                        // VisualStudio 2010 defaults to having the <DependentUpon> element
                        // ${sourcefile}.aspx.designer.cs be set to ${sourcefile}.aspx
                        writer.WriteElementString("DependentUpon", partial);
                    }
                    else
                    {
                        writer.WriteElementString("DependentUpon", partial + ".cs");
                    }
                }
                else
                {
                    // nedd to use full path in file exists here because relative path will be evaluated against current dir, not the project OutputDir
                    var dependentUponProbe = filename.Substring(0, filename.ToLower().LastIndexOf(".designer"));
                    if (File.Exists(dependentUponProbe + ".resx"))
                    {
                        writer.WriteElementString("DependentUpon", partial + ".resx");
                        isGlobalResource = true;
                    }
                    else if (File.Exists(dependentUponProbe + ".settings"))
                    {
                        writer.WriteElementString("DependentUpon", partial + ".settings");
                    }
                }
            }

            if (!fe.Path.IsPathInDirectory(OutputDir))
            {
                if (isGlobalResource)
                {
                    string linkPath = ConvertNamespaceToLinkPath(filename);
                    if (linkPath != null)
                        writer.WriteElementString("Link", linkPath + ".Designer.cs");
                }
                else
                {
                    // The source is outside the csproj directory.  Link to it, based on the baseDir of the fileSet
                    WriteLinkPath(writer, fe);
                }
            }

            writer.WriteEndElement(); //Compile
        }

        /// <summary>
        /// Since LinkPath (the Link tag) is used by VS05 to construct the name of the global resources, so we need 
        /// to make the link path same as the namespace structure of the global resource designer file.
        /// For example:
        /// Resources.Designer.cs: namespace DefaultNameSpace.Properties { class Resources ... }
        /// LinkPath (designer): should be Properties\\Resources.Designer.cs (DefaultNameSpace is omitted)
        /// LinkPath (resx)    : should be Properties\\Resources.resx
        /// </summary>
        /// <param name="filename"></param>
        /// <returns></returns>
        protected string ConvertNamespaceToLinkPath(string filename)
        {
            string retnamespace = null;
            string retclassname = null;
            using (StreamReader sr = File.OpenText(filename))
            {
                while (sr.Peek() > -1)
                {
                    string str = sr.ReadLine();
                    string matchnamespace = @"namespace ((\w+.)*)";
                    string matchnamespaceCaps = @"Namespace ((\w+.)*)";
                    string matchclassname = @"typeof\((\w+.)\).Assembly";
                    Regex matchNamespaceRE = new Regex(matchnamespace);
                    Regex matchNamespaceCapsRE = new Regex(matchnamespaceCaps);
                    Regex matchClassnameRE = new Regex(matchclassname);

                    if (matchNamespaceRE.Match(str).Success)
                    {
                        Match namematch = matchNamespaceRE.Match(str);
                        retnamespace = namematch.Groups[1].Value;
                        retnamespace = retnamespace.Replace("{", "");
                        retnamespace = retnamespace.Trim();
                    }
                    else if (matchNamespaceCapsRE.Match(str).Success)
                    {
                        Match namematch = matchNamespaceCapsRE.Match(str);
                        retnamespace = namematch.Groups[1].Value;
                        retnamespace = retnamespace.Trim();
                    }
                    else if (matchClassnameRE.Match(str).Success)
                    {
                        Match namematch = matchClassnameRE.Match(str);
                        retclassname = namematch.Groups[1].Value;
                        retclassname = retclassname.Trim();
                        break;
                    }
                }
            }
            if (retclassname != null && retnamespace != null)
            {
                string namespaceLink = retnamespace.Replace(RootNamespace, "");
                namespaceLink = namespaceLink.Replace(".", " ");
                namespaceLink = namespaceLink.Trim().Replace(" ", "\\");
                return namespaceLink + "\\" + retclassname;
            }
            return null;
        }

        protected void OnResponceFileUpdate(object sender, CachedWriterEventArgs e)
        {
            if (e != null)
            {
                Log.Status.WriteLine("{0}{1} response file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
            }
        }

        protected virtual string CreateResponseFileIfNeeded(Module_DotNet module)
        {
            string responsefile = null;

            if (!String.IsNullOrWhiteSpace(module.Compiler.AdditionalArguments))
            {
                // we need to write response file to contain these additional command line arguments.
                using (IMakeWriter writer = new MakeWriter(writeBOM: false))
                {
                    writer.FileName = responsefile = Path.Combine(Package.PackageConfigBuildDir.Path, Name + "_" + module.Configuration.Name + ".rsp");

                    writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnResponceFileUpdate);

                    writer.WriteLine(module.Compiler.AdditionalArguments);
                }
            }

            return responsefile;
        }

        protected void ComputeCodeBehindCache(IEnumerable<FileEntry> fentries, out IDictionary<string, string> srcHash, out IDictionary<string, string> resHash, out IDictionary<string, string> xamlHash, out IDictionary<string, string> xomlHash)
        {
            srcHash = new Dictionary<string, string>();
            resHash = new Dictionary<string, string>();
            xamlHash = new Dictionary<string, string>();
            xomlHash = new Dictionary<string, string>();

            foreach (var fe in fentries)
            {
                var ext = Path.GetExtension(fe.Path.Path).ToLowerInvariant();
                var relPathKey = PathUtil.RelativePath(fe.Path.Path.TrimExtension(), OutputDir.Path);

                var firstFe = fe.ConfigEntries.First();

                if (firstFe.IsKindOf(Source))
                {
                    if (ext == ".xaml")
                    {
                        xamlHash.Add(relPathKey, ext);

                        firstFe.ClearType(Source);
                        firstFe.SetType(Xaml);
                    }
                    else if (ext == ".xoml")
                    {
                        xamlHash.Add(relPathKey, ext);
                        firstFe.ClearType(Source);
                        firstFe.SetType(Xoml);
                    }
                    else
                    {
                        srcHash.Add(relPathKey, ext);
                    }
                }
                else if (firstFe.IsKindOf(Resource))
                {
                    string val = ext;
                    if (resHash.TryGetValue(relPathKey, out val))
                    {
                        val += "\n" + ext;
                    }
                    else
                    {
                        val = ext;
                    }
                    resHash[relPathKey] = val;
                }
            }
        }

        protected virtual void WriteBootstrapperPackageInfo(IXmlWriter writer)
        {
            /*
            writer.WriteStartElement("ItemGroup");
            writer.WriteStartElement("BootstrapperPackage");
            Include="Microsoft.Net.Framework.2.0">
      <Visible>False</Visible>
      <ProductName>.NET Framework 2.0 %28x86%29</ProductName>
      <Install>true</Install>
    </BootstrapperPackage>
    <BootstrapperPackage Include="Microsoft.Net.Framework.3.0">
      <Visible>False</Visible>
      <ProductName>.NET Framework 3.0 %28x86%29</ProductName>
      <Install>false</Install>
    </BootstrapperPackage>
    <BootstrapperPackage Include="Microsoft.Net.Framework.3.5">
      <Visible>False</Visible>
      <ProductName>.NET Framework 3.5</ProductName>
      <Install>false</Install>
    </BootstrapperPackage>
  </ItemGroup>
            */

        }

        protected virtual void WriteImportStandardTargets(IXmlWriter writer)
        {
            writer.WriteStartElement("Import");
            writer.WriteAttributeString("Project", @"$(MSBuildBinPath)\Microsoft.CSharp.targets");
            writer.WriteEndElement(); //Import
        }

        protected virtual void WriteImportTargets(IXmlWriter writer)
        {
            WriteImportStandardTargets(writer);

            foreach (string importProject in StartupModule.ImportMSBuildProjects.ToArray())
            {
                writer.WriteStartElement("Import");
                writer.WriteAttributeString("Project", importProject.TrimQuotes());
                writer.WriteEndElement(); //Import
            }

            if (StartupModule.IsProjectType(DotNetProjectTypes.WebApp))
            {
                writer.WriteStartElement("Import");
                writer.WriteAttributeString("Project", String.Format(@"$(MSBuildExtensionsPath32)\Microsoft\VisualStudio\v10.0\WebApplications\Microsoft.WebApplication.targets"));
                writer.WriteEndElement(); //Import

                writer.WriteStartElement("ProjectExtensions");
                writer.WriteStartElement("VisualStudio");
                writer.WriteStartElement("FlavorProperties");
                writer.WriteAttributeString("GUID", "{349c5851-65df-11da-9384-00065b846f21}");
                writer.WriteStartElement("WebProjectProperties");

                writer.WriteElementString("UseIIS", "False");
                writer.WriteElementString("AutoAssignPort", "True");
                writer.WriteElementString("DevelopmentServerPort", "53813");
                writer.WriteElementString("DevelopmentServerVPath", "/");
                writer.WriteElementString("IISUrl", "\n");
                writer.WriteElementString("NTLMAuthentication", "False");
                writer.WriteElementString("UseCustomServer", "False");
                writer.WriteElementString("CustomServerUrl", "\n");
                writer.WriteElementString("SaveServerSettingsInUserFile", "False");

                writer.WriteEndElement(); //WebProjectProperties 
                writer.WriteEndElement(); //FlavorProperties 
                writer.WriteEndElement(); //VisualStudio
                writer.WriteEndElement(); //ProjectExtensions
            }

            if (StartupModule.IsProjectType(DotNetProjectTypes.Workflow))
            {
                writer.WriteStartElement("Import");
                writer.WriteAttributeString("Project", String.Format(@"$(MSBuildExtensionsPath)\Microsoft\Windows Workflow Foundation\{0}\Workflow.Targets", TargetFrameworkVersion));
                writer.WriteEndElement(); //Import
            }

            if (EnableFxCop())
            {
                var customdictionary = Package.Project.Properties.GetPropertyOrDefault(StartupModule.GroupName + ".fxcop.customdictionary",
                                        Package.Project.Properties.GetPropertyOrDefault("package.fxcop.customdictionary", String.Empty)).TrimWhiteSpace();
                
                if (!String.IsNullOrEmpty(customdictionary))
                {
                    customdictionary = PathNormalizer.Normalize(customdictionary);

                    writer.WriteStartElement("ItemGroup");
                    writer.WriteStartElement("CodeAnalysisDictionary");
                    writer.WriteAttributeString("Include",  GetProjectPath(customdictionary));

                    writer.WriteElementString("Link", Path.GetFileName(customdictionary));

                    writer.WriteEndElement();//CodeAnalysisDictionary
                    writer.WriteEndElement();//ItemGroup
                }
            }

            if (EnableStyleCop())
            {
                TaskUtil.Dependent(StartupModule.Package.Project, "StyleCop", Project.TargetStyleType.Use);

                writer.WriteStartElement("Import");
                writer.WriteAttributeString("Project", StartupModule.Package.Project.Properties["package.StyleCop.MSBuildTargets"]);
                writer.WriteEndElement(); //Import
            }
        }

        private void AddCopyWithAttributesTask(IXmlWriter writer)
        {
            var taskPath = Path.GetFullPath(Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, @"data\FrameworkMsbuild.tasks"));
            writer.WriteStartElement("Import");
            writer.WriteAttributeString("Project", GetProjectPath(taskPath));
            writer.WriteEndElement(); //Import
        }

        protected void WriteResourceMappingTargets(IXmlWriter writer)
        {
            if (_resourceMapping != null && _resourceMapping.Count > 0)
            {
                AddCopyWithAttributesTask(writer);

                foreach (var group in _resourceMapping.Values.GroupBy(map => map.SourceFile.ConfigSignature, map => map))
                {
                    if (group.Count() > 0)
                    {

                        writer.WriteStartElement("Target");
                        writer.WriteAttributeString("Name", "BeforeBuild");
                        WriteConfigCondition(writer, group.First().SourceFile.ConfigEntries.Select(ce => ce.Configuration));

                        foreach (var map in group)
                        {
                            writer.WriteStartElement("CopyWithAttributes");
                            writer.WriteAttributeString("SourceFiles", GetProjectPath(map.SourceFile.Path));
                            writer.WriteAttributeString("DestinationFiles", GetProjectPath(map.TargetFile));
                            writer.WriteAttributeString("SkipUnchangedFiles", "true");
                            writer.WriteAttributeString("UseHardlinksIfPossible", "true");
                            writer.WriteAttributeString("ProjectPath", "$(MSBuildProjectDirectory)");
                            writer.WriteEndElement(); //Copy
                        }

                        writer.WriteStartElement("ItemGroup");
                        WriteConfigCondition(writer, group.First().SourceFile.ConfigEntries.Select(ce => ce.Configuration));
                        foreach (var map in group)
                        {
                            if (map.FileElement != null)
                            {
                                writer.WriteStartElement(map.FileElement);
                                writer.WriteAttributeString("Include", GetProjectPath(map.TargetFile));
                                foreach (var el in map.Elements)
                                {
                                    writer.WriteElementString(el.Item1, el.Item2);
                                }
                                writer.WriteEndElement(); //map.FileElement
                            }
                        }
                        writer.WriteEndElement(); //ItemGroup
                        writer.WriteEndElement(); //Target


                        writer.WriteStartElement("Target");
                        writer.WriteAttributeString("Name", "BeforeClean");
                        WriteConfigCondition(writer, group.First().SourceFile.ConfigEntries.Select(ce => ce.Configuration));

                        foreach (var map in group)
                        {
                            writer.WriteStartElement("Delete");
                            writer.WriteAttributeString("Files", GetProjectPath(map.TargetFile));
                            writer.WriteAttributeString("TreatErrorsAsWarnings", "true");
                            writer.WriteEndElement(); //Copy
                        }
                        writer.WriteEndElement(); //Target
                    }
                }
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

                var userFileEl = userFileDoc.GetOrAddElement("Project", Xmlns);
                userFileEl.SetAttribute("ToolsVersion", ToolsVersion);

                foreach (ProcessableModule module in Modules)
                {
                    WriteUserFileConfiguration(userFileEl, module);
                }

                userFileDoc.Save(userFilePath);
            }
        }

        protected void WriteUserFileConfiguration(XmlNode rootEl, ProcessableModule module)
        {
            if (module.AdditionalData != null && module.AdditionalData.DebugData != null)
            {
                var configCondition = String.Format("'$(Configuration)|$(Platform)' == '{0}'", GetVSProjConfigurationName(module.Configuration));
                var propertyGroupEl = rootEl.GetOrAddElementWithAttributes("PropertyGroup", "Condition", configCondition);

                if (module.AdditionalData.DebugData.EnableUnmanagedDebugging)
                {
                    var unmanagedDebuggingEl = propertyGroupEl.GetOrAddElement("EnableUnmanagedDebugging");
                    unmanagedDebuggingEl.InnerText = module.AdditionalData.DebugData.EnableUnmanagedDebugging.ToString();
                }

                var workingDirEl = propertyGroupEl.GetOrAddElement("StartWorkingDirectory");
                workingDirEl.InnerText = GetProjectPath(module.AdditionalData.DebugData.Command.WorkingDir);

                if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.RemoteMachine))
                {
                    var remoteDebugEnabledEl = propertyGroupEl.GetOrAddElement("RemoteDebugEnabled");
                    remoteDebugEnabledEl.InnerText = "true";

                    var remoteDebugMachineEl = propertyGroupEl.GetOrAddElement("RemoteDebugMachine");
                    remoteDebugMachineEl.InnerText = module.AdditionalData.DebugData.RemoteMachine;
                }

                var startArgsEl = propertyGroupEl.GetOrAddElement("StartArguments");
                startArgsEl.InnerText = module.AdditionalData.DebugData.Command.Options.ToString(" ");

                if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.Command.Executable.Path))
                {
                    var startActionEl = propertyGroupEl.GetOrAddElement("StartAction");
                    startActionEl.InnerText = "Program";

                    var startProgramEl = propertyGroupEl.GetOrAddElement("StartProgram");
                    startProgramEl.InnerText = GetProjectPath(module.AdditionalData.DebugData.Command.Executable);
                }
                else
                {
                    var startActionEl = propertyGroupEl.GetOrAddElement("StartAction");
                    startActionEl.InnerText = "Project";
                }
            }
        }

        #endregion Write Methods

        protected IEnumerable<IDotNetVisualStudioExtension> GetExtensionTasks(ProcessableModule module)
        {
            List<IDotNetVisualStudioExtension> extensions = new List<IDotNetVisualStudioExtension>();
            if (module == null)
            {
                return extensions;
            }
            // Dynamically created packages may use exising projects from different packages. Skip such projects
            if (module.Configuration.Name != module.Package.Project.Properties["config"])
            {
                return extensions;
            }

            var extensionTaskNames = new List<string>() { module.Configuration.Platform + "-visualstudio-extension" };

            extensionTaskNames.AddRange(module.Package.Project.Properties[module.Configuration.Platform + "-visualstudio-extension"].ToArray());

            extensionTaskNames.AddRange(module.Package.Project.Properties[module.GroupName + ".visualstudio-extension"].ToArray());

            foreach (var extensionTaskName in extensionTaskNames)
            {
                var task = module.Package.Project.TaskFactory.CreateTask(extensionTaskName, module.Package.Project);
                var extension = task as IDotNetVisualStudioExtension;
                if (extension != null)
                {
                    extension.Module = module;
                    extension.Generator = new IDotNetVisualStudioExtension.DotNetProjectGenerator(this);
                    extensions.Add(extension);
                }
                else if (task != null)
                {
                    Log.Warning.WriteLine("Visual Studio generator extension Task '{0}' does not implement IVisualStudioExtension. Task is ignored.", extensionTaskName);
                }
                
            }
            return extensions;
        }

        protected virtual DotNetCompiler.Targets OutputType
        {
            get
            {
                var targets = Modules.Cast<Module_DotNet>().Select(m => m.Compiler.Target).Distinct();

                if (targets.Count() > 1)
                {
                    throw new BuildException(String.Format("DotNet Module '{0}' has different targets ({1}) defined in different configurations. VisualStudio does not support this feature.", ModuleName, targets.ToString(", ", t => t.ToString())));
                }
                return targets.First();
            }
        }

        protected void WriteConfigCondition(IXmlWriter writer, IEnumerable<FileEntry> files)
        {
            var fe = files.FirstOrDefault();
            if(fe != null)
            {
                WriteConfigCondition(writer, fe.ConfigEntries.Select(ce => ce.Configuration));
            }
        }

        protected void WriteConfigCondition(IXmlWriter writer, IEnumerable<Configuration> configs)
        {
            writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(configs));
        }

        internal string GetConfigCondition(Configuration config)
        {
            return " '$(Configuration)|$(Platform)' == '" + GetVSProjConfigurationName(config) + "' ";
        }

        protected string GetConfigCondition(IEnumerable<Configuration> configs)
        {
            var condition = MSBuildConfigConditions.GetCondition(configs, AllConfigurations);

            return MSBuildConfigConditions.FormatCondition("$(Configuration)|$(Platform)", condition, GetVSProjConfigurationName);
        }

        protected virtual string GetLinkPath(FileEntry fe)
        {
            string link_path = String.Empty;

            if (!String.IsNullOrEmpty(fe.FileSetBaseDir) && fe.Path.Path.StartsWith(fe.FileSetBaseDir))
            {
                link_path = PathUtil.RelativePath(fe.Path.Path, fe.FileSetBaseDir).TrimStart(new char[] { '/', '\\', '.' });
            }
            else
            {
                link_path = PathUtil.RelativePath(fe.Path.Path, OutputDir.Path).TrimStart(new char[] { '/', '\\', '.' });
            }

            if (!String.IsNullOrEmpty(link_path))
            {
                if (Path.IsPathRooted(link_path) || (link_path.Length > 1 && link_path[1] == ':'))
                {
                    link_path = link_path[0] + "_Drive" + link_path.Substring(2);
                }
            }

            return link_path;
        }

        protected virtual string GetConfigPlatformMapping(Module_DotNet module)
        {
           var platform = module.Compiler.GetOptionValue("platform:");

            switch(platform)
            {            
                case "x86": return "Win32";
                case "x64": return "x64";
                default:
                case "anycpu": return "AnyCPU";
            }
        }

        protected virtual IEnumerable<BuildstepGroup> GroupBuildSteps()
        {
            var groups = new Dictionary<string, BuildstepGroup>();

            foreach (var module in Modules.Cast<Module>())
            {
                if (module.BuildSteps != null && module.BuildSteps.Count > 0)
                {
                    var key = module.BuildSteps.Aggregate(new StringBuilder(), (k, bs) => bs.Commands.Aggregate(k, (k1, cm) => k1.AppendLine(cm.CommandLine))).ToString();

                    BuildstepGroup bsgroup;

                    if (!groups.TryGetValue(key, out bsgroup))
                    {
                        bsgroup = new BuildstepGroup(module.BuildSteps);
                        groups.Add(key, bsgroup);
                    }
                    if (!bsgroup.Configurations.Contains(module.Configuration))
                    {
                        bsgroup.Configurations.Add(module.Configuration);
                    }
                }
            }

            return groups.Values;
        }

        protected class BuildstepGroup
        {
            internal BuildstepGroup(IEnumerable<BuildStep> buildsteps)
            {
                BuildSteps = buildsteps;
                Configurations = new List<Configuration>();
            }
            internal readonly IEnumerable<BuildStep> BuildSteps;
            internal List<Configuration> Configurations;
            internal string ConfigSignature
            {
                get
                {
                    if (_configSignature == null)
                    {
                        _configSignature = Configurations.Aggregate(new StringBuilder(), (s, c) => s.AppendLine(c.Name)).ToString();
                    }
                    return _configSignature;
                }
            }
            private string _configSignature;
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

        protected virtual void GetReferences(out IEnumerable<ProjectRefEntry> projectReferences, out IDictionary<PathString, FileEntry> references)
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
                    uint copyLocal = 0;
                    if ((module as Module_DotNet).CopyLocal != CopyLocalType.False)
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

                            if (copyLocal != ProjectRefEntry.CopyLocal)
                            {
                                //Check if copylocal is defined in module dependents.
                                var dep = module.Dependents.FirstOrDefault(d=> d.Dependent.Name == depProject.ModuleName && d.Dependent.Package.Name == depProject.Package.Name && d.Dependent.BuildGroup == depProject.Modules.FirstOrDefault().BuildGroup);
                                if (dep != null && dep.IsKindOf(DependencyTypes.CopyLocal))
                                {
                                    copyLocal = ProjectRefEntry.CopyLocal;
                                }
                            }

                            refEntry.TryAddConfigEntry(module.Configuration, copyLocal);
                        }
                    }
                }
            }

            references = new Dictionary<PathString, FileEntry>();

            foreach (var module in Modules.Cast<Module_DotNet>())
            {
                ISet<PathString> assemblies = null;
                projectReferencedAssemblies.TryGetValue(module.Configuration, out assemblies);

                foreach (var assembly in GetDefaultSystemAssemblies(module))
                {
                    var key = new PathString(assembly);
                    UpdateFileEntry(references, key, new FileItem(key, asIs: true), String.Empty, module.Configuration, flags: 0);
                }


                foreach (var assembly in module.Compiler.Assemblies.FileItems)
                {
                    // Exclude any assembly that is already included through project references:
                    if (assemblies != null && assemblies.Contains(assembly.Path))
                    {
                        continue;
                    }

                    uint copyLocal = 0;
                    if (module.CopyLocal == CopyLocalType.True && !assembly.AsIs)
                    {
                        copyLocal = FileEntry.CopyLocal;
                    }

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
                    UpdateFileEntry(references, key, assembly, module.Compiler.Assemblies.BaseDirectory, module.Configuration, flags: copyLocal);
                }

                foreach (var assembly in module.Compiler.DependentAssemblies.FileItems)
                {
                    // Exclude any assembly that is already included through project references:
                    if (assemblies != null && assemblies.Contains(assembly.Path))
                    {
                        continue;
                    }

                    uint copyLocal = 0;
                    if ((module.CopyLocal == CopyLocalType.True || module.CopyLocal == CopyLocalType.Slim) && !assembly.AsIs)
                    {
                        copyLocal = FileEntry.CopyLocal;
                    }

                    var key = new PathString(Path.GetFileName(assembly.Path.Path));
                    UpdateFileEntry(references, key, assembly, module.Compiler.DependentAssemblies.BaseDirectory, module.Configuration, flags: copyLocal);
                }
            }

            //IMTODO: I think this should be removed. Can't be Module_Native in .Net project
            foreach (var module in Modules.Where(m => m is Module_Native).Cast<Module_Native>())
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
            }
 
        }

        protected virtual IEnumerable<string> GetDefaultSystemAssemblies(Module_DotNet module)
        {
            // First add some system assemblies:
            var defaultSystemAssemblies = new List<string>() { "System.dll", "System.Data.Dll" };

            if (module.IsProjectType(DotNetProjectTypes.UnitTest))
            {
                defaultSystemAssemblies.Add("Microsoft.VisualStudio.QualityTools.UnitTestFramework.dll");
            }
            if (module.IsProjectType(DotNetProjectTypes.WebApp))
            {
                defaultSystemAssemblies.Add("System.Core.dll");
                defaultSystemAssemblies.Add("System.Configuration.dll");
                defaultSystemAssemblies.Add("System.EnterpriseServices.dll");
                defaultSystemAssemblies.Add("System.Data.DataSetExtensions.dll");
                defaultSystemAssemblies.Add("System.Web.dll");
                defaultSystemAssemblies.Add("System.Web.ApplicationServices.dll");
                defaultSystemAssemblies.Add("System.Web.DynamicData.dll");
                defaultSystemAssemblies.Add("System.Web.Entity.dll");
                defaultSystemAssemblies.Add("System.Web.Extensions.dll");
                defaultSystemAssemblies.Add("System.Web.Services.dll");
                defaultSystemAssemblies.Add("System.Xml.Linq.dll");
                defaultSystemAssemblies.Add("Microsoft.CSharp.dll");
            }
            if (module.IsProjectType(DotNetProjectTypes.Workflow))
            {
                defaultSystemAssemblies.Add("System.Core.dll");
                defaultSystemAssemblies.Add("System.WorkflowServices.dll");
                defaultSystemAssemblies.Add("System.Workflow.Activities.dll");
                defaultSystemAssemblies.Add("System.Workflow.ComponentModel.dll");
                defaultSystemAssemblies.Add("System.Workflow.Runtime.dll");
            }
            return defaultSystemAssemblies;
        }

        protected virtual void PrepareSubTypeMappings(IXmlWriter writer)
        {
            if (File.Exists(writer.FileName))
            {
                try
                {
                    XmlDocument doc = new XmlDocument();
                    doc.Load(writer.FileName);

                    foreach (XmlNode itemGroup in doc.DocumentElement.ChildNodes)
                    {
                        if (!(itemGroup.NodeType == XmlNodeType.Element && itemGroup.Name == "ItemGroup"))
                        {
                            continue;
                        }

                        foreach (XmlNode tool in itemGroup.ChildNodes)
                        {
                            if (itemGroup.NodeType == XmlNodeType.Element)
                            {
                                var includeAttr = tool.Attributes["Include"];
                                string subType = null;
                                foreach (XmlNode snode in tool.ChildNodes)
                                {
                                    if (snode.NodeType == XmlNodeType.Element)
                                    {
                                        if (snode.Name == "SubType")
                                        {
                                            subType = snode.InnerText.TrimWhiteSpace();
                                            break;
                                        }
                                    }
                                }

                                if (includeAttr != null && !String.IsNullOrEmpty(includeAttr.Value))
                                {
                                    if (!String.IsNullOrEmpty(subType))
                                    {
                                        _savedCsprojData.AddSubTypeMapping(tool.Name, includeAttr.Value, subType);
                                    }
                                    else
                                    {
                                        _savedCsprojData.AddSubTypeMapping(tool.Name, includeAttr.Value, String.Empty);
                                    }
                                }
                            }
                        }

                    }
                }
                catch (Exception ex)
                {
                    Log.Debug.WriteLine(LogPrefix+"Failed to load existing project file '{0}', reason {1}", writer.FileName, ex.Message);
                }
            }
        }

        protected virtual ResourceMappingData PrepareResourceMapping(FileEntry fe, ref string elementName)
        {
            ResourceMappingData data = null;
            if (_resourceMapping!= null && !fe.Path.IsPathInDirectory(OutputDir))
            {
                // The source is outside the csproj directory.  Link to it, based on the baseDir of the fileSet
                if (fe.FileSetBaseDir != null && fe.Path.Path.StartsWith(fe.FileSetBaseDir))
                {
                    string relFileToBaseDir = fe.Path.Path.Substring(fe.FileSetBaseDir.Length + 1);
                    string targetfile = Path.Combine(OutputDir.Path, relFileToBaseDir);

                    if (!_resourceMapping.TryGetValue(fe.Path.Path, out data))
                    {
                        data = new ResourceMappingData(fe, targetfile, elementName);
                        _resourceMapping.Add(fe.Path.Path, data);
                    }
                    elementName = "None";
                }
            }
            return data;
        }

        protected bool EnableStyleCop(IModule module = null)
        {
            if (module == null)
            {
                foreach (var mod in Modules)
                {
                    if (EnableStyleCop(mod))
                    {
                        return true;
                    }
                }
                return false;
            }

            return module.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".stylecop",
                    module.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".csproj.stylecop",
                    module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.stylecop.enable", false)));
        }

        protected bool EnableFxCop(IModule module = null)
        {
            if (module == null)
            {
                foreach (var mod in Modules)
                {
                    if (EnableFxCop(mod))
                    {
                        return true;
                    }
                }
                return false;
            }
            return module.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".fxcop",
                   module.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".csproj.fxcop",
                   module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.fxcop.enable", false)));
        }

        protected override IXmlWriterFormat ProjectFileWriterFormat
        {
            get { return _xmlWriterFormat; }
        }

        private static readonly IXmlWriterFormat _xmlWriterFormat = new XmlFormat(
            identChar: ' ',
            identation: 2,
            newLineOnAttributes: false,
            encoding: new UTF8Encoding(false) // no byte order mask!
            );

        protected class ResourceMappingData
        {
            public ResourceMappingData(FileEntry srcFile, string tgtFile, string fileElement)
            {
                SourceFile = srcFile;
                TargetFile = tgtFile;
                FileElement = fileElement;
            }
            public readonly FileEntry SourceFile;
            public readonly string TargetFile;
            public readonly string FileElement;
            public List<Tuple<string,string>> Elements = new List<Tuple<string,string>>();
        }

        protected class SavedCsprojData
        {
            public SavedCsprojData()
            {
                _subTypeMappings = new Dictionary<string, string>();
            }

            public void AddSubTypeMapping(string Element, string include, string subType)
            {
                var key = Element + ";#;" + include;

                _subTypeMappings[key] = subType;
            }

            public string GetSubTypeMapping(string Element, string include, string defaultSubType, bool allowRemove=false)
            {
                string subType;
                var key = Element + ";#;" + include;

                if (!_subTypeMappings.TryGetValue(key, out subType))
                {
                    subType = defaultSubType;
                }
                else if(String.IsNullOrEmpty(subType))
                {
                    subType = allowRemove? String.Empty : defaultSubType;
                }
                return subType;
            }


            private readonly IDictionary<string, string> _subTypeMappings;
        }

        protected IDictionary<string, ResourceMappingData> _resourceMapping;
        protected readonly SavedCsprojData _savedCsprojData = new SavedCsprojData();
    }
}
