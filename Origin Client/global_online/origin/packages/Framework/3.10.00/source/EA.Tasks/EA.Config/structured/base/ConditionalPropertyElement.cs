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
    [ElementName("conditionalproperty", StrictAttributeCheck = true, Mixed=true)]
    public class ConditionalPropertyElement : PropertyElement
    {
        bool _ifDefined = true;
        bool _unlessDefined = false;
        bool _append = true;
        string _attrvalue = null;

        protected readonly string _attrname;

        public ConditionalPropertyElement(string attrname = "value")
            : base()
        {
            _attrname = attrname;
        }

        public ConditionalPropertyElement(bool append, string attrname = "value")
            : base()
        {
            _append = append;
            _attrname = attrname;
        }


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
        
        /// <summary>Argument. Default is null.</summary>
        [TaskAttribute("value", Required = false, ExpandProperties = false)]
        public virtual string AttrValue
        {
            set { _attrvalue = value; }
            get { return _attrvalue; }
        }

        /// <summary>Append new data to the current value. The current value may come from partial modules. Default: 'true'.</summary>
        [TaskAttribute("append")]
        public virtual bool Append
        {
            get { return _append; }
            set { _append = value; }
        }

        public override void Initialize(XmlNode elementNode)
        {
            _ifDefined = true;
            _unlessDefined = false;
            base.Initialize(elementNode);
        }

        [XmlElement("do", Container = XmlElementAttribute.ContainerType.ConditionalContainer | XmlElementAttribute.ContainerType.Recursive, AllowMultiple = true, Mixed=true)]
        protected override void InitializeElement(XmlNode node)
        {
            if (IfDefined && !UnlessDefined)
            {
                if (_attrvalue != null)
                {
                    if (Value != null)
                    {
                        Value += Environment.NewLine + _attrvalue;
                    }
                    else
                    {
                        Value = _attrvalue;
                    }
                }

                foreach (XmlNode child in node)
                {
                    if (child.NodeType == XmlNodeType.Comment)
                    {
                        continue;
                    }

                    if (_attrvalue != null)
                    {
                        Error.Throw(Project, Location, Name, "Element value can not be defined in both '" + _attrname +"' attribute and as element text");
                    }

                    if (child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA)
                    {
                        if (Value != null)
                        {
                            Value += Environment.NewLine + child.InnerText;
                        }
                        else
                        {
                            Value = child.InnerText;
                        }
                    }
                    else if (child.Name == "do")
                    {
                        ConditionElement condition = new ConditionElement(Project, this);
                        condition.Initialize(child);
                    }
                    else
                    {
                        Error.Throw(Project, Location, Name, "Xml tag <{0}> is not allowed inside property values. Use <do> element for conditions", child.Name);
                    }
                }
                if (!String.IsNullOrEmpty(Value))
                {
                    Value = Project.ExpandProperties(Value);
                }
            }
        }
    }
    
    /// <summary></summary>
    [ElementName("if", StrictAttributeCheck = true)]
    class ConditionElement : PropertyElement
    {
        private bool _ifDefined = true;
        private bool _unlessDefined = false;

        private ConditionalPropertyElement _property;

        internal ConditionElement(Project project, ConditionalPropertyElement property)
            : base()
        {
            Project = project;
            _property = property;
        }

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

        public override void Initialize(XmlNode elementNode)
        {
            _ifDefined = true;
            _unlessDefined = false;
            base.Initialize(elementNode);
        }

        protected override void InitializeElement(XmlNode node)
        {
            if (IfDefined && !UnlessDefined)
            {
                foreach (XmlNode child in node)
                {
                    if (child.NodeType == XmlNodeType.Comment)
                    {
                        continue;
                    }

                    if (child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA)
                    {

                        if(_property.Value != null)
                        {
                            _property.Value += Environment.NewLine + child.InnerText;
                        }
                        else
                        {
                            _property.Value = child.InnerText;
                        }
                    }
                    else if (child.Name == "do")
                    {
                        ConditionElement condition = new ConditionElement(Project, _property);
                        condition.Initialize(child);
                    }
                    else
                    {
                        Error.Throw(Project, Location, _property.Name, "Xml tag <{0}> is not allowed inside property values. Use <do> element for conditions", child.Name);
                    }
                }
            }
        }
    }
}
