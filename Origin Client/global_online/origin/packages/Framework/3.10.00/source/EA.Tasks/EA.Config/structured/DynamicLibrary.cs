using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("DynamicLibrary")]
    public class DynamicLibraryTask : ModuleTask
    {
        public DynamicLibraryTask() : base("DynamicLibrary")
        {
        }
    }
}
