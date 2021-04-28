using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Modules
{
    public static class ModuleToProjectExtensions
    {

        public static void ToOptionSet(this Module_Native module, Project project, string optionsetname) 
        {
            CreateModuleOptionSet<Module_Native>(module, project, optionsetname);
            // Set other properties if needed;
        }

        public static void ToOptionSet(this Module_DotNet module, Project project, string optionsetname)
        {
            CreateModuleOptionSet<Module_DotNet>(module, project, optionsetname);
            // Set other properties if needed;
        }

        public static bool ToOptionSet<T>(this Tool tool, T module, Project project, string optionsetname) where T : Module
        {
            if (tool != null)
            {
                OptionSet optionset = CreateOptionSet(project);

                optionset.Options["build.tasks"] = tool.ToolName;
                tool.ToOptionSet(optionset, module);

                project.NamedOptionSets[optionsetname] = optionset;

                return true;
            }
            return false;
        }


        private static OptionSet CreateModuleOptionSet<T>(this T module, Project project, string optionsetname) where T : Module
        {
            if (module != null)
            {
                OptionSet optionset = CreateOptionSet(project);

                optionset.Options["build.tasks"] = module.Tools.Where(tool => tool.IsKindOf(Tool.TypeBuild)).Select(tool => tool.ToolName).ToString(" ");

                foreach (var tool in module.Tools)
                {
                    tool.ToOptionSet(optionset, module);
                }

                project.NamedOptionSets[optionsetname] = optionset;

                return optionset;
            }
            return null;
        }



        private static OptionSet CreateOptionSet(Project project)
        {
            OptionSet os = new OptionSet();
            os.Project = project;
            return os;
        }

    }
}
