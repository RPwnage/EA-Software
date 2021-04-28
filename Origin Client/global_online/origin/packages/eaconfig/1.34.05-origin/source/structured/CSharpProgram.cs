using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using System;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("CSharpProgram")]
    public class CSharpProgramTask : ModuleTask
    {
        public CSharpProgramTask() : base("CSharpProgram")
        {
        }
    }
}
