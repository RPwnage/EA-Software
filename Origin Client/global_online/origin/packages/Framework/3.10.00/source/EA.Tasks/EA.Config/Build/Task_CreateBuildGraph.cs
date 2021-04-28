using System;
using System.Xml;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading;
using System.Text;
using System.IO;
using Threading = System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Threading;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;

using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Build
{

    [TaskName("create-build-graph")]
    public class Task_CreateBuildGraph : Task
    {
        private readonly NAnt.Core.PackageCore.PackageAutoBuilCleanMap ModuleProcessMap = new NAnt.Core.PackageCore.PackageAutoBuilCleanMap();

        [TaskAttribute("build-group-names", Required = true)]
        public string BuildGroupNames
        {
            get { return _buildgroupNames; }
            set { _buildgroupNames = value; }
        }
        private string _buildgroupNames;

        [TaskAttribute("build-configurations", Required = true)]
        public string BuildConfigurations
        {
            get { return _buildConfigurations; }
            set { _buildConfigurations = value; }
        }
        private string _buildConfigurations;


        [TaskAttribute("process-generation-data", Required = false)]
        public bool ProcessGenerationData
        {
            get { return _processGenerationData; }
            set { _processGenerationData = value; }
        }
        private bool _processGenerationData = false;

        protected BuildGraph BuildGraph
        {
            get { return _buildGraph; }
        }

        protected override void ExecuteTask()
        {
            var timer = new Chrono();

#if NANT_CONCURRENT

            bool parallel = (PlatformUtil.IsMonoRuntime == false) && (Project.NoParallelNant == false);
#else
            bool parallel = false;
#endif

            _buildGraph = Project.BuildGraph();


            var configs = new HashSet<string>(BuildConfigurations.ToArray());
            var buildgroups = new HashSet<BuildGroups>(BuildGroupNames.ToArray().Select(gn => ConvertUtil.ToEnum<BuildGroups>(gn)));

            if (BuildGraph.IsBuildGraphComplete)
            {
                    Log.Info.WriteLine(LogPrefix + "[Packages {0}, modules {1}, active modules {2}] - build graph already completed.", BuildGraph.Packages.Count, BuildGraph.Modules.Count, BuildGraph.SortedActiveModules.Count());
                    return;
            }

            _buildGraph.SetConfigurationNames(configs);
            _buildGraph.SetBuildGroups(buildgroups);

            var topmodules = GetTopModules(Project, configs, buildgroups);

            foreach (var package in topmodules.Select(m => m.Package))
            {
                if(package.Project.Properties.Contains("builgraph.global.preprocess"))
                {
                    Log.Warning.WriteLine("Correct property name from builgraph.global.preprocess to buildgraph.global.preprocess");
                    break;
                }
            }
            ExecuteGlobalProcessSteps(topmodules, "builgraph.global.preprocess");
            ExecuteGlobalProcessSteps(topmodules, "buildgraph.global.preprocess");

            var activemodules = GetActiveModules(topmodules);

            var sortedActivemodules = SortActiveModules(activemodules);

            var propagatedTransitive = new ConcurrentDictionary<string, DependentCollection>();

            try
            {
                ForEachModule.Execute(sortedActivemodules, 0, (module) =>
                {
                    //Tiburon sets module build type in the preprocess steps ("vcproj.prebuildtarget"). Verify and reset build type if nedded"
                    VerifyResetModuleBuildType(module);

                    module.SetModuleBuildProperties();

                    var buildOptionSet = OptionSetUtil.GetOptionSetOrFail(module.Package.Project, module.BuildType.Name);

                    ExecutePreProcessSteps(module, buildOptionSet);

                    using (var moduleDataProcessor = new ModuleProcessor_SetModuleData(module, LogPrefix))
                    {
#if FRAMEWORK_PARALLEL_TRANSITION
                                    moduleDataProcessor.ProcessGenerationData = ProcessGenerationData;
#endif
                        moduleDataProcessor.Process(module);
                    }

                    if (ProcessGenerationData)
                    {
                        using (var moduleAdditionalDataProcessor = new ModuleAdditionalDataProcessor(module))
                        {
                            moduleAdditionalDataProcessor.Process(module);
                        }

                    }
                },
                LogPrefix,
                useModuleWaitForDependents:false,
                parallelExecution:parallel);
                
                // Add dependent public data:
                ForEachModule.Execute(sortedActivemodules, DependencyTypes.Build, (module) =>
                {
                    var package = module.Package;

                    using (NAnt.Core.PackageCore.PackageAutoBuilCleanMap.PackageAutoBuilCleanState package_Verify = ModuleProcessMap.StartBuild(package.Key, "Verify Package Public Data" + package.ConfigurationName))
                    {
                        if (!package_Verify.IsDone())
                        {
                            ModuleProcessor_AddModuleDependentsPublicData.VerifyPackagePublicData(package as Package, LogPrefix);
                        }
                    }

                    var packagePropagatedTransitive = package.Project.Properties.GetBooleanProperty(Project.NANT_PROPERTY_TRANSITIVE) ? propagatedTransitive : null;

                    var buildOptionSet = OptionSetUtil.GetOptionSetOrFail(module.Package.Project, module.BuildType.Name);

                    using (var moduleDataProcessor = new ModuleProcessor_AddModuleDependentsPublicData(module, packagePropagatedTransitive, LogPrefix))
                    {
#if FRAMEWORK_PARALLEL_TRANSITION
                            moduleDataProcessor.ProcessGenerationData = ProcessGenerationData;
#endif
                        moduleDataProcessor.Process(module);
                    }

                    ExecutePostProcessSteps(module, buildOptionSet);
                    using (NAnt.Core.PackageCore.PackageAutoBuilCleanMap.PackageAutoBuilCleanState package_Postprocess = ModuleProcessMap.StartBuild(package.Key, "Execute Package Postprocess Step" + package.ConfigurationName))
                    {
                        if (!package_Postprocess.IsDone())
                        {
                            ExecutePackagePostProcessSteps(package);
                        }
                    }
                },
                LogPrefix,
                useModuleWaitForDependents: false,
                parallelExecution: parallel);

                // Add transitive dependencies to module:
                foreach (var module in activemodules.Where(m => !(m is Module_UseDependency)))
                {
                    DependentCollection propagated;
                    if (propagatedTransitive.TryGetValue(module.Key, out propagated))
                    {
                        module.Dependents.AddRange(propagated);
                    }
                }
            }
            catch (System.Exception e)
            {
                ThreadUtil.RethrowThreadingException(String.Format("Error in task '{0}'", Name), Location, e);
            }

            SetBuilgGraphActiveModules(topmodules, sortedActivemodules);

            foreach (var package in topmodules.Select(m => m.Package))
            {
                if (package.Project.Properties.Contains("builgraph.global.postprocess"))
                {
                    Log.Warning.WriteLine("Correct property name from builgraph.global.postprocess to buildgraph.global.postprocess");
                    break;
                }
            }
            ExecuteGlobalProcessSteps(topmodules, "builgraph.global.postprocess");
            ExecuteGlobalProcessSteps(topmodules, "buildgraph.global.postprocess");

            // Remove Module.ExcludedFromBuild. This flag can be set in user pre/post process tasks.
            RemoveExcludedFromBuildModules(ref topmodules, ref sortedActivemodules);

            sortedActivemodules = DetectCircularBuildDependencies(sortedActivemodules);

            SetBuilgGraphActiveModules(topmodules, sortedActivemodules);

            Log.Status.WriteLine(LogPrefix + "{0} [Packages {1}, modules {2}, active modules {3}]  {4}", BuildGraph.BuildGroups.ToString(" "), BuildGraph.Packages.Count, BuildGraph.Modules.Count, sortedActivemodules.Count(), timer.ToString());
        }

        private void ExecutePreProcessSteps(ProcessableModule module, OptionSet buildOptionSet)
        {
            var prefix = ".vcproj";
            if(module.IsKindOf(Module.CSharp))
            {
                prefix = ".csproj";
            }
            else if (module.IsKindOf(Module.FSharp))
            {
                prefix = ".csproj";
            }

            if (ProcessGenerationData)
            {
                // Workaround a bug in ANT ConfigureAdf task. if both runtime and runtime_spu modules are present, task attaches code generation step to the spu module only.
                // In some cases there is no dependency on spu module, and, as result code is not autogenerated.
                //
                // var targetnames = module.Package.Project.Properties[module.GroupName + prefix + ".prebuildtarget"] ?? module.GroupName + prefix + ".prebuildtarget";
                //
                var targetproperty = module.Package.Project.Properties[module.GroupName + prefix + ".prebuildtarget"];
                if (targetproperty == null && module.GroupName.EndsWith("_runtime", StringComparison.Ordinal))
                {
                    targetproperty = module.Package.Project.Properties[module.GroupName + "_spu" + prefix + ".prebuildtarget"];
                }
                var targetnames = targetproperty ?? module.GroupName + prefix + ".prebuildtarget";
                // ----- End workaround ----------------------------------------------------------------

                foreach(var targetname in targetnames.ToArray())
                {
                    module.Package.Project.ExecuteTargetIfExists(targetname, force: false);
                }
            }

            // Collect all preprocess definitions. It can be task or a target.
            string preprocess = buildOptionSet.Options["preprocess"] + Environment.NewLine + module.Package.Project.Properties["eaconfig.preprocess"] + Environment.NewLine +
                                 module.Package.Project.Properties[module.GroupName + ".preprocess"];

            BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
            if (stepsLog != null)
            {
                stepsLog.Info.WriteLine(LogPrefix + "{0}: executing '{1}' + '{2}' + '{3}' steps", module.ModuleIdentityString(), "preprocess", "eaconfig.preprocess", module.GroupName + ".preprocess");
            }

            module.Package.Project.ExecuteProcessSteps(module, buildOptionSet, preprocess.ToArray(), stepsLog);

        }


        private void ExecutePostProcessSteps(ProcessableModule module, OptionSet buildOptionSet)
        {
            // Collect all postprocess definitions. It can be task or a target.
            string postprocess = buildOptionSet.Options["postprocess"] + Environment.NewLine + module.Package.Project.Properties["eaconfig.postprocess"] + Environment.NewLine +
                                 module.Package.Project.Properties[module.GroupName + ".postprocess"];

            BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
            if (stepsLog != null)
            {
                stepsLog.Info.WriteLine(LogPrefix + "{0}: executing '{1}' + '{2}' + '{3}' steps", module.ModuleIdentityString(), "postprocess", "eaconfig.postprocess", module.GroupName + ".postprocess");
            }

            module.Package.Project.ExecuteProcessSteps(module, buildOptionSet, postprocess.ToArray(), stepsLog);
        }


        private void ExecutePackagePostProcessSteps(IPackage package)
        {

        }

        private void VerifyResetModuleBuildType(ProcessableModule module)
        {
            BuildType buildtype = module.Package.Project.CreateBuildType(module.GroupName);
            if (buildtype.Name != module.BuildType.Name)
            {
                string msg = String.Format("package {0}-{1} module {2} ({3}): buildtype changed from {4} to {5} in package/module preprocess steps(targets). ", module.Package.Name, module.Package.Version, module.GroupName, module.Package.ConfigurationName, module.BuildType.Name, buildtype.Name);

                if (buildtype.SubSystem != module.BuildType.SubSystem || buildtype.IsCLR != module.BuildType.IsCLR || buildtype.IsMakeStyle != module.BuildType.IsMakeStyle)
                {
                    throw new BuildException(msg + "Framework can not recover from this change. Make sure property '" + module.GroupName + ".buildtype' is fully defined when package build script is loaded.");
                }
                else
                {
                    module.Package.Project.Log.Warning.WriteLine(LogPrefix + msg + "Framework autorecovered, but it is advisable to make property '" + module.GroupName + ".buildtype' fully defined when package build script is loaded.");
                    module.BuildType = buildtype;
                }
            }
        }

        private IEnumerable<IModule> GetTopModules(Project project, HashSet<string> configNames, HashSet<BuildGroups> buildGroups)
        {
            string packagename = project.Properties["package.name"];
            string packageversion = project.Properties["package.version"];

            var toppackages = BuildGraph.Packages.Where(e => e.Value.Name == packagename && e.Value.Version == packageversion && configNames.Contains(e.Value.ConfigurationName)).Select(e => e.Value);

            if (toppackages.Count() == 0)
            {
                project.Log.Warning.WriteLine(LogPrefix+"Found no packages that match condition [name={0}, version={1}, configurations={2}]. Total packages processed: {3}",
                    packagename, packageversion, configNames.ToString(";"), BuildGraph.Packages.Count);
            }

            return toppackages.SelectMany(p => p.Modules.Where(m => buildGroups.Contains(m.BuildGroup)));
        }

        private IEnumerable<IModule> GetActiveModules(IEnumerable<IModule> topmodules)
        {
            var activeModules = new HashSet<IModule>(Module.EqualityComparer);

            var stack = new Stack<ProcessableModule>();

            foreach(var module in topmodules)
            {
                stack.Push(module as ProcessableModule);
                while (stack.Count > 0)
                {
                    var mod = stack.Pop();

                    if (activeModules.Add(mod))
                    {
                        if (!(mod is Module_UseDependency))
                        {
                            using (var moduleProcessor = new ModuleProcessor_ProcessDependencies(mod, LogPrefix))
                            {
                                moduleProcessor.Process(mod);
                            }
                            foreach (var dep in mod.Dependents)
                            {
                                stack.Push(dep.Dependent as ProcessableModule);
                            }
                        }
                    }
                }
            }

            return activeModules;
        }

        private IEnumerable<IModule> SortActiveModules(IEnumerable<IModule> unsortedModules)
        {
            // Create DAG of build modules:
            var unsortedKeys = new Dictionary<string, DAGNode<IModule>>();
            var unsorted = new List<DAGNode<IModule>>();

            foreach (var module in unsortedModules.OrderBy(m=>m.Key))
            {
                var newNode = new DAGNode<IModule>(module);
                unsortedKeys.Add(module.Key, newNode);
                unsorted.Add(newNode);
            }

            // Populate dependencies
            foreach (var dagNode in unsorted)
            {
                // Sort dependents so that we always have the same order after sorting the whole graph
                uint detendencyType = dagNode.Value.IsKindOf(Module.Program | Module.DynamicLibrary | Module.DotNet) ? (DependencyTypes.Build | DependencyTypes.Link) : DependencyTypes.Build;
                foreach (var dep in dagNode.Value.Dependents.Where(d => d.IsKindOf(detendencyType)).OrderBy(d => d.Dependent.Key))
                {
                     DAGNode<IModule> childDagNode;
                    if (unsortedKeys.TryGetValue(dep.Dependent.Key, out childDagNode))
                    {
                        dagNode.Dependencies.Add(childDagNode);
                    }
                    else
                    {
                        throw new BuildException("INTERNAL ERROR");
                    }
                }
            }

            var sorted = DAGNode<IModule>.Sort(unsorted, (a, b) => {});

            return SetGraphOrder(sorted);

        }

        private void ExecuteGlobalProcessSteps(IEnumerable<IModule> topmodules, string propertyName)
        {
            BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
            if (stepsLog != null)
            {
                stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' steps", propertyName);
            }

            var procesed = new HashSet<string>();

            foreach (var package in topmodules.Select(m => m.Package))
            {
                var steps = new HashSet<string>(package.Project.Properties[propertyName].ToArray());

                package.Project.ExecuteGlobalProjectSteps(steps.Except(procesed), stepsLog);

                procesed.UnionWith(steps);
            }
        }

        private IEnumerable<IModule> DetectCircularBuildDependencies(IEnumerable<IModule> unsortedModules)
        {
            // Create DAG of build modules:
            var unsortedKeys = new Dictionary<string, DAGNode<IModule>>();
            var unsorted = new List<DAGNode<IModule>>();

            foreach (var module in unsortedModules.OrderBy(m => m.Key))
            {
                var newNode = new DAGNode<IModule>(module);
                unsortedKeys.Add(module.Key, newNode);
                unsorted.Add(newNode);
            }

            // Populate dependencies
            var added = new List<DAGNode<IModule>>();
            foreach (var dagNode in unsorted)
            {
                // Sort dependents so that we always have the same order after sorting the whole graph
                foreach (var dep in dagNode.Value.Dependents.Where(d=>d.IsKindOf(DependencyTypes.Build)).OrderBy(d => d.Dependent.Key))
                {
                    DAGNode<IModule> childDagNode;
                    if (unsortedKeys.TryGetValue(dep.Dependent.Key, out childDagNode))
                    {
                        dagNode.Dependencies.Add(childDagNode);
                    }
                    else
                    {
                        Log.Status.WriteLine(LogPrefix+"Added new module: {0} ({1}) Module '{2}.{3}', through dependency from [{4}] {5}.{6} ({7})", dep.Dependent.Package.Name, dep.Dependent.Configuration.Name, dep.Dependent.BuildGroup.ToString(), dep.Dependent.Name, dagNode.Value.Package.Name, dagNode.Value.BuildGroup.ToString(), dagNode.Value.Name, dagNode.Value.Configuration.Name);
                        var newNode = new DAGNode<IModule>(dep.Dependent);
                        unsortedKeys.Add(dep.Dependent.Key, newNode);
                        dagNode.Dependencies.Add(newNode);
                        added.Add(newNode);
                    }
                }
            }
            unsorted.AddRange(added);

            var sorted = DAGNode<IModule>.Sort(unsorted, (a, b) =>
            {
                var msg = String.Format("Circular build dependency between modules {0} ({1}) and {2} ({3})", a.Value.Name, a.Value.Configuration.Name, b.Value.Name, b.Value.Configuration.Name);
                throw new BuildException(msg);
            });

            return SetGraphOrder(sorted);

        }

        private IEnumerable<IModule> SetGraphOrder(List<DAGNode<IModule>> sorted)
        {
            int count = 0;
            var orderedModules = new List<IModule>();
            foreach (var dagNode in sorted)
            {
                (dagNode.Value as ProcessableModule).GraphOrder = count++;
                orderedModules.Add(dagNode.Value);
            }

            return orderedModules;
        }

        private void RemoveExcludedFromBuildModules(ref IEnumerable<IModule> topmodules, ref IEnumerable<IModule> sortedActivemodules)
        {
            topmodules = topmodules.Where(m=> !m.IsKindOf(Module.ExcludedFromBuild));
            sortedActivemodules = sortedActivemodules.Where(m=> !m.IsKindOf(Module.ExcludedFromBuild));

            foreach(var mod in sortedActivemodules)
            {
                mod.Dependents.RemoveIf(dep => dep.Dependent.IsKindOf(Module.ExcludedFromBuild));
            }
        }


        private void SetBuilgGraphActiveModules(IEnumerable<IModule> topmodules, IEnumerable<IModule> activemodules)
        {
            BuildGraph.SetTopModules(topmodules.OrderBy(m => (m as ProcessableModule).GraphOrder));
            BuildGraph.SetSortedActiveModules(activemodules);
        }

        private BuildGraph _buildGraph;
    }
}
