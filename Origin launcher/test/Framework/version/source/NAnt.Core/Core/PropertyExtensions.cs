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


namespace NAnt.Core
{
	public static class PropertyExtensions
	{

        public static string GetPropertyOrDefault(this PropertyDictionary properties, PropertyKey key, string defaultValue)
        {
            string val = properties[key];

            if (val == null)
            {
                val = defaultValue;
            }
            return val;
        }

        public static string GetPropertyOrDefault(this PropertyDictionary properties, string name, string defaultValue)
		{
			string val = properties[name];

			if (val == null)
			{
				val = defaultValue;
			}
			return val;
		}

		public static string GetPropertyOrDefault(this Project project, string name, string defaultValue)
		{
			string val = project.Properties[name];

			if (val == null)
			{
				val = defaultValue;
			}
			return val;
		}


		public static string GetPropertyOrFail(this PropertyDictionary properties, string name)
		{
			var val = properties[name];

			if (val == null)
			{
				throw new BuildException(String.Format("Property '{0}' is undefined.", name));
			}
			return val;
		}


		public static string GetPropertyOrFail(this Project project, string name)
		{
			var val = project.Properties[name];

			if (val == null)
			{
				throw new BuildException(String.Format("Property '{0}' is undefined.", name));
			}
			return val;
		}

		public static bool GetBooleanProperty(this PropertyDictionary properties, string name)
		{
			bool ret = false;
			string val = properties[name];
			if (!String.IsNullOrEmpty(val))
			{
				if (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
				{
					ret = true;
				}
			}
			return ret;
		}

		public static bool GetBooleanPropertyOrDefault(this PropertyDictionary properties, string name, bool defaultVal, bool failOnNonBoolValue = false)
		{
			if (properties.TryGetBooleanProperty(name, out bool value, failOnNonBoolValue))
			{
				return value;
			}
			return defaultVal;
		}

		public static bool GetBooleanPropertyOrFail(this PropertyDictionary properties, string name)
		{
			if (properties.TryGetBooleanProperty(name, out bool value, failOnNonBoolValue: true))
			{
				return value; 
			}
			throw new BuildException($"Property '{name}' is undefined.");
		}

		public static bool TryGetBooleanProperty(this PropertyDictionary properties, string name, out bool value, bool failOnNonBoolValue = false)
		{
			string strValue = properties[name];
			if (strValue == null)
			{
				value = default;
				return false;
			}
			else if (strValue.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
			{
				value = true;
				return true;
			}
			else if (strValue.Trim().Equals("false", StringComparison.OrdinalIgnoreCase))
			{
				value = false;
				return true;
			}
			else if (failOnNonBoolValue)
			{
				throw new BuildException($"Property '{name}' has no boolean value '{strValue}'!");
			}
			else
			{
				value = default;
				return false;
			}
		}
	}
}


