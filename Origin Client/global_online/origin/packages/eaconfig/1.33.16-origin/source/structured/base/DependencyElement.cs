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
    [ElementName("dependencies", StrictAttributeCheck = true)]
    public class DependenciesPropertyElement : Element
    {
        [NAnt.Core.Attributes.Description("Sets the dependencies to be used by the package")]
        [Property("use", Required = false)]
        public ConditionalPropertyElement UseDependencies
        {
            get { return _useDependencies; }
            set { _useDependencies = value; }
        }private ConditionalPropertyElement _useDependencies = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("Sets the dependencies to be built by the package")]
        [Property("build", Required = false)]
        public ConditionalPropertyElement BuildDependencies
        {
            get { return _buildDependencies; }
            set { _buildDependencies = value; }
        }private ConditionalPropertyElement _buildDependencies = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("")]
        [Property("interface", Required = false)]
        public ConditionalPropertyElement InterfaceDependencies
        {
            get { return _interfaceDependencies; }
            set { _interfaceDependencies = value; }
        }private ConditionalPropertyElement _interfaceDependencies = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("Sets or gets the dependencies that needs to be used during the linking task for this package ")]
        [Property("link", Required = false)]
        public ConditionalPropertyElement LinkDependencies
        {
            get { return _linkDependencies; }
            set { _linkDependencies = value; }
        }private ConditionalPropertyElement _linkDependencies = new ConditionalPropertyElement();

    }
}
