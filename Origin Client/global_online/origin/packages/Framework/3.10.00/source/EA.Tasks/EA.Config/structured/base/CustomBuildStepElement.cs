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
	/// <summary>Defines a custom build step that may execute before or 
    /// after another target.</summary>
    [ElementName("custom-build-step", StrictAttributeCheck = true)]
    public class CustomBuildStepElement : Element
    {
        /// <summary>
        /// Name of the target a custom build step should run before (build, link, run).
        /// Supported by native NAnt and MSBuild.
        /// </summary>
        [TaskAttribute("before")]
        public String Before
        {
            get { return _before; }
            set { _before = value; }
        }private String _before = String.Empty;

        /// <summary>
        /// Name of the target a custom build step should run after (build, run).
        /// Supported by native NAnt and MSBuild.
        /// </summary>
        [TaskAttribute("after")]
        public String After
        {
            get { return _after; }
            set { _after = value; }
        }private String _after = String.Empty;

        /// <summary>The commands to be executed by the custom build step, 
        /// either nant tasks or os commands.</summary>
        [Property("custom-build-tool", Required = true)]
        public XmlPropertyElement Script
        {
            get { return _script; }
            set { _script = value; }
        }private XmlPropertyElement _script = new XmlPropertyElement();

        /// <summary>A list of files that are added to the step's output dependencies.</summary>
        [FileSet("custom-build-outputs", Required = false)]
        public PropertyElement OutputDependencies
        {
            get { return _outputdependencies; }

        }private PropertyElement _outputdependencies = new PropertyElement();

        /// <summary>A list of files that are added to the step's input dependencies.</summary>
        [FileSet("custom-build-dependencies", Required = false)]
        public PropertyElement InputDependencies
        {
            get { return _inputdependencies; }

        }private PropertyElement _inputdependencies = new PropertyElement();
    }
}
