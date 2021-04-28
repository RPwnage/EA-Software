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
    /// <summary>A standard structured xml Module with user specified buildtype</summary>
    [TaskName("Module")]
    public class ModuleTask : ModuleBaseTask
    {
        public ModuleTask()
            : this(String.Empty)
        {
        }

        protected ModuleTask(string buildtype) : base(buildtype)
        {
            _config = new ConfigElement(this);
            _buildSteps = new BuildStepsElement(this);
            mCombinedState = new CombinedStateData();
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

        /// <summary>Adds a set of libraries </summary>
        [FileSet("libraries", Required = false)]
        public StructuredFileSet Libraries
        {
            get { return _libraries; }
            set { _libraries = value; }
        }private StructuredFileSet _libraries = new StructuredFileSet();

        /// <summary>Adds the list of objectfiles</summary>
        [FileSet("objectfiles", Required = false)]
        public StructuredFileSet ObjectFiles
        {
            get { return _objectfiles; }

        }private StructuredFileSet _objectfiles = new StructuredFileSet();

        /// <summary>Adds the list of sourcefiles</summary>
        [FileSet("sourcefiles", Required = false)]
        public StructuredFileSet SourceFiles
        {
            get { return _sourcefiles; }

        }private StructuredFileSet _sourcefiles = new StructuredFileSet();

        /// <summary>Adds a list of assembly source files</summary>
        [FileSet("asmsourcefiles", Required = false)]
        public StructuredFileSet AsmSourceFiles
        {
            get { return _asmsourcefiles; }

        }private StructuredFileSet _asmsourcefiles = new StructuredFileSet();

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

        /// <summary>Files with assocuated custombuildtools</summary>
        [FileSet("custombuildfiles", Required = false)]
        public StructuredFileSetCollection CustomBuildFiles
        {
            get { return _custombuildfiles; }

        }private StructuredFileSetCollection _custombuildfiles = new StructuredFileSetCollection();

        /// <summary>Specifies the Rootnamespace for a visual studio project</summary>
        [Property("rootnamespace", Required = false)]
        public ConditionalPropertyElement RootNamespace
        {
            get { return _rootnamespaceelem; }
            set { _rootnamespaceelem = value; }
        }private ConditionalPropertyElement _rootnamespaceelem = new ConditionalPropertyElement();

        /// <summary>Defines set of directories to search for header files</summary>
        [Property("includedirs", Required = false)]
        public ConditionalPropertyElement IncludeDirs
        {
            get { return _includedirselem; }
            set { _includedirselem = value; }
        }private ConditionalPropertyElement _includedirselem = new ConditionalPropertyElement();

        /// <summary>Includes a list of header files for a project</summary>
        [FileSet("headerfiles", Required = false)]
        public StructuredFileSet HeaderFiles
        {
            get { return _headerfiles; }
            set { _headerfiles = value; }
        }private StructuredFileSet _headerfiles = new StructuredFileSet();

        /// <summary>A list of COM assemblies for this module</summary>
        [FileSet("comassemblies", Required = false)]
        public StructuredFileSet ComAssemblies
        {
            get { return _comassemblies; }
            set { _comassemblies = value; }
        }private StructuredFileSet _comassemblies = new StructuredFileSet();

        /// <summary>A list of referenced assemblies for this module</summary>
        [FileSet("assemblies", Required = false)]
        public StructuredFileSet Assemblies
        {
            get { return _assemblies; }
            set { _assemblies = value; }
        }private StructuredFileSet _assemblies = new StructuredFileSet();

        [FileSet("force-usingassemblies", Required = false)]
        public StructuredFileSet FuAssemblies
        {
            get { return _fuassemblies; }
            set { _fuassemblies = value; }
        }private StructuredFileSet _fuassemblies = new StructuredFileSet();

        /// <summary>Adds a list of assembly source files</summary>
        [FileSet("additional-manifest-files", Required = false)]
        public StructuredFileSet AdditionalManifestFiles
        {
            get { return _additionalmanifestfiles; }

        }private StructuredFileSet _additionalmanifestfiles = new StructuredFileSet();

        /// <summary>Sets the configuration for a project</summary>
        [BuildElement("config", Required = false)]
        public ConfigElement Config
        {
            get { return _config; }
        }private ConfigElement _config;

        /// <summary>Sets the build steps for a project</summary>
        [BuildElement("buildsteps", Required = false)]
        public BuildStepsElement BuildSteps
        {
            get { return _buildSteps; }
        }private BuildStepsElement _buildSteps;

        /// <summary>Sets the bulkbuild configuration</summary>
        [BuildElement("bulkbuild", Required = false)]
        public BulkBuildElement BulkBuild
        {
            get { return _bulkbuild; }
        }private BulkBuildElement _bulkbuild = new BulkBuildElement();

        /// <summary></summary>
        [BuildElement("dotnet", Required = false)]
        public DotNetDataElement DotNetDataElement
        {
            get { return _dotNetData; }
        }private DotNetDataElement _dotNetData = new DotNetDataElement();

        /// <summary></summary>
        [BuildElement("visualstudio", Required = false)]
        public VisualStudioDataElement VisualStudioData
        {
            get { return _visualstudioData; }
        }private VisualStudioDataElement _visualstudioData = new VisualStudioDataElement();

        /// <summary></summary>
        [BuildElement("java", Required = false)]
        public JavaDataElement JavaData
        {
            get { return _javaData; }
        }private JavaDataElement _javaData = new JavaDataElement();

        /// <summary></summary>
        [BuildElement("platforms", Required = false)]
        public PlatformExtensions PlatformExtensions
        {
            get { return _platformExtensions; }
        }private PlatformExtensions _platformExtensions = new PlatformExtensions();


        protected override void SetupModule()
        {
            SetupConfig();

            SetupBuildData();

            SetupBulkBuildData();

            SetupDependencies();

            SetupDotNet();

            SetupVisualStudio();

            SetupJava();

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
                string finalbuildtype = BuildType = (buildtype??BuildType) + "-" + ModuleName;

                _config.buildOptionSet.Options["buildset.name"] = finalbuildtype;
                SetModuleProperty("buildtype", finalbuildtype);

                Project.NamedOptionSets[finalbuildtype + "-temp"] = _config.buildOptionSet;

                if (_config.buildOptionSet.Options.Contains("ps3-spu"))
                {
                    GenerateBuildOptionsetSPU.Execute(Project, _config.buildOptionSet, finalbuildtype + "-temp");
                }
                else
                {
                    GenerateBuildOptionset.Execute(Project, _config.buildOptionSet, finalbuildtype + "-temp");
                }
            }

            base.SetBuildType();

            SetModuleProperty("defines", StringUtil.EnsureSeparator(_config.Defines.Value, Environment.NewLine), _config.Defines.Append);
            SetModuleProperty("warningsuppression", StringUtil.EnsureSeparator(_config.Warningsuppression.Value, Environment.NewLine), _config.Warningsuppression.Append);

            SetModuleProperty("remove.defines", StringUtil.EnsureSeparator(_config.RemoveOptions.Defines.Value, Environment.NewLine), _config.RemoveOptions.Defines.Append);
            SetModuleProperty("remove.cc.options", StringUtil.EnsureSeparator(_config.RemoveOptions.CcOptions.Value, Environment.NewLine), _config.RemoveOptions.CcOptions.Append);
            SetModuleProperty("remove.as.options", StringUtil.EnsureSeparator(_config.RemoveOptions.AsmOptions.Value, Environment.NewLine), _config.RemoveOptions.AsmOptions.Append);
            SetModuleProperty("remove.link.options", StringUtil.EnsureSeparator(_config.RemoveOptions.LinkOptions.Value, Environment.NewLine), _config.RemoveOptions.LinkOptions.Append);
            SetModuleProperty("remove.lib.options", StringUtil.EnsureSeparator(_config.RemoveOptions.LibOptions.Value, Environment.NewLine), _config.RemoveOptions.LibOptions.Append);

            CombinedStateData.UpdateBoolean(ref mCombinedState.EnablePch, _config.PrecompiledHeader._enable);

            if (mCombinedState.EnablePch)
            {
                if (!String.IsNullOrEmpty(_config.PrecompiledHeader.PchFile))
                {
                    SetModuleProperty("pch-file", _config.PrecompiledHeader.PchFile, append: false);
                }
                if (!String.IsNullOrEmpty(_config.PrecompiledHeader.PchHeaderFile))
                {
                    SetModuleProperty("pch-header-file", _config.PrecompiledHeader.PchHeaderFile, append: false);
                }
            }
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

            SetModuleProperty("rootnamespace", RootNamespace);
            SetModuleProperty("includedirs", IncludeDirs);

            SetModuleFileset("sourcefiles", SourceFiles);            
            SetModuleFileset("asmsourcefiles", AsmSourceFiles);
            SetModuleFileset("objects", ObjectFiles);
            SetModuleFileset("headerfiles", HeaderFiles);
            SetModuleFileset("libs", Libraries);

            SetModuleFileset("comassemblies", ComAssemblies);
            SetModuleFileset("assemblies", Assemblies);
            SetModuleFileset("forceusing-assemblies", FuAssemblies);
            SetModuleFileset("additional-manifest-files", AdditionalManifestFiles);

            if (ResourceFiles.Includes.Count > 0 || ResourceFiles.Excludes.Count > 0)
            {
                    SetModuleFileset("resourcefiles", ResourceFiles);
                    SetModuleProperty("resourcefiles.prefix", GetValueOrDefault(ResourceFiles.Prefix, ModuleName));
                    SetModuleProperty("resourcefiles.basedir", SourceFiles.BaseDirectory??ResourceFiles.ResourceBasedir);
            }

            SetModuleProperty("res.includedirs", ResourceFiles.ResourceIncludeDirs, append: true);

            if (ResourceFilesNonEmbed.Includes.Count > 0 || ResourceFilesNonEmbed.Excludes.Count > 0)
            {
                SetModuleFileset("resourcefiles.notembed", ResourceFilesNonEmbed);
            }
            
            StructuredFileSet custombuildfiles = new StructuredFileSet();
            foreach (StructuredFileSet fs in CustomBuildFiles.FileSets)
            {
                custombuildfiles.AppendWithBaseDir(fs);
                custombuildfiles.AppendBase = fs.AppendBase;
            }
            if (custombuildfiles.BaseDirectory == null) custombuildfiles.BaseDirectory = Project.BaseDirectory;

            SetModuleFileset("custombuildfiles", custombuildfiles);

            SetModuleTarget(BuildSteps.PrebuildTarget, "prebuildtarget");
            SetModuleTarget(BuildSteps.PostbuildTarget, "postbuildtarget");

            SetModuleProperty("vcproj.custom-build-tool", BuildSteps.CustomBuildStep.Script.Value);
            SetModuleProperty("vcproj.custom-build-outputs", BuildSteps.CustomBuildStep.OutputDependencies.Value);
            SetModuleProperty("vcproj.custom-build-dependencies", BuildSteps.CustomBuildStep.InputDependencies.Value);
            SetModuleProperty("custom-build-before-targets", BuildSteps.CustomBuildStep.Before);
            SetModuleProperty("custom-build-after-targets", BuildSteps.CustomBuildStep.After);
        }

        protected void SetupBulkBuildData()
        {
            bool hasManualsourcefiles = BulkBuild.ManualSourceFiles.Includes.Count > 0;

            bool hasBulkBuildFilesets = BulkBuild.SourceFiles.NamedFilesets.Count > 0;

            if (BulkBuild._enable == null && (hasManualsourcefiles || hasBulkBuildFilesets || BulkBuild._partial == true || BulkBuild.MaxSize > 0))
            {
                // If any data are explicitly defined - enable.
                BulkBuild.Enable = true;

            }

            CombinedStateData.UpdateBoolean(ref mCombinedState.BulkBuildEnable, BulkBuild._enable);
            CombinedStateData.UpdateBoolean(ref mCombinedState.HasManualsourcefiles, (hasManualsourcefiles ? true : (bool?)null));
            CombinedStateData.UpdateBoolean(ref mCombinedState.HasBulkBuildFilesets, hasBulkBuildFilesets ? true : (bool?)null);


            if (BulkBuild.Enable)
            {
                if(hasManualsourcefiles)
                {
                    SetModuleFileset("bulkbuild.manual.sourcefiles", BulkBuild.ManualSourceFiles);
                }

                if(hasBulkBuildFilesets)
                {
                    var sb = new StringBuilder();
                    foreach (var entry in BulkBuild.SourceFiles.NamedFilesets)
                    {
                        SetModuleFileset("bulkbuild." + entry.Key + ".sourcefiles", entry.Value);
                        sb.AppendLine(entry.Key);
                    }
                    SetModuleProperty("bulkbuild.filesets", sb.ToString());
                }

                SetModuleProperty("enablepartialbulkbuild", BulkBuild.Partial.ToString(), append: false);
                if (BulkBuild.MaxSize > 0)
                {
                    SetModuleProperty("max-bulkbuild-size", BulkBuild.MaxSize.ToString(), append:false);
                }

                if (BulkBuild.LooseFiles.Includes.Count > 0)
                {
                    SetModuleFileset("loosefiles", BulkBuild.LooseFiles);
                }
            }
        }

        protected void SetupDotNet()
        {
            SetModuleOptionSet("webreferences", DotNetDataElement.WebReferences);

            SetModuleProperty("copylocal", DotNetDataElement.CopyLocal);
            SetModuleProperty("allowunsafe", DotNetDataElement.AllowUnsafe);
        }

        protected void SetupVisualStudio()
        {
            string projType = BuildTypeInfo.IsCSharp ? "csproj" : "vcproj";

            SetModuleTarget(VisualStudioData.PregenerateTarget, projType + ".prebuildtarget");

            SetDefaultBuildStep(VisualStudioData.PreBuildStep, BuildSteps.PrebuildTarget);
            SetDefaultBuildStep(VisualStudioData.PostBuildStep, BuildSteps.PostbuildTarget);

            SetModuleProperty(projType + ".pre-build-step", VisualStudioData.PreBuildStep);
            SetModuleProperty(projType + ".post-build-step", VisualStudioData.PostBuildStep);

            SetModuleProperty(projType + ".pre-link-step", VisualStudioData.VcProj.PreLinkStep);
            SetModuleFileset(projType + ".additional-manifest-files", VisualStudioData.VcProj.AdditionalManifestFiles);
            SetModuleFileset(projType + ".input-resource-manifests", VisualStudioData.VcProj.InputResourceManifests);
            SetModuleFileset(projType + ".excludedbuildfiles", VisualStudioData.ExcludedBuildFiles);
        }

        private void SetDefaultBuildStep(ConditionalPropertyElement step, BuildTargetElement targetProperty)
        {
            if (!String.IsNullOrEmpty(targetProperty.Command.Value) && String.IsNullOrEmpty(step.Value))
            {
                step.Value = targetProperty.Command.Value;
            }
        }

        protected void SetupAfterBuild()
        {
            // --- Packaging ---
            StringBuilder names = new StringBuilder();
            foreach (StructuredFileSet fs in BuildSteps.PackagingData.Assets.FileSets)
            {
                names.AppendLine(SetModuleFileset("assetfiles", fs));
            }
            if (names.Length > 0)
            {
                SetModuleProperty("assetfiles-set", names.ToString(), true);
            }

            SetModuleProperty("assetdependencies", BuildSteps.PackagingData.AssetDependencies);
            SetModuleFileset("manifestfile", BuildSteps.PackagingData.ManifestFile);
            // --- Run ---
            SetModuleProperty("workingdir", BuildSteps.RunData.WorkingDir);
            SetModuleProperty("commandargs", BuildSteps.RunData.Args);
            SetModuleProperty("commandprogram", BuildSteps.RunData.StartupProgram);
        }

        protected void SetupJava()
        {
            SetModuleFileset("java.sourcefiles", JavaData.SourceFiles);
            SetModuleFileset("java.entrypoint", JavaData.EntryPoint);
            SetModuleFileset("java.classes", JavaData.Classes);
            SetModuleFileset("java.archives", JavaData.Archives);
            SetModuleProperty("java.classroot", JavaData.ClassRoot);

            SetModuleProperty("jniheadersdir", JavaData.JniHeadersDir);
            SetModuleProperty("jniclasses", JavaData.JniClasses);

            SetModuleTarget(JavaData.PrebuildTarget, "java.prebuildtarget");
            SetModuleTarget(JavaData.PostbuildTarget, "java.postbuildtarget");

            if (!String.IsNullOrEmpty(JavaData.PrebuildTarget.Command.Value))
            {
                SetModuleProperty("java.pre-build-step", JavaData.PrebuildTarget.Command.Value);
            }
            if (!String.IsNullOrEmpty(JavaData.PostbuildTarget.Command.Value))
            {
                SetModuleProperty("java.post-build-step", JavaData.PostbuildTarget.Command.Value);
            }
        }

        protected override void InitModule()
        {
            mCombinedState.BulkBuildEnable = BulkBuild.Enable;
            mCombinedState.HasManualsourcefiles = false;
            mCombinedState.HasBulkBuildFilesets = false;

            mCombinedState.EnablePch = _config.PrecompiledHeader.Enable;
        }

        protected override void FinalizeModule()
        {
            SetModuleProperty("use.pch", mCombinedState.EnablePch ? "true" : "false");
            if (mCombinedState.EnablePch && null == GetModuleProperty("pch-file"))
            {
                SetModuleProperty("pch-file", ModuleName + ".pch", append: false);
            }

            if (mCombinedState.BulkBuildEnable && !mCombinedState.HasManualsourcefiles && !mCombinedState.HasBulkBuildFilesets)
            {
                // If no filesets are defined, this is a simple case:
                StructuredFileSet bulkbuildfileset = new StructuredFileSet();
                var sourcefiles = Project.GetFileSet(Group + "." + ModuleName + ".sourcefiles");
                bulkbuildfileset.IncludeWithBaseDir(sourcefiles);

                var loosefiles = Project.GetFileSet(Group + "." + ModuleName + ".loosefiles");

                bulkbuildfileset.Exclude(loosefiles);
                sourcefiles.Include(loosefiles);

                SetModuleFileset("bulkbuild.sourcefiles", bulkbuildfileset);
            }

        }

        internal class CombinedStateData
        {
            internal bool EnablePch;
            internal bool BulkBuildEnable;
            internal bool HasManualsourcefiles;
            internal bool HasBulkBuildFilesets;

            internal static void UpdateBoolean(ref bool val, bool? newval)
            {
                if (newval != null)
                {
                    val = (bool)newval;
                }
            }
        }
        internal CombinedStateData mCombinedState;
    }

}
