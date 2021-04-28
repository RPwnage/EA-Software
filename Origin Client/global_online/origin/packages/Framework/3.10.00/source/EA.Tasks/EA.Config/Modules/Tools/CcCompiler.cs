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
    public class CcCompiler : Tool
    {
        public enum PrecompiledHeadersAction { NoPch, UsePch, CreatePch };

        public CcCompiler(PathString toolexe, PrecompiledHeadersAction pch = PrecompiledHeadersAction.NoPch, PathString workingDir = null, string description = "")
            : base("cc", toolexe, Tool.TypeBuild, workingDir: workingDir, description:description)
        {
            Defines = new SortedSet<string>();
            CompilerInternalDefines = new SortedSet<string>();
            IncludeDirs = new List<PathString>();
            UsingDirs = new List<PathString>();

            SourceFiles = new FileSet();
            Assemblies = new FileSet();
            ComAssemblies = new FileSet();

            Assemblies.FailOnMissingFile = false;
            ComAssemblies.FailOnMissingFile = false;

            PrecompiledHeaders = pch;
        }

        public readonly PrecompiledHeadersAction PrecompiledHeaders;
        public readonly SortedSet<string> Defines;
        public readonly SortedSet<string> CompilerInternalDefines;
        public readonly List<PathString> IncludeDirs;
        public readonly List<PathString> UsingDirs;
        public readonly FileSet SourceFiles;
        public readonly FileSet Assemblies;
        public readonly FileSet ComAssemblies;
        public string ObjFileExtension;
    }
}
