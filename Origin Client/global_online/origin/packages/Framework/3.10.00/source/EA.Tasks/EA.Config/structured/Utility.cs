using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Util;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("Utility")]
    public class UtilityTask : ModuleBaseTask
    {
        public UtilityTask() : base("Utility")
        {
            _buildSteps = new BuildStepsElement(this);
        }

        /// <summary>Files with assocuated custombuildtools</summary>
        [FileSet("custombuildfiles", Required = false)]
        public StructuredFileSetCollection CustomBuildFiles
        {
            get { return _custombuildfiles; }

        }private StructuredFileSetCollection _custombuildfiles = new StructuredFileSetCollection();


        [FileSet("excludedbuildfiles", Required = false)]
        public StructuredFileSet ExcludedFiles
        {
            get { return _excludedfiles; }
            set { _excludedfiles = value; }
        }private StructuredFileSet _excludedfiles = new StructuredFileSet();

        /// <summary>Sets the build steps for a project</summary>
        [BuildElement("buildsteps", Required = false)]
        public BuildStepsElement BuildSteps
        {
            get { return _buildSteps; }
        }private BuildStepsElement _buildSteps;


        protected override void SetupModule()
        {
            SetupDependencies();
            SetupFiles();
        }

        protected void SetupFiles()
        {
            string projType = BuildTypeInfo.IsCSharp ? "csproj" : "vcproj";

            SetModuleFileset(projType + ".excludedbuildfiles", ExcludedFiles);

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

            SetModuleProperty(projType + ".pre-build-step", BuildSteps.PrebuildTarget.Command.Value);
            SetModuleProperty(projType + ".post-build-step", BuildSteps.PostbuildTarget.Command.Value);

            SetModuleProperty("vcproj.custom-build-tool", BuildSteps.CustomBuildStep.Script.Value);
            SetModuleProperty("vcproj.custom-build-outputs", BuildSteps.CustomBuildStep.OutputDependencies.Value);
            SetModuleProperty("vcproj.custom-build-dependencies", BuildSteps.CustomBuildStep.InputDependencies.Value);
            SetModuleProperty("custom-build-before-targets", BuildSteps.CustomBuildStep.Before);
            SetModuleProperty("custom-build-after-targets", BuildSteps.CustomBuildStep.After);
        }

        protected override void InitModule() { }

        protected override void FinalizeModule() { }

    }
}
