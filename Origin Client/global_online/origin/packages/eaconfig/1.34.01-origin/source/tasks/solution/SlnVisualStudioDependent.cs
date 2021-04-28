using NAnt.Core;
using NAnt.Core.Attributes;
using EA.FrameworkTasks;

using System;
using System.Collections.Generic;

namespace EA.Eaconfig
{
    [TaskName("SlnVisualStudioDependent")]
    public class SlnVisualStudioDependent : Task
    {
        public SlnVisualStudioDependent()
            : base()
        {
        }

        public SlnVisualStudioDependent(Project project)
            : base()
        {
            Project = project;
        }

        public static void Execute(Project project)
        {
            // find a task with the given name
            Task task = new SlnVisualStudioDependent(project);
            task.Execute();
        }

        protected override void ExecuteTask()
        {
            string vsVersion = Project.Properties["package.VisualStudio.version"];
            if (Object.ReferenceEquals(vsVersion, null))
            {
                Project.Properties["package.VisualStudio.exportbuildsettings"] = "false";
                string cc_before = PropertyUtil.GetPropertyOrDefault(Project, "cc", String.Empty);

                TaskUtil.Dependent(Project, "VisualStudio");

                if (cc_before != PropertyUtil.GetPropertyOrDefault(Project, "cc", String.Empty))
                {
                    Error.Throw(Project, Location, "SlnVisualStudioDependent", "ERROR: eaconfig requires a VisualStudio package which supports the 'package.VisualStudio.exportbuildsettings' property.  Please update your masterconfig.xml.");
                }

                vsVersion = Project.Properties["package.VisualStudio.version"];

                if (0 <= StringUtil.StrCompareVersions(vsVersion, "8.0.0"))
                {
                    Project.Properties["config-vs-version"] = "8.0";
                }
                if (0 <= StringUtil.StrCompareVersions(vsVersion, "9.0.0"))
                {
                    Project.Properties["config-vs-version"] = "9.0";
                }
                if (0 <= StringUtil.StrCompareVersions(vsVersion, "10.0.0"))
                {
                    Project.Properties["config-vs-version"] = "10.0";
                }

            }
        }
    }

}
