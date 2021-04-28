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
using System.Linq;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

using Release = NAnt.Core.PackageCore.Release;

namespace EA.PackageSystem.ConsolePackageManager
{
    // TODO: command clean up
    // dvaliant 10/08/2016
    /* version could be optional, no way to specify ondemandroot (explicit or via masterconfig would work, see InstallCommand) */

    [Command("remove")]
	internal class RemoveCommand : Command
	{
        internal override string Summary
		{
			get { return "Remove a package."; }
		}

        internal override string Usage
		{
			get { return "<packagename> <version>"; }
		}

        internal override string Remarks
		{
			get
			{
				return
@"Description
	Deletes a specific version of a package.

	NOTE: This command removes all files in the package directory including
	any modified files.

	THERE IS NO WAY TO UNDO THIS COMMAND.

Examples
	Remove the config 0.8.0 package from your machine:
	> eapm remove config 0.8.0

	Remove all versions of the packages from your machine: (USE CAUTION)
	> eapm list config | xargs -n 1 eapm remove";
			}
		}

		internal override void Execute()
		{
            if (Arguments.Count() != 2)
			{
				throw new InvalidCommandArgumentsException(this);
			}

            string name = Arguments.First();
            string version = Arguments.ElementAtOrDefault(1);

			PackageMap.Init(new Project(Log.Silent));
			Release release = PackageMap.Instance.FindInstalledRelease(new PackageMap.PackageSpec(name, version));
			if (release == null)
			{
				Console.WriteLine("Package '{0}-{1}' does not exist in '{2}'.", name, version, PackageMap.Instance.PackageRoots.ToString());
			}
			else
			{
				try
				{
					Release frameworkRelease = PackageMap.Instance.GetFrameworkRelease();
					if (release.Name != frameworkRelease.Name || release.Version != frameworkRelease.Version) // Framework can't uninstall itself
					{
						Console.WriteLine("Deleting directory {0}", release.Path);
						PathUtil.DeleteDirectory(release.Path, verify: false);
					}
					else
					{
						Console.WriteLine("\neapm.exe is a part of " + frameworkRelease.FullName + " and hence can not uninstall " + frameworkRelease.FullName + ". " + Environment.NewLine + "If you would like to uninstall " + frameworkRelease.FullName + ", please use the 'Uninstall' option in the Start Menu under " + frameworkRelease.FullName + "!");
					}
				}
				catch (Exception e)
				{
					throw new ApplicationException(String.Format("The directory '{0}' could not be deleted because:\n{1}", release.Path, e.Message));
				}
			}
		}
	}
}
