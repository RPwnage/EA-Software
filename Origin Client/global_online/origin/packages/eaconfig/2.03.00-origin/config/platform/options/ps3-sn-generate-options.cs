using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
    [TaskName("ps3-sn-generate-options")]
    public class PS3_SN_GenerateOptions : PS3_gcc_GenerateOptions
    {
        protected override void SetPlatformOptions()
        {
            if (ConfigSystem == "ps3" && ConfigOptionSet.Options["ps3-spu"] != null)
            {
                // SPU - use GCC
                ConfigCompiler = "gcc";
                ConfigPlatform = "ps3-gcc";

                SetPlatformOptions_GCC();
            }
            else
            {
                SetPlatformOptions_SN();
                SetSecureLinkoutputname();
            }
        }

        protected void SetPlatformOptions_SN()
        {
            Common_SN_Options();

            // ----- WARNING OPTIONS -----

            if (DefaultWarningSuppression == WarningSuppressionModes.On)
            {
                AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Xquit=1"); // Treat warnings as errors
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-Xdiag=1");
                AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.ps3-sn.warningsuppression", ""));
            }
            else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
            {
                AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Xquit=2");
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-Xdiag=2");
            }
            else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
            {
                AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Xquit=1"); // Treat warnings as errors
                AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.ps3-sn.warningsuppression", ""));
            }
            else
            {
                AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Xquit=1"); // Treat warnings as errors
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-Xdiag=1");
            }

            // ----- OPTIMIZATION OPTIONS -----
            AddOptionIf("optimization", "on", OptionType.cc_Options, "-Os"); //optimization for space
            //<!-- -Xfastmath=1 -->
            //<!-- -Xassumecorrectsign=1 -->
            //<!-- -Xassumecorrectalignment=1 -->

            // ----- CODEGENERATION OPTIONS -----

            AddOptionIf("clanguage", "on", OptionType.cc_Options, "-Tc");

            AddOptionIf("embedguid", "on", OptionType.link_Options, "-ppuguid");  // Store the unique build guid field, which allows unique identification of each .elf/.self.
            AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-Xrelaxalias=2");
            AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-Xrelaxalias=0");
            if (FlagEquals("clanguage", "off"))
            {
                AddOptionIf("rtti", "off", OptionType.cc_Options, "-Xc-=rtti");
                AddOptionIf("exceptions", "off", OptionType.cc_Options, "-Xc-=exceptions"); //Do not generate exception handling code
            }
            ReplaceOptionIf("rtti", "on", OptionType.cc_Options, "-Xc-=rtti", "-Xc += rtti");

            AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g");

            AddOptionIf("create-pch", "on", OptionType.cc_Options, "--create_pch=\"%pchfile%\"");
            AddOptionIf("use-pch", "on", OptionType.cc_Options, "--use_pch=\"%pchfile%\"");
            AddOptionIf("use-pch", "on", OptionType.cc_Options, "-Xpch_override=1");  //Warning 1643: the file being compiled needs to be in the same directory as the file used to create the PCH file
            AddOptionIf("use-pch", "on", OptionType.cc_Options, "--diag_suppress=631"); //Warning 631: the command line options do not match those used when precompiled header file
            AddOptionIf("create-pch", "on", OptionType.cc_Options, "--diag_suppress=631"); //Warning 631: the command line options do not match those used when precompiled header file

            //In case precompiled header is defined directly through cc options
            if (ContainsOption(OptionType.cc_Options, "--create_pch"))
            {
                ConfigOptionSet.Options["create-pch"] = "on";
            }
            if (ContainsOption(OptionType.cc_Options, "--use_pch"))
            {
                AddOption(OptionType.cc_Options, "-Xpch_override=1"); //Warning 1643: the file being compiled needs to be in the same directory as the file used to create the PCH file
            }


            // ----- GENERAL OPTIONS -----
            AddOptionIf("misc", "on", OptionType.cc_Options, "-c"); // Compile only

            // ----- LINK OPTIONS -----
            AddOptionIf("stripunusedsymbols", "on", OptionType.link_Options, "--strip-unused-data");
            AddOptionIf("stripunusedsymbols", "on", OptionType.link_Options, "--strip-duplicates");

            if (FlagEquals("exceptions", "on"))
            {
                AddOption(OptionType.link_Options, "--exceptions");
                SetOptionsString(OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                ReplaceOption(OptionType.link_Libraries, "\\fno-exceptions\\fno-rtti", "");
            }
            else
            {
                AddOption(OptionType.link_Options, "--no-exceptions");
            }

            // ----- OUTPUT OPTIONS -----
            string cmp = NAnt.Core.Functions.StringFunctions.StrCompareVersions(Project, Properties["package.ps3sdk.version"], "300.003");
            int cmpint = -1;
            if (Int32.TryParse(cmp, out cmpint))
            {
                if (cmpint > 0)
                {
                    string snTempPath = Properties["package.builddir"] + "\\" + Properties["config"] + "\\build\\SNTemp";
                    if (!Directory.Exists(snTempPath))
                    {
                        Directory.CreateDirectory(snTempPath);
                    }
                    AddOption(OptionType.cc_Options, "-td=\"" + snTempPath + "\"");
                }
            }
            AddOptionIf("generatedll", "on", OptionType.link_Options, "--oformat=fsprx");

            AddOptionIf("generatemapfile", "on", OptionType.link_Options, "--Map=\"%linkoutputmapname%\"");

        }
    }
}
