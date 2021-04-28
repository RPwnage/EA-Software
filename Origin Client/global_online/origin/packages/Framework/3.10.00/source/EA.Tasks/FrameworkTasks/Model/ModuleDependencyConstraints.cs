using System;

using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
    public class ModuleDependencyConstraints : BitMask
    {
        internal readonly BuildGroups Group;
        internal readonly string ModuleName;


        internal ModuleDependencyConstraints(BuildGroups group, string modulename, uint type=0) :base(type)
        {
            Group = group;
            ModuleName = modulename;
        }
    }
}
