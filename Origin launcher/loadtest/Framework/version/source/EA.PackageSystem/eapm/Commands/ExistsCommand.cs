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
using NAnt.Core.Logging;
using EA.PackageSystem.PackageCore.Services;
using System.IO;
using NAnt.Authentication;

namespace EA.PackageSystem.ConsolePackageManager
{
	// TODO: command clean up
	// dvaliant 10/08/2016
	/* should unify package server code in PackageServerHelper.cs */

	[Command("exists")]
	internal class ExistsCommand : Command
	{
		internal override string Summary
		{
			get { return "Returns 1 if package(s) exists on the web package server."; }
		}

		internal override string Usage
		{
			get { return "[-credentialfile:<credential_file_path>] <name> <version> [-usev1]"; }
		}

		internal override string Remarks
		{
			get
			{
				return
@"Description
	If package exists returns 1, otherwise 0.

Examples
	Tell if package exists on the package server:
	> eapm exists eaconfig

	Tell if a specific package version exists on the package server
	> eapm exists eaconfig dev

	By default this command uses authentication token that is generated using (fb eapm credstore -use-package-server) token has a ttl of a month, 
	is stored locally in default credstore location	and is refreshed upon use.
	The auth token can be specified using -packageservertoken:yourauthtoken or read from a plaintext file specified at -pathtopackageservertoken:<pathtofilecontainingtoken>
	> eapm exists eaconfig -packageservertoken:yourauthtoken
	> eapm exists eaconfig -pathtopackageservertoken:<pathtofilecontainingtoken>

	Use Version 1 API -> soon to be removed - uses old ntlm authentication method with old SOAP protocol
	> eapm exists eaconfig -usev1";
			}
		}

		internal override void Execute()
		{
			IEnumerable<string> positionalArgs = Arguments.Where(arg => !arg.StartsWith("-"));
			IEnumerable<string> optionalArgs = Arguments.Where(arg => arg.StartsWith("-"));
			string packageServerToken = "";
			string pathToPackageServerToken = "";

			// process positional args
			if (!positionalArgs.Any() || positionalArgs.Count() > 2)
			{
				// requires arguments, but no more than 2
				throw new InvalidCommandArgumentsException(this);
			}
			string packageName = positionalArgs.ElementAt(0);
			string packageVersion = positionalArgs.ElementAtOrDefault(1);
			bool useV1 = PackageServerAuth.UseV1PackageServer();
			// process optional args
			string credentialFile = null;
			foreach (string arg in optionalArgs)
			{
				if (arg.StartsWith("-credentialfile:"))
				{
					credentialFile = arg.Substring("-credentialfile:".Length);
				}

				if (arg.ToLower().Equals("-usev1"))
				{
					useV1 = true;
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

			IWebServices services = WebServicesFactory.Generate(Log.Default, credentialFile, useV1 , packageServerToken);
			if (String.IsNullOrEmpty(packageVersion))
			{
				var releases = services.GetPackageReleasesV2(packageName).Result;
				if (releases == null)
				{
					throw new ApplicationException(String.Format("Package '{0}' not found on package server.", packageName));
				}

				if (releases.Length <= 0)
				{
					string msg = String.Format("Package '{0}' has no releases.", packageName);
					throw new ApplicationException(msg);
				}
				Console.WriteLine("package '{0}' found on the package server.", packageName);
			}
			else
			{
				var release = services.GetReleaseV2(packageName, packageVersion).Result;

				if (release == null)
				{
					string msg = String.Format("Package '{0}-{1}' not found on package server.", packageName, packageVersion);
					throw new ApplicationException(msg);
				}
				Console.WriteLine("package release'{0}-{1}' found on the package server.", packageName, packageVersion);
			}
		}
	}
}
