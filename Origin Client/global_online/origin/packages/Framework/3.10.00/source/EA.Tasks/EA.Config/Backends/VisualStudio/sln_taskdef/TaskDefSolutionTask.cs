using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

//IMTODO: Implement dynamic loading
using EA.Eaconfig.Backends.VisualStudio;

namespace EA.Eaconfig.Backends.VisualStudio.sln_taskdef
{
    [TaskName("taskdef-sln-generator")]
    class TaskDefSolutionTask : Task
    {
        protected override void ExecuteTask()
        {
            var timer = new Chrono();

            var name = "TaskDefCodeSolution";
            var outputdir = new PathString(Path.Combine(Project.Properties["nant.project.temproot"], "TaskDefVisualStudioSolution"));
            var configurations = new List<string>() { "Debug", "Release" };            
            var startupconfig = configurations.First();

            var solution = new TaskDefSolution(Project.Log, name, outputdir);

            solution.Initialize(Project, configurations, startupconfig, solution.GetTopModules(Project));

            solution.Generate();

            Log.Status.WriteLine(LogPrefix + "  {0} generated files:{1}{2}", timer.ToString(), Environment.NewLine, solution.GeneratedFiles.ToString(Environment.NewLine, p => String.Format("\t\t{0}", p.Path.Quote())));
        }
    }
}
