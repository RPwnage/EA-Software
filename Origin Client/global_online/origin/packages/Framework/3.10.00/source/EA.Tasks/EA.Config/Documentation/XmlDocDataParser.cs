using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.Reflection;
using System.IO;

using NAnt.Core.Logging;
using System.Xml.Xsl;

namespace EA.Eaconfig
{
    public class XmlDocDataParser
    {
        private readonly Log Log;
        private readonly string LogPrefix;
        private readonly IDictionary<string, string> ElementSummaries = new Dictionary<string, string>();
        private readonly IDictionary<string, XmlNode> ElementDescriptions = new Dictionary<string, XmlNode>();
        private XslCompiledTransform transform;

        public XmlDocDataParser(Log log, string logprefix)
        {
            Log = log;
            LogPrefix = logprefix;
        }

        public string GetClassSummary(Type classType)
        {
            string description = String.Empty;
            string key = GetObjectKey(classType);
            ElementSummaries.TryGetValue(key, out description);
            return description;
        }

        public XmlNode GetClassDescription(Type classType)
        {
            XmlNode description = null;
            string key = GetObjectKey(classType);
            ElementDescriptions.TryGetValue(key, out description);
            return description;
        }

        public XmlNode GetMethodDescription(NantFunction function)
        {
            XmlNode description = null;
            string key = String.Format("m:{0}", function.ToString()).ToLowerInvariant();
            ElementDescriptions.TryGetValue(key, out description);
            return description;
        }

        public string GetClassPropertySummary(Type classType, PropertyInfo propertyInfo)
        {
            string description = String.Empty;

            Type currentType = classType;
            while (currentType != null && currentType != typeof(System.Object))
            {
                string key = GetObjectKey(currentType, propertyInfo); 

                if (ElementSummaries.TryGetValue(key, out description))
                {
                    break;
                }
                currentType = currentType.BaseType;
            }

            return description;
        }

        public XmlNode GetClassPropertyDescription(Type classType, PropertyInfo propertyInfo)
        {
            XmlNode description = null;

            Type currentType = classType;
            while (currentType != null && currentType != typeof(System.Object))
            {
                string key = GetObjectKey(currentType, propertyInfo); 

                if (ElementDescriptions.TryGetValue(key, out description))
                {
                    break;
                }
                currentType = currentType.BaseType;
            }

            return description;
        }

        /// <summary>Provides an xslt file that gets applied to all parsed xml files</summary>
        public void SetXsltTransform(XslCompiledTransform xsltfile)
        {
            transform = xsltfile;
        }

        public void ParseXMLDocumentation(string filename)
        {
            try
            {
                Console.WriteLine(filename);
                if (File.Exists(filename))
                {
                    XmlDocument doc = new XmlDocument();
                    doc.Load(filename);

                    XmlNodeList list1 = doc.SelectNodes("//members/member/summary");
                    foreach (XmlNode node in list1)
                    {
                        if (node.ParentNode != null && node.ParentNode.Attributes.GetNamedItem("name") != null)
                        {
                            string name = node.ParentNode.Attributes.GetNamedItem("name").InnerText.ToLower();
                            ElementSummaries[name] = node.InnerText;
                        }
                    }

                    if (transform != null)
                    {
                        String result = TransformXml(doc, transform);
                        doc.LoadXml(result);
                    }

                    XmlNodeList list2 = doc.SelectNodes("//members/member");
                    foreach (XmlNode node in list2)
                    {
                        if (node.Attributes.GetNamedItem("name") != null)
                        {
                            string name = node.Attributes.GetNamedItem("name").InnerText.ToLower();
                            ElementDescriptions[name] = node;
                        }
                    }
                }
                else
                {
                    if (Log != null)
                    {
                        Log.Debug.WriteLine("XML documentation file '{0}' does not exist", filename);
                    }
                }

            }
            catch (Exception e)
            {
                if (Log != null)
                {
                    Log.Warning.WriteLine("Failed to load XML doc file '{0}', reason: {1}", filename, e.Message);
                }
            }
        }

        private string GetObjectKey(Type type, PropertyInfo pinfo = null)
        {
            var key = (pinfo != null) ? String.Format("P:{0}.{1}", type.FullName, pinfo.Name) : String.Format("t:{0}", type.FullName);

            return key.Replace('+', '.').ToLowerInvariant();
        }

        private string TransformXml(XmlDocument doc, XslCompiledTransform xslt)
        {
            var stm = new MemoryStream();
            xslt.Transform(doc, null, stm);
            stm.Position = 1;
            var sr = new StreamReader(stm, true);
            string result = sr.ReadToEnd();
            int index = result.IndexOf('<');
            if (index > 0)
            {
                result = result.Substring(index, result.Length - index);
            }
            return result;
        }
    }
}
