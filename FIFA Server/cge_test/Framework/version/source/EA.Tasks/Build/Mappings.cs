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

using System.Collections.Generic;

using NAnt.Core;

using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;
using System;

namespace EA.Eaconfig.Build
{
	public static class Mappings
	{
		public static readonly KeyValuePair<string, uint>[] DependencyPropertyMapping = new KeyValuePair<string, uint> []
		{ 
			// Use
			new KeyValuePair<string, uint>(".interfacedependencies", DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".usedependencies", DependencyTypes.Interface | DependencyTypes.Link),                
			new KeyValuePair<string, uint>(".uselinkdependencies", DependencyTypes.Link),
			// Build
			new KeyValuePair<string, uint>(".builddependencies", DependencyTypes.Build| DependencyTypes.Interface | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".linkdependencies", DependencyTypes.Build | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".buildinterfacedependencies", DependencyTypes.Build | DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".buildonlydependencies", DependencyTypes.Build),
			// Auto Build/Use
			new KeyValuePair<string, uint>(".autodependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Interface | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".autointerfacedependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".autolinkdependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Link)
		};

		public static readonly KeyValuePair<string, uint>[] ModuleDependencyPropertyMapping = new KeyValuePair<string, uint>[]
		{ 
			// Use
			new KeyValuePair<string, uint>(".interfacemoduledependencies", DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".usemoduledependencies", DependencyTypes.Interface | DependencyTypes.Link),                
			new KeyValuePair<string, uint>(".uselinkmoduledependencies", DependencyTypes.Link),
			// Build
			new KeyValuePair<string, uint>(".moduledependencies", DependencyTypes.Build| DependencyTypes.Interface | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".linkmoduledependencies", DependencyTypes.Build | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".buildinterfacemoduledependencies", DependencyTypes.Build | DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".buildonlymoduledependencies", DependencyTypes.Build),
			// Auto Build/Use
			new KeyValuePair<string, uint>(".automoduledependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Interface | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".autointerfacemoduledependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".autolinkmoduledependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Link)
		};

		public static readonly KeyValuePair<string, uint>[] ModuleDependencyConstraintsPropertyMapping = new KeyValuePair<string, uint>[]
		{ 
			// Use
			new KeyValuePair<string, uint>(".interfacebuildmodules", DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".usebuildmodules", DependencyTypes.Interface | DependencyTypes.Link),                
			new KeyValuePair<string, uint>(".uselinkbuildmodules", DependencyTypes.Link),
			// Build
			new KeyValuePair<string, uint>(".buildmodules", DependencyTypes.Build| DependencyTypes.Interface | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".linkbuildmodules", DependencyTypes.Build | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".buildinterfacebuildmodules", DependencyTypes.Build | DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".buildonlybuildmodules", DependencyTypes.Build),
			// Auto Build/Use
			new KeyValuePair<string, uint>(".autobuildmodules", DependencyTypes.AutoBuildUse | DependencyTypes.Interface | DependencyTypes.Link),
			new KeyValuePair<string, uint>(".autointerfacebuildmodules", DependencyTypes.AutoBuildUse | DependencyTypes.Interface),
			new KeyValuePair<string, uint>(".autolinkbuildmodules", DependencyTypes.AutoBuildUse | DependencyTypes.Link)
		};

		// this function is only used for debugging
		public static Dictionary<string, string> GetDependencyPropertiesDebugHelper(IModule module)
		{
			Dictionary<string, string> returnValues = new Dictionary<string, string>();
			List<string> dependentPackages = new List<string>();

			// package dependencies
			foreach (KeyValuePair<string, uint> nameToType in DependencyPropertyMapping)
			{
				string propertyName = $"{module.GroupName}{nameToType.Key}";
				string propertyValue = module.Package.Project.Properties[propertyName];
				if (propertyValue != null)
				{
					returnValues[propertyName] = propertyValue;

					dependentPackages.AddRange(propertyValue.ToArray());
				}
			}

			// package dependency module constraints
			foreach (KeyValuePair<string, uint> nameToType in ModuleDependencyConstraintsPropertyMapping)
			{
				foreach (string package in dependentPackages)
				{
					foreach (BuildGroups buildGroup in (BuildGroups[])Enum.GetValues(typeof(BuildGroups)))
					{
						string propertyName = $"{module.GroupName}.{package}.{buildGroup}{nameToType.Key}";
						string propertyValue = module.Package.Project.Properties[propertyName];
						if (propertyValue != null)
						{
							returnValues[propertyName] = propertyValue;
						}
					}
				}
			}

			// intra-package module dependencies
			foreach (KeyValuePair<string, uint> nameToType in ModuleDependencyPropertyMapping)
			{
				foreach (BuildGroups buildGroup in (BuildGroups[])Enum.GetValues(typeof(BuildGroups)))
				{
					// 99.999% of the time we use config agnostic property
					{
						string propertyName = String.Format($"{module.GroupName}.{buildGroup}{nameToType.Key}");
						string propertyValue = module.Package.Project.Properties[propertyName];
						if (propertyValue != null)
						{
							returnValues[propertyName] = propertyValue;
						}
					}

					// config specific just because it's technically still supported
					{
						string propertyName = String.Format($"{module.GroupName}.{buildGroup}{nameToType.Key}.{module.Configuration.System}");
						string propertyValue = module.Package.Project.Properties[propertyName];
						if (propertyValue != null)
						{
							returnValues[propertyName] = propertyValue;
						}
					}
				}
			}

			return returnValues;
		}
	}
}
