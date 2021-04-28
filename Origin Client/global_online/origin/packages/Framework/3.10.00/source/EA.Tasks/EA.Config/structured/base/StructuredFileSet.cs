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
    [ElementName("StructuredFileSet", StrictAttributeCheck = true)]
    public class StructuredFileSet : FileSet
    {
        public StructuredFileSet() : base() {}

        public StructuredFileSet(StructuredFileSet source) : base(source)
        {
            _append = source.AppendBase;
        }

        public StructuredFileSet(FileSet source)
            : base(source)
        {
        }

        public StructuredFileSet(FileSet source, bool append)
            : base(source)
        {
            _append = append;
        }

        /// <summary></summary>
        [TaskAttribute("append", Required = false)]
        public bool AppendBase
        {
            get { return _append; }
            set { _append = value; }
        } private bool _append = true;

        /// <summary></summary>
        [TaskAttribute("name", Required = false)]
        public string Suffix
        {
            get { return _suffix; }
            set { _suffix = value; }
        } private string _suffix = String.Empty;

        [TaskAttribute("optionset", Required = false)] // For documentation
        public string DummyOptionset
        {
            set { }
        }

    }

    /// <summary></summary>
    [ElementName("StructuredFileSetCollection", StrictAttributeCheck = true)]
    public class StructuredFileSetCollection : Element
    {
        bool _ifDefined = true;
        bool _unlessDefined = false;

        public StructuredFileSetCollection() : base() { }

        public readonly List<StructuredFileSet> FileSets = new List<StructuredFileSet>();

        /// <summary></summary>
        [TaskAttribute("append", Required = false)]
        public bool AppendBase
        {
            get { return _append; }
            set { _append = value; }
        } private bool _append = true;

        /// <summary></summary>
        [TaskAttribute("name", Required = false)]
        public string Suffix
        {
            get { return _suffix; }
            set { _suffix = value; }
        } private string _suffix = String.Empty;

        /// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
        [TaskAttribute("if")]
        public virtual bool IfDefined
        {
            get { return _ifDefined; }
            set { _ifDefined = value; }
        }

        /// <summary>Opposite of if.  If false then the task will be executed; otherwise skipped. Default is "false".</summary>
        [TaskAttribute("unless")]
        public virtual bool UnlessDefined
        {
            get { return _unlessDefined; }
            set { _unlessDefined = value; }
        }

        // Define FileSet attributes as dummy attributes to avoid complaints about unknown attributes
        [TaskAttribute("basedir", Required = false)]
        public string BaseDir
        {
            get { return String.Empty; }
            set { }
        }

        // Define FileSet attributes as dummy attributes to avoid complaints about unknown attributes
        [TaskAttribute("fromfileset", Required = false)]
        public string FromFileset
        {
            get { return String.Empty; }
            set { }
        }

        [XmlElement("includes", "NAnt.Core.FileSet+IncludesElement", AllowMultiple = true)]
        [XmlElement("excludes", "NAnt.Core.FileSet+ExcludesElement", AllowMultiple = true)]
        [XmlElement("do", Container = XmlElementAttribute.ContainerType.ConditionalContainer, AllowMultiple = true)]
        [XmlElement("group", Container = XmlElementAttribute.ContainerType.ConditionalContainer, AllowMultiple = true)]
        public override void Initialize(XmlNode elementNode)
        {
            _ifDefined = true;
            _unlessDefined = false;
            base.Initialize(elementNode);
        }

        protected override void InitializeElement(XmlNode elementNode)
        {
            if (IfDefined && !UnlessDefined)
            {
                StructuredFileSet fs = new StructuredFileSet();
                fs.Project = Project;
                fs.Initialize(elementNode);

                FileSets.Add(fs);
            }
        }
    }
}
