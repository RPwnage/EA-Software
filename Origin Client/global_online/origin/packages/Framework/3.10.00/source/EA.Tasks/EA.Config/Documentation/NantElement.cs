using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace EA.Eaconfig
{
    public class NantElement
    {
        public NantElement(Type type, string elementName, bool isTask, bool isStrict, bool isSchemaElement, bool isMixed,
            bool isTaskContainer, string summary, XmlNode description)
        {
            ClassType = type;
            Key = type.FullName.NantSafeSchemaKey();

            ElementName = elementName;
            IsTask = isTask;
            IsStrict = isStrict;
            IsSchemaElement = isSchemaElement;
            IsMixed = isMixed;
            IsTaskContainer = isTaskContainer;
            Summary = summary;
            Description = description;
        }

        public NantElement(string typeName, string elementName, bool isTask, bool isStrict, bool isSchemaElement, bool isMixed,
            bool isTaskContainer, string summary, XmlNode description)
        {
            ClassType = typeof(Object);
            Key = typeName.NantSafeSchemaKey();

            ElementName = elementName;
            IsTask = isTask;
            IsStrict = isStrict;
            IsSchemaElement = isSchemaElement;
            IsMixed = isMixed;
            IsTaskContainer = isTaskContainer;
            Summary = summary;
            Description = description;
        }

        public readonly Type ClassType;
        public readonly string Key;
        public readonly string ElementName;
        public readonly bool IsTask;
        public readonly bool IsStrict;
        public readonly bool IsSchemaElement;
        public readonly bool IsMixed;
        public readonly string Summary;
        public readonly XmlNode Description;
        public bool IsTaskContainer;

        public readonly List<NantChildElement> ChildElements = new List<NantChildElement>();
        public readonly List<NantAttribute> Attributes = new List<NantAttribute>();
    };
}
