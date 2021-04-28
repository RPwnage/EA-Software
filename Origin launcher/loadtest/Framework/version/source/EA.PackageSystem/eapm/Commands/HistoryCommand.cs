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
using EA.PackageSystem.PackageCore;
using NAnt.Authentication;
using NAnt.Core.Logging;
using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.ConsolePackageManager
{
	[Command("history")]
	internal class HistoryCommand : Command
	{
		internal override string Summary
		{
			get { return "Retrieve the versions, dates, and statuses of all the releases of a given package."; }
		}

		internal override string Usage
		{
			get { return "[-credentialfile:<credential_file_path>] [-packageservertoken:<token>] [-pathtopackageservertoken:<filecontainingtoken>] [-usev1] [package1] [package2] ..."; }
		}

		internal override string Remarks
		{
			get { 
				return @"eapm history [-credentialfile:<credential_file_path>] [-packageservertoken:<token>] [-pathtopackageservertoken:<filecontainingtoken>] [-usev1] [package1] [package2] ...
				
				By default this command uses authentication token that is generated using (fb eapm credstore -use-package-server) token has a ttl of a month, 
				is stored locally in default credstore location	and is refreshed upon use.
				The auth token can be specified using -packageservertoken:yourauthtoken or read from a plaintext file specified at -pathtopackageservertoken:<pathtofilecontainingtoken>
				
				> eapm history -packageservertoken:yourauthtoken eaconfig 
				> eapm history -pathtopackageservertoken:<pathtofilecontainingtoken> eaconfig
				
				Use Version 1 API -> soon to be removed - uses old ntlm authentication method with old SOAP protocol
				> eapm exists eaconfig -usev1		
				"; 
			}
		}

		internal override void Execute()
		{
			IEnumerable<string> packages = Arguments.Where(arg => !arg.StartsWith("-"));
			IEnumerable<string> optionalArgs = Arguments.Where(arg => arg.StartsWith("-"));
			// process positional args
			if (!packages.Any())
			{
				// expects at least one argument
				throw new InvalidCommandArgumentsException(this);
			}

			// process optional args
			string credentialFile = null;
			bool useV1 = PackageServerAuth.UseV1PackageServer();
			string packageServerToken = "";
			string pathToPackageServerToken = "";
			foreach (string arg in optionalArgs)
			{
				if (arg.StartsWith("-credentialfile:"))
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


			PackageServer.IWebServices server = PackageServer.WebServicesFactory.Generate(Log.Default, credentialFile, useV1, packageServerToken);
			foreach (string argument in Arguments)
			{
				var releases = server.GetPackageReleasesV2(argument).Result;
				if (releases != null)
				{
					foreach (var r in releases)
					{
						if (r is PackageRelease release)
						{
							Console.WriteLine("{0} {1} {2} {3}", argument, release.Version, release.CreatedAt, release.Status);
						}
					}
				}
				else
				{
					throw new ApplicationException("Error: Package history could not be found. The specified package may not be listed on the package server.");
				}
			}
		}
	}
}
