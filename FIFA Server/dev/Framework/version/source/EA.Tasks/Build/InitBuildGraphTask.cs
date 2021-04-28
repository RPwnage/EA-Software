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
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using EA.FrameworkTasks.Model;

using PackageCore = NAnt.Core.PackageCore;

namespace EA.Eaconfig.Build
{
	/// <summary>
	/// Verify whether we can reuse build graph or need to reset it. This is only needed when we are chaining targets.
	/// </summary>
	[TaskName("init-build-graph")]
	public class Task_InitBuildGraph : Task
	{
		[TaskAttribute("build-group-names", Required = true)]
		public string BuildGroupNames { get; set; }

		[TaskAttribute("build-configurations", Required = true)]
		public string BuildConfigurations { get; set; }

		[TaskAttribute("process-generation-data", Required = false)]
		public bool ProcessGenerationData { get; set; } = false;

		protected override void ExecuteTask()
		{
			if (Project.HasBuildGraph() && !Project.BuildGraph().IsEmpty)
			{
				var buildGraph = Project.BuildGraph();

				if (!buildGraph.CanReuse(BuildConfigurations.ToArray(), BuildGroupNames.ToArray().Select(gn => ConvertUtil.ToEnum<BuildGroups>(gn)), Log, LogPrefix))
				{
					var name = Properties["package.name"];

					var configname = Properties["config"];

					// Packages that are loaded through config package will not be reloaded in this project.
					// instead of foreach(var package in Project.BuildGraph().Packages) reset only packages that have modules in the build graph.
					// these packages will be reloaded during build graph construction.
					foreach (var package in Project.BuildGraph().Modules.Where(m => m.Value.Configuration.Name == configname).Select(m => m.Value.Package).Distinct(new IPackageEqualityComparer()))
					{
						if (package.Name == name)
						{
							continue;
						}
						Project.Properties.RemoveReadOnly(String.Format("package.{0}-{1}.dir", package.Name, package.Version));
						Project.Properties.RemoveReadOnly(String.Format("package.{0}.dir", package.Name));
					}

					Log.Info.WriteLine("Reseting build graph.");

					bool hadOldPackage = Project.TryGetPackage(out IPackage p);

					Project.ResetBuildGraph(); // resets project package

					if (hadOldPackage)
					{
						if (!Project.TrySetPackage(PackageCore.PackageMap.Instance.FindOrInstallCurrentRelease(Project, name), p.ConfigurationName, out IPackage package)) // TODO what is going here?
						{
							throw new BuildException(String.Format("{0}-{1} [{2}] package already set, Framework internal error.", p.Name, p.Version, p.ConfigurationName));
						}
						else
						{
							package.SetType(p.IsKindOf(Package.Buildable) ? Package.Buildable : BitMask.None);
							package.PackageConfigBuildDir = p.PackageConfigBuildDir;
						}
					}

				}
			}
			else
			{
				Log.Info.WriteLine(LogPrefix + "Build graph does not exist");
			}
		}
	}
}
