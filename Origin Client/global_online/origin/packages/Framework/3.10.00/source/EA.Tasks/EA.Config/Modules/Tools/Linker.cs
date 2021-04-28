using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;


namespace EA.Eaconfig.Modules.Tools
{
    public class Linker : Tool
    {
        public Linker(PathString toolexe, PathString workingDir = null) 
            : base("link", toolexe, Tool.TypeBuild, workingDir:workingDir)
        {
            LibraryDirs = new List<PathString>();
            Libraries = new FileSet();
            DynamicLibraries = new FileSet();
            Libraries.FailOnMissingFile = false;
            ObjectFiles = new FileSet();
            ObjectFiles.FailOnMissingFile = false;
            UseLibraryDependencyInputs = false;
        }

        public readonly List<PathString> LibraryDirs;
        public readonly FileSet Libraries;
        public readonly FileSet DynamicLibraries;  // For assemblies and other cases when we need to link to dll.
        public readonly FileSet ObjectFiles;
        public string OutputName;
        public string OutputExtension;
        public PathString LinkOutputDir;
        public PathString ImportLibOutputDir;
        public PathString ImportLibFullPath;
        public bool UseLibraryDependencyInputs;
    }
}
