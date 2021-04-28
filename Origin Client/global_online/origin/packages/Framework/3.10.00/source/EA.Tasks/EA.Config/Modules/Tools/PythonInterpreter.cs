using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using NAnt.Core;

namespace EA.Eaconfig.Modules.Tools
{
    public class PythonInterpreter : Tool
    {
        public readonly FileSet SourceFiles;
        public readonly FileSet ContentFiles;

        public PythonInterpreter(string toolname, PathString toolexe)
            : base(toolname, toolexe, Tool.TypeBuild)
        {
            SourceFiles = new FileSet();
            ContentFiles = new FileSet();
        }
    }
}
