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
using System.Linq;

namespace EA.nxConfig
{
	[TaskName("GenerateOptions_nx")]
	public class GenerateOptions : GeneratePlatformOptions
	{
		protected override void SetPlatformOptions()
		{
			Common_Clang_Options();

			AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g"); // Generate debug information
			
			if (FlagEquals("clanguage", "off"))
			{
				AddOptionIf("rtti", "on", OptionType.cc_Options, "-frtti");

			  //AddOptionIf("exceptions", "on", OptionType.cc_Options, "-fexceptions");
				AddOption(OptionType.cc_Options, "-fno-exceptions");  // NOTE(rparolin): exceptions must be turned off as they are part of the type-system and cause function look-up issues that we need to fix.
			}

			AddOption(OptionType.cc_Options, "-MP");

			AddOptionIf("optimization", "on", OptionType.cc_Options, "-O3");  // Set optimization level 3

			AddOption(OptionType.cc_Options, "-Wno-format"); // Disable warnings related to printf formats because nx's clang has a broken defintion of PRNd64 and SCNd64.
			AddOption(OptionType.cc_Options, "-Wno-unneeded-internal-declaration");

			if (FlagEquals("shortchar", "off"))
			{
				AddOption(OptionType.cc_Options, "-fno-short-wchar");
			}
			else if (FlagEquals("shortchar", "on"))
			{
				NAnt.Core.Tasks.DoOnce.Execute("nx_setting_short_wchar_warning", () =>
					{
						Project.Log.Warning.WriteLine("Option 'shortchar' was explicitly being set to 'on' to set -fshort-wchar compiler option.  This is actually a \"Prohibited Compiler Option\" in official NX documentation.  You are using this at your own risk!");
					}
				);
				AddOption(OptionType.cc_Options, "-fshort-wchar");
			}
			
			// ----- GENERAL OPTIONS ----- 
			//AddOptionIf("misc", "on", OptionType.cc_Options, "-G0"); // Put global and static data smaller than

			// ----- OUTPUT OPTIONS ----- 
			AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");

			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");
		}
	}
}
