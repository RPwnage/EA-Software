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

using System.Linq;
using System;

using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using EA.PackageSystem.PackageCore;
using NAnt.Core;
using NAnt.Core.Logging;

namespace EA.PackageSystem.ConsolePackageManager
{
	// TODO: command clean up
	// dvaliant 10/08/2016
	/* ondemand is a required argument, would be nice to have default / masterconfig ways of detecting on demand root used by other 
    commands (see InstallCommand for example */

	[Command("prune")]
	class CleanOndmeandCommand : Command
	{
		internal override string Summary
		{
			get { return "Deletes ondemand releases that have not been depended on in a while."; }
		}

        internal override string Usage
		{
			get { return "<ondemand-folder-path> [-threshold:<days>] [-warn]"; }
		}

        internal override string Remarks
		{
			get
			{
				return @"Description
	This command will delete old packages in an ondemand root based on the referenced timestamp in a packages ondemand metadata file. 
	Framework has been set to update the referenced timestamp whenever there is a dependency on a package or when an ondemand package is the top level package.
	Any package that is missing an ondemand metadata file will not be delete and instead a new metadata file will be created for this package so that it can be deleted in the future.

	The threshold argument can be used to adjust how old (in days) packages must be to be considered safe to delete (the default is 90 days).

	The warn flag can be used to indicate that you only want the command to print warning messages about which packages should be deleted rather than actually delete them.
";
			}
		}

        internal override void Execute()
		{
			if (!Arguments.Any())
			{
				throw new InvalidCommandArgumentsException(this);
			}

			// unlike most commands clean outputs a lot of useful info at detailed level - it makes sense
			// for this to only be at default level when clean up is inside Framework but as part of this command we want more detail by default
			Log cleanLog = new Log
			(
				Log.LogLevel.Detailed,
				listeners: new ILogListener[] { new ConsoleListener() },
				errorListeners: new ILogListener[] { new ConsoleStdErrListener() }
			);

			Project project = new Project(cleanLog); 
			PackageMap.Init(project);

			string ondemandpath = PathString.MakeNormalized(Arguments.First()).ToString();
			int threshold = 90;
			bool warn = false;

            foreach (string arg in Arguments.Skip(1))
            {
                if (arg.StartsWith("-threshold:"))
				{
					threshold = Int32.Parse(arg.Substring(11));
				}
				else if (arg == "-warn")
				{
					warn = true;
				}
				else
				{
					throw new InvalidCommandArgumentsException(this, String.Format("Unknown argument '{0}'.", arg));
				}
			}

			PackageCleanup.CleanupOndemandPackages(project, ondemandpath, threshold, warn);
		}
	}
}
