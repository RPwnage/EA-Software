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
using System.IO;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;

using EA.FrameworkTasks.Model;
using EA.Eaconfig.Build;

using EA.Make.MakeItems;



namespace EA.Make.MakeTools
{
	// TODO Make step class hierarchy clean up
	// dvaliant 2016/06/15
	/* class tree is a bit bizarre: 
		- base class MakeToolBase takes a Tool in its constructor (but does not require one due to many null checks, see below)
		- MakeBuildStepBase (inherits directly from MakeToolBase) takes a generic parameter with no constraints but which in nearly all cases is sub 
		  class of Tool - it also passes this to base class but does 'Blah as Tool' to guard against things that aren't tools in the constructor
		- however there is ONE derivation which instead takes a BuildStep - I assume this is the reason the base class is polluted with excessive
		  null checks and the generic class has no contraints
		- there is then also a different direct derivtion of MakeToolBase called MakeBuildToolBase which also takes an unconstrained template type
		  BUT DOES NOTHING WITH IT (implicitly it should be a Tool as well but nothing enforces this)
	 
	   more sane hierarchy would be:
		- base: contains tool / build step independent functionality
		- buildstep derivation: handles all buildstep cases
		- generic tool derivation: handles storage of tool reference as dervied type and all tool functionality
		- specific tool derivations: handle tool specific cases
	*/
	public class MakeToolBase
	{
		public const string DefaultResponseFileTemplate = "@\"%responsefile%\"";

		protected readonly MakeModuleBase MakeModule;

		protected readonly Tool Tool;

		protected readonly MakeFragment ToolPrograms;

		protected readonly MakeFragment ToolOptions;

		protected readonly MakeFragment RulesSection;

		private readonly string TOOL_PREFIX;

		private readonly Dictionary<string, string> _tool_executables = new Dictionary<string, string>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

		protected readonly Project Project;

		protected readonly bool UseAltSepInResponseFile;


		protected MakeToolBase(MakeModuleBase makeModule, Project project, Tool tool)
		{
		   MakeModule = makeModule;
		   Project = project;
		   Tool = tool;
		   ToolPrograms = makeModule.Sections["tool_programs"];
		   ToolOptions = makeModule.Sections["tool_options"];
		   RulesSection = makeModule.Sections["rules_section"];

		   if (tool != null)
		   {
			   TOOL_PREFIX = tool.ToolName.ToUpperInvariant();
		   }
		   else
		   {
			   TOOL_PREFIX = String.Empty;
		   }


		   UseAltSepInResponseFile = GetToolParameter("usealtsepinresponsefile", "false").OptionToBoolean();
		}



		protected string SetToolExecutableVariable(Tool tool =null, FileItem fileItem = null)
		{
			var tool_to_use = tool ?? Tool;

			if (tool_to_use.Executable == null)
			{
				throw new BuildException("Tool.Executable is null");
			}
			string variable_name;
			if (!_tool_executables.TryGetValue(tool_to_use.Executable.Path, out variable_name))
			{
				if (tool_to_use.ToolName == "cc" && MakeModule.ToolWrapers.UseCompilerWrapper)
				{
					var tool_var = SetToolVariable(ToolPrograms, "TOOL", tool_to_use.Executable, fileItem);

					string wrapper_var;
					if (!_tool_executables.TryGetValue(MakeModule.ToolWrapers.CompilerWrapperExecutable.Path, out wrapper_var))
					{
						wrapper_var = SetToolVariable(ToolPrograms, "TOOL_WRAPPER", MakeModule.ToolWrapers.CompilerWrapperExecutable);
					}
					string toolOption = Project.Verbose ? " /verbose" : "";
					if (!String.IsNullOrEmpty(MakeModule.ToolWrapers.CompilerWrapperExtraOptions))
					{
						toolOption = toolOption + " " + MakeModule.ToolWrapers.CompilerWrapperExtraOptions;
					}
					variable_name = SetToolVariable(ToolPrograms, null, wrapper_var.ToMakeValue() + toolOption + " " + tool_var.ToMakeValue(), fileItem);
				}
				else if (tool_to_use.ToolName == "asm" && MakeModule.ToolWrapers.UseAssemblerWrapper)
				{
					var tool_var = SetToolVariable(ToolPrograms, "TOOL", tool_to_use.Executable, fileItem);

					string wrapper_var;
					if (!_tool_executables.TryGetValue(MakeModule.ToolWrapers.AssemblerWrapperExecutable.Path, out wrapper_var))
					{
						wrapper_var = SetToolVariable(ToolPrograms, "TOOL_WRAPPER", MakeModule.ToolWrapers.AssemblerWrapperExecutable);
					}
					string toolOption = Project.Verbose ? " /verbose" : "";
					if (!String.IsNullOrEmpty(MakeModule.ToolWrapers.AssemblerWrapperExtraOptions))
					{
						toolOption = toolOption + " " + MakeModule.ToolWrapers.AssemblerWrapperExtraOptions;
					}
					variable_name = SetToolVariable(ToolPrograms, null, wrapper_var.ToMakeValue() + toolOption + " " + tool_var.ToMakeValue(), fileItem);
				}
				else
				{
					variable_name = SetToolVariable(ToolPrograms, null, tool_to_use.Executable, fileItem);
				}
				_tool_executables.Add(tool_to_use.Executable.Path, variable_name);
			}
			return variable_name;
		}


		protected string SetToolVariable(MakeFragment fragment, string name, PathString value, FileItem fileItem = null)
		{
			return SetToolVariable(fragment, name, MakeModule.ToMakeFilePath(value), fileItem);
		}

		protected string SetToolVariable(MakeFragment fragment, string name, string value, FileItem fileItem = null)
		{
			return fragment.Variable(GetToolVariableName(name, fileItem), value).Label;
		}

		protected string SetToolVariable(MakeFragment fragment, string name, FileSet fs)
		{
			if (fs != null)
			{
				return SetToolVariable(fragment, name, fs.FileItems.Select(fi => fi.Path).Distinct());
			}
			return SetToolVariable(fragment, name, String.Empty);
		}

		protected string SetToolVariable(MakeFragment fragment, string name, IEnumerable<PathString> values)
		{
			return SetToolVariable(fragment, name, values != null ? values.Select(p=>MakeModule.ToMakeFilePath(p.Path)) : null);
		}

		protected string SetToolVariable(MakeFragment fragment, string name, IEnumerable<string> values)
		{
			return fragment.VariableMulti(GetToolVariableName(name), values).Label;
		}

		protected string GetToolVariableName(string name, FileItem fileItem = null)
		{
			if (String.IsNullOrEmpty(name))
			{
				name = TOOL_PREFIX;
			}
			else
			{
				name = TOOL_PREFIX + "_" + name;
			}
			return GetKeyedName(name, fileItem);
		}

		protected string GetKeyedName(string name, FileItem fileItem=null)
		{
			if(fileItem != null && !String.IsNullOrEmpty(fileItem.OptionSetName))
			{
				return String.Format("{0}_{1}", name, fileItem.OptionSetName);
			}

			return name;
		}

		protected string GetToolParameter(FileItem fileItem, string name, string defaultValue = null)
		{
			var tool = fileItem.GetTool();
			if(tool == null || tool.GetType() != Tool.GetType())
			{
				tool = Tool;
			}
			return GetToolParameter(name, defaultValue, tool);
		}

		protected string GetToolParameter(string name, string defaultValue = null, Tool tool = null)
		{
			var parametername = GetToolParameterName(name);

			string val;

			if (!(tool!=null && tool.Templates.TryGetValue(parametername, out val)))
			{
				val = null;
			}
			if (val == null)
			{
				val = Project.GetPropertyValue(parametername);
			}
			return val ?? defaultValue;
		}

		protected void AddCreateDitectoriesRule(MakeTarget target, IList<PathString> createDirectories)
		{
			if (target != null && createDirectories != null)
			{
				foreach (var dir in createDirectories)
				{
					AddCreateDirectoryRule(target, dir);
				}
			}
		}
		protected void AddCreateDirectoryRule(MakeTarget target, PathString dir)
		{
			if (target != null && dir!=null)
			{
				AddCreateDirectoryRule(target, dir.Path);
			}
		}

		protected void AddCreateDirectoryRule(MakeTarget target, String dir)
		{
			if (target != null && !String.IsNullOrEmpty(dir))
			{
				if (-1 ==dir.IndexOf('$'))
				{
					target.AddRuleLine(MakeModule.ToMakeFilePath(dir).MkDir());
					try
					{
						Directory.CreateDirectory(dir.TrimQuotes());
					}
					catch (Exception) { }
				}
				else
				{
					target.AddRuleLine(dir.MkDir());
				}
			}
		}

		protected string GetToolParameterName(string name)
		{
			if(Tool != null && !String.IsNullOrEmpty(Tool.ToolName))
			{
				return (Tool.ToolName == "asm" ? "as" : Tool.ToolName) + ((name[0] == '.') ? String.Empty : ".") + name;
			}
			return "tool" + ((name[0] == '.') ? String.Empty : ".") + name;
		}

		protected virtual IEnumerable<string> GetToolInputDependencies()
		{
			return Tool != null && Tool.InputDependencies != null && !Tool.InputDependencies.FileItems.IsNullOrEmpty() ?
				Tool.InputDependencies.FileItems.Select(item => item.Path.Path) :
				new string[] { };
		}		
		
		protected virtual IEnumerable<string> GetToolOutputDependencies()
		{
			 return Tool != null && Tool.OutputDependencies != null && !Tool.OutputDependencies.FileItems.IsNullOrEmpty() ?
				Tool.OutputDependencies.FileItems.Select(item => item.Path.Path) :
				new string[] { };
		}
	}
}
