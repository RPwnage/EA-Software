using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary>A Module buildable as a C Sharp library.</summary>
    [TaskName("CSharpLibrary")]
    public class CSharpLibraryTask : DotNetModuleTask
    {
        public CSharpLibraryTask() : base("CSharpLibrary", "csc")
        {
        }
    }
}
