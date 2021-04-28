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

using NAnt.Core;

namespace EA.Eaconfig
{
	public static class EAConfigExtensions
	{
		public static string UnescapeDefine(this string def)
		{
			// When defines are passed through command line quotes are  stripped for us by cmd line processor.
			// In response file we need them stripped.
			int ind = def.IndexOf('=');
			if (ind != -1 && ind < def.Length - 1)
			{
				string def_val = def.Substring(ind + 1);
				if (!String.IsNullOrEmpty(def_val) && def_val.Length > 1)
				{
					// Trim one quote:
					if ((def_val[0] == '"' || def_val[0] == '\'') && def_val[0] == def_val[def_val.Length - 1])
					{
						def_val = def_val.Substring(1, def_val.Length - 2);
					}
					// Unescape def_val
					if (def_val.Length > 3 && ((def_val[0] == '\\') && (def_val[0] == def_val[def_val.Length - 2])))
					{
						def_val = def_val.Substring(1, def_val.Length - 3) + def_val[def_val.Length - 1];
					}

					def_val = def.Substring(0, ind + 1) + def_val;
				}
				else
				{
					def_val = def;
				}

				if (!String.IsNullOrEmpty(def_val))
				{
					def = def_val;
				}
			}
			return def;

		}

		public static IEnumerable<string> UnescapeDefines(this IEnumerable<string> defines)
		{
			var cleandefines = new List<string>();
			foreach (string def in defines)
			{
				if (String.IsNullOrEmpty(def))
				{
					continue;
				}
				string def_val = def.TrimWhiteSpace();

				if (String.IsNullOrEmpty(def_val))
				{
					continue;
				}

				def_val = UnescapeDefine(def_val);

				cleandefines.Add(def_val);
			}
			return cleandefines;
		}

		public static IDictionary<string, string> BuildEnvironment(this Project project)
		{
			if (project != null)
			{
				var optionset = project.NamedOptionSets["build.env"];
				return optionset == null ? null : optionset.Options;
			}
			return null;
		}

		public static IDictionary<string, string> ExecEnvironment(this Project project)
		{
			if (project != null)
			{
				var optionset = project.NamedOptionSets["exec.env"];
				return optionset == null ? null : optionset.Options;
			}
			return null;
		}

		public static FileItemList ThreadSafeFileItems(this FileSet fs)
		{
			if (fs != null)
			{
				lock (fs)
				{
					return fs.FileItems;
				}
			}
			return new FileItemList();
		}
	}
}
