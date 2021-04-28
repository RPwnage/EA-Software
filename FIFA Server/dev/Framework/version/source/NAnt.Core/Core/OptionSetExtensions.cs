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

namespace NAnt.Core
{
	public static class OptionSetExtensions
	{
		public static bool GetBooleanOption(this OptionSet optionset, string optionname)
		{
			return optionset.GetBooleanOptionOrDefault(optionname, false);
		}

		public static bool GetBooleanOptionOrDefault(this OptionSet optionset, string optionname, bool defaultval)
		{
			bool result = defaultval;
			if (optionset != null)
			{
				string option = optionset.Options[optionname];
				if (!String.IsNullOrEmpty(option))
				{
					result = option.Equals("on", StringComparison.OrdinalIgnoreCase) || option.Equals("true", StringComparison.OrdinalIgnoreCase);
				}

			}
			return result;
		}


		public static string GetOptionOrDefault(this OptionSet set, string optionName, string defaultVal)
		{
			if (set == null || optionName == null)
			{
				return defaultVal;
			}

			string val = set.Options[optionName];
			if (val == null)
			{
				val = defaultVal;
			}
			return val;
		}

		public static Dictionary<string, string> GetOptionDictionary(this OptionSet options, string optionName)
		{
			Dictionary<string, string> result = null;
			if (options != null)
			{
				var properties = options.Options[optionName].LinesToArray();
				if (properties.Count > 0)
				{
					result = new Dictionary<string, string>();
					foreach (var line in options.Options[optionName].LinesToArray())
					{
						var prop = String.Empty;
						var val = String.Empty;

						int ind = line.IndexOf('=');
						if (ind < 0 || ind + 1 >= line.Length)
						{
							prop = line;
						}
						else
						{
							prop = line.Substring(0, ind).TrimWhiteSpace();
							val = line.Substring(ind + 1).TrimWhiteSpace();
						}
						if (!String.IsNullOrEmpty(prop))
						{
							result[prop] = val;
						}
					}
				}
			}
			return result;
		}
	}
}
