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
using System.Linq;
using System.Xml;

using NAnt.Core.Logging;

namespace NAnt.Core.Util
{
	internal static class XmlUtil
	{
		internal static string GetRequiredAttribute(XmlNode el, string name)
		{
			XmlAttribute attr = el.Attributes[name];

			if (attr == null)
			{
				Error(el, String.Format("XML element '{0}' is missing required attribute '{1}'.", el.Name, name));
			}
			if (String.IsNullOrEmpty(attr.Value))
			{
				Error(el, String.Format("required attribute '{0}' in XML element '{1}' is defined but has no value.", name, el.Name));
			}
			return attr.Value.TrimWhiteSpace();
		}

		internal static string GetOptionalAttribute(XmlNode el, string name, string defaultVal = null)
		{
			XmlAttribute attr = el.Attributes[name];
			if (attr != null)
			{
				if (!String.IsNullOrEmpty(attr.Value))
				{
					return attr.Value.TrimWhiteSpace();
				}
				else
				{
					return String.Empty;
				}
			}
			return defaultVal;
		}


		internal static bool ToBoolean(string value, XmlNode node = null) => ConvertUtil.ToBoolean(value, Location.GetLocationFromNode(node));
		internal static bool ToBooleanOrDefault(string value, bool defaultValue, XmlNode node = null) => ConvertUtil.ToBooleanOrDefault(value, defaultValue, Location.GetLocationFromNode(node));
		internal static bool? ToNullableBoolean(string value, XmlNode node = null) => ConvertUtil.ToNullableBoolean(value, Location.GetLocationFromNode(node));

		internal static void Error(XmlNode el, string msg)
		{
			throw new BuildException(msg, Location.GetLocationFromNode(el));
		}

		internal static void Warning(Log log, XmlNode el, string msg)
		{
			string locationString = Location.GetLocationFromNode(el).ToString();
			msg = locationString != String.Empty ? locationString + " " + msg : msg;
			log.Warning.WriteLine(msg);
		}

		internal static void ValidateAttributes(XmlNode childNode, params string[] validNames)
		{
			foreach (XmlAttribute attribute in childNode.Attributes)
			{
				if (!validNames.Contains(attribute.Name))
				{
					Error
					(
						childNode,
						$"XML element '{childNode.Name}' has invalid attribute '{attribute.Name}'. " +
						$"Valid attributes are {String.Join(", ", validNames.Select(name => $"'{name}'"))}."
					);
				}
			}
		}
	}
}