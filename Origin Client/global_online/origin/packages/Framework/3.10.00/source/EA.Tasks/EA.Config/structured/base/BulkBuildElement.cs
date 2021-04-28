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
    /// <summary>Bulkbuild input</summary>
    [ElementName("bulkbuild", StrictAttributeCheck = true)]
    public class BulkBuildElement : Element
    {
        public BulkBuildElement()
        {
            _enable = null;
            _partial = null;
            _maxsize = -1;
        }

        /// <summary>enable/disable bulkbuild generation.</summary>
        [TaskAttribute("enable")]
        public bool Enable
        {
            get { return _enable??false; }
            set { _enable = value; }
        } internal bool? _enable;

        /// <summary>If patrial enabled source files with custom build settings are excluded from bulkbuild generation and compiled separately.</summary>
        [TaskAttribute("partial")]
        public bool Partial
        {
            get { return _partial??false; }
            set { _partial = value; }
        } internal bool? _partial;

        /// <summary>If 'maxsize' is set generated bulkbuild files will contain no more than maxsize entries. I.e. They are split in several if number of files exceeds maxsize.</summary>
        [TaskAttribute("maxsize")]
        public int MaxSize
        {
            get { return _maxsize; }
            set { _maxsize = value; }
        } private int _maxsize;

        /// <summary>Files that are not included in the bulkbuild</summary>
        [FileSet("loosefiles", Required = false)]
        public StructuredFileSet LooseFiles
        {
            get { return _loosefiles; }

        }private StructuredFileSet _loosefiles = new StructuredFileSet();

        /// <summary>Groups of sourcefiles to be used to generate bulkbuild files</summary>
        [FileSet("sourcefiles", Required = false)]
        public NamedStructuredFileSets SourceFiles
        {
            get { return _sourcefiles; }

        }private NamedStructuredFileSets _sourcefiles = new NamedStructuredFileSets();

        /// <summary>Manual bulkbuild files</summary>
        [FileSet("manualsourcefiles", Required = false)]
        public StructuredFileSet ManualSourceFiles
        {
            get { return _manualsourcefiles; }

        }private StructuredFileSet _manualsourcefiles = new StructuredFileSet();

    }
}
