using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{

    internal class VS2011FSProject : VS2010FSProject
    {
        internal VS2011FSProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules)
        {
        }

        protected override string DefaultTargetFrameworkVersion
        {
            get { return "v4.5"; }
        }

        protected override string ToolsVersion
        {
            get { return "4.0"; }
        }

        protected override string ProductVersion
        {
            get { return "8.0.30703"; }
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
