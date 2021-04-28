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
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules
{
	internal static class ModuleFactory
	{
		static internal ProcessableModule CreateModule(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package)
		{
			ProcessableModule module;

			if (buildType.IsCLR && !buildType.IsManaged)
			{
				module = new Module_DotNet(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);

				if (buildType.IsCSharp) module.SetType(Module.CSharp);
				if (buildType.IsFSharp) module.SetType(Module.FSharp);
				if (OptionSetUtil.GetOptionSetOrFail(module.Package.Project, buildType.Name).GetBooleanOptionOrDefault("windowsruntime", false))
				{
					module.SetType(Module.WinRT);
				}
			}
			else if (buildType.IsJava)
			{
				module = new Module_Java(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
				module.SetType(Module.Java);
				if (buildType.IsApk)
				{
					module.SetType(Module.Apk);
				}
				if (buildType.IsAar)
				{
					module.SetType(Module.Aar);
				}
			}
			else if (buildType.IsMakeStyle)
			{
				module = new Module_MakeStyle(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
			}
			else if (buildType.BaseName == "Utility")
			{
				module = new Module_Utility(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
			}
			else if (buildType.BaseName == "PythonProgram")
			{
				module = new Module_Python(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
			}
			else if (buildType.BaseName == "VisualStudioProject")
			{
				module = new Module_ExternalVisualStudio(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
			}
			else
			{
				module = new Module_Native(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
				OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(module.Package.Project, buildType.Name);

				if (buildType.IsManaged)
				{
					module.SetType(Module.Managed);
				}

				if (buildType.IsLibrary)
				{
					module.SetType(Module.Library);
				}
				else if (buildType.IsDynamic)
				{
					module.SetType(Module.DynamicLibrary);
				}
				else if (buildType.IsProgram)
				{
					module.SetType(Module.Program);
				}
				else
				{
					IList<string> buildtasks = OptionSetUtil.GetOptionOrFail(buildOptionSet, "build.tasks").ToArray();
					if (buildtasks.Contains("link"))
					{
						module.SetType(Module.Program);
					}
					else if (buildtasks.Contains("lib"))
					{
						module.SetType(Module.Library);
					}
				}

				if (buildOptionSet.GetOptionOrDefault("windowsruntime", "off").ToLower() == "on")
				{
					module.SetType(Module.WinRT);
				}

				string slnForceMakeStyle = module.Package.Project.GetPropertyValue(
				String.Format("eaconfig.build.sln.{0}.force-makestyle", module.Configuration.System));
				if (!String.IsNullOrEmpty(slnForceMakeStyle))
				{
					if (slnForceMakeStyle.ToLowerInvariant() == "true")
					{
						module.SetType(Module.ForceMakeStyleForVcproj);
					}
				}
			}

			return module;
		}
	}
}
