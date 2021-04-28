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
    [NAnt.Core.Attributes.XmlSchema]    
    [NAnt.Core.Attributes.Description("")]
    [ElementName("buildsteps", StrictAttributeCheck = true)]
    public class BuildStepsElement : Element
    {
        public BuildStepsElement(ModuleBaseTask module)
        {
            _packagingData = new PackagingElement(module);
        }

        [NAnt.Core.Attributes.Description("Sets the prebuild steps for a project")]
        [Property("prebuild-step", Required = false)]
        public BuildTargetElement PrebuildTarget
        {
            get { return _preBuildTarget; }
        }private BuildTargetElement _preBuildTarget = new BuildTargetElement("prebuildtarget");

        [NAnt.Core.Attributes.Description("Sets the postbuild steps for a project")]
        [Property("postbuild-step", Required = false)]
        public BuildTargetElement PostbuildTarget
        {
            get { return _postBuildTarget; }
            set { _postBuildTarget = value; }
        }private BuildTargetElement _postBuildTarget = new BuildTargetElement("postbuildtarget");

        [NAnt.Core.Attributes.Description("Sets the packaging steps for a project")]
        [BuildElement("packaging", Required = false)]
        public PackagingElement PackagingData
        {
            get { return _packagingData; }
        }private PackagingElement _packagingData;

        [NAnt.Core.Attributes.Description("Sets the run steps for a project")]
        [BuildElement("run", Required = false)]
        public RunDataElement RunData
        {
            get { return _runData; }
        }private RunDataElement _runData = new RunDataElement();


    }

}
