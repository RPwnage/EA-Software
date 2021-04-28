// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

using EA.Eaconfig;
using EA.FrameworkTasks.Functions;

namespace EA.FrameworkTasks.Model
{
	public class Package : BitMask, IPackage
	{
		public const uint AutoBuildClean = 1; // Should we try to build/clean it
		public const uint Framework2 = 2;
		public const uint Buildable = 4;  // Is package buildable at all (declared in the manifest)
		public const uint FinishedLoading = 8;  

		public Package(Release release, string configurationName)
		{
			Release = release;
			Dir = PathString.MakeNormalized(release.Path);
			ConfigurationName = configurationName;
			Key = MakeKey(release.Name, release.Version, configurationName);

			Macros = new MacroMap
			{
				{ "packagename", Name },
				{ "package.name", Name },
				{ "package.root", () => PackageFunctions.GetPackageRoot(Project, Name) },
				{ "builddir", () => PackageBuildDir.Path },
				{ "package.builddir", () => PackageBuildDir.Path },
				{ "package.configbuilddir", () => PackageConfigBuildDir.Path },
				{ "config", configurationName },
				{ "configname", configurationName },
				{ "config.name", configurationName }
			};
		}

		public string Name => Release.Name;
		public string Version => Release.Version;

		public PathString Dir { get; }

		public string ConfigurationName { get; }

		public string Key { get; }

		public IEnumerable<IModule> Modules
		{
			get { return m_modules; }
		}

		public ReadOnlyCollection<IModule> ModulesNonLazy
		{
			get { return m_modules.AsReadOnly(); }
		}

		public Project Project { get; set; }
		public MacroMap Macros { get; private set; }

		public PathString PackageBuildDir { get; set; }
		public PathString PackageConfigBuildDir { get; set; }
		public PathString PackageGenDir { get; set; }
		public PathString PackageConfigGenDir { get; set; }

		public Release Release { get; private set; }
		public Dependency<IPackage>[] DependentPackages { get; private set; } = new Dependency<IPackage>[0];

		private readonly List<IModule> m_modules = new List<IModule>();
		private readonly object m_depPackagesLock = new object();
		private readonly Dictionary<string, Dependency<IPackage>> m_depPackages = new Dictionary<string, Dependency<IPackage>>();
		private readonly ConcurrentDictionary<string, IPublicData> m_publicdata = new ConcurrentDictionary<string, IPublicData>();

		public void AddModule(IModule module)
		{
			m_modules.Add(module);
		}

		public override string ToString()
		{
			return Name + "-" + Version;
		}

		public static string MakeKey(string name, string version, string config)
		{
			return config + "-" + name + "-" + version;
		}

		public bool TryGetPublicData(out IPublicData data, IModule parentModule, IModule dependentModule)
		{
			return m_publicdata.TryGetValue(makepublicdatakey(parentModule, dependentModule), out data);
		}

		public bool TryAddPublicData(IPublicData data, IModule parentModule, IModule dependentModule)
		{
			return m_publicdata.TryAdd(makepublicdatakey(parentModule, dependentModule), data);
		}

		public IEnumerable<IPublicData> GetModuleAllPublicData(IModule owner)
		{
			return m_publicdata.Where(e => e.Key.EndsWith((owner != null) ? owner.Key : "package")).Select(e=>e.Value);
		}

		internal bool TryAddDependentPackage(IPackage dependent, uint deptype)
		{
			lock (m_depPackagesLock)
			{
				if (m_depPackages.TryGetValue(dependent.Key, out Dependency<IPackage> value))
				{
					value.SetType(deptype);
				}
				else
				{
					m_depPackages.Add(dependent.Key, new Dependency<IPackage>(dependent, BuildGroups.runtime, deptype));
					DependentPackages = m_depPackages.Values.ToArray();
				}
			}
			return true;
		}

		private string makepublicdatakey(IModule parentModule, IModule dependentModule)
		{
			return "public_data_" + parentModule.Key + "_" + dependentModule.Key;
		}
	}
}
