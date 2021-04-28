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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Text;

namespace NAnt.Core
{
	public struct PropertyKey : IEquatable<PropertyKey>
	{
		public enum CaseSensitivityLevel
		{
			Sensitive,
			Insensitive,
			InsensitiveWithTracking,
			SensitiveWithTracking
		};

		public readonly string Str;
		public readonly int Hash;

		public static CaseSensitivityLevel CaseSensitivity { get; set; }
		public static bool IsCaseSensitive { get { return CaseSensitivity == CaseSensitivityLevel.Sensitive || CaseSensitivity == CaseSensitivityLevel.SensitiveWithTracking; } }

		private static ConcurrentBag<StringBuilder> s_sbBag = new ConcurrentBag<StringBuilder>();
		private static Dictionary<string, Dictionary<string, int>> s_caseMismatchTracker = new Dictionary<string, Dictionary<string, int>>();

		private PropertyKey(string str)
		{
			Str = str;
			Hash = str.GetHashCode();
		}

		public override string ToString()
		{
			return AsString();
		}

		public override int GetHashCode()
		{
			return Hash;
		}

		public bool Equals(PropertyKey other)
		{
			return String.Equals(Str, other.Str, StringComparison.Ordinal);
		}

		public string AsString()
		{
			return Str;
		}

		public static PropertyKey Create(string a)
		{
			if (CaseSensitivity == CaseSensitivityLevel.InsensitiveWithTracking || CaseSensitivity == CaseSensitivityLevel.SensitiveWithTracking)
			{
				TrackKeySensitivity(a);
			}
			if (CaseSensitivity == CaseSensitivityLevel.Insensitive || CaseSensitivity == CaseSensitivityLevel.InsensitiveWithTracking)
			{
				a = a.ToLower();
			}
			return new PropertyKey(a);
		}

		public static PropertyKey Create(string a, string b)
		{
			return Create(a + b);
		}

		public static PropertyKey Create(string a, string b, string c)
		{
			return Create(a + b + c);
		}

		public static PropertyKey Create(string a, string b, string c, string d)
		{
			StringBuilder sb = PopBuilder();
			sb.Append(a).Append(b).Append(c).Append(d);
			return Create(PushBuilder(sb));
		}

		public static PropertyKey Create(string a, string b, string c, string d, string e)
		{
			StringBuilder sb = PopBuilder();
			sb.Append(a).Append(b).Append(c).Append(d).Append(e);
			return Create(PushBuilder(sb));
		}

		public static PropertyKey Create(string a, string b, string c, string d, string e, string f)
		{
			StringBuilder sb = PopBuilder();
			sb.Append(a).Append(b).Append(c).Append(d).Append(e).Append(f);
			return Create(PushBuilder(sb));
		}

		public static Dictionary<string, Dictionary<string, int>> GetMismatchTracker()
		{
			return s_caseMismatchTracker;
		}

		private static void TrackKeySensitivity(string key)
		{
			string lowerKey = key.ToLower();

			if (!s_caseMismatchTracker.TryGetValue(lowerKey, out Dictionary<string, int> variations))
			{
				variations = new Dictionary<string, int>();
				s_caseMismatchTracker.Add(lowerKey, variations);
			}

			if (!variations.ContainsKey(key))
			{
				variations.Add(key, 1);
			}
			else
			{
				variations[key]++;
			}
		}

		private static StringBuilder PopBuilder()
		{
			if (!s_sbBag.TryTake(out StringBuilder sb))
			{
				return new StringBuilder(128);
			}
			sb.Clear();
			return sb;
		}

		private static string PushBuilder(StringBuilder sb)
		{
			string str = sb.ToString();
			s_sbBag.Add(sb);
			return str;
		}
	}
}
