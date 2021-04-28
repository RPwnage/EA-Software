using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("ProgramWithRSO")]
    public class ProgramWithRSOTask : ModuleTask
    {
        public ProgramWithRSOTask() : base("ProgramWithRSO")
        {
        }
    }
}
