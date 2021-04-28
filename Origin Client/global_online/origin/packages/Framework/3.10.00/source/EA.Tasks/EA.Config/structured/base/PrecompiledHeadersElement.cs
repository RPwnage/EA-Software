using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.IO;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;


namespace EA.Eaconfig.Structured
{
    /// <summary>Precompiled headers input</summary>
    [ElementName("pch", StrictAttributeCheck = true)]
    public class PrecompiledHeadersElement : ConditionalElement
    {
        public PrecompiledHeadersElement()
        {
            _enable = null;
        }

        /// <summary>enable/disable using precompiled headers.</summary>
        [TaskAttribute("enable")]
        public bool Enable
        {
            get { return _enable??false; }
            set { _enable = value; }
        } internal bool? _enable;

        /// <summary>Name of output precompiled header</summary>
        [TaskAttribute("pchfile", Required=false)]
        public string PchFile
        {
            get { return _pchfile; }
            set { _pchfile = value; }
        } private string _pchfile;

        /// <summary>Name of the precompiled header file</summary>
        [TaskAttribute("pchheader", Required=false)]
        public string PchHeaderFile
        {
            get { return _pchheaderfile; }
            set { _pchheaderfile = value; }
        } private string _pchheaderfile;
    }
}
