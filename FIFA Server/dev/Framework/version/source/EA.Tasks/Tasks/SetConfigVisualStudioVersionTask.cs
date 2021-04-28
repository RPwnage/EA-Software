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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;

namespace EA.Eaconfig
{
	[TaskName("get-visual-studio-install-info", XmlSchema = false)]
	public class Get_Visual_Studio_Info_Task : Task
	{
		[TaskAttribute("out-installed-version-property")]
		public string OutVersionProperty { get; set; }

		[TaskAttribute("out-appdir-property", Required = true)]
		public string OutAppDirProperty { get; set; }

		[TaskAttribute("out-tool-version-property")]
		public string OutToolsVersionProperty { get; set; }

		[TaskAttribute("out-redist-version-property")]
		public string OutRedistVersionProperty { get; set; }

		internal class VSInstallInfo
		{
			internal readonly string InstalledVersion;
			internal readonly string InstalledPath;
			internal readonly string MsvcToolsVersion;
			internal readonly string MsvcRedistVersion;

			internal VSInstallInfo(string installedVersion, string installPath, string msvcToolsVersion, string msvcRedistVersion)
			{
				InstalledVersion = installedVersion;
				InstalledPath = installPath;
				MsvcToolsVersion = msvcToolsVersion;
				MsvcRedistVersion = msvcRedistVersion;
			}
		}

		// concurrent dict - cache the result of executions of GetInstallInfo, this doesn't save us much because VisualStudioUtil
		// also caches
		private static ConcurrentDictionary<string, VSInstallInfo> s_installInfo = new ConcurrentDictionary<string, VSInstallInfo>();

		protected override void ExecuteTask()
		{
			bool allowPreview = Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.AllowPreview", false);
			string vsversion = Project.Properties.GetPropertyOrDefault("vsversion." + Project.Properties["config-system"], Project.Properties.GetPropertyOrDefault("vsversion", "2019"));

			VSInstallInfo installInfo = s_installInfo.GetOrAdd
			(
				$"{vsversion}-{(allowPreview ? "with-preview" : "without-preview")}",
				(_) => GetInstallInfo(vsversion, allowPreview)
			);

			Project.Properties[OutAppDirProperty] = installInfo.InstalledPath;
			if (!String.IsNullOrWhiteSpace(OutVersionProperty))
			{
				Project.Properties[OutVersionProperty] = installInfo.InstalledVersion;
			}
			if (!String.IsNullOrWhiteSpace(OutToolsVersionProperty))
			{
				Project.Properties[OutToolsVersionProperty] = installInfo.MsvcToolsVersion;
			}
			if (!String.IsNullOrWhiteSpace(OutRedistVersionProperty))
			{
				Project.Properties[OutRedistVersionProperty] = installInfo.MsvcRedistVersion;
			}
		}

		private VSInstallInfo GetInstallInfo(string vsVersion, bool allowPreview)
		{
			if (allowPreview && Convert.ToInt32(vsVersion) < 2019)
			{
				Log.ThrowWarning(
					NAnt.Core.Logging.Log.WarningId.VisualStudioPreviewNotSupported,
					NAnt.Core.Logging.Log.WarnLevel.Minimal,
					String.Format("Preview builds of Visual Studio are only supported for the most recent Visual Studio version - setting AllowPreview to false and continuing. (package.VisualStudio.AllowPreview was set to true, and vsversion specified is {0}", vsVersion));
				allowPreview = false;
			}

			bool result = VisualStudioUtilities.TryGetPathToVisualStudio
			(
				installedVersion: out string foundInstalledVersion,
				installedPath: out string foundInstalledPath,
				vsversion: vsVersion,
				checkForPreview: allowPreview
			);

			if (!result)
			{
				throw new FileNotFoundException(String.Format(
					"Failed to find a valid installed version of Visual Studio matching the following parameters: Version={0}, AllowPreview={1}",
					vsVersion, allowPreview));
			}

			// Make sure the user has the ATL component installed.
			IEnumerable<string> missingComponents = VisualStudioUtilities.GetMissingVisualStudioComponents(new string[] { "Microsoft.VisualStudio.Component.VC.ATL" }, vsVersion, allowPreview);

			if (missingComponents.Count() > 0)
			{
				if (missingComponents.Count() == 1 && missingComponents.First() == "Microsoft.VisualStudio.Component.VC.ATL")
				{
					Log.ThrowWarning(
						NAnt.Core.Logging.Log.WarningId.MissingVisualStudioComponents,
						NAnt.Core.Logging.Log.WarnLevel.Minimal,
						"Your Visual Studio installation doesn't appear to have the ATL component installed (Microsoft.VisualStudio.Component.VC.ATL). Some projects may not be able to build. If you are sure your projects don't need ATL, you can suppress this warning.");
				}
				else
				{
					throw new ApplicationException("Got missing component information for a component we didn't ask about.");
				}
			}

			string buildtoolRootPath = foundInstalledPath;
			if (Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.useCustomMSVC", false))
			{
				if (Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.use-non-proxy-build-tools", false))
				{
					buildtoolRootPath = Project.Properties.GetPropertyOrFail("package.MSBuildTools.appdir");
				}
			}

			return new VSInstallInfo
			(
				foundInstalledVersion,
				foundInstalledPath,
				GetMSVCComponentVersionFromDefaultFile("Tools", buildtoolRootPath),
				GetMSVCComponentVersionFromDefaultFile("Redist", buildtoolRootPath)
			);
		}

		private string GetMSVCComponentVersionFromDefaultFile(string componentName, string installedPath)
		{
			// Find the MSVC tools version
			string version = null;
			string msvcVerInfoFile = Path.Combine(installedPath, "VC", "Auxiliary", "Build", "Microsoft.VC" + componentName + "Version.default.txt");
			if (File.Exists(msvcVerInfoFile))
			{
				string[] filelines = File.ReadAllLines(msvcVerInfoFile);
				foreach (string line in filelines)
				{
					string ver = line.Trim();
					string msvcInstallDir = Path.Combine(installedPath, "VC", componentName, "MSVC", ver);
					if (Directory.Exists(msvcInstallDir))
					{
						version = ver;
					}
				}
				if (version == null)
				{
					// Found the file we expected but either couldn't extract the tools version info or physical directory doesn't exist.
					throw new BuildException(String.Format("Unable to extract MSVC {0} version info from file at: {1}.   Please make sure that 'Desktop development with C++' workload is selected in your Visual Studio installation!", componentName, msvcVerInfoFile));
				}
			}
			else
			{
				throw new BuildException(String.Format("Unable to locate MSVC {0} version file at: {1}.  Please make sure that 'Desktop development with C++' workload is selected in your Visual Studio installation!", componentName, msvcVerInfoFile));
			}

			return version;
		}
	}

	/// <summary>
	/// This task sets some properties that can be used if you need to execute your build
	/// differently depending on the version of Visual Studio that is being used.
	/// </summary>
	/// <remarks>
	/// <para>
	/// A property called config-vs-version is set to "14.0.0" if some variant of VisualStudio
	/// 2015 is being used, "15.0.0" if some variant of VisualStudio 2017 is being used, or 
	/// "16.0.0" if some variant of VisualStudio 2019 is being used.
	/// </para>
	/// <para>
	/// In addition to the config-vs-version property, some legacy properties called
	/// package.eaconfig.isusingvc[8, 9, 10, 11] are also set to true.  They are cumulative, 
	/// meaning that if package.eaconfig.isusingvc11 is set, it means that the ones for versions
	/// 8, 9, and 10 are also set to true.  We are 
	/// </para>
	/// </remarks>
	[TaskName("set-config-vs-version")]
	public class SetConfigVisualStudioVersion : Task
	{
		public SetConfigVisualStudioVersion()
			: base()
		{
		}

		private SetConfigVisualStudioVersion(Project project)
			: base()
		{
			Project = project;
		}

		public static ConcurrentDictionary<string, VersionInfo> VsVersionCache = new ConcurrentDictionary<string, VersionInfo>();

		public class VersionInfo
		{
			public string ShortVersion;
			public string FullVersion;
		}

		public static string Execute(Project project)
		{
			var task = new SetConfigVisualStudioVersion(project);
			task.Execute();
			return project.Properties["config-vs-version"];
		}

		private VersionInfo GetVisualStudioVersionInfo(string version)
		{
			bool result = VisualStudioUtilities.TryGetPathToVisualStudio
			(
				installedVersion: out string foundInstalledVersion,
				installedPath: out _,
				vsversion: version,
				checkForPreview: Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.AllowPreview", false)
			);
			VersionInfo versionInfo = new VersionInfo();

			if (!result)
			{
				// fall back to old mechanism of VS version detection since the above failed likely due to vswhere.exe not being present.
				string vsPackageVersion = PackageMap.Instance.GetMasterPackage(Project, "VisualStudio").Version;
				if (string.IsNullOrEmpty(vsPackageVersion))
				{
					throw new BuildException("VisualStudio package is not listed in masterconfig. Can not detect VisualStudio version.", Location);
				}

				// When dev version of Visual Studio package is used set vsVersion appropriately 
				if (vsPackageVersion.StartsWith("dev14", StringComparison.OrdinalIgnoreCase))
				{
					vsPackageVersion = "14.0.0";
				}

				if (0 <= vsPackageVersion.StrCompareVersions("14.0.0"))
				{
					versionInfo.FullVersion = "14.0.0.0";
					versionInfo.ShortVersion = "14.0";
				}
			}
			else
			{
				// set vs-version based on the version that was detected via vswhere.exe inside VisualStudioUtilities.GetPathToVisualStudio()
				versionInfo.FullVersion = foundInstalledVersion;
				versionInfo.ShortVersion = foundInstalledVersion.Split('.')[0] + ".0";
			}

			return versionInfo;
		}

		protected override void ExecuteTask()
		{
			bool useCustomMSVC = Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.useCustomMSVC", false);
			bool useNonProxyBuildTools = Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.use-non-proxy-build-tools", false);

			VersionInfo versionInfo = new VersionInfo();
			if (!useNonProxyBuildTools)
			{
				string configSystem = Project.Properties["config-system"];
				string vsversion = Project.Properties.GetPropertyOrDefault("vsversion." + configSystem, 
					Project.Properties.GetPropertyOrDefault("vsversion", "2019"));

				// vswhere is slow so cache the result to reduce the number of times we need to call it
				if (!VsVersionCache.TryGetValue(vsversion, out versionInfo))
				{
					versionInfo = GetVisualStudioVersionInfo(vsversion);
					VsVersionCache.TryAdd(vsversion, versionInfo);
				}

				if (useCustomMSVC)
				{
					Log.Info.WriteLine("package.VisualStudio.useCustomMSVC was set to true, but package.VisualStudio.use-non-proxy-build-tools was not. useCustomMSVC will be ignored, as it cannot be used without non-proxy build tools.");
				}
			}
			else
			{
				if (!PackageMap.Instance.MasterConfigHasPackage("MSBuildTools"))
				{
					throw new BuildException("MSBuildTools package is not listed in masterconfig, but package.VisualStudio.use-non-proxy-build-tools was set to true which requires the MSBuildTools package.", Location);
				}

				InternalQuickDependency.Dependent(Project, "MSBuildTools");

				versionInfo.FullVersion = Project.Properties.GetPropertyOrDefault("package.MSBuildTools.vs-version", "15.0.0.0");
				versionInfo.ShortVersion = versionInfo.FullVersion.Split('.')[0] + ".0";
			}

			Project.Properties["config-vs-version"] = versionInfo.ShortVersion;
			Project.Properties["config-vs-full-version"] = versionInfo.FullVersion;
			Project.Properties["config-vs-toolsetversion"] = VisualStudioUtilities.GetToolSetVersion(versionInfo.ShortVersion);
		}
	}
}
