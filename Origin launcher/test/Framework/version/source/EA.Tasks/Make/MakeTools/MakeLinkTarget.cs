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
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

using EA.Make.MakeItems;



namespace EA.Make.MakeTools
{

	class MakeLinkTarget : MakeBuildToolBase<Linker>
	{
		public const string DefaultLinkerCommandLineTemplate = "%options% %librarydirs% %objectfiles% %libraryfiles%";

		private readonly Linker Linker;
		public readonly MakeTarget LinkTarget;

		private readonly string OutputFile;
		private readonly string OutputFileVar = "OUTPUTFILE";


		public MakeLinkTarget(MakeModuleBase makeModule, Module_Native module)
			: base(makeModule, module, module.Link)
		{
			Linker = module.Link;

			OutputFile = MakeModule.ToMakeFilePath(PathString.MakeCombinedAndNormalized(Linker.LinkOutputDir.Path, Linker.OutputName + Linker.OutputExtension));

			LinkTarget = new MakeTarget(GetToolVariableName(OutputFileVar).ToMakeValue());
		}

		public void GenerateTargets(MakeCompileTarget compile, MakeCompileAsmTarget asmCompile)
		{

			var link = SetToolExecutableVariable();

			SetToolVariable(ToolPrograms, OutputFileVar, OutputFile);


			StringBuilder responsefilecommand;
			var commandline = GetCommandLine(compile, asmCompile, out responsefilecommand);

			if (UseResponseFile)
			{
				var rspfile = PathString.MakeCombinedAndNormalized(Module.IntermediateDir.Path, Linker.OutputName + Linker.OutputExtension + ".rsp");
				commandline = GetResponseFileCommandLine(rspfile, commandline, responsefilecommand);

				LinkTarget.Prerequisites += MakeModule.ToMakeFilePath(rspfile);
			}
			else
			{
				// Store linker options in a variable:
				var link_options = SetToolVariable(ToolOptions, "OPTIONS", commandline.ToString().LinesToArray());

				commandline = new StringBuilder(link_options.ToMakeValue());
			}

			// Add definitions for libs and other prerequisites:
			var link_libraries = SetToolVariable(ToolOptions, "LIBS", Linker.Libraries);

			LinkTarget.Prerequisites += "MAKEFILE".ToMakeValue();
			LinkTarget.Prerequisites += link_libraries.ToMakeValue();
			LinkTarget.Prerequisites += Linker.InputDependencies.FileItems.Select(fi => MakeModule.ToMakeFilePath(fi.Path));

			LinkTarget.AddRuleCommentLine("[{0}] Linking File $@ $@", Module.Name);
			LinkTarget.AddRuleLine("$(@D)".MkDir());
			if (!Linker.ImportLibFullPath.IsNullOrEmpty())
			{
				AddCreateDirectoryRule(LinkTarget, Path.GetDirectoryName(Linker.ImportLibFullPath.Path));
			}
			LinkTarget.AddRuleLine(GetQuietSymbol() + "$({0}) {1}", link, commandline.ToString());

			LinkTarget.AddRuleLine("$(eval BUILD_EXECUTED=true)");

			MakeModule.MakeFile.Items.Add(LinkTarget);

			MakeModule.EmptyRecipes.AddSuffixRecipy(Linker.InputDependencies.FileItems.Select(fi => fi.Path));
			MakeModule.EmptyRecipes.AddSuffixRecipy(Linker.ObjectFiles.FileItems.Select(fi => fi.Path));
			MakeModule.EmptyRecipes.AddSuffixRecipy(Linker.Libraries.FileItems.Select(fi => fi.Path));
			MakeModule.AddFileToClean(OutputFile);
			MakeModule.AddFileToClean(Linker.ImportLibFullPath);
		}

		private StringBuilder GetCommandLine(MakeCompileTarget compile, MakeCompileAsmTarget asmCompile, out StringBuilder responsefileTemplate)
		{
			// Get the template that will be used to assembly the parts of the command line
			var commandlineTemplate = new StringBuilder(GetTemplate("commandline", DefaultLinkerCommandLineTemplate, tool:Linker));

			var separator = UseResponseFile ? GetTemplate("responsefile.separator", " ", tool:Linker) : " ";

			if (compile.CcTarget != null)
			{
				LinkTarget.Prerequisites += compile.CcTarget.Target;
			}
			if (asmCompile.AsmTarget != null)
			{
				LinkTarget.Prerequisites += asmCompile.AsmTarget.Target;
			}


			var objs = String.Empty;
			if (Linker.ObjectFiles.FileItems.Count > 0)
			{
				var objectfiles = ReplaceTemplate(GetTemplate("objectfile", tool:Linker), "%objectfile%", Linker.ObjectFiles.FileItems.Select(fi => fi.Path.Path));

				objs = SetToolVariable(ToolOptions, "OBJS", objectfiles.LinesToArray()).ToMakeValue();
				LinkTarget.Prerequisites += objs;
			}

			string allobjectfiles = null;
			if (UseResponseFile)
			{
				allobjectfiles = ReplaceTemplate(GetTemplate("objectfile", tool:Linker), "%objectfile%", Linker.ObjectFiles.FileItems.Select(fi => fi.Path.Path).Union(compile.ObjectFiles).Union(asmCompile.ObjectFiles));
			}

			var libdirs = ReplaceTemplate(GetTemplate("librarydir", tool:Linker), "%librarydir%", Linker.LibraryDirs);
			var libfiles = ReplaceTemplate(GetTemplate("libraryfile", tool:Linker), "%libraryfile%", Linker.Libraries.FileItems.Select(fi=>MakeModule.ToMakeFilePath(fi.Path)).Distinct());

			string linkerOptions = Linker.Options.ToString(OptionsSeparator); 

			if (UseAltSepInResponseFile)
				linkerOptions = linkerOptions.PathToUnix(); // Some compilers (like unixclang on windows) don't like its parameters passed with \ character instead of treating it as a path separator it treats it as a the start of a special character like \n

			commandlineTemplate.Replace("%options%", linkerOptions); 
			commandlineTemplate.Replace("%librarydirs%", libdirs);
			commandlineTemplate.Replace("%libraryfiles%", libfiles);

			string obj_files_vars = objs;
			if (compile.CcTarget != null)
			{
				obj_files_vars += OptionsSeparator + compile.CcTarget.Target;
			}

			if (asmCompile.AsmTarget != null)
			{
				obj_files_vars += OptionsSeparator + asmCompile.AsmTarget.Target;
			}
			obj_files_vars += OptionsSeparator;

			commandlineTemplate.Replace("%objectfiles%", UseResponseFile ? allobjectfiles : obj_files_vars);
			commandlineTemplate.Replace("%outputdir%", MakeModule.ToMakeFilePath(Module.IntermediateDir));
			commandlineTemplate.Replace("%outname%", Module.OutputName);

			responsefileTemplate = null;
			if (UseResponseFile)
			{
				var responsefileTemplateStr = GetTemplate("responsefile", tool:Linker);
				if (responsefileTemplateStr != null)
				{
					responsefileTemplate = new StringBuilder(responsefileTemplateStr);

					responsefileTemplate.Replace("%options%", linkerOptions);
					responsefileTemplate.Replace("%librarydirs%", libdirs);
					responsefileTemplate.Replace("%libraryfiles%", libfiles);
					responsefileTemplate.Replace("%objectfiles%", allobjectfiles);

					responsefileTemplate.Replace("%outputdir%", MakeModule.ToMakeFilePath(Module.IntermediateDir));
					responsefileTemplate.Replace("%outname%", Module.OutputName);
				}
			}

			return commandlineTemplate;
		}
	}
}
