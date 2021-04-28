using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("FSharpLibrary")]
    public class FSharpLibraryTask : DotNetModuleTask
    {
        public FSharpLibraryTask() : base("CSharpLibrary", "fsc")
        {
        }
    }
}
