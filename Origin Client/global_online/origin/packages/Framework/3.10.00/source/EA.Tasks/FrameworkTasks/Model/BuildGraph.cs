using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;

namespace EA.FrameworkTasks.Model
{
    public sealed class BuildGraph
    {
        internal static bool WasReset = false;

        private IEnumerable<IModule> _sortedActiveModules;
        private IEnumerable<IModule> _topModules;
        private HashSet<string> _configurationNames = new HashSet<string>();
        private HashSet<BuildGroups> _buildGroups = new HashSet<BuildGroups>();


        public readonly ConcurrentDictionary<string, IPackage> Packages = new ConcurrentDictionary<string, IPackage>();
        public readonly BlockingConcurrentDictionary<string, IModule> Modules = new BlockingConcurrentDictionary<string, IModule>();
        public readonly PackageAutoBuilCleanMap PackageAutoBuilCleanMap = new PackageAutoBuilCleanMap();

        private ConcurrentDictionary<string, PackageDependentsProcessingStatus> _packageDependentsProcessingStatus = new ConcurrentDictionary<string, PackageDependentsProcessingStatus>();

        public IEnumerable<IModule> TopModules 
        {
            get 
            {
                // Combune top modules across all configurations:
                return _topModules??new List<IModule>();
            }
        }

        public IEnumerable<IModule> SortedActiveModules
        {
            get
            {
                return _sortedActiveModules ??new List<IModule>();
            }
        }

        public IEnumerable<string> ConfigurationNames
        {
            get
            {
                // Combune top modules across all configurations:
                return _configurationNames;
            }
        }

        public IEnumerable<BuildGroups> BuildGroups
        {
            get
            {
                // Combune top modules across all configurations:
                return _buildGroups;
            }
        }


        public void SetTopModules(IEnumerable<IModule> topmodules)
        {
            _topModules = topmodules;
        }

        public void SetSortedActiveModules(IEnumerable<IModule> activemodules)
        {
            _sortedActiveModules = activemodules;
        }

        public void SetConfigurationNames(HashSet<string> configNames)
        {
            _configurationNames = new HashSet<string>(configNames);
        }

        public void SetBuildGroups(HashSet<BuildGroups> buildGroups)
        {
            _buildGroups = new HashSet<BuildGroups>(buildGroups);
        }

        public bool IsBuildGraphComplete
        {
            get
            {
                return (_topModules != null) && (_sortedActiveModules != null);
            }
        }

        public bool IsEmpty
        {
            get
            {
                return !IsBuildGraphComplete && _configurationNames.Count== 0 && _buildGroups.Count == 0;
            }
        }


        public bool CanReuse(IEnumerable<string> configNames, IEnumerable<BuildGroups> buildGroups, Log log, string prefix)
        {
            bool ret = true;

            if (_topModules != null || _sortedActiveModules != null)
            {
                if (!(_configurationNames.IsSupersetOf(configNames) && _buildGroups.IsSupersetOf(buildGroups)))
                {
                    ret = false;
                }
                if (log != null && log.Level >= Log.LogLevel.Detailed)
                {
                    log.WriteLine((prefix ?? String.Empty) + "Build Graph exists{0}", ret ? " and can be reused." : ", but it can not be reused.");
                    log.WriteLine((prefix ?? String.Empty) + "Build Graph groups {0}, configurations: {1}", _buildGroups.ToString(" ").Quote(), _configurationNames.ToString(" ").Quote());
                    log.WriteLine((prefix ?? String.Empty) + "Required groups {0}, configurations: {1}", buildGroups.ToString(" ").Quote(), configNames.ToString(" ").Quote());
                }
            }

            return ret;
        }


        internal PackageDependentsProcessingStatus GetPackageDependentProcessingStatus(IPackage package)
        {
            return _packageDependentsProcessingStatus.GetOrAdd(package.Key, new PackageDependentsProcessingStatus(package));
        }

        internal void CleanLoadPackageTempObjects()
        {
            _packageDependentsProcessingStatus = null;
        }
        internal class PackageDependentsProcessingStatus
        {
            private object SyncRoot = new object();
            private readonly IPackage Package;
            internal readonly HashSet<string> ProcessedModules;
            private HashSet<string> ModulesToProcess;
            internal Project.TargetStyleType CurrentTargetStyle;
            internal string AutobuildTarget;
            private bool ProcessAllModules;
            private bool loadComplete;
         //   private bool processedAllModules = false;

            private Func<IPackage,IEnumerable<string>, IEnumerable<string>> ModuledependenciesResolver;

            internal PackageDependentsProcessingStatus(IPackage package)
            {
                Package = package;
               
                ProcessedModules = new HashSet<string>();
                ModulesToProcess = new HashSet<string>();
                ProcessAllModules = false;
                loadComplete = false;
            }

            internal IEnumerable<string> GetUnProcessedModules(IEnumerable<string> processedModules, Project.TargetStyleType currentTargetStyle, string autobuildTarget, Func<IPackage, IEnumerable<string>, IEnumerable<string>> moduledependenciesResolver)
            {
                List<string> unprocessed = null;
                lock (SyncRoot)
                {
                    loadComplete = true;
                    CurrentTargetStyle = currentTargetStyle;
                    AutobuildTarget = autobuildTarget;
                    ModuledependenciesResolver = moduledependenciesResolver;


                    ProcessedModules.UnionWith(processedModules);

                    unprocessed = new List<string>();
                    if (ProcessAllModules)
                    {
                        foreach (var mod in Package.Modules)
                        {
                            var key = mod.BuildGroup + ":" + mod.Name;
                            if (ProcessedModules.Add(key))
                            {
                                unprocessed.Add(key);
                            }
                        }
                    }
                    else
                    {
                        var modulestoprocesswithdep = ModuledependenciesResolver(Package, ModulesToProcess);
                        foreach (var key in modulestoprocesswithdep)
                        {
                            if (ProcessedModules.Add(key))
                            {
                                unprocessed.Add(key);
                            }
                        }
                    }

                    ModulesToProcess = new HashSet<string>();
                }
                return unprocessed;
            }

            internal IEnumerable<string> SetUnProcessedModules(IEnumerable<string> modulestoprocess)
            {
                List<string> processlocally = null;
                lock (SyncRoot)
                {
                    if (modulestoprocess != null)
                    {

                        if (loadComplete)
                        {
                            processlocally = new List<string>();

                            foreach (var key in ModuledependenciesResolver(Package, modulestoprocess))
                            {
                                if (ProcessedModules.Add(key))
                                {
                                    processlocally.Add(key);
                                }
                            }
                        }
                        else
                        {
                            ModulesToProcess.UnionWith(modulestoprocess);
                        }
                    }
                    else
                    {
                        if (loadComplete)
                        {
                            processlocally = new List<string>();
                            foreach (var mod in Package.Modules)
                            {
                                var key = mod.BuildGroup + ":" + mod.Name;

                                if (ProcessedModules.Add(key))
                                {
                                    processlocally.Add(key);
                                }
                            }
                        }
                        else
                        {
                            ProcessAllModules = true;
                        }
                    }
                }
                return processlocally;
            }

        }
    }
}
