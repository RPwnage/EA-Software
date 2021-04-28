using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;


namespace EA.Eaconfig
{
    [TaskName("eaconfig-legacy-initializevariables")]
    public class Task_LegacyInitializeVariables : EAConfigTask
    {

        public Task_LegacyInitializeVariables()
            : base()
        {
        }

        protected override void ExecuteTask()
        {
            var module = GetModule();
            if (module != null && module.Package.Project != null)
            {
                module.Package.Project.Properties["build.outputname"] =  module.Package.Project.Properties[module.GroupName + ".outputname"] ?? module.Name;
                module.Package.Project.NamedFileSets["build.headerfiles.all"] = module.ExcludedFromBuild_Files;

                if (module is Module_Native)
                {
                    SetProjectData(module as Module_Native);
                }
                else if (module is Module_MakeStyle)
                {
                    SetProjectData(module as Module_MakeStyle);
                }
                else if (module is Module_Utility)
                {
                    SetProjectData(module as Module_Utility);
                }
                else if (module is Module_DotNet)
                {
                    SetProjectData(module as Module_DotNet);
                }
            }
        }

        private void SetProjectData(Module_Native module)
        {
            if(module != null)
            {
                var sourcefiles = new FileSet();
                if(module.Cc != null)
                {
                    module.Package.Project.Properties["build.includedirs.all"] = module.Cc.IncludeDirs.ToNewLineString();
                    module.Package.Project.Properties["build.defines.all"] = module.Cc.Defines.ToNewLineString();
                    
                    sourcefiles.IncludeWithBaseDir(module.Cc.SourceFiles);
                }
                if (module.Asm != null)
                {

                    sourcefiles.IncludeWithBaseDir(module.Asm.SourceFiles);
                }


                module.Package.Project.NamedFileSets["build.sourcefiles.all"] = sourcefiles;
            }
        }

        private void SetProjectData(Module_MakeStyle module)
        {
            if (module != null)
            {
                var sourcefiles = new FileSet();
                sourcefiles.IncludeWithBaseDir(module.ExcludedFromBuild_Sources);

                module.Package.Project.NamedFileSets["build.sourcefiles.all"] = sourcefiles;
            }
        }

        private void SetProjectData(Module_Utility module)
        {
            if (module != null)
            {
                var sourcefiles = new FileSet();
                sourcefiles.IncludeWithBaseDir(module.ExcludedFromBuild_Sources);

                module.Package.Project.NamedFileSets["build.sourcefiles.all"] = sourcefiles;
            }
        }

        private void SetProjectData(Module_DotNet module)
        {
            if (module != null)
            {
                if (module.Compiler != null)
                {
                    var sourcefiles = new FileSet();
                    sourcefiles.IncludeWithBaseDir(module.Compiler.SourceFiles);
                    module.Package.Project.Properties["build.defines.all"] = module.Compiler.Defines.ToNewLineString();
                    module.Package.Project.NamedFileSets["build.sourcefiles.all"] = sourcefiles;
                }
            }
        }

        private Module GetModule()
        {
            return Project.BuildGraph().TopModules.Where(m => (m.Configuration.Name == Config) && (m.Name == Properties["build.module"])).FirstOrDefault() as Module;
        }
    }
}

