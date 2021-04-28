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
using System.Diagnostics;
using System.IO;
using System.Linq;

using NAnt.Core;

using Newtonsoft.Json;

namespace EA.Eaconfig
{
	// Example vswhere.exe output.
	//
	// Visual Studio Locator version 2.5.2+gebb9f26a3d [query version 1.18.21.37008]
	// Copyright (C) Microsoft Corporation. All rights reserved.
	//
	// instanceId: 799e3e55
	// installDate: 2017-10-19 2:08:13 PM
	// installationName: VisualStudio/15.9.7+28307.423
	// installationPath: C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional
	// installationVersion: 15.9.28307.423
	// productId: Microsoft.VisualStudio.Product.Professional
	// productPath: C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\IDE\devenv.exe
	// isPrerelease: 0
	// displayName: Visual Studio Professional 2017
	// description: Professional developer tools and services for small teams
	// channelId: VisualStudio.15.Release
	// channelPath: C:\Users\johnleung\AppData\Local\Microsoft\VisualStudio\Packages\_Channels\4CB340F5\catalog.json
	// channelUri: https://aka.ms/vs/15/release/channel
	// enginePath: C:\Program Files (x86)\Microsoft Visual Studio\Installer\resources\app\ServiceHub\Services\Microsoft.VisualStudio.Setup.Service
	// releaseNotes: https://go.microsoft.com/fwlink/?LinkId=660692#15.9.7
	// thirdPartyNotices: https://go.microsoft.com/fwlink/?LinkId=660708
	// updateDate: 2019-02-12T21:18:31.7701078Z
	// catalog_buildBranch: d15.9
	// catalog_buildVersion: 15.9.28307.423
	// catalog_id: VisualStudio/15.9.7+28307.423
	// catalog_localBuild: build-lab
	// catalog_manifestName: VisualStudio
	// catalog_manifestType: installer
	// catalog_productDisplayVersion: 15.9.7
	// catalog_productLine: Dev15
	// catalog_productLineVersion: 2017
	// catalog_productMilestone: RTW
	// catalog_productMilestoneIsPreRelease: False
	// catalog_productName: Visual Studio
	// catalog_productPatchVersion: 7
	// catalog_productPreReleaseMilestoneSuffix: 1.0
	// catalog_productRelease: RTW
	// catalog_productSemanticVersion: 15.9.7+28307.423
	// catalog_requiredEngineVersion: 1.18.1049.33485
	// properties_campaignId: 1228841668.1508446870
	// properties_channelManifestId: VisualStudio.15.Release/15.9.7+28307.423
	// properties_nickname:
	// properties_setupEngineFilePath: C:\Program Files (x86)\Microsoft Visual Studio\Installer\vs_installershell.exe

	internal class VisualStudioUtilities
	{
		private static readonly Lazy<Dictionary<string, object>[]> s_vsWhereInstallEntries = new Lazy<Dictionary<string, object>[]>
		(
			() => GetVSWhereInstallEntries().ToArray()
		);

		private static string PathToVsWhereExe
		{
			get
			{
				string programFilesPath = @"C:\Program Files (x86)";
				try
				{
					programFilesPath = Environment.GetEnvironmentVariable("ProgramFiles(x86)") ?? programFilesPath;
				}
				catch (Exception)
				{
					// swallow the exception and attempt to use default C:\ location
				}

				return Path.Combine(programFilesPath, @"Microsoft Visual Studio\Installer\vswhere.exe");
			}
		}

		/// <summary>
		/// Coerces the given Visual Studio version into the equivalent toolset version.
		/// </summary>
		internal static string GetToolSetVersion(string config_vs_version)
		{
			if (config_vs_version == "14.0")
				return "v140";
			else if (config_vs_version == "15.0")
				return "v141";
			else
				return "v142"; // default is v 16.0/VS2019 for now
		}

		/// <summary>
		/// Finds an installed version of Visual Studio that matches the given parameters (the first match returned by vswhere.exe)
		/// and returns the version and path as out parameters.
		/// </summary>
		/// <returns>True if and only if a valid Visual Studio installation is found</returns>
		internal static bool TryGetPathToVisualStudio(out string installedVersion, out string installedPath, string vsversion = "2019", bool checkForPreview = false)
		{
			installedVersion = string.Empty;
			installedPath = string.Empty;
			if (File.Exists(PathToVsWhereExe))
			{
				IEnumerable<Dictionary<string, object>> vsInstallInfo = GetInstalledVisualStudioInfo(vsversion, checkForPreview);
				if (!vsInstallInfo.Any())
				{
					return false;
				}

				Dictionary<string, object> firstMatch = vsInstallInfo.First();
				installedVersion = firstMatch["installationVersion"] as string;
				installedPath = firstMatch["installationPath"] as string + "\\"; // trailing slash needed because other scripts don't add it.

				return true;
			}
			return false;
		}

		/// <summary>
		/// Finds an installed version of Visual Studio matching the given vsversion and checkForPreview parameters, and
		/// determines which of the specified required components are not installed. Useful for printing warnings to the user,
		/// to alert them to exactly which components are missing.
		/// </summary>
		internal static IEnumerable<string> GetMissingVisualStudioComponents(IEnumerable<string> requiredComponents, string vsversion = "2019", bool checkForPreview = false, bool failOnMissing = true)
		{
			List<string> missingComponentsList = new List<string>();

			try
			{
				IEnumerable<Dictionary<string, object>> allVsInstallInfo = GetInstalledVisualStudioInfo(vsversion, checkForPreview);

				// Determine if the VS install we chose is missing any components that should be required.
				// Unfortunately vswhere.exe doesn't return which filters fail the '-requires ' command line flag,
				// so we need to do this ourselves by calling vswhere multiple times and seeing if any VS installs get filtered out
				if (requiredComponents != null)
				{
					foreach (string component in requiredComponents)
					{
						IEnumerable<Dictionary<string, object>> filteredVsInstallInfo = GetInstalledVisualStudioInfo(vsversion, checkForPreview, new string[] { component });
						if (filteredVsInstallInfo.Count() != allVsInstallInfo.Count())
						{
							missingComponentsList.Add(component);
						}
					}
				}
			}
			catch (BuildException ex)
			{
				if (failOnMissing)
				{
					throw ex;
				}
			}

			return missingComponentsList;
		}

		/// <summary>
		/// Returns a list of dictionaries that contain the info returned by vswhere.exe for the installed versions of Visual Studio that match the given parameters.
		/// </summary>
		private static IEnumerable<Dictionary<string, object>> GetInstalledVisualStudioInfo(string vsversion = "2019", bool checkForPreview = false, IEnumerable<string> requiredComponents = null)
		{
			IEnumerable<Dictionary<string, object>> installEntries;
			if (requiredComponents != null && requiredComponents.Any())
			{
				installEntries = GetVSWhereInstallEntries(requiredComponents);
			}
			else
			{
				installEntries = s_vsWhereInstallEntries.Value;
			}

			return installEntries.Where
			(
				install =>
				{
					string catalogString = install["catalog"].ToString();
					Dictionary<string, object> catalog = JsonConvert.DeserializeObject<Dictionary<string, object>>(catalogString);
					if ((catalog["productLineVersion"] as string) != vsversion)
					{
						return false;
					}

					if (!checkForPreview && Convert.ToBoolean(install["isPrerelease"]))
					{
						return false;
					}

					if (checkForPreview && (!install.ContainsKey("isPrerelease") || !Convert.ToBoolean(install["isPrerelease"])))
					{
						return false;
					}

					return true;
				}
			);
		}

		private static IEnumerable<Dictionary<string, object>> GetVSWhereInstallEntries(IEnumerable<string> requiredComponents = null)
		{
			string pathToVsWhere = PathToVsWhereExe;
			if (!File.Exists(pathToVsWhere))
			{
				throw new BuildException($"Unable to find vswhere.exe at '{pathToVsWhere}' to enumerate Visual Studio install info. Please check that Visual Studio 2017 or later is installed on the machine.");
			}

			ProcessStartInfo startInfo = new ProcessStartInfo(pathToVsWhere)
			{
				Arguments = "-nologo -prerelease -format json",
				UseShellExecute = false,
				RedirectStandardOutput = true,
				RedirectStandardError = true,
				CreateNoWindow = true
			};

			if (requiredComponents != null)
			{
				foreach (string component in requiredComponents)
				{
					startInfo.Arguments += " -requires " + component;
				}
			}

			Process detectProcess = Process.Start(startInfo);
			string fullOutput = detectProcess.StandardOutput.ReadToEnd();
			detectProcess.WaitForExit();

			return JsonConvert.DeserializeObject<List<Dictionary<string, object>>>(fullOutput);
		}
	}
}