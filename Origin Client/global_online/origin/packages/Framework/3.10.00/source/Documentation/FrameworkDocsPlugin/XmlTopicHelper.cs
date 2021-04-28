using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace FrameworkDocsPlugin
{
    public class XmlTopicHelper
    {
        public static XmlElement CreateSection(XmlDocument document, XmlElement parent, string title, string content)
        {
            XmlElement section = document.CreateElement("section");
            parent.AppendChild(section);

            XmlElement sectiontitle = document.CreateElement("title");
            sectiontitle.InnerText = title;
            section.AppendChild(sectiontitle);

            if (content != null)
            {
                XmlElement sectioncontent = document.CreateElement("content");
                sectioncontent.InnerText = content;
                section.AppendChild(sectioncontent);
            }

            return section;
        }

        public static XmlAttribute CreateXmlAttribute(XmlDocument doc, string name, string value)
        {
            XmlAttribute attribute = doc.CreateAttribute(name);
            attribute.Value = value;
            return attribute;
        }
    }
}
