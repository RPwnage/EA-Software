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

namespace EA.Eaconfig
{
	/// <summary>
	/// Extract a list of dependent packages from build graph for all top modules in current package.
	/// </summary>
	[TaskName("get-all-dependent-packages")]
	public class GetAllDependentPackagesTask : Task
	{
		/// <summary>Put the result this this property</summary>
		[TaskAttribute("property", Required=false)]
		public string OutProperty { get; set; }

		public GetAllDependentPackagesTask()
			: base("get-all-dependent-packages")
		{
		}

		protected override void ExecuteTask()
		{
			BuildGraph currBuildGraph = Project.BuildGraph();

			if (!currBuildGraph.IsBuildGraphComplete)
			{
				throw new BuildException("Build graph needs to be setup before you can execute this <get-all-dependent-packages> task.");
			}

			if (currBuildGraph.TopModules == null || !currBuildGraph.TopModules.Any())
			{
				Log.Minimal.WriteLine(LogPrefix + "Build graph doesn't have any top modules!  Nothing to view.");
				return;
			}

			List<IModule> allModules = currBuildGraph.TopModules.Union(currBuildGraph.TopModules.SelectMany(mod => mod.Dependents.FlattenAll(), (m, d) => d.Dependent)).ToList();
			List<string> pkgList = allModules.Select(m => m.Package.Name).Distinct().ToList();
			pkgList.Sort();
			
			System.Text.StringBuilder outString = new System.Text.StringBuilder();
			foreach (string pkgName in pkgList)
			{
				outString.AppendLine(pkgName);
			}
			if (!string.IsNullOrEmpty(OutProperty))
			{
				Project.Properties[OutProperty] = outString.ToString();
			}
			else
			{
				Log.Minimal.WriteLine(Environment.NewLine + "Recursive dependent packages of " + currBuildGraph.TopModules.First().Package.Name + ":" + Environment.NewLine + outString.ToString());
			}
		}
	}
}