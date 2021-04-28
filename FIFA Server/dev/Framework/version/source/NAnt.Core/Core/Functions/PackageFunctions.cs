// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2002 Gerry Shaw
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
using System;
using System.IO;

using NAnt.Core.Attributes;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of NAnt Project routines.
	/// </summary>
	[FunctionClass("Package Functions")]
	public class PackageFunctions : FunctionClassBase
	{
		/// <summary>
		/// Returns package masterversion declared in masterconfig file. This function does not perform dependent task.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageName"></param>
		/// <returns>masterversion string.</returns>
		/// <remarks>Throws error if package is not not declared in masterconfig file.</remarks>
		[Function()]
		public static string PackageGetMasterversion(Project project, string packageName) // this lower case 'v' haunts me - Dave V
		{
			if (!PackageMap.Instance.TryGetMasterPackage(project, packageName, out MasterConfig.IPackage packageSpec))
			{
				throw new BuildException($"PackageGetMasterversion({packageName}): package '{packageName}' is not declared in masterconfig file.");
			}
			return packageSpec.Version;
		}

		/// <summary>
		/// Returns package master release directory.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageName"></param>
		/// <returns>package directory string.</returns>
		/// <remarks>Throws error if package is not not declared in masterconfig file or failed to install.</remarks>
		[Function()]
		public static string PackageGetMasterDir(Project project, string packageName)
		{
			return PackageMap.Instance.FindOrInstallCurrentRelease(project, packageName).Path;		
		}

		/// <summary>
		/// Returns package master release directory.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="packageName"></param>
		/// <returns>package directory string.</returns>
		/// <remarks>returns empty string if package is not not declared in masterconfig file or is not installed.</remarks>
		[Function()]
		public static string PackageGetMasterDirOrEmpty(Project project, string packageName)
		{
			if (PackageMap.Instance.MasterConfigHasPackage(packageName))
			{
				return PackageMap.Instance.FindOrInstallCurrentRelease(project, packageName).Path;
			}
			return String.Empty;
		}

		/// <summary>
		/// </summary>
		[Function()]
		public static string GetPackageRoot(Project project, string packageName = null) // TODO: doc, unit tests, error checking
		{
			packageName = packageName ?? project.Properties[PackageProperties.PackageNamePropertyName];
			if (PackageMap.Instance.TryGetMasterPackage(project, packageName, out MasterConfig.IPackage package))
			{
				Release info = PackageMap.Instance.FindOrInstallRelease(project, package);
				bool isFlat = info.IsFlattened;
				return isFlat ?
					PathNormalizer.Normalize(Path.Combine(info.Path, "..")) :
					PathNormalizer.Normalize(Path.Combine(info.Path, "..", ".."));
			}
			return String.Empty;
		}
	}
}
