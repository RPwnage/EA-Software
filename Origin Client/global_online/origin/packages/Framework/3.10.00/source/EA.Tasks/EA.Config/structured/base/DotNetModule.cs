using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using EA.FrameworkTasks;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Linq;

using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Structured
{
    /// <summary>A module buildable by the dot net framework</summary>
    public class DotNetModuleTask : ModuleBaseTask
    {
        public readonly string CompilerName;
        private readonly string _prefix;

        protected DotNetModuleTask()
            : this(String.Empty, "csc")
        {
        }

        protected DotNetModuleTask(string buildtype, string compiler)
            : base(buildtype)
        {
            CompilerName = compiler;
            _config = new DotNetConfigElement(this);
            _buildSteps = new BuildStepsElement(this);
            mCombinedState = new CombinedStateData();

            if (CompilerName == "csc")
            {
                _prefix = "csproj";
            }
            else if (CompilerName == "fsc")
            {
                _prefix = "fsproj";
            }
            else
            {
                Error.Throw(Project, Location, "DotNetModule", "Invalid compiler parameter");
            }
        }

        /// <summary>Overrides the default framework directory where built files are located.</summary>
        [TaskAttribute("outputdir")]
        public String OutputDir
        {
            get { return _outputdir; }
            set { _outputdir = value; }
        } String _outputdir = String.Empty;

        /// <summary>Overrides the defualt name of built files.</summary>
        [TaskAttribute("outputname")]
        public String OutputName
        {
            get { return _outputname; }
            set { _outputname = value; }
        } String _outputname = String.Empty;

        /// <summary>Is this a Workflow module.</summary>
        [TaskAttribute("workflow")]
        public bool Workflow
        {
            get { return _workflow; }
            set { _workflow = value; }
        } bool _workflow = false;

        /// <summary>Is this a UnitTest module.</summary>
        [TaskAttribute("unittest")]
        public bool UnitTest
        {
            get { return _unittest; }
            set { _unittest = value; }
        } bool _unittest = false;

        /// <summary>Indicates this is a web application project and enables web debugging.</summary>
        [TaskAttribute("webapp")]
        public bool WebApp
        {
            get { return _webapp; }
            set { _webapp = value; }
        } bool _webapp = false;

        /// <summary>Specifies the Rootnamespace for a visual studio project</summary>
        [Property("rootnamespace", Required = false)]
        public ConditionalPropertyElement RootNamespace
        {
            get { return _rootnamespaceelem; }
            set { _rootnamespaceelem = value; }
        }private ConditionalPropertyElement _rootnamespaceelem = new ConditionalPropertyElement();

        /// <summary>Specifies the location of the Application manifest</summary>
        [Property("applicationmanifest", Required = false)]
        public ConditionalPropertyElement ApplicationManifest
        {
            get { return _applicationmanifestelem; }
            set { _applicationmanifestelem = value; }
        }private ConditionalPropertyElement _applicationmanifestelem = new ConditionalPropertyElement();

        /// <summary>Specifies the name of the App designer folder</summary>
        [Property("appdesignerfolder", Required = false)]
        public ConditionalPropertyElement AppdesignerFolder
        {
            get { return _appdesignerfolderelem; }
            set { _appdesignerfolderelem = value; }
        }private ConditionalPropertyElement _appdesignerfolderelem = new ConditionalPropertyElement();

        /// <summary>used to enable/disable Visual Studio hosting process during debugging</summary>
        [Property("disablevshosting", Required = false)]
        public ConditionalPropertyElement DisableVsHosting
        {
            get { return _disablevshostingelem; }
            set { _disablevshostingelem = value; }
        }private ConditionalPropertyElement _disablevshostingelem = new ConditionalPropertyElement();

        /// <summary>Additional MSBuild project imports</summary>
        [Property("importmsbuildprojects", Required = false)]
        public ConditionalPropertyElement ImportMSBuildProjects
        {
            get { return _importmsbuildprojectselem; }
            set { _importmsbuildprojectselem = value; }
        }private ConditionalPropertyElement _importmsbuildprojectselem = new ConditionalPropertyElement();

        /// <summary>Postbuild event run condition</summary>
        [Property("runpostbuildevent", Required = false)]
        public ConditionalPropertyElement RunPostbuildEvent
        {
            get { return _runpostbuildeventelem; }
            set { _runpostbuildeventelem = value; }
        }private ConditionalPropertyElement _runpostbuildeventelem = new ConditionalPropertyElement();

        /// <summary>The location of the Application Icon file</summary>
        [Property("applicationicon", Required = false)]
        public ConditionalPropertyElement ApplicationIcon
        {
            get { return _applicationiconelem; }
            set { _applicationiconelem = value; }
        }private ConditionalPropertyElement _applicationiconelem = new ConditionalPropertyElement();

        /// <summary>property enables/disables generation of XML documentation files</summary>
        [Property("generatedoc", Required = false)]
        public ConditionalPropertyElement GenerateDoc
        {
            get { return _generatedocelem; }
            set { _generatedocelem = value; }
        }private ConditionalPropertyElement _generatedocelem = new ConditionalPropertyElement();

        /// <summary>Key File</summary>
        [Property("keyfile", Required = false)]
        public ConditionalPropertyElement KeyFile
        {
            get { return _keyfileelem; }
            set { _keyfileelem = value; }
        }private ConditionalPropertyElement _keyfileelem = new ConditionalPropertyElement();

        [Property("copylocal-dependencies", Required = false)]
        public ConditionalPropertyElement CopyLocalDependencies
        {
            get { return _copylocalDependelem; }
            set { _copylocalDependelem = value; }
        }private ConditionalPropertyElement _copylocalDependelem = new ConditionalPropertyElement();

        /// <summary>Adds the list of sourcefiles</summary>
        [FileSet("sourcefiles", Required = false)]
        public StructuredFileSet SourceFiles
        {
            get { return _sourcefiles; }

        }private StructuredFileSet _sourcefiles = new StructuredFileSet();

        /// <summary>A list of referenced assemblies for this module</summary>
        [FileSet("assemblies", Required = false)]
        public StructuredFileSet Assemblies
        {
            get { return _assemblies; }

        }private StructuredFileSet _assemblies = new StructuredFileSet();

        /// <summary>Adds a list of resource files</summary>
        [FileSet("resourcefiles", Required = false)]
        public ResourceFilesElement ResourceFiles
        {
            get { return _resourcefiles; }

        }private ResourceFilesElement _resourcefiles = new ResourceFilesElement();

        /// <summary>Adds a list of resource files</summary>
        [FileSet("nonembed-resourcefiles", Required = false)]
        public StructuredFileSet ResourceFilesNonEmbed
        {
            get { return _resourcefilesNonEmbed; }

        }private StructuredFileSet _resourcefilesNonEmbed = new StructuredFileSet();

        /// <summary>Adds a list of 'Content' files</summary>
        [FileSet("contentfiles", Required = false)]
        public StructuredFileSet ContentFiles
        {
            get { return _contentfiles; }

        }private StructuredFileSet _contentfiles = new StructuredFileSet();

        /// <summary>A list of webreferences for this module</summary>
        [OptionSet("webreferences", Required = false)]
        public StructuredOptionSet WebReferences
        {
            get { return _webreferences; }
            set { _webreferences = value; }
        }private StructuredOptionSet _webreferences = new StructuredOptionSet();

        /// <summary>Adds the list of modules</summary>
        [FileSet("modules", Required = false)]
        public StructuredFileSet Modules
        {
            get { return _modules; }

        }private StructuredFileSet _modules = new StructuredFileSet();

        /// <summary>A list of COM assemblies for this module</summary>
        [FileSet("comassemblies", Required = false)]
        public StructuredFileSet ComAssemblies
        {
            get { return _comassemblies; }
            set { _comassemblies = value; }
        }private StructuredFileSet _comassemblies = new StructuredFileSet();

        /// <summary>Sets the configuration for a project</summary>
        [BuildElement("config", Required = false)]
        public DotNetConfigElement Config
        {
            get { return _config; }
        }private DotNetConfigElement _config;

        /// <summary>Sets the build steps for a project</summary>
        [BuildElement("buildsteps", Required = false)]
        public BuildStepsElement BuildSteps
        {
            get { return _buildSteps; }
        }private BuildStepsElement _buildSteps;

        /// <summary></summary>
        [BuildElement("visualstudio", Required = false)]
        public VisualStudioDataElement VisualStudioData
        {
            get { return _visualstudioData; }
        }private VisualStudioDataElement _visualstudioData = new VisualStudioDataElement();

        /// <summary></summary>
        [BuildElement("platforms", Required = false)]
        public PlatformExtensions PlatformExtensions
        {
            get { return _platformExtensions; }
        }private PlatformExtensions _platformExtensions = new PlatformExtensions();

        /// <summary>Indicates this application supports Xbox Live Services. WinPhone only</summary>
        [Property("xbox-live-services", Required = false)]
        public ConditionalPropertyElement XbLiveServises
        {
            get { return _xbLiveServises; }
            set { _xbLiveServises = value; }
        }private ConditionalPropertyElement _xbLiveServises = new ConditionalPropertyElement();


        /// <summary>Adds a spa file for Xbox Live Services. WinPhone only</summary>
        [FileSet("xbox-live-services.spa", Required = false)]
        public StructuredFileSet SpaFile
        {
            get { return _spafile; }

        }private StructuredFileSet _spafile = new StructuredFileSet();


        protected override void SetupModule()
        {
            SetupConfig();

            SetupBuildData();

            SetupDependencies();

            SetupVisualStudio();

            PlatformExtensions.ExecutePlatformTasks(this);

            SetupAfterBuild();
        }

        protected override void SetBuildType()
        {
        }

        protected void SetupConfig()
        {
            string buildtype = GetModuleProperty("buildtype");
            _config.BuildOptions.InternalInitializeElement(buildtype);

            if (_config.buildOptionSet.Options.Count > 0)
            {
                string finalbuildtype = BuildType = (buildtype ?? BuildType) + "-" + ModuleName;

                _config.buildOptionSet.Options["buildset.name"] = finalbuildtype;
                SetModuleProperty("buildtype", finalbuildtype);

                Project.NamedOptionSets[finalbuildtype + "-temp"] = _config.buildOptionSet;

                GenerateBuildOptionset.Execute(Project, _config.buildOptionSet, finalbuildtype + "-temp");
            }

            base.SetBuildType();

            SetModuleProperty("defines", StringUtil.EnsureSeparator(_config.Defines.Value, Environment.NewLine), _config.Defines.Append);
            SetModuleProperty(CompilerName + "-args", _config.AdditionalOptions.Value, _config.AdditionalOptions.Append);

            SetModuleProperty("copylocal", _config.CopyLocal.Value);
            SetModuleProperty("platform", _config.Platform.Value);
            SetModuleProperty("copylocal.dependencies", CopyLocalDependencies.Value);
            SetModuleProperty("allowunsafe", _config.AllowUnsafe.Value);
            SetModuleProperty(FrameworkPrefix + ".targetframeworkversion", _config.TargetFrameworkVersion.Value);
            SetModuleProperty("generateserializationassemblies", _config.GenerateSerializationAssemblies.Value);
            SetModuleProperty("suppresswarnings", StringUtil.EnsureSeparator(_config.Suppresswarnings.Value, Environment.NewLine), _config.Suppresswarnings.Append);
            SetModuleProperty("warningsaserrors.list", StringUtil.EnsureSeparator(_config.WarningsaserrorsList.Value, Environment.NewLine), _config.WarningsaserrorsList.Append);
            SetModuleProperty("warningsaserrors", _config.Warningsaserrors.Value);

            SetModuleProperty("remove.defines", StringUtil.EnsureSeparator(_config.RemoveOptions.Defines.Value, Environment.NewLine), _config.RemoveOptions.Defines.Append);
            SetModuleProperty("remove." + CompilerName + ".options", StringUtil.EnsureSeparator(_config.RemoveOptions.Options.Value, Environment.NewLine), _config.RemoveOptions.Options.Append);
        }

        protected void SetupBuildData()
        {
            if (OutputDir != String.Empty)
            {
                SetModuleProperty("outputdir", OutputDir);
            }
            if (OutputName != String.Empty)
            {
                SetModuleProperty("outputname", OutputName);
            }

            SetModuleProperty(FrameworkPrefix + ".rootnamespace", RootNamespace);
            SetModuleProperty(FrameworkPrefix + ".application-manifest", ApplicationManifest);
            SetModuleProperty(FrameworkPrefix + ".appdesigner-folder", AppdesignerFolder);
            SetModuleProperty(FrameworkPrefix + ".disablevshosting", DisableVsHosting);
            SetModuleProperty(FrameworkPrefix + ".disablevshosting", ImportMSBuildProjects, ImportMSBuildProjects.Append);
            SetModuleProperty(FrameworkPrefix + ".runpostbuildevent", RunPostbuildEvent);

            if (UnitTest)
            {
                SetModuleProperty(FrameworkPrefix + ".unittest", "true");
            }

            if (WebApp)
            {
                SetModuleProperty(FrameworkPrefix + ".webapp", "true");
            }

            if (Workflow)
            {
                SetModuleProperty(FrameworkPrefix + ".workflow", "true");
            }

            SetModuleProperty("win32icon", ApplicationIcon);
            SetModuleProperty(CompilerName + "-doc", GenerateDoc);
            SetModuleProperty("keyfile", KeyFile);
            SetModuleFileset("sourcefiles", SourceFiles);
            SetModuleFileset("assemblies", Assemblies);
            SetModuleFileset("modules", Modules);
            SetModuleFileset("comassemblies", ComAssemblies);
            

            if (ResourceFiles.Includes.Count > 0 || ResourceFiles.Excludes.Count > 0)
            {
                SetModuleFileset("resourcefiles", ResourceFiles);
                SetModuleProperty("resourcefiles.prefix", GetValueOrDefault(ResourceFiles.Prefix, ModuleName));
                SetModuleProperty("resourcefiles.basedir", SourceFiles.BaseDirectory ?? ResourceFiles.ResourceBasedir);
            }
            if (ResourceFilesNonEmbed.Includes.Count > 0 || ResourceFilesNonEmbed.Excludes.Count > 0)
            {
                SetModuleFileset("resourcefiles.notembed", ResourceFilesNonEmbed);
            }

            if (ContentFiles.Includes.Count > 0 || ContentFiles.Excludes.Count > 0)
            {
                SetModuleFileset("contentfiles", ContentFiles);
            }


            SetModuleOptionSet("webreferences", WebReferences);

            SetModuleTarget(BuildSteps.PrebuildTarget, "prebuildtarget");
            SetModuleTarget(BuildSteps.PostbuildTarget, "postbuildtarget");
            SetModuleProperty(FrameworkPrefix + ".pre-build-step", BuildSteps.PrebuildTarget.Command);
            SetModuleProperty(FrameworkPrefix + ".post-build-step", BuildSteps.PostbuildTarget.Command);

            if (XbLiveServises.Value.ToBooleanOrDefault(false))
            {
                SetModuleProperty("xbox-live-services", "true");
                SetModuleFileset("xbox-live-services.spa", SpaFile);
            }

            SetModuleProperty("vcproj.custom-build-tool", BuildSteps.CustomBuildStep.Script.Value);
            SetModuleProperty("vcproj.custom-build-outputs", BuildSteps.CustomBuildStep.OutputDependencies.Value);
            SetModuleProperty("vcproj.custom-build-dependencies", BuildSteps.CustomBuildStep.InputDependencies.Value);
            SetModuleProperty("custom-build-before-targets", BuildSteps.CustomBuildStep.Before);
            SetModuleProperty("custom-build-after-targets", BuildSteps.CustomBuildStep.After);
        }

        protected void SetupVisualStudio()
        {

            SetModuleFileset(FrameworkPrefix + ".excludedbuildfiles", VisualStudioData.ExcludedBuildFiles);
            
            SetModuleTarget(VisualStudioData.PregenerateTarget, FrameworkPrefix + ".prebuildtarget");

            if(!String.IsNullOrEmpty(VisualStudioData.EnableUnmanaGeddebugging.Value.TrimWhiteSpace()))
            {
                var enableval = ConvertUtil.ToNullableBoolean(VisualStudioData.EnableUnmanaGeddebugging.Value.TrimWhiteSpace(), onerror: (val) => { throw new BuildException("enableunmanageddebugging element accepts only true or false values, value='" + val + "' is invalid", VisualStudioData.EnableUnmanaGeddebugging.Location); });

                SetModuleProperty("enableunmanageddebugging", enableval.ToString().ToLowerInvariant());
            }

        }

        protected void SetupAfterBuild()
        {

            SetModuleProperty("assetdependencies", BuildSteps.PackagingData.AssetDependencies);
            SetModuleFileset("manifestfile", BuildSteps.PackagingData.ManifestFile);
            // --- Run ---
            SetModuleProperty("workingdir", BuildSteps.RunData.WorkingDir);
            SetModuleProperty("commandargs", BuildSteps.RunData.Args);
            SetModuleProperty("commandprogram", BuildSteps.RunData.StartupProgram);
        }

        protected override void InitModule()
        {
        }

        protected override void FinalizeModule()
        {
        }

        private string FrameworkPrefix
        {
            get
            {
                return _prefix;
            }
        }


        internal class CombinedStateData
        {

            internal static void UpdateBoolean(ref bool val, bool? newval)
            {
                if (newval != null)
                {
                    val = (bool)newval;
                }
            }
        }
        internal CombinedStateData mCombinedState;

        /// <summary>Sets various attributes for a config</summary>
        [ElementName("config", StrictAttributeCheck = true)]
        public class DotNetConfigElement : Element
        {
            private ModuleBaseTask _module;

            public readonly OptionSet buildOptionSet = new OptionSet();

            public DotNetConfigElement(ModuleBaseTask module)
            {
                _module = module;

                _buildOptions = new BuildTypeElement(_module, buildOptionSet);
            }

            /// <summary>Gets the build options for this config.</summary>
            [BuildElement("buildoptions", Required = false)]
            public BuildTypeElement BuildOptions
            {
                get { return _buildOptions; }
            }private BuildTypeElement _buildOptions;

            /// <summary>Gets the macros defined for this config</summary>
            [Property("defines", Required = false)]
            public ConditionalPropertyElement Defines
            {
                get { return _defines; }
            }private ConditionalPropertyElement _defines = new ConditionalPropertyElement();

            /// <summary>Additional commandline options, new line separated (added to options defined through optionset</summary>
            [Property("additionaloptions", Required = false)]
            public ConditionalPropertyElement AdditionalOptions
            {
                get { return _additionaloptions; }
            }private ConditionalPropertyElement _additionaloptions = new ConditionalPropertyElement();

            /// <summary>Defines whether referenced assemblies are copied into the output folder of the module</summary>
            [Property("copylocal", Required = false)]
            public ConditionalPropertyElement CopyLocal
            {
                get { return _copylocalelem; }
                set { _copylocalelem = value; }
            }private ConditionalPropertyElement _copylocalelem = new ConditionalPropertyElement();

            /// <summary>Specifies the platform to build against</summary>
            [Property("platform", Required = false)]
            public ConditionalPropertyElement Platform
            {
                get { return _platform; }
                set { _platform = value; }
            }private ConditionalPropertyElement _platform = new ConditionalPropertyElement();

            [Property("allowunsafe", Required = false)]
            public ConditionalPropertyElement AllowUnsafe
            {
                get { return _allowUnsafeelem; }
                set { _allowUnsafeelem = value; }
            }private ConditionalPropertyElement _allowUnsafeelem = new ConditionalPropertyElement();

            /// <summary>used to define target .Net framework version</summary>
            [Property("targetframeworkversion", Required = false)]
            public ConditionalPropertyElement TargetFrameworkVersion
            {
                get { return _targetframeworkversionelem; }
                set { _targetframeworkversionelem = value; }
            }private ConditionalPropertyElement _targetframeworkversionelem = new ConditionalPropertyElement();

            /// <summary>Generate serialization assemblies:  None, Auto, On, Off</summary>
            [Property("generateserializationassemblies", Required = false)]
            public ConditionalPropertyElement GenerateSerializationAssemblies
            {
                get { return _generateserializationassemblieselem; }
                set { _generateserializationassemblieselem = value; }
            }private ConditionalPropertyElement _generateserializationassemblieselem = new ConditionalPropertyElement();

            /// <summary>Gets the warning suppression property for this config</summary>
            [Property("suppresswarnings", Required = false)]
            public ConditionalPropertyElement Suppresswarnings
            {
                get { return _suppresswarningselem; }
            }private ConditionalPropertyElement _suppresswarningselem = new ConditionalPropertyElement();

            /// <summary>List of warnings to treat as errors</summary>
            [Property("warningsaserrors.list", Required = false)]
            public ConditionalPropertyElement WarningsaserrorsList
            {
                get { return _warningsaserrorslistelem; }
            }private ConditionalPropertyElement _warningsaserrorslistelem = new ConditionalPropertyElement();

            /// <summary>Treat warnings as errors</summary>
            [Property("warningsaserrors", Required = false)]
            public ConditionalPropertyElement Warningsaserrors
            {
                get { return _warningsaserrorselem; }
            }private ConditionalPropertyElement _warningsaserrorselem = new ConditionalPropertyElement();

            /// <summary>Define options to removefrom the final optionset</summary>
            [Property("remove", Required = false)]
            public RemoveDotNetBuildOptions RemoveOptions
            {
                get { return _removeoptions; }
            }private RemoveDotNetBuildOptions _removeoptions = new RemoveDotNetBuildOptions();
        }

        /// <summary>Sets options to be removed from final configuration</summary>
        [ElementName("remove", StrictAttributeCheck = true)]
        public class RemoveDotNetBuildOptions : Element
        {
            /// <summary>Defines to be removed</summary>
            [Property("defines", Required = false)]
            public ConditionalPropertyElement Defines
            {
                get { return _defines; }
            }private ConditionalPropertyElement _defines = new ConditionalPropertyElement();

            /// <summary>Compiler options to be removed</summary>
            [Property("options", Required = false)]
            public ConditionalPropertyElement Options
            {
                get { return _options; }
            }private ConditionalPropertyElement _options = new ConditionalPropertyElement();
        }


        /// <summary></summary>
        [ElementName("VisualStudio", StrictAttributeCheck = true)]
        public class VisualStudioDataElement : Element
        {
            /// <summary></summary>
            [Property("pregenerate-target", Required = false)]
            public BuildTargetElement PregenerateTarget
            {
                get { return _pregenerateTarget; }
            }private BuildTargetElement _pregenerateTarget = new BuildTargetElement("vcproj.prebuildtarget");

            /// <summary>A list of files that are excluded from the build but are added to the visual studio
            /// project as non-buildable files</summary>
            [FileSet("excludedbuildfiles", Required = false)]
            public StructuredFileSet ExcludedBuildFiles
            {
                get { return _excludedbuildfiles; }
                set { _excludedbuildfiles = value; }
            }private StructuredFileSet _excludedbuildfiles = new StructuredFileSet();


            /// <summary></summary>
            [Property("enableunmanageddebugging", Required = false)]
            public ConditionalPropertyElement EnableUnmanaGeddebugging
            {
                get { return _enableunmanageddebugging; }
                set { _enableunmanageddebugging = value; }
            }private ConditionalPropertyElement _enableunmanageddebugging = new ConditionalPropertyElement();


            
        }
    }

}

