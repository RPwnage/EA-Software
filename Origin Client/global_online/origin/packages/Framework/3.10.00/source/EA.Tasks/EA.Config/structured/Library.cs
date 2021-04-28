using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("Library")]
    public class LibraryTask : ModuleTask
    {
        public LibraryTask() : base("Library")
        {
        }
    }
}
