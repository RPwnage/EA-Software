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
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.CPlusPlusTasks;

using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Build;

using EA.Make.MakeItems;


namespace EA.Make.MakeTools
{
	class MakeCompileAsmTarget : MakeBuildToolBase<AsmCompiler>
	{
		private readonly AsmCompiler Compiler;

		public readonly List<string> ObjectFiles = new List<string>();

		public MakeTarget AsmTarget;

		private readonly HashSet<string> _optionsVars = new HashSet<string>();


		public MakeCompileAsmTarget(MakeModuleBase makeModule, Module_Native module)
			: base(makeModule, module, module.Asm)
		{
			Compiler = module.Asm;
		}

		public void GenerateTargets()
		{
			if (Compiler == null || Compiler.SourceFiles.FileItems.Count == 0)
			{
				return;
			}

			if (Module.Configuration.Compiler == "vc" && Module.Configuration.System.StartsWith("pc"))
			{
				MakeModule.ToolWrapers.UseAssemblerWrapper = true;
				MakeModule.ToolWrapers.AssemblerWrapperExtraOptions = "/src:\"$<\" /obj:\"$@\" /dep:\"$(patsubst %.obj,%.d,$@)\"";
				MakeModule.ToolWrapers.AssemblerWrapperExecutable = new PathString(MakeModule.ToMakeFilePath(Path.Combine(Project.NantLocation, "ml_wrapper.exe")));
			}

			var cc_macro = CompilerInvocationMacro();

			var compilervars = new HashSet<string>();

			foreach (var fileItem in Compiler.SourceFiles.FileItems)
			{
				string optionsetName;
				var compiler = GetCompiler(fileItem, out optionsetName);

				if (compiler != null)
				{
					var cc = SetToolExecutableVariable(compiler, fileItem);

					var obj_file = MakeModule.ToMakeFilePath(ObjectFilePath(fileItem));

					ObjectFiles.Add(obj_file);

					var cc_target = MakeModule.MakeFile.Target(obj_file);

					cc_target.Prerequisites += MakeModule.ToMakeFilePath(fileItem.Path);

					cc_target.Prerequisites += "MAKEFILE".ToMakeValue();

					string cc_options;
					string obj_options;
					string src_options;

					SetCompilerOptions(compiler, fileItem, optionsetName, out cc_options, out obj_options, out src_options);

					// Note that Microsoft's ml.exe require output -Fo[X].obj arguments come before source files.
					cc_target.AddRuleLine("@$(call {0},$({1}),$({2}),{3},{4})", cc_macro, cc, cc_options, obj_options.TrimWhiteSpace(), src_options.TrimWhiteSpace());

					MakeModule.EmptyRecipes.AddSuffixRecipy(fileItem.Path);
					MakeModule.AddFileToClean(obj_file);

				}
			}

			var objs = ToolOptions.VariableMulti("AS_OBJS", ObjectFiles);

			ToolOptions.LINE = "";
			ToolOptions.LINE = "ifneq ($(wildcard $(AS_OBJS)),)";
			ToolOptions.LINE = "-include $(patsubst %.obj,%.d,$(wildcard $(AS_OBJS)))";
			ToolOptions.LINE = "endif";
			ToolOptions.LINE = "";

			AsmTarget = MakeModule.MakeFile.Target(objs.Label.ToMakeValue());
		}

		private string CompilerInvocationMacro()
		{
			var define = new MakeDefine(GetKeyedName("COMPILE_ASM"));

			define.AddCommentLine("[{0}] Assembling file $<", Module.Name);
			define.AddLine("$(@D)".MkDir());
			define.AddLine(@"$(1) $(2) $(3) $(4)");

			RulesSection.Items.Add(define);

			return define.Label;
		}


		private string GetCompilerOptions(AsmCompiler compiler, Dictionary<string, Tuple<string, string>> extractTokens)
		{
			StringBuilder options = new StringBuilder("");
			foreach (string op in compiler.Options)
			{
				string foundTokenKey = null;
				foreach (string ky in extractTokens.Keys)
				{
					if (op.Contains(ky))
					{
						foundTokenKey = ky;
						break;
					}
				}
				if (foundTokenKey != null)
				{
					string tokenReplacement = extractTokens[foundTokenKey].Item1;
					StringBuilder currentOptionValue = null;
					if (!String.IsNullOrEmpty(extractTokens[foundTokenKey].Item2))
					{
						currentOptionValue = new StringBuilder(extractTokens[foundTokenKey].Item2);
						currentOptionValue.Append(" ");
					}
					else
					{
						currentOptionValue = new StringBuilder();
					}
					currentOptionValue.Append(op.Replace("\""+foundTokenKey+"\"", tokenReplacement).Replace(foundTokenKey,tokenReplacement));

					extractTokens[foundTokenKey] = new Tuple<string, string>(tokenReplacement, currentOptionValue.ToString());
				}
				else
				{
					if (options.Length > 0)
					{
						options.Append(OptionsSeparator);
					}
					options.Append(MakeExtensions.QuoteOptionAsNeeded(op));
				}
			}
			return options.ToString();
		}


		protected void SetCompilerOptions(AsmCompiler compiler, FileItem fileItem, string optionsetName, out string cc_options, out string obj_options, out string src_options)
		{

			var commandlineTemplate = new StringBuilder(GetTemplate("commandline", DefaultCompilerCommandLineTemplate, tool: compiler));

			obj_options = ExtractTemplateFieldWithOption(commandlineTemplate, "%objectfile%").Replace("\"%objectfile%\"", "$@").Replace("%objectfile%", "$@").ToString().TrimWhiteSpace();
			src_options = ExtractTemplateFieldWithOption(commandlineTemplate, "%sourcefile%").Replace("\"%sourcefile%\"", "$<").Replace("%sourcefile%", "$<").ToString().TrimWhiteSpace();

			SortedSet<string> definesList = compiler.Defines;
			definesList.Add("EA_MAKE_BUILD=1");

			var defines = ReplaceTemplate(GetTemplate("define", tool: compiler), "%define%", definesList, MakeExtensions.QuoteOptionAsNeeded);
			var includedirs = ReplaceTemplate(GetTemplate("includedir", tool: compiler), "%includedir%", compiler.IncludeDirs);
			var systemincludedirs = ReplaceTemplate(GetTemplate("system-includedir", tool: compiler), "%system-includedir%", compiler.SystemIncludeDirs);

			// If following special nant tokens are present in the option itself, they need to be converted to makefile tokens.  
			// However, if we turn on using response files, we cannot have any makefile tokens show up in response files. We need to strip
			// them from the standard build options list and handle the token stuff separately.
			Dictionary<string, Tuple<string, string>> extractTokens = new Dictionary<string, Tuple<string, string>>();
			extractTokens.Add(EA.CPlusPlusTasks.Dependency.DependencyFileVariable, new Tuple<string, string>("$(patsubst %.obj,%.d,$@)", null));
			extractTokens.Add("%objectfile%", new Tuple<string, string>("$@", null));
			extractTokens.Add("%sourcefile%", new Tuple<string, string>("$<", null));

			string asmOptions = GetCompilerOptions(compiler, extractTokens);

			if (UseAltSepInResponseFile)
				asmOptions = asmOptions.PathToUnix(); // Some compilers (like unixclang on windows) don't like its parameters passed with \ character instead of treating it as a path separator it treats it as a the start of a special character like \n

			// Process the special token option that is extracted and put them to eother obj_options or src_options group.
			if (!String.IsNullOrEmpty(extractTokens["%sourcefile%"].Item2))
			{
				src_options = string.IsNullOrEmpty(src_options)
					? extractTokens["%sourcefile%"].Item2
					: extractTokens["%sourcefile%"].Item2 + " " + src_options;
			}
			if (!String.IsNullOrEmpty(extractTokens["%objectfile%"].Item2))
			{
				obj_options = string.IsNullOrEmpty(obj_options)
					? extractTokens["%objectfile%"].Item2
					: extractTokens["%objectfile%"].Item2 + " " + obj_options;
			}
			if (!String.IsNullOrEmpty(extractTokens[EA.CPlusPlusTasks.Dependency.DependencyFileVariable].Item2))
			{
				obj_options = string.IsNullOrEmpty(obj_options)
					? extractTokens[EA.CPlusPlusTasks.Dependency.DependencyFileVariable].Item2
					: extractTokens[EA.CPlusPlusTasks.Dependency.DependencyFileVariable].Item2 + " " + obj_options;
			}

			commandlineTemplate.Replace("%defines%", defines);
			commandlineTemplate.Replace("%includedirs%", includedirs);
			commandlineTemplate.Replace("%system-includedirs%", includedirs);
			commandlineTemplate.Replace("%options%", asmOptions);
			// There are no usingdirs or forceusing-assemblies for the assemblier.  Just in case someone added them to the commandline template by copying the cc
			// template, we'll just check for these tokens and remove them.
			commandlineTemplate.Replace("%usingdirs%", "");
			commandlineTemplate.Replace("%forceusing-assemblies", "");

			commandlineTemplate.Replace("%outputdir%", MakeModule.ToMakeFilePath(Module.IntermediateDir));
			commandlineTemplate.Replace("%outname%", Module.OutputName);

			cc_options = GetToolVariableName("OPTIONS" + optionsetName);
			if (_optionsVars.Add(cc_options))
			{
				cc_options = SetToolVariable(ToolOptions, "OPTIONS" + optionsetName, commandlineTemplate.ToString().LinesToArray());
			}
		}

		public virtual string ObjectFilePath(FileItem fileItem)
		{
			string objPath = "";

			EA.Eaconfig.Core.FileData filedata = fileItem.Data as EA.Eaconfig.Core.FileData;

			if (filedata != null && filedata.ObjectFile != null && !String.IsNullOrEmpty(filedata.ObjectFile.Path))
			{
				objPath = filedata.ObjectFile.Path;
			}
			else
			{
				objPath = CompilerBase.GetOutputBasePath(fileItem, Module.IntermediateDir.Path, fileItem.BaseDirectory ?? Compiler.SourceFiles.BaseDirectory) + GetObjectFileExtension(fileItem);
			}
			return objPath;
		}

		protected string ObjectFileExtensionProperty { get { return "as.objfile.extension"; } }

		public string GetObjectFileExtension(FileItem fileItem)
		{
			string ext = GetOption(fileItem, ObjectFileExtensionProperty);
			if (ext == null)
			{
				ext = Compiler.ObjFileExtension; 
			}
			return ext;
		}

		public string GetOption(FileItem fileItem, string propertyName)
		{
			string propertyValue = null;
			if (fileItem != null && fileItem.OptionSetName != null)
			{
				var tool = fileItem.GetTool() ?? Compiler;
				var comp = tool as AsmCompiler;

				if (comp != null)
				{
					propertyValue = comp.Templates[propertyName];
				}

			}

			if (propertyValue == null)
			{
				propertyValue = Compiler.Templates[propertyName];
			}

			return propertyValue;
		}


		private AsmCompiler GetCompiler(FileItem fileItem, out string optionsetName)
		{
			AsmCompiler compiler = null;

			optionsetName = String.Empty;

			if (fileItem != null && fileItem.OptionSetName != null)
			{
				compiler = fileItem.GetTool() as AsmCompiler;
				if (compiler != null)
				{
					optionsetName = fileItem.OptionSetName;
				}
			}
			return compiler ?? Compiler;
		}

	}
}
