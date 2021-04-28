using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("Library")]
    public class LibraryTask : ModuleTask
    {
        public LibraryTask() : base("Library")
        {
        }
    }
}
