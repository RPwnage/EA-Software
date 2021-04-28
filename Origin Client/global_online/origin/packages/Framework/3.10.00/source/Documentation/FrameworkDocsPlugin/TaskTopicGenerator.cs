using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;
using System.IO;
using System.Text;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using EA.Eaconfig;

using SandcastleBuilder.Utils.BuildEngine;
using SandcastleBuilder.Utils.ConceptualContent;

namespace FrameworkDocsPlugin
{
    public class TaskTopicGenerator
    {
        private readonly SchemaMetadata SchemaData;
        private static readonly List<string> ProjectLevelTasks = new List<string>();

        private Dictionary<string, string> Guids = new Dictionary<string, string>();

        public TaskTopicGenerator(SchemaMetadata schemaData)
        {
            SchemaData = schemaData;
            ProjectLevelTasks.Add("package");
        }

        public void GenerateGuids()
        {
            foreach (NantElement task in SchemaData.NantElements.Values)
            {
                if (!Guids.ContainsKey(task.Key))
                {
                    Guids[task.Key] = Hash.MakeGUIDfromString(task.Key).ToString();
                }
            }
        }

        public void GenerateTopics(BuildProcess builder, IEnumerable<TopicFilter> taskFilters, TopicCollection nantElementTopics)
        {
            var topicTable = new Dictionary<string, TopicFilter.TopicWithLink>();

            foreach (NantElement task in SchemaData.NantElements.Values)
            {
                XmlDocument document = new XmlDocument();

                XmlNode rootNode = document.CreateNode(XmlNodeType.Element, "topic", String.Empty);
                rootNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "revisionNumber", "1"));
                rootNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "id", Guids[task.Key]));
                document.AppendChild(rootNode);

                XmlElement mainNode = document.CreateElement("developerConceptualDocument");
                mainNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(
                    document, "xmlns", "http://ddue.schemas.microsoft.com/authoring/2003/5"));
                mainNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(
                    document, "xmlns:xlink", "http://www.w3.org/1999/xlink"));
                rootNode.AppendChild(mainNode);

                XmlElement intro = document.CreateElement("introduction");
                mainNode.AppendChild(intro);

                AttachTaskComments(task, mainNode, document);
                AttachTaskSyntax(task, mainNode, document);
                AttachInheritanceTree(task, mainNode, document);
                AttackAttributesSection(document, mainNode, task);
                AttachNestedElementsSection(document, mainNode, task);

                //XmlElement section3 = XmlTopicHelper.CreateSection(document, mainNode, "Examples", "");

                XmlElement related = XmlTopicHelper.CreateSection(document, mainNode, "relatedTopics", "");

                // Save the document:
                var filepath = Path.Combine(GetWorkingFolder(builder, task.IsTask), task.ClassType.Namespace, (task.IsTask ? String.Empty:task.Key)+ task.ElementName + ".aml");
                Directory.CreateDirectory(Path.GetDirectoryName(filepath));
                document.Save(filepath);

                // Populate Toc Table:
                if (task.IsTask)
                {
                    string title = String.Format("<{0}> Task", task.ElementName);
                    foreach (var filter in taskFilters)
                    {
                        TopicFilter.TopicWithLink topicInfo;
                        if (filter.FilterFiles(builder, filepath, task.ClassType.FullName, title, out topicInfo))
                        {
                            if(topicInfo != null)
                            {
                                topicTable.Add(task.Key, topicInfo);
                            }
                            break;
                        }
                    }
                }
                else
                {
                    // All elements that are not a task will be added to the hidden TOC node. These elements can be referenced
                    string linkPath;
                    var topic = TopicFilter.CreateTopic(builder, filepath, task.Key, task.ElementName, out linkPath);
                    if (topic != null)
                    {
                        nantElementTopics.Add(topic);
                        topicTable.Add(task.Key, new TopicFilter.TopicWithLink(topic, linkPath));
                    }
                }
            }

            // Fill TOC child nodes for tasks
            foreach (NantElement task in SchemaData.NantTasks.Values)
            {
                //IM: This is experimental code. Very slow and needs some work.
              //  SetElementTOCSubentries(builder, topicTable, task);
            }
        }

        private void SetElementTOCSubentries(BuildProcess builder, IDictionary<string, TopicFilter.TopicWithLink> topicTable, NantElement el)
        {
            if (el.IsTaskContainer)
            {
                // Don't fill containers
                return;
            }
            TopicFilter.TopicWithLink elTopic;
            if (topicTable.TryGetValue(el.Key, out elTopic))
            {
                foreach (var child in el.ChildElements)
                {
                    NantElement childElement;
                    if (SchemaData.NantElements.TryGetValue(child.Key, out childElement))
                    {
                        TopicFilter.TopicWithLink childTopic;
                        if (topicTable.TryGetValue(childElement.Key, out childTopic))
                        {
                            // Create new doc (SFHB does not allow reference same doc more than once, Visual Studio does,
                            // but we want to build it both ways
                            XmlDocument document = new XmlDocument();

                            XmlNode rootNode = document.CreateNode(XmlNodeType.Element, "topic", String.Empty);
                            rootNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "revisionNumber", "1"));
                            rootNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "id", Guid.NewGuid().ToString()));
                            document.AppendChild(rootNode);

                            XmlElement mainNode = document.CreateElement("developerConceptualDocument");
                            mainNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(
                                document, "xmlns", "http://ddue.schemas.microsoft.com/authoring/2003/5"));
                            mainNode.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(
                                document, "xmlns:xlink", "http://www.w3.org/1999/xlink"));
                            rootNode.AppendChild(mainNode);

                            XmlElement intro = document.CreateElement("introduction");
                            mainNode.AppendChild(intro);
                            intro.InnerXml = String.Format("<para><link xlink:href=\"{0}\">{1}</link></para>", childTopic.Topic.Id, child.Name);

                            // Save the document:
                            var newFileName = child.Name + "_" + childElement.Key + "_" + Guid.NewGuid().ToString() + ".aml";

                            var filepath = Path.Combine(GetWorkingFolder(builder, false), el.ClassType.Namespace, newFileName);
                            Directory.CreateDirectory(Path.GetDirectoryName(filepath));
                            document.Save(filepath);

                            builder.ReportProgress("Generated TOC file {0}", newFileName);

                            var linkPath = Path.Combine(builder.ProjectFolder, "Generated", newFileName);
                            var msbuildItem = builder.CurrentProject.MSBuildProject.AddItem("None", filepath, new Dictionary<string, string>() { { "Link", linkPath } });

                            builder.CurrentProject.MSBuildProject.ReevaluateIfNecessary();
                            if (msbuildItem == null)
                            {
                                builder.ReportProgress("ERROR in FrameworkDocsPlugin: Can not add file '{0}' to MSbuild", filepath);
                            }

                            var sfhbItem = builder.CurrentProject.FindFile(linkPath);

                            if (sfhbItem == null)
                            {
                                builder.ReportProgress("ERROR in FrameworkDocsPlugin: builder.CurrentProject.FindFile({0}) returned null", linkPath);
                            }
                            else
                            {
                                var topic = new Topic();
                                topic.Title = child.Name;
                                topic.TopicFile = new TopicFile(sfhbItem);

                                elTopic.Topic.Subtopics.Add(topic);
                                

                                if (el.Key != childElement.Key) // to avoid infinite recursion
                                {
                                    if (false == (child.Name == "do" || child.Name == "group"
                                        || child.Key.IndexOf("FileSet", StringComparison.InvariantCultureIgnoreCase) != -1
                                        || child.Key.IndexOf("OptionSet", StringComparison.InvariantCultureIgnoreCase) != -1))
                                    {
                                        SetElementTOCSubentries(builder, topicTable, childElement);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        public void CleanTaskFolders(BuildProcess builder)
        {
            try
            {
                var dir = GetWorkingFolder(builder, true);

                if (Directory.Exists(dir))
                {
                    Directory.Delete(dir);
                }

                dir = GetWorkingFolder(builder, false);
                if (Directory.Exists(dir))
                {
                    Directory.Delete(dir);
                }

            }
            catch(Exception){}
        }

        private string NamespaceFromKey(string key)
        {
            int index = key.LastIndexOf('.');
            return key.Substring(0, index);
        }

        private void AttachTaskComments(NantElement task, XmlElement parent, XmlDocument document)
        {
            if (task.Description != null)
            {
                foreach (XmlNode child in task.Description.ChildNodes)
                {
                    parent.AppendChild(parent.OwnerDocument.ImportNode(child, true));
                }
            }
        }

        private void AttachTaskSyntax(NantElement task, XmlElement parent, XmlDocument document)
        {
            if (task.ElementName != task.Key)
            {
                XmlNode section = XmlTopicHelper.CreateSection(document, parent, "Syntax", null);
                section.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "address", "syntax"));

                XmlNode content = document.CreateElement("content");
                section.AppendChild(content);

                XmlNode code = document.CreateElement("code");
                code.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "language", "xml"));
                code.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "title", " "));
                content.AppendChild(code);

                code.InnerText = GenerateTaskSyntax(task);
            }
        }

        private string GenerateTaskSyntax(NantElement task)
        {
            string[] hiddenAttributes = { "verbose", "failonerror", "nopackagelog" };

            string attributes = "";
            foreach (NantAttribute meminfo in task.Attributes)
            {
                if (!hiddenAttributes.Contains(meminfo.AttributeName))
                {
                    attributes += string.Format(" {0}=\"\"", meminfo.AttributeName);
                }
            }

            if (task.ChildElements.Count() == 0)
            {
                if (task.IsMixed)
                {
                    return string.Format("<{0}{1}>{2}</{0}>", task.ElementName, attributes, Environment.NewLine+"\tText" + Environment.NewLine);
                }
                else
                {
                    return string.Format("<{0}{1}/>", task.ElementName, attributes);
                }
            }

            string children = "";

            if (task.IsTaskContainer)
            {
                children = 
                    "\t\t. . . . . . . . . ." + Environment.NewLine +
                    "\t\t    NAnt script " + Environment.NewLine +
                    "\t\t. . . . . . . . . ." + Environment.NewLine;
            }
            else
            {
                foreach (NantChildElement child in task.ChildElements)
                {
                    children += GenerateChildElementSyntax(child, 1) + Environment.NewLine;
                }
                if (task.IsMixed)
                {
                    children += "\tText" + Environment.NewLine;
                }
            }

            return string.Format("<{0}{1}>\n{2}</{0}>", task.ElementName, attributes, children);
        }

        private string GenerateChildElementSyntax(NantChildElement childelement, int depth)
        {
            string tabs = "";
            for (int i = 0; i < depth; ++i)
            {
                tabs += "\t";
            }

            if (SchemaData.NantElements.ContainsKey(childelement.Key))
            {
                NantElement task = SchemaData.NantElements[childelement.Key];
                if (task != null)
                {
                    string[] hiddenAttributes = { "verbose", "failonerror", "nopackagelog" };

                    string attributes = "";
                    foreach (NantAttribute meminfo in task.Attributes)
                    {
                        if (!hiddenAttributes.Contains(meminfo.AttributeName))
                        {
                            attributes += string.Format(" {0}=\"\"", meminfo.AttributeName);
                        }
                    }

                    if (task.ChildElements.Count() == 0)
                    {
                        if (task.IsMixed)
                        {
                            return string.Format("{0}<{1}{2}>{3}{0}</{1}>", tabs, childelement.Name, attributes, Environment.NewLine + tabs + "\tText" + Environment.NewLine);
                        }

                        return string.Format("{0}<{1}{2}/>", tabs, childelement.Name, attributes);
                    }

                    if (task.ClassType.IsSubclassOf(typeof(NAnt.Core.TaskContainer)))
                    {
                        string inner = ". . .";
                        if (task.IsMixed)
                        {
                            inner = ". . . + Text";
                        }

                        return string.Format("{0}<{1}{2}>{3}</{1}>", tabs, childelement.Name, attributes, inner);
                    }

                    string children = "";
                    foreach (NantChildElement child in task.ChildElements)
                    {
                        if (childelement.Key.ToLowerInvariant().StartsWith("ea.eaconfig.structured")
                            && child.Key != task.Key && depth < 10)
                        {
                            if (task.Key.ToString().Contains("StructuredFileSet") 
                                || (task.ClassType.BaseType != null && task.ClassType.BaseType.ToString().Contains("StructuredFileSet")))
                            {
                                children = tabs + "\t<!-- Structured Fileset -->" + Environment.NewLine;
                            }
                            else if (childelement.Name == "do")
                            {
                                children = tabs + "\t<!-- NAnt Script -->" + Environment.NewLine;
                            }
                            else if (task.Key.Contains("ConditionalPropertyElement")
                                || (task.ClassType.BaseType != null && task.ClassType.BaseType.ToString().Contains("ConditionalPropertyElement")))
                            {
                                children = tabs + "\t<do ... />" + Environment.NewLine;
                            }
                            else
                            {
                                children += GenerateChildElementSyntax(child, depth + 1) + Environment.NewLine;
                            }
                        }
                        else
                        {
                            NantElement child_task = SchemaData.NantElements[child.Key];
                            string child_attributes = "";
                            foreach (NantAttribute meminfo in child_task.Attributes)
                            {
                                if (!hiddenAttributes.Contains(meminfo.AttributeName))
                                {
                                    child_attributes += string.Format(" {0}=\"\"", meminfo.AttributeName);
                                }
                            }
                            children += string.Format("{0}\t<{1}{2} ... />\n", tabs, child.Name, child_attributes);
                        }
                    }
                    if (task.IsMixed)
                    {
                        children += tabs+"\tText" + Environment.NewLine;
                    }

                    return string.Format("{3}<{0}{1}>\n{2}{3}</{0}>", childelement.Name, attributes, children, tabs);
                }
            }
            return string.Format("{1}<{0} ... />", childelement.Name, tabs);
        }

        private void AttachInheritanceTree(NantElement task, XmlElement parent, XmlDocument document)
        {
            XmlElement relationship = XmlTopicHelper.CreateSection(document, parent, "Inheritance Hierarchy", null);
            relationship.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "address", "hierarchy"));

            XmlElement relationshipcontent = document.CreateElement("content");
            relationship.AppendChild(relationshipcontent);

            XmlElement relationshiplist = document.CreateElement("list");
            relationshipcontent.AppendChild(relationshiplist);

            Type currentType = task.ClassType;
            int depth = 0;
            while (currentType != null && currentType != typeof(System.Object))
            {
                XmlElement relationshiplistItem = document.CreateElement("listItem");
                relationshiplist.AppendChild(relationshiplistItem);

                string tabs = "";
                for (int i = 0; i < depth; ++i)
                {
                    tabs += "----";
                }

                XmlElement relationshiplistItemPara = document.CreateElement("para");
                relationshiplistItem.AppendChild(relationshiplistItemPara);
                relationshiplistItemPara.InnerText = tabs + "> " + currentType.FullName;

                depth++;
                currentType = currentType.BaseType;
            }
        }

        private void AttackAttributesSection(XmlDocument document, XmlElement parent, NantElement task) 
        {
            if (task.Attributes != null && task.Attributes.Count > 0)
            {
                XmlElement section1 = XmlTopicHelper.CreateSection(document, parent, "Attributes", null);

                XmlElement section1content = document.CreateElement("content");
                section1.AppendChild(section1content);

                XmlElement parametertable = document.CreateElement("table");
                section1content.AppendChild(parametertable);

                XmlElement paratableheader = document.CreateElement("tableHeader");
                paratableheader.AppendChild(CreateTableHeaderRow(document,
                    new string[] { "Attribute", "Description", "Type", "Required" }));
                parametertable.AppendChild(paratableheader);

                foreach (NantAttribute attr in task.Attributes)
                {
                    parametertable.AppendChild(CreateTableAttributeRow(document, attr));
                }
            }
        }

        private void AttachNestedElementsSection(XmlDocument document, XmlElement parent, NantElement task)
        {
            if (task.ChildElements != null && task.ChildElements.Count > 0)
            {
                XmlElement section2 = XmlTopicHelper.CreateSection(document, parent, "Nested Elements", null);
                section2.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "address", "nestedelements"));

                XmlElement section2content = document.CreateElement("content");
                section2.AppendChild(section2content);

                if (task.IsTaskContainer)
                {
                    XmlElement nestedtext = document.CreateElement("par");
                    section2content.AppendChild(nestedtext);
                    nestedtext.InnerText = "Arbitrary NAnt script inside this element";
                }
                else
                {
                    XmlElement nestedtable = document.CreateElement("table");
                    section2content.AppendChild(nestedtable);

                    XmlElement nestedheader = document.CreateElement("tableHeader");
                    nestedheader.AppendChild(CreateTableHeaderRow(document,
                        new string[] { "Name", "Description", "Type" }));
                    nestedtable.AppendChild(nestedheader);

                    foreach (NantChildElement child in task.ChildElements)
                    {
                        nestedtable.AppendChild(CreateTableChildRow(document, child));
                       // AttachChildDetailsSection(document, parent, child);
                    }
                }
            }
        }

        private void AttachChildDetailsSection(XmlDocument document, XmlElement parent, NantChildElement child)
        {
            NantElement element = null;
            if (SchemaData.NantElements.TryGetValue(child.Key, out element))
            {
                XmlElement childsection = XmlTopicHelper.CreateSection(document, parent, child.Name, null);
                childsection.Attributes.Append(XmlTopicHelper.CreateXmlAttribute(document, "address", child.Name));

                XmlElement childsectioncontent = document.CreateElement("content");
                childsection.AppendChild(childsectioncontent);

                XmlNode typenode = document.CreateElement("para");
                string mappedkey = LinkKey(child);
                XmlNode linknode = CreateLinkNode(document, mappedkey, false);
                if (linknode == null)
                {
                    typenode.InnerText = "Type: " + child.Key;
                }
                else
                {
                    typenode.AppendChild(linknode);
                    typenode.InnerXml = "Type: " + typenode.InnerXml;
                }
                childsectioncontent.AppendChild(typenode);

                if (child.Description != null)
                {
                    XmlNodeList contentnodes = child.Description.SelectNodes("section/content");
                    foreach (XmlNode node in contentnodes)
                    {
                        childsectioncontent.AppendChild(childsectioncontent.OwnerDocument.ImportNode(node, true));
                    }
                }
                else if (child.Summary != null)
                {
                    XmlElement para2 = document.CreateElement("para");
                    para2.InnerText = child.Summary;
                    childsectioncontent.AppendChild(para2);
                }

                if (element.Attributes != null && element.Attributes.Count > 0)
                {
                    /*XmlElement section1 = XmlTopicHelper.CreateSection(document, devConDoc, "Parameters", null);

                    XmlElement section1content = document.CreateElement("content");
                    section1.AppendChild(section1content);*/

                    XmlElement parametertable = document.CreateElement("table");
                    childsectioncontent.AppendChild(parametertable);

                    XmlElement paratableheader = document.CreateElement("tableHeader");
                    paratableheader.AppendChild(CreateTableHeaderRow(document,
                        new string[] { "Attribute", "Description", "Type", "Required" }));
                    parametertable.AppendChild(paratableheader);

                    foreach (NantAttribute attr in element.Attributes)
                    {
                        parametertable.AppendChild(CreateTableAttributeRow(document, attr));
                    }
                }

                XmlNode link_to_top_para = document.CreateElement("para");
                childsectioncontent.AppendChild(link_to_top_para);

                XmlNode link_to_top = CreateLinkNode(document, "nestedelements", true);
                link_to_top.InnerXml = "&lt;&lt; Back to Nested Elements";
                link_to_top_para.AppendChild(link_to_top);
                
                /*AttachTaskComments(element, childsectioncontent, document);*/
            }
        }

        private XmlElement CreateTableHeaderRow(XmlDocument doc, String[] values)
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

        private XmlElement CreateTableAttributeRow(XmlDocument doc, NantAttribute attr)
        {
            XmlElement row = doc.CreateElement("row");

            // Name
            XmlElement entry1 = doc.CreateElement("entry");
            row.AppendChild(entry1);

            XmlElement para1 = doc.CreateElement("para");
            para1.InnerText = attr.AttributeName;
            entry1.AppendChild(para1);

            // Description
            XmlElement entry2 = doc.CreateElement("entry");
            row.AppendChild(entry2);

            if (attr.Description != null)
            {
                XmlNodeList contentnodes = attr.Description.SelectNodes("section/content");
                foreach (XmlNode node in contentnodes)
                {
                    entry2.AppendChild(entry2.OwnerDocument.ImportNode(node, true));
                }
            }
            else if (attr.Summary != null)
            {
                XmlElement para2 = doc.CreateElement("para");
                para2.InnerText = attr.Summary;
                entry2.AppendChild(para2);
            }


            // ClassType Name
            XmlElement entry3 = doc.CreateElement("entry");
            row.AppendChild(entry3);

            XmlElement para3 = doc.CreateElement("para");
            para3.InnerText = attr.ClassType.Name;
            entry3.AppendChild(para3);

            // IsRequired
            XmlElement entry4 = doc.CreateElement("entry");
            row.AppendChild(entry4);

            XmlElement para4 = doc.CreateElement("para");
            para4.InnerText = attr.IsRequired.ToString();
            entry4.AppendChild(para4);
            
            return row;
        }

        private XmlElement CreateTableChildRow(XmlDocument doc, NantChildElement child)
        {
            XmlElement row = doc.CreateElement("row");

            // Name
            XmlElement entry1 = doc.CreateElement("entry");
            entry1.AppendChild(CreateLinkNode(doc, child.Key, false, text:child.Name));
            row.AppendChild(entry1);

            // Description
            XmlElement entry2 = doc.CreateElement("entry");
            row.AppendChild(entry2);

            if (child.Description != null)
            {
                XmlNodeList contentnodes = child.Description.SelectNodes("section/content");
                foreach (XmlNode node in contentnodes)
                {
                    entry2.AppendChild(entry2.OwnerDocument.ImportNode(node, true));
                }
            }
            else if (child.Summary != null)
            {
                XmlElement para2 = doc.CreateElement("para");
                para2.InnerText = child.Summary;
                entry2.AppendChild(para2);
            }


            // Key
            XmlElement entry3 = doc.CreateElement("entry");
            row.AppendChild(entry3);

            string mappedkey = LinkKey(child);
            if (mappedkey != null)
            {
                XmlNode linknode = CreateLinkNode(doc, mappedkey, false);
                if (linknode == null)
                {
                    linknode = doc.CreateElement("para");
                }
                linknode.InnerText = child.Key;
                entry3.AppendChild(linknode);
            }
            else
            {
                XmlElement para3 = doc.CreateElement("para");
                para3.InnerText = child.Key;
                entry3.AppendChild(para3);
            }

            return row;
        }

        private string LinkKey(NantChildElement child)
        {
            if (SchemaData.NantTasks.ContainsKey(child.Key))
            {
                return child.Key;
            }

            if (child.Key == "NAnt.Core.FileSet")
            {
                return "NAnt.Core.Tasks.FileSetTask";
            }
            else if (child.Key == "NAnt.Core.OptionSet")
            {
                return "NAnt.Core.Tasks.OptionSetTask";
            }
            else if (child.Key == "NAnt.Core.PropertyElement")
            {
                return "NAnt.Core.Tasks.PropertyTask";
            }
            return null;
        }

        private XmlNode CreateLinkNode(XmlDocument doc, string key, bool local, string text=null)
        {
            if (key != null)
            {
                XmlElement linknode = doc.CreateElement("link");
                XmlAttribute attribute = doc.CreateAttribute("xlink", "href", "http://www.w3.org/1999/xlink");
                linknode.Attributes.Append(attribute);
                linknode.InnerText = (text != null)? text:key;
                if (local)
                {
                    attribute.Value = "#" + key;
                    return linknode;
                }
                else if (Guids.ContainsKey(key))
                {
                    attribute.Value = Guids[key];
                    return linknode;
                }
            }
            return null;
        }

        private string GetWorkingFolder(BuildProcess builder, bool isTask)
        {
            if (isTask)
            {
                return Path.Combine(builder.WorkingFolder, "FrameworkDocsPlugin", "Tasks");
            }
            else
            {
                return Path.Combine(builder.WorkingFolder, "FrameworkDocsPlugin", "Elements");
            }
        }


    }
}
