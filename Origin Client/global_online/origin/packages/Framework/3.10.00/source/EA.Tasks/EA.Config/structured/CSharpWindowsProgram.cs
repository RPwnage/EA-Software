using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary>This task allows you to set attributes for a CSharp Program Task</summary>
    [TaskName("CSharpWindowsProgram")]
    public class CSharpWindowsProgramTask : DotNetModuleTask
    {
        public CSharpWindowsProgramTask() : base("CSharpWindowsProgram", "csc")
        {
        }
    }
}
