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
    public class AsmCompiler : Tool
    {
        public AsmCompiler(PathString toolexe, PathString workingDir = null)
            : base("asm", toolexe, Tool.TypeBuild, workingDir: workingDir)
        {
            Defines = new SortedSet<string>();
            IncludeDirs = new List<PathString>();
            SourceFiles = new FileSet();
        }

        public readonly SortedSet<string> Defines;
        public readonly List<PathString> IncludeDirs;
        public readonly FileSet SourceFiles;
        public string ObjFileExtension;
    }
}
