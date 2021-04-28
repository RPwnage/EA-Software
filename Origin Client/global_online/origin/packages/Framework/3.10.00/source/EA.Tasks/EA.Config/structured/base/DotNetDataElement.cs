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
    [ElementName("DotNet", StrictAttributeCheck = true)]
    public class DotNetDataElement : Element
    {
        [Property("copylocal", Required = false)]
        public ConditionalPropertyElement CopyLocal
        {
            get { return _copylocalelem; }
            set { _copylocalelem = value; }
        }private ConditionalPropertyElement _copylocalelem = new ConditionalPropertyElement();

        [Property("allowunsafe", Required = false)]
        public ConditionalPropertyElement AllowUnsafe
        {
            get { return _allowUnsafe; }
            set { _allowUnsafe = value; }
        }private ConditionalPropertyElement _allowUnsafe = new ConditionalPropertyElement();

        [OptionSet("webreferences", Required = false)]
        public StructuredOptionSet WebReferences
        {
            get { return _webreferences; }
            set { _webreferences = value; }
        }private StructuredOptionSet _webreferences = new StructuredOptionSet();

    }
}