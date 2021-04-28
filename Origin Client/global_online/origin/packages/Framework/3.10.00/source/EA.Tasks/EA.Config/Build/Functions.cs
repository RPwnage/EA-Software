using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.Eaconfig.Core;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Build
{
    /// <summary>
    /// Collection of utility functions usually used in build, run and other targets.
    /// </summary>
    [FunctionClass("Build Functions")]
    public class BuildFunctions : FunctionClassBase
    {
        /// <summary>
        /// Eliminates duplicates from the list of space separated items.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="str">The string to process.</param>
        /// <returns>String containing distinct items.</returns>
        [Function()]
        public static string DistinctItems(Project project, string str)
        {
            return str.ToArray().OrderedDistinct().ToString(" ");
        }

        /// <summary>
        /// Returns output directory for a given module. For programs and dlls it is usually 'bin' directory, for libraries 'lib'.
        /// This function takes into account output mapping.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="type">type can be either "lib" or "bin"</param>
        /// <param name="packageName">name of the package</param>
        /// <returns>module output directory</returns>
        [Function()]
        public static string GetModuleOutputDir(Project project, string type, string packageName)
        {
            var map = project.GetOutputMapping(packageName);
            string pathmapping = String.Empty; 
            if(map != null)
            {

                string diroption = "config" + ((type == "lib") ? "lib" : "bin") + "dir";
                pathmapping = PathNormalizer.Normalize(map.Options[diroption]);

            }
            if (String.IsNullOrEmpty(pathmapping))
            {
                pathmapping = project.ExpandProperties("${package.configbuilddir}/${build.module}");

            }

            return pathmapping;
        }

        /// <summary>
        /// Returns outputname for a module. Takes into account output mapping. 
        /// </summary>
        /// <param name="project"></param>
        /// <param name="type">Can be either "lib" or "link"</param>
        /// <param name="packageName">name of a package</param>
        /// <param name="moduleName">name of a module in the package</param>
        /// <returns>outputname</returns>
        [Function()]
        public static string GetModuleOutputName(Project project, string type, string packageName, string moduleName)
        {
            var map = project.GetOutputMapping(packageName);
            string mappedname = moduleName;
            if (map != null)
            {
                if (map.Options.TryGetValue(type + "outputname", out mappedname))
                {
                    mappedname = mappedname.Replace("%outputname%", moduleName); 
                }
            }

            return mappedname;
        }

        /// <summary>
        /// Returns list of module groupmanes for modules defined in package loaded into the current Project.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="groups">list of groups to examine. I.e. 'runtime', 'test', ...</param>
        /// <returns>module groupnames, new line separated.</returns>
        [Function()]
        public static string GetModuleGroupnames(Project project, string groups)
        {
            IPackage p;
            var groupnames = new StringBuilder();
            if(project.TryGetPackage(out p))
            {
                var buildgroups = new HashSet<BuildGroups>(groups.ToArray().Select(g=> ConvertUtil.ToEnum<BuildGroups>(g)));

                foreach(var module in p.Modules.Where(m=>buildgroups.Contains(m.BuildGroup)))
                {
                    groupnames.AppendLine(module.GroupName);
                }
            }

            return groupnames.ToString();
        }



    }
}
