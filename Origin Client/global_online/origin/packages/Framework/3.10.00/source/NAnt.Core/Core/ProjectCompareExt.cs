using System;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Xml;
using System.Text;
using System.Xml.Xsl;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Threading;

namespace NAnt.Core
{
    public static class ProjectComparerExt
    {
        public static string ToComparableString(this Project project)
        {
            var props = from de in project.Properties.Cast<DictionaryEntry>()
                        let prop = (Property)de.Value
                        orderby prop.Name
                        select ToCompareString(prop);

            var targets = from target in project.Targets
                          orderby target.Key
                          select ToCompareString(target.Value);

            var filesets = from de in project.NamedFileSets.Cast<DictionaryEntry>()
                           let fileset = (FileSet)de.Value
                           let key = (string)de.Key
                           orderby key
                           select ToCompareString(key, fileset);

            var optionsets = from de in project.NamedOptionSets.Cast<DictionaryEntry>()
                             let optionset = (OptionSet)de.Value
                             let key = (string)de.Key
                             orderby key
                             select ToCompareString(key, optionset);

            var combined = String.Join("\n", props.Concat(targets.Concat(filesets.Concat(optionsets))).ToArray());

            return combined.Replace("\r", "");
        }

        private static string ToCompareString(Property v) { return String.Format("<property name=\"{0}\" value=\"{1}\"/>", v.Name, String.Join(" ", (v.Value ?? "").Split(default(string[]), StringSplitOptions.RemoveEmptyEntries))); }
        private static string ToCompareString(Target v) { return String.Format("<target name=\"{0}\" dependencies=\"{1}\"/>", v.Name, String.Join(" ", v.Dependencies.Cast<string>().ToArray())); }
        private static string ToCompareString(string name, OptionSet v)
        {
            var options = from de in v.Options.Cast<DictionaryEntry>()
                          let key = (string)de.Key
                          let value = (string)de.Value
                          orderby key
                          select String.Format("<option name=\"{0}\" value=\"{1}\"/>", key, value);

            return String.Format("<optionset name=\"{0}\">{1}{2}\n</optionset>", name, NewLineAndIndent, String.Join(NewLineAndIndent, options.ToArray()));
        }

        private static string ToCompareString(string name, FileSet v)
        {
            var files = from fi in v.FileItems.Cast<FileItem>()
                        orderby fi.FullPath
                        select String.Format("<file path=\"{0}\"/>", fi.FullPath);

            return String.Format("<fileset name=\"{0}\">{1}{2}\n</fileset>", name, NewLineAndIndent, String.Join(NewLineAndIndent, files.ToArray()));
        }

        private const int Indent = 4;
        private static readonly string NewLineAndIndent = "\n" + "".PadLeft(Indent);
    }

    internal static class XmlWriterExtensions
    {
        internal static void WriteNAntProperies(this XmlWriter writer, IEnumerable<Property> properties)
        {
            if (writer != null && properties != null)
            {
                foreach (var property in properties.OrderBy(p => p.Name))
                {
                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", property.Name);
                    writer.WriteString(property.Value);
                    writer.WriteEndElement();
                }
            }
        }

        internal static void WriteNAntFileSets(this XmlWriter writer, IEnumerable<KeyValuePair<string, FileSet>> filesets)
        {
            if (writer != null && filesets != null)
            {
                foreach (var fe in filesets.OrderBy(e => e.Key))
                {
                    writer.WriteStartElement("fileset");
                    writer.WriteAttributeString("name", fe.Key);
                    foreach (var file in fe.Value.FileItems)
                    {
                        writer.WriteStartElement("file");
                        writer.WriteString(file.FullPath);
                        writer.WriteEndElement();
                    }
                    writer.WriteEndElement();
                }
            }
        }

        internal static void WriteNAntOptionSets(this XmlWriter writer, IEnumerable<KeyValuePair<string, OptionSet>> optionsets)
        {
            if (writer != null && optionsets != null)
            {
                foreach (var oe in optionsets.OrderBy(e => e.Key))
                {
                    writer.WriteStartElement("optionset");
                    writer.WriteAttributeString("name", oe.Key);
                    foreach (var option in oe.Value.Options)
                    {
                        writer.WriteStartElement("option");
                        writer.WriteAttributeString("name", option.Key);
                        writer.WriteAttributeString("value", option.Value);
                        writer.WriteEndElement();
                    }
                    writer.WriteEndElement();
                }
            }
        }

    }
}
