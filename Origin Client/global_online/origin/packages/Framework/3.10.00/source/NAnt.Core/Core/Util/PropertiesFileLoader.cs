using System;
using System.Xml;
using System.Collections.Generic;
using System.Text;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Writers;

namespace NAnt.Core.Util
{
    public class PropertiesFileLoader
    {

        public static void LoadPropertiesFromFile(string file, Dictionary<string, string> properties)
        {
            LineInfoDocument doc = LineInfoDocument.Load(file);

            if (doc.DocumentElement.Name != "project")
            {
                    Console.WriteLine("[warning] Expected root element is 'project', found root '{0}' in the properties file {1}.", doc.DocumentElement.Name, file);
                }

                foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "properties":
                                ParseProperties(childNode, properties, file);
                                break;
                            default:
                                XmlUtil.Warning(childNode, String.Format("Unknown Xml element '{0}' in properties file. Expected element 'properties'.", childNode.Name), null); 
                                break;
                        }
                    }

                }
        }

        public static void SavePropertiesToFile(string file, IDictionary<string, string> properties)
        {
            if (properties != null)
            {
                using (IXmlWriter writer = new NAnt.Core.Writers.XmlWriter(_xmlWriterFormat))
                {
                    writer.FileName = file;

                    writer.WriteStartDocument();

                    writer.WriteStartElement("project");

                    writer.WriteStartElement("properties");
                    foreach (var prop in properties)
                    {
                        if (prop.Value != null && !String.IsNullOrEmpty(prop.Key))
                        {
                            writer.WriteStartElement("property");
                            writer.WriteAttributeString("name", prop.Key);
                            writer.WriteString(prop.Value);
                            writer.WriteEndElement();
                        }
                    }
                    writer.WriteEndElement(); // properties
                    writer.WriteEndElement(); // project
                }
            }

        }

        static private void ParseProperties(XmlNode node, Dictionary<string, string> properties, string file)
        {
                foreach (XmlNode childNode in node.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(node.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "property":
                                string name = XmlUtil.GetRequiredAttribute(childNode, "name");
                                string val = XmlUtil.GetOptionalAttribute(childNode, "value", null);
                                string text = StringUtil.Trim(childNode.InnerText);
                                if(val != null && !String.IsNullOrEmpty(text))
                                {
                                    XmlUtil.Error(childNode, "ERROR in property file: property '" + name + "' has values defined in 'value' atribute and as element text.");
                                }
                                if(val == null)
                                {
                                    val = text;
                                }
                                string existingVal;
                                if (properties.TryGetValue(name, out existingVal))
                                {
                                    if (existingVal != val)
                                    {
                                        XmlUtil.Warning(childNode, String.Format("Waning: Property \"{0}\" defined more than once in properties file or on the nant command line with different values {1} and {2}. Second value will be ignored", name, existingVal, val), null);
                                    }
                                }
                                else
                                {
                                    properties.Add(name, val);
                                }
                                break;
                            default:
                                XmlUtil.Warning(childNode, String.Format("Unknown Xml element '{0}' in properties file. Expected element 'properties'.", childNode.Name), null); 
                                break;
                        }
                    }
                }

        }

        private static readonly IXmlWriterFormat _xmlWriterFormat = new XmlFormat(
                identChar: ' ',
                identation: 2,
                newLineOnAttributes: false,
                encoding: new UTF8Encoding(false) // no byte order mask!
            );

    }
}
