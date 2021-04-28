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
    [NAnt.Core.Attributes.Description("Sets various attributes for a config")]
    [NAnt.Core.Attributes.XmlSchema]
    [ElementName("config", StrictAttributeCheck = true)]
    public class ConfigElement : Element
    {
        private ModuleBaseTask _module;

        public readonly OptionSet buildOptionSet = new OptionSet();

        public ConfigElement(ModuleBaseTask module)
        {
            _module = module;

            _buildOptions = new BuildTypeElement(_module, buildOptionSet);
        }

        [NAnt.Core.Attributes.Description("Gets the build options for this config.")]
        [BuildElement("buildoptions", Required = false)]
        public BuildTypeElement BuildOptions
        {
            get { return _buildOptions; }
        }private BuildTypeElement _buildOptions;

        [NAnt.Core.Attributes.Description("Gets the macros defined for this config")]
        [Property("defines", Required = false)]
        public ConditionalPropertyElement Defines
        {
            get { return _defines; }
        }private ConditionalPropertyElement _defines = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("Gets the warning suppression property for this config")]
        [Property("warningsuppression", Required = false)]
        public ConditionalPropertyElement Warningsuppression
        {
            get { return _warningsuppression; }
        }private ConditionalPropertyElement _warningsuppression = new ConditionalPropertyElement();
    }

}
