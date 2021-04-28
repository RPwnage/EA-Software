using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
            foreach (var p in project.BuildGraph().Packages)
            {
                var path = p.Value.Dir.Path;
                if(!String.Equals(Path.GetFileName(p.Value.Dir.Path), p.Value.Name, StringComparison.OrdinalIgnoreCase))
                {
                    path = Path.GetDirectoryName(path);
                }
                packageDirs.Add(path);
            }
        }

        internal class FilterEntry
        {
            internal FilterEntry(string key, string name, bool iscustom)
            {
                FilterKey = key;
                _name = name;
                IsCustomFilter = iscustom;
                Files = new List<FileEntry>();
                Filters = new List<FilterEntry>();
            }
            internal readonly bool IsCustomFilter;
            internal readonly string FilterKey; // Full path to the current filter

            internal string FilterName // Filter name
            {
                get { return _name; }
            } private string _name;

            internal string FilterPath // Path starting with root filter.
            {
                get { return _path; }
            } private string _path;

            

            internal readonly List<FileEntry> Files;
            internal readonly List<FilterEntry> Filters;

            internal void SetFilterName(string name)
            {
                _name = name;
            }

            internal void SetPath(string parentPath = null)
            {
                _path = String.IsNullOrEmpty(parentPath) ? _name : parentPath + "\\" + _name;
                foreach (var filter in Filters)
                {
                    filter.SetPath(_path);
                }
            }
        }


        private FileFilters() 
        {
            Roots = new List<FilterEntry>();
        }

        public void ForEach(Action<FilterEntry> open, Action<FilterEntry> close=null)
        {
#if FRAMEWORK_PARALLEL_TRANSITION
            foreach (var root in Roots)
#else
            foreach (var root in Roots.OrderBy(f=>f.FilterName))
#endif
            {
                TraverseTree(root, open, close);
            }
        }

        public void ForEach(Action<FilterEntry, FilterEntry> open, Action<FilterEntry, FilterEntry> close = null)
        {
#if FRAMEWORK_PARALLEL_TRANSITION
            foreach (var root in Roots)
#else
            foreach (var root in Roots.OrderBy(f => f.FilterName))
#endif
            {
                TraverseTree(null, root, open, close);
            }
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
#if FRAMEWORK_PARALLEL_TRANSITION
            const string BULK_BUILD_FILTER_NAME = "BulkBuild";
#else
            const string BULK_BUILD_FILTER_NAME = "__BulkBuild__";
#endif
            var packages = modules.Select(m => m.Package);
            var packageDir = packages.FirstOrDefault().Dir;
            var moduleBaseDir = packageDir;

            var scriptFile = modules.FirstOrDefault().ScriptFile;
            if (scriptFile != null && !String.IsNullOrEmpty(scriptFile.Path))
            {
                moduleBaseDir = PathString.MakeNormalized(Path.GetDirectoryName(scriptFile.Path));
            }

#if FRAMEWORK_PARALLEL_TRANSITION
            var customFilters = new List<FileFilterDef>();
#else

            var customFilters = new List<FileFilterDef>(packages.Select(p => new FileFilterDef(p.PackageConfigBuildDir, (p.PackageConfigBuildDir == p.PackageBuildDir) ? "__Generated__" : "__Generated__/"+p.ConfigurationName)).Distinct(new FileFilterDefEqualityComparer()));
            
            if (!PathUtil.IsPathInDirectory(packageDir.Path, PackageMap.Instance.BuildRoot))
            {
                // Some setups have package roots inside build root. In this case we do not want to add this filter. 
                // Otherwise package source files will end up in __Generated__ folder.
                customFilters.Add(new FileFilterDef(PathString.MakeNormalized(PackageMap.Instance.BuildRoot), "__Generated__"));
            }
            customFilters.Add(new FileFilterDef(moduleBaseDir, String.Empty));
#endif
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
                customFilters.Select(fd => fd.Root.Path));

            var filtermap = new Dictionary<string,FilterEntry>(StringComparer.OrdinalIgnoreCase);

            var filters = new FileFilters();

            var bulkBuildFilterRoot = new FilterEntry(BULK_BUILD_FILTER_NAME, BULK_BUILD_FILTER_NAME, true);            
            
            int allConfigCount = allConfigurations.Count();

            foreach (var file in files)
            {
                int configEntriesCount = file.ConfigEntries.Count();

                var testBBentry = file.ConfigEntries.FirstOrDefault();

                if (testBBentry != null && testBBentry.IsKindOf(FileEntry.BulkBuild))
                {
                    var bulkBuildFilter = bulkBuildFilterRoot;

                    if (configEntriesCount == 1 && allConfigCount > 1)
                    {
                        // Create config specific bulk build filter
                        bulkBuildFilter = bulkBuildFilterRoot.Filters.Where(f=> f.FilterName == testBBentry.Configuration.Name).SingleOrDefault();
                        if(bulkBuildFilter == null)
                        {
                            bulkBuildFilter = new FilterEntry(BULK_BUILD_FILTER_NAME+"/"+testBBentry.Configuration.Name, testBBentry.Configuration.Name, true);
                            bulkBuildFilterRoot.Filters.Add(bulkBuildFilter);
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

            if (bulkBuildFilterRoot.Files.Count > 0 || bulkBuildFilterRoot.Filters.Count > 0)
            {
                filters.Roots.Insert(0, bulkBuildFilterRoot);
            }

            // Recompute actual roots within each subtree;
            AdjustRoots(filters);

            //Check for duplicates:
            ResolveDuplicates(filters.Roots, packageDir);

            // Set final internal path to the root //VS2010 uses it.
            foreach (var root in filters.Roots)
            {
                root.SetPath();
            }

            return filters;
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
                            //Root is outside package or generated. For all childs go down until multiple filters or package dir found.
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
            string prefix = commonroots.GetPrefix(filepath);
            string root = commonroots.Roots[prefix];

            key = Path.GetDirectoryName(filepath);

            path = GetPath(root, key);

            FileFilterDef def;
            if (customFiltersMap.TryGetValue(prefix, out def))
            {
                iscustom = true;

                if (!String.IsNullOrEmpty(def.Path))
                {

                    // Substitute root element
                    int path_root_ind = path.IndexOfAny(new char[] { '\\', '/' });
                    if (path_root_ind > 0 && path.Length > path_root_ind + 1)
                    {
                        path = Path.Combine(def.Path, path.Substring(path_root_ind + 1));
                    }
                    else
                    {
                        path = def.Path;
                    }
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
