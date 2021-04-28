// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Xml;
using System.Text;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace EA.Eaconfig
{
	public enum WarningSuppressionModes { On, Off, Strict, Custom };

	[TaskName("GenerateOptions")]
	public abstract class GeneratePlatformOptions : EAConfigTask
	{
		protected enum OptionType : int
		{
			cc_Options = 0,
			cc_Defines = 1,
			link_Options = 2,
			link_Libraries = 3,
			link_Librarydirs = 4,
			lib_Options = 5,
			cc_CompilerInternalDefines = 6
		}

		private static readonly string[] _optionNames = new string[] { "cc.options", "cc.defines", "link.options", "link.libraries", "link.librarydirs", "lib.options", "cc.compilerinternaldefines" };
		private StringBuilder[] _optionBuilder = new StringBuilder[] { new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder() };

		[TaskAttribute("configsetname", Required = true)]
		public string ConfigSetName { get; set; } = String.Empty;

		public OptionSet ConfigOptionSet
		{
			get
			{
				if (_ConfigOptionSet == null)
				{
					_ConfigOptionSet = Project.NamedOptionSets[ConfigSetName];
					if (_ConfigOptionSet == null)
					{
						throw new BuildException($"[GenerateOptions] ${Location}: OptionSet '{ConfigSetName}' does not exist.");
					}
				}
				return _ConfigOptionSet;
			}
			set { _ConfigOptionSet = value; }
		}
		private OptionSet _ConfigOptionSet = null;

		public static void Execute(Project project, OptionSet configOptionSet, string configsetName)
		{
			GeneratePlatformOptions task = null;

			string config_platform = configOptionSet.GetOptionOrDefault("config-platform", project.Properties["config-platform"]);

			if (!string.IsNullOrEmpty(config_platform))
			{
				task = LoadPlatform_GenerateOptionsTask(project, config_platform + "-generate-options", configOptionSet, configsetName);
			}

			// For backwards compatibility check system prefix:
			if (task == null)
			{
				string config_system = configOptionSet.GetOptionOrDefault("config-system", project.Properties["config-system"]);

				if (!string.IsNullOrEmpty(config_system))
				{
					task = LoadPlatform_GenerateOptionsTask(project, "GenerateOptions_"+config_system, configOptionSet, configsetName);
				}
			}

			if (task != null)
			{
				task.Execute();
			}
		}

		private static GeneratePlatformOptions LoadPlatform_GenerateOptionsTask(Project project, string taskname, OptionSet configOptionSet, string configsetName)
		{
			GeneratePlatformOptions task = null;

			if (!string.IsNullOrEmpty(taskname))
			{
				task = project.TaskFactory.CreateTask(taskname, project) as GeneratePlatformOptions;
				if (task != null)
				{
					task.Init(project, configOptionSet, configsetName);
				}
			}

			return task;
		}

		protected GeneratePlatformOptions() : base() { }

		public GeneratePlatformOptions(Project project, OptionSet configOptionSet, string configsetName)
			: base(project)
		{
			_ConfigOptionSet = configOptionSet;
			ConfigSetName = configsetName;
			InternalInit();
		}

		protected override void InitializeTask(XmlNode taskNode)
		{
			base.InitializeTask(taskNode);
			InternalInit();
		}

		protected override void ExecuteTask()
		{
			// Init Builders
			foreach (OptionType type in Enum.GetValues(typeof(OptionType)))
			{
				string option = ConfigOptionSet.Options[_optionNames[(int)type]];
				if (!String.IsNullOrWhiteSpace(option))
				{
					_optionBuilder[(int)type].AppendLine(option);
				}
			}

			// --- Common section ---

			AddOptionIf("debugflags", "on", OptionType.cc_Defines, "EA_DEBUG");
			AddOptionIf("debugflags", "on", OptionType.cc_Defines, "_DEBUG");  //First added for Wii.  Needed for VC++ too, as per Paul Pedriana.
			AddOptionIf("debugflags", "off", OptionType.cc_Defines, "NDEBUG");
			AddOptionIf("debugflags", "custom", OptionType.cc_Defines, ConfigOptionSet.Options["debugflags.custom.cc"]);

			AddOptionIf("debugsymbols", "custom", OptionType.cc_Options, ConfigOptionSet.Options["debugsymbols.custom.cc"]);
			AddOptionIf("debugsymbols", "custom", OptionType.link_Options, ConfigOptionSet.Options["debugsymbols.custom.link"]);
			AddOptionIf("debugsymbols", "custom", OptionType.lib_Options, ConfigOptionSet.Options["debugsymbols.custom.lib"]);

			AddOptionIf("optimization", "custom", OptionType.cc_Options, ConfigOptionSet.Options["optimization.custom.cc"]);
			AddOptionIf("optimization", "custom", OptionType.link_Options, ConfigOptionSet.Options["optimization.custom.link"]);
			AddOptionIf("optimization", "custom", OptionType.lib_Options, ConfigOptionSet.Options["optimization.custom.lib"]);

			AddOptionIf("codecoverage", "on", OptionType.cc_Defines, "EA_CODE_COVERAGE");

			AddOptionIf("retailflags", "on", OptionType.cc_Defines, "EA_RETAIL=1");

			// ----- GENERAL OPTIONS -----
			AddOptionIf("misc", "custom", OptionType.cc_Options, ConfigOptionSet.Options["misc.custom.cc"]);

			// ----- LINK OPTIONS -----
			AddOptionIf("usedebuglibs", "custom", OptionType.cc_Options, ConfigOptionSet.Options["usedebuglibs.custom.cc"]);

			// ----- WARNING OPTIONS -----
			AddOptionIf("warnings", "custom", OptionType.cc_Options, ConfigOptionSet.Options["warnings.custom.cc"]);
			AddOptionIf("warnings", "custom", OptionType.link_Options, ConfigOptionSet.Options["warnings.custom.link"]);

			if (FlagEquals("standardsdklibs", "on"))
			{
				AddOptionIf("usedebuglibs", "off", OptionType.link_Libraries, Properties["platform.sdklibs.regular"]);
				AddOptionIf("usedebuglibs", "on", OptionType.link_Libraries, Properties["platform.sdklibs.debug"]);
			}
			else if (FlagEquals("standardsdklibs", "custom"))
			{
				AddOption(OptionType.link_Libraries, ConfigOptionSet.Options["standardsdklibs.custom.link"]);
			}

			SetPlatformOptions();

			// Commit new values
			foreach (OptionType type in Enum.GetValues(typeof(OptionType)))
			{
				if (_optionBuilder[(int)type].Length > 0)
				{
					ConfigOptionSet.Options[_optionNames[(int)type]] = _optionBuilder[(int)type].ToString();
				}
			}

			if (ConfigCompiler == "gcc" || ConfigCompiler == "clang")
			{
				// Generating clang/gcc compiler internal defines must be last
				// after processing all platform specific options as all these
				// other options are required in order for us to query the
				// compiler for the correct internal defines.
				GatherGccClangCompilerInternalDefines();
			}

			// Wireframe uses internal property build-compiler:
			Project.Properties["build-compiler"] = ConfigCompiler;
		}

		protected virtual void SetPlatformOptions()
		{
			// this method will be overriden with platform specific options
			throw new BuildException($"GenerateOptions task for '{ConfigPlatform}' platform not found.", Location);
		}

		protected virtual void Common_CppLanguage_Version_Options(bool vcCompiler=false)
		{
			string switchPrefix = (vcCompiler ? "/std:" : "-std=");

			if (FlagEquals("clanguage", "off") || vcCompiler)		// We don't set clanguage for vc compilers
			{
				string cc_std_prop_value = PropertyUtil.GetPropertyOrDefault(Project, "cc.std", string.Empty);
				string cc_std_platform = "cc.std." + ConfigSystem;
				string cc_std_platform_prop_value = PropertyUtil.GetPropertyOrDefault(Project, cc_std_platform, string.Empty);
				string cc_std_platform_default_value = PropertyUtil.GetPropertyOrDefault(Project, cc_std_platform + ".default", string.Empty);

				// Order of evaluation:
				//    1. User provided global property "cc.std.${config-system}" which will override ALL modules for specific platform
				//    2. User provided their own build options with cc.std.$(config-system) to control only specific modules for specific platform
				//    3. User provided global property "cc.std" which will override ALL modules for all platforms (except above platform specific setting)
				//    4. User provided their own build options with cc.std to control specific modules for all platform (except above platform specific setting)
				//    5. Use setting in global property "cc.std.${config-system}.default" property (initialized in config package) which user can override
				//    6. Hard code to c++17 (except android and stadia still on c++14)

				string assigned_cc_std = "c++17";

				if (!String.IsNullOrEmpty(cc_std_platform_prop_value))
				{
					assigned_cc_std = cc_std_platform_prop_value;
				}
				else if (ConfigOptionSet.Options.Contains(cc_std_platform))
				{
					assigned_cc_std = ConfigOptionSet.Options[cc_std_platform];
				}
				else if (!string.IsNullOrEmpty(cc_std_prop_value))
				{
					assigned_cc_std = cc_std_prop_value;
				}
				else if (ConfigOptionSet.Options.Contains("cc.std"))
				{
					assigned_cc_std = ConfigOptionSet.Options["cc.std"];
				}
				else if (vcCompiler && 
						Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.AllowPreview", false) &&
						!ContainsOption(OptionType.cc_Options, "clr")) // VS2019 Preview 2.0 made /clr and /std:c++latest incompatible)
				{
					//assigned_cc_std = "c++latest";
					assigned_cc_std = "c++17"; // VS2019 Preview 16.6 Preview 2 removed some c++17 deprecated things in c++20/c++latest and we don't have the fixes for all of them right now, switch back later
				}
				else if (!String.IsNullOrEmpty(cc_std_platform_default_value))
				{
					assigned_cc_std = cc_std_platform_default_value;
				}

				AddOption(OptionType.cc_Options, switchPrefix + assigned_cc_std);

				// Create the "cc.std" in optionset so that other functions know what has been set and don't need to duplicate this if-blocks.  This is 
				// important as platforms like iphone need to further set -stdlib= switches to go along this setting.
				ConfigOptionSet.Options["cc.std"] = assigned_cc_std;
			}
			else if (FlagEquals("clanguage", "on"))
			{
				// Same evaluation rule as above but using cc_clanguage.std instead.  And by default we don't set anything if we don't find a match.
				string cc_clanguage_std_prop_value = PropertyUtil.GetPropertyOrDefault(Project, "cc_clanguage.std", string.Empty);
				string cc_clanguage_std_platform = "cc_clanguage.std." + ConfigSystem;
				string cc_clanguage_std_platform_prop_value = PropertyUtil.GetPropertyOrDefault(Project, cc_clanguage_std_platform, string.Empty);
				string cc_clanguage_std_platform_default_value = PropertyUtil.GetPropertyOrDefault(Project, cc_clanguage_std_platform + ".default", string.Empty);

				string assigned_cc_std = string.Empty;   // Don't set anything as default!

				if (!String.IsNullOrEmpty(cc_clanguage_std_platform_prop_value))
				{
					assigned_cc_std = cc_clanguage_std_platform_prop_value;
				}
				else if (ConfigOptionSet.Options.Contains(cc_clanguage_std_platform))
				{
					assigned_cc_std = ConfigOptionSet.Options[cc_clanguage_std_platform];
				}
				else if (!string.IsNullOrEmpty(cc_clanguage_std_prop_value))
				{
					assigned_cc_std = cc_clanguage_std_prop_value;
				}
				else if (ConfigOptionSet.Options.Contains("cc_clanguage.std"))
				{
					assigned_cc_std = ConfigOptionSet.Options["cc_clanguage.std"];
				}
				else if (!String.IsNullOrEmpty(cc_clanguage_std_platform_default_value))
				{
					assigned_cc_std = cc_clanguage_std_platform_default_value;
				}

				if (!String.IsNullOrEmpty(assigned_cc_std))
				{
					AddOption(OptionType.cc_Options, switchPrefix + assigned_cc_std);
					ConfigOptionSet.Options["cc_clanguage.std"] = assigned_cc_std;
				}
			}
		}

		protected virtual void Common_Clang_Options()
		{
			string currCcOptions = _optionBuilder[(int)OptionType.cc_Options].ToString();
			if (FlagEquals("clanguage-strict", "on"))
			{
				// Don't add -x option if existing optionset already setup one.  On platforms like ios/osx, we cannot have
				// both "-x objective-c" and "-x c" on the same compiler command line.  The latter one will win and we cannot
				// use "-x c" for objective c files.  We also don't want this to get used when doing precompile header build which
				// uses option"-x c-header" or "-x c++-header".
				if (!currCcOptions.Contains("-x "))
				{
					AddOptionIf("clanguage", "on", OptionType.cc_Options, "-x c");
				}
			}
			if (FlagEquals("cpplanguage-strict", "on"))
			{
				// Same as above
				if (!currCcOptions.Contains("-x "))
				{
					AddOptionIf("clanguage", "off", OptionType.cc_Options, "-x c++");
				}
			}

			if (FlagEquals("clanguage", "off"))
			{
				AddOptionIf("rtti", "off", OptionType.cc_Options, "-fno-rtti"); //Do not generate run time type information
				AddOptionIf("exceptions", "off", OptionType.cc_Options, "-fno-exceptions"); //Do not generate exception handling code
			}

			// ----- WARNING OPTIONS -----
			if (FlagEquals("warningsaserrors", "on"))
			{
				AddOption(OptionType.cc_Options, "-Werror"); // Treat warnings as errors
			}
			else
			{
				AddOption(OptionType.cc_Options, "-Wno-error"); // Disable warnings as errors
			}

			if (FlagEquals("warnings", "on"))
			{
				if (DefaultWarningSuppression == WarningSuppressionModes.On)
				{
					AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
				}
				else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
				{
					AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
				}
				else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
				{
					if (FlagEquals("optimization", "on"))
					{
						PrependOption(OptionType.cc_Options, "-Winline");
					}
					PrependOption(OptionType.cc_Options, "-Wextra");
				}

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
			}
			else
			{
				PrependOption(OptionType.cc_Options, "-w"); // Disable warnings
			}

			AddOptionIf("stripallsymbols", "on", OptionType.link_Options, "-s"); //Strip all symbol information

			Common_CppLanguage_Version_Options();

			// Precompiled header support
			if (Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
			{
				ClangEnablePch();
			}

			AddOptionIf("buildlogcoloring", "on", OptionType.cc_Options, "-fcolor-diagnostics"); // Enable Color Diagnostic Build Output.
			AddOptionIf("buildlogcoloring", "on", OptionType.cc_Options, "-fansi-escape-codes"); // Needed when enabling Color Diagnostic Build Output.
			AddOption(OptionType.cc_Options, "-fno-elide-type"); // Causes Clang to output better detail on types in Template warning/error messages	

			if (FlagEquals("debugsymbols", "on"))
			{
				AddOptionIf("debugsymbols.linetablesonly", "on", OptionType.cc_Options, "-gline-tables-only");
			}

			// Setup profile code coverage flags
			if (FlagEquals("codecoverage", "on"))
			{
				AddOption(OptionType.cc_Options, "-fcoverage-mapping");
				AddOption(OptionType.cc_Options, "-fprofile-instr-generate=" +
					(Properties["eaconfig.codecoverage-profile"] ?? ConfigOptionSet.Options["codecoverage-profile"] ?? "/hostapp/coverage.profraw"));
			}

			// Enable the clang timing report to help diagnose compile-time performance issues
			AddOptionIf("buildtiming", "on", OptionType.cc_Options, "-ftime-report");

			// Setup profile guided optimization flags
			if (FlagEquals("optimization.ltcg", "on"))
			{
				ConfigureClangLTCGProfiling();
			}
		}

		private void ClangEnablePch()
		{
			// Note that clang command line need to have create-pch to set -o to point to the .pch instead of .obj. This is 
			// done in the cc.template.pch.commandline property.
			if (FlagEquals("clanguage-strict", "on") && FlagEquals("clanguage", "on"))
			{
				AddOptionIf("create-pch", "on", OptionType.cc_Options, "-x c-header");
			}
			else
			{
				AddOptionIf("create-pch", "on", OptionType.cc_Options, "-x c++-header");
			}
			AddOptionIf("use-pch", "on", OptionType.cc_Options, "-include-pch \"%pchfile%\"");

			//In case precompiled header is defined directly through cc options
			if (ContainsOption(OptionType.cc_Options, "-x c-header") || ContainsOption(OptionType.cc_Options, "-x c++-header"))
			{
				// Need to set this flag so that nant build can schedule the file with this flag to be built first.
				ConfigOptionSet.Options["create-pch"] = "on";
			}
			else if (ContainsOption(OptionType.cc_Options, "-include-pch"))
			{
				// Need to make sure this is set so that module set data will setup the PrecompiledFile variable.
				ConfigOptionSet.Options["use-pch"] = "on";
			}
		}

		private void ConfigureClangLTCGProfiling()
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
				case "useinstrumentedprofile":  // Added new name to differentiate with "sampled profile"
				case "useprofile":              // Kept this old name to not break existing build script.  This code is used by both PS4 and Linux!
					AddOption(OptionType.cc_Options, "-fprofile-instr-use=" + fileName);
					AddOption(OptionType.link_Options, "-fprofile-instr-use=" + fileName);
					break;
				case "usesampledprofile":
					if (!FlagEquals("debugsymbols", "on"))
					{
						throw new BuildException($"[GenerateOptions]: Option 'optimization.ltcg.mode' was set to 'usesampleprofile' but 'debugsymbols' option is not set.  Please turn on 'debugsymbols' when using 'usesampleprofile' setting.");
					}
					AddOption(OptionType.cc_Options, "-fprofile-sample-use=" + fileName);
					AddOption(OptionType.link_Options, "-fprofile-sample-use=" + fileName);
					break;
			}
		}

		// enables a warning that is defaulted off in MSVC by promoting it to given warning level,
		// but only if the compiler options strings doesn't complain an explicit disable for the
		// warning already
		protected void EnabledDefaultOffVCWarningUnlessExplicitlyDisabled(string warningNumberString, char warninglevel)
		{
			StringBuilder currentOptionsSb = _optionBuilder[(int)OptionType.cc_Options];

			// This is super rare so do a first pass of just checking if the warningNumberString is in there at all before doing heavy regexp lifting
			if (ContainsSubString(currentOptionsSb, warningNumberString))
			{
				if (Regex.IsMatch(currentOptionsSb.ToString(), @"[/-]w[ed0-4]" + warningNumberString, RegexOptions.IgnoreCase))
				{
					// contains explicit disable, don't enable
					return;
				}
			}
			AddOption(OptionType.cc_Options, String.Format("-w{0}{1}", warninglevel, warningNumberString));
		}

		protected void SetClangWarningIfNotAlreadySet(string warningName, bool enabled)
		{
			string currentOptions = _optionBuilder[(int)OptionType.cc_Options].ToString();
			if (Regex.IsMatch(currentOptions, @"-W(?:no-)?" + warningName, RegexOptions.IgnoreCase))
			{
				// contains settings for this warning already don't modify
				return;
			}

			AddOption(OptionType.cc_Options, String.Format("-W{0}{1}", enabled ? String.Empty : "no-", warningName));
		}

		class DefinesRecord
		{
			public ManualResetEventSlim Done;
			public string Defines;
		}
		static Dictionary<string, DefinesRecord> g_gccClangCompilerInternalDefines = new Dictionary<string, DefinesRecord>();
		static object g_gccClangCompilerInternalDefinesLock = new object();
		static Regex defineParseRegex = new Regex(@"#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.*)$");

		static List<string> ParseDefines(string input)
		{
			List<string> defines = new List<string>();
			string[] lines = input.Split(new char[] { '\n', '\r' }, StringSplitOptions.RemoveEmptyEntries);

			foreach (string line in lines)
			{
				Match match = defineParseRegex.Match(line);
				if (!match.Success)
				{
					continue;
				}

				GroupCollection groups = match.Groups;

				if (groups.Count > 1)
				{
					string define = groups[1].Value;
					string value = null;

					if (groups.Count == 3)
					{
						value = groups[2].Value;
						defines.Add(define + "=" + value);
					}
					else
					{
						defines.Add(define);
					}
				}
			}

			return defines;
		}

		static Regex g_matchWhitespace = new Regex(@"\s+");
		static Regex g_matchGenerateDependencyFile = new Regex(@"-MM?D");
		static List<string> InternalGatherGccClangCompilerInternalDefines(string compilerPath, string options)
		{
			// GCC and Clang can be queried for their internal defines using by
			// passing the "-dM -E" options on the command line and passing the
			// compiler an empty file. 

			// If the -MD or -MMD options are specified they need to be removed otherwise
			// these compilers generate a file called "-.d" in the directory that 
			// Framework is invoked from.
			options = g_matchGenerateDependencyFile.Replace(options, "");

			// Finally, remove any newlines that may have made it in and clean up any 
			// whitespace.
			options = options.Replace('\n', ' ').Replace('\r', ' ');
			options = g_matchWhitespace.Replace(options, " ");

			string compilerOptions = options + " -dM -E -";

			// run clang and capture all output and error text
			// if the process was successful try and extract 
			// defines from the output
			// if the process failed then quietly fail - the only place
			// these defines are really used is in trying to correctly
			// populate makefile intellisense defines
			Process process = new Process();
			process.StartInfo.FileName = compilerPath;
			process.StartInfo.Arguments = compilerOptions;
			using (ProcessRunner processRunner = new ProcessRunner
			(
				process,
				redirectOutput: true,
				redirectError: true,
				redirectIn: true,
				createNoWindow: true,
				createStdThreads: true
			))
			{
				processRunner.StartAsync();
				processRunner.Process.StandardInput.Write("");
				processRunner.Process.StandardInput.Close();
				processRunner.WaitForExit();
				if (processRunner.ExitCode == 0)
				{
					// Retrieve the output only after we finished running the program.
					string output = processRunner.GetOutput();
					return ParseDefines(output);
				}
				else
				{
					return new List<string>();
				}
			}
		}

		void GatherGccClangCompilerInternalDefines()
		{
			try
			{
				string compilerPath = Properties["cc"];

				// Certain options, such as -fexceptions, will change which
				// internal defines the compiler enables. In order to see all
				// relevant defines we need to pass all the switches to the
				// compiler now that we intend to invoke the compiler with
				// later. We do not need to pass defines as command line defines
				// will not affect internal defines, only feature switches.
				string rawOptions = ConfigOptionSet.Options[_optionNames[(int)OptionType.cc_Options]];

				// Clang/GCC make assumptions of what type of file is being
				// compiled from its file extension. Reading from stdin, as this
				// process does, means the compiler will default to C. If it 
				// encounters an option like -std=c++11 when it is assuming C
				// language the compiler will error out.
				if (FlagEquals("clanguage", "off") && !rawOptions.Contains(" -x "))
				{
					rawOptions = rawOptions + " -x c++";
				}

				// On some platforms, we used multiple -arch option to build single binary with multiple architectures, 
				// but when we try to get preprocessor defines to output we can only keep one and remove the rest.
				// Otherwise, clang would complain.
				MatchCollection matches = Regex.Matches(rawOptions, @"[/-]arch [^ ]+", RegexOptions.IgnoreCase);
				if (matches.Count > 1)
				{
					// Keep the first -arch and strip the rest.
					for (int idx = 1; idx < matches.Count; ++idx)
					{
						rawOptions = rawOptions.Replace(matches[idx].Value, "");
					}
				}

				// Remove any other arguments that is known to trigger error that we don't care about.
				Match match = Regex.Match(rawOptions, "[/-]include-pch *\"[^\"]+\"", RegexOptions.IgnoreCase);
				if (match.Success)
				{
					rawOptions = rawOptions.Replace(match.Value, "");
				}
				match = Regex.Match(rawOptions, "[/-]o *\"[^\"]+\"", RegexOptions.IgnoreCase);
				if (match.Success)
				{
					rawOptions = rawOptions.Replace(match.Value, "");
				}

				// Remove all unnecessary warnings.  We are running this just to get compiler preprocessor defines.
				rawOptions = rawOptions.Replace("-Werror", "") + " -Wno-everything";

				var optionsList = rawOptions.Split(new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
				var compilerInputBuilder = new StringBuilder();
				foreach (var opt in optionsList)
					if (!string.IsNullOrWhiteSpace(opt))
						compilerInputBuilder.Append(opt.Trim()).Append(" ");
				string compilerInput = compilerInputBuilder.ToString();



				string key = compilerPath + compilerInput;

				DefinesRecord record;
				bool buildRecord = false;
				lock (g_gccClangCompilerInternalDefinesLock)
				{
					if (!g_gccClangCompilerInternalDefines.TryGetValue(key, out record))
					{
						record = new DefinesRecord();
						record.Done = new ManualResetEventSlim();
						g_gccClangCompilerInternalDefines.Add(key, record);
						buildRecord = true;
					}
				}

				if (buildRecord)
				{
					try
					{
						if (System.IO.File.Exists(compilerPath))
						{
							List<string> compilerInternalDefines = InternalGatherGccClangCompilerInternalDefines(compilerPath, compilerInput);
							record.Defines = compilerInternalDefines.ToString("\r\n");
						}
						record.Done.Set();
					}
					catch
					{
						// Worst case, if we are unable to capture the internal compiler
						// defines, we are no worse off than earlier versions of 
						// Framework.
						record.Defines = "";
						record.Done.Set();
					}
				}
				else
				{
					record.Done.Wait();
				}

				if (!string.IsNullOrEmpty(record.Defines))
				{
					string options = record.Defines;

					// This is probably always empty, but just in case
					StringBuilder sb = _optionBuilder[(int)OptionType.cc_CompilerInternalDefines];
					if (sb.Length != 0)
						options += sb.ToString();

					ConfigOptionSet.Options[_optionNames[(int)OptionType.cc_CompilerInternalDefines]] = options;
				}
			}
			catch (Exception)
			{
				// Worst case, if we are unable to capture the internal compiler
				// defines, we are no worse off than earlier versions of 
				// Framework.
			}
		}

		public void Init(Project project, OptionSet configOptionSet, string configsetName)
		{
			Project = project;
			ConfigOptionSet = configOptionSet;
			ConfigSetName = configsetName;
			InternalInit();
		}

		protected override void InternalInit()
		{
			base.InternalInit();

			string property = PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.default_warning_suppression", "on").ToLower();
			switch(property)
			{
				case "on":
					DefaultWarningSuppression = WarningSuppressionModes.On;
					break;
				case "off":
					DefaultWarningSuppression = WarningSuppressionModes.Off;
					break;
				case "strict":
					DefaultWarningSuppression = WarningSuppressionModes.Strict;
					break;
				case "custom":
					DefaultWarningSuppression = WarningSuppressionModes.Custom;
					break;
				default:
					Log.Warning.WriteLine($"{LogPrefix}{Location}Invalid value of property 'eaconfig.default_warning_suppression'='{property}'. Valid values are 'on/off/strict/custom', will use default value 'on'.");
					DefaultWarningSuppression = WarningSuppressionModes.On;
					break;
			}
		}

		protected WarningSuppressionModes DefaultWarningSuppression { get; private set; } = WarningSuppressionModes.On;

		protected void AddOptionIf(string flag, string flagValue, OptionType type, string value)
		{
			if (FlagEquals(flag, flagValue))
			{
				AddOption(type, value);
			}
		}

		protected void PrependOptionIf(string flag, string flagValue, OptionType type, string value)
		{
			if (FlagEquals(flag, flagValue))
			{
				PrependOption(type, value);
			}
		}

		protected void SetOptionsString(OptionType type, string value)
		{
			_optionBuilder[(int)type] = new StringBuilder(value);
		}

		protected void AddOption(OptionType type, string value)
		{
			_optionBuilder[(int)type].AppendLine(value);
		}

		protected void PrependOption(OptionType type, string value)
		{
			_optionBuilder[(int)type].Insert(0, value + "\n");
		}

		protected void ReplaceOptionIf(string flag, string flagValue, OptionType type, string oldValue, string newValue)
		{
			if (FlagEquals(flag, flagValue))
			{
				ReplaceOption(type, oldValue, newValue);
			}
		}

		protected void ReplaceOption(OptionType type, string oldValue, string newValue)
		{
			if (ContainsOption(type, oldValue))
			{
				_optionBuilder[(int)type].Replace(oldValue, newValue);
			}
			else
			{
				AddOption(type, newValue);
			}
		}

		protected bool ContainsOption(OptionType type, string value)
		{
			string option = _optionBuilder[(int)type].ToString();
			return (!String.IsNullOrEmpty(option) && option.Contains(value));
		}

		protected bool FlagEquals(string name, string val)
		{
			string tmpVar = Flag(name);
			bool outvar = tmpVar.Equals(val, StringComparison.OrdinalIgnoreCase);
			return outvar;
			//return Flag(name).Equals(val, StringComparison.OrdinalIgnoreCase);
		}

		private string Flag(string name)
		{
			// warningsaserrors is a special case, we allow a global stomp list (eaconfig.warningsaserrors.packages.includes)
			// to stomp the regular global stomp property (eaconfig.warningsaserrors)
			// it also stomps option (<option name="warningsaserrors"/>) if global stomp isn't set
			if (name == "warningsaserrors")
			{
				string packageName = Project.Properties["package.name"];
				string warningsaserrorsIncludes = Project.Properties.GetPropertyOrDefault("eaconfig.warningsaserrors.packages.includes", null);
				if (warningsaserrorsIncludes.ToArray().Contains(packageName))
				{
					return "on";
				}
				string warningsaserrorsExcudes = Project.Properties.GetPropertyOrDefault("eaconfig.warningsaserrors.packages.excludes", null);
				if (warningsaserrorsExcudes.ToArray().Contains(packageName))
				{
					return "off";
				}
			}

			string option = Properties["eaconfig." + name] ?? ConfigOptionSet.Options[name];
			if (!String.IsNullOrEmpty(option))
			{
				if (option.Equals("true", StringComparison.OrdinalIgnoreCase) || option.Equals("on", StringComparison.OrdinalIgnoreCase))
				{
					option = "on";
				}
				else if (option.Equals("false", StringComparison.OrdinalIgnoreCase) || option.Equals("off", StringComparison.OrdinalIgnoreCase))
				{
					option = "off";
				}
			}
			return option ?? String.Empty;
		}

		private bool ContainsSubString(StringBuilder builder, string sub)
		{
			int subLen = sub.Length;
			int builderLen = builder.Length;
			int lastPossible = builderLen - subLen;

			for (int i = 0; i < lastPossible; ++i)
			{
				if (builder[i] != sub[0])
					continue;

				bool isEqual = true;
				for (int j = 1; j < subLen; ++j)
					isEqual &= sub[j] == builder[i + j];
				if (isEqual)
					return true;
			}
			return false;
		}
	}
}
