using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;

namespace EA.Eaconfig.Structured
{
    /// <summary>BuildStepsElement</summary>
    [ElementName("buildsteps", StrictAttributeCheck = true)]
    public class BuildStepsElement : Element
    {
        public BuildStepsElement(ModuleBaseTask module)
        {
            _packagingData = new PackagingElement(module);
        }

        /// <summary>Sets the prebuild steps for a project</summary>
        [Property("prebuild-step", Required = false)]
        public BuildTargetElement PrebuildTarget
        {
            get { return _preBuildTarget; }
        }private BuildTargetElement _preBuildTarget = new BuildTargetElement("prebuildtarget");

        /// <summary>Sets the postbuild steps for a project</summary>
        [Property("postbuild-step", Required = false)]
        public BuildTargetElement PostbuildTarget
        {
            get { return _postBuildTarget; }
            set { _postBuildTarget = value; }
        }private BuildTargetElement _postBuildTarget = new BuildTargetElement("postbuildtarget");

        /// <summary>Defines a custom build step that may execute before or 
        /// after another target</summary>
        [Property("custom-build-step", Required = false)]
        public CustomBuildStepElement CustomBuildStep
        {
            get { return _customBuildStep; }
            set { _customBuildStep = value; }
        }private CustomBuildStepElement _customBuildStep = new CustomBuildStepElement();

        /// <summary>Sets the packaging steps for a project</summary>
        [BuildElement("packaging", Required = false)]
        public PackagingElement PackagingData
        {
            get { return _packagingData; }
        }private PackagingElement _packagingData;

        /// <summary>Sets the run steps for a project</summary>
        [BuildElement("run", Required = false)]
        public RunDataElement RunData
        {
            get { return _runData; }
        }private RunDataElement _runData = new RunDataElement();
    }
}
