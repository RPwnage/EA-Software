using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;


namespace EA.Eaconfig
{
    [TaskName("eaconfig-build-caller")]
    public class EaconfigBuildCaller : EAConfigTask
    {

        [TaskAttribute("build-target-name", Required = true)]
        public string BuildTargetName
        {
            get { return _buildTargetName; }
            set { _buildTargetName = value; }
        }

        public EaconfigBuildCaller(Project project, string buildTargetName)
            : base(project)
        {
            BuildTargetName = buildTargetName;
        }

        public EaconfigBuildCaller()
            : base()
        {
        }

        protected override void ExecuteTask()
        {
            // Because  target ${build-target-name} may not not be thread safe:
            bool parallel = false;

            ForEachModule.Execute(GetPackageModules(), DependencyTypes.Build, (module) =>
            {
                // Set module level properties.
                module.SetModuleBuildProperties();

                module.Package.Project.ExecuteTarget(BuildTargetName, force: true);
            },
                LogPrefix,
                parallelExecution: parallel);

        }

        private IEnumerable<IModule> GetPackageModules()
        {
            var modules = Project.BuildGraph().TopModules;

            if (modules.Count() == 0)
            {
                Log.Warning.WriteLine(LogPrefix + "Found no modules in eaconfig-build-caller target.");
            }
            return modules;
        }

        private string _buildTargetName = String.Empty;
    }
}
