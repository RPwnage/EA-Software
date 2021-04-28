using System;

namespace NAnt.Core.Attributes
{
    [AttributeUsage(AttributeTargets.Method, Inherited = true, AllowMultiple=false)]
    public class XmlTaskContainerAttribute : Attribute
    {
        private string _filter;

        public XmlTaskContainerAttribute(string filter="*")
        {
            _filter = filter;
        }

        public string Filter
        {
            get { return _filter; }
            set { _filter = value; }
        }
    }
}
