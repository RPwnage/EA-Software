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

namespace EA.EAConfig
{
	[TaskName("ps5-clang-generate-options")]
	public class Ps5_Clang_GenerateOptions : GeneratePlatformOptions
	{
		protected override void SetPlatformOptions()
		{
			// Precompiled header support
			if (Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
			{
				// Note that clang command line need to have create-pch to set -o to point to the .pch instead of .obj.  This is 
				// done in the cc.template.pch.commandline property.
				if (FlagEquals("clanguage-strict","on") && FlagEquals("clanguage","on"))
					AddOptionIf("create-pch", "on", OptionType.cc_Options, "-x c-header");
				else
					AddOptionIf("create-pch", "on", OptionType.cc_Options, "-x c++-header");
				AddOptionIf("use-pch", "on", OptionType.cc_Options, "-include-pch \"%pchfile%\"");

				//In case precompiled header is defined directly through cc options
				if (ContainsOption(OptionType.cc_Options, "-x c-header") || ContainsOption(OptionType.cc_Options, "-x c++-header"))
				{
					// Need to set this flag so that nant build can schedule the file with this flag to be built first.
					ConfigOptionSet.Options["create-pch"] = "on";
				}
				else if (ContainsOption(OptionType.cc_Options,"-include-pch"))
				{
					// Need to make sure this is set so that module set data will setup the PrecompiledFile variable.
					ConfigOptionSet.Options["use-pch"] = "on";
				}
			}

			AddOptionIf("buildlogcoloring", "on", OptionType.cc_Options, "-fcolor-diagnostics"); // Enable Color Diagnostic Build Output.
			AddOptionIf("buildlogcoloring", "on", OptionType.cc_Options, "-fansi-escape-codes"); // Needed when enabling Color Diagnostic Build Output.
			AddOption(OptionType.cc_Options, "-fno-elide-type"); // Causes Clang to output better detail on types in Template warning/error messages			
			
			if (FlagEquals("clanguage-strict", "on"))
            {
                AddOptionIf("clanguage", "on", OptionType.cc_Options, "-x c");
			}
			if (FlagEquals("cpplanguage-strict", "on"))
			{
                AddOptionIf("clanguage", "off", OptionType.cc_Options, "-x c++");
            }
			
			if (FlagEquals("debugsymbols", "on"))
			{
				AddOption(OptionType.cc_Options, "-g");
				AddOptionIf("debugsymbols.linetablesonly", "on", OptionType.cc_Options, "-gline-tables-only");
			}
			
			// According to the Sony docs, fno-rtti and fno-exceptions are the default options
			// this is an example of where Sony's version of clang has different default values than the open source version
			if (FlagEquals("clanguage", "off"))
			{
				AddOptionIf("rtti", "on", OptionType.cc_Options, "-frtti");
				AddOptionIf("exceptions", "on", OptionType.cc_Options, "-fexceptions");
			}

			Common_CppLanguage_Version_Options();

			// Enable the clang timing report to help diagnose compile-time performance issues
			AddOptionIf("buildtiming", "on", OptionType.cc_Options, "-ftime-report");

			// By default, PS4 and XB1 have link time optimization on in retail
			if (FlagEquals("retailflags", "on") && ConfigOptionSet.Options["optimization.ltcg"] == null)
			{
				ConfigOptionSet.Options["optimization.ltcg"] = "on";
			}

			if (FlagEquals("optimization", "on"))
			{
				AddOption(OptionType.cc_Options, "-O3");  // Set optimization level 3

				// add link time optimization which is whole-program optimization similar to MSVC's ltcg flag
				// Note: frostbite build times increase significantly with this flag (2-4x), but game teams want the option to turn this on.
				AddOptionIf("optimization.ltcg", "on", OptionType.cc_Options, "-flto");
			}
			
			// Setup profile code coverage flags
			if (FlagEquals("codecoverage", "on"))
			{
				AddOption(OptionType.cc_Options, "-fcoverage-mapping");
				AddOption(OptionType.cc_Options, "-fprofile-instr-generate=" + 
					(Properties["eaconfig.codecoverage-profile"] ?? ConfigOptionSet.Options["codecoverage-profile"] ?? "/hostapp/coverage.profraw"));
			}

			// Setup profile guided optimization flags
			if (FlagEquals("optimization.ltcg", "on"))
			{
				string mode = Properties["eaconfig.optimization.ltcg.mode"] ?? ConfigOptionSet.Options["optimization.ltcg.mode"];
				string fileName = Properties["eaconfig.optimization.ltcg.fileName"] ?? ConfigOptionSet.Options["optimization.ltcg.fileName"];
				switch (mode)
				{
					case "normal":
						break;
					case "instrument":
						if (String.IsNullOrEmpty(fileName))
						{
							AddOption(OptionType.cc_Options, "-fprofile-instr-generate");
							AddOption(OptionType.link_Options, "-fprofile-instr-generate");
						}
						else
						{
							AddOption(OptionType.cc_Options, "-fprofile-instr-generate=" + fileName);
							AddOption(OptionType.link_Options, "-fprofile-instr-generate=" + fileName);
						}
						break;
					case "useprofile":
						AddOption(OptionType.cc_Options, "-fprofile-instr-use=" + fileName);
						AddOption(OptionType.link_Options, "-fprofile-instr-use=" + fileName);
						break;
				}
			}

			// ----- WARNING OPTIONS ----- 
			if (FlagEquals("warnings", "on"))
			{
				// note: -W4 is not supported on clang, medium is set to frostbite's traditional default which is no flag.
				if (FlagEquals("warninglevel", "high"))
				{
					// turn on all warnings, traditionally used by eaconfig
					PrependOption(OptionType.cc_Options, "-Wall");
				}
				else if (FlagEquals("warninglevel", "pedantic"))
				{
					// turns on addition diagnostic warnings
					PrependOption(OptionType.cc_Options, "-Wall -Weverything -pedantic");
				}

				if (DefaultWarningSuppression == WarningSuppressionModes.On)
				{
					// historically we disabled these due to compiler issues which have long since been resolved, right now this can be enanled explicitly 
					// but in future we should consider setting -Wno-format-security instead which would allow compiler to catch formatting mistakes
					SetClangWarningIfNotAlreadySet("format", enabled: false);   
					SetClangWarningIfNotAlreadySet("unneeded-internal-declaration", enabled: false);

					// Below warning is only available in SDK 2.0 and beyond version of clang
					var newToolsVersion = new Version("2.00.00");
					var installedVersion = new Version(Project.GetPropertyOrDefault("package.ps5sdk.FullInstalledVersion", "1.00.00"));
					if (installedVersion >= newToolsVersion)
					{
						SetClangWarningIfNotAlreadySet("misleading-indentation", enabled: false);
					}

					AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.ps5-clang.warningsuppression", ""));
				}
				else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
				{
					AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.ps5-clang.warningsuppression", ""));
				}
			}
			else if (FlagEquals("warnings", "off"))
			{
				PrependOption(OptionType.cc_Options, "-Wno-everything");
			}

			if (FlagEquals("warnings", "on") || FlagEquals("warnings", "custom"))
			{
				AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Werror"); // Treat warnings as errors
				//According to the migration guide, -Werror should be replaced with --fatal-warnings, but it's unrecognized as an argument. Leaving this here as a reminder to revisit
				//AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "--fatal-warnings"); // Treat warnings as errors. Previously -Werror on PS4.
			}

			// SN-DBS deliberately down-cases file paths when it copies files to remote machines. So, make sure to disable the nonportable-include-path warning.
			// https://p.siedev.net/support/issue/10827/_SN-DBS_build_errors_when_using_-Wnonportable-include-path
			AddOptionIf("enablesndbs", "on", OptionType.cc_Options, "-Wno-nonportable-include-path");

			// ----- GENERAL OPTIONS ----- 
			//AddOptionIf("misc", "on", OptionType.cc_Options, "-G0"); // Put global and static data smaller than

			// ----- OUTPUT OPTIONS ----- 
			AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");

			if (FlagEquals("retailflags", "on") || PropertyUtil.GetPropertyOrDefault(Project, "config-name", "") == "final")
			{
				AddOption(OptionType.link_Options, "--gc-sections"); // Previously --strip-unused-data on PS4
			}

			AddOptionIf("generatemapfile", "on", OptionType.link_Options, "--Map=\"%linkoutputmapname%\"");
		}
	}
}
