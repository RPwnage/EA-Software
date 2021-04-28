using NAnt.Core;
using NAnt.Core.Util;

using System;
using System.Collections;

using System.IO;


namespace EA.Eaconfig
{
    public class ComputeBuildOutput
    {
        public static string GetOutputDir(Project project, string groupName, BuildType buildType)
        {
            string dir = project.Properties[groupName + ".outputdir"];

            if (String.IsNullOrEmpty(dir))
            {
                OptionSet buildSet = OptionSetUtil.GetOptionSet(project, buildType.Name);
                if (buildSet != null)
                {
                    string output_filename = buildSet.Options["linkoutputname"];
                    if (!String.IsNullOrEmpty(output_filename))
                    {
                        output_filename = output_filename.Replace("%outputdir%", project.ExpandProperties("${package.configbindir}${eaconfig.${eaconfig.build.group}.outputfolder}"));
                        output_filename = output_filename.Replace("%outputname%", project.Properties["build.outputname"]);
                        dir = Path.GetDirectoryName(output_filename);
                    }
                    else
                    {
                        dir = project.ExpandProperties("${package.configbindir}${eaconfig.${eaconfig.build.group}.outputfolder}");
                    }
                }
                else
                {
                    dir = project.ExpandProperties("${package.configbindir}${eaconfig.${eaconfig.build.group}.outputfolder}");
                }
            }

            return dir;
        }
    }
}
