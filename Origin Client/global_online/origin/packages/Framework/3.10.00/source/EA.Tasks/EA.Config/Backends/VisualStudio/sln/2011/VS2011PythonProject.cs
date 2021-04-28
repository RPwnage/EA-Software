using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using EA.FrameworkTasks.Model;
using NAnt.Core;
using EA.Eaconfig.Core;
using NAnt.Core.Writers;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using NAnt.Core.Util;
using System.IO;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class VS2011PythonProject : VS2010PythonProject
    {
        internal VS2011PythonProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules)
        {
        }

        protected override string Version
        {
            get
            {
                return "11.00";
            }
        }
    }
}
