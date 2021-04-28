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


[TaskName("addglobalinclude")]
class AddGlobalInclude : Task
{
    protected override void ExecuteTask()
    {
        foreach(Module_Native module in Project.BuildGraph().SortedActiveModules.Where(m => (m is Module_Native)))
        {
            if (module.Cc != null)
            {
                string coreallocatorIncludes = Project.Properties["package.coreallocator.includedirs"];
                foreach(string includePath in coreallocatorIncludes.Split(' '))
                {
                    PathString p = PathString.MakeNormalized(includePath.Trim());
                    if (!module.Cc.IncludeDirs.Contains(p))
                        module.Cc.IncludeDirs.Add(p);
                }
            }
        }
    }
}
