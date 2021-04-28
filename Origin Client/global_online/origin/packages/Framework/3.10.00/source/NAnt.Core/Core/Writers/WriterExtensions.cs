using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
    public static class WriterExtensions
    {

        public static void WriteNonEmptyElementString(this IXmlWriter writer, string name, string value)
        {
            if (writer != null && !String.IsNullOrWhiteSpace(value))
            {
                writer.WriteElementString(name, value);
            }
        }

        public static void WriteNonEmptyAttributeString(this IXmlWriter writer, string localName, string value)
        {
            if (writer != null && !String.IsNullOrWhiteSpace(value))
            {
                writer.WriteAttributeString(localName, value);
            }
        }
    }
}
