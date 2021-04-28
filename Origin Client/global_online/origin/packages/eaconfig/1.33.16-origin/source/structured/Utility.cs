using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("Utility")]
    public class UtilityTask : ModuleBaseTask
    {
        public UtilityTask() : base("Utility")
        {
        }

        protected override void SetupModule()
        {
        }

    }
}
