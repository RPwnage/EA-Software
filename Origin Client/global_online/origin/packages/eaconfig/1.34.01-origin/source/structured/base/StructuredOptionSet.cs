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
    [ElementName("StructuredOptionSet", StrictAttributeCheck = true)]
    public class StructuredOptionSet : OptionSet
    {
        public StructuredOptionSet() : base() { }

        public StructuredOptionSet(StructuredOptionSet source)
        {
            _append = source.AppendBase;
        }

        public StructuredOptionSet(OptionSet source)
        {
            base.Append(source);
        }

        public StructuredOptionSet(OptionSet source, bool append)
            :   this(source)
        {
            _append = append;
        }

        [TaskAttribute("append", Required = false)]
        public bool AppendBase
        {
            get { return _append; }
            set { _append = value; }
        } private bool _append = true;
    }

}
