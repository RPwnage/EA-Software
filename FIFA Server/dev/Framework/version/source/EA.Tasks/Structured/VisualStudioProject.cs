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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary>Define a module that represents an existing visual studio project file</summary>
	[TaskName("VisualStudioProject", NestedElementsCheck = true)]
	public class VisualStudioProjectTask : ModuleBaseTask
	{
		/// <summary>A Visual Studio Project GUID which is inserted into the generated solution. </summary>
		[TaskAttribute("guid")]
		public string Guid { get; set; }

		/// <summary>Set to true if the project is a managed project</summary>
		[TaskAttribute("managed")]
		public bool Managed { get; set; }

		/// <summary>For unit test C# projects</summary>
		[TaskAttribute("unittest")]
		public bool UnitTest { get; set; }

		/// <summary>The full path to the Visual Studio Project File</summary>
		[Property("ProjectFile", Required = false)]
		public ConditionalPropertyElement ProjectFile { get; set; } = new ConditionalPropertyElement();

		/// <summary>The Visual Studio Project Configuration name that corresponds to the current Framework ${config} value</summary>
		[Property("ProjectConfig", Required = false)]
		public ConditionalPropertyElement ProjectConfig { get; set; } = new ConditionalPropertyElement();

		/// <summary>The Visual Studio Project Platform name that corresponds to the current Framework ${config} value</summary>
		[Property("ProjectPlatform", Required = false)]
		public ConditionalPropertyElement ProjectPlatform { get; set; } = new ConditionalPropertyElement();

		/// <summary>The full path to the project's output assembly</summary>
		[Property("ProjectOutput", Required = true)]
		public ConditionalPropertyElement ProjectOutput { get; set; } = new ConditionalPropertyElement();

		/// <summary>The full path to the project's output symbols</summary>
		[Property("ProjectDebugSymbols", Required = false)]
		public ConditionalPropertyElement ProjectDebugSymbols { get; set; } = new ConditionalPropertyElement();

		public VisualStudioProjectTask() : base("VisualStudioProject")
		{
		}

		protected override void SetupModule()
		{
			SetModuleProperty("project-file", ProjectFile.Value);
			SetModuleProperty("project-config", ProjectConfig.Value);
			SetModuleProperty("project-platform", ProjectPlatform.Value);
			SetModuleProperty("project-output", ProjectOutput.Value);
			SetModuleProperty("project-symbols", ProjectDebugSymbols.Value);			
			SetModuleProperty("project-guid", Guid);
			SetModuleProperty("project-managed", Managed.ToString());
			SetModuleProperty("unittest", UnitTest.ToString());
		}

		protected override void InitModule() { }

		protected override void FinalizeModule() { }

	}
}
