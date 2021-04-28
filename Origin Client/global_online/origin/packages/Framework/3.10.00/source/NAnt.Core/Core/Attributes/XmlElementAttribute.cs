using System;

namespace NAnt.Core.Attributes
{
    [AttributeUsage(AttributeTargets.Method, Inherited = true, AllowMultiple=true)]
    public class XmlElementAttribute : Attribute
    {
        public enum ContainerType { None=0, Container=1, ConditionalContainer=2, Recursive = 4 }

        private string _name;
        private string _elementtype;
        private string _description;
        private bool _allowMultiple = false;
        private bool _mixed = false;
        private ContainerType _container = ContainerType.None;
        private string _filter;

        public XmlElementAttribute(string name, string elementtype)
        {
            Name = name;
            ElementType = elementtype;
        }

        public XmlElementAttribute(string name)
        {
            Name = name;
        }

        public string Name
        {
            get { return _name; }
            set { _name = value; }
        }

        public string ElementType
        {
            get { return _elementtype; }
            set { _elementtype = value; }
        }

        public string Description
        {
            get { return _description??String.Empty; }
            set { _description = value; }
        }


        public bool AllowMultiple
        {
            get { return _allowMultiple; }
            set { _allowMultiple = value; }
        }

        /// <summary>Task xml node may contain combination of nested elements and plain text,
        /// rather than only nested elements.</summary>
        public bool Mixed
        {
            get { return _mixed; }
            set { _mixed = value; }
        }

        public ContainerType Container
        {
            get { return _container; }
            set { _container = value; }
        }

        public string Filter
        {
            get { return _filter; }
            set { _filter = value; }
        }

        public bool IsContainer()
        {
            return (_container & ContainerType.Container) != 0 || (_container & ContainerType.ConditionalContainer) != 0;
        }

        public bool IsConditionalContainer()
        {
            return (_container & ContainerType.ConditionalContainer) != 0;
        }


        public bool IsRecursive()
        {
            return (_container & ContainerType.Recursive) != 0;
        }

    }
}
