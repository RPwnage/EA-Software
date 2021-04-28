using System;
using System.Linq;

using NAnt.Core.Logging;

using EA.PackageSystem.PackageCore.Services;
using NAnt.Core.PackageCore;


using NAnt.Authentication;

namespace EA.PackageSystem.ConsolePackageManager
{
	internal static class PackageServerHelper
	{
		// packageServerName out parameter is used to return name using casing from package server
		internal static string GetLatestVersion(Log log, string name, string credentialFile, out string packageServerName, bool useV1 = false, string token = "")
		{
			try
			{
				IWebServices server = WebServicesFactory.Generate(log, credentialFile, useV1, token);
				ReleaseBase[] releases = server.GetPackageReleasesV2(name).Result;
				if (releases == null)
				{
					throw new ApplicationException(String.Format("Package '{0}' not found on package server.", name));
				}
				if (!releases.Any())
				{
					throw new ApplicationException(String.Format("Package '{0}' has no releases.", name));
				}
				packageServerName = releases.First().Name;
				return releases.First().Version;
			}
			catch (Exception e)
			{
				throw new ApplicationException(String.Format("Could not determine latest version because:\n{0}", e.Message));
			}
		}
	}
}