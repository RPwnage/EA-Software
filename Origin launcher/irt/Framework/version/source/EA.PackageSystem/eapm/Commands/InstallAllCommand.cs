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
using System.IO;
using System.Linq;
using System.Net;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Shared.Properties;

using Release = NAnt.Core.PackageCore.Release;
using NAnt.Authentication;

namespace EA.PackageSystem.ConsolePackageManager
{
	[Command("installall")]
	internal class InstallAllCommand : Command
	{
		internal override string Summary
		{
			get { return "Install all packages in a masterconfig file."; }
		}

		internal override string Usage
		{
			get { return "<masterconfigfile> [-ondemandroot:<path>] [-credentialfile:<credential_file_path>] [-packageservertoken:<token>] [-pathtopackageservertoken:<filecontainingtoken>] [-usev1]"; }
		}

		internal override string Remarks
		{
			get
			{
				return @"Description
(Experimental) Used to install all files in a given masterconfig. 
This is an experimental new command that may change or be removed without notice, feel free to try out the command but we don't recommend writting scripts that rely on it just yet.

Options
	masterconfigfile						The path to the masterconfig file.
	-ondemandroot:<path>                    Optional. Specify a specific on demand root. If not specified 
											will install to default on demand root (unless -masterconfigfile 
											is specified).
	-credentialfile:<credential_file_path>  Optional. Packages installed from p4 uri protocol or web package 
											server may require credentials. If a credential file is used to 
											authenticate against these services then this parameter can be 
											used to specify the location of the credential file (if it is 
											not in default location).
	-packageservertoken:yourauthtoken		Optional. Auth token to use when interacting with Package server - for  
											use in automation tasks
	-pathtopackageservertoken:<file_path>	Optional. Location of file containing auth token in plain text

Examples
	Install all packages in a masterconfig file:
	> eapm installall C:\my-masterconfig.xml

	Pass a token to the package server auth (otherwise will prompt user for email and password if not stored in credfile) and store returned token for future calls
	> eapm installall C:\my-masterconfig.xml -packageservertoken:yourauthtoken
	> eapm installall C:\my-masterconfig.xml -pathtopackageservertoken:<pathtofilecontainingtoken>

	Install all packages in a masterconfig file to a non-default location:
	> eapm installall C:\my-masterconfig.xml -ondemandroot:C:\temporary-packages

	Use V1 of the package server api (soon to be removed):
	> eapm installall C:\my-masterconfig.xml -usev1 ";

			}
		}

		internal override void Execute()
		{
			IEnumerable<string> positionalArgs = Arguments.Where(arg => !arg.StartsWith("-"));
			IEnumerable<string> optionalArgs = Arguments.Where(arg => arg.StartsWith("-"));

			// process positional args
			if (!positionalArgs.Any() || positionalArgs.Count() > 1)
			{
				// requires arguments, but no more than 1
				throw new InvalidCommandArgumentsException(this);
			}
			string masterConfigfile = positionalArgs.ElementAtOrDefault(0);

			// process optional args
			string onDemandRootOverride = null;
			string credentialFile = null;
			bool useV1 = PackageServerAuth.UseV1PackageServer();
			string packageServerToken = "";
			string pathToPackageServerToken = "";
			foreach (string arg in optionalArgs)
			{
				if (arg.StartsWith("-ondemandroot:"))
				{
					onDemandRootOverride = arg.Substring("-ondemandroot:".Length);
				}
				else if (arg.StartsWith("-credentialfile:"))
				{
					credentialFile = arg.Substring("-credentialfile:".Length);
				}
				else if (arg.ToLower().Equals("-usev1"))
				{
					useV1 = true;
				}
				else if (arg.StartsWith("-packageservertoken:"))
				{
					packageServerToken = arg.Substring("-packageservertoken:".Length);
				}
				else if (arg.StartsWith("pathtopackageservertoken:"))
				{
					pathToPackageServerToken = arg.Substring("-pathtopackageservertoken:".Length);
				}
				else
				{
					throw new InvalidCommandArgumentsException(this);
				}
			}

			if (useV1 == false)
			{
				// check for the token in the path
				if (String.IsNullOrEmpty(packageServerToken))
				{
					if (!String.IsNullOrEmpty(pathToPackageServerToken))
					{
						FileInfo file = new FileInfo(pathToPackageServerToken);
						if (file.Exists)
						{
							packageServerToken = File.ReadAllText(file.FullName);
						}
					}
				}
			}
			else
			{
				Console.WriteLine($"Using V1 of the API - soon to be deprecated as is not secure please start to use V2 - message the package-server slack channel for more information ");
			}

			// init package map
			Project project = new Project(Log.Silent);
			MasterConfig masterConfig = null;
			if (masterConfigfile != null)
			{
				masterConfig = MasterConfig.Load(project, masterConfigfile);
			}
			PackageMap.Init(project, masterConfig, false, null, null, useV1, packageServerToken);

			if (onDemandRootOverride != null)
			{
				// override ondemand root to package map so we can see if package is alreauy installed, this also sets the install location
				// that package servers will use to install in InstallRelease
				PackageMap.Instance.OverrideOnDemandRoot(onDemandRootOverride);
			}
			else if (!PackageMap.Instance.PackageRoots.HasOnDemandRoot)
			{
				throw new ApplicationException("Cannot install package as no ondemand root was detected in masterconfig or environment variables and no -ondemandroot parameter was specified.");
			}

			if (credentialFile != null)
			{
				project.Properties[FrameworkProperties.CredStoreFilePropertyName] = credentialFile;
			}

			InstallAllPackages(project);
		}

		private void InstallAllPackages(Project project)
		{
			List<string> packagesFailed = new List<string>();
			int packagesFound = 0;
			int packagesInstalled = 0;
			foreach (MasterConfig.Package package in PackageMap.Instance.MasterConfig.Packages.Values)
			{
				InstallPackageSpec(project, package, ref packagesFound, ref packagesInstalled, ref packagesFailed);
				foreach (MasterConfig.Exception<MasterConfig.IPackage> exception in package.Exceptions)
				{
					foreach (MasterConfig.Condition<MasterConfig.IPackage> condition in exception)
					{
						InstallPackageSpec(project, package, ref packagesFound, ref packagesInstalled, ref packagesFailed);
						// DAVE-FUTURE-REFACTOR-TODO: an unlikely but possible case we run into looping through exceptions like this
						// is that we have a package with the same version in multiple conditions but different uri/localondemand/etc
						// that causes it to be redownloaded in a different way after just being downloaded. What should we do in this case?
						// skip? throw warning? continue to ignore?
					}
				}
			}

			Console.WriteLine();
			Console.Write("Package Install Complete: {0} Found", packagesFound);
			if (packagesInstalled > 0)
			{
				Console.Write(", {0} Downloaded", packagesInstalled);
			}
			if (packagesFailed.Count > 0)
			{
				Console.Write(", {0} Failed", packagesFailed.Count);
			}
			Console.WriteLine();
			if (packagesFailed.Count > 0)
			{
				Console.WriteLine(ConsolePackageManager.LogPrefix + "Failed to install the following packages:");
				foreach (string p in packagesFailed)
				{
					Console.WriteLine(p);
				}
			}
		}

		private static void InstallPackageSpec(Project project, MasterConfig.IPackage package, ref int packagesFound, ref int packagesInstalled, ref List<string> packagesFailed)
		{
			try
			{
				Release masterPackageRelease = PackageMap.Instance.FindInstalledRelease(package);
				if (masterPackageRelease != null)
				{
					++packagesFound;
				}
				else
				{
					PackageMap.Instance.FindOrInstallRelease(project, package);
					Console.WriteLine($"{ConsolePackageManager.LogPrefix}Package '{package.Name}-{package.Version}' installed.");
					++packagesInstalled;
				}
			}
			catch (Exception e)
			{
				Console.WriteLine($"{ConsolePackageManager.LogPrefix}Error: Package '{package.Name}-{package.Version}' install FAILED:");
				Console.WriteLine(e.Message);
				packagesFailed.Add($"{package.Name}-{package.Version}");
			}
		}
	}
}
