using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.Xml.Xsl;
using System.IO;
using System.Reflection;
using NAnt.Core.Util;
using EA.Eaconfig;

using SandcastleBuilder.Utils.BuildEngine;
using SandcastleBuilder.Utils.ConceptualContent;


namespace FrameworkDocsPlugin
{
    public class FunctionTopicGenerator
    {
        private readonly SchemaMetadata SchemaData;

        private Dictionary<string, string> Guids = new Dictionary<string, string>();


        public FunctionTopicGenerator(SchemaMetadata schemaData)
        {
            SchemaData = schemaData;
        }

        public void CleanFunctionFolders(BuildProcess builder)
        {
            try
            {
                var dir = GetWorkingFolder(builder);

                if (Directory.Exists(dir))
                {
                    Directory.Delete(dir);
                }
            }
            catch (Exception) { }
        }

        public void GenerateGuids()
        {
            foreach (NantFunctionClass functionclass in SchemaData.FunctionClasses.Values)
            {
                if (!Guids.ContainsKey(functionclass.Name))
                {
                    Guids[functionclass.Name] = Hash.MakeGUIDfromString(functionclass.Name).ToString();
                }
            }
        }

        public void GenerateTopics(BuildProcess builder, TopicCollection functionTopics)
        {
            foreach (NantFunctionClass functionclass in SchemaData.FunctionClasses.Values)
            {
                XmlDocument document = new XmlDocument();

                XmlNode rootNode = document.CreateNode(XmlNodeType.Element, "topic", String.Empty);
                rootNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "revisionNumber", "1"));
                rootNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "id", Guids[functionclass.Name]));

                XmlElement devConDoc = document.CreateElement("developerConceptualDocument");
                devConDoc.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(
                    document, "xmlns", "http://ddue.schemas.microsoft.com/authoring/2003/5"));
                devConDoc.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(
                    document, "xmlns:xlink", "http://www.w3.org/1999/xlink"));
                rootNode.AppendChild(devConDoc);

                XmlElement intro = document.CreateElement("introduction");
                devConDoc.AppendChild(intro);

                AttachClassSummaryComments(functionclass, intro, document);

                foreach (NantFunction function in functionclass.Functions) {
                    AttachFunctionDetails(function, devConDoc, document);
                }

                XmlElement section3 = XmlTopicHelper.CreateSection(document, devConDoc, "Examples", "");

                XmlElement related = XmlTopicHelper.CreateSection(document, devConDoc, "relatedTopics", "");

                document.AppendChild(rootNode);

                // Save Doc
                var filepath = Path.Combine(GetWorkingFolder(builder), functionclass.Name + ".aml");
                Directory.CreateDirectory(Path.GetDirectoryName(filepath));
                document.Save(filepath);

                // Add file to topics
                string linkPath;
                var topic = TopicFilter.CreateTopic(builder, filepath, functionclass.Name, functionclass.Name, out linkPath);
                if (topic != null)
                {
                    functionTopics.Add(topic);
                }
            }
        }

        private void AttachFunctionDetails(NantFunction function, XmlElement parent, XmlDocument document)
        {
            string title = function.GetShortStringWithNames();
            XmlElement section = XmlTopicHelper.CreateSection(document, parent, title, "");

            XmlNode content = document.CreateElement("content");
            section.AppendChild(content);

            if (function.Description != null) {
                XmlNode summary = function.Description.SelectSingleNode("section/content");
                if (summary != null)
                {
                    /*XmlNode para = XmlTopicHelper.CreateXmlElement(document, "para");
                    para.InnerText = summary.InnerText;
                    content.AppendChild(para);*/

                    foreach (XmlNode node in function.Description.ChildNodes)
                    {
                        content.AppendChild(parent.OwnerDocument.ImportNode(node, true));
                    }
                }
            }
        }

        private void AttachClassSummaryComments(NantFunctionClass functionclass, XmlElement parent, XmlDocument document)
        {
            if (functionclass.Description != null)
            {
                foreach (XmlNode child in functionclass.Description.ChildNodes)
                {
                    parent.AppendChild(parent.OwnerDocument.ImportNode(child, true));
                }

                XmlNode table = document.CreateElement("table");
                parent.AppendChild(table);

                XmlNode header = document.CreateElement("tableHeader");
                table.AppendChild(header);

                header.AppendChild(CreateHeaderRow(document, new string[] { "Function", "Summary" }));

                foreach (NantFunction function in functionclass.Functions)
                {
                    XmlNode row = document.CreateElement("row");
                    table.AppendChild(row);

                    XmlNode entry1 = document.CreateElement("entry");
                    row.AppendChild(entry1);

                    XmlNode para1 = document.CreateElement("para");
                    para1.InnerText = function.GetShortString();
                    entry1.AppendChild(para1);

                    XmlNode entry2 = document.CreateElement("entry");
                    row.AppendChild(entry2);

                    if (function.Description != null)
                    {
                        XmlNode summary = function.Description.SelectSingleNode("section/content");
                        if (summary != null)
                        {
                            XmlNode para2 = document.CreateElement("para");
                            para2.InnerText = summary.InnerText;
                            entry2.AppendChild(para2);
                        }
                    }
                }
            }
        }

        private XmlElement CreateHeaderRow(XmlDocument doc, String[] values)
        {
            XmlElement row = doc.CreateElement("row");
            for (int i = 0; i < values.Length; ++i)
            {
                XmlElement entry = doc.CreateElement("entry");
                row.AppendChild(entry);

                XmlElement para = doc.CreateElement("para");
                para.InnerText = values[i];
                entry.AppendChild(para);
            }
            return row;
        }

        private string GetWorkingFolder(BuildProcess builder)
        {
            return Path.Combine(builder.WorkingFolder, "FrameworkDocsPlugin", "Functions");
        }

    }
}
