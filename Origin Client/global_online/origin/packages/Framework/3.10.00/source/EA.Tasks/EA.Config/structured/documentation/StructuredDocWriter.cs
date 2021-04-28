using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace EA.Eaconfig.Structured.Documentation
{
    public static class StructuredDocWriter
    {
        public static string Indent = "\t";
        public static string BeginComment = "<!--";
        public static string EndComment = "-->";
        public static string Newline = "\n";

        private static string GenerateTagComments(StructuredDoc.Tag tag)
        {
            string emptyComment = BeginComment + Newline + EndComment + Newline;
            StringBuilder sb = new StringBuilder();

            sb.AppendLine(BeginComment);
            string prefix = "Element";
            if (tag.Type != StructuredDoc.TagType.Undefined)
                prefix = tag.Type.ToString();
            sb.AppendFormat("{0}: {1}{2}", prefix, tag.Name, Newline);

            if(!String.IsNullOrEmpty(tag.Description))
            {
                sb.AppendLine(tag.Description);
            }

            if (tag.Required)
                sb.AppendFormat("{0} is required{1}", tag.Name, Newline);
            else
                sb.AppendFormat("{0} is NOT required{1}", tag.Name, Newline);

            if (tag.Attributes.Count > 0)
            {
                sb.AppendLine();
                foreach (StructuredDoc.TagAttribute tagAttr in tag.Attributes)
                {
                    sb.AppendFormat("[indent]Attribute: {0}{1}", tagAttr.Name, Newline);
                    if (!String.IsNullOrEmpty(tagAttr.Description))
                        sb.AppendFormat("[indent][indent]{0}{1}", tagAttr.Description, Newline);
                    if (tagAttr.Required)
                        sb.AppendFormat("[indent][indent]{0} is required{1}", tagAttr.Name, Newline);
                    else
                        sb.AppendFormat("[indent][indent]{0} is NOT required{1}", tagAttr.Name, Newline);
                }
            }
            sb.AppendLine(EndComment);
            if(sb.Length != emptyComment.Length)
                return sb.ToString();
            return string.Empty;
        }

        private static string GenerateTagXml(StructuredDoc.Tag tag)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(GenerateTagComments(tag));

            sb.AppendFormat("<{0}", tag.Name);
            if (tag.Attributes.Count > 0)
            {
                foreach (StructuredDoc.TagAttribute tagAttr in tag.Attributes)
                {
                    sb.AppendFormat(" {0}=\"placeholder\"", tagAttr.Name);
                }
            }
            sb.AppendLine(">");

            if (tag.Type == StructuredDoc.TagType.FileSet)
            {
                sb.AppendLine("[indent]<includes name=\"placeholder\"/>");
            }
            else if (tag.Type == StructuredDoc.TagType.OptionSet)
            {
                sb.AppendLine("[indent]<option name=\"placeholder\" value=\"placeholder\"/>");
            }
            else if (tag.Type == StructuredDoc.TagType.Property)
            {
                sb.AppendLine("[indent]placeholder");
            }
            else
            {
                foreach (StructuredDoc.Tag nestedTag in tag.Tags)
                {
                    sb.Append(GenerateTagXml(nestedTag));
                }
            }

            sb.AppendFormat("</{0}>{1}", tag.Name, Newline);
            return FormatOutput(sb.ToString());
        }

        private static string FormatOutput(string output)
        {
            StringBuilder finalOut = new StringBuilder();
            string[] splitOut = output.Split('\n');
            foreach (string split in splitOut)
            {
                string finalSplit = split.Replace("[indent]", Indent);
                finalSplit = finalSplit.Replace("[newline]", "\n");
                finalOut.AppendFormat("{0}{1}{2}", Indent, finalSplit, "\n");
            }

            return finalOut.ToString();
        }

        public static void Write(StructuredDoc doc, string outputdir)
        {
            if (!Directory.Exists(outputdir))
                Directory.CreateDirectory(outputdir);

            foreach (StructuredDoc.Tag rootTag in doc.RootTags)
            {
                string xmlContents = GenerateTagXml(rootTag);
                string xmlPath = Path.Combine(outputdir, rootTag.Name);
                xmlPath += ".xml";
                File.WriteAllText(xmlPath, xmlContents);
            }
        }
    }
}
