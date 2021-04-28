// (c) Electronic Arts. All rights reserved.

using NAnt.Core.Attributes;

using EA.Eaconfig;

namespace EA.KettleConfig
{
	[TaskName("kettle-clang-generate-options", XmlSchema = false)]
	public class Kettle_Clang_GenerateOptions : GeneratePlatformOptions
	{
		protected override void SetPlatformOptions()
		{
			// By default, PS4 and XB1 have link time optimization on in retail
			if (FlagEquals("retailflags", "on") && ConfigOptionSet.Options["optimization.ltcg"] == null)
			{
				ConfigOptionSet.Options["optimization.ltcg"] = "on";
			}

			Common_Clang_Options();	

			if (FlagEquals("debugsymbols", "on"))
			{
				AddOption(OptionType.cc_Options, "-g");
			}
			
			// According to the Sony docs, fno-rtti and fno-exceptions are the default options
			// this is an example of where Sony's version of clang has different default values than the open source version
			if (FlagEquals("clanguage", "off"))
			{
				AddOptionIf("rtti", "on", OptionType.cc_Options, "-frtti");
				AddOptionIf("exceptions", "on", OptionType.cc_Options, "-fexceptions");
			}

			if (FlagEquals("optimization", "on"))
			{
				AddOption(OptionType.cc_Options, "-O3");  // Set optimization level 3

				// add link time optimization which is whole-program optimization similar to MSVC's ltcg flag
				// Note: frostbite build times increase significantly with this flag (2-4x), but game teams want the option to turn this on.
				AddOptionIf("optimization.ltcg", "on", OptionType.cc_Options, "-flto");
			}

			// See Engine.Base/StaticInitRegister.h for an explanation of these options
			// AddOption(OptionType.link_Options, "--whole-archive");
			if (FlagEquals("optimization", "on"))
			{
				// Enabled by default on Orbis-Clang, let's be explicit
				AddOption(OptionType.cc_Options, "-fdata-sections");
				AddOption(OptionType.cc_Options, "-ffunction-sections");

				// These Options only added extra 20sec to release build link-time
				// Since most people build release nice to know if things are stripped in there instead of waiting until retail/final builds
				AddOption(OptionType.link_Options, "--strip-unused");
				AddOption(OptionType.link_Options, "--strip-unused-data");

				AddOption(OptionType.link_Options, "--dont-strip-section=.fbglobalregisterdata");
				AddOption(OptionType.link_Options, "--dont-strip-section=.fbglobalconstregisterdata");

				// This destroys link-time; maybe make an option for it but it is SN Sony Linker Specific
				// AddOption(OptionType.link_Options, "--strip-report=\"%outputdir%\\strip-report.txt\"");
			}
			
			// ----- WARNING OPTIONS ----- 
			if (FlagEquals("warnings", "on"))
			{
				if (DefaultWarningSuppression == WarningSuppressionModes.On) // TODO move to Common_Clang_Options?
				{
					// historically we disabled these due to compiler issues which have long since been resolved, right now this can be enanled explicitly 
					// but in future we should consider setting -Wno-format-security instead which would allow compiler to catch formatting mistakes
					SetClangWarningIfNotAlreadySet("format", enabled: false);   
					SetClangWarningIfNotAlreadySet("unneeded-internal-declaration", enabled: false);
				}
			}
			else if (FlagEquals("warnings", "off"))
			{
				PrependOption(OptionType.cc_Options, "-Wno-everything");
			}

			if (FlagEquals("warnings", "on") || FlagEquals("warnings", "custom"))
			{
				AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Werror"); // Treat warnings as errors 
			}

			// SN-DBS deliberately down-cases file paths when it copies files to remote machines. So, make sure to disable the nonportable-include-path warning.
			// https://p.siedev.net/support/issue/10827/_SN-DBS_build_errors_when_using_-Wnonportable-include-path
			AddOptionIf("enablesndbs", "on", OptionType.cc_Options, "-Wno-nonportable-include-path");

			// This prints timing out for each stage of the linker. Which can be helpful in optimizing link times.
			AddOptionIf("print_link_timings", "on", OptionType.link_Options, "--time-report");

			// ----- OUTPUT OPTIONS ----- 
			AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");

			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "--Map=\"%linkoutputmapname%\"");
			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "--sn-full-map");	// Include symbols for static functions and static variables in PS4 map files.  This produces more accurate results when translating callstacks/addresses with symmogrifier (e.g. BugSentry)
		}
	}
}
