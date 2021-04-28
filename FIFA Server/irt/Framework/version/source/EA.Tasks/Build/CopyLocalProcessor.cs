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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.ExceptionServices;
using System.Threading.Tasks;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.PackageCore;
using NAnt.NuGet;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Build
{
	// aliasing type for copy local cache dictionary
	/* potentially composite key of bool, CopyLocalDelegate, Dependency<IModule> and a flat dictionary might be more efficient */
	using CopyLocalDelegateCache = ConcurrentDictionary
	<
		CopyLocalInfo.CopyLocalDelegate, // need a separate cache for each copy local delegate type, DotNet delegate for example return different results than native delegate
		ConcurrentDictionary
		<
			Dependency<IModule>,         /* For each *dependency* cache all the copy local files from the subgraph of modules below that dependency link in the graph. We need 
                                         to cache on dependency rather than module because a module can export different copylocal files to its parent based on the properties
                                         set by the parent module's package. For example if A depends on C, B depends on C, and C depends on D we need to evaluate D once (C ->
                                         D) link but C twice ( A -> C, B -> C ) because A and B can have different "views" of C. A and B will have the same view of D though
                                         because they only reference via a single dependency link C. */
			Dictionary<string, CachedCopyLocalInfo>    /* the cache, caches a list of copy local files from the subgraph below the dependency */
		>
	>;

	class DependencyComparer : IEqualityComparer<Dependency<IModule>>
	{
		public bool Equals(Dependency<IModule> x, Dependency<IModule> y)
		{
			if (!object.ReferenceEquals(x.Dependent, y.Dependent))
				return false;
			return x.BuildGroup == y.BuildGroup && x.Type == y.Type;
		}
		public int GetHashCode(Dependency<IModule> obj)
		{
			return obj.Dependent.GetHashCode() ^ obj.BuildGroup.GetHashCode() ^ obj.Type.GetHashCode();
		}
	}

	internal class CopyLocalProcessor
	{
		/* Declare TWO caches, we need two because we are caching all the files from a sub graph below a dependency, however the results of walking that subgraph will be
        different depending on whether we did the walk in a subgraph below a module which specified copy local override or not. consider the following graph modules:

            A (declares copylocal override)             A depends (without explict copy local dependency) on C
            B (doesn't declare copy local override)     B depends (with explict copy local dependency) on C
            C (doesn't declare copy local override)     C depends (with explict copy local dependency) on D
            D (doesn't declare copy local override)     D depends (without explict copy local dependency) on E
            E (depends on nothing

        Assuming copy local scan starts with module B it will follow explicit dependency chain from B to D but will not go as far down as E because there is no explicit 
        copy local chain between D and E. We cache the copylocal files that C inherits from D (which includes nothing from E). Copy local scan then scans from A. Follows
        A - C then C -> D. On reaching C -> D it takes the cached files from the previous chain that led us here. THIS IS INCORRECT because A declares copy local override
        and so it's copy local scan should evaluate links that aren't explicitly copy local - it should evaulate D -> E as well. To avoid this problem we have an override
        cache and a non-override cache depending on which case we are in. */
		private CopyLocalDelegateCache _copyLocalRegularCache = new CopyLocalDelegateCache();
		private CopyLocalDelegateCache _copyLocalOverrideCache = new CopyLocalDelegateCache();

		struct PostCopyLocalRecord
		{
			public CopyLocalInfo OriginalCopyLocalInfo;
			public ConcurrentBag<IModule> DestinationModules;
		}

		internal void ProcessAll(Project project, List<IModule> buildGraphModules)
		{
			// set the copylocal files for every module in a build graph, this is the only place
			// copy local files should be assigned (barring postbuild option below)
#if NANT_CONCURRENT
			bool parallel = (Project.NoParallelNant == false);
#else
			bool parallel = false;
#endif

			IModule[] modules = buildGraphModules.ToArray();
			int currentIndex = 0;
			long processedCount = 0;

			ConcurrentDictionary<KeyValuePair<string, string>, CopyLocalInfo> allCopyLocals = new ConcurrentDictionary<KeyValuePair<string, string>, CopyLocalInfo>();
			ExceptionDispatchInfo processingException = null;

			void processModules()
			{
				while (processingException == null)
				{
					int index = Interlocked.Increment(ref currentIndex) - 1;
					if (index >= modules.Length)
					{
						break;
					}

					ProcessableModule module = (ProcessableModule)modules[index];
					try
					{
						HashSet<string> runtimeDependencyFiles = null;
						(module.CopyLocalFiles, runtimeDependencyFiles) = ProcessCopyLocalDependencies(module);
						module.RuntimeDependencyFiles.UnionWith(runtimeDependencyFiles);
					}
					catch (Exception e)
					{
						processingException = ExceptionDispatchInfo.Capture(e);
						break;
					}

					// We need to validate UseHardLink and AllowLink.
					foreach (CopyLocalInfo copyLocal in module.CopyLocalFiles)
					{
						CopyLocalInfo cached = allCopyLocals.GetOrAdd(new KeyValuePair<string, string>(copyLocal.AbsoluteSourcePath, copyLocal.AbsoluteDestPath), copyLocal);
						if (cached == copyLocal)
						{
							// this reference is already validated
							continue;
						}

						// Check so allow-linking and use-hard-link is consistent (Can use "First()" destination module since all of these are "forward" copy local and only have one DestinationModule)
						if (copyLocal.AllowLinking != cached.AllowLinking)
						{
							processingException = ExceptionDispatchInfo.Capture(new BuildException(String.Format("Copy local \"AllowLinking\" conflict! Module {0} (from package {1}) and module {2} (from package {3}) both do the same copy local but with different AllowLinking setting ({4})."
								, copyLocal.DestinationModules.First().Name, copyLocal.DestinationModules.First().Package.Name, cached.DestinationModules.First().Name, cached.DestinationModules.First().Package.Name, copyLocal.ToString())));
							break;
						}
						if (copyLocal.UseHardLink != cached.UseHardLink)
						{
							processingException = ExceptionDispatchInfo.Capture(new BuildException(String.Format("Copy local \"UseHardLink\" conflict! Module {0} (from package {1}) and module {2} (from package {3}) both do the same copy local but with different UseHardLink setting ({4})."
								, copyLocal.DestinationModules.First().Name, copyLocal.DestinationModules.First().Package.Name, cached.DestinationModules.First().Name, cached.DestinationModules.First().Package.Name, copyLocal.ToString())));
							break;
						}
					}
					Interlocked.Increment(ref processedCount);
				}
			}

			if (parallel)
			{
				for (int i = 0, e = Environment.ProcessorCount - 1; i != e; ++i)
				{
					System.Threading.Tasks.Task.Run(() => processModules());
				}
			}

			// Smart to place some work here while other threads might processing the modules --
			// if post build copy local is set then for every copy local operation reverse map the operations so
			// that the source module for the file is responsible for copying it
			ConcurrentDictionary<KeyValuePair<string, string>, PostCopyLocalRecord>[] reverseCopyLists = null;
			bool postBuildCopyLocal = project.Properties.GetBooleanPropertyOrDefault("nant.postbuildcopylocal", false);
			if (postBuildCopyLocal)
			{
				reverseCopyLists = new ConcurrentDictionary<KeyValuePair<string, string>, PostCopyLocalRecord>[modules.Length];
				for (int i = 0, e = modules.Length; i != e; ++i)
				{
					reverseCopyLists[i] = new ConcurrentDictionary<KeyValuePair<string, string>, PostCopyLocalRecord>();
				}
			}

			// Help out processing modules, or process serially if parallel disabled
			processModules();

			// Make sure all modules are processed before continuing (all CopyLocalFile lists are up to date after this while loop)
			while ((int)Interlocked.Read(ref processedCount) != modules.Length)
			{
				if (processingException != null)
				{
					break;
				}
				Thread.Yield();
			}

			// Throw exception that might have happened in the parallel processing code
			if (processingException != null)
			{
				processingException.Throw();
			}

			// Read description further up
			if (postBuildCopyLocal)
			{
				// First calculate all the reversed copy locals without modifying the actual CopyLocalFiles list (so we can do it in parallel)
				Parallel.ForEach(modules, (destinationModule) =>
				{
					foreach (CopyLocalInfo copyLocalInfo in destinationModule.CopyLocalFiles)
					{
						ProcessableModule sourceModule = (ProcessableModule)copyLocalInfo.SourceModule;
						bool isNotSameModule = sourceModule != destinationModule; // don't need to reverse copy of module's own files
						bool isNotUseDependency = !(sourceModule is Module_UseDependency); // if module is use dependency is performs no build action so we don't need to reverse copy
						if (isNotSameModule && isNotUseDependency)
						{
							KeyValuePair<string, string> key = new KeyValuePair<string, string>(copyLocalInfo.AbsoluteSourcePath, copyLocalInfo.AbsoluteDestPath);
							PostCopyLocalRecord record = reverseCopyLists[sourceModule.GraphOrder].GetOrAdd(key, (k) => new PostCopyLocalRecord() { OriginalCopyLocalInfo = copyLocalInfo, DestinationModules = new ConcurrentBag<IModule>() });

							record.DestinationModules.Add(destinationModule);
						}
					}
				});

				// Merge the CopyLocalFiles so we have both directions stored
				Parallel.ForEach(modules, (m) =>
				{
					foreach (var rec in reverseCopyLists[((ProcessableModule)m).GraphOrder])
					{
						var reversedCopy = rec.Value.OriginalCopyLocalInfo.PostBuildReverse(rec.Value.DestinationModules.ToList());
						m.CopyLocalFiles.Add(reversedCopy);
					}
				});
			}
		}

		private (List<CopyLocalInfo>, HashSet<string>) ProcessCopyLocalDependencies(IModule targetModule)
		{
			// we want to transitive propagate copy local dependencies through copy local nodules,
			// so keep track of the stack of copy local modules in the dependency chain, if this stack
			// contains any modules then we should treat all dependencies as copy local, we can also 
			// peek at the top of the stack for reporting
			bool isTopLevelCopyLocalModule = targetModule.CopyLocal == CopyLocalType.True || targetModule.CopyLocal == CopyLocalType.Slim;
			Stack<IModule> copyLocalModuleStack = new Stack<IModule>();
			if (isTopLevelCopyLocalModule)
			{
				copyLocalModuleStack.Push(targetModule);
			}

			// add copy local files from target module, modules can have files they copy to their own output dir (e.g content files)
			Dictionary<string, CachedCopyLocalInfo> copyLocalFiles = GetTargetModuleCopyLocalFiles(targetModule);

			// evaluate dependencies recursively, recursive function decides at each dependency whether to stop traversing
			Stack<Dependency<IModule>> dependencyStack = new Stack<Dependency<IModule>>();
			foreach (Dependency<IModule> dependency in targetModule.Dependents)
			{
				dependencyStack.Push(dependency); // keep track of dependency chain, to avoid getting stuck in a cycle
				bool isCopyLocalDependencyChain = dependency.IsKindOf(DependencyTypes.CopyLocal) || isTopLevelCopyLocalModule;
				if (isCopyLocalDependencyChain)
				{
					Dictionary<string, CachedCopyLocalInfo> copyLocalsFromDependency = GetCacheCopyLocalFiles(targetModule, targetModule, dependency, copyLocalModuleStack, dependencyStack);
                    if (copyLocalsFromDependency != null)
                    {
                        foreach (var it in copyLocalsFromDependency)
                        {
                            AddTo(ref copyLocalFiles, it.Value, targetModule);
                        }
                    }
				}
				dependencyStack.Pop();
			}

			if (copyLocalFiles.Count == 0)
			{
				return (new List<CopyLocalInfo>(), new HashSet<string>());
			}

			// copy locals gathered to this point are the cache variety that are independent of target module, now translate these to concrete operations
			// that have target module, 
			IEnumerable<CopyLocalInfo> concreteCopies = copyLocalFiles.Select(cached => new CopyLocalInfo(cached.Value, targetModule));

			// capture file we identified as needing to be alongside the output as a runtime dependency. This is before stripping 
			// files that are already in the correct location that don't need to be copied as this information can be used output
			// runtime file dependencies
			HashSet<string> runtimeDependencies = new HashSet<string>(concreteCopies.Select(concrete => concrete.AbsoluteDestPath));

			// in some cases source and target modules might have the same destination path so filter these now paths are resolved
			List<CopyLocalInfo> copies = concreteCopies
				.Where(concrete => concrete.AbsoluteSourcePath != concrete.AbsoluteDestPath) // the way these paths are constructed guarantees we can do a direct string / string compare
				.ToList();

			return (copies, runtimeDependencies);
		}

		private void AddTo(ref Dictionary<string, CachedCopyLocalInfo> copies, CachedCopyLocalInfo newCopy, IModule topLevelModule)
		{
			if (copies.TryGetValue(newCopy.RelativeDestPath, out CachedCopyLocalInfo previousCopy))
			{
				// we already have an entry for this file, need to determine if this is correct or not
				// note this only catches copy local conflicts to the same *module*, modules in the same
				// package and group often share the same output *folder* so conflicts can still happen
				if (newCopy.SourceModule == previousCopy.SourceModule)
				{
					if (newCopy.AbsoluteSourcePath != previousCopy.AbsoluteSourcePath)
					{
						string newSourcePath = Path.GetDirectoryName(newCopy.AbsoluteSourcePath);
						string previousSourcePath = Path.GetDirectoryName(previousCopy.AbsoluteSourcePath);
						throw new BuildException(String.Format("Copy local name collision! Module {0} (from package {1}) exports two files named {2} from '{3}' and '{4}'. Initialize.xml may be exporting wrong path.",
							newCopy.SourceModule.Name, newCopy.SourceModule.Package.Name, newCopy.RelativeDestPath, newSourcePath, previousSourcePath));
					}
				}
				else // files are coming from different modules
				{
					// Only error if source paths differ. This is allowed because for use dependency packages (packages 
					// that export no modules) to include dependencies they must include the fileset of their dependencies. In a diamond 
					// dependency graph this means the mid level of the graph exports file from the bottom level via two different paths 
					// to the top level. It's also common for serveral packages to export the same third party dll from a single source
					if (newCopy.AbsoluteSourcePath != previousCopy.AbsoluteSourcePath)
					{
						throw new BuildException(String.Format("Copy local name collision! Module {0} (from package {1}) and module {2} (from package {3}) both export a file named {4}. Both cannot be copied to output directory of module {5}.",
							newCopy.SourceModule.Name, newCopy.SourceModule.Package.Name, previousCopy.SourceModule.Name, previousCopy.SourceModule.Package.Name, newCopy.RelativeDestPath, topLevelModule.Name));
					}
				}
			}
			else
			{
				copies.Add(newCopy.RelativeDestPath, newCopy);
			}
		}

		ConcurrentDictionary<Module_DotNet, List<CopyLocalFileSet>> m_contentFileSetsCache = new ConcurrentDictionary<Module_DotNet, List<CopyLocalFileSet>>();

		// TODO module private copy local files refactoring
		// -dvaliant 2016/05/05
		/*
			shares some finicky code / concepts with GetCopyLocalBinaryOutputs in CopyLocalInfo
			related to getting private (not declared in initialize.xml) dlls and assemblies
			that aren't the primary build outputs. examples being:
			<CSharpProgram>
				<dlls>...</dlls>
				<assemblies>...</assemblies>
				<comassemblies>...</comassemblies>
			</CSharpProgram>
			would be nice to consolidate these
		*/
		// get files module should copy to it's own output dir
		private Dictionary<string, CachedCopyLocalInfo> GetTargetModuleCopyLocalFiles(IModule targetModule)
		{
			var copyLocalFiles = new Dictionary<string, CachedCopyLocalInfo>();
			bool isModuleCopyLocal = targetModule.CopyLocal == CopyLocalType.True || targetModule.CopyLocal == CopyLocalType.Slim;

			// get content files and external assemblies from dot net modules
			{
				if (targetModule is Module_DotNet dotNetModule)
				{
					List<CopyLocalFileSet> contentFileSets = m_contentFileSetsCache.GetOrAdd(dotNetModule, (k) =>
					{
						// This might look weird but it is way to make getters initialize code that is expensive to initialize
						List<CopyLocalFileSet> cfs = CopyLocalInfo.GetContentFileSets(dotNetModule, null, includeResources: true);
						foreach (CopyLocalFileSet copyFileSet in cfs)
						{
							foreach (FileItem item in copyFileSet.FileSet.FileItems)
							{
								// gn dn
							}
						}
						return cfs;
					});

					// content files have special handling
					foreach (CopyLocalFileSet copyFileSet in contentFileSets)
					{
						foreach (FileItem item in copyFileSet.FileSet.FileItems)
						{
							// TODO item copy local
							// -dvaliant 2016/04/20
							/* should use module settings for copy local rather than fileset options */
							CopyLocalType itemCopyLocal = item.GetCopyLocal(targetModule);
							bool fileItemExplicitCopyLocalFalse = itemCopyLocal == CopyLocalType.False;

							// determine if we should skip copying file item
							if (item.AsIs)
							{
								// regardless of any other factors we never expect copy local asis include as we expect these to be system files that will always be found implicitly
								targetModule.Package.Project.Log.Debug.WriteLine("Module {0} NOT copying {1} as file is marked as 'asis'.", targetModule.Key, item.Path.Path);
								continue;
							}
							else if (fileItemExplicitCopyLocalFalse)
							{
								targetModule.Package.Project.Log.Debug.WriteLine("Module {0} NOT copying {1} as file optionset explicit sets copylocal to false.", targetModule.Key, item.Path.Path);
								continue;
							}

							string sourceAbsolutePath = Path.GetFullPath(item.FileName);
							string destinationAbsolutePath;
							bool copyRelativePath = !String.IsNullOrEmpty(item.OptionSetName)
									&& NAnt.Core.Functions.OptionSetFunctions.OptionSetGetValue(targetModule.Package.Project, item.OptionSetName, "preserve-basedir") == "on"
									&& !String.IsNullOrEmpty(item.BaseDirectory);
							if (copyFileSet.KeepRelativePath || copyRelativePath)
							{
								// relative dir is only preserved if basedir contains the item
								string baseDir = PathNormalizer.Normalize(item.BaseDirectory ?? copyFileSet.FileSet.BaseDirectory);
								string itemFilename = PathNormalizer.Normalize(item.FileName);
								if (!String.IsNullOrEmpty(baseDir) && itemFilename.StartsWith(baseDir))
								{
									destinationAbsolutePath = PathUtil.RelativePath(item.FileName, baseDir);
								}
								else
								{
									destinationAbsolutePath = Path.GetFileName(item.FileName);
								}
							}
							else
							{
								destinationAbsolutePath = Path.GetFileName(item.FileName);
							}

							CachedCopyLocalInfo newCopy = new CachedCopyLocalInfo
							(
								sourceAbsolutePath,
								destinationAbsolutePath,
								skipUnchanged: copyFileSet.SkipUnchanged,
								nonCritical: copyFileSet.NonCritical,
								allowLinking: copyFileSet.AllowLinking,
								sourceModule: targetModule
							);
							AddTo(ref copyLocalFiles, newCopy, targetModule);
						}
					}
				}
			}

			// add private files that aren't the main build output (they may or may not be declared publically)
			List<CopyLocalFileSet> copyLocalFileSets = new List<CopyLocalFileSet>(4)
			{
				new CopyLocalFileSet(CopyLocalInfo.GetModulePrivateAssembliesFileset(targetModule)),
				new CopyLocalFileSet(CopyLocalInfo.GetModulePrivateComAssembliesFileset(targetModule)),
				new CopyLocalFileSet(CopyLocalInfo.GetModulePrivateDynamicLibrariesFileset(targetModule)),
			};

			foreach (CopyLocalFileSet copyFileSet in copyLocalFileSets)
			{
				foreach (FileItem item in copyFileSet.FileSet.FileItems)
				{
					// TODO item copy local
					// -dvaliant 2016/04/20
					/* should use module settings for copy local rather than fileset options */
					CopyLocalType itemCopyLocal = item.GetCopyLocal(targetModule);
					bool fileItemExplicitCopyLocalTrue = itemCopyLocal == CopyLocalType.True || itemCopyLocal == CopyLocalType.Slim;
					bool fileItemExplicitCopyLocalFalse = itemCopyLocal == CopyLocalType.False;

					// determine if we need to copy
					if (item.AsIs)
					{
						// regardless of any other factors we never expect copy local asis include as we expect these to be system files that will always be found implicitly
						targetModule.Package.Project.Log.Debug.WriteLine("Module {0} NOT copying {1} as file is marked as 'asis'.", targetModule.Key, item.Path.Path);
						continue;
					}
					else if (fileItemExplicitCopyLocalFalse)
					{
						targetModule.Package.Project.Log.Debug.WriteLine("Module {0} NOT copying {1} as file optionset explicit sets copylocal to false.", targetModule.Key, item.Path.Path);
						continue;
					}
					else if (isModuleCopyLocal)
					{
						targetModule.Package.Project.Log.Debug.WriteLine("Module {0} copying {1} as it is a copy local module.", targetModule.Key, item.Path.Path);
					}
					else if (fileItemExplicitCopyLocalTrue)
					{
						targetModule.Package.Project.Log.Debug.WriteLine("Module {0} copying {1} as file optionset explicit sets copylocal to true.", targetModule.Key, item.Path.Path);
					}
					else
					{
						// should be the most common case, use debug verbosity level
						targetModule.Package.Project.Log.Debug.WriteLine("Module {0} NOT copying {1} as module is not copy local.", targetModule.Key, item.Path.Path);
						continue;
					}

					CachedCopyLocalInfo newCopy = new CachedCopyLocalInfo
					(
						Path.GetFullPath(item.FileName),
						Path.GetFileName(item.FileName),
						skipUnchanged: copyFileSet.SkipUnchanged,
						nonCritical: copyFileSet.NonCritical,
						allowLinking: copyFileSet.AllowLinking,
						sourceModule: targetModule
					);
					AddTo(ref copyLocalFiles, newCopy, targetModule);
				}
			}

			// special handling for nuget references
			if (targetModule.Package.Project?.UseNugetResolution ?? false) // if project is null then this is a use dep, this only valid to a nuget package when resolution is disabled (and should be discouraged) so default to false
			{
				FileSet nuGetReferences = null;
				if (targetModule is Module_DotNet dotNetModule)
				{
					nuGetReferences = dotNetModule.NuGetReferences;
				}
				else if (targetModule is Module_Native nativeModule && nativeModule.IsKindOf(Module.Managed))
				{
					nuGetReferences = nativeModule.NuGetReferences;
				}
				if (nuGetReferences != null)
				{
					NugetPackage[] flattenedContents = PackageMap.Instance.GetNugetPackages(targetModule.Package.Project.BuildGraph().GetTargetNetVersion(targetModule.Package.Project), nuGetReferences.FileItems.Select(x => x.FileName)).ToArray();
					foreach (NugetPackage packageContents in flattenedContents)
					{
						foreach ((string relativePath, string baseDir) in packageContents.CopyItems)
						{
							string fullPath = Path.Combine(baseDir, relativePath);
							CachedCopyLocalInfo newCopy = new CachedCopyLocalInfo
							(
								Path.GetFullPath(fullPath),
								relativePath,
								skipUnchanged: true,
								nonCritical: false,
								allowLinking: false,
								sourceModule: targetModule
							);
							AddTo(ref copyLocalFiles, newCopy, targetModule);
						}
					}
				}
			}

			return copyLocalFiles;
		}

		private Dictionary<string, CachedCopyLocalInfo> ProcessCopyLocalDependentsRecursive(IModule toplevelModule, Dependency<IModule> dependency, IModule parent, ref Stack<IModule> copyLocalModuleStack, ref Stack<Dependency<IModule>> dependencyStack)
		{
			IModule dependencyModule = dependency.Dependent;

			// we don't take copy local files from interface dependencies nor
			// do we propagate through them
			if (dependency.Type == DependencyTypes.Interface)
			{
				return null;
			}

			// calculate conditions for continued traversal
			bool isCopyLocalDependency = dependency.IsKindOf(DependencyTypes.CopyLocal);
			bool topIsCopyLocalOverride = copyLocalModuleStack.Any();
            bool isModuleCopyLocal = dependencyModule.CopyLocal == CopyLocalType.True || dependencyModule.CopyLocal == CopyLocalType.Slim;

            // gather files from this module
            var copyLocalFiles = ProcessCopyLocalDependency(toplevelModule, dependency, parent, isCopyLocalDependency, topIsCopyLocalOverride, ref copyLocalModuleStack);

			// gather files for this module dependencies if appropriate
			bool keepTraversing = (isCopyLocalDependency || topIsCopyLocalOverride || isModuleCopyLocal);
			if (keepTraversing)
			{
				// as we recurse keep tracking the copy modules in the chain
				if (isModuleCopyLocal)
				{
					copyLocalModuleStack.Push(dependencyModule);
				}

				foreach (Dependency<IModule> subDependency in dependencyModule.Dependents.OrderBy(x => (x.Dependent as ProcessableModule).GraphOrder))
				{
					if (!dependencyStack.Contains(subDependency))
					{
						dependencyStack.Push(subDependency); // tracking dependency chain rather than modules:
															 // if A => B => C => D => B we want to evaluate A => B and D => B dependencies
						Dictionary<string, CachedCopyLocalInfo> copyLocalsFromDependency = GetCacheCopyLocalFiles(toplevelModule, dependencyModule, subDependency, copyLocalModuleStack, dependencyStack);
                        if (copyLocalsFromDependency != null)
                        {
                            foreach (KeyValuePair<string, CachedCopyLocalInfo> it in copyLocalsFromDependency)
                            {
                                AddTo(ref copyLocalFiles, it.Value, toplevelModule);
                            }
                        }
						dependencyStack.Pop();
					}
				}

				// mirrors push from above
				if (isModuleCopyLocal)
				{
					copyLocalModuleStack.Pop();
				}
			}

			return copyLocalFiles;
		}

		// gather files that the dependency between parent and dependency.Dependent should transtively copy local to topLevelModule
		private Dictionary<string, CachedCopyLocalInfo> ProcessCopyLocalDependency(IModule topLevelModule, Dependency<IModule> dependency, IModule parent, bool isCopyLocalDependency, bool isCopyLocalOverride, ref Stack<IModule> copyLocalModuleStack)
		{
			IModule sourceModule = dependency.Dependent;

			Dictionary<string, CachedCopyLocalInfo> copyLocalFiles = new Dictionary<string, CachedCopyLocalInfo>();
			foreach (CopyLocalFileSet copyFileSet in topLevelModule.CopyLocalDelegate(sourceModule, parent))
			{
				foreach (FileItem item in copyFileSet.FileSet.FileItems)
				{
					// TODO item copy local
					// -dvaliant 2016/04/04
					/* individual file items can mark themselves for copy local either through the copylocal option or via having the optionset
					"copylocal" this is semi useful for force disabling copy local on a file (via pption copylocal = off/false) but using it to
					force copy local on only works if this algorthim find the fileset - in wich case we were considering it for copy local anyway.
					Having this available indicates that user can force copy local on a file item when really it depends on build graph - we should
					replace the option with something like never-copy-local = on and deprecate copylocal option. */
					CopyLocalType itemCopyLocal = item.GetCopyLocal(parent);
					bool fileItemExplicitCopyLocalTrue = itemCopyLocal == CopyLocalType.True || itemCopyLocal == CopyLocalType.Slim;
					bool fileItemExplicitCopyLocalFalse = itemCopyLocal == CopyLocalType.False;

					// determine if we need to copy
					if (item.AsIs)
					{
						// regardless of any other factors we never expect copy local asis include as we expect these to be system files that will always be found implicitly
						topLevelModule.Package.Project.Log.Info.WriteLine("Module {0} NOT copying {1} to module {2} as file is marked as 'asis'.", sourceModule.Key, item.Path.Path, topLevelModule.Key);
						continue;
					}
					else if (fileItemExplicitCopyLocalFalse)
					{
						topLevelModule.Package.Project.Log.Info.WriteLine("Module {0} NOT copying {1} to module {2} as file optionset explicit sets copylocal to false.", sourceModule.Key, item.Path.Path, topLevelModule.Key);
						continue;
					}
					else if (isCopyLocalDependency)
					{
						topLevelModule.Package.Project.Log.Info.WriteLine("Module {0} copying {1} to module {2} as there is a transitive copy local dependency through {3}.", sourceModule.Key, item.Path.Path, topLevelModule.Key, parent.Key);
					}
					else if (isCopyLocalOverride)
					{
						topLevelModule.Package.Project.Log.Info.WriteLine("Module {0} copying {1} to module {2} as module {3} in dependency chain is a copy local module.", sourceModule.Key, item.Path.Path, topLevelModule.Key, copyLocalModuleStack.Peek());
					}
					else if (fileItemExplicitCopyLocalTrue)
					{
						// check this last as module public data filesets have this set during public data processing, but really as a function
						// of above situations, we only want to report this if this is the cause and not the above reasons (isCopyLocalDependency, isCopyLocalOverride)
						topLevelModule.Package.Project.Log.Info.WriteLine("Module {0} copying {1} to module {2} as file optionset explicit sets copylocal to true.", sourceModule.Key, item.Path.Path, topLevelModule.Key);
					}
					else
					{
						// should be the most common case, use debug verbosity level
						topLevelModule.Package.Project.Log.Debug.WriteLine("Module {0} NOT copying {1} to module {2} as there is no copy local dependency.", sourceModule.Key, item.Path.Path, topLevelModule.Key);
						continue;
					}

					string sourceAbsolutePath = Path.GetFullPath(item.FileName);
					string destinationAbsolutePath;
					bool copyRelativePath = !String.IsNullOrEmpty(item.OptionSetName)
							&& NAnt.Core.Functions.OptionSetFunctions.OptionSetGetValue(topLevelModule.Package.Project, item.OptionSetName, "preserve-basedir") == "on"
							&& !String.IsNullOrEmpty(item.BaseDirectory);
					if (copyFileSet.KeepRelativePath || copyRelativePath)
					{
						// relative dir is only preserved if basedir contains the item
						string baseDir = PathNormalizer.Normalize(item.BaseDirectory ?? copyFileSet.FileSet.BaseDirectory);
						if (baseDir != null && item.FileName.StartsWith(baseDir))
						{
							destinationAbsolutePath = PathUtil.RelativePath(item.FileName, baseDir);
						}
						else
						{
							destinationAbsolutePath = Path.GetFileName(item.FileName);
						}
					}
					else
					{
						destinationAbsolutePath = Path.GetFileName(item.FileName);
					}

					CachedCopyLocalInfo newCopy = new CachedCopyLocalInfo
					(
						sourceAbsolutePath,
						destinationAbsolutePath,
						copyFileSet.SkipUnchanged,
						copyFileSet.NonCritical,
						copyFileSet.AllowLinking,
						sourceModule
					);
					AddTo(ref copyLocalFiles, newCopy, topLevelModule);
				}
			}

			return copyLocalFiles;
		}

		private Dictionary<string, CachedCopyLocalInfo> GetCacheCopyLocalFiles(IModule toplevelModule, IModule targetModule, Dependency<IModule> dependencyKey, Stack<IModule> copyLocalModuleStack, Stack<Dependency<IModule>> dependencyStack)
		{
			// select cache based on override case
			CopyLocalDelegateCache delegateCacheToUse = copyLocalModuleStack.Any() ? _copyLocalOverrideCache : _copyLocalRegularCache;

			var cacheToUse = delegateCacheToUse.GetOrAdd(toplevelModule.CopyLocalDelegate, k =>
			{
				return new ConcurrentDictionary<Dependency<IModule>, Dictionary<string, CachedCopyLocalInfo>>(new DependencyComparer());
			});

            bool isCopyLocalOverrideChain = copyLocalModuleStack.Any();
            return cacheToUse.GetOrAdd(dependencyKey, k =>
			{
				// We need to start from clear stacks everytime otherwise the cached result might end up with different content (it is unfortunate that we might duplicate some stuff but whatever
				//var ds = new Stack<Dependency<IModule>>();

				return ProcessCopyLocalDependentsRecursive(toplevelModule, dependencyKey, targetModule, ref copyLocalModuleStack, ref dependencyStack);
			});
		}
	}
}
