using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
    [TaskName("ps3-sn-preprocess")]
    class PS3_SN_PreprocessOptions : PS3_Common_PreProcessOptions
    {
        public override void Process(Module_Native module)
        {
            base.Process(module);

            if (Module.Configuration.SubSystem == ".spu")
            {
                BuildTypeOptionSet.Options["cc.includedirs"] = String.Empty;
            }
        }
    }

    [TaskName("ps3-gcc-preprocess")]
    class PS3_GCC_PreprocessOptions : PS3_Common_PreProcessOptions
    {
        public override void Process(Module_Native module)
        {
            base.Process(module);

            BuildTypeOptionSet.Options["cc.includedirs"] = String.Empty;
        }
    }

    class PS3_Common_PreProcessOptions : AbstractModuleProcessorTask
    {
        public override void Process(Module_Native module)
        {
        }
    }

    [TaskName("ps3-gcc-postprocess")]
    class PS3_GCC_PostprocessOptions : PS3_Common_PostProcessOptions
    {
        public override void ProcessTool(CcCompiler cc)
        {
            ConvertSystemIncludes(cc);
        }
        public override void Process(Module_Native module)
        {
            //BuildTypeOptionSet.Options["cc.includedirs"] = BuildTypeOptionSet.Options["cc.systemincludedirs"];
        }
    }

    [TaskName("ps3-sn-postprocess")]
    class PS3_SN_PostprocessOptions : PS3_Common_PostProcessOptions
    {
        public override void ProcessTool(CcCompiler cc)
        {
            if (Module.Configuration.SubSystem == ".spu")
            {
                ConvertSystemIncludes(cc);
            }
        }
        public override void Process(Module_Native module)
        {
            if (module.Cc != null)
            {
                var temppath = module.Cc.Options.FirstOrDefault(o=> o.StartsWith("-td="));
                if(temppath != null && temppath.Length > 4)
                {
                    temppath = temppath.Substring(4).TrimQuotes();
                    module.Cc.CreateDirectories.Add(PathString.MakeNormalized(temppath));
                }
            }
        }

    }


    class PS3_Common_PostProcessOptions : AbstractModuleProcessorTask
    {
        public override void Process(Module_Native module)
        {
            SetPostBuildStep(module);
        }

        private void SetPostBuildStep(Module_Native module)
        {
            if (module.IsKindOf(EA.FrameworkTasks.Model.Module.DynamicLibrary))
            {
                //A (hopefully) temporary solution to move any PRX stub libraries and their
                // associated TXT files to the configlibdir.  We can't find any command-line
                // method of controlling the output location.  They just seem to get dumped
                // in the current directory.  There doesn't seem to any need to support
                // controlling the stub LIB filenames via options, because there the PRX
                // system defines a standard naming convention for them itself.  We still
                // don't really know what the TXT file is for. -->
                string libPath = PathFunctions.PathToWindows(Project, module.Link.ImportLibOutputDir.Path);
                string textPath = libPath;

                var postBuildStep = new StringBuilder();
                
                if (module.Link != null && module.Link.ImportLibFullPath != null && !String.IsNullOrEmpty(module.Link.ImportLibFullPath.Path))
                {
                    libPath = PathFunctions.PathToWindows(Project, module.Link.ImportLibFullPath.Path);
                    textPath = Path.ChangeExtension(libPath, ".txt");
                }
                postBuildStep.AppendFormat("if exist *_stub.a %SystemRoot%\\system32\\xcopy *_stub.a {0} /d /f /y", libPath);
                postBuildStep.AppendLine();
                postBuildStep.AppendLine("if exist *_stub.a del *_stub.a");
                postBuildStep.AppendFormat("if exist *.txt %SystemRoot%\\system32\\xcopy *.txt  {0} /d /f /y", textPath);
                postBuildStep.AppendLine();
                postBuildStep.Append("if exist *.txt del *.txt");

                var step = new BuildStep("copy *_sub.a and *.txt files", BuildStep.PostBuild);
                step.Commands.Add(new Command(postBuildStep.ToString().TrimWhiteSpace()));
                module.BuildSteps.Insert(0,step);
            }
        }

        protected void ConvertSystemIncludes(CcCompiler tool)
        {
#if !FRAMEWORK_PARALLEL_TRANSITION
            if (tool != null && BuildTypeOptionSet != null)
            {
                string systemincludedirs;
                if (BuildTypeOptionSet.Options.TryGetValue("cc.systemincludedirs", out systemincludedirs))
                {
                    foreach (var includedir in systemincludedirs.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct())
                    {
                        tool.Options.Add(String.Format("-isystem {0}", includedir.Path.Quote()));
                    }
                }
            }
#endif
        }
    }

}
