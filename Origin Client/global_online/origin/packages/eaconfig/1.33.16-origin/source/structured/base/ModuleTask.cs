using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using EA.Eaconfig.Structured.ObjectModel;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("Module")]
    public class ModuleTask : ModuleBaseTask
    {
        protected ModuleTask(string buildtype) : base(buildtype)
        {
            _config = new ConfigElement(this);
            _buildSteps = new BuildStepsElement(this);
        }

        [NAnt.Core.Attributes.Description("Adds a set of libraries")]
        [FileSet("libraries", Required = false)]
        public StructuredFileSet Libraries
        {
            get { return _libraries; }
            set { _libraries = value; }
        }private StructuredFileSet _libraries = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("Adds the list of sourcefiles")]
        [FileSet("sourcefiles", Required = false)]
        public StructuredFileSet SourceFiles
        {
            get { return _sourcefiles; }

        }private StructuredFileSet _sourcefiles = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("Adds a list of assembly source files")]
        [FileSet("asmsourcefiles", Required = false)]
        public StructuredFileSet AsmSourceFiles
        {
            get { return _asmsourcefiles; }

        }private StructuredFileSet _asmsourcefiles = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("Adds a list of resource files")]
        [FileSet("resourcefiles", Required = false)]
        public ResourceFilesElement ResourceFiles
        {
            get { return _resourcefiles; }
            
        }private ResourceFilesElement _resourcefiles = new ResourceFilesElement();

        [NAnt.Core.Attributes.Description("Defines set of directories to search for header files")]
        [Property("includedirs", Required = false)]
        public ConditionalPropertyElement IncludeDirs
        {
            get { return _includedirselem; }
            set { _includedirselem = value; }
        }private ConditionalPropertyElement _includedirselem = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("Includes a list of header files for a project")]
        [FileSet("headerfiles", Required = false)]
        public StructuredFileSet HeaderFiles
        {
            get { return _headerfiles; }
            set { _headerfiles = value; }
        }private StructuredFileSet _headerfiles = new StructuredFileSet();

        [FileSet("comassemblies", Required = false)]
        public StructuredFileSet ComAssemblies
        {
            get { return _comassemblies; }
            set { _comassemblies = value; }
        }private StructuredFileSet _comassemblies = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("Sets the dependencies for a project")]
        [BuildElement("dependencies", Required = false)]
        public DependenciesPropertyElement Dependencies
        {
            get { return _dependencies; }
        }private DependenciesPropertyElement _dependencies = new DependenciesPropertyElement();

        [NAnt.Core.Attributes.Description("Sets the configuration for a project")]
        [BuildElement("config", Required = false)]
        public ConfigElement Config
        {
            get { return _config; }
        }private ConfigElement _config;

        [NAnt.Core.Attributes.Description("Sets the build steps for a project")]
        [BuildElement("buildsteps", Required = false)]
        public BuildStepsElement BuildSteps
        {
            get { return _buildSteps; }
        }private BuildStepsElement _buildSteps;

        [NAnt.Core.Attributes.Description("")]
        [BuildElement("dotnet", Required = false)]
        public DotNetDataElement DotNetDataElement
        {
            get { return _dotNetData; }
        }private DotNetDataElement _dotNetData = new DotNetDataElement();

        [NAnt.Core.Attributes.Description("")]
        [BuildElement("visualstudio", Required = false)]
        public VisualStudioDataElement VisualStudioData
        {
            get { return _visualstudioData; }
        }private VisualStudioDataElement _visualstudioData = new VisualStudioDataElement();

        [NAnt.Core.Attributes.Description("")]
        [BuildElement("java", Required = false)]
        public JavaDataElement JavaData
        {
            get { return _javaData; }
        }private JavaDataElement _javaData = new JavaDataElement();

        protected override void SetupModule()
        {
            SetupConfig();

            SetupBuildData();

            SetupDependencies();

            SetupDotNet();

            SetupVisualStudio();

            SetupJava();

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
                string finalbuildtype = BuildType = buildtype + "-" + ModuleName;

                _config.buildOptionSet.Options["buildset.name"] = finalbuildtype;
                SetModuleProperty("buildtype", finalbuildtype);

                GenerateBuildOptionset.Execute(Project, _config.buildOptionSet, finalbuildtype + "-temp");
            }

            base.SetBuildType();

            SetModuleProperty("defines", StringUtil.EnsureSeparator(_config.Defines.Value, Environment.NewLine), _config.Defines.Append);
            SetModuleProperty("warningsuppression", StringUtil.EnsureSeparator(_config.Warningsuppression.Value, Environment.NewLine), _config.Warningsuppression.Append);
        }

        protected void SetupBuildData()
        {
            SetModuleProperty("includedirs", IncludeDirs);

            SetModuleFileset("sourcefiles", SourceFiles);
            SetModuleFileset("asmsourcefiles", AsmSourceFiles);
            SetModuleFileset("headerfiles", HeaderFiles);
            SetModuleFileset("libs", Libraries);

            SetModuleFileset("comassemblies", ComAssemblies);

            if (ResourceFiles.Embed)
            {
                if (ResourceFiles.Includes.Count > 0 || ResourceFiles.Excludes.Count > 0)
                {
                    SetModuleFileset("resourcefiles", ResourceFiles);
                    SetModuleProperty("resourcefiles.prefix", GetValueOrDefault(ResourceFiles.Prefix, ModuleName));
                    SetModuleProperty("resourcefiles.basedir", ResourceFiles.ResourceBasedir);
                }
            }
            else
            {
                SetModuleFileset("resourcefiles.notembed", ResourceFiles);
            }

            SetModuleTarget(BuildSteps.PrebuildTarget, "prebuildtarget");
            SetModuleTarget(BuildSteps.PostbuildTarget, "postbuildtarget");
        }

        protected void SetupDotNet()
        {
            SetModuleOptionSet("webreferences", DotNetDataElement.WebReferences);

            SetModuleProperty("copylocal", DotNetDataElement.CopyLocal);
            SetModuleProperty("allowunsafe", DotNetDataElement.AllowUnsafe);
        }

        protected void SetupDependencies()
        {
            SetModuleDependencies("builddependencies", Dependencies.BuildDependencies.Value);
            SetModuleDependencies("usedependencies", Dependencies.UseDependencies.Value);
            SetModuleDependencies("interfacedependencies", Dependencies.InterfaceDependencies.Value);
            SetModuleDependencies("linkdependencies", Dependencies.LinkDependencies.Value);
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
        }

        private void SetDefaultBuildStep(ConditionalPropertyElement step, BuildTargetElement targetProperty)
        {
            if (String.IsNullOrEmpty(step.Value))
            {
                if (targetProperty != null && !String.IsNullOrEmpty(targetProperty.TargetName))
                {
                    string command = Project.ExpandProperties("${nant.location}/nant.exe " + targetProperty.TargetName + " -D:config=${config} -buildfile:${nant.project.buildfile} -masterconfigfile:${nant.project.masterconfigfile} -buildroot:${nant.project.buildroot}");
                    
                    step.Value = command;
                }
            }
        }

        protected void SetupAfterBuild()
        {
            // --- Packaging ---
            StringBuilder names = new StringBuilder();
            foreach(StructuredFileSet fs in BuildSteps.PackagingData.Assets.FileSets)
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

            SetModuleProperty("jniheadersdir", JavaData.JniHeadersDir);
            SetModuleProperty("jniclasses", JavaData.JniClasses);
        }


    }

}
