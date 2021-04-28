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

using NAnt.Core;

using System;

namespace EA.Eaconfig
{
	public class OptionSetUtil
	{
		public static bool OptionSetExists(Project project, string name)
		{
			if (String.IsNullOrEmpty(name))
			{
				return false;
			}
			return project.NamedOptionSets.Contains(name);
		}

		public static OptionSet GetOptionSet(Project project, string name)
		{
			if (String.IsNullOrEmpty(name))
			{
				return null;
			}

			OptionSet optionSet = project.NamedOptionSets[name];
			return optionSet;
		}

		public static OptionSet GetOptionSetOrFail(Project project, string name)
		{
			if (String.IsNullOrEmpty(name))
			{
				throw new BuildException("[GetOptionSetOrFail] Optionset name is null or empty.");
			}

			OptionSet optionSet = project.NamedOptionSets[name];
			if (optionSet == null)
			{
				throw new BuildException($"[GetOptionSetOrFail] Optionset '{name}' does not exist.");
			}
			return optionSet;
		}

		public static void CreateOptionSetInProjectIfMissing(Project project, string name)
		{
			if (String.IsNullOrEmpty(name))
			{
				throw new BuildException("Can't create OptionSet set with empty name.");
			}
			if (project.NamedOptionSets[name] == null)
			{
				project.NamedOptionSets[name] = new OptionSet(); 
			}
		}

		public static bool IsOptionContainValue(OptionSet optionset, string optionName, string optionValue)
		{
			bool ret = false;
			if (optionset != null && optionset.Options.Contains(optionName))
			{
				if (-1 != optionset.Options[optionName].IndexOf(optionValue))
				{
					ret = true;
				}
			}
			return ret;
		}

		public static string GetOption(OptionSet set,  string optionName)
		{
			if (set == null || optionName == null)
			{
				return String.Empty;
			}

			return set.Options[optionName];
		}

		[Obsolete("GetOptionOrFail no longer needs to take a Project as a first argument.")]
		public static string GetOptionOrFail(Project project, OptionSet set, string optionName)
		{
			project.Log.ThrowDeprecation(NAnt.Core.Logging.Log.DeprecationId.OldOptionSetAPI, NAnt.Core.Logging.Log.DeprecateLevel.Normal, "GetOptionOrFail no longer needs to take a Project as a first argument.");
			return GetOptionOrFail(set, optionName);
		}

		public static string GetOptionOrFail(OptionSet set, string optionName)
		{
			string value = set.Options[optionName];
			if (String.IsNullOrEmpty(value))
			{
				throw new BuildException($"[GetOptionOrFail] Option '{optionName}' does not exist or has an empty value.");
			}
			return value;
		}

		public static string GetOptionOrDefault(OptionSet set, string optionName, string defaultVal)
		{            
			if (set == null || optionName == null)
			{
				return defaultVal;
			}

			string val =  set.Options[optionName];
			if (val == null)
			{
				val = defaultVal;
			}
			return val;
		}

		public static string GetOption(Project project, string optionsetName, string optionName)
		{
			return GetOptionSetOrFail(project, optionsetName).Options[optionName];
		}

		public static bool GetBooleanOption(OptionSet set, string optionName)
		{
			string option = set.Options[optionName];
			if (!String.IsNullOrEmpty(option) &&
				(option.Equals("true", StringComparison.OrdinalIgnoreCase) ||
				option.Equals("on", StringComparison.OrdinalIgnoreCase)))
			{
				return true;
			}
			return false;
		}

		public static bool GetBooleanOption(Project project, string optionsetName, string optionName)
		{
			string option = GetOptionSetOrFail(project, optionsetName).Options[optionName];
			if (!String.IsNullOrEmpty(option) &&
				(option.Equals("true", StringComparison.OrdinalIgnoreCase) ||
				option.Equals("on", StringComparison.OrdinalIgnoreCase)))
			{
				return true;
			}
			return false;
		}

		public static bool AppendOption(OptionSet optionset, string optionName, string optionValue)
		{
			bool ret = false;
			if (optionset.Options.Contains(optionName))
			{
				optionset.Options[optionName] += "\n" + optionValue;
				ret = true;
			}
			else
			{
				optionset.Options[optionName] = optionValue;
			}
			return ret;
		}

		public static void ReplaceOption(OptionSet optionSet, string optionName, string oldValue, string newValue)
		{
			if(newValue == null) 
				newValue = string.Empty;

			if (oldValue == newValue)
				return;

			string option = optionSet.Options[optionName];
			if (!String.IsNullOrEmpty(option))
			{
				if (!String.IsNullOrEmpty(newValue))
				{
					if (-1 == option.IndexOf(newValue))
					{
						if (!String.IsNullOrEmpty(oldValue) && (-1 != option.IndexOf(oldValue)))
						{
							option = option.Replace(oldValue, newValue);
						}
						else if (!String.IsNullOrEmpty(newValue))
						{
							option += "\n" + newValue;
						}
					}
					else if (!String.IsNullOrEmpty(oldValue))
					{
						if (oldValue != newValue)
						{
							option = option.Replace(oldValue, String.Empty);
						}
					}
				}
				else
				{
					if (!String.IsNullOrEmpty(oldValue))
					{
						option = option.Replace(oldValue, String.Empty);
					}
				}

				optionSet.Options[optionName] = option;
			}
			else if (!String.IsNullOrEmpty(newValue))
			{
				optionSet.Options[optionName] = newValue;
			}
		}

		public static bool ReplaceOnlyOption(OptionSet optionSet, string optionName, string oldValue, string newValue)
		{
			bool res = false;

			if (newValue == null)
				newValue = string.Empty;

			if (oldValue == newValue)
				return true;

			if (String.IsNullOrEmpty(oldValue))
				return false;

			string option = optionSet.Options[optionName];
			if (!String.IsNullOrEmpty(option))
			{
				res = -1 != option.IndexOf(oldValue);
				if (res)
				{
					option = option.Replace(oldValue, newValue);
					optionSet.Options[optionName] = option;
				}
			}
			return res;
		}
	}

}


