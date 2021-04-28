using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace EA.Eaconfig
{
    public class NantChildElement
    {
        public NantChildElement(Type type, string name, string summary, XmlNode description, string typeDescription, NantEnumType validValues)
            : this(type.FullName, name, summary, description, typeDescription, validValues)
        {
        }

        public NantChildElement(string typeName, string name, string summary, XmlNode description, string typeDescription, NantEnumType validValues)
        {
            Key = typeName.NantSafeSchemaKey();
            Name = name;
            IsConditionalTaskContainer = false;
            Summary = summary;
            Description = description;
            TypeDescription = typeDescription;
            ValidValues = validValues;
        }

        public readonly string Key;
        public readonly string Name;
        public readonly bool IsConditionalTaskContainer;
        public readonly string Summary;
        public readonly XmlNode Description;
        public readonly string TypeDescription;
        public readonly NantEnumType ValidValues;
    };
}
