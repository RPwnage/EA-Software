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
using System.IO;
using NAnt.Core.Logging;
using EA.PackageSystem.PackageCore;
using EA.SharpZipLib;

namespace EA.PackageSystem.ConsolePackageManager
{
	[Command("post")]
	internal class PostCommand : Command
	{
		internal override string Summary
		{
			get { return "post a package"; } 
		}

		internal override string Usage
		{
			get { return "<packagename>-<version>.zip [account] [credentialfile] [-mark-old-official-as-accepted] [-credentialfile:<credential_file_path>] [-packageservertoken:<token>] [-pathtopackageservertoken:<filecontainingtoken>] [-usev1] [--createnewpackage]"; } 
		}

		internal override string Remarks
		{
			get
			{
				return @"Description
	Post the given package zip file to the EA Package Server. The 
	Manifest.xml file will be used to specify the change details.

	NOTE: When posting the first release of a package, use the --createnewpackage flag

Examples
	Post the Config-0.8.0 package to the server:
	> eapm post build/Config-0.8.0.zip
	
	Create a new package with the release --createnewpackage
    > eapm post build/Config-0.8.0.zip --createnewpackage

	uses authentication token that is generated using (fb eapm credstore -use-package-server) token has a ttl of a month, 
	is stored locally in default credstore location	and is refreshed upon use.
	The auth token can be specified using -packageservertoken:yourauthtoken or read from a plaintext file specified at -pathtopackageservertoken:<pathtofilecontainingtoken>
	> eapm post build/Config-0.8.0.zip -packageservertoken:yourauthtoken
	> eapm post build/Config-0.8.0.zip -pathtopackageservertoken:<pathtofilecontainingtoken>	


	Use V1 of the package server api (soon to be removed) - uses old SOAP endpoint and ntlm authentication
	> eapm post build/Config-0.8.0.zip -usev1";
	

			} 
		}

		internal override void Execute()
		{
			IEnumerable<string> positionalArgs = Arguments.Where(arg => !arg.StartsWith("-"));
			IEnumerable<string> optionalArgs = Arguments.Where(arg => arg.StartsWith("-"));

			if (positionalArgs.Count() < 1 || positionalArgs.Count() > 3)
			{
				throw new InvalidCommandArgumentsException(this);
			}

			string path = positionalArgs.ElementAt(0);
			string account = positionalArgs.ElementAtOrDefault(1);
			string credentialfile = positionalArgs.ElementAtOrDefault(2);
			bool createNewPackage = false;
			bool useV1 = false;
			string packageServerToken = "";
			string pathToPackageServerToken = "";
			string manifestContents = "";
			bool markOldOfficialAsAccepted = false;
			var fileName = Path.GetFileNameWithoutExtension(path);
			var packageNameAndVersion = fileName.Split(new[] { '-' }, 2);
			string packageName = packageNameAndVersion.ElementAt(0);
			string packageVersion = packageNameAndVersion.ElementAt(1);
			foreach (string arg in optionalArgs)
			{
				if (arg.Equals("-mark-old-official-as-accepted"))
				{
					markOldOfficialAsAccepted = true;
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
				else if (arg.Equals("--createnewpackage"))
				{
					createNewPackage = true;
				}
				else
				{
					throw new InvalidCommandArgumentsException(this);
				}
			}
			if (useV1 == false)
			{
				
				// Get the contents of the manifest
				ZipLib zipLib = new ZipLib();
				// Create temp directory for unzipped manifest file, unzip and read contents.
				if (!File.Exists(path))
				{
					throw new ApplicationException($"File {path} does not exist");
				}
				string tempDirectory = Path.Combine(Path.GetTempPath(), "eapmTemp");
				string tempPackageDir = Path.Combine(tempDirectory, packageName, packageVersion);
				DirectoryInfo directory = Directory.CreateDirectory(tempPackageDir);
				directory.Attributes &= ~FileAttributes.ReadOnly;
				zipLib.UnzipSpecificFile(path, tempDirectory, $"{packageName}/{packageVersion}/Manifest.xml");
				string manifest = Path.Combine(tempPackageDir, "Manifest.xml");
				if (!File.Exists(manifest))
				{
					//zip lib will find it regardless of case but file exists requires specific casing
					manifest = Path.Combine(tempPackageDir, "manifest.xml");
					if (!File.Exists(manifest))
					{
						throw new ApplicationException($"Package could not be posted, manifest does not exist at {manifest}");
					}
				}

				manifestContents = File.ReadAllText(manifest).Trim();
					// delete the files
				
				try
				{
					File.Delete(manifest);
					Directory.Delete(tempDirectory, true);
				}
				catch
				{
					Console.WriteLine($"Failed to remove directory at {tempDirectory}");
				}

				// check for the token in the path
				// if the token is empty, it will use the credstore token, if that's missing / invalid the user will be prompted for email and password and have the token stored in their credstore location
				if (String.IsNullOrEmpty(packageServerToken))
				{
					if (!String.IsNullOrEmpty(pathToPackageServerToken))
					{
						FileInfo tok = new FileInfo(pathToPackageServerToken);
						if (tok.Exists)
						{
							packageServerToken = File.ReadAllText(tok.FullName);
						}
						else
						{
							throw new ApplicationException($"Provided file for token {tok.FullName} does not exist");
						}
					}
				}
			}
			try
			{
				PackagePoster poster = new PackagePoster(Log.Default, path, packageName, credentialfile, useV1, packageServerToken);
				if (useV1 == false && createNewPackage)
				{
					bool packageCreated = poster.CreateNewPackage(manifestContents);		
					if (!packageCreated)
					{
						throw new ApplicationException("See log for package post server response error messages.");
					}
				}
				bool postSuccessful = poster.Post(account, markOldOfficialAsAccepted, manifestContents);
				if (!postSuccessful)
				{
					// The actual server error is printed in log output.
					throw new ApplicationException("See log for package post server response error messages.");
				}
			} 
			catch (Exception e) 
			{
				throw new ApplicationException("Package could not be posted.", e);
			}
		}
	}
}
