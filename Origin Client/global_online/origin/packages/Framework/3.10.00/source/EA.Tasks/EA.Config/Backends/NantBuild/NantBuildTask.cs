using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading;
using Threading = System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using EA.CPlusPlusTasks;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

using EA.Eaconfig.Backends.VisualStudio;


namespace EA.Eaconfig.Backends
{
    [TaskName("nant-build")]
    class NantBuildTask : Task
    {
        [TaskAttribute("configs", Required = true)]
        public string Configurations
        {
            get { return _configs; }
            set { _configs = value; }
        }
        private string _configs;


        protected override void ExecuteTask()
        {
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
            bool parallel = true;
#else
            bool parallel = false;
#endif
            bool syncModulesInPackage = !Project.Properties.GetBooleanPropertyOrDefault("nant.build-package-modules-in-parallel", false);

            var modulesToBuild = GetModulesToBuild();

            ForEachModule.Execute(modulesToBuild, DependencyTypes.Build, (module) =>
                {
                    bool cloneProject = false;
 
                    if (!syncModulesInPackage)
                    {
                        //If this module Package contains other modules we are building which aren't build dependents then these modules can be invoked in parallel
                        // and we need to clone project.
                        var otherModulesFromPackage = modulesToBuild.Where(m => (m.Package.Key == module.Package.Key) && (m.Key != module.Key));
                        cloneProject = otherModulesFromPackage
                            .Except(module.Dependents.Where(d => !(d.Dependent is Module_UseDependency) && d.IsKindOf(DependencyTypes.Build)).Select(d=>d.Dependent))
                            .Count() > 0;
                    }

                    var buildProject = cloneProject ? module.Package.Project.Clone() : module.Package.Project;

                    module.Package.Project.Log.Status.WriteLine(LogPrefix + "building {0}-{1} ({2}) '{3}.{4}'", module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup.ToString(), module.Name);

                    // Set module level properties.
                    module.SetModuleBuildProperties(buildProject);

                    if (buildProject != null)
                    {

                        string outputdir;
                        string outputname;
                        string outputext;

                        module.PrimaryOutput(out  outputdir, out outputname, out outputext);

                        buildProject.Properties["outputpath"] = outputdir;
                        buildProject.Properties["build.outputname"] = outputname;
                        buildProject.Properties["build.outputextension"] = outputext;
                    }


                    // Execute build task:
                    var task = new DoBuildModuleTask();
                    task.Project = buildProject;
                    task.Parent = this;
                    task.Module = module as ProcessableModule;
                    task.Execute();

                    processedModules++;
                },
                LogPrefix,
                parallelExecution: parallel,
                syncModulesInPackage: syncModulesInPackage);

            return processedModules;
        }

        private IEnumerable<IModule> GetModulesToBuild()
        {
            var  modulesToBuild = Project.BuildGraph().SortedActiveModules.Where(m => !(m is Module_UseDependency));
            return modulesToBuild;
        }

        /// <summary>The prefix used when sending messages to the log.</summary>
        public override string LogPrefix
        {
            get
            {
                string prefix = "[nant-build] ";
                return prefix.PadLeft(prefix.Length + Log.IndentSize);
            }
        }


    }
}

