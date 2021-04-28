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
    [TaskName("pc-vc-generate-options")]
    public class PC_VC_GenerateOptions : VC_Common_Options
    {
        protected override void SetPlatformOptions()
        {
            base.SetPlatformOptions();

            // ----- OPTIMIZATION OPTIONS ----- 
            if (FlagEquals("optimization", "on"))
            {

                AddOption(OptionType.cc_Options, "-O2");// Maximize speed (equivalent to /Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy) 
                AddOption(OptionType.cc_Options, "-GS-");// Disable runtime buffer security checking
                if (Properties["eaconfig.disable_framepointer_optimization"] != null)
                {
                    AddOption(OptionType.cc_Options, "-Oy-");// Disable runtime buffer security checking
                }
                // link time code generation:
                if (FlagEquals("optimization.ltcg", "on"))
                {
                    AddOption(OptionType.cc_Options, "-GL");     //enable link-time code generation
                    AddOption(OptionType.lib_Options, "-LTCG"); //enable link-time code generation
                    switch (ConfigOptionSet.Options["optimization.ltcg.mode"])
                    {
                        case "normal":
                            AddOption(OptionType.link_Options, "-LTCG");
                            break;
                        case "instrument":
                            AddOption(OptionType.link_Options, "-LTCG:PGINSTRUMENT");
                            break;
                        case "optimize":
                            AddOption(OptionType.link_Options, "-LTCG:PGOPTIMIZE");
                            break;
                        case "update":
                            AddOption(OptionType.link_Options, "-LTCG:PGUPDATE");
                            break;
                        default:
                            AddOption(OptionType.link_Options, "-LTCG");
                            break;
                    }
                }
                if (ConfigOptionSet.Options["buildset.name"] != "ManagedCppAssembly")
                {
                    if (!PropertyUtil.GetBooleanProperty(Project, "incredibuild.xge.useincredilink"))
                    {
                        AddOptionIf("disable_reference_optimization", "off", OptionType.link_Options, "-opt:ref"); // eliminates functions and/or data that are never referenced
                        AddOptionIf("disable_reference_optimization", "on", OptionType.link_Options, "-opt:noref");
                    }
                }
            }

            // ----- GENERAL OPTIONS ----- 
            AddOptionIf("pcconsole", "on", OptionType.link_Options, "-subsystem:console");
            AddOptionIf("pcconsole", "off", OptionType.link_Options, "-subsystem:windows");

            AddOptionIf("analyze", "build", OptionType.cc_Options, "-analyze");
            AddOptionIf("analyzewarnings", "on", OptionType.cc_Options, "-analyze:WX");
            AddOptionIf("analyzewarnings", "off", OptionType.cc_Options, "-analyze:WX-");

            // ----- WARNING OPTIONS ----- 
        }
    }
}
