using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{
    public static class NantSchemaExtensionMethods
    {
        public static TaskNameAttribute NantTaskNameAttribute(this Type type)
        {
            return Attribute.GetCustomAttribute(type, typeof(TaskNameAttribute)) as TaskNameAttribute;
        }

        public static string NantTaskName(this Type type)
        {
            var attr = type.NantTaskNameAttribute();
            if (attr != null)
            {
                return attr.Name;
            }
            return String.Empty;
        }

        /// <summary>Replaces characters in a tasks type name to remove characters that are illegal in xml,
        /// yet still keeps the typename unique.</summary>
        public static string NantSafeSchemaKey(this string typename)
        {
            return typename.Replace("+", "-").Replace("[", "_").Replace("]", "_");
        }

    }
}
