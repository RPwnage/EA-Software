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
using NAnt.Core.Attributes;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends
{
	internal abstract class BackendGenerateBaseTask : Task
	{
		/// <summary>A whitespace delimited list of module names. If provided, overrides the default modules slngenerator will use a starting point when searching the graph for modules to include in the solution.</summary>
		[TaskAttribute("top-modules", Required = false)]
		public string TopModules { get; set; }

		/// <summary>A whitespace delimited list of module names. The solution generator will not include projects that are dependencies of these modules - can be used to exclude subgraphs when searching graph for modules to include in the solution.</summary>
		[TaskAttribute("excluded-root-modules", Required = false)]
		public string ExcludedRootModules { get; set; }

		protected IEnumerable<IModule> GetTopModules()
		{
			IEnumerable<IModule> topmodules;
			if (!String.IsNullOrEmpty(TopModules))
			{
				List<IModule> newTopModules = new List<IModule>();
				ILookup<string, string> moduleStrings = TopModules.ToArray().ToLookup(kv => kv);

				foreach (var kv in Project.BuildGraph().Modules)
				{
					if (!moduleStrings.Contains(kv.Value.Name))
					{
						continue;
					}
					newTopModules.Add(kv.Value);
				}

				if ((newTopModules.Select(x => x.Name).ToList().Distinct()).Count() != moduleStrings.Count())
				{
					throw new BuildException(string.Format("Not all top modules are found in the build graph!"));
				}

				topmodules = newTopModules;
			}
			else
			{
				topmodules = Generator.GetTopModules(Project);
			}

			return topmodules;
		}

		protected IEnumerable<IModule> GetExcludedRootModules()
		{
			IEnumerable<IModule> excludedRootModules = null;

			if (!String.IsNullOrEmpty(ExcludedRootModules))
			{
				List<IModule> excludedRootModuleList = new List<IModule>();
				ILookup<string, string> moduleStrings = ExcludedRootModules.ToArray().ToLookup(kv => kv);
				foreach (KeyValuePair<string, IModule> nameToModule in Project.BuildGraph().Modules.Where(kvp => moduleStrings.Contains(kvp.Value.Name)))
				{
					excludedRootModuleList.Add(nameToModule.Value);
				}
				excludedRootModules = excludedRootModuleList;
			}

			return excludedRootModules;
		}
	}
}