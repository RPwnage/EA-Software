using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;


namespace NAnt.DotNetTasks
{
    class NantBackCompatibility
    {
        private static Dictionary<string, string> _framework_assembly_map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            { "EA.CPlusPlusTasks.dll", "EA.Tasks.dll"},
            { "EA.DependencyGenerator.GCC.dll", "EA.DependencyGenerator.GCC.dll"},
            { "EA.DependencyGenerator_32.dll", "EA.DependencyGenerator_32.dll"},
            { "EA.DependencyGenerator_64.dll", "EA.DependencyGenerator_64.dll"},
            { "EA.FrameworkTasks.dll", "EA.Tasks.dll"},
            { "EA.IDependencyGenerator.dll", "EA.Tasks.dll"},
            { "EA.JavaTasks.dll", "EA.Tasks.dll"},
            { "EA.MetricsProcessor.dll", "EA.Tasks.dll"},
            { "EA.PackageCore.dll", "NAnt.Core.dll"},
            { "EA.SharpZipLib.dll", "NAnt.Tasks.dll"},
            { "ICSharpCode.SharpZipLib.dll", "ICSharpCode.SharpZipLib.dll" },
            { "NAnt.Core.dll", "NAnt.Core.dll"},
            { "NAnt.DotNetTasks.dll", "NAnt.Tasks.dll"},
            { "NAnt.PerforceTasks.dll", "NAnt.Tasks.dll"},
            { "NAnt.Shared.dll", "NAnt.Core.dll"},
            { "NAnt.Win32Tasks.dll", "NAnt.Tasks.dll"},
            { "NAnt.ZipTasks.dll", "NAnt.Tasks.dll"}            
        };

        internal static IEnumerable<string> GetCompatibleReferences(FileSet references)
        {
            references.FailOnMissingFile = false;

            var mappedreferences = new List<string>();
            foreach (var fileItem in references.FileItems)
            {
                string filename = Path.GetFileName(fileItem.FileName);
                string mappedname;
                if (_framework_assembly_map.TryGetValue(filename, out mappedname))
                {
                    mappedreferences.Add(Path.Combine(Project.NantLocation, mappedname));
                }
                else
                {
                    mappedreferences.Add(fileItem.FileName);
                }
            }
            return mappedreferences.OrderedDistinct();
        }
    }
}
