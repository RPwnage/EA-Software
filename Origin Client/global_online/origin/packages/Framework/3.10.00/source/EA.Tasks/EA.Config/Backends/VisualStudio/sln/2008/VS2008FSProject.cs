using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
    
    internal class VS2008FSProject : VSDotNetProject
    {
        internal VS2008FSProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules, FSPROJ_GUID)
        {
        }

        public override string ProjectFileName
        {
            get
            {
                return ProjectFileNameWithoutExtension + ".fsproj";
            }
        }

        protected override string DefaultTargetFrameworkVersion
        {
            get { return "v3.5"; }
        }

        protected override string ToolsVersion
        {
            get { return "3.0"; }
        }

        protected override string ProductVersion
        {
            get { return "9.0.21022"; }
        }

        protected override string Version
        {
            get
            {
                return "9.00";
            }
        }


    }
}
