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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary>A program that runs through a python interpreter</summary>
	[TaskName("PythonProgram", NestedElementsCheck = true)]
	public class PythonProgramTask : ModuleBaseTask
	{
		public PythonProgramTask()
			: this("PythonProgram")
		{
		}

		protected PythonProgramTask(string buildtype)
			: base(buildtype)
		{
		}

		/// <summary>The python file where the programs execution starts.</summary>
		[TaskAttribute("startupfile", Required = false)]
		public string StartupFile { get; set; } = String.Empty;

		/// <summary>Is this project a windows application, defaults to false.</summary>
		[TaskAttribute("windowsapp", Required = false)]
		public string WindowsApp { get; set; } = "false";

		/// <summary>Sets the working directory for this project.</summary>
		[TaskAttribute("workdir", Required = false)]
		public string WorkDir { get; set; } = ".";

		/// <summary>The projects home directory, search paths and the startup file path
		/// need to be relative to this directory. By default it is the directory containing the
		/// visual studio project files.</summary>
		[TaskAttribute("projecthome", Required = false)]
		public string ProjectHome { get; set; } = ".";

		/// <summary>Custom environment variable setup</summary>
		[OptionSet("environment", Required = false)]
		public StructuredOptionSet Environment { get; set; } = new StructuredOptionSet();

		/// <summary>Adds the list of sourcefiles.</summary>
		[FileSet("sourcefiles", Required = false)]
		public StructuredFileSet SourceFiles { get; } = new StructuredFileSet();

		/// <summary>Adds the list of contentfiles.</summary>
		[FileSet("contentfiles", Required = false)]
		public StructuredFileSet ContentFiles { get; } = new StructuredFileSet();

		/// <summary>A semicolon separated list of directories that will be added to the Search Path.</summary>
		[Property("searchpaths", Required = false)]
		public ConditionalPropertyElement SearchPaths { get; } = new ConditionalPropertyElement();

		protected override void SetupModule() 
		{
			SetModuleProperty("startupfile", StartupFile, false);
			SetModuleProperty("windowsapp", WindowsApp, false);
			SetModuleProperty("workdir", WorkDir, false);
			SetModuleProperty("projecthome", ProjectHome, false);

			SetModuleFileset("sourcefiles", SourceFiles);
			SetModuleFileset("contentfiles", ContentFiles);
			SetModuleProperty("searchpaths", SearchPaths, false);

			SetModuleOptionSet("environment", Environment);

			SetupDependencies();
		}

		protected override void InitModule() { }

		protected override void FinalizeModule() { }
	}
}