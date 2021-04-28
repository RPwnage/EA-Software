using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Linq;
using EA.Tools;

namespace FwCodeDocToMd
{
    static public class XmlToMarkdown
    {
        public class MDConfig
        {
            public List<KeyValuePair<string, string>> ConfigList = new List<KeyValuePair<string, string>>();
            public int BaseHeading = 0;

            public MDConfig(int baseHeading)
            {
                BaseHeading = baseHeading;
            }


            public string GetSetting(string setting, string defaultValue = null)
            {
                int index = ConfigList.FindLastIndex(item => item.Key == setting);
                Debug.Assert(defaultValue != null || index >= 0);
                return index >= 0 ? ConfigList[index].Value : defaultValue;
            }

            public void SetSetting(string setting, string value)
            {
                int index = ConfigList.FindLastIndex(item => item.Key == setting);
                Debug.Assert(index >= 0);
                ConfigList[index] = new KeyValuePair<string, string>(setting, value);
            }

            public void PushSetting(string setting, string value)
            {
                ConfigList.Add(new KeyValuePair<string, string>(setting, value));
            }

            public void PopSetting(string setting)
            {
                int index = ConfigList.FindLastIndex(item => item.Key == setting);
                Debug.Assert(index >= 0);
                ConfigList.RemoveAt(index);
            }

            public int CountSettings(string setting)
            {
                return ConfigList.Count(item => { return item.Key == setting; });
            }
        }

        public static readonly Dictionary<string, MDElement> MDElements = new Dictionary<string, MDElement>
        {
            {"title", new MDElement_Ignore_NoProcess() },
            {"autoOutline" , new MDElement_Ignore_NoProcess()},
            {"b", new MDElement_Bold()},
            {"legacyItalic", new MDElement_Italic()},
            {"para", new MDElement_Paragraph()},
            {"list", new MDElement_List()},
            {"item", new MDElement_ListItem()},
            {"codeInline", new MDElement_CodeInline() },
            {"c", new MDElement_CodeInline() },
            {"code", new MDElement_Code() },
            {"table", new MDElement_Table() },
            {"summary", new MDElement_Title(2, "Summary") },
            {"remarks", new MDElement_Title(2, "Remarks") },
            {"example", new MDElement_Title(3, "Example") },
            {"h1", new MDElement_Title(2) },
            {"h2", new MDElement_Title(4) },
            {"h3", new MDElement_Title(5) },
            {"h4", new MDElement_Title(6) },
            {"br", new MDElement_Break() },
            {"param", new MDElement_ElementWithPrefix("Parameter: ") },
            {"returns", new MDElement_ElementWithPrefix("Return Value: ") },

        };


        public abstract class MDElement
        {
            public abstract void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent);

            public abstract void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config);
        }

        public class MDElement_Ignore_NoProcess : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                processContent = false;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
            }
        }


        public class MDElement_ElementWithPrefix : MDElement
        {
            private string PrefixString = null;
            public MDElement_ElementWithPrefix(string prefix = null)
            {
                PrefixString = prefix;
            }

            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                var nameAttr = element.AttributeAnyNS("name");
                var name = nameAttr != null ? nameAttr.Value : "";
                writer.WriteLine("###### " + (PrefixString +" " + name).Trim() + " ######");
                processContent = true;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                writer.WriteLine();
                writer.WriteLine();
            }
        }


        public class MDElement_Title : MDElement
        {
            private int Level = 1;
            private string Name = null;
            public MDElement_Title(int level, string name = null)
            {
                Level = level;
                Name = name;
            }
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                var headingString = new string('#', Level + config.BaseHeading);
                var title = Name != null ? Name : element.Value;
                writer.WriteLine($"{headingString} {title} {headingString}");
                processContent = Name != null;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                writer.WriteLine();
                writer.WriteLine();

            }
        }

        public class MDElement_Bold : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                writer.Write("**");
                processContent = true;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                writer.Write("**");
            }
        }

        public class MDElement_Break : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                writer.WriteLine();
                writer.WriteLine();
                processContent = true;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
            }
        }

        public class MDElement_Italic : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                writer.Write("*");
                processContent = true;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                writer.Write("*");
            }
        }

        public class MDElement_Paragraph : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                processContent = true;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                writer.WriteLine("");
                writer.WriteLine("");
            }
        }

        public class MDElement_List : MDElement
        {
            MDElement_Table TableElement = new MDElement_Table();
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                var listType = element.AttributeAnyNS("type");
                if (listType != null && listType.Value.Equals("table"))
                {
                    // redirect to table
                    TableElement.Prefix(writer, element, config, out processContent);
                }
                else
                {
                    writer.WriteLine("");
                    config.PushSetting("listType", listType != null ? listType.Value : "bullet");
                    config.PushSetting("listCurrentNumber", "1");

                    processContent = true;
                }
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                var listType = element.AttributeAnyNS("type");
                if (listType != null && listType.Value.Equals("table"))
                {
                    // redirect to table
                    TableElement.Postfix(writer, element, config);
                }
                else
                {
                    config.PopSetting("listCurrentNumber");
                    config.PopSetting("listType");
                }
            }
        }

        public class MDElement_ListItem : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                var listType = config.GetSetting("listType");
                var count = config.GetSetting("listCurrentNumber");
                config.SetSetting("listCurrentNumber", (int.Parse(count) + 1).ToString());
                var indent = new String(' ', config.CountSettings("listType"));


                switch (listType)
                {
                    case "bullet":
                        writer.Write("{0}- ", indent);
                        break;
                    case "nobullet":
                        writer.Write("{0}  ", indent);
                        break;
                    case "ordered":
                        writer.Write("{0}{1}. ", indent, count);
                        break;
                }
                processContent = true;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                writer.WriteLine();
            }
        }

        public class MDElement_CodeInline : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                writer.Write(" `");
                foreach (var node in element.Nodes())
                    ExportNode(writer, node, config, true); // export code with html decoded as markdown will deal with this.
                processContent = false;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                writer.Write("` ");
            }
        }

        public class MDElement_Code : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                // See if we have a title
                var xmlTitle = element.AttributeAnyNS("title");
                if (xmlTitle != null && !string.IsNullOrEmpty(xmlTitle.Value))
                {
                    var headingString = new string('#', 5 + config.BaseHeading);
                    writer.WriteLine($"{headingString} {xmlTitle.Value} {headingString}");
                }

                var language = "xml"; // set xml as default
                var xmlLanguage = element.AttributeAnyNS("language");
                if (xmlLanguage != null && xmlLanguage.Value != "none")
                {
                    language = xmlLanguage.Value;
                }
                writer.WriteLine("\n```{0}", language);

                {
                    foreach (var node in element.Nodes())
                        ExportNode(writer, node, config, true); // export code with html decoded as markdown will deal with this.
                }

                processContent = false;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
                writer.WriteLine("\n```");
            }
        }

        public class MDElement_Table : MDElement
        {
            public override void Prefix(System.IO.TextWriter writer, XElement element, MDConfig config, out bool processContent)
            {
                // Go into manual for tables...

                // check for a title
                var xTitle = element.ElementAnyNS("title");
                if (xTitle != null)
                {
                    var headingString = new string('#', 3 + config.BaseHeading);
                    writer.WriteLine($"{headingString} {xTitle.Value} {headingString}");
                }

                // check for a header
                var xHeader = element.ElementAnyNS("listheader");
                if (xHeader != null)
                {
                    var xEntries = xHeader.Elements();
                    foreach (var xEntry in xEntries)
                    {
                        var entryString = xEntry.Value;
                        writer.Write($"{entryString} |");
                    }
                    writer.WriteLine("");
                    foreach (var xEntry in xEntries)
                    {

                        writer.Write($"--- |");
                    }
                    writer.WriteLine("");
                }
                else
                {
                    // No header, but md needs one, so do empty one
                    var xRow = element.ElementAnyNS("item");
                    var xEntries = xRow.Elements();
                    writer.Write($"|");
                    foreach (var xEntry in xEntries)
                    {
                        writer.Write("   |");
                    }
                    writer.WriteLine();
                    writer.Write($"|");
                    foreach (var xEntry in xEntries)
                    {
                        writer.Write("---|");
                    }
                    writer.WriteLine();
                }

                // process rows
                var xRows = element.ElementsAnyNS("item");
                foreach (var xRow in xRows)
                {
                    writer.Write($"| ");
                    var xEntries = xRow.Elements();
                    foreach (var xEntry in xEntries)
                    {
                        StringBuilder sb = new StringBuilder();
                        TextWriter tw = new StringWriter(sb);
                        ExportNode(tw, xEntry, config);

                        var contentString = sb.ToString();
                        // Massage the contentstring as markdown has limited table support, so we need to go into html mode"
                        // Replace all line breaks
                        contentString = MakeListSafe(contentString);
                        writer.Write($"{contentString} | ");

                    }
                    writer.WriteLine();
                }
                writer.WriteLine();
                processContent = false;
            }

            public override void Postfix(System.IO.TextWriter writer, XElement element, MDConfig config)
            {
            }
        }

        public static string MakeListSafe(string contentString)
        {
            // Replace all line breaks
            contentString = contentString.Replace("\r\n", "<br>");
            return contentString.Replace("\n", "<br>");
        }

        public static void ExportElement(System.IO.TextWriter writer, XElement element, MDConfig config, bool includeSelf = true)
        {
            // Look up this element and see if we need to do extra pre/postfix processing.
            MDElement mdElement;
            MDElements.TryGetValue(element.Name.LocalName, out mdElement);

            bool processContent = true;
            if (mdElement != null && includeSelf)
                mdElement.Prefix(writer, element, config, out processContent);

            if (processContent)
            {
                // Traverse the xml deep and write out
                foreach (var node in element.Nodes())
                {
                    ExportNode(writer, node, config);
                }
            }

            if (mdElement != null && includeSelf)
                mdElement.Postfix(writer, element, config);
        }

        public static void ExportNode(TextWriter writer, XNode node, MDConfig config, bool decodeHtml = false)
        {
            if (node is XElement)
                ExportElement(writer, (node as XElement), config);
            else
            {
                var nodeText = node.ToString();
                var text = nodeText;

                if (text.Contains('\n'))
                {
                    // Process indentation (make prettier by only allowing one extra whitespace from the previous line)
                    Func<string, int> calculateIndent = s =>
                    {
                        if (string.IsNullOrWhiteSpace(s))
                        {
                            return 0;
                        }
                        else
                        {
                            return s.Replace(s.TrimStart(), "").Count();
                        }
                    };
                    text = text.Trim();
                    var lines = new List<string>(Regex.Split(text, Environment.NewLine));
                    int baseIndent = int.MaxValue;
                    // Search for smallest indentation to get a base value
                    // Skip the first line as that was processed by xml reader
                    foreach (var line in lines.Skip(1))
                    {
                        if (!string.IsNullOrWhiteSpace(line))
                        {
                            int indent = calculateIndent(line);
                            baseIndent = indent < baseIndent ? indent : baseIndent;
                        }
                    }

                    int previousIndent = baseIndent;
                    // Fix indentation
                    lines[0] = lines[0].Trim();

                    for (int lineIndex = 1; lineIndex < lines.Count(); lineIndex++)
                    {
                        var line = lines[lineIndex];
                        if (!string.IsNullOrWhiteSpace(line))
                        {
                            int indent = line.Replace(line.TrimStart(), "").Count();
                            lines[lineIndex] = new string(' ', indent - baseIndent) + line.Trim();
                        }
                    }

                    text = string.Join("\n", lines);
                }
                else
                {
                    var trimmedStart = nodeText.TrimStart();
                    var trimmedEnd = nodeText.TrimEnd();
                    // If there was something to trim, make sure we add a space in front/end
                    text = (trimmedStart != nodeText ? " " : "") + nodeText.Trim() + (trimmedEnd != nodeText ? " " : "");
                }

                if (decodeHtml)
                {
                    text = WebUtility.HtmlDecode(text);
                }
                else
                {
                    text = WebUtility.HtmlDecode(text);
                    text = WebUtility.HtmlEncode(text);
                }

                text = text.Replace("<![CDATA[", "").Replace("]]>", "");
                writer.Write(text);
            }
        }

        static public string ToMarkDown(XElement element, bool includeSelf = true, int baseHeading = 0)
        {
            var sb = new StringBuilder();
            var tw = new StringWriter(sb);
            var config = new MDConfig(baseHeading);
            ExportElement(tw, element, config, includeSelf);

            return sb.ToString();
        }
    }
}
