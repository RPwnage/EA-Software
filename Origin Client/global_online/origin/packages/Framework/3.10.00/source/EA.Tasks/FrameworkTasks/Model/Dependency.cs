using System;

using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{

    public class Dependency<T> : BitMask
    {
        public Dependency(T dependent, BuildGroups buildgroup, uint type = 0) : base(type)
        {
            Dependent = dependent;
            BuildGroup = buildgroup;
        }

        public Dependency(T dependent, BuildGroups buildgroup = BuildGroups.runtime) : this(dependent, buildgroup, 0)
        {
        }

        public Dependency(Dependency<T> other)
            : this(other.Dependent, other.BuildGroup, other.Type)
        {
        }

        public readonly BuildGroups BuildGroup;
        public readonly T Dependent;
    }
}
