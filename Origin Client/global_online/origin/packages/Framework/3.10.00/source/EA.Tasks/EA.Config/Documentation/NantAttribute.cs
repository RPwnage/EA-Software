using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace EA.Eaconfig
{
    public class NantAttribute
    {
        public NantAttribute(Type type, string attributeName, bool isRequired, string summary, XmlNode description, NantEnumType validValues)
        {
            ClassType = type;
            AttributeName = attributeName;
            IsRequired = isRequired;
            Summary = summary;
            Description = description;
            TypeDescription = type.Name;
            ValidValues = validValues;
        }

        public readonly Type ClassType;
        public readonly string AttributeName;
        public readonly bool IsRequired;
        public readonly string Summary;
        public readonly XmlNode Description;
        public string TypeDescription;
        public readonly NantEnumType ValidValues;
    };
}