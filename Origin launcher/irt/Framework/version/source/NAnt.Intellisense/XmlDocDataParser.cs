// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Xml;
using System.Reflection;
using System.IO;

using NAnt.Core.Logging;
using System.Xml.Xsl;

namespace NAnt.Intellisense
{
	internal class XmlDocDataParser
	{
		private readonly Log Log;
		private readonly string LogPrefix;
		private readonly IDictionary<string, string> ElementSummaries = new Dictionary<string, string>();
		private readonly IDictionary<string, XmlNode> ElementDescriptions = new Dictionary<string, XmlNode>();
		private XslCompiledTransform transform;

		public XmlDocDataParser(Log log, string logprefix)
		{
			Log = log;
			LogPrefix = logprefix ?? "[XmlDocDataParser]";
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
					else
					{
						Console.WriteLine("XML documentation file '{0}' does not exist", filename);
					}
				}

			}
			catch (Exception e)
			{
				if (Log != null)
				{
					Log.ThrowWarning
					(
						 Log.WarningId.SystemIOError, Log.WarnLevel.Normal,
						 "Failed to load XML doc file '{0}', reason: {1}", filename, e.Message
					);
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
