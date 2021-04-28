using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.DotNetTasks;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules.Tools
{
    public class CscCompiler : DotNetCompiler
    {
        public CscCompiler(PathString toolexe, Targets target)
            : base("csc", toolexe, target)
        {
        }

        public PathString Win32icon;

        

    }
}
