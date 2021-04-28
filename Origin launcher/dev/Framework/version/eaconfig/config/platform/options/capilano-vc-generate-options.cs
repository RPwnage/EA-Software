// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using EA.Eaconfig;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
	[TaskName("capilano-vc-generate-options", XmlSchema = false)]
	public class Capilano_VC_GenerateOptions : VC_Common_Options
	{
		protected override void SetPlatformOptions()
		{
			if (FlagEquals("retailflags", "on") && ConfigOptionSet.Options["optimization.ltcg"] == null)
			{
				ConfigOptionSet.Options["optimization.ltcg"] = "on";
			}
			SetCommonOptions();

            if (FlagEquals("optimization.ltcg", "on"))
            {
                string mode = Properties["eaconfig.optimization.ltcg.mode"] ?? ConfigOptionSet.Options["optimization.ltcg.mode"];
                string fileName = Properties["eaconfig.optimization.ltcg.fileName"] ?? ConfigOptionSet.Options["optimization.ltcg.fileName"];
                switch (mode)
                {
                    case "usesampledprofile":
                        AddOption(OptionType.link_Options, "-LTCG");
                        // The /spgo flag must be specified on the command line manually as there is no
                        // Visual Studio property currently defined that sets this flag
                        AddOption(OptionType.link_Options, "/spgo");
                        // The /spdin flag must be specified on the command line manually as there is no
                        // Visual Studio property currently defined that sets this switch
                        AddOptionIf("generatedll", "off", OptionType.link_Options, "/spdin:\"" + fileName + "\"");
                        break;
                    case "generatesampledprofile":
                        AddOption(OptionType.link_Options, "-LTCG");
                        // The /spgo flag must be specified on the command line manually as there is no
                        // Visual Studio property currently defined that sets this flag
                        AddOption(OptionType.link_Options, "/spgo");
                        // The /spd flag must be specified on the command line manually as there is no
                        // Visual Studio property currently defined that sets this flag
                        // The /spd flag overrides the location of the spd file
                        AddOptionIf("generatedll", "off", OptionType.link_Options, "/spd:\"" + fileName + "\"");
                        break;
                    default:
                        break;
                }
            }

            AddOptionIf("profile", "on", OptionType.cc_Defines, "PROFILE_BUILD");
			AddOptionIf("debugsymbols", "off", OptionType.cc_Options, "-Fd\" \"");

			if (FlagEquals("usedebuglibs", "on") || FlagEquals("profile", "on"))
			{
				if (Properties.Contains("package.CapilanoSDK.profilelib"))
				{
					foreach (var lib in Properties["package.CapilanoSDK.profilelib"].LinesToArray())
					{
						AddOption(OptionType.link_Libraries, lib);
					}
				}
			}

			// ----- GENERAL OPTIONS ----- 
			AddOption(OptionType.cc_Options, "-bigobj"); // Allows object files to hold more than 2^16 addressable sections.
			AddOption(OptionType.cc_Options, "-arch:AVX"); // Defaults to SSE2 on x64, MS recommends AVX for Capilano
			AddOption(OptionType.cc_Defines, "_UITHREADCTXT_SUPPORT=0"); // Defines a property added by Visual Studio upon boot which unless defined will prevent builds from succeeding if they depend on ppltasks.h
            // AddOptionIf("optimization.ltcg", "on", OptionType.link_Options, "/d2:-FuncCache0"); //Work around for Linker crash on VS2017.  Issue was reported to Microsoft and they were unlikely to patch the VS2017 toolchain.  Microsoft said that this flag does not affect the code that is generated just affects the speed at which LTCG takes to run.
            AddOptionIf("optimization.ltcg", "on", OptionType.link_Options, "/d2:-notypeopt"); //Work around for Linker crash on VS2017.  Issue was reported to Microsoft and they were unlikely to patch the VS2017 toolchain.  Microsoft said that this flag does not affect the code that is generated just affects the speed at which LTCG takes to run.
        }
	}
}
