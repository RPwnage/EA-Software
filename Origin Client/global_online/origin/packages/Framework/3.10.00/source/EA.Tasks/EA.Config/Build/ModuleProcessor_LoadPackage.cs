using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Threading;
using System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using NAnt.Core.Reflection;
using NAnt.Core.PackageCore;
using EA.FrameworkTasks.Model;
using NAnt.Shared.Properties;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Build
{
    internal class ModuleProcessor_LoadPackage : ModuleDependencyProcessorBase, IDisposable
    {
        public readonly string AutobuildTarget;
        public readonly Project.TargetStyleType CurrentTargetStyle;

        private int errors = 0;

        private List<ModuleDependencyConstraints> Constraints;

        internal ModuleProcessor_LoadPackage(ProcessableModule module, string autobuildtarget, Project.TargetStyleType currentTargetStyle, string logPrefix)
            : base(module, logPrefix)
        {
            AutobuildTarget = autobuildtarget;
            CurrentTargetStyle = currentTargetStyle;
            Constraints = Project.GetConstraints();
        }


        public void Process(ProcessableModule module)
        {
            LoadDependentPackages(module);

            DoDelayedInit(module);
        }


        private void LoadDependentPackages(IModule module)
        {
#if NANT_CONCURRENT
            // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
            bool parallel = (PlatformUtil.IsMonoRuntime == false) && (Project.NoParallelNant == false);
#else
            bool parallel = false;
#endif
            try
            {
                if (parallel)
                {
                    Parallel.ForEach(GetModuleDependentDefinitions(module), (d) => LoadOneDependentPackage(module, d));
                }
                else
                {
                    foreach (var d in GetModuleDependentDefinitions(module))
                    {
                        LoadOneDependentPackage(module, d);
                    }
                }
            }
            catch (Exception e)
            {
                ThreadUtil.RethrowThreadingException(
                    String.Format("Error loading dependent packages of [{0}-{1}], module '{2}'  {3}", module.Package.Name, module.Package.Version, module.Name, module.Configuration.Name),
                    Location.UnknownLocation, e);
            }

        }

        private void LoadOneDependentPackage(IModule module, Dependency<PackageDependencyDefinition> d)
        {
            try
            {
                if (0 == Interlocked.CompareExchange(ref errors, 0, 0))
                {
                    if (module.Package.Name == d.Dependent.PackageName)
                    {
                        Error.Throw(Project, "ProcessBuildModule.DoDependents", "Package '{0}' cannot depend on itself as '{1}'.", module.Package.Name, DependencyTypes.ToString(d.Type));
                    }

                    var targetStyleValue = d.IsKindOf(DependencyTypes.Build | DependencyTypes.AutoBuildUse) ? Project.TargetStyleType.Build : Project.TargetStyleType.Use;

                    // If task is called from within Use target = set target style to use:
                    if (CurrentTargetStyle == Project.TargetStyleType.Use)
                    {
                        targetStyleValue = Project.TargetStyleType.Use;
                    }

                    Log.Info.WriteLine(LogPrefix + "{0}: loading dependent package '{1}', target '{2}' (targetstyle={3}).", module.ModuleIdentityString(), d.Dependent.PackageName, AutobuildTarget, Enum.GetName(typeof(Project.TargetStyleType), targetStyleValue).ToLowerInvariant());

                    var dependent_constraints = GetModuleDependenciesConstraints(Project, module, d.Dependent.PackageName);

                    string packageVersion = TaskUtil.Dependent(Project, d.Dependent.PackageName, d.Dependent.ConfigurationName,
                                                                target: AutobuildTarget,
                                                                targetStyle: targetStyleValue,
                                                                dropCircular: true,
                                                                constraints: dependent_constraints);

                    // Add Dependent modules. Get dependent package, and add modules. Filter by moduledependency.
                    IPackage dependentPackage = null;
                    if (!Project.BuildGraph().TryGetPackage(d.Dependent.PackageName, packageVersion, d.Dependent.ConfigurationName, out dependentPackage))
                    {
                        throw new BuildException(String.Format("Can't find instance of Package after dependent task was executed for '{0}-{1}'. Verify that package has a .build file with <package> task.", d.Dependent.PackageName, packageVersion));
                    }

                    if (targetStyleValue != Project.TargetStyleType.Use)
                    {
                        AddSkippedDependentDependents(module, dependentPackage, dependent_constraints);
                    }
                }
            }
            catch (Exception)
            {
                // Throw the first error occured in any thread. Ignore others as they are likey concequences of the first one.
                if (1 == Interlocked.Increment(ref errors))
                {
                    throw;
                }
            }
        }

        private void AddSkippedDependentDependents(IModule module, IPackage dependentPackage, List<ModuleDependencyConstraints> dependent_constraints)
        {
            // DependentTask with build style was called on the dependent package, but in some cases because of circular dependencies on the package level 
            // Dependent task will drop dependency and won't wait untill dependent package is loaded. In this case we can not check skipped dependencies.

            var PackageDependentProcessingStatus = Project.BuildGraph().GetPackageDependentProcessingStatus(dependentPackage);

            var modulestoprocess = PackageDependentProcessingStatus.SetUnProcessedModules(dependent_constraints != null ? dependent_constraints.Select(c => c.Group + ":" + c.ModuleName) : null);

            if (modulestoprocess != null && modulestoprocess.Count() > 0)
            {
                foreach (var key in modulestoprocess)
                {
                    var depMod = dependentPackage.Modules.Where(m => m.BuildGroup + ":" + m.Name == key).FirstOrDefault() as ProcessableModule;

                    if (depMod != null)
                    {
                        using (var moduleProcessor = new ModuleProcessor_LoadPackage(depMod, PackageDependentProcessingStatus.AutobuildTarget, PackageDependentProcessingStatus.CurrentTargetStyle, LogPrefix))
                        {
                            moduleProcessor.Process(depMod);
                        }
                    }
                }
            }
        }

        internal void DoDelayedInit(IModule module)
        {
            string deayedInitTargetName;
            if (BuildOptionSet.Options.TryGetValue("delayedinit", out deayedInitTargetName))
            {
                Log.Info.WriteLine(LogPrefix + "'{0}': executing delayedinit target '{1}'", module.ModuleIdentityString(), deayedInitTargetName);

                Project.ExecuteTargetIfExists(deayedInitTargetName, false);
            }
        }

        #region Dispose interface implementation

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing)
                {
                    // Dispose managed resources here
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;

        #endregion
    }
}

