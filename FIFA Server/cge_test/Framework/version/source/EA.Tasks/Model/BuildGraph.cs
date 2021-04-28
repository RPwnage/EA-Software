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

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;

using EA.Eaconfig.Modules;
using EA.Tasks.Model;

namespace EA.FrameworkTasks.Model
{
	public sealed class BuildGraph
	{
		internal static bool WasReset = false;

		private IEnumerable<IModule> _sortedActiveModules;
		private IEnumerable<IModule> _topModules;
		private HashSet<string> _configurationNames = new HashSet<string>();
		private HashSet<BuildGroups> _buildGroups = new HashSet<BuildGroups>();

		public readonly PackageLoadWait PackageLoadCompletion = new PackageLoadWait();

		public readonly ConcurrentDictionary<string, IPackage> Packages = new ConcurrentDictionary<string, IPackage>();
		public readonly BlockingConcurrentDictionary<string, IModule> Modules = new BlockingConcurrentDictionary<string, IModule>();
		public readonly PackageAutoBuildCleanMap PackageAutoBuildCleanMap = new PackageAutoBuildCleanMap();

		private ConcurrentDictionary<string, PackageDependentsProcessingStatus> _packageDependentsProcessingStatus = new ConcurrentDictionary<string, PackageDependentsProcessingStatus>();

		public IEnumerable<IModule> TopModules 
		{
			get 
			{
				// Combine top modules across all configurations:
				return _topModules??new List<IModule>();
			}
		}

		/// <summary>
		/// List of modules sorted in a bottom-up fashion (i.e. leaf dependencies first followed by their immediate parents etc).
		/// This same order is also reflected in <see cref="ProcessableModule.GraphOrder"/>
		/// </summary>
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
				// Combine top modules across all configurations:
				return _configurationNames;
			}
		}

		public IEnumerable<Configuration> ConfigurationList { get; private set; } = new List<Configuration>();

		public IEnumerable<BuildGroups> BuildGroups
		{
			get
			{
				// Combine top modules across all configurations:
				return _buildGroups;
			}
		}

		public DotNetFrameworkFamilies TargetNetFamily { get; private set; }

		public string GetTargetNetVersion(Project project)
		{
			return DotNetTargeting.GetNetVersion(project, TargetNetFamily);
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

		public void SetConfigurationList(IEnumerable<Configuration> configs)
		{
			ConfigurationList = configs;
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
					log.Status.WriteLine((prefix ?? String.Empty) + "Build Graph exists{0}", ret ? " and can be reused." : ", but it can not be reused.");
					log.Status.WriteLine((prefix ?? String.Empty) + "Build Graph groups {0}, configurations: {1}", _buildGroups.ToString(" ").Quote(), _configurationNames.ToString(" ").Quote());
					log.Status.WriteLine((prefix ?? String.Empty) + "Required groups {0}, configurations: {1}", buildGroups.ToString(" ").Quote(), configNames.ToString(" ").Quote());
				}
			}

			return ret;
		}

		public PackageDependentsProcessingStatus TryGetPackageDependentProcessingStatus(IPackage package) // this should not be public, but fBConfig needs it exposed for runtime binding nonsense
		{
			if (_packageDependentsProcessingStatus.TryGetValue(package.Key, out PackageDependentsProcessingStatus status))
			{
				return status;
			}
			else
			{
				return null;
			}
		}

		internal PackageDependentsProcessingStatus GetPackageDependentProcessingStatus(IPackage package)
		{
			return _packageDependentsProcessingStatus.GetOrAdd(package.Key, new PackageDependentsProcessingStatus(package));
		}

		internal void CleanLoadPackageTempObjects()
		{
			_packageDependentsProcessingStatus = null;
		}

		public void ClearBuildGraph()
		{
			_packageDependentsProcessingStatus.Clear();
			PackageAutoBuildCleanMap.ClearAll();
			Modules.Clear();
			Packages.Clear();
			if (_topModules != null)
			{
				_topModules = null;
			}
			if (_sortedActiveModules != null)
			{
				_sortedActiveModules = null;
			}
		}

		public class PackageDependentsProcessingStatus // this should not be public, but fBConfig needs it exposed for runtime binding nonsense
		{
			public readonly HashSet<string> ProcessedModules; // this should not be public, but fBConfig needs it exposed for runtime binding nonsense

			internal Project.TargetStyleType CurrentTargetStyle;
			internal string AutobuildTarget;

			private readonly ReaderWriterLockSlim m_syncLock = new ReaderWriterLockSlim(LockRecursionPolicy.NoRecursion);
			private readonly IPackage m_package;

			private bool m_processAllModules;
			private bool m_loadComplete;
			private HashSet<string> m_modulesToProcess;

			private Func<IPackage, IEnumerable<string>, IEnumerable<string>> m_moduleDependenciesResolver; // DAVE-FUTURE-REFACTOR-TODO: lifetime of this member is a bit sketchy, should really be set at construction

			internal PackageDependentsProcessingStatus(IPackage package)
			{
				m_package = package;

				ProcessedModules = new HashSet<string>();
				m_modulesToProcess = new HashSet<string>();
				m_processAllModules = false;
				m_loadComplete = false;
			}

			internal IEnumerable<string> GetUnProcessedModules(IEnumerable<string> processedModules, Project.TargetStyleType currentTargetStyle, string autobuildTarget, Func<IPackage, IEnumerable<string>, IEnumerable<string>> moduleDependenciesResolver)
			{
				List<string> unprocessed = null;
				m_syncLock.EnterWriteLock();
				try
				{
					m_loadComplete = true;
					CurrentTargetStyle = currentTargetStyle;
					AutobuildTarget = autobuildTarget;
					m_moduleDependenciesResolver = moduleDependenciesResolver;


					ProcessedModules.UnionWith(processedModules);

					unprocessed = new List<string>();
					if (m_processAllModules)
					{
						foreach (IModule mod in m_package.Modules)
						{
							string key = mod.BuildGroup + ":" + mod.Name;
							if (ProcessedModules.Add(key))
							{
								unprocessed.Add(key);
							}
						}
					}
					IEnumerable<string> modulesToProcessWtihDep = m_moduleDependenciesResolver(m_package, m_modulesToProcess);
					foreach (string key in modulesToProcessWtihDep)
					{
						if (ProcessedModules.Add(key))
						{
							unprocessed.Add(key);
						}
					}
					m_modulesToProcess = new HashSet<string>();
				}
				finally
				{
					m_syncLock.ExitWriteLock();
				}
				return unprocessed;
			}

			internal IEnumerable<string> SetUnProcessedModules(IEnumerable<string> modulesToProcess, Func<IPackage, IEnumerable<string>, IEnumerable<string>> moduleDependenciesResolver)
			{
				List<string> processlocally = null;
				m_syncLock.EnterUpgradeableReadLock();
				try
				{
					if (modulesToProcess != null)
					{
						if (m_loadComplete)
						{
							processlocally = new List<string>();

							List<string> unprocessed = new List<string>();
							foreach (string p in modulesToProcess)
							{
								if (!ProcessedModules.Contains(p))
								{
									unprocessed.Add(p);
								}
							}

							if (unprocessed.Count() != 0)
							{
								m_syncLock.EnterWriteLock();
								try
								{
									m_moduleDependenciesResolver = moduleDependenciesResolver;
									foreach (string key in m_moduleDependenciesResolver(m_package, unprocessed))
									{
										if (ProcessedModules.Add(key))
										{
											processlocally.Add(key);
										}
									}
								}
								finally
								{
									m_syncLock.ExitWriteLock();
								}
							}
						}
						else
						{
							m_syncLock.EnterWriteLock();
							try
							{
								m_modulesToProcess.UnionWith(modulesToProcess);
							}
							finally
							{
								m_syncLock.ExitWriteLock();
							}
						}
					}
					else
					{
						if (m_loadComplete)
						{
							processlocally = new List<string>();
							foreach (IModule mod in m_package.Modules)
							{
								string key = mod.BuildGroup + ":" + mod.Name;
								if (!ProcessedModules.Contains(key))
								{
									m_syncLock.EnterWriteLock();
									try
									{
										if (ProcessedModules.Add(key))
										{
											processlocally.Add(key);
										}
									}
									finally
									{
										m_syncLock.ExitWriteLock();
									}
								}
							}
						}
						else
						{
							m_processAllModules = true;
						}
					}
				}
				finally
				{
					m_syncLock.ExitUpgradeableReadLock();
				}

				return processlocally;
			}
		}

		internal void SetBuildGraphNetFamily(Project project, DotNetFrameworkFamilies? buildGraphFamily)
		{
			TargetNetFamily = buildGraphFamily ?? DotNetFrameworkFamilies.Framework;
		}
	}

	public class PackageLoadWait
	{
		private int _tasksCount = 0;
		private ManualResetEventSlim _event = new ManualResetEventSlim(false);
		private ConcurrentBag<System.Threading.Tasks.Task> _tasks = new ConcurrentBag<System.Threading.Tasks.Task>();

		public bool StartLoadTask(System.Threading.Tasks.Task task)
		{
			if (task != null)
			{
				int count = Interlocked.Increment(ref _tasksCount);
				_tasks.Add(task);

				if (count == 1)
				{
					_event.Reset();
					return true;
				}
			}
			return false;
		}

		public void FinishedLoadTask()
		{
			int count = Interlocked.Decrement(ref _tasksCount);
			if (0 == count)
			{
				_event.Set();
			}
		}

		public void Wait()
		{
			_event.Wait();

			var completedtasks = _tasks;

			Thread.MemoryBarrier();

			_tasks = new ConcurrentBag<System.Threading.Tasks.Task>();

			ThrowExceptionsForCompletedTasks(completedtasks);
		}


		private void ThrowExceptionsForCompletedTasks(IEnumerable<System.Threading.Tasks.Task> tasks)
		{
			List<Exception> exceptions = null;

			foreach(var t in tasks)
			{
				if (t.IsCompleted)
				{
					var ex = t.Exception;

					if (ex != null)
					{
						if (exceptions == null)
						{
							exceptions = new List<Exception>(ex.InnerExceptions.Count);
						}
						exceptions.AddRange(ex.InnerExceptions);
					}
				}
			}

			if (exceptions != null && exceptions.Count > 0)
			{
				throw new AggregateException(exceptions); 
			}
		}

	}
}