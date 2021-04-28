// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.IO;

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
		
		/// <summary>Gets a child element or adds it if missing. If no namespace is provided it will
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

		public static string GetXmlNodeText(this XmlNode el, bool TrimWhiteSpace = true)
		{
			var text = new StringBuilder();
			if (el != null)
			{
				foreach (XmlNode child in el)
				{
					if (child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA)
					{
						if (TrimWhiteSpace)
						{
							text.AppendLine(child.InnerText.TrimWhiteSpace());
						}
						else
						{
							text.AppendLine(child.InnerText);
						}
					}
				}
			}
			return text.ToString();
		}

		public static string GetFormattedInnerXml(this XmlNode el, XmlWriterSettings settings = null)
		{
			using (var text = new StringWriter())
			{
				if (el != null)
				{
					if (settings == null)
					{
						settings = new XmlWriterSettings();
						settings.Encoding = new UTF8Encoding(false);
						settings.Indent = true;
						settings.IndentChars = new string(' ', 2);
						settings.NewLineOnAttributes = false;
						settings.CloseOutput = true;
						settings.ConformanceLevel = ConformanceLevel.Fragment;
					}

					using (var w = XmlTextWriter.Create(text, settings))
					{
						el.WriteContentTo(w);
					}

				}
				return text.ToString();
			}
		}

	}
}
