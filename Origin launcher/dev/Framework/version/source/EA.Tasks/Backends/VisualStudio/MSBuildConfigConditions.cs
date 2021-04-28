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
using System.Linq;
using System.Text;
using EA.FrameworkTasks.Model;


namespace EA.Eaconfig.Backends.VisualStudio
{
	internal class MSBuildConfigConditions
	{
		internal class Condition
		{
			internal enum Type { None, Positive, Negative };

			internal Condition(Type type, IEnumerable<Configuration> configs = null)
			{
				ConditionType = type;
				Configurations = configs;

			}

			internal readonly Type ConditionType;
			internal readonly IEnumerable<Configuration> Configurations;

		}

		internal static Condition GetCondition(IEnumerable<Configuration> configs, IEnumerable<Configuration> allconfigs, bool invert = false)
		{
			if (invert)
			{
				configs = allconfigs.Except(configs);
			}

			int configCount = configs.Count();
			int allconfigCount = allconfigs.Count();

			if (configCount == allconfigCount)
			{
				return new Condition(Condition.Type.None);
			}

			if (configCount > allconfigCount / 2)
			{
				return new Condition(Condition.Type.Negative, allconfigs.Except(configs));
			}
		 
			return new Condition(Condition.Type.Positive, configs);
		}

		internal static string FormatCondition(string macro, Condition condition, Func<Configuration, string> configformatter)
		{
			if (condition.ConditionType == Condition.Type.None)
			{
				return string.Empty;
			}

			StringBuilder sb = new StringBuilder();
			if (condition.ConditionType == Condition.Type.Negative)
			{
				if (condition.Configurations.Count() == 1)
				{
					sb.AppendFormat(" '{0}' != '{1}' ", macro, configformatter(condition.Configurations.First()));
					return sb.ToString();
				}
				sb.Append("!(");
			}

			foreach (Configuration config in condition.Configurations.OrderBy(c => c.Name))
			{
				if (sb.Length > 3)
				{
					sb.Append("Or");
				}
				sb.AppendFormat(" '{0}' == '{1}' ", macro, configformatter(config));
			}

			if (condition.ConditionType == Condition.Type.Negative)
			{
				sb.Append(")");
			}

			return sb.ToString();


		}
	}
}
