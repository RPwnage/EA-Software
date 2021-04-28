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

using NAnt.Core.Logging;
using NAnt.Core.Writers;

namespace NAnt.Core.Util
{
	public class PropertiesFileLoader
	{
		public static void LoadPropertiesFromFile(Log log, string file, out Dictionary<string, string> properties, out Dictionary<string, string> globalproperties)
		{
			properties = new Dictionary<string, string>();
			globalproperties = new Dictionary<string, string>();

			LineInfoDocument doc = LineInfoDocument.Load(file);
			if (doc.DocumentElement.Name != "project")
			{
				log.Warning.WriteLine("Expected root element is 'project', found root '{0}' in the properties file {1}.", doc.DocumentElement.Name, file);
			}

			foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
			{
				if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
				{
					switch (childNode.Name)
					{
						case "properties":
							ParseProperties(log, childNode, properties, file);
							break;
						case "globalproperties":
							ParseProperties(log, childNode, globalproperties, file);
							break;
						default:
							XmlUtil.Warning(log, childNode, String.Format("Unknown XML element '{0}' in properties file. Expected element 'properties'.", childNode.Name)); 
							break;
					}
				}
			}
		}

		public static void SavePropertiesToFile(string file, IDictionary<string, string> properties, IDictionary<string, string> globalproperties)
		{
			if (properties != null)
			{
				using (IXmlWriter writer = new Writers.XmlWriter(XmlFormat.Default))
				{
					writer.FileName = file;

					writer.WriteStartDocument();

					writer.WriteStartElement("project");
					{
						writer.WriteStartElement("properties");
						{
							WriteProperties(writer, properties);
						}
						writer.WriteEndElement();

						writer.WriteStartElement("globalproperties");
						{
							WriteProperties(writer, globalproperties);
						}
						writer.WriteEndElement();
					}
					writer.WriteEndElement();
				}
			}
		}

		private static void WriteProperties(IXmlWriter writer, IDictionary<string, string> properties)
		{
			if (properties != null)
			{
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
			}
		}

		private static void ParseProperties(Log log, XmlNode node, Dictionary<string, string> properties, string file)
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
								XmlUtil.Error(childNode, "ERROR in property file: property '" + name + "' has values defined in 'value' attribute and as element text.");
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
									XmlUtil.Warning(log, childNode, String.Format("Property \"{0}\" defined more than once in properties file with different values {1} and {2}. Second value will be ignored", name, existingVal, val));
								}
							}
							else
							{
								properties.Add(name, val);
							}
							break;
						default:
							XmlUtil.Warning(log, childNode, String.Format("Unknown XML element '{0}' in properties file. Expected element 'properties'.", childNode.Name)); 
							break;
					}
				}
			}
		}
	}
}
