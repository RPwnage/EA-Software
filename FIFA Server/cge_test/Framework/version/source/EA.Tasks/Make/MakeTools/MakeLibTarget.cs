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
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

using EA.Make.MakeItems;


namespace EA.Make.MakeTools
{
	class MakeLibTarget : MakeBuildToolBase<Librarian>
	{
		private static readonly  string OPTIONS_RSP_SEPARATOR = ("\\" + Environment.NewLine);

		private readonly Librarian Lib;
		public readonly MakeTarget LibTarget;
		private readonly string OutputFile;
		private readonly string OutputFileVar="OUTPUTFILE";


		public MakeLibTarget(MakeModuleBase makeModule, Module_Native module)
			: base(makeModule, module, module.Lib)
		{
			Lib = module.Lib;

			OutputFile = MakeModule.ToMakeFilePath(PathString.MakeCombinedAndNormalized(Module.OutputDir.Path, Lib.OutputName + Lib.OutputExtension));

			LibTarget = new MakeTarget(GetToolVariableName(OutputFileVar).ToMakeValue());
			
		}


		public void GenerateTargets(MakeCompileTarget compile, MakeCompileAsmTarget asmCompile)
		{
			var lib =  SetToolExecutableVariable();

			SetToolVariable(ToolPrograms, OutputFileVar, OutputFile);

			var responsefilecommand = new StringBuilder();
			var commandline = GetCommandLine(compile, asmCompile, out responsefilecommand );

			if (UseResponseFile)
			{
				var rspfile = PathString.MakeCombinedAndNormalized(Module.IntermediateDir.Path, Lib.OutputName + Lib.OutputExtension + ".rsp");
				commandline = GetResponseFileCommandLine(rspfile, commandline, responsefilecommand);

				LibTarget.Prerequisites += MakeModule.ToMakeFilePath(rspfile);
			}
			else
			{
				// Store linker options in a variable:
				var lib_options = SetToolVariable(ToolOptions, "OPTIONS", commandline.ToString().LinesToArray());

				commandline = new StringBuilder(lib_options.ToMakeValue());
			}

			LibTarget.Prerequisites += "MAKEFILE".ToMakeValue();
			LibTarget.Prerequisites += Lib.InputDependencies.FileItems.Select(fi => MakeModule.ToMakeFilePath(fi.Path));

			LibTarget.AddRuleCommentLine("[{0}] Creating Library $@", Module.Name);
			LibTarget.AddRuleLine("$(@D)".MkDir());

			// Delete the old library file first.  On linux machine (or on Windows using the Linux cross tools), the
			// archiver 'ar' program won't re-create the .a lib file.  If the file already exists, it just insert or
			// replace old entries.  We run into situation where the resulting .a lib file contain old data that is
			// out of date causing build error that took over a couple of days to track down.  So we want to make sure
			// the old .a lib file is deleted first before we create a new one.
			LibTarget.AddRuleLine("$@".DelFile());

			// Build the library using the library tool.
			LibTarget.AddRuleLine(GetQuietSymbol() + "$({0}) {1}", lib, commandline.ToString());

			LibTarget.AddRuleLine("$(eval BUILD_EXECUTED=true)");

			MakeModule.MakeFile.Items.Add(LibTarget);

			MakeModule.EmptyRecipes.AddSuffixRecipy(Lib.InputDependencies.FileItems.Select(fi => fi.Path));
			MakeModule.EmptyRecipes.AddSuffixRecipy(Lib.ObjectFiles.FileItems.Select(fi => fi.Path));
			MakeModule.AddFileToClean(OutputFile);

		}

		private StringBuilder GetCommandLine(MakeCompileTarget compile, MakeCompileAsmTarget asmCompile, out StringBuilder responsefileTemplate)
		{
			var commandlineTemplate = new StringBuilder(GetTemplate("commandline", "%options% %objectfiles%", Lib));

			var separator = UseResponseFile ? GetTemplate("responsefile.separator", " ", Lib) : " ";

			if (compile.CcTarget != null)
			{
				LibTarget.Prerequisites += compile.CcTarget.Target;
			}
			if(asmCompile.AsmTarget != null)
			{
				LibTarget.Prerequisites += asmCompile.AsmTarget.Target;
			}

			var objs = String.Empty;
			if (Lib.ObjectFiles.FileItems.Count > 0)
			{
				var objectfiles = ReplaceTemplate(GetTemplate("objectfile", tool:Lib), "%objectfile%", Lib.ObjectFiles.FileItems.Select(fi => fi.Path.Path));

				objs = SetToolVariable(ToolOptions, "OBJS", objectfiles.LinesToArray()).ToMakeValue();
				LibTarget.Prerequisites += objs;
			}

			string allobjectfiles = null;
			if (UseResponseFile)
			{
				allobjectfiles = ReplaceTemplate(GetTemplate("objectfile", tool: Lib), "%objectfile%", Lib.ObjectFiles.FileItems.Select(fi => fi.Path.Path).Union(compile.ObjectFiles).Union(asmCompile.ObjectFiles));
			}

			string obj_files_vars = objs;
			if (compile.CcTarget != null)
			{
				obj_files_vars += OptionsSeparator + compile.CcTarget.Target;
			}

			if (asmCompile.AsmTarget != null)
			{
				obj_files_vars += OptionsSeparator + asmCompile.AsmTarget.Target;
			}

			string libOptions = Lib.Options.ToString(OptionsSeparator);

			if (UseAltSepInResponseFile)
				libOptions = libOptions.PathToUnix(); // Some compilers (like unixclang on windows) don't like its parameters passed with \ character instead of treating it as a path separator it treats it as a the start of a special character like \n

			commandlineTemplate.Replace("%options%", libOptions);  
			commandlineTemplate.Replace("%objectfiles%", UseResponseFile ? allobjectfiles : obj_files_vars);
			commandlineTemplate.Replace("%outputdir%", MakeModule.ToMakeFilePath(Module.OutputDir));
			commandlineTemplate.Replace("%outputname%", Lib.OutputName);

			responsefileTemplate = null;
			if (UseResponseFile)
			{
				var responsefileTemplateStr = GetTemplate("responsefile", tool: Lib);
				if (responsefileTemplateStr != null)
				{
					responsefileTemplate = new StringBuilder(responsefileTemplateStr);
					responsefileTemplate.Replace("%options%", libOptions);
					responsefileTemplate.Replace("%objectfiles%", allobjectfiles);
					responsefileTemplate.Replace("%outputdir%", MakeModule.ToMakeFilePath(Module.OutputDir));
					responsefileTemplate.Replace("%outputname%", Lib.OutputName);
				}
			}
			return commandlineTemplate;
		}
	}
}
