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

/* (c) Electronic Arts. All Rights Reserved. */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using NAnt.Authentication;
using NAnt.Core.Logging;

namespace EA.PackageSystem.ConsolePackageManager
{
	[Command("latest")]
	internal class LatestCommand : Command
	{
		internal override string Summary
		{
			get { return "query the latest version of a package on the package server"; }
		}

		internal override string Usage
		{
			get { return "[credential_file_path] <packagename> [-packageservertoken:<token>] [-pathtopackageservertoken:<filecontainingtoken>] -usev1"; }
		}

		internal override string Remarks
		{
			get
			{
				return @"Description
	Query the latest version of the package on the package server without installing it.

Examples
	Query the latest version of eaconfig:
	> eapm latest eaconfig
	uses authentication token that is generated using (fb eapm credstore -use-package-server) token has a ttl of a month, 
	is stored locally in default credstore location	and is refreshed upon use.
	The auth token can be specified using -packageservertoken:yourauthtoken or read from a plaintext file specified at -pathtopackageservertoken:<pathtofilecontainingtoken>

	> eapm latest eaconfig -packageservertoken:yourauthtoken
	> eapm latest eaconfig -pathtopackageservertoken:<pathtofilecontainingtoken>



	Use V1 of the package server api (soon to be removed) - uses old SOAP endpoint and ntlm authentication
	> eapm latest eaconfig -usev1";
			}
		}

		internal override void Execute()
		{
			IEnumerable<string> positionalArgs = Arguments.Where(arg => !arg.StartsWith("-"));
			IEnumerable<string> optionalArgs = Arguments.Where(arg => arg.StartsWith("-"));

			// process positional args
			if (positionalArgs.Count() != 1)
			{
				// requires exactly one arg
				throw new InvalidCommandArgumentsException(this);
			}
			string package = positionalArgs.First();

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
				else if (arg.StartsWith("-pathtopackageservertoken:"))
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
			Console.WriteLine("Finding latest version ...");
			string packageServerName = null;
			string version = PackageServerHelper.GetLatestVersion(Log.Default, package, credentialFile, out packageServerName, useV1, packageServerToken);

			Console.WriteLine("{0}-{1}", packageServerName, version);
		}
	}
}
