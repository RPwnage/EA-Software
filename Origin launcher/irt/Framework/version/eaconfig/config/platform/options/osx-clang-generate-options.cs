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
	[TaskName("osx-64-clang-generate-options")]
	public class Clang64GenerateOptions : ClangGenerateOptions
	{
	}

	[TaskName("osx-x64-clang-generate-options")]
	public class ClangX64GenerateOptions : ClangGenerateOptions
	{
	}

	[TaskName("osx-clang-generate-options")]
	public class ClangGenerateOptions : GeneratePlatformOptions
	{
		protected override void SetPlatformOptions()
		{
			Common_Clang_Options();

			if (FlagEquals("debugsymbols", "on"))
			{
				AddOption(OptionType.cc_Options, "-g");
				AddOptionIf("debugsymbols.linetablesonly", "on", OptionType.cc_Options, "-gline-tables-only");
			}

			// Some versions of Runtime do not have MacOSX definition. Use integer value 6
			AddOption(OptionType.cc_Defines, "EA_PLATFORM_OSX=1");

			if (Properties["config-compiler"] == "clang")
			{
				if (FlagEquals("clanguage", "off"))
				{
					// The above Common_Clang_Options() should have added "cc.std" already.  Do a test just in case!
					if (ConfigOptionSet.Options.ContainsKey("cc.std"))
					{
						string stdVer = ConfigOptionSet.Options["cc.std"];

						// We need both compiler and linker to have the following switch because we still need 
						// the compiler to point to the consistent C++ template library.
						if (stdVer.StartsWith("c++"))
						{
							// We should be using the Clang libc++ libs for C++11 only if deployment target is osx 10.7 or later.  
							// Otherwise, user's OS might not have the correct system library and xcode will give you a build error as well even if you try.
							string deploySdkVer = Properties.GetPropertyOrDefault("osx-deployment-target-version", "10.6");
							if (deploySdkVer.StrCompareVersions("10.7") >= 0)
							{
								// We need both compiler and linker to have the following switch because we still need
								// the compiler to point to the correct Clang stdlib instead of the GNU stdlib.
								AddOption(OptionType.cc_Options, "-stdlib=libc++");
								AddOption(OptionType.link_Options, "-stdlib=libc++");
							}
							else
							{
								// Deployment version older than 10.7 must use GNU stdlib.
								AddOption(OptionType.cc_Options, "-stdlib=libstdc++");
								AddOption(OptionType.link_Options, "-stdlib=libstdc++");
							}
						}
						else if (stdVer.StartsWith("gnu++"))
						{
							AddOption(OptionType.cc_Options, "-stdlib=libstdc++");
							AddOption(OptionType.link_Options, "-stdlib=libstdc++");
						}
					}
				}
			}

			// ----- GENERAL OPTIONS -----
			AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file
			AddOption(OptionType.cc_Options, "-msse4.2"); // required because we use SSE instructions in metrohash and Engine.Render.Core2.PlatformNull projects.

			if (FlagEquals("misc", "on"))
			{
				AddOption(OptionType.cc_Options, "-c");             // Compile only
				AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
				// rather than as common blocks.  If a variable is declared in two different
				// compilations, it will cause a link error
				AddOption(OptionType.cc_Options, "-fmessage-length=0");  // Put the output entirely on one line; don't add newlines
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

			// In the following code, we explicitly not use FlagEquals("enable_objc_arc", "on") directly and test for presence of
			// that optionset first because we need to make sure optionset's value take precedence over property specification.
			// Not every project can build with -fobjc-arc option.  So if we allow user to have a property to turn on -fobjc-arc
			// for ALL projects, we need to allow modules specify local optionset override to turn it off.  That's why we need
			// to disable the "eaconfig.enable_objc_arc" property support (which FlagEquals will take property settings as precendence).  
			// Because we are using non-standard global property support for "enable_objc_arc", I created a new "iphone.enable_objc_arc" 
			// property support to differentiate default behaviour of "eaconfig.XXXX" properties.
			if (ConfigOptionSet.Options.Contains("enable_objc_arc"))
			{
				if (FlagEquals("enable_objc_arc", "on"))
				{
					// Project building with -fobjc-arc need to link with -fobjc-arc as well in order for
					// program modules to properly link with required libraries.
					AddOption(OptionType.cc_Options, "-fobjc-arc");
					AddOption(OptionType.link_Options, "-fobjc-arc");
				}
				else if (FlagEquals("enable_objc_arc", "off"))
				{
					// If the project is not ARC-enabled, linking with ARC library should not have
					// any impact.  So no need to add linker flag.
					AddOption(OptionType.cc_Options, "-fno-objc-arc");
				}
			}
			else
			{
				string global_objc_arc_opt = Project.Properties.GetPropertyOrDefault("osx.enable_objc_arc", null);
				if (!String.IsNullOrEmpty(global_objc_arc_opt))
				{
					string opt_lower = global_objc_arc_opt.ToLowerInvariant();
					if (opt_lower == "true" || opt_lower == "on")
					{
						// Project building with -fobjc-arc need to link with -fobjc-arc as well in order for
						// program modules to properly link with required libraries.
						AddOption(OptionType.cc_Options, "-fobjc-arc");
						AddOption(OptionType.link_Options, "-fobjc-arc");
					}
					else if (opt_lower == "false" || opt_lower == "off")
					{
						// If the project is not ARC-enabled, linking with ARC library should not have
						// any impact.  So no need to add linker flag.
						AddOption(OptionType.cc_Options, "-fno-objc-arc");
					}
				}
			}

			//-- Optimization --
			AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
			AddOptionIf("optimization", "on", OptionType.cc_Options, "-O3");
			// Note that support for custom optimization setting is already setup in parent function that
			// call this function.  It should be setup like the followings already:
			// AddOptionIf("optimization", "custom", OptionType.cc_Options, ConfigOptionSet.Options["optimization.custom.cc"]);
			// AddOptionIf("optimization", "custom", OptionType.link_Options, ConfigOptionSet.Options["optimization.custom.link"]);
			// AddOptionIf("optimization", "custom", OptionType.lib_Options, ConfigOptionSet.Options["optimization.custom.lib"]);

			if (FlagEquals("optimization.ltcg", "on"))
			{
				AddOption(OptionType.cc_Options, "-flto=thin");         //enable link-time code generation
				AddOption(OptionType.link_Options, "-flto=thin");       //enable link-time code generation      
			}

			// ----- LINK OPTIONS ----- 
			AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
			AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");

			// ----- OUTPUT OPTIONS ----- 
			AddOptionIf("generatedll", "on", OptionType.link_Options, "-dynamiclib"); //Make a DLL, not an exe.
			// For dynamic lib on osx, we need to provide a -install_name linker option to set the install name so that any executable that is
			// linked with this dylib will set LC_LOAD_DYLIB to use this path instead of using the dynamic library's full path during link time.
			AddOptionIf("generatedll", "on", OptionType.link_Options, "-Wl,-install_name,\"@rpath/%outputname%" + Project.Properties["dll-suffix"] + "\"");
			AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
			// The following linker switch set LC_RPATH symbol to @loader_path/ to allow the built executable to search for shared library from
			// the executable's folder.  Note that if the executable is built as an app bundle, people will need to add extra rpath themselves to
			// where they copy the dylib to inside the app bundle folder structure.
			AddOption(OptionType.link_Options, "-Wl,-rpath,'@loader_path/'");
			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-map,\"%linkoutputmapname%\"");
			AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
			AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

			if (FlagEquals("warningsaserrors", "on"))
			{
				if (ContainsOption(OptionType.cc_Options, "objective-c"))
				{
					// Treat deprecated warning as warning
					AddOption(OptionType.cc_Options, "-Wno-error=deprecated-declarations");
				}
			}

			if (FlagEquals("warnings", "on"))
			{
				if (DefaultWarningSuppression == WarningSuppressionModes.On)
				{
					AddOption(OptionType.cc_Options, "-Wno-unused-private-field");
					//AddOption(OptionType.cc_Options, "-Wno-implicit-exception-spec-mismatch");

					AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
				}
				else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
				{
				}
				else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
				{
					AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
				}
			}
		}
	}
}
