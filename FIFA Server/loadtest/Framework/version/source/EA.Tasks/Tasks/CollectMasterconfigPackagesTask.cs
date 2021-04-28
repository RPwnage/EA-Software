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
using NAnt.Core.PackageCore;

namespace EA.Eaconfig
{
	/// <summary>
	/// Gets a list of packages in a specific group in the masterconfig file and stores it in a property
	/// </summary>
	[TaskName("collect-masterconfig-packages")]
	sealed class CollectMasterconfigPackagesTask : Task
	{
		/// <summary>
		/// The name of a group in the masterconfig config file
		/// </summary>
		[TaskAttribute("groupname", Required = true)]
		public string GroupName { get; set; }

		/// <summary>
		/// The name of a property where the list will be stored
		/// </summary>
		[TaskAttribute("target-property", Required = true)]
		public string TargetProperty { get; set; }

		protected override void ExecuteTask()
		{
			var packages = FindGroupPackages(Project, GroupName);

			if (!packages.Any())
			{
				Log.Warning.WriteLine("Could not find any packages in group \"" + GroupName + "\" or the group does not exist.");
			}

			Properties[TargetProperty] = String.Join(Environment.NewLine, packages);
		}

		private IEnumerable<string> FindGroupPackages(Project project, string groupName)
		{
			return PackageMap.Instance.MasterConfig.Packages
				.Where(nameToPackage => nameToPackage.Value.EvaluateGroupType(project).Name == groupName)
				.Select(nameToPackage => nameToPackage.Key);
		}
	}
}
