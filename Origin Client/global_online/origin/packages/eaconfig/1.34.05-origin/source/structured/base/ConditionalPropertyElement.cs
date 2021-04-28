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
    [NAnt.Core.Attributes.XmlSchema]
    [NAnt.Core.Attributes.Description("")]
    [ElementName("conditionalproperty", StrictAttributeCheck = true)]
    public class ConditionalPropertyElement : PropertyElement
    {
        bool _ifDefined = true;
        bool _unlessDefined = false;
        bool _append = true;

        public ConditionalPropertyElement()
            : base()
        {
        }

        public ConditionalPropertyElement(bool append)
            : base()
        {
            _append = append;
        }


        /// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
        [NAnt.Core.Attributes.Description("If true then the task will be executed; otherwise skipped. Default is \"true\".")]
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

        /// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
        [NAnt.Core.Attributes.Description("If true then the task will be executed; otherwise skipped. Default is \"true\"")]
        [TaskAttribute("append")]
        public virtual bool Append
        {
            get { return _append; }
            set { _append = value; }
        }

        protected override void InitializeElement(XmlNode node)
        {
            if (IfDefined && !UnlessDefined)
            {
                XmlAttribute attr = node.Attributes["value"];
                if (attr != null)
                {
                    if (Value != null)
                    {
                        Value += Environment.NewLine + attr.Value;
                    }
                    else
                    {
                        Value = attr.Value;
                    }
                }

                foreach (XmlNode child in node)
                {
                    if (child.NodeType == XmlNodeType.Comment)
                    {
                        continue;
                    }

                    if (attr != null)
                    {
                        Error.Throw(Project, Location, Name, "Element value can not be defined in both 'value' attribute and as element text");
                    }

                    if (child.NodeType == XmlNodeType.Text)
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
                if(!String.IsNullOrEmpty(Value))
                {
                   Value = Project.ExpandProperties(Value);
                }
            }
        }
    }

    [NAnt.Core.Attributes.XmlSchema]
    [NAnt.Core.Attributes.Description("")]
    [ElementName("if", StrictAttributeCheck = true)]
    public class ConditionElement : PropertyElement
    {
        bool _ifDefined = true;
        bool _unlessDefined = false;

        ConditionalPropertyElement _property;

        public ConditionElement(Project project, ConditionalPropertyElement property)
            : base()
        {
            Project = project;
            _property = property;
        }

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

                    if (child.NodeType == XmlNodeType.Text)
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
