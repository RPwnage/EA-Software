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
    [TaskName("xenon-vc-generate-options")]
    public class Xenon_VC_GenerateOptions : VC_Common_Options
    {
        protected override void SetPlatformOptions()
        {
            base.SetPlatformOptions();

            AddOptionIf("analyze", "static", OptionType.cc_Options, "/analyze:only");
            //AddOptionIf("analyze", "static", OptionType.cc_Options, "/analyze:wdpath"); //Enable static analysis
            AddOptionIf("analyze", "build", OptionType.cc_Options, "/analyze");
            //AddOptionIf("analyze", "build", OptionType.cc_Options, "/analyze:wdpath");

            AddOptionIf("analyzewarnings", "on", OptionType.cc_Options, "-analyze:WX");
            AddOptionIf("analyzewarnings", "off", OptionType.cc_Options, "-analyze:WX-");


            if (FlagEquals("optimization", "on"))
            {
                // (TODO)
                // These 3 options really shouldn't be here (some are mutually exclusive with each other)
                // But we've noticed problems with existing test data that's
                // dependent on one or more of these optimization flags being present.
                // We haven't figured out why really, but for backwards compatibility
                // they're staying...
                AddOption(OptionType.cc_Options, "-Oi");// Enable intrinsic functions 
                AddOption(OptionType.cc_Options, "-Os");// Favor smaller code 
                AddOption(OptionType.cc_Options, "-Ob2");// Inline Function Expansion 

                AddOption(OptionType.cc_Options, "-Ox");// Maximize speed  (this implys -Oi, -Oy, -Ob2, -GF)
                AddOption(OptionType.cc_Options, "-Oz");// Optimizations inline assembly
                AddOption(OptionType.cc_Options, "-Ou");// Enable prescheduling 
                AddOption(OptionType.cc_Options, "-Gy");// Enable separate functions for linker

                if (!PropertyUtil.GetBooleanProperty(Project, "incredibuild.xge.useincredilink"))
                {
                    AddOptionIf("disable_reference_optimization", "off", OptionType.link_Options, "-opt:ref"); // eliminates functions and/or data that are never referenced
                    AddOptionIf("disable_reference_optimization", "on", OptionType.link_Options, "-opt:noref");
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
            }
            // ----- GENERAL OPTIONS ----- 
            AddOptionIf("misc", "on", OptionType.cc_Options, "-QVMX128");   // Generates VMX128 native code 

            // ----- WARNING OPTIONS ----- 
            if (FlagEquals("warnings", "on") && DefaultWarningSuppression == WarningSuppressionModes.On)
            {
                //Suppress extra warnings if provided.  This has been taken out in the earlier version
                //but we find it necessary to put this back in because we don't want eaconfig to have
                //a dependency of a particular sdk version.  By having sdk packages suggesting which 
                //warning to suppress, config packages can choose whether to suppress them or not, and 
                //they don't have to care about which warning should be disabled for a particular version
                //of the sdk 

                // We can now use eaconfig.xenon-vc.warningsuppression in the xenonsdk package.  We keep the following
                // for backward compatibility.
                AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "package.xenonsdk.warningsuppression", ""));
            }
            // ----- PROFILE OPTIONS ----- 
            ReplaceOptionIf("profile", "on", OptionType.cc_Options, "-GL", String.Empty);
            AddOptionIf("profile", "on", OptionType.cc_Options, "-callcap"); //Enable callcap profiling
            ReplaceOptionIf("profile", "on", OptionType.link_Options, "-LTCG", String.Empty);
        }
    }
}

