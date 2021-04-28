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
using System.Collections.Concurrent;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Core
{
	public class ForEachModule : IDisposable
	{
		private readonly ConcurrentDictionary<string, Task> _taskcollection;
		private uint _deptypes = 0;
		private readonly CancellationTokenSource _cancellationTokenSource;
		private readonly Action<ProcessableModule> _action;
		private readonly IEnumerable<IModule> _modules;
		private readonly string _logPrefix;
		private readonly bool _useModuleWaitForDependents;
		private readonly bool _parallelExecution;
		private readonly bool _syncModulesInPackage;
		private readonly Func<ProcessableModule, bool> _getsyncstate;

		public static void Execute(IEnumerable<IModule> modules, uint deptypes, Action<ProcessableModule> action, string logPrefix = null, bool useModuleWaitForDependents = true, bool parallelExecution=true, bool syncModulesInPackage=true, Func<ProcessableModule, bool> getsyncstate = null)
		{
			var inst = new ForEachModule(modules, deptypes, action, logPrefix, useModuleWaitForDependents, parallelExecution, syncModulesInPackage, getsyncstate);
			inst.Start();
			inst.Wait();
		}

		private ForEachModule(IEnumerable<IModule> modules, uint deptypes, Action<ProcessableModule> action, string logPrefix, bool waitForDependents, bool parallelExecution, bool syncModulesInPackage, Func<ProcessableModule, bool> getsyncstate = null)
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
			_getsyncstate = getsyncstate;
		}

		bool _disposed;

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (_disposed)
			{
				return;
			}

			if (disposing)
			{
				_cancellationTokenSource.Dispose();
			}

			_disposed = true;
		}
		 
		private void Start()
		{
			Task lastTask = null;
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
							if (!_useModuleWaitForDependents || (_useModuleWaitForDependents && module.ProcessModulleShouldWaitForDependents))
							{
								if (!PlatformUtil.IsMonoRuntime && lastTask != null)
								{
									//because of scheduler weirdness, it may not push tasks in order, I need to preserve order here. 
									// Yield main thread and wait for Scheduler to pick up task
									while (lastTask.Status == TaskStatus.Created
										|| lastTask.Status == TaskStatus.WaitingForActivation
										|| lastTask.Status == TaskStatus.WaitingForChildrenToComplete
										|| lastTask.Status == TaskStatus.WaitingToRun)
									{
										if (_cancellationTokenSource.IsCancellationRequested)
										{
											break;
										}

										Thread.Yield();
									}
								}
							}
							task.Start();
						}
						catch (Exception ex)
						{
							if (!_cancellationTokenSource.IsCancellationRequested)
							{
								// If we aren't in canceled state - there is something wrong. Otherwise ignore this exception;
								throw new NAnt.Core.ExceptionStackTraceSaver(ex);
							}
						}
					}
					else
					{
						task.RunSynchronously();
					}

					lastTask = task;
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
					if (SyncModulesInPackage(module) && module.Package.Modules.Count() > 1)
					{
						lock (module.Package.Project.BuildSyncRoot)
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

		private bool SyncModulesInPackage(ProcessableModule module)
		{
			bool sync = _syncModulesInPackage;
			if(_getsyncstate != null)
			{
				sync = _getsyncstate(module);
			}
			return sync;
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
	}
}
