// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2002 Gerry Shaw
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
// File Maintainers:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;

namespace NAnt.Core
{

	public class Property
	{
		public const string PrefixSeperator = ".";

		public string Name { get { return Key.AsString();  } }

		public string Value
		{
			get { return m_value; }
			set
			{
				if (ReadOnly)
				{
					string msg = String.Format("Property '{0}' is readonly and cannot be modified.", Name);
					throw new BuildException(msg);
				}
				m_value = value;
			}
		}

		public string Prefix
		{
			get { return GetPrefix(Name); }
		}

		public string Suffix
		{
			get { return GetSuffix(Name); }
		}

		public readonly PropertyKey Key;

		public bool ReadOnly;
		public bool Deferred;
		public bool Inheritable;

		private string m_value;

		public Property(PropertyKey key, string value, bool readOnly = false, bool deferred = false, bool inheritable = false)
		{           
			Key = key;
			m_value = value;
			ReadOnly = readOnly;
			Deferred = deferred;
			Inheritable = inheritable;
		}

		internal Property(Property other)
		{
			Key = other.Key;
			m_value = other.m_value;

			ReadOnly = other.ReadOnly;
			Deferred = other.Deferred;
			Inheritable = other.Inheritable;
		}

		public override string ToString()
		{
			return "{" + Name + ", " + m_value + "}";
		}

		// Validate property name.If the property name is invalid a BuildException will be thrown.
		// A valid property name contains any word character, '-' character, or '.' character.
		public static bool VerifyPropertyName(string name)
		{
			bool isValid = true;
			for (int i = 0, e = name.Length; i < e; i++)
			{
				var ch = name[i];
				isValid &= (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '.' || ch == '-' || ch == '+' || ch == '/';
			}
			return isValid;
		}

		public static string Combine(string prefix, string name)
		{
			return (prefix + PrefixSeperator + name);
		}

		// Get the prefix of the specified property name.
		// e.g: package.Framework.dir => package.Framework
		private static string GetPrefix(string name)
		{
			int index = name.LastIndexOf('.');
			if (index >= 0)
			{
				return name.Substring(0, index);
			}
			return name;
		}

		// Get the suffix of the specified property name.
		// e.g: package.Framework.dir => dir
		private static string GetSuffix(string name)
		{
			int index = name.LastIndexOf('.');
			if (index >= 0)
			{
				return name.Substring(index + 1, name.Length - index - 1);
			}
			return name;
		}
	}
}
