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
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;

namespace EA.Eaconfig.Modules
{
	public class Module_ExternalVisualStudio : ProcessableModule
	{
		public override CopyLocalType CopyLocal { get { return CopyLocalType.False; } }
		public override CopyLocalInfo.CopyLocalDelegate CopyLocalDelegate { get { throw new InvalidOperationException("Cannot gather copy local files for external Visual Studio project!"); } }
		public override bool CopyLocalUseHardLink { get { return false; } }

		internal Module_ExternalVisualStudio(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildtype, IPackage package)
			: base(name, groupname, groupsourcedir, configuration, buildgroup, buildtype, package)
		{
			SetType(Module.ExternalVisualStudio);
		}

		public Module_ExternalVisualStudio(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, IPackage package)
			: this(name, groupname, groupsourcedir, configuration, buildgroup, new BuildType("MakeStyle", "MakeStyle", isMakestyle:true), package)
		{
		}

		public override void Apply(IModuleProcessor processor)
		{
			processor.Process(this);
		}

		public bool IsProjectType(DotNetProjectTypes test)
		{
			// using bitwise & operator to test a single bit of the bitmask
			return (DotNetProjectTypes & test) == test;
		}

		public PathString VisualStudioProject;
		public string VisualStudioProjectConfig;
		public string VisualStudioProjectPlatform;
		public Guid VisualStudioProjectGuild;
		public DotNetProjectTypes DotNetProjectTypes = 0;
	}
}

