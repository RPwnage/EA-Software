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

using System.Collections.Generic;

using NAnt.Core.Util;

using EA.Eaconfig;
using EA.Eaconfig.Build;

namespace EA.FrameworkTasks.Model
{
	public interface IModule
	{
		string Name { get; }
		string GroupName { get; }
		string GroupSourceDir { get; }

		BuildGroups BuildGroup { get; }

		string Key { get; }

		IPackage Package { get; }

		Configuration Configuration { get; }

		DependentCollection Dependents { get; }

		bool IsKindOf(uint t);

		bool CollectAutoDependencies { get; }

		string OutputName { get; }

		PathString OutputDir { get; set; }

		PathString IntermediateDir { get; set; }

		PathString ScriptFile { get; set; }

		IPublicData Public(IModule parentModule);

		ISet<IModule> ModuleSubtree(uint dependencytype, ISet<IModule> modules);

		/// <summary>
		/// <para>
		/// Whether this module treats all transitive dependencies as copylocal automatically.
		/// </para>
		/// <para>
		/// Most module types set undefined (use copy local dependencies to determine copy local to this module).
		/// Some modules types set false (never copy local to this module).
		/// DotNet and Native modules allow users to control copy local types.
		/// </para>
		/// </summary>
		CopyLocalType CopyLocal { get; }

		/// <summary>
		/// Algorithm this module uses to gather copy local files from dependents.
		/// For example, most modules gather all exe or shared lib outputs but native modules only gather native exes or shared libs.
		/// </summary>
		CopyLocalInfo.CopyLocalDelegate CopyLocalDelegate { get; }

		/// <summary>
		/// Whether post build copies should use hard links.
		/// Can be set to true by user for native modules, false otherwise.
		/// </summary>
		bool CopyLocalUseHardLink { get; }


		/// <summary>A list of all files that should be copied by this module as part of copy local feature</summary>
		List<CopyLocalInfo> CopyLocalFiles { get; }

		/// <summary>A set of files that should exist alongside the module output after the build either by being built directly to
		/// that location or via a copy (latter being from CopyLocalFiles)</summary>
		HashSet<string> RuntimeDependencyFiles { get; }

		string LogPrefix { get; }

		// useful paths
		PathString ModuleConfigGenDir { get; }
		PathString ModuleConfigBuildDir { get; }

		// returns the first found path that make a dependency chain from this module to another
		// module, intended for debugging only
		IModule[] DependencyPathTo(IModule otherModule);

		MacroMap Macros { get; }
	}
}
