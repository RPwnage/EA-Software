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
    [ElementName("resourcefiles", StrictAttributeCheck = true)]
    public class ResourceFilesElement : StructuredFileSet
    {
        public ResourceFilesElement() : base(){
        }

        public ResourceFilesElement(ResourceFilesElement source)
            : base(source)
        {
            _prefix = source._prefix;
            _resourcebasedir = source._resourcebasedir;
            _resourceincludedirs = source._resourceincludedirs;
        }

        /// <summary>Indicates the prefix to prepend to the actual resource.  This is usually the 
        /// default namspace of the assembly.</summary>
        [TaskAttribute("prefix")]
        public string Prefix
        {
            get { return _prefix; }
            set { _prefix = value; }
        } private string _prefix = String.Empty; // default to empty prefix


        /// <summary></summary>
        [TaskAttribute("resourcebasedir")]
        public string ResourceBasedir
        {
            get { return _resourcebasedir; }
            set { _resourcebasedir = value; }
        } private string _resourcebasedir; // default to empty prefix

        /// <summary>Additional include directories to pass to the resource compiler</summary>
        [TaskAttribute("includedirs", Required = false)]
        public string ResourceIncludeDirs
        {
            get { return _resourceincludedirs; }
            set { _resourceincludedirs = value; }
        } private string _resourceincludedirs; // default to empty string
    }
}
