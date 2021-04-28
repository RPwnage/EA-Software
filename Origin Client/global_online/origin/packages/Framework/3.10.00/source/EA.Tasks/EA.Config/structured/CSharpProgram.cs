using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("CSharpProgram")]
    public class CSharpProgramTask : DotNetModuleTask
    {
        public CSharpProgramTask() : base("CSharpProgram", "csc")
        {
        }
    }
}
