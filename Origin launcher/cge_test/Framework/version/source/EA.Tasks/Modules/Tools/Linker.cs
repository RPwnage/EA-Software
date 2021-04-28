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

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;


namespace EA.Eaconfig.Modules.Tools
{
	public class Linker : Tool
	{
		public Linker(PathString toolexe, PathString workingDir = null) 
			: base("link", toolexe, Tool.TypeBuild, workingDir:workingDir)
		{
			LibraryDirs = new List<PathString>();
			Libraries = new FileSet();
			DynamicLibraries = new FileSet();
			Libraries.FailOnMissingFile = false;
			ObjectFiles = new FileSet();
			ObjectFiles.FailOnMissingFile = false;
			UseLibraryDependencyInputs = false;
		}

		public readonly List<PathString> LibraryDirs;
		public readonly FileSet Libraries;
		public readonly FileSet DynamicLibraries;  // For assemblies and other cases when we need to link to DLL.
		public readonly FileSet ObjectFiles;
		public string OutputName;
		public string OutputExtension;
		public PathString LinkOutputDir;		// TODO replace LinkOutputDir usage iwth module.OutputDir
												// -dvaliant 2016/05/10
												// anywhere LinkOutputDir is used module.OutputDir should suffice, if you look at how
												// Lib tool is defined it doesn't have a member for output path, no reason for link
												// tool to have one that I can tell. Getting rid of this will reduce complexity
												// and we won't have to make sure any transformations applied to module outputdir
												// also apply to link outputdir
		public PathString ImportLibOutputDir;
		public PathString ImportLibFullPath;
		public bool UseLibraryDependencyInputs;
	}
}
