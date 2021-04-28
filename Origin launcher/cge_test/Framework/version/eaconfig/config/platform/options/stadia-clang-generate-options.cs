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
	[TaskName("stadia-clang-generate-options", XmlSchema = false)]
	public class Stadia_Clang_Common_Options : GeneratePlatformOptions
	{
		protected override void SetPlatformOptions()
		{
			Common_Clang_Options();

			AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
			AddOptionIf("optimization", "on", OptionType.cc_Options, "-O3");
			AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
			AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

			if (FlagEquals("debugsymbols", "on"))
			{
				AddOption(OptionType.cc_Options, "-ggdb");
				AddOptionIf("debugsymbols.linetablesonly", "on", OptionType.cc_Options, "-gline-tables-only");
			}

			if (OsUtil.PlatformID == (int)PlatformID.Unix || 
			    OsUtil.PlatformID == (int)PlatformID.Win32NT)    // We're doing stadia cross build on PC.
			{
				AddOption(OptionType.cc_Defines, "__yeti__");
				AddOption(OptionType.cc_Defines, "_GNU_SOURCE");
				AddOption(OptionType.cc_Defines, "VK_USE_PLATFORM_GGP");
				AddOption(OptionType.cc_Defines, "EA_PLATFORM_LINUX=1");
				AddOption(OptionType.cc_Defines, "EA_PLATFORM_STADIA=1");

				if (!FlagEquals("generatedll", "on"))
				{
					AddOptionIf("optimization", "on", OptionType.link_Options, "-Wl,--gc-sections"); //strip out unused functions in the output binary and make it smaller
				}
			}

			// ----- GENERAL OPTIONS -----
			if (FlagEquals("misc", "on"))
			{
				AddOption(OptionType.cc_Options, "-c");             //Compile only
				AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
				// rather than as common blocks.  If a variable is declared in two different
				// compilations, it will cause a link error
				AddOption(OptionType.cc_Options, "-fmessage-length=0");  // Put the output entirely on one line; don't add newlines
			}

			// ----- GENERAL OPTIONS ----- 
			var useMpThreads = ConfigOptionSet.Options["multiprocessorcompilation"]??Properties["eaconfig.build-MP"];
			if (useMpThreads != null)
			{
				useMpThreads = useMpThreads.TrimWhiteSpace();

				if (String.IsNullOrEmpty(useMpThreads) || (useMpThreads.IsOptionBoolean() && useMpThreads.OptionToBoolean()))
				{
					AddOption(OptionType.cc_Options, "-MP");
				}
				else if(!useMpThreads.IsOptionBoolean())
				{
					int threads;
					if (!Int32.TryParse(useMpThreads, out threads))
					{
						Log.Warning.WriteLine("eaconfig.build-MP='{0}' value is invalid, it can be empty or contain integer number of threads to use with /MP option : /MP${{eaconfig.build-MP}}. Adding '/MP' option with no value.", useMpThreads);

						useMpThreads = String.Empty;
					}
					AddOption(OptionType.cc_Options, "-MP" + useMpThreads);
				}
			}

			AddOption(OptionType.cc_Options, Project.Properties["package.stadiasdk.cc.options"]);
			AddOption(OptionType.cc_Options, "-MD"); //Dump a .d file

			// ----- LINK OPTIONS -----
			AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
			AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");
			AddOption(OptionType.link_Options, Project.Properties["package.stadiasdk.link.options"]);
			AddOptionIf("exportdynamic", "on", OptionType.link_Options, "-Wl,--export-dynamic");

			// ----- OUTPUT OPTIONS -----
			AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
			AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");
		}
	}
}
