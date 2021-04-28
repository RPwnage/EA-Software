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
    [TaskName("backend-generator")]
    class BackendGenerateTask  : Task
    {
        [TaskAttribute("generator-name", Required = true)]
        public string GeneratorTaskName
        {
            get { return _generatortaskname; }
            set { _generatortaskname = value; }
        }
        private string _generatortaskname;

        [TaskAttribute("startup-config", Required = true)]
        public string StartupConfig
        {
            get { return _startupconfig; }
            set { _startupconfig = value; }
        }
        private string _startupconfig;

        [TaskAttribute("configurations", Required = true)]
        public string Configurations
        {
            get { return _configurations; }
            set { _configurations = value; }
        }
        private string _configurations;

        [TaskAttribute("generate-single-config", Required = false)]
        public bool GenerateSingleConfig
        {
            get { return _generatesingleconfig; }
            set { _generatesingleconfig = value; }
        }
        private bool _generatesingleconfig = false;

        [TaskAttribute("split-by-group-names", Required = false)]
        public bool SplitByGroupNames
        {
            get { return _splitByGroupNames; }
            set { _splitByGroupNames = value; }
        } 
        private bool _splitByGroupNames = true;


        protected override void ExecuteTask()
        {
            var timer = new Chrono();

            var generatedFiles = new List<PathString>();

            var confignames = (Configurations + " " + StartupConfig).ToArray();

            FileFilters.Init(Project);

            BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
            if (stepsLog != null)
            {
                stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' + '{1}' steps", "backend.VisualStudio.pregenerate", "backend.pregenerate");
            }

            Project.ExecuteGlobalProjectSteps((Properties["backend.VisualStudio.pregenerate"] + Environment.NewLine + Properties["backend.pregenerate"]).ToArray(), stepsLog);
            {
                int groupcount = 0;
                var topmodules = Generator.GetTopModules(Project);

                var duplicateProjectNames = Generator.FindDuplicateProjectNames(Project.BuildGraph().SortedActiveModules);

                foreach (var modulegroup in Generator.GetTopModuleGroups(Project, topmodules, GenerateSingleConfig, SplitByGroupNames))
                {
                    var output = RunOneGenerator(modulegroup.Key, modulegroup, confignames, duplicateProjectNames, StartupConfig, groupcount);

                    generatedFiles.AddRange(output);
                    groupcount++;
                }
            }

            stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
            if (stepsLog != null)
            {
                stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' + '{1}' steps", "backend.VisualStudio.postgenerate", "package.postgenerate");
            }

            Project.ExecuteGlobalProjectSteps((Properties["backend.VisualStudio.postgenerate"] + Environment.NewLine + Project.Properties["package.postgenerate"]).ToArray(), stepsLog);

            Log.Status.WriteLine(LogPrefix + "  {0} generated files:{1}{2}", timer.ToString(), Environment.NewLine, generatedFiles.ToString(Environment.NewLine, p => String.Format("\t\t{0}", p.Path.Quote())));

        }

        private IEnumerable<PathString> RunOneGenerator(Generator.GeneratedLocation location, IEnumerable<IModule> topmodules, IEnumerable<string> confignames, IDictionary<string, Generator.DuplicateNameTypes> duplicateProjectNames, string startupConfigName, int groupnumber)
        {
                var generator = CreateGenerator(location.Name, location.Dir);

                generator.DuplicateProjectNames = duplicateProjectNames;

                if (generator.Initialize(Project, confignames, startupConfigName, topmodules, GenerateSingleConfig, location.GeneratorTemplates, groupnumber))
                {
                    generator.Generate();
                }
                return generator.GeneratedFiles;
        }

        private Generator CreateGenerator(string slnname, PathString slndir)
        {

            switch (SetConfigVisualStudioVersion.Execute(Project))
            {
                case "9.0":
                    return new VS2008Solution(Project.Log, slnname, slndir);
                case "11.0":
                    return new VS2011Solution(Project.Log, slnname, slndir);
                default:
                case "10.0":
                    return new VS2010Solution(Project.Log, slnname, slndir);
            }
        }
    }
}
