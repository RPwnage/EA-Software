using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("FSharpProgram")]
    public class FSharpProgramTask : DotNetModuleTask
    {
        public FSharpProgramTask() : base("FSharpProgram", "fsc")
        {
        }
    }
}
