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
using NAnt.Core.PackageCore;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
    [TaskName("pc-vc-postprocess")]
    class PC_VC_PostprocessOptions : VC_Common_PostprocessOptions
    {
        protected virtual string DefaultVinver
        {
            get { return Module.Configuration.System == "pc" ? "0x0501" : "0x0601"; }
        }

        public override void ProcessTool(CcCompiler cc)
        {
            // ------ defines ----- 

            //If the end-user has specified a "winver" property, then use that to add WINVER and _WIN32_WINNT
            //#defines.   Otherwise, we default to the settings for Windows XP (0x501).
            //
            //See this blog entry for more information on these defines.
            //
            //"What's the difference between WINVER, _WIN32_WINNT, _WIN32_WINDOWS, and _WIN32_IE?"
            //http://blogs.msdn.com/oldnewthing/archive/2007/04/11/2079137.aspx 
            //
            //Apprarently microsoft has removed some of the forced _WIN32_IE define in windowssdk, which used to exist in VC8 platformsdk, see the following lines existing in the original platformsdk but not in the new windowssdk
            //  C:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Include\CommCtrl.h   
            //      #ifndef _WINRESRC_
            //      #ifndef _WIN32_IE
            //      #define _WIN32_IE 0x0501
            //      #else
            //      #if (_WIN32_IE < 0x0400) && defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0500)
            //      #error _WIN32_IE setting conflicts with _WIN32_WINNT setting
            //      #endif
            //      #endif
            //      #endif

            string winver = PropGroupValue("defines.winver", (Project.Properties["eaconfig.defines.winver"] ?? DefaultVinver)).TrimWhiteSpace();

            if (!String.IsNullOrEmpty(winver))
            {
                cc.Defines.Add("_WIN32_WINNT=" + winver);
                cc.Defines.Add("WINVER=" + winver);
                cc.Defines.Add("_WIN32_IE=" + winver);
            }

            if (Module.IsKindOf(EA.FrameworkTasks.Model.Module.Managed) )
            {
                if (Module.Package.Project.Properties.GetBooleanPropertyOrDefault(PropGroupName("vc8transition"),
                    Module.Package.Project.Properties.GetBooleanPropertyOrDefault(Module.Package.Name + ".vc8transition", false)))
                {
                    cc.Defines.Add("_CRT_SECURE_NO_DEPRECATE");
                    bool found = false;
                    for (int i = 0; i < cc.Options.Count; i++)
                    {
                        if (cc.Options[i] == "-clr" || cc.Options[i].StartsWith("-clr:"))
                        {
                            cc.Options[i] = "-clr:oldSyntax";
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        cc.Options.Add("-clr:oldSyntax");
                    }
                }
            }

            // C compiler chocks on -AI option when it contains spaces. if cc options contain -TC clear usingDirs:
            // UsingDirs aren't used in C code anyway.

            if (null != cc.Options.FirstOrDefault(op => op == "-TC" || op == "/TC"))
            {
                cc.UsingDirs.Clear();
            }

            base.ProcessTool(cc);
        }

        public override void ProcessTool(Linker link)
        {

            base.ProcessTool(link);

            if (Module.BuildType.IsCLR)
            {
                string keyfile = PropGroupValue("keyfile").TrimWhiteSpace();
                if (!String.IsNullOrEmpty(keyfile))
                {
                    link.Options.Add(String.Format("-KEYFILE:\"{0}\"", keyfile));
                }
            }

            // --- Set manifest flags ---:
            OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, Module.BuildType.Name);
            var embedManifest = OptionSetUtil.GetOptionSetOrFail(Project, Module.BuildType.Name).GetBooleanOptionOrDefault("embedmanifest", true).ToString().ToLowerInvariant(); 
            var embed = PropGroupValue("embedmanifest", embedManifest);
            if (embed != null)
            {
                embedManifest = embed.ToBooleanOrDefault(true).ToString().ToLowerInvariant();
            }
            Module.MiscFlags.Add("embedmanifest", embedManifest);

        }

        public override void Process(Module_Native module)
        {
            base.Process(module);

            if (module.Link != null && !PathString.IsNullOrEmpty(module.Link.ImportLibFullPath))
            {
                var libPath = PathFunctions.PathToWindows(module.Package.Project, module.Package.Project.Properties["package.configlibdir"]);
                var outputpath = PathFunctions.PathToWindows(module.Package.Project, Path.GetDirectoryName(module.Link.ImportLibFullPath.Path));

                if (0 != String.Compare(outputpath, module.Link.LinkOutputDir.Path, true))
                {
                    var preBuildStep = new StringBuilder();
                    preBuildStep.AppendFormat("@if not exist {0} mkdir {0} & SET ERRORLEVEL=0" + Environment.NewLine, outputpath.Quote());
                    var prestep = new BuildStep("Create import library directory", BuildStep.PreBuild);
                    prestep.Commands.Add(new Command(preBuildStep.ToString().TrimWhiteSpace()));
                    module.BuildSteps.Add(prestep);
                }

                var postBuildStep = new StringBuilder();
                postBuildStep.AppendFormat("@if not exist {0} mkdir {0} & SET ERRORLEVEL=0"+Environment.NewLine, libPath);
#if FRAMEWORK_PARALLEL_TRANSITION
                postBuildStep.AppendFormat("if exist {0} %SystemRoot%\\system32\\xcopy {0} {1} /d /f /y\n", PathFunctions.PathToWindows(module.Package.Project, module.Link.ImportLibFullPath.Path), libPath);
#else
                if (!module.Link.ImportLibOutputDir.Path.StartsWith(libPath))
                {
                    postBuildStep.AppendFormat("if exist {0} %SystemRoot%\\system32\\xcopy {0} {1} /d /f /y\n", PathFunctions.PathToWindows(module.Package.Project, Path.Combine(module.Link.LinkOutputDir.Path, Path.GetFileName(module.Link.ImportLibFullPath.Path))), libPath);
                }
#endif
                var step = new BuildStep("copy import library to the lib folder", BuildStep.PostBuild);
                step.Commands.Insert(0, new Command(postBuildStep.ToString().TrimWhiteSpace()));
                module.BuildSteps.Add(step);
            }
        }

        public override void Process(Module_DotNet module)
        {
            base.Process(module);

            if (module.Compiler != null)
            {
                module.Compiler.Assemblies.Include("System.dll", asIs: true);
                module.Compiler.Assemblies.Include("System.Data.dll", asIs: true);
                module.Compiler.Assemblies.Include("System.XML.dll", asIs: true);
                module.Compiler.Assemblies.Include("System.Drawing.dll", asIs: true);
                module.Compiler.Assemblies.Include("System.Drawing.Design.dll", asIs: true);
                module.Compiler.Assemblies.Include("System.Windows.Forms.dll", asIs: true);
                if (PackageMap.Instance.GetMasterVersion("DotNet", Project).StrCompareVersions("3.0") >= 0)
                {
                    module.Compiler.Assemblies.Include("System.Core.dll", asIs: true);
                }
            }
        }
    }
}
