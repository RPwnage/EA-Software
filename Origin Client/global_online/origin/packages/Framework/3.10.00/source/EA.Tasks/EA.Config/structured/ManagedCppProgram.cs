using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("ManagedCppProgram")]
    public class ManagedCppProgramTask : ModuleTask
    {
        public ManagedCppProgramTask() : base("ManagedCppProgram")
        {
        }
    }
}
