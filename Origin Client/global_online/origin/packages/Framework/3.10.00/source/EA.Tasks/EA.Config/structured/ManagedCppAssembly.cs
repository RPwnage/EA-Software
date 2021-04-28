using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("ManagedCppAssembly")]
    public class ManagedCppAssemblyTask : ModuleTask
    {
        public ManagedCppAssemblyTask() : base("ManagedCppAssembly")
        {
        }
    }
}
