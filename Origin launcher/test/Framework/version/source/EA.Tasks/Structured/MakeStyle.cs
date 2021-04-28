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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary>MakeStyle modules are used to execute external build or clean commands</summary>
	[TaskName("MakeStyle", NestedElementsCheck = true)]
	public class MakeStyleTask : ModuleBaseTask
	{
		/// <summary>A makestyle build command, this should contain executable OS command(s)/script</summary>
		[Property("MakeBuildCommand", Required = false)]
		public ConditionalPropertyElement MakeBuildCommand { get; set; } = new ConditionalPropertyElement();

		/// <summary>A makestyle rebuild command, this should contain executable OS command(s)/script</summary>
		[Property("MakeRebuildCommand", Required = false)]
		public ConditionalPropertyElement MakeRebuildCommand { get; set; } = new ConditionalPropertyElement();

		/// <summary>A makestyle clean command, this should contain executable OS command(s)/script</summary>
		[Property("MakeCleanCommand", Required = false)]
		public ConditionalPropertyElement MakeCleanCommand { get; set; } = new ConditionalPropertyElement();

		/// <summary>A list of expected output files (separated by semi-colon.  Only used in setting up Visual Studio vcxproj file.</summary>
		[Property("MakeOutput", Required = false)]
		public ConditionalPropertyElement MakeOutput { get; set; } = new ConditionalPropertyElement();

		/// <summary>Specified that any dependencies listed in this module will be propagated for transitive linking. Only use this if module is a wrapper for a static library.</summary>
		[TaskAttribute("transitivelink")]
		public bool TransitiveLink { get; set; } = false;

		/// <summary>Adds the list of sourcefiles, does not participate directly in the 
		/// build but can be used in generation of projects for external build systems</summary>
		[FileSet("sourcefiles", Required = false)]
		public StructuredFileSet SourceFiles { get; } = new StructuredFileSet();

		/// <summary>Adds the list of asmsourcefiles, does not participate directly in the 
		/// build but can be used in generation of projects for external build systems</summary>
		[FileSet("asmsourcefiles", Required = false)]
		public StructuredFileSet AsmSourceFiles { get; } = new StructuredFileSet();

		/// <summary>Adds the list of headerfiles, does not participate directly in the 
		/// build but can be used in generation of projects for external build systems</summary>
		[FileSet("headerfiles", Required = false)]
		public StructuredFileSet HeaderFiles { get; } = new StructuredFileSet();

		/// <summary>Adds the list of excluded build files, does not participate directly in the 
		/// build but can be used in generation of projects for external build systems</summary>
		[FileSet("excludedbuildfiles", Required = false)]
		public StructuredFileSet ExcludedBuildFiles { get; } = new StructuredFileSet();

		public MakeStyleTask() : base("MakeStyle")
		{
		}

		protected override void SetupModule()
		{
			SetModuleProperty("MakeBuildCommand", MakeBuildCommand.Value);
			SetModuleProperty("MakeCleanCommand", MakeCleanCommand.Value);
			SetModuleProperty("MakeRebuildCommand", MakeRebuildCommand.Value);
			SetModuleProperty("MakeOutput", MakeOutput.Value);

			SetModuleProperty("transitivelink", TransitiveLink ? "true" : "false");

			SetModuleFileset("sourcefiles", SourceFiles);
			SetModuleFileset("asmsourcefiles", AsmSourceFiles);
			SetModuleFileset("headerfiles", HeaderFiles);
			SetModuleFileset("vcproj.excludedbuildfiles", ExcludedBuildFiles);
		}

		protected override void InitModule() { }

		protected override void FinalizeModule() { }
	}
}
