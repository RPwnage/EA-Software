using NAnt.Core;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Functions;

using System;
using System.Collections.Generic;

namespace EA.Eaconfig
{

    [TaskName("set-config-vs-version")]
    public class SetConfigVisualStudioVersion : Task
    {
        public SetConfigVisualStudioVersion()
            : base()
        {
        }

        private SetConfigVisualStudioVersion(Project project)
            : base()
        {
            Project = project;
        }

        public static string Execute(Project project)
        {
            // find a task with the given name
            var task = new SetConfigVisualStudioVersion(project);
            task.Execute();

            return task.config_vs_version;
        }

        protected override void ExecuteTask()
        {
            string vsVersion = PackageFunctions.GetPackageVersion(Project, "VisualStudio");

            if(String.IsNullOrEmpty(vsVersion))
            {
                Error.Throw(Project, Location, "set-config-vs-version", "VisualStudio package is not listed in masterconfig. Can not detect VisualStudio version");
            }

            if (0 <= StringUtil.StrCompareVersions(vsVersion, "8.0.0"))
            {
                config_vs_version = "8.0";

                Project.Properties["package.eaconfig.isusingvc8"] = "true";
            }
            if (0 <= StringUtil.StrCompareVersions(vsVersion, "9.0.0"))
            {
                config_vs_version = "9.0";

                Project.Properties["package.eaconfig.isusingvc9"] = "true";
            }
            if (0 <= StringUtil.StrCompareVersions(vsVersion, "10.0.0"))
            {
                config_vs_version = "10.0";

                Project.Properties["package.eaconfig.isusingvc10"] = "true";
            }

            if (0 <= StringUtil.StrCompareVersions(vsVersion, "11.0.0"))
            {
                config_vs_version = "11.0";

                Project.Properties["package.eaconfig.isusingvc11"] = "true";
            }


            Project.Properties["config-vs-version"] = config_vs_version;
        }

        private string config_vs_version = "10.0";
    }
}
