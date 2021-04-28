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
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using System.Text.RegularExpressions;

namespace EA.Eaconfig.Build.MergeModules
{
	internal class MergeModules_Native : MergeModules_Base<Module_Native>
	{
		internal MergeModules_Native(MergeModules_Data data, Project project, IEnumerable<IModule> buildmodules)
			: base(data, project, buildmodules)
		{
		}

		internal static void Merge(Project project, IEnumerable<IModule> buildmodules)
		{
			foreach (var data in MergeModules_Invoker.GetMergeData(buildmodules.Where(m => m.IsKindOf(Module.Native) && (m is Module_Native) && !m.IsKindOf(Module.Program)), MakeMergeName, MakeMergeDataKey))
			{
				new MergeModules_Native(data, project, buildmodules).Merge();
			}
		}

		private static string MakeMergeDataKey(string mergemodulename, BuildGroups mergemodulegroup, IModule srcmod)
		{
			return MergeModules_Data.MakeKey(mergemodulename, mergemodulegroup, srcmod.Configuration);
		}

		private static string MakeMergeName(string mergemodulename, IModule srcmod)
		{
			return mergemodulename;
		}

		protected override void MergeSpecificModuleType(Module_Native sourceModule)
		{
			MergeTool(sourceModule, sourceModule.Cc);
			MergeTool(sourceModule, sourceModule.Asm);
			MergeTool(sourceModule, sourceModule.Lib);
			MergeTool(sourceModule, sourceModule.Link);

			AdjustLibraryReferences(sourceModule.LibraryOutput(), TargetModule.LibraryOutput());
		}

		protected override bool InitMerge()
		{
			return true;
		}

		protected override bool FinalizeMerge()
		{
			return true;
		}

		private void MergeTool(ProcessableModule srcmod, CcCompiler compiler)
		{
			if (compiler != null)
			{
				if (TargetModule.Cc == null)
				{
					TargetModule.Cc = CloneCompiler(compiler);

					TargetModule.Cc.Options.AddUnique(TranslateOptions(srcmod, compiler));
				}

				foreach (var file in compiler.SourceFiles.FileItems)
				{
					TargetModule.Cc.SourceFiles.Include(file);

					FileData filedata = file.Data as FileData;
					if (filedata.Tool == null)
					{
						filedata.Tool = TargetModule.Cc;
						file.OptionSetName = srcmod.BuildType.Name;
					}
				}

				TargetModule.Cc.IncludeDirs.AddUnique(compiler.IncludeDirs);
				TargetModule.Cc.Defines.AddUnique(compiler.Defines);

				// Merge Warning Suppression Flags
				// the merged module will need to suppress all of the same warnings as the original packages to compile
				List<string> warning_suppressions = new List<string>();
				Regex warning_regex = new Regex(@"[/-]wd\d+");
				foreach (string option in compiler.Options)
				{
					if (warning_regex.Match(option).Success)
					{
						warning_suppressions.Add(option);
					}
				}
				TargetModule.Cc.Options.AddUnique(warning_suppressions);
			}
		}

		private CcCompiler CloneCompiler(CcCompiler compiler)
		{
			var clone = new CcCompiler(compiler.Executable, compiler.PrecompiledHeaders, compiler.WorkingDir);

			return clone;
		}

		private List<string> TranslateOptions(ProcessableModule srcmod, CcCompiler compiler)
		{
			if (compiler == null)
			{
				return null;
			}

			var resultoptions = new List<string>();

			string src_output_path = Path.Combine(srcmod.OutputDir.Path, srcmod.OutputName);
			string target_output_path = Path.Combine(TargetModule.OutputDir.Path, TargetModule.OutputName);

			foreach (string option in compiler.Options)
			{
				resultoptions.Add(option.Replace(src_output_path, target_output_path));
			}

			return resultoptions;
		}

		private void MergeTool(ProcessableModule srcmod, AsmCompiler compiler)
		{
			if (compiler != null)
			{
				if (TargetModule.Asm == null)
				{
					TargetModule.Asm = compiler;
				}
				else
				{
					foreach (var file in compiler.SourceFiles.FileItems)
					{
						TargetModule.Asm.SourceFiles.Include(file);

						FileData filedata = file.Data as FileData;
						if (filedata.Tool == null)
						{
							filedata.Tool = compiler;
							file.OptionSetName = srcmod.BuildType.Name;
						}
					}
				}
			}
		}

		private void MergeTool(ProcessableModule srcmod, Librarian lib)
		{
			if (lib != null)
			{
				if (TargetModule.Lib == null)
				{
					TargetModule.Lib = CloneLib(lib);

					ModulePath mergedModulePath = ModulePath.Private.GetModuleLibPath(TargetModule);

					TargetModule.Lib.OutputName = mergedModulePath.OutputName;
					TargetModule.Lib.OutputExtension = mergedModulePath.Extension;
				}
				TargetModule.Lib.Options.AddUnique(TranslateOptions(srcmod, lib));
				TargetModule.Lib.ObjectFiles.Include(lib.ObjectFiles);
			}				
		}

		private Librarian CloneLib(Librarian lib)
		{
			return new Librarian(lib.Executable, lib.WorkingDir);
		}

		private List<string> TranslateOptions(ProcessableModule srcmod, Librarian lib)
		{
			if (lib == null)
			{
				return null;
			}

			var resultoptions = new List<string>();

			var srcliboutput = srcmod.LibraryOutput();
			var targetliboutput = TargetModule.LibraryOutput();

			foreach (var option in lib.Options)
			{
				var translated = option
						.Replace(srcmod.IntermediateDir.Path, TargetModule.IntermediateDir.Path)
						.Replace(srcmod.OutputDir.Path, TargetModule.OutputDir.Path)
						.Replace(lib.OutputName, TargetModule.Lib.OutputName);

				resultoptions.Add(translated);
			}

			return resultoptions;
		}

		private void MergeTool(ProcessableModule srcmod, Linker link)
		{
			if (link != null)
			{
				if (TargetModule.Link == null)
				{
					TargetModule.Link = CloneLink(link);

					ModulePath mergedModulePath = ModulePath.Private.GetModuleBinPath(Project, TargetModule.Package.Name, TargetModule.Name, TargetModule.BuildGroup.ToString(), TargetModule.BuildType);

					TargetModule.Link.OutputName = mergedModulePath.OutputName;
					TargetModule.Link.OutputExtension = mergedModulePath.Extension;
					TargetModule.Link.LinkOutputDir = PathString.MakeNormalized(mergedModulePath.OutputDir);

					ModulePath importLibPath = ModulePath.Private.GetModuleLibPath(TargetModule);
					TargetModule.Link.ImportLibOutputDir = PathString.MakeNormalized(importLibPath.OutputDir);
					if (link.ImportLibFullPath != null)
					{
						TargetModule.Link.ImportLibFullPath = PathString.MakeNormalized(Path.Combine(importLibPath.OutputDir, importLibPath.OutputName + importLibPath.Extension));
						
					}
				}				
	
				TargetModule.Link.DynamicLibraries.Include(link.DynamicLibraries);
				TargetModule.Link.Libraries.Include(link.Libraries);				
				TargetModule.Link.Libraries.Exclude(link.ImportLibFullPath.Path);

				TargetModule.Link.LibraryDirs.AddUnique(link.LibraryDirs);
				TargetModule.Link.ObjectFiles.Include(link.ObjectFiles);
				TargetModule.Link.Options.AddUnique(TranslateOptions(srcmod, link));
			}					
		}

		private Linker CloneLink(Linker link)
		{
			var clone = new Linker(link.Executable, link.WorkingDir);

			return clone;
		}

		private List<string> TranslateOptions(ProcessableModule srcmod, Linker link)
		{
			if (link == null)
			{
				return null;
			}

			var resultoptions = new List<string>();

			var srcbinoutput = Path.Combine(link.LinkOutputDir.Path, link.OutputName);
			var targetbinoutput = Path.Combine(TargetModule.Link.LinkOutputDir.Path, TargetModule.Link.OutputName);

			var srcliboutput = "@@@@@@@@@@@@@@@@@@@@@@";
			var targetliboutput = "@@@@@@@@@@@@@@@@@@@@@@";

			if (!TargetModule.Link.ImportLibFullPath.IsNullOrEmpty() && !link.ImportLibFullPath.IsNullOrEmpty())
			{
				srcliboutput = link.ImportLibFullPath.Path;
				targetliboutput = TargetModule.Link.ImportLibFullPath.Path;
			}

			var srcIntermediate = Path.Combine(srcmod.IntermediateDir.Path, link.OutputName);
			var dstIntermediate = Path.Combine(TargetModule.IntermediateDir.Path, TargetModule.Link.OutputName);

			foreach (var option in link.Options)
			{
				var translated = option
					.Replace(srcbinoutput, targetbinoutput)
					.Replace(srcliboutput, targetliboutput)
					.Replace(srcIntermediate, dstIntermediate);

				resultoptions.Add(translated);
			}

			return resultoptions;
		}
	}
}