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
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Build.MergeModules
{
	internal class MergeModules_DotNet : MergeModules_Base<Module_DotNet>
	{

		private List<string> _merged_options;

		private List<PathString> _excluded_AssemblyInfo = new List<PathString>();

		internal MergeModules_DotNet(MergeModules_Data data, Project project, IEnumerable<IModule> buildmodules)
			: base(data, project, buildmodules)
		{
			_merged_options = new List<string>();
		}

		internal static void Merge(Project project, IEnumerable<IModule> buildmodules)
		{
			foreach (var data in MergeModules_Invoker.GetMergeData(buildmodules.Where(m => m.IsKindOf(Module.DotNet) && (m is Module_DotNet) && (m as Module_DotNet).Compiler.Target == DotNetCompiler.Targets.Library), MakeMergeName, MakeMergeDataKey))
			{
				new MergeModules_DotNet(data, project, buildmodules).Merge();
			}
		}

		private static string MakeMergeDataKey(string mergemodulename, BuildGroups mergemodulegroup, IModule srcmod)
		{
			return MergeModules_Data.MakeKey(mergemodulename, mergemodulegroup, srcmod.Configuration) +":" + (srcmod as Module_DotNet).Compiler.TargetPlatform;
		}

		private static string MakeMergeName(string mergemodulename, IModule srcmod)
		{
			return mergemodulename + "_" + (srcmod as Module_DotNet).Compiler.TargetPlatform;
		}

		protected override void MergeSpecificModuleType(Module_DotNet srcmod)
		{
			if (TargetModule.Compiler == null)
			{
				TargetModule.Compiler = CreateCompiler(srcmod);
			}

			MergeModule(srcmod);

			MergeCompiler(TargetModule.Compiler, srcmod);

			TargetModule.Compiler.OutputName = GetOutputName();
			TargetModule.OutputDir = GetBinOutputDir(TargetModule);

		}

		protected override bool InitMerge()
		{
			return true;
		}

		protected override bool FinalizeMerge()
		{
			TargetModule.Compiler.Options.AddRange(_merged_options.OrderedDistinct());

			WriteMergeInfo();

			return true;
		}

		
		private string GetOutputName()
		{
			return _merge_data.TargetModule.Name;
		}

		private PathString GetBinOutputDir(IModule module)
		{
			var    outputdir = PathString.MakeNormalized(Path.Combine(
													  module.Package.Project.Properties["package.configbindir"],
													  module.Package.Project.Properties["eaconfig." + module.BuildGroup + ".outputfolder"].TrimLeftSlash()));
			return outputdir;
		}

		private DotNetCompiler CreateCompiler(Module_DotNet srcmod)
		{
			DotNetCompiler compiler = null;

			if (srcmod.IsKindOf(Module.CSharp))
			{
				compiler = new CscCompiler(srcmod.Compiler.Executable, srcmod.Compiler.Target);
			}
			else if (srcmod.IsKindOf(Module.FSharp))
			{
				compiler = new FscCompiler(srcmod.Compiler.Executable, srcmod.Compiler.Target);
			}
			else
			{
				throw new BuildException($"Module {srcmod.ModuleIdentityString()} is not C# or F# type. MergeMOdules_DotNet class can't handle other types.");
			}
			return compiler;
		}

		private void MergeCompiler(DotNetCompiler compiler, Module_DotNet srcmod)
		{

			if (srcmod.IsKindOf(Module.CSharp))
			{
				var csc = compiler as CscCompiler;
				var src_csc = srcmod.Compiler as CscCompiler;

				if (csc != null && src_csc != null)
				{
					//TODO: How do we merge these?
					csc.Win32icon = src_csc.Win32icon;
					csc.Win32manifest = src_csc.Win32manifest;
				}
			}

			MergeSettings(ref compiler.GenerateDocs, srcmod.Compiler.GenerateDocs);
			
			compiler.OutputExtension = srcmod.Compiler.OutputExtension;

			compiler.Defines.AddRange(srcmod.Compiler.Defines);


			MergeFiles(compiler.SourceFiles, srcmod.Compiler.SourceFiles);
			MergeFiles(compiler.Assemblies, srcmod.Compiler.Assemblies);
			MergeFiles(compiler.DependentAssemblies, srcmod.Compiler.DependentAssemblies);
			MergeFiles(compiler.Modules, srcmod.Compiler.Modules);

			MergeFiles(compiler.ContentFiles, srcmod.Compiler.ContentFiles);
			MergeFiles(compiler.NonEmbeddedResources, srcmod.Compiler.NonEmbeddedResources);

			MergeFiles(compiler.Resources, srcmod.Compiler.Resources);

			FilterAssemblyInfoFiles(compiler, srcmod);

			//TODO: How do we merge these?

			/*
			if(    !String.IsNullOrEmpty(compiler.Resources.BaseDirectory)
				&& !String.IsNullOrEmpty(srcmod.Compiler.Resources.BaseDirectory)
				&& !String.Equals(compiler.Resources.BaseDirectory, srcmod.Compiler.Resources.BaseDirectory))
			{
				Log.Status.WriteLine("Merged module {0} [{1}]: Source modules contain conflicting Resources.BaseDirectory values {2} and {3}.", TargetModule.Name, TargetModule.Configuration.Name, compiler.Resources.BaseDirectory, srcmod.Compiler.Resources.BaseDirectory);
			}

			if(!String.IsNullOrEmpty(srcmod.Compiler.Resources.BaseDirectory))
			{
				compiler.Resources.BaseDirectory = srcmod.Compiler.Resources.BaseDirectory;
			}
			 */

			if (srcmod.Compiler.Resources.Prefix != srcmod.Name)
			{
				if (!String.IsNullOrEmpty(compiler.Resources.Prefix)
					&& !String.IsNullOrEmpty(srcmod.Compiler.Resources.Prefix)
					&& !String.Equals(compiler.Resources.Prefix, srcmod.Compiler.Resources.Prefix))
				{
					Log.Status.WriteLine("Merged module {0} [{1}]: Source modules contain conflicting Resources.Prefix values {2} and {3}.", TargetModule.Name, TargetModule.Configuration.Name, compiler.Resources.Prefix, srcmod.Compiler.Resources.Prefix);
				}
				if (!String.IsNullOrEmpty(srcmod.Compiler.Resources.Prefix))
				{
					compiler.Resources.Prefix = srcmod.Compiler.Resources.Prefix;
				}
			}

			MergeArray(ref compiler.AdditionalArguments, srcmod.Compiler.AdditionalArguments, (s) => s.ToArray(), (a) => a.ToString(" "));

			// Merge options:
			_merged_options.AddRange(srcmod.Compiler.Options);
		}

		private void MergeModule(Module_DotNet srcmod)
		{
			//TODO: How do we merge these
			//MergeSettings(ref TargetModule.TargetFrameworkVersion, srcmod.TargetFrameworkVersion);
			MergeSettings(ref TargetModule.RootNamespace, srcmod.RootNamespace);

			 MergeSettings(ref TargetModule.ApplicationManifest, srcmod.ApplicationManifest);
			 MergeSettings(ref TargetModule.AppDesignerFolder, srcmod.AppDesignerFolder);
			 MergeSettings(ref TargetModule.DisableVSHosting, srcmod.DisableVSHosting);
			 MergeSettings(ref TargetModule.WebReferences, srcmod.WebReferences);

			// TODO: warn on conflict
			 MergeSettings(ref TargetModule.RunPostBuildEventCondition, srcmod.RunPostBuildEventCondition);

			 MergeArray(ref TargetModule.ImportMSBuildProjects, srcmod.ImportMSBuildProjects, (s) => s.LinesToArray(), (a) => a.ToString(Environment.NewLine));


			TargetModule.ProjectTypes |= srcmod.ProjectTypes;


			// Merging enums

			// DotNetGenerateSerializationAssembliesTypes { None, Auto, On, Off };
			if(srcmod.GenerateSerializationAssemblies != DotNetGenerateSerializationAssembliesTypes.None)
			{
				if(TargetModule.GenerateSerializationAssemblies ==  DotNetGenerateSerializationAssembliesTypes.None)
				{
					TargetModule.GenerateSerializationAssemblies = srcmod.GenerateSerializationAssemblies;
				}
				else if(srcmod.GenerateSerializationAssemblies ==  DotNetGenerateSerializationAssembliesTypes.On)
				{
					TargetModule.GenerateSerializationAssemblies = srcmod.GenerateSerializationAssemblies;
				}
				else if(srcmod.GenerateSerializationAssemblies ==  DotNetGenerateSerializationAssembliesTypes.Auto && TargetModule.GenerateSerializationAssemblies !=  DotNetGenerateSerializationAssembliesTypes.On)
				{
					TargetModule.GenerateSerializationAssemblies = srcmod.GenerateSerializationAssemblies;
				}
			}

			// CopyLocalType { True, False, Slim, Undefined }
			if (srcmod.CopyLocal != CopyLocalType.Undefined)
			{
				if (TargetModule.CopyLocal == CopyLocalType.Undefined)
				{
					TargetModule.SetCopyLocal(srcmod.CopyLocal);
				}
				else if (TargetModule.CopyLocal == CopyLocalType.True)
				{
					TargetModule.SetCopyLocal(srcmod.CopyLocal);
				}
				else if (srcmod.CopyLocal == CopyLocalType.Slim && TargetModule.CopyLocal != CopyLocalType.True)
				{
					TargetModule.SetCopyLocal(srcmod.CopyLocal);
				}
			}
		}

		private void FilterAssemblyInfoFiles(DotNetCompiler compiler, Module_DotNet srcmod)
		{
			if(srcmod.Package.Project.Properties.GetBooleanPropertyOrDefault(srcmod.PropGroupName("merge-exclude-assembly-info"),
										srcmod.Package.Project.Properties.GetBooleanPropertyOrDefault("merge-exclude-assembly-info", false)))
			{
				foreach(var file in srcmod.Compiler.SourceFiles.FileItems)
				{
					if(file.Path.Path.EndsWith("AssemblyInfo.cs", StringComparison.OrdinalIgnoreCase))
					{
						compiler.SourceFiles.Exclude(file.Path.Path);
						_excluded_AssemblyInfo.Add(file.Path);
					}
				}
			}
		}

		private void WriteMergeInfo()
		{
			if(_excluded_AssemblyInfo.Count > 0)
			{
				Log.Status.WriteLine("Merged module {0} [{1}]: excluding AssemblyInfo files:{2}" + Environment.NewLine, TargetModule.Name, TargetModule.Configuration.Name, _excluded_AssemblyInfo.ToString(Environment.NewLine, p => "           " + p.Path.Quote()));
				
			}


		}

	}
}
