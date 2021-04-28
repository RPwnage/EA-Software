using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Core
{
    public class ForEachModule
    {
        public static void Execute(IEnumerable<IModule> modules, uint deptypes, Action<ProcessableModule> action, string logPrefix = null, bool useModuleWaitForDependents = true, bool parallelExecution=true, bool syncModulesInPackage=true)
        {
            var inst = new ForEachModule(modules, deptypes, action, logPrefix, useModuleWaitForDependents, parallelExecution, syncModulesInPackage);

            inst.Start();

            inst.Wait();
        }

        private ForEachModule(IEnumerable<IModule> modules, uint deptypes, Action<ProcessableModule> action, string logPrefix, bool waitForDependents, bool parallelExecution, bool syncModulesInPackage)
        {
            _taskcollection = new ConcurrentDictionary<string, Task>();

            _deptypes = deptypes;

            _action = action;

            _modules = modules;

            _logPrefix = logPrefix ?? String.Empty;

            _cancellationTokenSource = new CancellationTokenSource();

            _useModuleWaitForDependents = waitForDependents;

            _parallelExecution = parallelExecution;
            _syncModulesInPackage = syncModulesInPackage;
        }
         
        private void Start()
        {
            foreach (ProcessableModule module in _modules.Where(m => !(m is Module_UseDependency)))
            {
                if (_cancellationTokenSource.IsCancellationRequested)
                {
                    break;
                }
                
                var task = new Task((m) => ExecuteActionWithDependents(m), module, _cancellationTokenSource.Token, TaskCreationOptions.PreferFairness);

                if (_taskcollection.TryAdd(module.Key, task))
                {
                    if (_parallelExecution)
                    {
                        try
                        {
                            task.Start();
                        }
                        catch (Exception ex)
                        {
                            if (!_cancellationTokenSource.IsCancellationRequested)
                            {
                                // If we aren't in cancelled state - there is something wrong. Otherwise ignore this exception;
                                throw new NAnt.Core.ExceptionStackTraceSaver(ex);
                            }
                        }

                        if (!_useModuleWaitForDependents || (_useModuleWaitForDependents && module.ProcessModulleShouldWaitForDependents))
                        {
                            if (!PlatformUtil.IsMonoRuntime)
                            {
                                //because of scheduler weirdness, it may not push tasks in order, I need to preserve order here. 
                                // Yield main thread and wait for Scheduler to pick up task
                                do
                                {
                                    if (_cancellationTokenSource.IsCancellationRequested)
                                    {
                                        break;
                                    }

                                    Thread.Sleep(1);
                                }
                                while (task.Status == TaskStatus.Created
                                    || task.Status == TaskStatus.WaitingForActivation
                                    || task.Status == TaskStatus.WaitingForChildrenToComplete
                                    || task.Status == TaskStatus.WaitingToRun);
                            }
                        }
                    }
                    else
                    {
                        task.RunSynchronously();
                    }
                }
            }
        }

        private void Wait()
        {
            try
            {
                Task.WaitAll(_taskcollection.Values.ToArray());
            }
            catch (Exception ex)
            {
                NAnt.Core.Threading.ThreadUtil.RethrowThreadingException(_logPrefix, ex);
            }
        }

        private void ExecuteActionWithDependents(object param)
        {
            ProcessableModule module = param as ProcessableModule;

            if (!_useModuleWaitForDependents || (_useModuleWaitForDependents && module.ProcessModulleShouldWaitForDependents))
            {
                var deptasks = GetDependentTasks(module);

                if (deptasks.Count > 0)
                {
                    try
                    {
                        Task.WaitAll(deptasks.ToArray(), _cancellationTokenSource.Token);
                    }
                    catch (Exception ex)
                    {
                        lock (_cancellationTokenSource)
                        {

                            if (!_cancellationTokenSource.Token.IsCancellationRequested)
                            {
                                _cancellationTokenSource.Cancel(throwOnFirstException: false);

                                var packageinfo = module.Package != null ? String.Format(", package: {0}-{1} [{2}], one of dependent builds failed.", module.Package.Name, module.Package.Version, module.Package.ConfigurationName) : string.Empty;

                                NAnt.Core.Threading.ThreadUtil.RethrowThreadingException(_logPrefix + packageinfo, ex);
                            }
                        }
                    }
                }
            }
            try
            {
                if (!_cancellationTokenSource.Token.IsCancellationRequested)
                {
                    if (_syncModulesInPackage)
                    {
                        lock (module.Package.Project.SyncRoot)
                        {
                            _action(module);
                        }
                    }
                    else
                    {
                        _action(module);
                    }
                }
            }
            catch (Exception ex)
            {
                lock (_cancellationTokenSource)
                {
                    if (!_cancellationTokenSource.Token.IsCancellationRequested)
                    {
                        _cancellationTokenSource.Cancel(throwOnFirstException: false);

                        var packageinfo = module.Package != null ? String.Format(", package: {0}-{1} [{2}] ", module.Package.Name, module.Package.Version, module.Package.ConfigurationName) : string.Empty;
                        var msg = String.Format(_logPrefix + "{0} failed: {1} ", packageinfo, ex.Message);
                        throw new NAnt.Core.BuildException(msg, ex);
                    }
                }
            }

        }

        private IList<Task> GetDependentTasks(IModule module)
        {
            var depTasks = new List<Task>();
            foreach (var dep in module.Dependents.Where(d => !(d.Dependent is Module_UseDependency) && d.IsKindOf(_deptypes)))
            {
                Task depTask;
                if (_taskcollection.TryGetValue(dep.Dependent.Key, out depTask))
                {
                    depTasks.Add(depTask);
                }
                else
                {
                    module.Package.Project.Log.Warning.WriteLine("ForEachModule: can not find task for dependency between '{0}'[order={1}] and '{2}' [order={3}]. Dependency type='{4}', Dependency filter='{5}'", module.Name, ((ProcessableModule)module).GraphOrder, dep.Dependent.Name, ((ProcessableModule)dep.Dependent).GraphOrder, DependencyTypes.ToString(dep.Type), DependencyTypes.ToString(_deptypes));
                }
            }
            return depTasks;
        }

        private readonly ConcurrentDictionary<string, Task> _taskcollection;
        private uint _deptypes = 0;
        private readonly CancellationTokenSource _cancellationTokenSource;
        private readonly Action<ProcessableModule> _action;
        private readonly IEnumerable<IModule> _modules;
        private readonly string _logPrefix;
        private readonly bool _useModuleWaitForDependents;
        private readonly bool _parallelExecution;
        private readonly bool _syncModulesInPackage;

    }
}
