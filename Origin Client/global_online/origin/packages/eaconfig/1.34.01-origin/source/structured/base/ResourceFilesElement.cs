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
    [ElementName("resourcefiles", StrictAttributeCheck = true)]
    public class ResourceFilesElement : StructuredFileSet
    {
        public ResourceFilesElement() : base(){
        }

        public ResourceFilesElement(ResourceFilesElement source)
            : base(source)
        {
            _prefix = source._prefix;
            _embed = source._embed;
            _resourcebasedir = source._resourcebasedir;
        }

        /// <summary>Indicates the prefix to prepend to the actual resource.  This is usually the 
        /// default namspace of the assembly.</summary>
        [NAnt.Core.Attributes.Description("Indicates the prefix to prepend to the actual resource.  This is usually the default namspace of the assembly.")]
        [TaskAttribute("prefix")]
        public string Prefix
        {
            get { return _prefix; }
            set { _prefix = value; }
        } private string _prefix = String.Empty; // default to empty prefix

        [NAnt.Core.Attributes.Description("")]
        [TaskAttribute("embed")]
        public bool Embed
        {
            get { return _embed; }
            set { _embed = value; }
        } private bool _embed = true; // default to embed

        [NAnt.Core.Attributes.Description("")]
        [TaskAttribute("resourcebasedir")]
        public string ResourceBasedir
        {
            get { return _resourcebasedir; }
            set { _resourcebasedir = value; }
        } private string _resourcebasedir; // default to empty prefix



    }

}
