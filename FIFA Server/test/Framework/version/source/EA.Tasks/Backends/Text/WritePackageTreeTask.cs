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

using NAnt.Core;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.Text
{
	/// <summary>
	/// This Task prints the dependency tree for a specified package. Shows all of the packages
	/// that are added as package dependents in the current configuration.
	/// </summary>
	[TaskName("WritePackageTree")]
	public class WritePackageTreeTask : Task
	{

		/// <summary>
		/// The name of the package whose dependency tree should be printed.
		/// </summary>
		[TaskAttribute("packagename", Required = true)]
		public string PackageName { get; set; }

		protected override void ExecuteTask()
		{
			Log.Status.WriteLine("Package dependencies");
			Log.Status.WriteLine();
			int indent = 0;
			// Print package dependency tree for the specified package.
			foreach (IPackage package in Project.BuildGraph().Packages.Values.Where(p => p.Name == PackageName))
			{
				Log.Status.WriteLine("    Configuration: {0}", package.ConfigurationName);
				Log.Status.WriteLine();

				PrintPackage(package, indent, new HashSet<string>());
			}
			Log.Status.WriteLine();
		}

		private void PrintPackage(IPackage package, int indent, HashSet<string> printedPackageNames)
		{
			if (printedPackageNames.Contains(package.Name))
			{
				Log.Status.WriteLine("{0}{1}-{2} [{3}] (circular dependency)", String.Empty.PadLeft(indent), package.Name, package.Version, package.ConfigurationName);
				return;
			}

			Log.Status.WriteLine("{0}{1}-{2} [{3}]", String.Empty.PadLeft(indent), package.Name, package.Version, package.ConfigurationName);

			printedPackageNames.Add(package.Name);
			indent += 4;
			foreach (var dependent in package.DependentPackages)
			{
				PrintPackage(dependent.Dependent, indent, printedPackageNames);
			}
			printedPackageNames.Remove(package.Name);
		}
	}
}

