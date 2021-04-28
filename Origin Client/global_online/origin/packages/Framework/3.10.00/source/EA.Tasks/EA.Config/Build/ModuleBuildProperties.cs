using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

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

namespace EA.Eaconfig.Build
{
    public static class ModuleBuildProperties
    {
        public static void SetModuleBuildProperties(this IModule module, Project buildproject=null)
        {
            var project = buildproject??module.Package.Project;
            if(project != null)
            {
                var pm = module as ProcessableModule;

                project.Properties["groupname"] = module.GroupName;
                project.Properties["build.module"] = module.Name;
                project.Properties["groupsourcedir"] = module.GroupSourceDir;
                // For FW3 to work with build graph from targets
                project.Properties["build.module.key"] = module.Key;
                
                if (pm != null)
                {
                    project.Properties["build.buildtype"] = pm.BuildType.Name;
                    project.Properties["build.buildtype.base"] = pm.BuildType.BaseName;
                    project.Properties["subsystem"] = pm.BuildType.SubSystem;
                }
            }
        }
    }
}
