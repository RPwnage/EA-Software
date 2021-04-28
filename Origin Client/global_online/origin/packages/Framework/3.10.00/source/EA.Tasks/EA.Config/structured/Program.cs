using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("Program")]
    public class ProgramTask : ModuleTask
    {
        public ProgramTask() : base("Program")
        {
        }
    }
}
