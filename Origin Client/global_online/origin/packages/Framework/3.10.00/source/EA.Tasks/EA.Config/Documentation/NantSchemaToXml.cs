using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.IO;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{
    public class NantSchemaToXml
    {
        private readonly Log Log;
        private readonly string LogPrefix;
        private readonly bool ExcludeComments;
        private readonly SchemaMetadata Data;

        private const int INDENT_SIZE = 4;


        public NantSchemaToXml(Log log, string logprefix, SchemaMetadata data, bool excludecomments = false)
        {
            Log = log;
            LogPrefix = logprefix;
            Data = data;
            ExcludeComments = excludecomments;
        }

        public void GenerateXMLFromNantSchema(string file, bool bynamespaces = false)
        {
            if (bynamespaces)
            {
                var nantTasksInNamespaces = Data.SortTasksByNamespaces();

                foreach (var e in nantTasksInNamespaces)
                {
                    var namespFile = Path.Combine(Path.GetDirectoryName(file), Path.GetFileNameWithoutExtension(file) + "_" + e.Key + Path.GetExtension(file));
                    GenerateXMLFromNantSchema(e.Value, namespFile);
                }
            }
            else
            {
                GenerateXMLFromNantSchema(Data.NantTasks.Values, file);
            }
        }

        private void GenerateXMLFromNantSchema(IEnumerable<NantElement> tasks, string file)
        {
            using (var writer = CreateXmlWriter(file))
            {
                writer.WriteStartDocument();
                writer.WriteStartElement("project");

                foreach (var task in tasks)
                {
                    if (task.IsSchemaElement)
                    {
                        writer.WriteWhitespace(Environment.NewLine);
                        writer.WriteWhitespace(Environment.NewLine);
                        WriteNantTask(writer, task);
                    }
                }

                writer.WriteFullEndElement();
                WriteIndentedNewLine(writer, 0);
                writer.WriteEndDocument();
            }
        }


        private void WriteNantTask(XmlWriter writer, NantElement task)
        {
            int indent = 1;

            var description = new StringBuilder();
            WriteTaskDescription(description, task, indent);
            WriteAttributesDescription(description, task, indent);
            WriteComment(writer, "                                                                               ", indent);
            WriteComment(writer, description.ToString(), indent);

            writer.WriteStartElement(task.ElementName);
            WriteElementCore(writer, task, indent);
            writer.WriteFullEndElement();

        }

        private void WriteChildElement(XmlWriter writer, NantElement parent, NantChildElement childElement, int indent)
        {
            var description = new StringBuilder();

            if (childElement.Key == parent.Key)
            {
                //To avoid infinite recursion:
                WriteIndentedNewLine(writer, indent);
                writer.WriteElementString(childElement.Name, String.Empty);
                WriteIndentedNewLine(writer, indent);
            }
            else
            {
                WriteNestedElementDescription(description, childElement, indent);

                NantElement element;

                if (Data.NantElements.TryGetValue(childElement.Key, out element))
                {
                    WriteAttributesDescription(description, element, indent);

                    WriteComment(writer, description.ToString(), indent);

                    writer.WriteStartElement(childElement.Name);

                    WriteElementCore(writer, element, indent);
                }
                else
                {
                    WriteComment(writer, description.ToString(), indent);

                    writer.WriteStartElement(childElement.Name);
                    WriteIndentedNewLine(writer, indent + 1);
                    writer.WriteComment(String.Format("ERROR: Nested Element type not found: {0}", childElement.Key));
                    WriteIndentedNewLine(writer, indent);
                }
                writer.WriteFullEndElement();
            }
            
        }

        private void WriteComment(XmlWriter writer, String text, int indent, bool forceinclude=false)
        {
                var prefix = String.Empty.PadLeft(INDENT_SIZE * indent);
                if (!ExcludeComments||forceinclude)
                {
                    writer.WriteWhitespace(Environment.NewLine);
                    writer.WriteWhitespace(prefix);
                    writer.WriteComment(text);
                }
                writer.WriteWhitespace(Environment.NewLine);
                writer.WriteWhitespace(prefix);
        }

        private void WriteTaskDescription(StringBuilder output, NantElement element, int indent)
        {
            var prefix = String.Empty.PadLeft(INDENT_SIZE * indent);
            output.AppendFormat(prefix + "Task: '{0}'", element.ElementName); output.AppendLine();
            output.AppendFormat(prefix + "Task: '{0}'", element.ElementName); output.AppendLine();
            prefix += String.Empty.PadLeft(INDENT_SIZE);
            output.AppendFormat(prefix + "Description   : '{0}'", element.Description); output.AppendLine();
        }


        private void WriteNestedElementDescription(StringBuilder output, NantChildElement childElement, int indent)
        {
            var prefix = String.Empty.PadLeft(INDENT_SIZE * indent);
            output.AppendLine();
            output.AppendFormat(prefix + "Element: '{0}'", childElement.Name); output.AppendLine();
            prefix += String.Empty.PadLeft(INDENT_SIZE);
            var summary = childElement.Summary;
            if (String.IsNullOrEmpty(summary))
            {
                //Take description from the referenced element
                NantElement referencedElement;
                if (Data.NantElements.TryGetValue(childElement.Key, out referencedElement))
                {
                    summary = referencedElement.Summary;
                }
            }
            output.AppendFormat(prefix + "Description : '{0}'", childElement.Description); output.AppendLine();
            output.AppendFormat(prefix + "Type        : '{0}'", childElement.TypeDescription); output.AppendLine();
            if (childElement.ValidValues != null)
            {
                output.AppendFormat(prefix + "Valid Values: '{0}'", childElement.ValidValues.Names.ToString("; ")); output.AppendLine();
            }
        }

        private void WriteAttributesDescription(StringBuilder output, NantElement element, int indent)
        {
            var prefix = String.Empty.PadLeft(INDENT_SIZE * (indent + 1));

            output.AppendFormat(prefix + "Strict AttributeCheck : '{0}'", element.IsStrict); output.AppendLine();
            output.AppendFormat(prefix + "Has Text Value        : '{0}'", element.IsMixed); output.AppendLine();
            output.AppendLine();
            foreach (var attr in element.Attributes)
            {
                prefix = String.Empty.PadLeft(INDENT_SIZE * (indent + 1));
                output.AppendFormat(prefix + "Attribute :  '{0}'", attr.AttributeName); output.AppendLine();
                prefix += String.Empty.PadLeft(INDENT_SIZE);
                output.AppendFormat(prefix + "Description   :  '{0}'", attr.Description); output.AppendLine();
                output.AppendFormat(prefix + "Type          :  '{0}'", attr.TypeDescription); output.AppendLine();
                output.AppendFormat(prefix + "IsRequired    :  '{0}'", attr.IsRequired); output.AppendLine();
                if (attr.ValidValues != null)
                {
                    output.AppendFormat(prefix + "Valid Values  :  '{0}'", attr.ValidValues.Names.ToString("; ")); output.AppendLine();
                }
            }
        }

        private void WriteElementCore(XmlWriter writer, NantElement element, int indent)
        {
            foreach (var attr in element.Attributes)
            {
                var value = String.Empty;
                if (attr.ValidValues != null)
                {
                    value = attr.ValidValues.Names.ToString("; ");
                }
                else if (attr.ClassType == typeof(bool))
                {
                    value = "boolean expression";
                }

                writer.WriteAttributeString(attr.AttributeName, value);
            }

            if (element.IsTaskContainer)
            {
                // For containers we dont want to write every task.
                WriteComment(writer, "Task container: Arbitrary nant script here", indent + 1, forceinclude: true);
            }
            else
            {
                foreach (var nested in element.ChildElements)
                {
                    WriteChildElement(writer, element, nested, indent + 1);
                }
            }

            var prefix = Environment.NewLine + String.Empty.PadLeft(INDENT_SIZE * indent);
            if (element.IsMixed)
            {
                writer.WriteString(prefix + String.Empty.PadLeft(INDENT_SIZE * 2) + "Text Value is allowed" + prefix);
            }
            else
            {
                writer.WriteString(prefix);
            }
        }

        private XmlWriter CreateXmlWriter(string filepath)
        {
            string xmlPath = filepath;

            string targetDir = Path.GetDirectoryName(Path.GetFullPath(xmlPath));
            if (String.IsNullOrEmpty(targetDir))
            {
                Directory.CreateDirectory(targetDir);
            }

            XmlWriterSettings writerSettings = new XmlWriterSettings();
            writerSettings.Encoding = Encoding.Unicode;
            writerSettings.Indent = true;
            writerSettings.IndentChars = String.Empty.PadLeft(INDENT_SIZE);
            writerSettings.NewLineOnAttributes = true;
            writerSettings.NewLineChars = Environment.NewLine;
            writerSettings.CloseOutput = true;

            return XmlTextWriter.Create(filepath, writerSettings);
        }

        private static void WriteIndentedNewLine(XmlWriter writer, int indent)
        {
            writer.WriteWhitespace(Environment.NewLine);
            writer.WriteWhitespace(String.Empty.PadLeft(INDENT_SIZE* indent));
        }
    }
}
