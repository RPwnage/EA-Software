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
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Modules.Tools
{
	internal static class ToolToOptionSetExt
	{
		internal static void ToOptionSet<T>(this Tool tool, OptionSet set, T module) where T : Module
		{
			if (tool != null)
			{
				if (tool is CcCompiler)
					((CcCompiler)tool).ToOptionSet(set, module);
				else if (tool is AsmCompiler)
					((AsmCompiler)tool).ToOptionSet(set, module);
				else if (tool is Linker)
					((Linker)tool).ToOptionSet(set, module);
				else if (tool is Librarian)
					((Librarian)tool).ToOptionSet(set, module);
				else if (tool is PostLink)
					((PostLink)tool).ToOptionSet(set, module);
			}
		}

		internal static void ToOptionSet<T>(this CcCompiler tool, OptionSet set, T module) where T : Module
		{
			InitSet(tool, set);

			if (tool != null)
			{
				Module_Native native_module = module as Module_Native;

				set.Options[tool.ToolName + ".defines"] = tool.Defines.ToString(Environment.NewLine);
				set.Options[tool.ToolName + ".compilerinternaldefines"] = tool.CompilerInternalDefines.ToString(Environment.NewLine);
				set.Options[tool.ToolName + ".objfile.extension"] = tool.ObjFileExtension;

				//Assign this option to use in NAnt compiler task  dependency 
				if (tool.PrecompiledHeaders == CcCompiler.PrecompiledHeadersAction.CreatePch)
				{
					set.Options["create-pch"] = "on";
					set.Options["pch-file"] = (native_module != null) ? native_module.PrecompiledFile.Path : String.Empty;
				}
			}
		}


		internal static void ToOptionSet<T>(this AsmCompiler tool, OptionSet set, T module) where T : Module
		{
			InitSet(tool, set, "as");

			if (tool != null)
			{
				set.Options["as.defines"] = tool.Defines.ToString(Environment.NewLine);
				set.Options["as.objfile.extension"] = tool.ObjFileExtension;
			}
		}

		internal static void ToOptionSet<T>(this Linker tool, OptionSet set, T module) where T : Module
		{
			InitSet(tool, set);

			if (tool != null)
			{
				set.Options[tool.ToolName + ".librarydirs"] = StringUtil.ArrayToString(tool.LibraryDirs.Select<PathString, string>(item => item.Normalize().Path), Environment.NewLine);
			}
		}

		internal static void ToOptionSet<T>(this PostLink tool, OptionSet set, T module)  where T : Module
		{
			if (tool != null)
			{
				set.Options["link.postlink.program"] = tool.Executable.Path;
				set.Options["link.postlink.commandline"] = StringUtil.ArrayToString(tool.Options, Environment.NewLine);
			}
		}


		internal static void ToOptionSet<T>(this Librarian tool, OptionSet set, T module)
		{
			InitSet(tool, set);
		}


		private static void InitSet(Tool tool, OptionSet set, string altToolName = null)
		{
			if (tool != null)
			{
				var toolName = altToolName ?? tool.ToolName;

				foreach (string option_name in option_names)
				{
					set.Options[toolName + option_name] = null;
				}

				foreach (var entry in tool.Templates)
				{
					set.Options[entry.Key] = entry.Value;
				}

				if (!tool.WorkingDir.IsNullOrEmpty())
				{
					set.Options[toolName + ".workingdir"] = tool.WorkingDir.Path;
				}

				set.Options[toolName] = tool.Executable.Path;
				set.Options[toolName + ".options"] = StringUtil.ArrayToString(tool.Options, Environment.NewLine);
			}
		}

		private static List<string> option_names = new List<string>{ ".defines", ".includedirs", ".usingdirs", ".objfile.extension" };
	}
}
