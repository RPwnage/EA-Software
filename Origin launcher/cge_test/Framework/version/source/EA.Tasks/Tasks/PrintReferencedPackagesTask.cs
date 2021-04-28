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
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.PackageCore;

using EA.FrameworkTasks.Model;

namespace EA.EAMConfig
{
	// DAVE-FUTURE-REFACTOR-TODO: this can probably be removed along with calling targets in eaconfig
	[TaskName("PrintReferencedPackages")]
	public class PrintReferencedPackagesTask : Task
	{
		[TaskAttribute("configname", Required = false)]
		public string ConfigName { get; set; }

		protected override void ExecuteTask()
		{
			Log.Status.WriteLine();
			PrintBuildGraphPackages();
			PrintMasterPackages();
		}

		private void PrintBuildGraphPackages()
		{
			// Unsorted...
			//foreach (IPackage package in Project.BuildGraph().Packages.Values)
			//{
			//  Log.Status.WriteLine("{0,-20} {1} {2}", package.Name, package.Version, package.ConfigurationName);
			//}

			// Sorted...
			var sort = from p in Project.BuildGraph().Packages.Values orderby p.Name orderby p.ConfigurationName select p;

			int max_len_name = 0;
			//int max_len_version = 0;
			foreach (IPackage package in sort)
			{
				if (package.Name.Length > max_len_name)
					max_len_name = package.Name.Length;
				//if (package.Version.Length > max_len_version)
				//  max_len_version = package.Version.Length;
			}

			string format = "{0,-" + max_len_name + "} {1}";
			//string format = "{0,-" + max_len_name + "} {1,-" + max_len_version + "} {2}";
			string previous_config = null;
			foreach (IPackage package in sort)
			{
				if (previous_config != package.ConfigurationName)
				{
					Log.Status.WriteLine();
					Log.Status.WriteLine("Config: {0}", package.ConfigurationName);
					Log.Status.WriteLine();
					previous_config = package.ConfigurationName;
				}
				if (ConfigName == null || ConfigName == package.ConfigurationName)
					Log.Status.WriteLine(format, package.Name, package.Version);
			}

			Log.Status.WriteLine();
		}

		private void PrintMasterPackages()
		{
			Log.Status.WriteLine("Master packages:");
			Log.Status.WriteLine();

			var sort = from p in PackageMap.Instance.MasterPackages orderby p select p;

			int max_len_name = 0;
			//int max_len_version = 0;
			foreach (string packageName in sort)
			{
				//string packageVersion = PackageMap.Instance.GetMasterVersion(packageName, Project);
				if (packageName.Length > max_len_name)
					max_len_name = packageName.Length;
				//if (packageVersion.Length > max_len_version)
				//  max_len_version = packageVersion.Length;
			}

			//string format = "{0,-" + max_len_name + "} {1,-" + max_len_version + "} {2}";
			string format = "{0,-" + max_len_name + "} {1}";

			foreach (string packageName in sort)
			{
				if (PackageMap.Instance.TryGetMasterPackage(Project, packageName, out MasterConfig.IPackage packageSpec))
				{
					Release release = PackageMap.Instance.FindInstalledRelease(packageSpec);
					if (release != null)
					{
						//Log.Status.WriteLine(format, r.Package.Name, packageVersion, r.Path);
						Log.Status.WriteLine(format, release.Name, release.Path);
						continue;
					}
				}

				Log.Status.WriteLine(format, packageName, String.Empty);
			}

			Log.Status.WriteLine();
		}
	}
}
