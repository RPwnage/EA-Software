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
    [ElementName("conditionalproperty", StrictAttributeCheck = true)]
    public class BuildTypePropertyElement : ConditionalPropertyElement
    {
        public BuildTypePropertyElement()
            : base(false, "name")
        {
            Append = false;
        }

        /// <summary>Argument. Default is null.</summary>
        [TaskAttribute("name", Required = true, ExpandProperties = false)]
        public override string AttrValue
        {
            set { base.AttrValue = value; }
            get { return base.AttrValue; }
        }

        protected override void InitializeElement(XmlNode node)
        {
            base.InitializeElement(node);
        }
    }
}
