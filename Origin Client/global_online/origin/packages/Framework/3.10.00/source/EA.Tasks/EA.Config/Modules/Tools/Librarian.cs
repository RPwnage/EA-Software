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
    public class Librarian : Tool
    {
        public Librarian(PathString toolexe, PathString workingDir = null)
            : base("lib", toolexe, Tool.TypeBuild, workingDir: workingDir)
        {
            ObjectFiles = new FileSet();
            ObjectFiles.FailOnMissingFile = false;
        }

        public readonly FileSet ObjectFiles;

        public string OutputName;
        public string OutputExtension;
    }
}
