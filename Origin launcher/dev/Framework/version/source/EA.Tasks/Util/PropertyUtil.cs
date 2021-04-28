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
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;

namespace EA.Eaconfig
{
	public class PropertyUtil
	{

		public static bool PropertyExists(Project project, string name)
		{
			return project.Properties.Contains(name);
		}

		public static string SetPropertyIfMissing(Project project, string name, string defaultValue)
		{
			string val = project.Properties[name];

			if (val == null)
			{
				project.Properties[name] = val = defaultValue;
			}
			return val;
		}

		public static bool GetBooleanProperty(Project project, string name)
		{
			bool ret = false;
			string val = project.Properties[name];
			if (!String.IsNullOrEmpty(val))
			{
				if (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
				{
					ret = true;
				}
			}
			return ret;
		}

		public static bool GetBooleanPropertyOrDefault(Project project, string name, bool defaultVal)
		{
            return StringUtil.GetBooleanValueOrDefault(project.Properties[name], defaultVal);
		}

		public static string GetPropertyOrDefault(Project project, string name, string defaultValue)
		{
			string val = project.Properties[name];

			if (val == null)
			{
				val = defaultValue;
			}
			return val;
		}

		public static bool IsPropertyEqual(Project project, string name, string value)
		{
			bool ret = false;
			string val = project.Properties[name];
			if (!String.IsNullOrEmpty(val))
			{
				if (val.Trim().Equals(value, StringComparison.Ordinal))
				{
					ret = true;
				}
			}
			else
			{
				if (string.IsNullOrEmpty(value))
				{
					ret = true;
				}
			}
			return ret;
		}

		public static string AddNonDuplicatePathToNormalizedPathListProp(string currentNormalizedPathProperty, string newInfo)
		{
			// Convert currentNormalizedPathProperty to StringBuilder and StringCollection
			StringBuilder newProperty = new StringBuilder();
			System.Collections.Specialized.StringCollection newPathList = new System.Collections.Specialized.StringCollection();
			if (!String.IsNullOrEmpty(currentNormalizedPathProperty))
			{
				newProperty = new StringBuilder(currentNormalizedPathProperty);
				newPathList.AddRange(currentNormalizedPathProperty.Split(new char[] { ';', '\n' }));
			}
			// Now Add the new info to the property
			if (!String.IsNullOrEmpty(newInfo))
			{
				foreach (string line in newInfo.Split(new char[] { ';', '\n' }))
				{
					string trimmed = line.Trim();
					if (trimmed.Length > 0)
					{
						string normalizedPath = NAnt.Core.Util.PathNormalizer.Normalize(trimmed);
						bool foundPath = false;
						// Check and see if this path is already added to the list.
						// This is a linear search, but we can't re-sort this list in case build
						// script include paths were setup with expectation of certain paths 
						// taking precedence over another path.  We just append the list as is.
						foreach (string pathInCollection in newPathList)
						{
							// We need to trim the path here as well because the end of the string
							// might contain a \r character which also needs to be stripped out.
							string trimmedPathInCollection = pathInCollection.Trim();
							if (trimmedPathInCollection == normalizedPath)
							{
								foundPath = true;
							}
						}
						if (!foundPath)
						{
							newProperty.AppendLine(normalizedPath);
							newPathList.Add(normalizedPath);
						}
					}
				}
			}
			if (newProperty.Length > 0)
			{
				return newProperty.ToString();
			}
			else
			{
				return currentNormalizedPathProperty;
			}            
		}
	}
}


