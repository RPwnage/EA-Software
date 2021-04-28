using NAnt.Core;
using EA.FrameworkTasks;

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace EA.Eaconfig
{
    public class TargetUtil
    {
        public static bool TargetExists(Project project, string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                return false;
            }
            return null != project.Targets.Find(name);
        }


        public static void ExecuteTarget(Project project, string targetName, bool force)
        {
            if (String.IsNullOrEmpty(targetName))
            {
                Error.Throw(project, "ExecuteTarget", "Target name is null or empty.");
            }

            if (force)
            {
                Target t = project.Targets.Find(targetName);
                if (t == null)
                {
                    // if we can't find it, then neither should Project.Execute.
                    // Let them do the error handling and exception generation.
                    project.Execute(targetName);
                }

                //Execute a copy.
                t.Copy().Execute();

            }
            else
            {
                project.Execute(targetName);
            }
        }

        public static bool ExecuteTargetIfExists(Project project, string targetName, bool force)
        {
            if (!String.IsNullOrEmpty(targetName))
            {
                Target t = project.Targets.Find(targetName);
                if (t != null)
                {
                    if (force)
                    {
                        t.Copy().Execute();
                    }
                    else
                    {
                        t.Execute();
                    }
                    return true;
                }
            }
            return false;
        }

    }

}


