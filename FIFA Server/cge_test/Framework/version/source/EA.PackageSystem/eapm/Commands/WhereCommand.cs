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
using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;

namespace EA.PackageSystem.ConsolePackageManager
{

	[Command("where")]
	internal class WhereCommand : Command
	{
        internal override string Summary
        {
            get { return "Return the location on disk of a package."; }
        }

        internal override string Usage
        {
            get { return "-masterconfigfile:<masterconfig path> <package name> [-D:property=value] [-G:property=value]"; }
        }

        internal override string Remarks
        {
            get
            {
                return @"where is a command that can be used to determine the location, on
disk, where a package resides. 
Options
	-masterconfigfile:<full_file_path>      This will be used to determine the version of the package to look for (including masterconfig fragments and exceptions). 
	packagename                             Name of the package to find.
	-D:property=value						Optional properties that affect masterconfig exceptions
	-G:property=value						Optional properties that affect masterconfig exceptions

Examples
	Find the version of the EASTL package that is installed in a packageroot:
	> eapm where EASTL -masterconfigfile:C:\my-masterconfig.xml

	Find the version of the DotNet package that is installed in a packageroot and use a property for an exception block to use the non-proxy version:
	> eapm where DotNet -masterconfigfile:C:\my-masterconfig.xml -D:package.DotNet.use-non-proxy=true";
			}
        }

		const string MasterconfigFileArgPrefix = "-masterconfigfile:";
		const string PropertyPrefix = "-D:";
		const string GlobalPropertyPrefix = "-G:";

		class Args
		{
			public readonly string m_masterconfigFile;
			public readonly string m_package;
			public readonly Dictionary<string, string> m_properties;

			Args(string masterconfigFile, string package, Dictionary<string, string> properties)
			{
				m_masterconfigFile = masterconfigFile;
				m_package = package;
				m_properties = properties;
			}

			public static Args ValidateArgs(IEnumerable<string> args, out string errorMessage)
			{
				string masterconfigFile = null;
				string package = null;
				Dictionary<string, string> properties = new Dictionary<string, string>();

				errorMessage = null;

				foreach (string arg in args)
				{
					if (arg.StartsWith(MasterconfigFileArgPrefix))
					{
						if (masterconfigFile != null)
						{
							errorMessage = "Only one masterconfig file can be specified.";
							return null;
						}

						masterconfigFile = arg.Substring(MasterconfigFileArgPrefix.Length);
						
						if (!File.Exists(masterconfigFile))
						{
							errorMessage = "Masterconfig file specified does not exist.";
							return null;
						}
						continue;
					}
					else if (arg.StartsWith(PropertyPrefix) || arg.StartsWith(GlobalPropertyPrefix))
					{
						string[] propArray = arg.Substring(3).Split(new char[] { '=' });
						if (propArray.Length != 2)
						{
							errorMessage = "Property(s) not specified in the correct format of -D:property=value or -G:property=value.";
							return null;
						}

						properties[propArray[0]] = propArray[1];
					}
					else
					{
						if (package != null)
						{
							errorMessage = string.Format("Multiple package names specified ('{0}' and '{1}'), only one package may be specified at a time.", package, arg);
							return null;
						}

						package = arg;
						continue;
					}
				}

				if (masterconfigFile == null || package == null)
				{
					errorMessage = "Invalid number of arguments specified!";
					return null;
				}

				return new Args(masterconfigFile, package, properties);
			}
		}

		internal override void Execute()
		{
			string validationError;
			Args validatedArgs = Args.ValidateArgs(Arguments, out validationError);

			if (validationError != null)
			{
				throw new ApplicationException(validationError);
			}

			Project project = new Project(Log.Silent, validatedArgs.m_properties);
			MasterConfig masterConfig = null;
			if (validatedArgs.m_masterconfigFile != null)
			{
				masterConfig = MasterConfig.Load(project, validatedArgs.m_masterconfigFile);
			}
			PackageMap.Init(project, masterConfig);
			if (PackageMap.Instance.TryGetMasterPackage(project, validatedArgs.m_package, out MasterConfig.IPackage packageSpec))
			{
				Release installedPackage = PackageMap.Instance.FindInstalledRelease(packageSpec);
				if (installedPackage != null)
				{
					Console.WriteLine(installedPackage.Path);
				}
				else
				{
					throw new ApplicationException($"ERROR: Unable to determine path for package '{validatedArgs.m_package}-{packageSpec.Version}'.Please ensure package is installed first.");
				}
			}
			else
			{
				throw new ApplicationException($"ERROR: Requested package '{validatedArgs.m_package}' was not listed in the masterconfig!");
			}

		}


	}
}

