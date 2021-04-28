using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class VS2010Project : VSCppMsbuildProject
    {


        internal VS2010Project(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules)
        {
        }

        public override string ProjectFileName
        {
            get
            {
                return ProjectFileNameWithoutExtension + ".vcxproj";
            }
        }

        protected override string Version 
        {
            get
            {
                return "10.00";
            }
        }

        protected override string ToolsVersion 
        {
            get 
            { 
                return "4.0"; 
            } 
        }

        protected override string DefaultToolset 
        {
            get 
            {
                return "v100";
            } 
        }

        protected override string DefaultTargetFrameworkVersion
        {
            get { return "4.0"; }
        }

        internal override bool SupportsDeploy(Configuration config)
        {
            var module = Modules.FirstOrDefault(m => m.Configuration == config) as Module_Native;

            if (module != null && module.Package.Project != null && 
                module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.platform-supports-deploy", false))
            {
                return module.Tools.Any(t => (t.ToolName == "deploy" && !t.IsKindOf(BuildTool.ExcludedFromBuild)));
                    
            }
            return false;
        }
    }
}
