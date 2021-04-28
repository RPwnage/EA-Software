// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2020 Electronic Arts Inc.
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
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace EA.Eaconfig.Structured
{
	/// <summary>A structured XML Shared PCH module information export specification to be used in Initialize.xml file</summary>
	[TaskName("sharedpchmodule")]
	public class SharedPchPublicData : ModulePublicData
	{
		/// <summary>
		/// Precompile Header (.h)'s filename
		/// </summary>
		[TaskAttribute("pchheader", Required = true)]
		public string PchHeader { set; get; }

		/// <summary>
		/// Directory where the 'pchheader' is located.
		/// </summary>
		[TaskAttribute("pchheaderdir", Required = true)]
		public string PchHeaderDir { set; get; }

		/// <summary>
		/// Full path to source file used to generate the pch
		/// </summary>
		[TaskAttribute("pchsource", Required = true)]
		public string PchSource { set; get; }

		/// <summary>
		/// Full path to the pch binary output
		/// </summary>
		[TaskAttribute("pchfile", Required = false)]
		public string PchFile { set; get; }

		public SharedPchPublicData() : base()
		{
		}

		protected override void ExecuteTask()
		{
			string finalBuildType = BuildType;
			if (String.IsNullOrEmpty(finalBuildType))
			{
				finalBuildType = "Library";
			}

			string configSystem = Project.Properties["config-system"];
			if (!Project.Properties.GetBooleanPropertyOrDefault(configSystem + ".support-shared-pch", false) ||
				!Project.Properties.GetBooleanPropertyOrDefault("eaconfig.use-shared-pch-binary", true) ||
				!Project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
			{
				finalBuildType = "Utility";
			}

			BuildType = finalBuildType;

			base.ExecuteTask();

			SetModuleProperty("issharedpchmodule", "true");

			SetModuleProperty("pchheader", PchHeader);
			SetModuleProperty("pchheaderdir", PchHeaderDir);
			SetModuleProperty("pchsource", PchSource);
			string pchfile = PchFile;
			if (string.IsNullOrEmpty(pchfile))
			{
				pchfile = Path.Combine(Project.Properties["package." + PackageName + ".libdir"], ModuleName + ".pch");
			}
			SetModuleProperty("pchfile", pchfile);
		}
	}
}
