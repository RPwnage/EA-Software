using System;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Build;

namespace EA.Eaconfig
{
    [TaskName("xenon-vc-postprocess")]
    class Xenon_VC_PostprocessOptions : VC_Common_PostprocessOptions
    {
        public override void Process(Module_Native module)
        {
            base.Process(module);

            if (module.Link != null)
            {
                OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, module.BuildType.Name);
                CreateImageTool(module, buildOptionSet);
                CreateDeployTool(module, buildOptionSet);
            }
        }

        private void CreateImageTool(Module_Native module, OptionSet buildOptionSet)
        {
            //IMTODO: I think I need to set postlink program to empty since I use image tool for this, check if native nant runs image tool as well.
            var outputname = module.OutputName; // PropGroupValue("outputname", module.Name);

            PathString outputFile = PathString.MakeNormalized(buildOptionSet.Options["imgbldoutputname"]
                .Replace("%intermediatedir%", module.IntermediateDir.Path)
                        .Replace("%outputdir%", module.OutputDir.Path)
                        .Replace("%outputlibdir%", module.OutputDir.Path)
                        .Replace("%outputname%", outputname));

            PathString inputFile = PathString.MakeNormalized(Path.Combine(module.Link.LinkOutputDir.Path,  module.Link.OutputName +  module.Link.OutputExtension));

            BuildTool imgbld = new BuildTool(Project, "xenonimage", PathString.MakeNormalized(Project.ExpandProperties(@"${package.xenonsdk.appdir}\bin\win32\imagexex.exe")), BuildTool.TypePostLink,
                outputdir: new PathString(Path.GetDirectoryName(outputFile.Path), PathString.PathState.Directory),
                intermediatedir: module.IntermediateDir);

            imgbld.InputDependencies.Include(inputFile.Path, failonmissing: false);
            imgbld.OutputDependencies.Include(outputFile.Path, failonmissing: false);
#if FRAMEWORK_PARALLEL_TRANSITION
#else
            imgbld.Options.Add("-nologo");
#endif
            imgbld.Options.Add(String.Format("-in:\"{0}\"\n", inputFile.Path));

            string defaults = PropGroupValue("imgbld.projectdefaults").TrimWhiteSpace();
            if (!String.IsNullOrEmpty(defaults))
            {
                imgbld.Options.Add("-config:" + defaults);
            }

            imgbld.Options.Add(String.Format("-out:\"{0}\"", outputFile.Path));

            var titleid = PropGroupValue("imgbld.titleid").TrimWhiteSpace();

            foreach (var option in PropGroupValue("imgbld.options").LinesToArray())
            {
                
                imgbld.Options.Add(option);
            }

            module.ReplaceTemplates(imgbld);

            module.SetTool(imgbld);
        }

        private void CreateDeployTool(Module_Native module, OptionSet buildOptionSet)
        {
                BuildTool deploy = new BuildTool(Project, "deploy", PathString.MakeNormalized(Project.ExpandProperties(@"${package.xenonsdk.appdir}\bin\win32\imagexex.exe")), BuildTool.TypePostLink,
                    outputdir: new PathString(PropGroupValue("dvdemulationroot", PropGroupValue("customvcprojremoteroot", String.Empty))));

                string dvdemulationroot = PropGroupValue("dvdemulationroot");

                if(!String.IsNullOrWhiteSpace(dvdemulationroot))
                {
                    deploy.Options.Add("-dvd");
                }
                else
                {
                    foreach(var file in PropGroupValue("deploymentfiles").ToArray())
                    {
                        deploy.InputDependencies.Include(file, failonmissing: false, asIs: !PathUtil.IsValidPathString(file) );
                    }
                }

                if (!Project.Properties.GetBooleanProperty("package.consoledeployment"))
                {
                    deploy.SetType(BuildTool.ExcludedFromBuild);
                }
                module.SetTool(deploy);
            
        }
    }
}