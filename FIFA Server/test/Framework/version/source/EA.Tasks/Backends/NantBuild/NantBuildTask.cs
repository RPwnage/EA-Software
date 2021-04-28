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
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using EA.CPlusPlusTasks;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;


namespace EA.Eaconfig.Backends
{
	[TaskName("nant-build")]
	class NantBuildTask : Task
	{
		private static readonly Object _threadPoolSyncRoot = new Object();
		private static bool threadpool_adjusted = false;

		[TaskAttribute("configs", Required = true)]
		public string Configurations { get; set; }

		protected override void ExecuteTask()
		{
			// <fail unless="${platform.supports-nantbuild??true}" message="${platform.supports-nantbuild-message??Platform ${config-system} does not support nant-build.}"/>
			FailTask platformSupportCheck = new FailTask();
			platformSupportCheck.Project = Project;
			platformSupportCheck.UnlessDefined = Project.Properties.GetBooleanPropertyOrDefault("platform.supports-nantbuild", true);
			platformSupportCheck.Message = Project.ExpandProperties("${platform.supports-nantbuild-message??Platform ${config-system} does not support nant-build.}");
			platformSupportCheck.Execute();

			var timer = new Chrono();

			BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed ) ? new BufferedLog(Log) : null;
			if (stepsLog != null)
			{
				stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' + '{1}' steps", "backend.nant.pregenerate", "backend.pregenerate");
			}

			Project.ExecuteGlobalProjectSteps((Properties["backend.nant.pregenerate"] + Environment.NewLine + Properties["backend.pregenerate"]).ToArray(), stepsLog);

			int modulesCount = BuildAllModules();

			Log.Status.WriteLine(LogPrefix + "finished target '{0}' ({1} modules) {2}", Properties["target.name"], modulesCount, timer.ToString());

		}

		private int BuildAllModules()
		{
			int processedModules = 0;
#if NANT_CONCURRENT
			bool parallel = (Project.NoParallelNant == false);
#else
			bool parallel = false;
#endif
			bool syncModulesInPackage_HasUserSetting = Project.Properties.Contains("nant.build-package-modules-in-parallel");
			bool syncModulesInPackage = !Project.Properties.GetBooleanPropertyOrDefault("nant.build-package-modules-in-parallel", true);

			bool collectCompilerStatistic = Project.Properties.GetBooleanPropertyOrDefault("nant.collect-compiler-stat", false);
			bool generateDependencyOnly = Project.Properties.GetBooleanPropertyOrDefault("nant.generate-dependency-only", false);

			var modulesToBuild = GetModulesToBuild();

			AdjustMonoThreadPool(Project);

			var copyLocalSync = new ConcurrentDictionary<string, bool>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

			ForEachModule.Execute(modulesToBuild, DependencyTypes.Build, (module) =>
			{
				bool cloneProject = false;

				if (!syncModulesInPackage)
				{
					//If this module Package contains other modules we are building which aren't build dependents then these modules can be invoked in parallel
					// and we need to clone project.
					var otherModulesFromPackage = modulesToBuild.Where(m => (m.Package.Key == module.Package.Key) && (m.Key != module.Key));
					cloneProject = otherModulesFromPackage
						.Except(module.Dependents.Where(d => !(d.Dependent is Module_UseDependency) && d.IsKindOf(DependencyTypes.Build)).Select(d => d.Dependent))
						.Count() > 0;
				}

				var buildProject = cloneProject ? module.Package.Project.Clone() : module.Package.Project;

				module.Package.Project.Log.Status.WriteLine(LogPrefix + "building {0}-{1} ({2}) '{3}.{4}'", module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup.ToString(), module.Name);

				// Set module level properties.
				module.SetModuleBuildProperties(buildProject);

				string buildgroup = null;
				try
				{
					if (buildProject != null)
					{
						module.PrimaryOutput(out string outputdir, out string outputname, out string outputext);

						buildProject.Properties["outputpath"] = outputdir;
						buildProject.Properties["build.outputname"] = outputname;
						buildProject.Properties["build.outputextension"] = outputext;
						buildgroup = buildProject.Properties["eaconfig.build.group"];
						buildProject.Properties["eaconfig.build.group"] = Enum.GetName(typeof(BuildGroups), module.BuildGroup);
					}

					// Execute build task:
					DoBuildModuleTask task = new DoBuildModuleTask(copyLocalSync)
					{
						Project = buildProject,
						Parent = this,
						Module = module as ProcessableModule,
						collectCompilerStatistic = collectCompilerStatistic,
						generateDependencyOnly = generateDependencyOnly
					};

					if (collectCompilerStatistic || generateDependencyOnly)
					{
						task.FailOnError = false;
					}

					task.Execute();
				}
				finally
				{
					if (buildProject != null && !String.IsNullOrEmpty(buildgroup))
					{ 
						buildProject.Properties["eaconfig.build.group"] = buildgroup; }

				}
				processedModules++;
			},
			LogPrefix,
			parallelExecution: parallel,
			useModuleWaitForDependents: false,
			syncModulesInPackage: syncModulesInPackage,
			getsyncstate:
			(mod) => 
			{
				if (syncModulesInPackage_HasUserSetting)
				{
					return syncModulesInPackage;
				}
				return (mod.IsKindOf(Module.Program) || mod.BuildGroup == BuildGroups.test) ? true : syncModulesInPackage;
			});

			if (collectCompilerStatistic)
			{
				CompileStatistics.WriteReport(Log);
			}

			return processedModules;
		}

		private IEnumerable<IModule> GetModulesToBuild()
		{
			return Project.BuildGraph().TopModules.Union(Project.BuildGraph().TopModules.SelectMany(m => m.Dependents.FlattenAll(DependencyTypes.Build), (m, d) => d.Dependent).Where(m => !(m is Module_UseDependency))).OrderBy(mod => ((ProcessableModule)mod).GraphOrder);
		}

		private static void AdjustMonoThreadPool(Project project)
		{
			if (!threadpool_adjusted)
			{
				if (PlatformUtil.IsMonoRuntime)
				{
					lock (_threadPoolSyncRoot)
					{
						if (!threadpool_adjusted)
						{
							// Mono can grow number of threads to unreasonably big number when oversubscribing long running tasks.
							int maxthreads;
							if (!Int32.TryParse(project.GetPropertyValue("cc.threadcount") ?? String.Empty, out maxthreads))
							{
								maxthreads = Environment.ProcessorCount * 3 + 1;
							}

							// Make sure new max is not less than than currently allocated

							int currentWorkerThreads, currentCompletionPortThreads;
							ThreadPool.GetAvailableThreads(out currentWorkerThreads, out currentCompletionPortThreads);

							maxthreads = Math.Max(currentWorkerThreads, maxthreads);

							int workerThreads, completionPortThreads;
							ThreadPool.GetMaxThreads(out workerThreads, out completionPortThreads);
							ThreadPool.SetMaxThreads(maxthreads, completionPortThreads);

							threadpool_adjusted = true;
						}

					}
				}
			}
		}


		/// <summary>The prefix used when sending messages to the log.</summary>
		public override string LogPrefix
		{
			get
			{
				return " [nant-build] ";
			}
		}


	}
}

