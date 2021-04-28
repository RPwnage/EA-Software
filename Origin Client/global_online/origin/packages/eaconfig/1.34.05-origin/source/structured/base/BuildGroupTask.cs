using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;


namespace EA.Eaconfig.Structured
{
    public abstract class BuildGroupBaseTask : TaskContainer
    {
        private const string GROUP_NAME_PROPERTY = "__private_structured_buildgroup";

        public static string GetGroupName(Project project)
        {
            string val = project.Properties[GROUP_NAME_PROPERTY];

            return (val == null) ? "runtime" : val;

        }

        protected string _groupName = "runtime";

        protected BuildGroupBaseTask(string group) : base()
        {
            _groupName = group;
        }
        protected override void ExecuteTask()
        {
            try
            {
                Project.Properties[GROUP_NAME_PROPERTY] = _groupName;
                base.ExecuteTask();
            }
            finally
            {
                Project.Properties.Remove(GROUP_NAME_PROPERTY);
            }
        }
    }
}
