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

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Build.MergeModules
{
	internal static class MergeModules_Invoker
	{
		internal static void MergeModules(Project project, IEnumerable<IModule> buildmodules)
		{
			bool enablemerge = project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-module-merge", false);

			if (enablemerge || project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-dotnet-module-merge", false))
			{
				MergeModules_DotNet.Merge(project, buildmodules);
			}
			if (enablemerge || project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-native-module-merge", false))
			{
				MergeModules_Native.Merge(project, buildmodules);
			}
		}


		internal static IEnumerable<MergeModules_Data> GetMergeData(IEnumerable<IModule> buildmodules, Func<string, IModule, string> makeName, Func<string, BuildGroups, IModule, string> makeKey)
		{
			var mergedata = new Dictionary<string, MergeModules_Data>();
			foreach (var mod in buildmodules)
			{
				var mergedef = mod.PropGroupValue("merge-into");

				if (!String.IsNullOrEmpty(mergedef))
				{
					string mergename;
					BuildGroups mergegroup;

					if (ParseMergeDef(mergedef, out mergename, out mergegroup))
					{
						mergename = makeName(mergename, mod);

						var key = makeKey(mergename, mergegroup, mod);

						MergeModules_Data mergeentry;

						if (!mergedata.TryGetValue(key, out mergeentry))
						{
							mergeentry = new MergeModules_Data(mod.Configuration, mergename, mergegroup);
							mergedata.Add(key, mergeentry);
						}

						mergeentry.SourceModules.Add(mod);
					}
				}

			}
			return mergedata.Values;
		}

		private static bool ParseMergeDef(string mergedef, out string mergename, out BuildGroups mergegroup)
		{
			var items = mergedef.ToArray("/\\\\");

			if (items.Count == 1)
			{
				mergename = items[0];
				mergegroup = BuildGroups.runtime;
			}
			else if (items.Count == 2)
			{
				mergegroup = ConvertUtil.ToEnum<BuildGroups>(items[0]);
				mergename = items[1];
			}
			else
			{
				mergegroup = BuildGroups.runtime;
				mergename = String.Empty;

				return false;
			}
			return true;
		}


	}
}
