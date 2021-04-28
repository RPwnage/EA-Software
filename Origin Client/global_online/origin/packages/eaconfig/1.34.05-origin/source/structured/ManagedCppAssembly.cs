using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("ManagedCppAssembly")]
    public class ManagedCppAssemblyTask : ModuleTask
    {
        public ManagedCppAssemblyTask() : base("ManagedCppAssembly")
        {
        }
    }
}
