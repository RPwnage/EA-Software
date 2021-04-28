using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
    public enum EnvironmentTypes { exec, build };

    public static class ProjectExtensions
    {
        private static readonly Guid TaskDefSetObjectId = new Guid("BA94A62B-6289-449D-BE11-EE45C8F6700C");

        public static IDictionary<string, string> Env(this Project project, EnvironmentTypes envtype)
        {
            IDictionary<string, string> envMap = new Dictionary<string, string>();

            string EnvironmentVariablePropertyNamePrefix = envtype + ".env";

            foreach (Property property in project.Properties)
            {
                if (property.Prefix == EnvironmentVariablePropertyNamePrefix)
                {
                    envMap[property.Suffix] = property.Value;
                }
            }
            return envMap;
        }


        public static bool TargetExists(this Project project, string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                return false;
            }
            return null != project.Targets.Find(name);
        } 


        public static void ExecuteTarget(this Project project, string targetName, bool force)
        {
            project.Execute(targetName, force);
        }

        public static bool ExecuteTargetIfExists(this Project project, string targetName, bool force)
        {
            return project.Execute(targetName, force, false);
        }
        

        public static Tasks.TaskDefModules TaskDefModules(this Project project)
        {
            return project.NamedObjects.GetOrAdd(TaskDefSetObjectId, (key) =>
            {
                lock (Project.GlobalNamedObjects)
                {
                    Project.GlobalNamedObjects.Add(TaskDefSetObjectId);
                }

                return new Tasks.TaskDefModules();
            }) as Tasks.TaskDefModules;
        }

    }
}
