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
using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;
using PackageMap = NAnt.Core.PackageCore.PackageMap;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal class FileFilters
	{
		internal readonly List<FilterEntry> Roots;

		private static HashSet<string> packageDirs = new HashSet<string>();

		internal static void Init(Project project)
		{
			lock (packageDirs)
			{
				foreach (var p in project.BuildGraph().Packages)
				{
					var path = p.Value.Dir.Path;
					if (!String.Equals(Path.GetFileName(p.Value.Dir.Path), p.Value.Name, StringComparison.OrdinalIgnoreCase))
					{
						path = Path.GetDirectoryName(path);
					}
					packageDirs.Add(path);
				}
			}
		}

		internal class FilterEntry
		{
			internal FilterEntry(string key, string name, bool iscustom)
			{
				FilterKey = key;
				FilterName = name;
				IsCustomFilter = iscustom;
				Files = new List<FileEntry>();
				Filters = new List<FilterEntry>();
			}
			internal readonly bool IsCustomFilter;
			internal readonly string FilterKey; // Full path to the current filter

			internal string FilterName // Filter name
			{ get; private set; }

			internal string FilterPath // Path starting with root filter.
			{ get; private set; }

			internal readonly List<FileEntry> Files;
			internal readonly List<FilterEntry> Filters;

			internal void SetFilterName(string name)
			{
				FilterName = name;
			}

			internal void SetPath(string parentPath = null)
			{
				FilterPath = String.IsNullOrEmpty(parentPath) ? FilterName : parentPath + "\\" + FilterName;
				foreach (var filter in Filters)
				{
					filter.SetPath(FilterPath);
				}
			}
		}


		private FileFilters() 
		{
			Roots = new List<FilterEntry>();
		}

		public void ForEach(Action<FilterEntry> open, Action<FilterEntry> close=null)
		{
			foreach (var root in Roots.OrderBy(f=>f.FilterName))
			{
				TraverseTree(root, open, close);
			}
		}

		public void ForEach(Action<FilterEntry, FilterEntry> open, Action<FilterEntry, FilterEntry> close = null)
		{
			foreach (var root in Roots.OrderBy(f => f.FilterName))
			{
				TraverseTree(null, root, open, close);
			}
		}

		public IEnumerable<FilterEntry> FilterEntries
		{
			get
			{
				foreach (var root in Roots.OrderBy(f => f.FilterName))
					foreach (var x in TreeFilterEntryEnnumeration(root))
						yield return x;
			}
		}

		private IEnumerable<FilterEntry> TreeFilterEntryEnnumeration(FilterEntry entry)
		{
			// yield our filter-entry
			yield return entry;

			// recursively yield all child filter-entries
			foreach (FilterEntry child in entry.Filters.OrderBy(f => f.FilterName))
				foreach (var x in TreeFilterEntryEnnumeration(child))
					yield return x;
		}

		private void TraverseTree(FilterEntry entry, Action<FilterEntry> open, Action<FilterEntry> close)
		{
			open(entry);

			foreach (var child in entry.Filters.OrderBy(f => f.FilterName))
			{
				TraverseTree(child, open, close);
			}

			if (close != null)
			{
				close(entry);
			}
		}

		private void TraverseTree(FilterEntry parent, FilterEntry entry, Action<FilterEntry, FilterEntry> open, Action<FilterEntry, FilterEntry> close)
		{
			open(parent, entry);

			foreach (var child in entry.Filters.OrderBy(f => f.FilterName))
			{
				TraverseTree(entry, child, open, close);
			}

			if (close != null)
			{
				close(parent, entry);
			}
		}

		internal static FileFilters ComputeFilters(IOrderedEnumerable<FileEntry> files, IEnumerable<Configuration> allConfigurations, IEnumerable<IModule> modules)
		{
			var packages = modules.Select(m => m.Package);
			var packageDir = packages.FirstOrDefault().Dir;
			var moduleBaseDir = packageDir;

			var scriptFile = modules.FirstOrDefault().ScriptFile;
			if (!scriptFile.IsNullOrEmpty() && scriptFile.IsPathInDirectory(packageDir))
			{
				moduleBaseDir = PathString.MakeNormalized(Path.GetDirectoryName(scriptFile.Path));
			}

			string generatedFilterName;
			string bulkbuildFilterName;
			bool preserveFolderStructure;

			var customFilters = new List<FileFilterDef>();

			SetCustomFilters(modules.FirstOrDefault(), packages, customFilters, out generatedFilterName, out bulkbuildFilterName, out preserveFolderStructure);

			customFilters.AddRange(packages.Select(p => new FileFilterDef(p.PackageConfigBuildDir, (p.PackageConfigBuildDir == p.PackageBuildDir) ? generatedFilterName : generatedFilterName + p.ConfigurationName)).Distinct(new FileFilterDefEqualityComparer()));
			if (!PathUtil.IsPathInDirectory(packageDir.Path, PackageMap.Instance.BuildRoot))
			{
				// Some setups have package roots inside build root. In this case we do not want to add this filter. 
				// Otherwise package source files will end up in _gen_ folder.
				customFilters.Add(new FileFilterDef(PathString.MakeNormalized(PackageMap.Instance.BuildRoot), generatedFilterName));
			}
			
			customFilters.Add(new FileFilterDef(moduleBaseDir, String.Empty));

			var customFiltersMap = new Dictionary<string, FileFilterDef>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

			customFilters.ForEach((fd) => { if (!customFiltersMap.ContainsKey(fd.Root.Path)) customFiltersMap.Add(fd.Root.Path, fd); });

			var commonroots = new CommonRoots<FileEntry>(files, fe=>
				{
					// Bulkbuild files are processed separately below.
					if (!fe.IsKindOf(fe.ConfigEntries.FirstOrDefault().Configuration, FileEntry.BulkBuild))
					{
						string path = fe.Path.Path;

						string dirPath = Path.GetDirectoryName(path);
						if (dirPath.IndexOf(Path.DirectorySeparatorChar) != dirPath.LastIndexOf(Path.DirectorySeparatorChar)
							&& !dirPath.Equals(moduleBaseDir.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
						{
							return dirPath;
						}
						return path;
					}
					
					return null;
				},
				customFilters.Select(fd => fd.Root.Path),
				usePrefixesAsRoot: preserveFolderStructure);

			var filtermap = new Dictionary<string,FilterEntry>(StringComparer.OrdinalIgnoreCase);

			var filters = new FileFilters();

			FilterEntry bulkBuildFilterRootTop = null;
			FilterEntry bulkBuildFilterRootBottom = null;

			foreach (var folder in bulkbuildFilterName.ToArray("/\\"))
			{
				var newFilter = new FilterEntry(bulkBuildFilterRootBottom == null ? folder : bulkBuildFilterRootBottom.FilterKey + "/" + folder, folder, true);

				if (bulkBuildFilterRootBottom != null)
				{
					bulkBuildFilterRootBottom.Filters.Add(newFilter);
				}
				else
				{
					bulkBuildFilterRootTop = newFilter;
				}
				bulkBuildFilterRootBottom = newFilter;
			}
			
			int allConfigCount = allConfigurations.Count();

			foreach (var file in files)
			{
				int configEntriesCount = file.ConfigEntries.Count();

				var testBBentry = file.ConfigEntries.FirstOrDefault();

				if (testBBentry != null && testBBentry.IsKindOf(FileEntry.BulkBuild))
				{
					var bulkBuildFilter = bulkBuildFilterRootBottom;

					if (configEntriesCount == 1 && allConfigCount > 1)
					{
						// Create config specific bulk build filter
						bulkBuildFilter = bulkBuildFilterRootBottom.Filters.Where(f => f.FilterName == testBBentry.Configuration.Name).SingleOrDefault();
						if(bulkBuildFilter == null)
						{
							bulkBuildFilter = new FilterEntry(bulkBuildFilterRootBottom.FilterKey + "/" + testBBentry.Configuration.Name, testBBentry.Configuration.Name, true);
							bulkBuildFilterRootBottom.Filters.Add(bulkBuildFilter);
						}
					}
					bulkBuildFilter.Files.Add(file);
				}
				else
				{
					string key;
					string path;
					bool iscustom;

					GetKeyAndPath(file.Path.Path, commonroots, customFiltersMap, out key, out path, out iscustom);

					FilterEntry filter;
					if (filtermap.TryGetValue(key, out filter))
					{
						filter.Files.Add(file);
					}
					else
					{
						AddNewFilter(file, key, path, filters, filtermap, iscustom);
					}
				}
			}

			if (bulkBuildFilterRootBottom.Files.Count > 0 || bulkBuildFilterRootBottom.Filters.Count > 0)
			{
				filters.Roots.Insert(0, bulkBuildFilterRootTop);
			}

			// Recompute actual roots within each subtree;
			if (!preserveFolderStructure)
			{
				AdjustRoots(filters);
			}

			//Check for duplicates:
			ResolveDuplicates(filters.Roots, packageDir);

			// Set final internal path to the root //VS2010 uses it.
			foreach (var root in filters.Roots)
			{
				root.SetPath();
			}

			return filters;
		}

		private static void SetCustomFilters(IModule module, IEnumerable<IPackage> packages, List<FileFilterDef> customFilters, out string generatedFilterName, out string bulkbuildFilterName, out bool preserveFolderStructure)
		{
			const string DEFAULT_BULK_BUILD_FILTER_NAME = "__BulkBuild__";
			const string DEFAULT_GENERATED_FILTER_NAME = "_gen_";

			var filersOptionSetModuleName = module.PropGroupName("vs-file-folders");
			var filersOptionSetName = "generator.vs-file-folders";

			var generatedFilterModulePropertyName = module.PropGroupName("vs-generated-folder");
			var bulkbuildFilterModulePropertyName = module.PropGroupName("vs-bulkbuild-folder");
			var preserveFoldersModulePropertyName = module.PropGroupName("vs-preserve-folder-structure");

			var generatedFilterPropertyName = "generator.vs-generated-folder";
			var bulkbuildFilterPropertyName = "generator.vs-bulkbuild-folder";
			var preserveFoldersPropertyName = "generator.vs-preserve-folder-structure";

			OptionSet filterData;
			generatedFilterName = null;
			bulkbuildFilterName = null;
			preserveFolderStructure = false;

			var duplicates = new HashSet<PathString>();

			foreach(var package in packages)
			{
				if(package.Project != null)
				{
					if (!package.Project.NamedOptionSets.TryGetValue(filersOptionSetModuleName, out filterData))
					{
						package.Project.NamedOptionSets.TryGetValue(filersOptionSetName, out filterData);
					}
					if (filterData != null)
					{
						foreach (var op in filterData.Options)
						{
							var path = PathString.MakeNormalized(package.Project.ExpandProperties(op.Key));
							if (duplicates.Add(path))
							{
								customFilters.Add(new FileFilterDef(path, package.Project.ExpandProperties(op.Value)));
							}
						}
					}

					if(String.IsNullOrEmpty(generatedFilterName))
					{
						generatedFilterName = package.Project.GetPropertyValue(generatedFilterModulePropertyName);
						 if(String.IsNullOrEmpty(generatedFilterName))
						 {
							 generatedFilterName = package.Project.GetPropertyValue(generatedFilterPropertyName);
						 }
					}

					if (String.IsNullOrEmpty(bulkbuildFilterName))
					{
						bulkbuildFilterName = package.Project.GetPropertyValue(bulkbuildFilterModulePropertyName);

						if (String.IsNullOrEmpty(bulkbuildFilterName))
						{
							bulkbuildFilterName = package.Project.GetPropertyValue(bulkbuildFilterPropertyName);
						}
					}

					if (package.Project.Properties.Contains(preserveFoldersModulePropertyName))
					{
						preserveFolderStructure = package.Project.Properties.GetBooleanProperty(preserveFoldersModulePropertyName);
					}
					else if (package.Project.Properties.Contains(preserveFoldersPropertyName))
					{
						preserveFolderStructure = package.Project.Properties.GetBooleanProperty(preserveFoldersPropertyName);
					}
				}
			}

			if(String.IsNullOrEmpty(generatedFilterName))
			{
				generatedFilterName = DEFAULT_GENERATED_FILTER_NAME;
			}
			// Make sure that we're using consistent path separator throughout the code.
			generatedFilterName = generatedFilterName.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);

			if (String.IsNullOrEmpty(bulkbuildFilterName))
			{
				bulkbuildFilterName = DEFAULT_BULK_BUILD_FILTER_NAME;
			}
			// Make sure that we're using consistent path separator throughout the code.
			bulkbuildFilterName = bulkbuildFilterName.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
		}

		private static void AdjustRoots(FileFilters filters)
		{
			var newRoots = new List<FilterEntry>();
			foreach (var root in filters.Roots)
			{
				var newroot = root;

				while (newroot != null && newroot.Files.Count == 0 && newroot.Filters.Count < 2)
				{
					if (!newroot.IsCustomFilter)
					{
						if (packageDirs.Contains(newroot.FilterKey))
						{
							break;
						}
					}
					newroot = newroot.Filters.FirstOrDefault();
				}
				if(newroot != null)
				{
					if (newroot.IsCustomFilter && !Object.ReferenceEquals(root, newroot))
					{
						root.Filters.Clear();
						root.Filters.Add(newroot);
						newRoots.Add(root);
					}
					else
					{
						if (newroot.Files.Count == 0)
						{
							//Root is outside package or generated. For all children go down until multiple filters or package dir found.
							foreach (var child in newroot.Filters)
							{
								var newChild = child;
								while (newChild != null && newChild.Files.Count == 0 && newChild.Filters.Count < 2 && !packageDirs.Contains(newChild.FilterKey))
								{
									newChild = newChild.Filters.FirstOrDefault();
								}
								if (newChild != null && !Object.ReferenceEquals(child, newChild))
								{
									child.Filters.Clear();
									if (newChild.Files.Count > 0 || packageDirs.Contains(newChild.FilterKey))
									{
										child.Filters.Add(newChild);
									}
									else
									{
										child.Filters.AddRange(newChild.Filters);
									}

								}
							}
						}
						newRoots.Add(newroot);
					}
				}
			}
			filters.Roots.Clear();
			filters.Roots.AddRange(newRoots);
		}

		private static void ResolveDuplicates(List<FilterEntry> filters, PathString packageDir)
		{
			var filterstocheck = new List<FilterEntry>();
			foreach (var filter in filters)
			{
				if (String.IsNullOrEmpty(filter.FilterName))
				{
					filterstocheck.AddRange(filter.Filters);
				}
				else
				{
					filterstocheck.Add(filter);
				}
			}

			var packageDrive = PathUtil.GetDriveLetter(packageDir.Path);
			// Check for duplicate names:
			foreach (var group in filterstocheck.GroupBy(f => f.FilterName.ToLowerInvariant(), f => f))
			{
				if (group.Count() > 1)
				{
					var drives = group.Where(f => f.FilterKey != f.FilterName).GroupBy(f => (f.FilterKey != f.FilterName) ? PathUtil.GetDriveLetter(f.FilterKey) : String.Empty, f => f);

					var driveCount = drives.Count();

					foreach (var drive in drives)
					{
						int count = 0;
						foreach (var filter in drive)
						{
							//If files are on different drives, add drive letter
							if (packageDrive != drive.Key && driveCount > 1)
							{
								filter.SetFilterName(String.Format("{0}_DRIVE_{1}", drive.Key, filter.FilterName));
							}

							if (count > 0)
							{
								filter.SetFilterName(String.Format("{0}_{1:00}", filter.FilterName, count++));
							}

							count++;
						}
					}
				}
			}
			
		}


		private static void AddNewFilter(FileEntry file, string key, string path, FileFilters filters, Dictionary<string, FilterEntry> filtermap, bool iscustom)
		{
			var filter = CreateFilter(file, key, path, filtermap, iscustom);

			bool done = false;
			while (!done)
			{
				if (String.IsNullOrEmpty(path))
				{
					filters.Roots.Add(filter);
					done = true;
				}
				else
				{
					key = Path.GetDirectoryName(key);
					path = Path.GetDirectoryName(path);

					FilterEntry parentfilter;
					if (!String.IsNullOrEmpty(key))
					{
						if (filtermap.TryGetValue(key, out parentfilter))
						{
							parentfilter.Filters.Add(filter);
							done = true;
						}
						else
						{
							parentfilter = CreateFilter(null, key, path, filtermap, iscustom);
							parentfilter.Filters.Add(filter);
							filter = parentfilter;
						}
					}
				}

			}
		}

		private static FilterEntry CreateFilter(FileEntry file, string key, string path, Dictionary<string, FilterEntry> filtermap, bool iscustom)
		{
			string name = Path.GetFileName(path);  // Filter name;
			var filter = new FilterEntry(key, name, iscustom);
			filtermap.Add(filter.FilterKey, filter);
			if (file != null)
			{
				filter.Files.Add(file);
			}
			return filter;
		}

		private static void GetKeyAndPath(string filepath, CommonRoots<FileEntry> commonroots, Dictionary<string, FileFilterDef> customFiltersMap, out string key, out string path, out bool iscustom)
		{
			iscustom = false; 
			// Throughout the code, we have been using result of Path.GetDirectoryName() to generate the key for commonroots.Roots and
			// Path.GetDirectoryName() will return a reformatted path with consistent separator usage in the entire path.  So we need to 
			// make sure that the input filepath has a proper format that is expected.
			string filepath_with_consistent_separator = filepath.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
			string prefix = commonroots.GetPrefix(filepath_with_consistent_separator);
			string root = commonroots.Roots[prefix];

			key = Path.GetDirectoryName(filepath);

			// The "path" variable contain the "key" value with the "root" already stripped out!
			path = GetPath(root, key);

			FileFilterDef def;
			if (customFiltersMap.TryGetValue(prefix, out def))
			{
				iscustom = true;

				if (!String.IsNullOrEmpty(def.Path))
				{
					path = Path.Combine(def.Path, path);
					key = path;
				}
			}
		}

		private static string GetPath(string root, string key)
		{
			string path = key.Substring(Math.Min(root.Length, key.Length));
			return path;
		}

	}
}
