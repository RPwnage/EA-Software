using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("This task allows you to set attributes for a CSharp Program Task")]
    [NAnt.Core.Attributes.XmlSchema()]
    [TaskName("CSharpWindowsProgram")]
    public class CSharpWindowsProgramTask : ModuleTask
    {
        public CSharpWindowsProgramTask() : base("CSharpWindowsProgram")
        {
        }
    }
}
