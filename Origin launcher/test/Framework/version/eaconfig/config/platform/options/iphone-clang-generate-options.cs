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
	// The generate option tasks is executed by finding ${config-platform}-generate-options in load config functions.  At present,
	// iphone-arm64-clang-* and iphonesim-x64-clang-* configs set config-platform to "iphone-clang".  The following 
	// iphone-arm64-clang-generate-options and iphone-x64-clang-generate-options are provided just in case we make changes to
	// config-platform property in the future and try to future proof this workflow.

	[TaskName("iphone-arm64-clang-generate-options")]
	public class Iphone_Arm43_Clang_GenerateOptions : Iphone_Clang_GenerateOptions
	{
	}

	[TaskName("iphone-x64-clang-generate-options")]
	public class Iphone_x64_Clang_GenerateOptions : Iphone_Clang_GenerateOptions
	{
	}

	[TaskName("iphone-clang-generate-options")]
	public class Iphone_Clang_GenerateOptions : GeneratePlatformOptions
	{
		protected override void SetPlatformOptions()
		{
			// Xcode 5.1 fails if strip symbols is turned on
			ConfigOptionSet.Options["stripallsymbols"] = "off";

			// Calling a base class function first (setup in Framework itself).
			Common_Clang_Options();

			AddOption(OptionType.cc_Options, Project.Properties["package.iphonesdk.cc.options"]);
			AddOption(OptionType.link_Options, Project.Properties["package.iphonesdk.link.options"]);

			if (FlagEquals("debugsymbols", "on"))
			{
				AddOption(OptionType.cc_Options, "-g");
				AddOptionIf("debugsymbols.linetablesonly", "on", OptionType.cc_Options, "-gline-tables-only");
			}

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
						AddOption(OptionType.cc_Options, "-stdlib=libc++");
						AddOption(OptionType.link_Options, "-stdlib=libc++");
					}
					else if (stdVer.StartsWith("gnu++"))
					{
						AddOption(OptionType.cc_Options, "-stdlib=libstdc++");
						AddOption(OptionType.link_Options, "-stdlib=libstdc++");
					}
				}
			}

			// ----- GENERAL OPTIONS ----- 
			AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file

			if (FlagEquals("misc", "on"))
			{
				AddOption(OptionType.cc_Options, "-c");             //Compile only
				AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
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

			if (FlagEquals("enable_bitcode", "on"))
			{
				AddOptionIf("bitcode_generation_mode", "bitcode", OptionType.cc_Options, "-fembed-bitcode");
				AddOptionIf("bitcode_generation_mode", "marker", OptionType.cc_Options, "-fembed-bitcode-marker");
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
				string global_objc_arc_opt = Project.Properties.GetPropertyOrDefault("iphone.enable_objc_arc", null);
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

			//(NOTE): -mdynamic-no-pic does not work with dll builds.
			if (Project.Properties.GetBooleanPropertyOrDefault("package.iphone.nopic", false))
			{
				AddOptionIf("optimization", "on", OptionType.cc_Options, "-mdynamic-no-pic");  // Generates code that is not position-independent but has position-independent external references
				//dll builds want position independent addresses for globals/statics
				AddOptionIf("generatedll", "on", OptionType.link_Options, "-read_only_relocs suppress");
			}

			AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
			AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

			// ----- OUTPUT OPTIONS ----- 
			AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
			AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-map,\"%linkoutputmapname%\"");

			//IM: remove this. This is only for compatibility with older iphonesdk
			if (Project.Properties.GetBooleanPropertyOrDefault("package.iphonesdk.usexcrun", false) && !String.IsNullOrEmpty(Project.GetPropertyValue("cc-clanguage.template.commandline")))
			{

				if (FlagEquals("clanguage", "on"))
				{
					ConfigOptionSet.Options["cc.template.commandline"] = Project.GetPropertyValue("cc-clanguage.template.commandline");
					ConfigOptionSet.Options["link.template.commandline"] = Project.GetPropertyValue("link-clanguage.template.commandline");
				}
			}

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
