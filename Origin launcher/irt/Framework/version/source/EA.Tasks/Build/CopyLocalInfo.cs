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
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;
using NAnt.Core.PackageCore;
using NAnt.Core.Nuget;

namespace EA.Eaconfig.Build
{
	public enum CopyLocalType 
	{ 
		True,		// copy local everything
		False,		// copy local nothing
		Slim,		// copy local direct dependencies only - used in DotNet context to indicate not to copy dependent assemblies, in all other context equivalent to true
		Undefined	// no value - in most scenarios this means no override copy local and logic should only follow copy local as defined by dependency

		// TODO: remove copy local slim
		// -dvaliant 2016/04/04
		// right now slim is always treated exactly the same way as true - it should be removed, however we should
		// add deprecation message in case anyone uses it i.e <copylocal>slim</coplylocal>
	}

	// a fileset reference valid for copylocal along with some extra data required for copy local
	public class CopyLocalFileSet
	{
		public readonly FileSet FileSet;

		public readonly bool AllowLinking;
		public readonly bool KeepRelativePath;
		public readonly bool NonCritical;
		public readonly bool SkipUnchanged;

		public CopyLocalFileSet(FileSet fileSet, bool keepRelativePath = false, bool nonCritical = false, bool skipUnchanged = true, bool allowLinking = true)
		{
			FileSet = fileSet;

			AllowLinking = allowLinking;
			KeepRelativePath = keepRelativePath;
			NonCritical = nonCritical;
			SkipUnchanged = skipUnchanged;
		}
	}

	// base class for copy local operations, contains most of the information needed about a copy local file
	public abstract class CopyLocalInfoBase
	{
		public readonly string AbsoluteSourcePath;

		// TODO copy local info settings
		// -dvaliant 2016/04/05
		/* NonCritical and SkipUnchanged are only supported in Visual Studio right now, should make sure all code that actually does copy local respects these */

		public readonly bool AllowLinking;				// if true, copy can be a hard link if if destination module specifies hard link copylocal
		public readonly bool NonCritical;				// if true, doesn't matter if this copy fails
		public readonly bool PostBuildCopyLocal;		// if true, this copy operation should be performed by it's SourceModule
		public readonly bool SkipUnchanged;				// if true, copy should only be performed if source file is newer

		public readonly IModule SourceModule;			// module that source path file belongs to 

		protected CopyLocalInfoBase(string absoluteSourcePath, bool nonCritical, bool skipUnchanged, bool postBuildCopyLocal, bool allowLinking, IModule sourceModule)
		{
			AbsoluteSourcePath = absoluteSourcePath;
			AllowLinking = allowLinking;
			NonCritical = nonCritical;
			SourceModule = sourceModule;
			PostBuildCopyLocal = postBuildCopyLocal;
			SkipUnchanged = skipUnchanged;
		}

		protected CopyLocalInfoBase(CopyLocalInfoBase other)
		{
			AbsoluteSourcePath = other.AbsoluteSourcePath;
			AllowLinking = other.AllowLinking;
			NonCritical = other.NonCritical;
			PostBuildCopyLocal = other.PostBuildCopyLocal;
			SkipUnchanged = other.SkipUnchanged;
			SourceModule = other.SourceModule;
		}
	}

	// represents a copy local operation independent of target destination module, this class is only used
	// as a caching optimization when calcualting copy local operations
	public class CachedCopyLocalInfo : CopyLocalInfoBase
	{
		public readonly string RelativeDestPath;

		public CachedCopyLocalInfo(string absoluteSourcePath, string relativeDestPath, bool skipUnchanged, bool nonCritical, bool allowLinking, IModule sourceModule)
			: base(absoluteSourcePath, nonCritical, skipUnchanged, false, allowLinking, sourceModule)
		{
			RelativeDestPath = relativeDestPath;
		}

		public override string ToString()
		{
			return String.Format("{0} -> {1}", AbsoluteSourcePath, RelativeDestPath);
		}
	}

	// represents a single fully defined copy local operations that contains information about source and
	// destination modules
	public class CopyLocalInfo : CopyLocalInfoBase
	{
		public readonly string AbsoluteDestPath;	// final copy destination for this file
		public readonly List<IModule> DestinationModules;  // module this file is being copied to
		public readonly bool UseHardLink;

		// This is kept for backwards compatibility. DestinationModules always contain 1 entry when no "reverse" copy locals exist
		public IModule DestinationModule { get { return DestinationModules.First(); } }

		public delegate IEnumerable<CopyLocalFileSet> CopyLocalDelegate(IModule module, IModule parent); // function to collect the copy local outputs relevant to a module

		public static CopyLocalDelegate CommonDelegate = GetCopyLocalBinaryOutputs;
		public static CopyLocalDelegate NativeDelegate = GetNativeCopyLocalOutputs;

		public CopyLocalInfo(CachedCopyLocalInfo cached, IModule destinationModule)
			: base(cached)
		{
			AbsoluteDestPath = PathString.MakeCombinedAndNormalized(destinationModule.OutputDir.Path, cached.RelativeDestPath).Path;
			DestinationModules = new List<IModule>() { destinationModule };
			UseHardLink = destinationModule.CopyLocalUseHardLink;
		}

		// NOT a normal copy constructor, this is only used for the post build reverse function
		private CopyLocalInfo(CopyLocalInfo reverse, List<IModule> destinationModules)
			: base (
				absoluteSourcePath: reverse.AbsoluteSourcePath,
				nonCritical: true, // post build copies are a workflow convenience so failure shouldn't break the build
				skipUnchanged: reverse.SkipUnchanged,

				postBuildCopyLocal: true, // this will always be a postbuild copy local
				allowLinking: reverse.AllowLinking,
				sourceModule: reverse.SourceModule
			)
		{
			AbsoluteDestPath = reverse.AbsoluteDestPath;
			DestinationModules = destinationModules;
			UseHardLink = reverse.UseHardLink;
		}

		// creates a copy of this operation with the source and target modules reversed for post build copies
		public CopyLocalInfo PostBuildReverse(List<IModule> destinationModules)
		{
			return new CopyLocalInfo(this, destinationModules);
		}

		public override string ToString()
		{
			return String.Format("{0} -> {1}", AbsoluteSourcePath, AbsoluteDestPath);
		}

		// gets privately declared assemblies from a module (that may or may not also be public) but aren't the primary build output
		internal static FileSet GetModulePrivateAssembliesFileset(IModule module)
		{
			// returning fileset by name, module fileset include resolved paths to dependent build outputs which are handled elsewhere
			if (module.Package.Project != null)
			{
				FileSet assemblies = module.PropGroupFileSet("assemblies").SafeClone();
				assemblies.FailOnMissingFile = false;

				FileSet extraFiles = new FileSet(module.Package.Project);
				foreach (FileItem itm in assemblies.FileItems)
				{
					string normalizedPath = PathString.MakeNormalized(itm.FullPath).Path;

					string configFile = normalizedPath + ".config";
					if (File.Exists(configFile))
					{
						extraFiles.Include(configFile, baseDir: itm.BaseDirectory, optionSetName: itm.OptionSetName);
					}

					AddImplicitDebugSymbolsCopy(module.Package.Project, itm, ref extraFiles);
				}
				if (extraFiles.FileItems.Any())
				{
					assemblies.Append(extraFiles);
				}

				return assemblies;
			}
			return new FileSet();
		}

		internal static FileSet GetModulePrivateComAssembliesFileset(IModule module)
		{
			// returning fileset by name, module fileset include resolved paths to dependent build outputs which are handled elsewhere
			if (module.Package.Project != null)
			{
				FileSet comAssemblies = module.PropGroupFileSet("comassemblies").SafeClone();
				comAssemblies.FailOnMissingFile = false;

				FileSet pdbFiles = new FileSet(module.Package.Project);
				foreach (FileItem itm in comAssemblies.FileItems)
				{
					AddImplicitDebugSymbolsCopy(module.Package.Project, itm, ref pdbFiles);
				}
				if (pdbFiles.FileItems.Any())
				{
					comAssemblies.Append(pdbFiles);
				}

				return comAssemblies;
			}
			return new FileSet();
		}

		// gets privately declared shared libaries from a module (that may or may not also be public) but aren't the primary build output
		internal static FileSet GetModulePrivateDynamicLibrariesFileset(IModule module)
		{
			if (module.Package.Project != null)
			{
				// returning fileset by name, module fileset include resolved paths to dependent build outputs which are handled elsewhere
				FileSet dlls = module.PropGroupFileSet("dlls").SafeClone();
				dlls.FailOnMissingFile = false;

				if (module.Package.Project.Properties["config-compiler"] == "vc")
				{
					FileSet pdbFiles = new FileSet(module.Package.Project);
					foreach (FileItem itm in dlls.FileItems)
					{
						AddImplicitDebugSymbolsCopy(module.Package.Project, itm, ref pdbFiles);
					}
					if (pdbFiles.FileItems.Any())
					{
						dlls.Append(pdbFiles);
					}
				}

				return dlls;
			}
			return new FileSet();
		}

		// helper functions for getting content files, if module isn't a dotnet module
		// and it needs to expose copylocal files it should do it through public data
		// in initialize.xml
		internal static List<CopyLocalFileSet> GetContentFileSets(IModule module, IPublicData publicData, bool includeResources = false)
		{
			FileSet alwaysCopyContentFiles = new FileSet();
			FileSet copyIfNewerContentFiles = new FileSet();

			// special handling for dotnet, we want to know the content files we should be copying
			// from the module definition
			Module_DotNet dotNetModule = module as Module_DotNet;
			if (dotNetModule != null)
			{
				AddContentFileSetsToCopySets(dotNetModule.Compiler.ContentFiles, ref alwaysCopyContentFiles, ref copyIfNewerContentFiles);
				if (includeResources)
				{
					AddContentFileSetsToCopySets(dotNetModule.Compiler.Resources, ref alwaysCopyContentFiles, ref copyIfNewerContentFiles);
					AddContentFileSetsToCopySets(dotNetModule.Compiler.NonEmbeddedResources, ref alwaysCopyContentFiles, ref copyIfNewerContentFiles);
				}
			}

			// this allows initialize only / prebuilt packages to export content files (particularly useful for nuget packages)
			if (publicData != null)
			{
				AddContentFileSetsToCopySets(publicData.ContentFiles, ref alwaysCopyContentFiles, ref copyIfNewerContentFiles);
			}

			return new List<CopyLocalFileSet> 
			{
				new CopyLocalFileSet(alwaysCopyContentFiles, keepRelativePath: true, skipUnchanged: false),
				new CopyLocalFileSet(copyIfNewerContentFiles, keepRelativePath: true)
			};
		}

		// gather all binary outputs that qualify for copy local, ie executables and shared libraries
		private static IEnumerable<CopyLocalFileSet> GetCopyLocalBinaryOutputs(IModule module, IModule parent)
		{
			return GetNativeCopyLocalOutputs(module, parent).Concat(GetManagedCopyLocalOutputs(module, parent));
		}

		// gather native copy local outputs, ie native executables and shared libs
		private static IEnumerable<CopyLocalFileSet> GetNativeCopyLocalOutputs(IModule module, IModule parent)
		{
			List<FileSet> additional = new List<FileSet>() { GetModulePrivateDynamicLibrariesFileset(module) };
			IPublicData publicData = module.Public(parent);
			if (publicData != null)
			{
				additional.Add(publicData.DynamicLibraries);  // This fileset contains both "dlls" and "dlls.external" fileset export!
				{
					// If a module is purely use dependent, it's build file is not loaded and the module.Package.Project will be null.
					// In that case, just use the parent's Project.
					Project proj = (module is Module_UseDependency) ? parent.Package.Project : module.Package.Project;
					bool isVcCompiler = proj.Properties["config-compiler"] == "vc";
					if (isVcCompiler)
					{
						// If this is a utility module and we are doing build using Visual Studio compiler, test for presence of pdb file
						// and also copy pdb if file exists.  (For buildable module, it would be more beneficial to just build directly to output folder.)
						FileSet pdbFiles = new FileSet(publicData.DynamicLibraries.Project);
						pdbFiles.BaseDirectory = publicData.DynamicLibraries.BaseDirectory;
						foreach (FileItem itm in publicData.DynamicLibraries.FileItems)
						{
							string normalizedPath = PathString.MakeNormalized(itm.FullPath).Path;

							// If this file points to the module output dir already, this module is not a Utililty module and 
							// it is a buildable module.  Naturally, the build will create the pdb already and we can skip it.
							// If this module is a utility module (or came from dlls.external), it won't be 
							// pointing to module output dir.
							if (module.OutputDir != null && normalizedPath.StartsWith(module.OutputDir.Path))
							{
								continue;
							}

							AddImplicitDebugSymbolsCopy(proj, itm, ref pdbFiles);
						}
						if (pdbFiles.FileItems.Any())
							additional.Add(pdbFiles);
					}
					if (publicData is PublicData fullPublicData)
					{
						// If program files fileset in a Utility are not supposed to be copied,
						// The fullPublicData.Programs property will be initialized with empty FileSet in
						// ModuleProcessor_SetModuleData.SetDependentPublicData().
						additional.Add(fullPublicData.Programs);
						if (isVcCompiler)
						{
							FileSet pdbFiles = new FileSet(fullPublicData.Programs.Project);
							pdbFiles.BaseDirectory = fullPublicData.Programs.BaseDirectory;
							foreach (FileItem itm in fullPublicData.Programs.FileItems)
							{
								string normalizedPath = PathString.MakeNormalized(itm.FullPath).Path;
								if (module.OutputDir != null && normalizedPath.StartsWith(module.OutputDir.Path))
								{
									continue;
								}

								AddImplicitDebugSymbolsCopy(proj, itm, ref pdbFiles);

							}
							if (pdbFiles.FileItems.Any())
							{
								additional.Add(pdbFiles);
							}
						}
					}
				}
			}

			List<CopyLocalFileSet> fileSets = GetCopyLocalFileSets
			(
				module,
				module.IsKindOf(Module.Native) && module.IsKindOf(Module.DynamicLibrary | Module.Program) && !module.IsKindOf(Module.ExternalVisualStudio),
				additionalFileSets: additional
			);

			// add content files
			fileSets.AddRange(GetContentFileSets(module, publicData));

			return fileSets;
		}

		// gather all managed copy local outputs, typically you should use CopyLocalBinaryOutputs, for managed modules
		// as they can depend on native modules
		private static IEnumerable<CopyLocalFileSet> GetManagedCopyLocalOutputs(IModule module, IModule parent)
		{
			List<FileSet> additional = new List<FileSet>() 
			{ 
				GetModulePrivateAssembliesFileset(module),
				GetModulePrivateComAssembliesFileset(module)
			};
			IPublicData publicData = module.Public(parent);
			if (publicData != null)
			{
				additional.Add(publicData.Assemblies);
				{
					// If a module is purely use dependent, it's build file is not loaded and the module.Package.Project will be null.
					// In that case, just use the parent's Project.
					Project project = (module is Module_UseDependency) ? parent.Package.Project : module.Package.Project;
					FileSet assembliesExtraFiles = new FileSet(publicData.Assemblies.Project)
					{
						BaseDirectory = publicData.Assemblies.BaseDirectory
					};
					foreach (FileItem itm in publicData.Assemblies.FileItems)
					{
						string normalizedPath = PathString.MakeNormalized(itm.FullPath).Path;

						// If this file points to the module output dir already, this module is not a Utililty module and 
						// it is a buildable module.  Naturally, the build will create the pdb already and we can skip it.
						// If this module is a utility module (or came from dlls.external), it won't be 
						// pointing to module output dir.
						if (module.OutputDir != null && normalizedPath.StartsWith(module.OutputDir.Path))
						{
							continue;
						}

						string configFile = normalizedPath + ".config";
						if (File.Exists(configFile))
						{
							assembliesExtraFiles.Include(configFile, baseDir: itm.BaseDirectory, optionSetName: itm.OptionSetName);
						}

						AddImplicitDebugSymbolsCopy(project, itm, ref assembliesExtraFiles);
					}
					if (assembliesExtraFiles.FileItems.Any())
						additional.Add(assembliesExtraFiles);

					if (publicData is PublicData fullPublicData)
					{
						additional.Add(fullPublicData.Programs);
						FileSet programsExtraFiles = new FileSet();
						foreach (FileItem itm in fullPublicData.Programs.FileItems)
						{
							string normalizedPath = PathString.MakeNormalized(itm.FullPath).Path;

							// We can skip buildable module as it would be created already.
							if (module.OutputDir != null && normalizedPath.StartsWith(module.OutputDir.Path))
								continue;

							string configFile = normalizedPath + ".config";
							if (File.Exists(configFile))
							{
								programsExtraFiles.Include(configFile, baseDir: itm.BaseDirectory, optionSetName: itm.OptionSetName);
							}
							AddImplicitDebugSymbolsCopy(project, itm, ref programsExtraFiles);
						}
						if (programsExtraFiles.FileItems.Any())
						{
							additional.Add(programsExtraFiles);
						}
					}
				}
			}

			// gather shared libraries and dlls
			List<CopyLocalFileSet> fileSets = GetCopyLocalFileSets
			(
				module, 
				includePrimaryOutputs: module.IsKindOf(Module.Managed | Module.DotNet) && !module.IsKindOf(Module.ExternalVisualStudio),
				additionalFileSets: additional
			);

			// add content files
			fileSets.AddRange(GetContentFileSets(module, publicData));

			return fileSets;
		}

		// for binaries types, we want to implicity also copy any debug symbols files as well // TODO pdb specific right now, should make platform abstract
		private static void AddImplicitDebugSymbolsCopy(Project project, FileItem binaryFileItem, ref FileSet debugSymbolsFileset)
		{
			if (project.Properties.GetBooleanPropertyOrDefault("eaconfig.prebuilt.skip-copylocal-pdb", true))
			{
				return;
			}

			string copyPdbOption = "false";
			if (
				!String.IsNullOrEmpty(binaryFileItem.OptionSetName) &&
				project.NamedOptionSets.TryGetValue(binaryFileItem.OptionSetName, out OptionSet itemSet) &&
				(itemSet?.Options?.TryGetValue("copy-pdb", out copyPdbOption) ?? false) &&
				Expression.Evaluate(copyPdbOption)
			)
			{
				return;
			}

			string pdbFile = Path.ChangeExtension(PathNormalizer.Normalize(binaryFileItem.FullPath), "pdb");
			if (File.Exists(pdbFile))
			{
				debugSymbolsFileset.Include(pdbFile, baseDir: binaryFileItem.BaseDirectory, optionSetName: binaryFileItem.OptionSetName);
			}
		}

		// helper function for extracting necessary filesets from a module, assumes if includePrimaryOutputs is set to true then module.PrimaryOutput() is
		// valid
		private static List<CopyLocalFileSet> GetCopyLocalFileSets(IModule module, bool includePrimaryOutputs, List<FileSet> additionalFileSets)
		{
			List<CopyLocalFileSet> fileSets;
			if (includePrimaryOutputs)
			{
				fileSets = new List<CopyLocalFileSet>();

				string primaryOutput = module.PrimaryOutput();

				// add the primary output
				FileSet primaryOuputFileSet = new FileSet(module.Package.Project)
				{
					FailOnMissingFile = false // this a build output and may not be created yet
				};
				primaryOuputFileSet.Include(primaryOutput, module.OutputDir.Path);
				fileSets.Add(new CopyLocalFileSet(primaryOuputFileSet));

				// implicitly add debug symbols copy if appropriate
				string primaryDebugSymbolsOutput = module.PrimaryDebugSymbolsOutput();
				if (primaryDebugSymbolsOutput != null && primaryDebugSymbolsOutput != primaryOutput) // extra safety, should be be null for platforms that don't have a separate symnols file but there's an outside chance it has been set to same as binary path
				{
					// add pdbs regardless of what build options are set on the module, as we can never be 100% sure what the user has done
					// to their compiler flags, instead just mark this as a non critical file
					FileSet debugSymbolsFileSet = new FileSet(module.Package.Project)
					{
						FailOnMissingFile = false
					};
					debugSymbolsFileSet.Include(primaryDebugSymbolsOutput, module.OutputDir.Path);
					fileSets.Add(new CopyLocalFileSet(debugSymbolsFileSet, nonCritical: true));
				}

				// Test if this module is a C# module that contains an app.config file in the resource files fileset.  If it
				// does, a [outputfile].config will get generated and we need to copy that file over as well.
				if (module is Module_DotNet)
				{
					Module_DotNet dotNetModule = module as Module_DotNet;
					if (dotNetModule.Compiler != null)
					{
						if (dotNetModule.Compiler.Resources.FileItems.Any(f => Path.GetFileName(f.FileName).ToLower() == "app.config") ||
							dotNetModule.Compiler.NonEmbeddedResources.FileItems.Any(f => Path.GetFileName(f.FileName).ToLower() == "app.config"))
						{
							FileSet appConfigFileSet = new FileSet(module.Package.Project);
							appConfigFileSet.FailOnMissingFile = false;
							appConfigFileSet.Include(module.PrimaryOutput() + ".config", module.OutputDir.Path);
							fileSets.Add(new CopyLocalFileSet(appConfigFileSet, nonCritical: true));
						}
					}
				}

				// add additional filesets but use copies that exclude the primary output
				foreach (FileSet fileSet in additionalFileSets)
				{
					FileSet primaryExcludeCopy = new FileSet(fileSet);
					primaryExcludeCopy.Exclude(primaryOutput);
					fileSets.Add(new CopyLocalFileSet(primaryExcludeCopy));
				}
			}
			else
			{
				fileSets = new List<CopyLocalFileSet>(additionalFileSets.Count);
				// if not including primary output we can just take all additional files as is
				fileSets.AddRange(additionalFileSets.Select(fs => new CopyLocalFileSet(fs)));
			}

			return fileSets;
		}

		// given contentFiles - a fileset containing content files, add the includes and exclude to alwaysCopyContentFiles and copyIfNewerContentFiles
		// based on the optionsets of each file pattern
		private static void AddContentFileSetsToCopySets(FileSet contentFiles, ref FileSet alwaysCopyContentFiles, ref FileSet copyIfNewerContentFiles)
		{
			// copy fileset includes that set copy local optionsets
			foreach (FileSetItem contentInclude in contentFiles.Includes)
			{
				// see ModuleProcessor_SetModuleData.cs for how these optionsets are handled 
				if (contentInclude.OptionSet == "copy-always")
				{
					FileSetUtil.FileSetInclude
					(
						alwaysCopyContentFiles,
						contentInclude.Pattern.Value,
						contentInclude.Pattern.BaseDirectory ?? contentFiles.BaseDirectory,
						contentInclude.OptionSet,
						contentInclude.AsIs
					);
				}
				else if (contentInclude.OptionSet == "copy-if-newer")
				{
					FileSetUtil.FileSetInclude
					(
						copyIfNewerContentFiles,
						contentInclude.Pattern.Value,
						contentInclude.Pattern.BaseDirectory ?? contentFiles.BaseDirectory,
						contentInclude.OptionSet,
						contentInclude.AsIs
					);
				}
			}

			// copy excludes
			foreach (FileSetItem contentExclude in contentFiles.Excludes)
			{
				FileSetUtil.FileSetExclude
				(
					alwaysCopyContentFiles,
					contentExclude.Pattern.Value,
					contentExclude.Pattern.BaseDirectory ?? contentFiles.BaseDirectory,
					contentExclude.OptionSet,
					contentExclude.AsIs
				);
				FileSetUtil.FileSetExclude
				(
					copyIfNewerContentFiles,
					contentExclude.Pattern.Value,
					contentExclude.Pattern.BaseDirectory ?? contentFiles.BaseDirectory,
					contentExclude.OptionSet,
					contentExclude.AsIs
				);
			}
		}
	}
}
