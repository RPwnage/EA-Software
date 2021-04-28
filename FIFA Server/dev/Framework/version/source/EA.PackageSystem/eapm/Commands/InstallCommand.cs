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
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using NAnt.Authentication;
using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Shared.Properties;

using Release = NAnt.Core.PackageCore.Release;

namespace EA.PackageSystem.ConsolePackageManager
{
	[Command("install")]
	internal class InstallCommand : Command
	{
		// an implementation of IPackggeSpec with no attributes or localondemand, just contains
		// the information needed to downloaded the package from web package server to regular
		// ondemand root

		internal override string Summary
		{
			get { return "Install a package."; }
		}

		internal override string Usage
		{
			get { return "<packagename> [version] [-ondemandroot:<path>] [-masterconfigfile:<full_file_path>] [-credentialfile:<credential_file_path>] [-D:property=value] [-G:property=value] [-packageservertoken:<token>] [-pathtopackageservertoken:<filecontainingtoken>] [-usev1]"; }
		}

		internal override string Remarks
		{
			get
			{
				return @"Description
Used to install a package to the ondemand folder. Note the default ondemand folder is the directory where 
Framework is installed. This can be overridden using environment variable FRAMEWORK_ONDEMAND_ROOT.

Options
	packagename                             Name of the package to install.
	version                                 Optional. Version of the package to install. If not specified 
											will install latest version from web package server (unless 
											-masterconfigfile is specified).
	-ondemandroot:<path>                    Optional. Specify a specific on demand root. If not specified 
											will install to default on demand root (unless -masterconfigfile 
											is specified).
	-packageservertoken:yourauthtoken		Optional. Auth token to use when interacting with Package server - for  
											use in automation tasks
	-pathtopackageservertoken:<file_path>	Optional. Location of file containing auth token in plain text
	-masterconfigfile:<full_file_path>      Optional. If specified this will be used to determine version to 
											install (including masterconfig fragments and exceptions). 
											Packages that specify uri protocols in the masterconfig will be 
											installed via those protocols. If 'version' parameter is 
											specified it will override the version in the masterconfig and 
											the web package server will be used unless version matches the 
											version in the masterconfig and the version in masterconfig 
											specifies a uri protocol.
	-credentialfile:<credential_file_path>  Optional. Packages installed from p4 uri protocol or web package 
											server may require credentials. If a credential file is used to 
											authenticate against these services then this parameter can be 
											used to specify the location of the credential file (if it is 
											not in default location).
	-D:property=value						Optional properties that affect masterconfig exceptions
	-G:property=value						Optional properties that affect masterconfig exceptions

Examples
	Install the latest version of the EASTL package:
	> eapm install EASTL

	Install a specific version of the EASTL package:
	> eapm install EASTL 3.00.00

	Pass a token to the package server auth (otherwise will prompt user for email and password) and store returned token for future calls
	> eapm install EASTL 3.00.00 -packageservertoken:yourauthtoken
	> eapm install EASTL 3.00.00 -pathtopackageservertoken:<pathtofilecontainingtoken>

	Install a specific version of the EASTL package to a non-default location:
	> eapm install EASTL 3.00.00 -ondemandroot:C:\temporary-packages

	Install the version of the EASTL package specifed by a masterconfig:
	> eapm install EASTL -masterconfigfile:C:\my-masterconfig.xml

	Install the version of the DotNet package specifed by a masterconfig and use a property for an exception block to use the non-proxy version:
	> eapm install DotNet -masterconfigfile:C:\my-masterconfig.xml -D:package.DotNet.use-non-proxy=true

	Use V1 of the package server api (soon to be removed) - uses ntlm authentication and old SOAP protocol.
	> eapm install EASTL -usev1";
			}
		}

		internal override void Execute()
		{
			IEnumerable<string> positionalArgs = Arguments.Where(arg => !arg.StartsWith("-"));
			IEnumerable<string> optionalArgs = Arguments.Where(arg => arg.StartsWith("-"));

			// process positional args
			if (!positionalArgs.Any() || positionalArgs.Count() > 2)
			{
				// requires arguments, but no more than 2
				throw new InvalidCommandArgumentsException(this);
			}
			string providedPackageName = positionalArgs.ElementAtOrDefault(0);
			string versionOverride = positionalArgs.ElementAtOrDefault(1);

			// process optional args
			string onDemandRootOverride = null;
			string credentialFile = null;
			string masterConfigfile = null;
			Dictionary<string,string> properties = new Dictionary<string, string>();
			bool useV1 = PackageServerAuth.UseV1PackageServer();
			string packageServerToken = "";
			string pathToPackageServerToken = "";
			foreach (string arg in optionalArgs)
			{
				if (arg.StartsWith("-masterconfigfile:"))
				{
					masterConfigfile = arg.Substring("-masterconfigfile:".Length);
					if (!File.Exists(masterConfigfile))
					{
						throw new InvalidCommandArgumentsException(this, "Masterconfig file specified does not exist.");
					}
				}
				else if (arg.StartsWith("-ondemandroot:"))
				{
					onDemandRootOverride = arg.Substring("-ondemandroot:".Length);
				}
				else if (arg.StartsWith("-credentialfile:"))
				{
					credentialFile = arg.Substring("-credentialfile:".Length);
				}
				else if (arg.StartsWith("-D:") || arg.StartsWith("-G:"))
				{
					string[] propArray = arg.Substring(3).Split(new char[] { '=' });
					if (propArray.Length != 2)
					{
						throw new InvalidCommandArgumentsException(this, "Property(s) not specified in the correct format of -D:property=value or -G:property=value.");
					}

					properties[propArray[0]] = propArray[1];
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

			// init package map
			Project project = new Project(Log.Silent, properties);
			MasterConfig masterConfig = null;
			if (masterConfigfile != null)
			{
				masterConfig = MasterConfig.Load(project, masterConfigfile);
			}
			PackageMap.Init(project, masterConfig, useV1: useV1, token: packageServerToken);

			// resolve version to install
			MasterConfig.IPackage installSpec = DeterminePackageNameAndVersion(project, providedPackageName, versionOverride, masterConfig, credentialFile, useV1, packageServerToken);
			if (onDemandRootOverride != null)
			{
				// override ondemand root to package map so we can see if package is already installed, this also sets the install location
				// that package servers will use to install in InstallRelease
				PackageMap.Instance.OverrideOnDemandRoot(onDemandRootOverride);
			}
			else if (!PackageMap.Instance.PackageRoots.HasOnDemandRoot)
			{
				throw new ApplicationException("Cannot install package as no ondemand root was detected in masterconfig or environment variables and no -ondemandroot parameter was specified.");
			}

			// install
			if (credentialFile != null)
			{
				project.Properties[FrameworkProperties.CredStoreFilePropertyName] = credentialFile;
			}

			try
			{
				Release release = PackageMap.Instance.FindOrInstallRelease(project, installSpec, allowOnDemand: true);
				Console.WriteLine(ConsolePackageManager.LogPrefix + "Package '{0}-{1}' installed to {2}.", release.Name, release.Version, release.Path);
			}
			catch (BuildException ex)
			{
				// Most known exceptions (including WebExceptions) are re-thrown as BuildException in Framework.  Should re-wrap the standard Framework build exception
				// as ApplicationException (which is what eapm expects) so that eapm won't report it as "INTERNAL ERROR". Shouldn't report as INTERNAL ERROR when it is not!
				throw new ApplicationException(ex.Message);
			}
			catch
			{
				// re-throw any other exceptions that Framework doesn't know about and not caught.
				throw;
			}
		}

		private MasterConfig.IPackage DeterminePackageNameAndVersion(Project project, string packageName, string versionName, MasterConfig masterConfig, string credentialFile, bool useV1 = true, string token = null)
		{
			// if masterconfig if available and contains entry for the package, we resolve it 
			// (even if we are installing explicit version, we still want to use the name casing from masterconfig)
			if (masterConfig != null)
			{
				if (PackageMap.Instance.TryGetMasterPackage(project, packageName, out MasterConfig.IPackage packageSpec))
				{
					if (versionName == null || versionName == packageSpec.Version) // if user didn't specify a version OR if the version is the masterconfig versions use masterconfig metadata (uri, localondemand, etc)
					{
						return packageSpec;
					}

					// if user has specified a version, try to look in all exceptions (including the unevalueted ones) for a potential match
					if (versionName != null && packageSpec is MasterConfig.Package)
					{
						foreach (MasterConfig.Exception<MasterConfig.IPackage> exception in ((MasterConfig.Package)packageSpec).Exceptions)
						{
							foreach (MasterConfig.PackageCondition condition in exception)
							{
								if (versionName == condition.Version)
								{
									return condition.Value;
								}
							}
						}
					}

					return new PackageMap.PackageSpec(packageSpec.Name, versionName);
				}
				else
				{
					// Throw a warning that this package is not listed in masterconfig but we used -masterconfigfile commandline switch.  But in the warning
					// message, also check if user mis-spell package names and provide suggestion.
					System.Text.StringBuilder msg = new System.Text.StringBuilder();
					msg.AppendLine(String.Format("WARNING: Unable to locate package name '{0}' from provided masterconfig file.", packageName));
					IEnumerable<string> closestMatches = NAnt.Core.Util.StringUtil.FindClosestMatchesFromList(masterConfig.Packages.Keys, packageName);
					if (closestMatches.Any())
					{
						msg.AppendLine(String.Format("Found the following package name(s) with closest match in provided masterconfig: {0}", String.Join("", closestMatches.Select(m => Environment.NewLine + "\t" + m))));
					}
					msg.AppendLine("Will be assuming package download from the Web Package Server.");

					// Should have used project.Log.Warning.WriteLine().  But "project" was initialized with Log.Slience.  Don't know the reason why.  For now, just use Console.WriteLine().
					Console.WriteLine(msg.ToString());
				}
			}

			// if we don't have a masterconfig version or explictly provided version assume web server install of latest version and also use it to correct package name casing
			if (versionName == null)
			{
				try
				{
					string latestVersion = PackageServerHelper.GetLatestVersion(project.Log, packageName, credentialFile, out packageName, useV1, token);
					project.Log.Status.WriteLine(ConsolePackageManager.LogPrefix + "Finding latest version ...");
					return new PackageMap.PackageSpec(packageName, latestVersion);
				}
				catch (Exception e)
				{
					throw new ApplicationException(e.Message);
				}
			}
			else
			{
				// if version override was specified and no masterconfig was specified return passed in values, note package name has not bee case corrected in this instance
				return new PackageMap.PackageSpec(packageName, versionName);
			}
		}
	}
}
