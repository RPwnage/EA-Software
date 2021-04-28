using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("MakeStyle")]
    public class MakeStyleTask : ModuleBaseTask
    {
        public MakeStyleTask() : base("MakeStyle")
        {
        }

        protected override void SetupModule()
        {
        }
    }
}
