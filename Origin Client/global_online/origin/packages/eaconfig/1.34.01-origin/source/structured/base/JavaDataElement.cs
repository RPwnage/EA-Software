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
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [ElementName("JavaData", StrictAttributeCheck = true)]
    public class JavaDataElement : Element
    {
        [NAnt.Core.Attributes.Description("List of source files to be used")]
        [FileSet("sourcefiles", Required = false)]
        public StructuredFileSet SourceFiles
        {
            get { return _sourcefiles; }

        }private StructuredFileSet _sourcefiles = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("Files to be used as an entry point")]
        [FileSet("entrypoint", Required = false)]
        public StructuredFileSet EntryPoint
        {
            get { return _entrypoint; }
        }private StructuredFileSet _entrypoint = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("")]
        [FileSet("archives", Required = false)]
        public StructuredFileSet Archives
        {
            get { return _archives; }

        }private StructuredFileSet _archives = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("")]
        [FileSet("classes", Required = false)]
        public StructuredFileSet Classes
        {
            get { return _classes; }

        }private StructuredFileSet _classes = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("")]
        [Property("jniheadersdir", Required = false)]
        public ConditionalPropertyElement JniHeadersDir
        {
            get { return _jniheadersdir; }
        }private ConditionalPropertyElement _jniheadersdir = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("")]
        [Property("jniclasses", Required = false)]
        public ConditionalPropertyElement JniClasses
        {
            get { return _jniclasses; }
        }private ConditionalPropertyElement _jniclasses = new ConditionalPropertyElement();
    }

}
