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
using System.IO;
using System.Linq;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;

namespace EA.PackageSystem.PackageCore
{
	public class PackagePoster
	{
		private const string StatusOfficial = "official";
		private const string StatusAccepted = "accepted";
		private static readonly string PackageServerIncomingFolder = $@"{Path.DirectorySeparatorChar}{Path.DirectorySeparatorChar}eac-as5.eac.ad.ea.com{Path.DirectorySeparatorChar}incoming";

		private readonly string FileName;
		private readonly string PackageName;
		private readonly Services.IWebServices Services;
		private readonly string PackagePath;
		private readonly bool useV1PackageServerAPI;
		private readonly Log Log;

		public PackagePoster(Log log, string path, string packageName, string credentialsFile = null, bool useV1 = false, string token = "")
		{
			FileName = Path.GetFileName(path);
			Log = log;
			PackageName = packageName;
			useV1PackageServerAPI = useV1;
			PackagePath = path;
			Services = PackageCore.Services.WebServicesFactory.Generate(log, credentialsFile, useV1, token);
		}

		public bool Post(string account = null, bool markOldOfficialAsAccepted = false, string manifestContents = null)
		{
			string tempPath = "";

			// package server status for latest package is "official", given that a new release is about to be posted
			// first change all the current "official" packages to "accepted" status
			if (markOldOfficialAsAccepted)
			{
				var allReleases = Services.GetPackageReleasesV2(PackageName).Result;
				var officialReleases = allReleases.Where(r => r is PackageRelease release && release.Status == StatusOfficial);

				foreach (ReleaseBase release in officialReleases)
				{
					try
					{
						var edited = Services.EditPackageStatus(PackageName, release.Version, StatusAccepted);
						if (!edited)
							throw new Exception($"Could not change status of version {release.Version} to accepted for package {PackageName}");
					}
					catch (Exception e)
					{
						// Don't fail post because previous versions status could not be edited but let the user know
						Log.Warning.WriteLine(e.Message);
					}
				}
			}
			// upload to server
			if (useV1PackageServerAPI == true)
			{
				tempPath = Path.Combine(PackageServerIncomingFolder, FileName);

				// Copy the file to the incoming folder. If the file already exist, it will get overwritten.
				File.Copy(PackagePath, tempPath, true);
			}
			try
			{
				// call web service to post new release
				string errorMessage = Services.PostPackageV2OrGetError(useV1PackageServerAPI ? FileName : PackagePath, manifestContents, null, 0, null, null, account);
				if (!String.IsNullOrEmpty(errorMessage))
				{
					if (errorMessage.StartsWith("warning", StringComparison.InvariantCultureIgnoreCase))
					{
						Log.Warning.WriteLine(errorMessage);
						return true;
					}
					else
					{
						Log.Error.WriteLine(errorMessage);
						return false;
					}
				}
			}
			catch (Exception e)
			{
				Log.Error.WriteLine($"Failed to post {e}");
				return false;
			}
			finally
			{
				// always clean up the file in incoming if it exists, regardless of outcome
				if (File.Exists(tempPath))
				{
					File.Delete(tempPath);
				}
			}
			return true;
		}

		public bool CreateNewPackage(string manifestContents)
		{
			return Services.CreateNewPackage(PackageName, manifestContents);
		}

	}
}
