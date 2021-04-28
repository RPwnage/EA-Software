// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2016 Electronic Arts Inc.
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

using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Modules.Tools
{
	public abstract class NativeCompilerTool : Tool
	{
		public NativeCompilerTool(string toolName, PathString toolExe, string description = "", PathString workingDir = null)
			: base(toolName, toolExe, Tool.TypeBuild, description, workingDir)
		{
			Defines = new SortedSet<string>();
			IncludeDirs = new List<PathString>();
			SystemIncludeDirs = new List<PathString>();
			SourceFiles = new FileSet();
		}

		// We probably could update and setup the <compiler> SXML block to include this template.  But that would be unnecessary touching
		// a lot of different external config packages that I may not even have control.  In general, we are only dealing with only two type
		// of compiler (Visual Studio or clang) and they are consistent with different platform for the same compiler type.  So for now,
		// we just hard code this.  If this assumption is no longer true, we can refactor this in the future.
		public string GetForcedIncludeCcSwitch(Module_Native module)
		{
			switch (module.Configuration.Compiler)
			{
				case "vc":
					return "/FI \"" + module.PrecompiledHeaderFile + "\"";
				case "clang":
					return "-include \"" + module.PrecompiledHeaderFile + "\"";
				default:
					return string.Empty;
			}
		}

		public readonly SortedSet<string> Defines;
		public readonly List<PathString> IncludeDirs;
		public readonly List<PathString> SystemIncludeDirs;
		public readonly FileSet SourceFiles;
		public string ObjFileExtension;
	}
}
