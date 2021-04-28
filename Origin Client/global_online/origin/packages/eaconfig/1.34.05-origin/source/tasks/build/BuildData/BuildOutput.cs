using NAnt.Core;
using NAnt.Core.Util;

using System;
using System.Collections;
using System.IO;


namespace EA.Eaconfig
{
    public class BuildOutput
    {
        public readonly string FullOutputName;
        public readonly string OutputName;
        public readonly string OutputBuildDir;
        public readonly string OutputBinDir;
        public readonly string OutputLibDir;

        public static BuildOutput GetBuildOutput(Project project, string groupName, OptionSet builOptionSet, BuildType buildType, string BuildProjName)
        {
            string buildGroup = project.Properties["eaconfig.build.group"];
            string groupOutputDir = project.Properties["eaconfig." + buildGroup + ".outputfolder"];

            string fulloutputName = String.Empty;
            string outputName = String.Empty;
            string buildDir = project.Properties["package.configbuilddir"] + "/" + groupOutputDir + "/" + BuildProjName;
            string binDir = String.Empty;
            string libDir = String.Empty;

            string dir = project.Properties[groupName + ".outputdir"];

            if (String.IsNullOrEmpty(dir))
            {
                binDir = project.Properties["package.configbindir"] + "/" + groupOutputDir;
                libDir = project.Properties["package.configlibdir"] + "/" + groupOutputDir;

                fulloutputName = builOptionSet.Options["linkoutputname"];
                if (!String.IsNullOrEmpty(fulloutputName))
                {
                    fulloutputName = fulloutputName.Replace("%outputdir%", binDir);
                    if (fulloutputName.Contains("%outputname%"))
                    {
                        outputName = project.Properties["build.outputname"];
                        fulloutputName = fulloutputName.Replace("%outputname%", outputName);
                        fulloutputName = PathNormalizer.Normalize(fulloutputName, false);
                    }
                    else
                    {
                        fulloutputName = PathNormalizer.Normalize(fulloutputName, false);
                        outputName = Path.GetFileNameWithoutExtension(fulloutputName);
                    }
                    
                    binDir = Path.GetDirectoryName(fulloutputName);
                }
            }
            else
            {
                binDir = dir;
                libDir = dir;
                outputName = project.Properties["build.outputname"];
            }

            if (String.IsNullOrEmpty(fulloutputName))
            {
                //fulloutputName property is only used in C#/MCPP builds
                string suffix = String.Empty;

                if (buildType.IsManaged)
                {
                    if (buildType.BaseName == "ManagedCppAssembly")
                    {
                        suffix = ".dll";
                    }
                    else
                    {
                        suffix = project.Properties["exe-suffix"];
                    }
                }
                else if (buildType.IsCSharp)
                {
                    string cscTarget = builOptionSet.Options["csc.target"];
                    if (cscTarget == "library")
                    {
                        suffix = ".dll";
                    }
                    else
                    {
                        suffix = project.Properties["exe-suffix"];
                    }
                }
                else if (buildType.IsFSharp)
                {
                    string fscTarget = builOptionSet.Options["fsc.target"];
                    if (fscTarget == "library")
                    {
                        suffix = ".dll";
                    }
                    else
                    {
                        suffix = project.Properties["exe-suffix"];
                    }
                }
                if (String.IsNullOrEmpty(fulloutputName))
                {
                    fulloutputName = binDir + "/" + project.Properties["build.outputname"] + suffix;
                    fulloutputName = PathNormalizer.Normalize(fulloutputName, false);
                }
            }
            if(String.IsNullOrEmpty(outputName))
            {
                outputName = project.Properties["build.outputname"];
            }

            return new BuildOutput(fulloutputName, outputName, buildDir, binDir, libDir);

        }

        private BuildOutput(string fullOutputName,  string outputName,  string outputBuildDir,  string outputBinDir, string outputLibDir)
        {
            FullOutputName = PathNormalizer.Normalize(fullOutputName, false);
            OutputName = PathNormalizer.Normalize(outputName, false);
            OutputBuildDir = PathNormalizer.Normalize(outputBuildDir, false);
            OutputBinDir = PathNormalizer.Normalize(outputBinDir, false);
            OutputLibDir = PathNormalizer.Normalize(outputLibDir, false);
        }
    }

}
