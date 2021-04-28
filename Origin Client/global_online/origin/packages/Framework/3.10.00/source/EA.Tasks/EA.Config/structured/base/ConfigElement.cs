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
    /// <summary>Sets various attributes for a config</summary>
    [ElementName("config", StrictAttributeCheck = true)]
    public class ConfigElement : Element
    {
        private ModuleBaseTask _module;

        internal readonly OptionSet buildOptionSet = new OptionSet();

        public ConfigElement(ModuleBaseTask module)
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

        /// <summary>Gets the warning suppression property for this config</summary>
        [Property("warningsuppression", Required = false)]
        public ConditionalPropertyElement Warningsuppression
        {
            get { return _warningsuppression; }
        }private ConditionalPropertyElement _warningsuppression = new ConditionalPropertyElement();

        /// <summary>Set up precompiled headers</summary>
        [Property("pch", Required = false)]
        public PrecompiledHeadersElement PrecompiledHeader
        {
            get { return _pch; }
        }private PrecompiledHeadersElement _pch = new PrecompiledHeadersElement();

        /// <summary>Define options to removefrom the final optionset</summary>
        [Property("remove", Required = false)]
        public RemoveBuildOptions RemoveOptions
        {
            get { return _removeoptions; }
        }private RemoveBuildOptions _removeoptions = new RemoveBuildOptions();
    }
}
