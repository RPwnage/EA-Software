using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;


namespace EA.FrameworkTasks.Model
{
    public interface IPublicData
    {
        IEnumerable<string> Defines { get; }

        IEnumerable<PathString> IncludeDirectories  { get; }

        IEnumerable<PathString> UsingDirectories { get; }

        IEnumerable<PathString> LibraryDirectories { get; }

        FileSet Libraries { get; }

        FileSet Assemblies {  get; }

        FileSet DynamicLibraries { get; }
    }
}
