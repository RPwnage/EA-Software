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

using System.Text;
using System.Collections.Generic;
using System.Collections.ObjectModel;

using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

using EA.Eaconfig;

namespace EA.FrameworkTasks.Model
{
	public interface IPackage
	{
		string Name { get; }
		string Version { get; }
		string ConfigurationName { get; }
		string Key { get; }

		PathString Dir { get; }

		void AddModule(IModule module);

		IEnumerable<IModule> Modules { get; }
		ReadOnlyCollection<IModule> ModulesNonLazy { get; }

		Dependency<IPackage>[] DependentPackages { get; }

		bool IsKindOf(uint t);

		void SetType(uint t);

		bool TryGetPublicData(out IPublicData data, IModule parentModule, IModule dependentModule);

		bool TryAddPublicData(IPublicData data, IModule parentModule, IModule dependentModule);

		PathString PackageBuildDir { get; set; }

		PathString PackageConfigBuildDir { get; set; }

		PathString PackageGenDir { get; set; }

		PathString PackageConfigGenDir { get; set; }

		Project Project { get; set; }

		MacroMap Macros { get; }
		Release Release { get; }
	}

	public struct PackageDependencyTypes
	{
		public const uint Use = 1;
		public const uint Build = 2;

		public static string ToString(uint type)
		{
			StringBuilder sb = new StringBuilder();

			test(type, Use, "use", sb);
			test(type, Build, "build", sb);

			return sb.ToString().TrimEnd('|');
		}

		private static void test(uint t, uint mask, string name, StringBuilder sb)
		{
			if ((mask & t) != 0) sb.AppendFormat("{0}|", name);
		}

	}


	public class IPackageEqualityComparer : IEqualityComparer<IPackage>
	{
		public bool Equals(IPackage x, IPackage y)
		{
			return x.Key == y.Key;
		}

		public int GetHashCode(IPackage obj)
		{
			return obj.Key.GetHashCode();
		}
	}

}
