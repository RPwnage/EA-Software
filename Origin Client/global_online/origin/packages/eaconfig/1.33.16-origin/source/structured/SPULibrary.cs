using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("SPULibrary")]
    public class SPULibraryTask : ModuleTask
    {
        public SPULibraryTask() : base("SPULibrary")
        {
        }
    }
}
