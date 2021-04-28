using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EA.FrameworkTasks.Model
{
    public static class ModuleExtensions
    {
        public static bool Contains(this IEnumerable<IModule> modules, string modulename)
        {
            foreach (var m in modules)
            {
                if (m.Name == modulename) return true;
            }
            return false;
        }

    }
}
