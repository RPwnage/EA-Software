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
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using EA.CPlusPlusTasks;

using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Build;

using EA.Make.MakeItems;


namespace EA.Make.MakeTools
{
	class MakeCompileTarget : MakeBuildToolBase<CcCompiler>
	{
		private readonly CcCompiler Compiler;

		public readonly List<string> ObjectFiles = new List<string>();

		public MakeTarget CcTarget;

		private readonly HashSet<string> _optionsVars = new HashSet<string>();
		

		public MakeCompileTarget(MakeModuleBase makeModule, Module_Native module)
			: base(makeModule, module, module.Cc)
		{
			Compiler = module.Cc;
		}

		public void GenerateTargets()
		{
			if (Compiler == null || Compiler.SourceFiles.FileItems.Count == 0)
			{
				return;
			}

			if (Module.Configuration.Compiler == "vc" && Module.Configuration.System.StartsWith("pc"))
			{
				MakeModule.ToolWrapers.UseCompilerWrapper = true;
				MakeModule.ToolWrapers.CompilerWrapperExtraOptions = "/src:\"$<\" /obj:\"$@\" /dep:\"$(patsubst %.obj,%.d,$@)\"";
				MakeModule.ToolWrapers.CompilerWrapperExecutable = new PathString(MakeModule.ToMakeFilePath(Path.Combine(Project.NantLocation, "cl_wrapper.exe")));
			}

			var cc_macro = CompilerInvocationMacro();

			var  compilervars = new HashSet<string>();

			string pchMakeTargetFile = null;

			foreach(var fileItem in Compiler.SourceFiles.FileItems)
			{
				string optionsetName;
				var compiler = GetCompiler(fileItem, out optionsetName);

				if (compiler != null)
				{
					string cc = SetToolExecutableVariable(compiler, fileItem);

					string pchFile = null;
					if (compiler.PrecompiledHeaders == CcCompiler.PrecompiledHeadersAction.CreatePch)
					{
						pchFile = Module.PrecompiledFile != null ? Module.PrecompiledFile.Path : null;
						// The MakeModule.ToMakeFilePath() used later requires input path to be using specific path style format.
						// So re-normalize the path to specific format.
						pchFile = PathString.MakeNormalized(pchFile).Path;
					}

					string makeTargetFilePath = null;
					string makeTargetFile = null;
					bool usingPchFile = false;
					if (!string.IsNullOrEmpty(pchFile))
					{
						if (Module.Configuration.Compiler == "clang")
						{
							usingPchFile = true;
							makeTargetFilePath = pchFile;
							makeTargetFile = MakeModule.ToMakeFilePath(pchFile);
							pchMakeTargetFile = makeTargetFile;
						}
						else // for vc compiler
						{
							// For vc compiler, we still need to generate the obj for this cpp file and link with this obj.
							makeTargetFilePath = ObjectFilePath(fileItem);
							makeTargetFile = MakeModule.ToMakeFilePath(makeTargetFilePath);
							// We need to make sure that this is built first.
							ObjectFiles.Insert(0, makeTargetFile);
							pchMakeTargetFile = makeTargetFile;
						}
					}
					else
					{
						makeTargetFilePath = ObjectFilePath(fileItem);
						makeTargetFile = MakeModule.ToMakeFilePath(makeTargetFilePath);
						ObjectFiles.Add(makeTargetFile);
					}

					MakeTarget cc_target = MakeModule.MakeFile.Target(makeTargetFile);

					cc_target.Prerequisites += MakeModule.ToMakeFilePath(fileItem.Path.ToString());

					cc_target.Prerequisites += "MAKEFILE".ToMakeValue();

					string cc_options;
					string obj_options;
					string src_options;
					PathString rspfile;

					SetCompilerOptions(compiler, fileItem, optionsetName, out cc_options, out obj_options, out src_options, out rspfile, usingPchFile);

					if(!rspfile.IsNullOrEmpty())
					{
						cc_target.Prerequisites += MakeModule.ToMakeFilePath(rspfile);
					}

					// Listing source file last to be consistent with the ASM compile MakeTarget setup.
					cc_target.AddRuleLine(GetQuietSymbol() + 
						"$(call {0},$({1}),$({2}),{3},{4})", cc_macro, cc, cc_options, obj_options.TrimWhiteSpace(), src_options.TrimWhiteSpace());

					MakeModule.EmptyRecipes.AddSuffixRecipy(fileItem.Path);
					MakeModule.AddFileToClean(makeTargetFile);
					MakeModule.AddFileToClean(MakeModule.ToMakeFilePath(Path.ChangeExtension(makeTargetFilePath, ".d")));
					
				}
			}

			string pchExtension = null;
			MakeTarget pchMakeTarget = null;
			if (!String.IsNullOrEmpty(pchMakeTargetFile))
			{
				string unquotedPath = pchMakeTargetFile.Trim(new char[] { '\"' });
				pchExtension = Path.GetExtension(unquotedPath);		// This function returns the dot character as well.
				MakeVariableScalar pchfile_makevar = ToolOptions.Variable("PCHFILE", pchMakeTargetFile);
				pchMakeTarget = MakeModule.MakeFile.Target(pchfile_makevar.Label.ToMakeValue());
			}

			List<string> objsVarData = ObjectFiles;
			if (!String.IsNullOrEmpty(pchMakeTargetFile) && ObjectFiles.Count > 0 && ObjectFiles[0] == pchMakeTargetFile)
			{
				// On PC, the vc compiler actually create create .obj as well as .pch file.  So we still need the "ObjectFiles"
				// to have the full list of obj for linking or for archiving.  But don't want that obj being listed in
				// the $(OBJS) list as we created a $(PCHFILE) variable for it.
				objsVarData = ObjectFiles.GetRange(1, ObjectFiles.Count - 1);
			}
			MakeVariableArray objs = null;
			if (objsVarData.Any())
			{
				objs = ToolOptions.VariableMulti("OBJS", objsVarData);
			}

			if (pchMakeTarget != null)
			{
				ToolOptions.LINE = "";
				ToolOptions.LINE = "ifneq ($(wildcard $(PCHFILE)),)";
				ToolOptions.LINE = "-include $(patsubst %" + pchExtension + ",%.d,$(wildcard $(PCHFILE)))";
				ToolOptions.LINE = "endif";
			}
			if (objs != null)
			{
				ToolOptions.LINE = "";
				ToolOptions.LINE = "ifneq ($(wildcard $(OBJS)),)";
				ToolOptions.LINE = "-include $(patsubst %.obj,%.d,$(wildcard $(OBJS)))";
				ToolOptions.LINE = "endif";
			}
			ToolOptions.LINE = "";

			//TODO : use "include $(sources:.c=.d)"

			CcTarget = null;
			if (objs != null)
			{
				CcTarget = MakeModule.MakeFile.Target(objs.Label.ToMakeValue());
			}
			if (pchMakeTarget != null)
			{
				if (CcTarget != null)
					CcTarget.Prerequisites += pchMakeTarget.Target;
				else
					CcTarget = pchMakeTarget;
			}
		}

		private string CompilerInvocationMacro()
		{
			var define = new MakeDefine(GetKeyedName("COMPILE_CC"));

			define.AddCommentLine("[{0}] Compiling file $<", Module.Name);
			define.AddLine("$(@D)".MkDir());

			define.AddLine(GetQuietSymbol() + "$(1) $(2) $(3) $(4)");

			RulesSection.Items.Add(define);

			return define.Label;
		}


		private string GetCompilerOptions(CcCompiler compiler, Dictionary<string, Tuple<string, string>> extractTokens)
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
					currentOptionValue.Append(op.Replace("\"" + foundTokenKey + "\"", tokenReplacement).Replace(foundTokenKey, tokenReplacement));

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

			if (Module.Configuration.Compiler == "gcc" || Module.Configuration.Compiler == "clang")
			{
				if (Module.Configuration.System == "osx" || Module.Configuration.System == "iphone" || Module.Configuration.System == "nx")
				{
					if (!compiler.Options.Contains("-MMD"))
					{
						options.Append(OptionsSeparator + "-MMD");
					}
				}
				else
				{
					if (!compiler.Options.Contains("-MD"))
					{
						options.Append(OptionsSeparator + "-MD");
					}
				}
			}

			return options.ToString();
		}


		protected void SetCompilerOptions(CcCompiler compiler, FileItem fileItem, string optionsetName, out string cc_options, out string obj_options, out string src_options, out PathString rspfile, bool usingPchFile = false)
		{
			string rawTemplate = null;
			if (usingPchFile)
			{
				// target file in question is actually a pch file isntead of obj.  Try getting the pch.commandline template first to see if it exists.
				rawTemplate = GetTemplate("pch.commandline", null, tool: compiler);
			}
			if (String.IsNullOrEmpty(rawTemplate))
			{
				rawTemplate = GetTemplate("commandline", DefaultCompilerCommandLineTemplate, tool: compiler);
			}
			var commandlineTemplate = new StringBuilder(rawTemplate);

			obj_options = "";
			if (usingPchFile && rawTemplate.Contains("%pchfile%"))
			{
				obj_options = ExtractTemplateFieldWithOption(commandlineTemplate, "%pchfile%").Replace("\"%pchfile%\"", "$@").Replace("%pchfile%", "$@").ToString().TrimWhiteSpace();
			}
			if (string.IsNullOrEmpty(obj_options))
			{
				obj_options = ExtractTemplateFieldWithOption(commandlineTemplate, "%objectfile%").Replace("\"%objectfile%\"", "$@").Replace("%objectfile%", "$@").ToString().TrimWhiteSpace();
			}
			src_options = ExtractTemplateFieldWithOption(commandlineTemplate, "%sourcefile%").Replace("\"%sourcefile%\"", "$<").Replace("%sourcefile%", "$<").ToString().TrimWhiteSpace();

			SortedSet<string> definesList = compiler.Defines;
			definesList.Add("EA_MAKE_BUILD=1");

			var defines = ReplaceTemplate(GetTemplate("define", tool: compiler), "%define%", definesList, MakeExtensions.QuoteOptionAsNeeded);
			// For includedirs, we would prefer specifying full path.  If -I "../../__Generated" where used and you have long include
			// path in code (#include <very ... long .. path .h>), some compiler is having compile error trying to reconstruct a full path
			// for the .h and ended up having path too long error (or header file not found) during compile.
			var includedirs = ReplaceTemplate(GetTemplate("includedir", tool: compiler), "%includedir%", compiler.IncludeDirs, pathTypeOverride: PathType.FullPath);
			var systemincludedirs = ReplaceTemplate(GetTemplate("system-includedir", tool: compiler), "%system-includedir%", compiler.SystemIncludeDirs, pathTypeOverride: PathType.FullPath);
			var usingdirs = ReplaceTemplate(GetTemplate("usingdir", tool: compiler), "%usingdir%", compiler.UsingDirs);
			var usingAssemblies = ReplaceTemplate(GetTemplate("forceusing-assembly", tool:compiler), "%forceusing-assembly%", compiler.Assemblies.FileItems.Select(fitm => fitm.FullPath));

			// If following special nant tokens are present in the option itself, they need to be converted to makefile tokens.  
			// However, if we turn on using response files, we cannot have any makefile tokens show up in response files. We need to strip
			// them from the standard build options list and handle the token stuff separately.
			Dictionary<string, Tuple<string, string>> extractTokens = new Dictionary<string, Tuple<string, string>>();
			extractTokens.Add(EA.CPlusPlusTasks.Dependency.DependencyFileVariable, new Tuple<string, string>("$(patsubst %.obj,%.d,$@)", null));
			extractTokens.Add("%objectfile%", new Tuple<string, string>("$@", null));
			extractTokens.Add("%sourcefile%", new Tuple<string, string>("$<", null));

			var ccOptions = GetCompilerOptions(compiler, extractTokens);

			if (UseAltSepInResponseFile)
				ccOptions = ccOptions.PathToUnix(); // Some compilers (like unixclang on windows) don't like its parameters passed with \ character instead of treating it as a path separator it treats it as a the start of a special character like \n

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
			commandlineTemplate.Replace("%system-includedirs%", systemincludedirs);
			commandlineTemplate.Replace("%usingdirs%", usingdirs);
			commandlineTemplate.Replace("%options%", ccOptions);
			commandlineTemplate.Replace("%forceusing-assemblies%", usingAssemblies);

			commandlineTemplate.Replace("%outputdir%", MakeModule.ToMakeFilePath(Module.IntermediateDir));
			commandlineTemplate.Replace("%outname%", Module.OutputName);

			rspfile = null;

			StringBuilder responsefileTemplate = null;
			if (UseResponseFile)
			{
				if (Module.Configuration.Compiler == "clang" && (Module.Configuration.System == "unix" || Module.Configuration.System == "unix64") && PlatformUtil.IsWindows)
				{
					// gross bug fix, clang on windows has a detection mechanism for whether it needs to use
					// a response file for calling sub process (clang calls ld for link and itself for compile)
					// however this doesn't seem to take into account the length of the actual executable
					// here we bloat our command line if it's anywhere near the limit to make sure it goes 
					// over

					const uint safetyRange = 4096;	// clang performs a bunch of transforms to command line (including adding a lot of defaults)
													// so command line length as passed to subprocess can vary dramatically - give ourselves
													// a very wide safety range to compensate for these shifts

					uint executableLength = (uint)compiler.Executable.Path.Length;
										
					uint badRangeLowerBound = PlatformUtil.MaxCommandLineLength - executableLength - safetyRange / 2;
					uint badRangeUpperBound = PlatformUtil.MaxCommandLineLength - executableLength + safetyRange / 2;

					if (commandlineTemplate.Length >= badRangeLowerBound && commandlineTemplate.Length < badRangeUpperBound)
					{
						// if we're in the danger zone add meaningless defines until we're safe
						while (commandlineTemplate.Length < badRangeUpperBound)
						{
							commandlineTemplate.Append(" -D _COMMAND_LINE_SAFETY_BLOAT_");
						}
					}
				}

				var responsefileTemplateStr = GetTemplate("responsefile", tool: compiler);
				if (responsefileTemplateStr != null)
				{
					responsefileTemplate = new StringBuilder(responsefileTemplateStr);

					responsefileTemplate.Replace("%defines%", defines);
					responsefileTemplate.Replace("%includedirs%", includedirs);
					responsefileTemplate.Replace("%system-includedirs%", systemincludedirs);
					responsefileTemplate.Replace("%usingdirs%", usingdirs);
					responsefileTemplate.Replace("%options%", ccOptions);
					responsefileTemplate.Replace("%forceusing-assemblies%", usingAssemblies);

					responsefileTemplate.Replace("%outputdir%", MakeModule.ToMakeFilePath(Module.IntermediateDir));
					responsefileTemplate.Replace("%outname%", Module.OutputName);
				}

				rspfile = PathString.MakeCombinedAndNormalized(Module.IntermediateDir.Path, "cc_options_" + (String.IsNullOrEmpty(optionsetName) ? Module.BuildType.Name : optionsetName) + ".rsp");

				commandlineTemplate = GetResponseFileCommandLine(rspfile, commandlineTemplate, responsefileTemplate);
			}

			cc_options = GetToolVariableName("OPTIONS" + (String.IsNullOrEmpty(optionsetName) ? "" : "_" + optionsetName));
			if (_optionsVars.Add(cc_options))
			{
				cc_options = SetToolVariable(ToolOptions, "OPTIONS" + (String.IsNullOrEmpty(optionsetName) ? "" : "_" + optionsetName), commandlineTemplate.ToString().LinesToArray());
			}

		}

		private string ObjectFilePath(FileItem fileItem)
		{
			string objPath = "";

			EA.Eaconfig.Core.FileData filedata = fileItem.Data as EA.Eaconfig.Core.FileData;

			if (filedata != null && filedata.ObjectFile != null && !String.IsNullOrEmpty(filedata.ObjectFile.Path))
			{
				objPath = filedata.ObjectFile.Path;
			}
			else
			{
				objPath = CompilerBase.GetOutputBasePath(fileItem, Module.IntermediateDir.Path, fileItem.BaseDirectory ?? Compiler.SourceFiles.BaseDirectory)  + GetObjectFileExtension(fileItem);
			}
			return objPath;
		}

		protected string ObjectFileExtensionProperty { get { return "cc.objfile.extension"; } }

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
				var comp = tool as CcCompiler;

				if (comp != null)
				{
					propertyValue = comp.Templates[propertyName];
				}
				
			}

			if (propertyValue == null)
			{
				if (Compiler.Templates.TryGetValue(propertyName, out propertyValue))
				{
					return propertyValue;
				}
			}

			return propertyValue;
		}

		private CcCompiler GetCompiler(FileItem fileItem, out string optionsetName)
		{
			CcCompiler compiler = null;

			optionsetName = String.Empty;

			if (fileItem != null && fileItem.OptionSetName != null)
			{
				compiler = fileItem.GetTool()  as CcCompiler;
				if (compiler != null)
				{
					optionsetName = fileItem.OptionSetName;
				}
			}
			return compiler ?? Compiler;
		}

	}
}
