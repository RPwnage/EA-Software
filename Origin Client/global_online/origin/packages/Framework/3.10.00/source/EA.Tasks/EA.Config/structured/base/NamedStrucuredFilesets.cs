using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
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
    public class NamedStructuredFileSets : StructuredFileSet
    {

        private IDictionary<string,StructuredFileSet> _filesets = new Dictionary<string, StructuredFileSet>();

        public NamedStructuredFileSets() : base() { }

        public NamedStructuredFileSets(StructuredFileSet source)
            : base(source)
        {
        }

        public NamedStructuredFileSets(FileSet source)
            : base(source)
        {
        }

        public NamedStructuredFileSets(FileSet source, bool append)
            : base(source, append)
        {
        }

        public IDictionary<string, StructuredFileSet> NamedFilesets
        {
            get { return _filesets; }
        }

        /// <summary></summary>
        [TaskAttribute("name", Required = true)]
        public string FileSetName
        {
            get { return _name; }
            set { _name = value; }
        } private string _name;


        public override void Initialize(XmlNode elementNode)
        {
            var attr = elementNode.Attributes["name"];
            if (attr == null)
            {
                throw new BuildException("Missing required attribute 'name' in NamedStructuredFileSets Element", Location);
            }
            FileSetName = Project.ExpandProperties(attr.Value);
            var fileset = new StructuredFileSet();
            fileset.Project = Project;
            fileset.AppendBase = AppendBase;
            fileset.Initialize(elementNode);

            StructuredFileSet existing;
            if (_filesets.TryGetValue(FileSetName, out existing))
            {
                existing.Append(fileset);
            }
            else
            {
                _filesets.Add(FileSetName, fileset);
            }
        }
    }

}
