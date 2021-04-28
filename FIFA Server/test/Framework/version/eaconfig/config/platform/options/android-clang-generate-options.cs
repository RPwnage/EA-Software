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

namespace EA.AndroidConfig
{
	[TaskName("GenerateOptions_android")]
	public class Android_GenerateOptions : GeneratePlatformOptions
	{
		protected override void SetPlatformOptions()
		{
			Common_Clang_Options();

			AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
			AddOptionIf("optimization", "on", OptionType.cc_Options, "-O3"); //Set optimization level 3

			// Android's own headers have attributes that Clang cannot recognize.
			AddOption(OptionType.cc_Options, "-Wno-unknown-attributes"); // new warnings, They trip up AndroidNDK's own headers.

			AddOptionIf("optimization", "on", OptionType.cc_Options, "-fomit-frame-pointer"); //Omit frame pointer when possible
			AddOptionIf("optimization", "on", OptionType.cc_Options, "-fno-stack-protector"); // Don't emit extra code to check for buffer overflows, such as stack smashing attacks. This is done by adding a guard variable to functions with vulnerable objects. This includes functions that call alloca, and functions with buffers larger than 8 bytes. The guards are initialized when a function is entered and then checked when the function exits. If a guard check fails, an error message is printed and the program exits.

			if (FlagEquals("debugsymbols", "off"))
			{
				AddOption(OptionType.cc_Options, "-g0");
			}
			else
			{
				AddOption(OptionType.cc_Options, "-g2");
				AddOption(OptionType.cc_Options, "-ggdb");
			}

			AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
			AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

			if (FlagEquals("misc", "on"))
			{
				AddOption(OptionType.cc_Options, "-c");				//Compile only
				AddOption(OptionType.cc_Options, "-fno-common");	// In C, allocate even uninitialized global variables in data section
																	// rather than as common blocks.  If a variable is declared in two different                                
																	// compilations, it will cause a link error
				AddOption(OptionType.cc_Options, "-fmessage-length=0"); // Put the output entirely on one line; don't add newlines.
			}

			if (FlagEquals("shortchar", "off"))
			{
				AddOption(OptionType.cc_Options, "-fno-short-wchar");
			}
			else
			{
				// default case since prior to this option being added to eaconfig it would default to on
				// Override the underlying type for wchar_t to `unsigned short
				AddOption(OptionType.cc_Options, "-fshort-wchar");
			}

			if (FlagEquals("clanguage", "off"))
			{
				if (FlagEquals("rtti", "off"))
				{
					AddOption(OptionType.cc_Options, "-fno-rtti"); //Do not generate run time type information
				}
				if (FlagEquals("rtti", "on"))
				{
					AddOption(OptionType.cc_Options, "-frtti"); //Generate run time type information
					AddOption(OptionType.link_Options, "-frtti"); //Generate run time type information
				}

				if (FlagEquals("exceptions", "on"))
				{
					AddOption(OptionType.cc_Options, "-fexceptions");
					AddOption(OptionType.link_Options, "-fexceptions");
					AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
					if (!FlagEquals("rtti", "on"))
					{
						AddOption(OptionType.link_Options, "-frtti"); //Generate run time type information
					}
				}
			}

			// ----- OUTPUT OPTIONS -----
			AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
			AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");

			// ----- LINK OPTIONS ----- 
			AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
			AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");
		}
	}
}
