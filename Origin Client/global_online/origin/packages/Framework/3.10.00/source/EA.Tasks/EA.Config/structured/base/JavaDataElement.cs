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
    /// <summary></summary>
    [ElementName("JavaData", StrictAttributeCheck = true)]
    public class JavaDataElement : Element
    {
        /// <summary>List of source files to be used</summary>
        [FileSet("sourcefiles", Required = false)]
        public StructuredFileSet SourceFiles
        {
            get { return _sourcefiles; }

        }private StructuredFileSet _sourcefiles = new StructuredFileSet();

        /// <summary>Files to be used as an entry point</summary>
        [FileSet("entrypoint", Required = false)]
        public StructuredFileSet EntryPoint
        {
            get { return _entrypoint; }
        }private StructuredFileSet _entrypoint = new StructuredFileSet();

        /// <summary></summary>
        [FileSet("archives", Required = false)]
        public StructuredFileSet Archives
        {
            get { return _archives; }

        }private StructuredFileSet _archives = new StructuredFileSet();

        /// <summary></summary>
        [FileSet("classes", Required = false)]
        public StructuredFileSet Classes
        {
            get { return _classes; }

        }private StructuredFileSet _classes = new StructuredFileSet();

        /// <summary></summary>
        [Property("jniheadersdir", Required = false)]
        public ConditionalPropertyElement JniHeadersDir
        {
            get { return _jniheadersdir; }
        }private ConditionalPropertyElement _jniheadersdir = new ConditionalPropertyElement();

        /// <summary></summary>
        [Property("jniclasses", Required = false)]
        public ConditionalPropertyElement JniClasses
        {
            get { return _jniclasses; }
        }private ConditionalPropertyElement _jniclasses = new ConditionalPropertyElement();

        /// <summary></summary>
        [Property("classroot", Required = false)]
        public ConditionalPropertyElement ClassRoot
        {
            get { return _classroot; }
        }private ConditionalPropertyElement _classroot = new ConditionalPropertyElement();

        /// <summary>Sets the prebuild steps for a java build</summary>
        [Property("prebuild-step", Required = false)]
        public BuildTargetElement PrebuildTarget
        {
            get { return _preBuildTarget; }
        }private BuildTargetElement _preBuildTarget = new BuildTargetElement("prebuildtarget");

        /// <summary>Sets the postbuild steps for a java build</summary>
        [Property("postbuild-step", Required = false)]
        public BuildTargetElement PostbuildTarget
        {
            get { return _postBuildTarget; }
            set { _postBuildTarget = value; }
        }private BuildTargetElement _postBuildTarget = new BuildTargetElement("postbuildtarget");


    }

}
