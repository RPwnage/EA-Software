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
// Kosta Arvanitis (karvanitis@ea.com) 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;

using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

namespace EA.FrameworkTasks.Functions
{
	/// <summary>
	/// A collection of package functions.
	/// </summary>
	[FunctionClass("Package Functions")]
	public class PackageFunctions : FunctionClassBase
	{
		/// <summary>
		/// Gets a delimited list of all the package roots.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="delimiter">The delimiter to use when separating the package roots.</param>
		/// <returns>The default package root or empty string if none exists.</returns>
		/// <include file='Examples/Package/PackageGetPackageRoots.example' path='example'/>        
		[Function()]
		public static string PackageGetPackageRoots(Project project, string delimiter) {
			StringBuilder sb = new StringBuilder();
			for(int i =0; i < PackageMap.Instance.PackageRoots.Count; i++) {
				var root = PackageMap.Instance.PackageRoots[i];
				sb.Append(root.Dir.FullName);
				if (i < PackageMap.Instance.PackageRoots.Count - 1) {
					sb.Append(delimiter.Trim());
				}
			}
			return sb.ToString();
		}

		/// <summary>
		/// Returns a version associated with a package within masterconfig.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageName"></param>
		/// <returns>The version of package being used in masterconfig.</returns>
		/// <remarks>If package cannot be found, an empty string is returned.</remarks>
		[Function()]
		public static string GetPackageVersion(Project project, string packageName)
		{
			if (PackageMap.Instance.TryGetMasterPackage(project, packageName, out MasterConfig.IPackage packageSpec))
			{
				return packageSpec.Version;
			}
			return String.Empty;
		}

		/// <summary>
		/// Returns true/false whether package is buildable as defined within its manifest.xml file.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageName"></param>
		/// <returns>true/false dependent on whether package is buildable based on manifest.xml file.</returns>
		/// <remarks>If package cannot be found, false is returned.</remarks>
		[Function()]
		public static string IsPackageBuildable(Project project, string packageName)
		{
			Release info = PackageMap.Instance.FindOrInstallCurrentRelease(project, packageName);
			if (info != null)
			{
				return info.Manifest.Buildable.ToString().ToLowerInvariant();
			}
			return false.ToString().ToLowerInvariant();
		}

		/// <summary>
		/// Returns true/false whether package is autobuildclean within masterconfig.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageName"></param>
		/// <returns>true/false dependent on whether package is autobuildclean or not in masterconfig.</returns>
		/// <remarks>If package cannot be found, false is returned.</remarks>
		[Function()]
		public static string IsPackageAutoBuildClean(Project project, string packageName)
		{
			return PackageMap.Instance.IsPackageAutoBuildClean(project, packageName).ToString().ToLowerInvariant();
		}

		/// <summary>
		/// Returns true/false whether package is present in masterconfig.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageName"></param>
		/// <returns>true/false dependent on whether package is in masterconfig or not.</returns>
		/// <remarks>If package cannot be found, false is returned.</remarks>
		[Function()]
		public static string IsPackageInMasterconfig(Project project, string packageName)
		{
			return PackageMap.Instance.MasterConfigHasPackage(packageName).ToString().ToLowerInvariant();
		}

		/// <summary>
		/// Returns true if all packages are present in the masterconfig.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageNames">whitespace separated list of package names to check</param>
		/// <returns>true/false dependent on whether all packages are in masterconfig or not.</returns>
		/// <remarks>If any of packages cannot be found, false is returned.</remarks>
		[Function()]
		public static string AreAllPackagesInMasterconfig(Project project, string packageNames)
		{
			return packageNames.ToArray()
				.All(name => PackageMap.Instance.MasterConfigHasPackage(name))
				.ToString().ToLowerInvariant();
		}

		/// <summary>
		/// Returns masterconfig grouptype name.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageName">name of the package</param>
		/// <returns>grouptype name or empty string if package is not under a grouptype element in masterconfig.</returns>
		/// <remarks>If package cannot be found, empty string is returned.</remarks>
		[Function()]
		public static string GetPackageMasterconfigGrouptypeName(Project project, string packageName)
		{
			return PackageMap.Instance.GetMasterGroup(project, packageName)?.Name ?? String.Empty;
		}

		/// <summary>
		/// Indicates whether a config extension listed in the masterconfig is enabled.
		/// A config extension is a package that is listed in an extension element as a child of the config element in the masterconfig.
		/// A config extension is enabled as long as it does not have a condition on the extension element that currently evaluates to false.
		/// All enabled config extensions are applied all of the time, so even if you are not using an android config the android_config package will still be enabled.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="configPackageName">name of the config package to check</param>
		/// <returns>true if the config package is enabled, false otherwise.</returns>
		[Function()]
		public static string ConfigExtensionEnabled(Project project, string configPackageName)
		{
			return PackageMap.Instance.ConfigPackageName == configPackageName 
				|| PackageMap.Instance.GetConfigExtensions(project).Contains(configPackageName) ? "true" : "false";
		}

		// this function has been moved to NAnt.Core.Functions, this is just provided as a forward
		public static string GetPackageRoot(Project project, string name)
		{
			return NAnt.Core.Functions.PackageFunctions.GetPackageRoot(project, name);
		}
	}
}
