using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace FrameworkDocsPlugin
{
    internal static class TopicFilterDefinitions
    {
        //IMTODO: Probably we want to read filters from some XML file, plugin configuration file?
        // For now I'm hardcoding values. 
//          NAnt Core Tasks
//                NAnt.Core.Tasks

//                Perforce Tasks
//                      NAnt.PerforceTasks 
//                Zip Tasks
//                      NAnt.ZipTasks

//                Build Tasks

//                    Native
//                       EA.CPlusPlusTasks
//                    DotNet
//                       NAnt.DotNetTasks 
//                    Java
//                       EA.JavaTasks  
//
//             Structured XML
//                 EA.Eaconfig.Structured

//          Framework Build Helper Tasks
//              EA.CallTargetIfOutOfDate
//              EA.FrameworkTasks
//              EA.Eaconfig.GenerateBuildOptionset
//              EA.Eaconfig.GenerateBuildOptionsetSPU
//              EA.Eaconfig.GenerateOptionsSPU
//              EA.Eaconfig.GetModuleBaseType
//              EA.Eaconfig.LoadPlatformConfig
//              EA.Eaconfig.repeatexecif

//          Framework Internal Tasks
//                 EA.Config.Backends.VisualStudio.sln_taskdef
//                 EA.Eaconfig.*
//                 EA.GenerateBulkBuildFiles 

        internal static IEnumerable<TopicFilter> GetTaskFilters()
        {
            var filters = new List<TopicFilter>();

            var nantCore = new TopicFilter("Nant Core Tasks", filePattern:"NAnt.Core.Tasks.*");
            filters.Add(nantCore);

            nantCore.ChildFilters.Add( new TopicFilter("Perforce Tasks", filePattern:"NAnt.PerforceTasks.*"));
            nantCore.ChildFilters.Add( new TopicFilter("Zip Tasks", filePattern:"NAnt.ZipTasks.*"));

            var buildTasks = new TopicFilter("Build Tasks");
            nantCore.ChildFilters.Add( buildTasks);

            buildTasks.ChildFilters.Add( new TopicFilter("Native", filePattern:"EA.CPlusPlusTasks.*"));
            buildTasks.ChildFilters.Add( new TopicFilter(".Net", filePattern:"NAnt.DotNetTasks.*"));
            buildTasks.ChildFilters.Add( new TopicFilter("Java", filePattern:"EA.JavaTasks.*"));

            var structuredXml = new TopicFilter("Structured XML", filePattern: "EA.Eaconfig.Structured.*");
            filters.Add(structuredXml);

            var buildHelper = new TopicFilter("Framework Tasks", filePatterns: new string[] 
            {
                "EA.CallTargetIfOutOfDate.*",
                "EA.FrameworkTasks.*",
                "EA.Eaconfig.GenerateBuildOptionset",
                "EA.Eaconfig.GenerateBuildOptionsetSPU",
                "EA.Eaconfig.GenerateOptionsSPU",
                "EA.Eaconfig.GetModuleBaseType",
                "EA.Eaconfig.LoadPlatformConfig",
                "EA.Eaconfig.repeatexecif"
            });
            filters.Add(buildHelper);

            var utility = new TopicFilter("Framework Utility Tasks", filePatterns: new string[] 
            {
               "EA.Config.Backends.VisualStudio.sln_taskdef.*",
               "EA.Eaconfig.*",
               "EA.GenerateBulkBuildFiles.*"
            });
            filters.Add(utility);

            return filters;
        }
    }
}
