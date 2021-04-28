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
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;
using System;

namespace EA.Eaconfig.Build
{
    public class MapDependentBuildOutputs
	{
		public static string RemapExtension(IModule module, string filename)
		{
			if (module is Module_DotNet)
			{
				Module_DotNet dnModule = module as Module_DotNet;
				if (dnModule.TargetFrameworkFamily == DotNetFrameworkFamilies.Core && (filename.EndsWith(".exe")))
				{
					// A .Net Core build even with exe target actually creates a .dll assembly output.  During MSBuild
					// a separate CreateAppHost msbuild target will combine this .dll with apphost.exe from SDK to
					// create the final .exe.  Also, you cannot run this .exe without the .dll, but you can run the
					// .dll build output using "dotnet [x].dll" without the .exe, so for now, if it is a .Net Core
					// build, we set the extension back to .dll.
					return filename.Replace(".exe", ".dll");
				}
			}
			return filename;
		}
		
		public static FileSet RemapModuleBinaryPaths(ProcessableModule module, ProcessableModule parentModule, FileSet binaryFiles, string defaultSuffix)
		{
			if (binaryFiles == null)
			{
				return binaryFiles;
			}

			if (!(module is Module_UseDependency))
			{
				// get public pre and private post mapped path
				ModulePath preMappingPath = ModulePath.Public.GetModuleBinPath(module.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), defaultSuffix, module.BuildType, ignoreOutputMapping: true);
				ModulePath postMappingPath = ModulePath.Private.GetModuleBinPath(module.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), module.BuildType);
				
				string normalizedPreMappedDirectory = PathNormalizer.Normalize(preMappingPath.OutputDir);
				string preMappedName = preMappingPath.OutputName + RemapExtension(module, preMappingPath.Extension);
				string normalizedPostMappedDirectory = PathNormalizer.Normalize(postMappingPath.OutputDir);
				string postMappedName = postMappingPath.OutputName + RemapExtension(module, postMappingPath.Extension);

				return RemapFileSet(module, binaryFiles, preMappedName, postMappedName, normalizedPreMappedDirectory, normalizedPostMappedDirectory);
			}
			else // for use dependency we don't have Project so use parent's and pull in public data
			{
				// get pre and post mapped path
				ModulePath preMappingPath = ModulePath.Public.GetModuleBinPath(parentModule.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), defaultSuffix, module.BuildType, ignoreOutputMapping: true);
				ModulePath postMappingPath = ModulePath.Public.GetModuleBinPath(parentModule.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), defaultSuffix, module.BuildType);

				string normalizedPreMappedDirectory = PathNormalizer.Normalize(preMappingPath.OutputDir);
				string preMappedName = preMappingPath.OutputName + RemapExtension(module, preMappingPath.Extension);
				string normalizedPostMappedDirectory = PathNormalizer.Normalize(postMappingPath.OutputDir);
				string postMappedName = postMappingPath.OutputName + RemapExtension(module, postMappingPath.Extension);

				return RemapFileSet(module, binaryFiles, preMappedName, postMappedName, normalizedPreMappedDirectory, normalizedPostMappedDirectory);
			}
		}

		public static FileSet RemapModuleLibraryPaths(ProcessableModule module, ProcessableModule parentModule, FileSet libraryFiles)
		{
			if (libraryFiles == null)
			{
				return libraryFiles;
			}

			if (!(module is Module_UseDependency))
			{
				// get pre and post mapped path
				ModulePath preMappingPath = ModulePath.Public.GetModuleLibPath(module.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), module.BuildType, ignoreOutputMapping: true);
				ModulePath postMappingPath = ModulePath.Private.GetModuleLibPath(module);
				string normalizedPreMappedDirectory = PathNormalizer.Normalize(preMappingPath.OutputDir);
				string preMappedName = preMappingPath.OutputName + preMappingPath.Extension;
				string normalizedPostMappedDirectory = PathNormalizer.Normalize(postMappingPath.OutputDir);
				string postMappedName = postMappingPath.OutputName + postMappingPath.Extension;

				return RemapFileSet(module, libraryFiles, preMappedName, postMappedName, normalizedPreMappedDirectory, normalizedPostMappedDirectory);
			}
			else // for use dependency we don't have Project so use parent's and pull in public data
			{
				// get pre and post mapped path
				ModulePath preMappingPath = ModulePath.Public.GetModuleLibPath(parentModule.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), module.BuildType, ignoreOutputMapping: true);
				ModulePath postMappingPath = ModulePath.Public.GetModuleLibPath(parentModule.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), module.BuildType);
				string normalizedPreMappedDirectory = PathNormalizer.Normalize(preMappingPath.OutputDir);
				string preMappedName = preMappingPath.OutputName + preMappingPath.Extension;
				string normalizedPostMappedDirectory = PathNormalizer.Normalize(postMappingPath.OutputDir);
				string postMappedName = postMappingPath.OutputName + postMappingPath.Extension;

				return RemapFileSet(module, libraryFiles, preMappedName, postMappedName, normalizedPreMappedDirectory, normalizedPostMappedDirectory);
			}
		}

		private static FileSet RemapFileSet(ProcessableModule module, FileSet inputFiles, string preMappedName, string postMappedName, string normalizedPreMappedDirectory, string normalizedPostMappedDirectory)
		{
			// exit early if no remap required
			if (preMappedName == postMappedName && normalizedPreMappedDirectory == normalizedPostMappedDirectory)
			{
				return inputFiles;
			}

			// copy input set but don't fail on missing files, file paths that haven't been remapped won't exist
			FileSet safeInputFiles = new FileSet(inputFiles)
			{
				FailOnMissingFile = false
			};

			// synchronized expansion of fileset items, legacy code had the option to lock this so presumably a concurrency issue exists
			FileItemList expandedFileItems = null;
			lock (safeInputFiles)
			{
				expandedFileItems = safeInputFiles.FileItems;
			}

			// ensure slashes for correct substring offsets
			normalizedPreMappedDirectory = normalizedPreMappedDirectory.EnsureTrailingSlash();
			normalizedPostMappedDirectory = normalizedPostMappedDirectory.EnsureTrailingSlash();

			string RemapPath(string normalizedPath)
			{
				if (normalizedPath != null && normalizedPath.StartsWith(normalizedPreMappedDirectory) && !normalizedPath.StartsWith(normalizedPostMappedDirectory))
				{
					return Path.Combine(normalizedPostMappedDirectory, normalizedPath.Substring(normalizedPreMappedDirectory.Length));
				}
				return normalizedPath;
			}

			FileSet remappedFiles = new FileSet
			{
				BaseDirectory = RemapPath(safeInputFiles.BaseDirectory)
			};

			foreach (FileItem item in expandedFileItems)
			{
				string itemNormalizedPath = item.FullPath;
				itemNormalizedPath = RemapExtension(module, itemNormalizedPath);

				// remap from old directory to new directory
				itemNormalizedPath = RemapPath(itemNormalizedPath);

				// remap from old name to new name
				if (itemNormalizedPath.EndsWith(preMappedName) && !itemNormalizedPath.EndsWith(postMappedName))
				{
					itemNormalizedPath = Path.Combine(itemNormalizedPath.Substring(0, itemNormalizedPath.Length - preMappedName.Length), postMappedName);
				}

				remappedFiles.Include(itemNormalizedPath, RemapPath(item.BaseDirectory), item.AsIs, safeInputFiles.FailOnMissingFile, item.Data, item.OptionSetName);
			}

			return remappedFiles;
		}


		public static IEnumerable<PathString> MapDependentOutputDir(Project project, IEnumerable<PathString> input, OptionSet outputmapOptionset, string packageName, string type)
		{
			var output = input;
			if (outputmapOptionset != null && input != null && input.Count() > 0)
			{
				string pathmapping;
				string diroption = "config" + ((type == "lib") ? "lib" : "bin") + "dir";
				if (outputmapOptionset.Options.TryGetValue(diroption, out pathmapping))
				{
					var newpath = PathString.MakeNormalized(pathmapping);
					var inputbasedir = PackageMap.Instance.GetBuildGroupRoot(project, packageName);

					output = input.Select(path => (path.Path.StartsWith(inputbasedir) || path.Path.StartsWith(newpath.Path)) ? newpath : path);
				}
			}
			return output;
		}

		public static PathString MapDependentOutputDir(Project project, PathString input, OptionSet outputmapOptionset, string packageName, string type)
		{
			var output = input;
			if (outputmapOptionset != null && input != null && !input.IsNullOrEmpty())
			{
				string pathmapping;
				string diroption = "config" + ((type == "lib") ? "lib" : "bin") + "dir";
				if (outputmapOptionset.Options.TryGetValue(diroption, out pathmapping))
				{
					var newpath = PathString.MakeNormalized(pathmapping);
					var inputbasedir = PackageMap.Instance.GetBuildGroupRoot(project, packageName);

					output = (input.Path.StartsWith(inputbasedir) || input.Path.StartsWith(newpath.Path)) ? newpath : input;
				}
			}
			return output;
		}
	}
}
