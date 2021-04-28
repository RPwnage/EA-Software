using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace NAnt.Core.Util
{
    public static class XmlExtensions
    {
        /// <summary>Loops through immediate child elements and returns all children that matches the given name</summary>
        public static List<XmlNode> GetChildElementsByName(this XmlNode parent, string elementName)
        {
            List<XmlNode> nodes = new List<XmlNode>();
            if (parent != null)
            {
                foreach (XmlNode child in parent.ChildNodes)
                {
                    if (child.Name == elementName)
                    {
                        nodes.Add(child);
                    }
                }
            }
            return nodes;
        }

        /// <summary>Loops through immediate child elements and returns the first that matches the given name</summary>
        public static XmlNode GetChildElementByName(this XmlNode parent, string elementName)
        {
            if (parent != null)
            {
                foreach (XmlNode child in parent.ChildNodes)
                {
                    if (child.Name == elementName)
                    {
                        return child;
                    }
                }
            }
            return null;
        }

        /// <summary>Loops through immediate child elements and returns the first that match the given name 
        /// and has the given attribute</summary>
        public static XmlNode GetChildElementByNameAndAttribute(this XmlNode parent, string elementName, 
            string attributeName, string attributeValue)
        {
            if (parent != null)
            {
                foreach (XmlNode child in parent.ChildNodes)
                {
                    if (child.Name == elementName 
                        && child.Attributes[attributeName] != null
                        && child.Attributes[attributeName].Value != null
                        && child.Attributes[attributeName].Value.Trim() == attributeValue.Trim())
                    {
                        return child;
                    }
                }
            }
            return null;
        }
        
        /// <summary>Gets a child element or adds it if missing. If no namepsace is provided it will
        /// only search immediate child nodes.</summary>
        public static XmlNode GetOrAddElement(this XmlDocument doc, string elementName, string namespaceURI = null)
        {
            if (doc != null && !String.IsNullOrEmpty(elementName))
            {
                XmlNode node = null;
                if (namespaceURI == null)
                {
                    node = doc.GetChildElementByName(elementName);
                }
                else
                {
                    XmlNamespaceManager nsmgr = new XmlNamespaceManager(doc.NameTable);
                    nsmgr.AddNamespace("msb", namespaceURI);

                    node = doc.SelectSingleNode("/msb:" + elementName, nsmgr);
                }
                if (node == null)
                {
                    node = namespaceURI == null ? doc.CreateElement(elementName): doc.CreateElement(elementName, namespaceURI);

                    doc.AppendChild(node);
                }
                return node;
            }
            throw new BuildException("XmlDocument.GetOrAddDocElement(doc, elementName) - input parameters are null or empty");
        }

        /// <summary>Searches immediate child elements for one with the given name and adds it if not found.</summary>
        public static XmlNode GetOrAddElement(this XmlNode parent, string elementName, string namespaceURI = null)
        {
            if (parent != null && !String.IsNullOrEmpty(elementName))
            {
                var node = parent.GetChildElementByName(elementName);
                if (node == null)
                {
                    node = parent.OwnerDocument.CreateElement(elementName, parent.NamespaceURI);
                    parent.AppendChild(node);
                }
                return node;
            }
            throw new BuildException("XmlDocument.GetOrAddElement(node, elementName) - input parameters are null or empty");
        }

        /// <summary>Searches immediate child elements for one with the given name and attribute and adds it if not found</summary>
        public static XmlNode GetOrAddElementWithAttributes(this XmlNode parent, string elementName, string attribName, string attribValue)
        {
            if (parent != null && !String.IsNullOrEmpty(elementName))
            {
                var node = parent.GetChildElementByNameAndAttribute(elementName, attribName, attribValue);
                if (node == null)
                {
                    node = parent.OwnerDocument.CreateElement(elementName, parent.NamespaceURI);
                    parent.AppendChild(node);
                    node.SetAttribute(attribName, attribValue);
                }
                return node;
            }
            throw new BuildException("XmlDocument.GetOrAddElement(node, elementName) - input parameters are null or empty");
        }

        /// <summary>Sets an attribute value of a node, or adds a new attribute if an attribute
        /// by the given name does not exist.</summary>
        public static bool SetAttribute(this XmlNode node, string attrName, string value)
        {
            bool ret = false;
            if (node != null && !String.IsNullOrEmpty(attrName))
            {
                var attr = node.Attributes.GetNamedItem(attrName);
                if (attr != null)
                {
                    attr.Value = value;
                }
                else                
                {
                    var newAttr = node.OwnerDocument.CreateAttribute(attrName);
                    newAttr.Value = value ?? String.Empty;
                    node.Attributes.Append(newAttr);
                    ret = true;
                }
            }
            return ret;
        }

        public static bool SetAttributeIfValueNonEmpty(this XmlNode node, string attrName, string value)
        {
            bool ret = false;
            if (node != null && !String.IsNullOrEmpty(attrName))
            {
                var attr = node.Attributes.GetNamedItem(attrName);
                if (attr != null && !String.IsNullOrEmpty(value))
                {
                    attr.Value = value;
                }
                else
                {
                    var newAttr = node.OwnerDocument.CreateAttribute(attrName);
                    newAttr.Value = value ?? String.Empty;
                    node.Attributes.Append(newAttr);
                    ret = true;
                }
            }
            return ret;
        }

        /// <summary>Sets the value of a node's attribute only if the attribute's value
        /// is equal to null.</summary>
        public static bool SetAttributeIfMissing(this XmlNode node, string attrName, string value)
        {
            bool ret = false;
            if (node != null && !String.IsNullOrEmpty(attrName))
            {
                if (node.Attributes[attrName] == null)
                {
                    var attr = node.OwnerDocument.CreateAttribute(attrName);
                    attr.Value = value ?? String.Empty;
                    node.Attributes.Append(attr);
                    ret = true;
                }
            }
            return ret;
        }

        /// <summary>Returns the value of an attribute, or a provided default value if either the
        /// node or attribute are null.</summary>
        public static string GetAttributeValue(this XmlNode node, string attribName, string defaultVal = "")
        {
            string val = defaultVal;
            if (node != null)
            {
                XmlAttribute attrib = node.Attributes[attribName];
                if (attrib != null)
                {
                    val = attrib.Value;
                }
            }
            return val;
        }

        public static string GetXmlNodeText(this XmlNode el)
        {
            var text = new StringBuilder();
            if (el != null)
            {
                foreach (XmlNode child in el)
                {
                    if ((child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA) && child.NamespaceURI.Equals(el.NamespaceURI))
                    {
                        text.AppendLine(child.InnerText.TrimWhiteSpace());
                    }
                }
            }
            return text.ToString();
        }
    }
}
