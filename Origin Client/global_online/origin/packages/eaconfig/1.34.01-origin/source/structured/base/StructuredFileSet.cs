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

        [NAnt.Core.Attributes.Description("")]
        [TaskAttribute("append", Required = false)]
        public bool AppendBase
        {
            get { return _append; }
            set { _append = value; }
        } private bool _append = true;

        [NAnt.Core.Attributes.Description("")]
        [TaskAttribute("name", Required = false)]
        public string Suffix
        {
            get { return _suffix; }
            set { _suffix = value; }
        } private string _suffix = String.Empty;
    }

    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [ElementName("StructuredFileSetCollection", StrictAttributeCheck = true)]
    public class StructuredFileSetCollection : Element
    {
        bool _ifDefined = true;
        bool _unlessDefined = false;

        public StructuredFileSetCollection() : base() { }

        public readonly List<StructuredFileSet> FileSets = new List<StructuredFileSet>();

        [NAnt.Core.Attributes.Description("")]
        [TaskAttribute("append", Required = false)]
        public bool AppendBase
        {
            get { return _append; }
            set { _append = value; }
        } private bool _append = true;



        [NAnt.Core.Attributes.Description("")]
        [TaskAttribute("name", Required = false)]
        public string Suffix
        {
            get { return _suffix; }
            set { _suffix = value; }
        } private string _suffix = String.Empty;

        /// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
        [NAnt.Core.Attributes.Description("If true then the task will be executed; otherwise skipped. Default is \"true\"")]
        [TaskAttribute("if")]
        public virtual bool IfDefined
        {
            get { return _ifDefined; }
            set { _ifDefined = value; }
        }

        /// <summary>Opposite of if.  If false then the task will be executed; otherwise skipped. Default is "false".</summary>
        [NAnt.Core.Attributes.Description("Opposite of if.  If false then the task will be executed; otherwise skipped. Default is \"false\"")]
        [TaskAttribute("unless")]
        public virtual bool UnlessDefined
        {
            get { return _unlessDefined; }
            set { _unlessDefined = value; }
        }

        public void Initialize(XmlNode elementNode)
        {
            //reset if and uless so that they aren't carried over to the appearance of the element in xml.
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
            _ifDefined = true;
            _unlessDefined = false;
        }
    }



}
