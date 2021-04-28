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
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Console
{
	/// <summary>A NAnt Command Line Option</summary>
	internal class Option
	{
		/// <summary>The name of the command line option, including the starting dash character</summary>
		internal readonly string Name;

		/// <summary>A summary of the format of the arguments accepted by this option. If the option is just a boolean flag this will be Empty</summary>
		internal readonly string ArgumentFormat;

		/// <summary>A short summary of how the option is used</summary>
		internal readonly string Description;

		/// <summary>The initial value if unspecifed - only used to reset state in unit tests</summary>
		internal readonly object DefaultValue;

		/// <summary>This option no longer does anything. Passing it on command is considered a user error. Will be excluded from help text. </summary>
		internal readonly bool Deprecated;

		/// <summary>The value parsed from the command line arguments</summary>
		internal object Value;

		internal Option(object value, string name, string description, bool deprecated = false)
			: this(value, name, String.Empty, description, deprecated)
		{
		}

		internal Option(object value, string name, string argumentFormat, string description, bool deprecated = false)
		{
			if (!name.StartsWith("-"))
			{
				throw new ArgumentException($"Invalid option name '{name}'.", nameof(name));
			}

			Value = value;
			DefaultValue = value;
			Name = name;
			ArgumentFormat = argumentFormat;
			Description = description;
			Deprecated = deprecated;
		}

		// logging settings
		public static readonly Option DeprecationLevel = new Option(Log.DeprecateLevel.Normal, "-deprecatelevel:", "<level>", "  {0} Sets the deprecation warning level to one of " + EnumOptions<Log.DeprecateLevel>() + ", default '" + Log.DeprecateLevel.Normal.ToString() + "'.");
		public static readonly Option DeprecationDisabled = new Option(new List<string>(), "-dd:", "<id>", "  {0} Forces a deprecation to be disabled.");
		public static readonly Option DeprecationEnabled = new Option(new List<string>(), "-de:", "<id>", "  {0} Forces a deprecation to be enabled.");
		public static readonly Option LogAppend = new Option(false, "-logappend", "  {0} If using a file logger this option will not recreate the log file but append to existing log.");
		public static readonly Option Logger = new Option(new List<string>(), "-logger:", "  {0} Use specified logger class " + EnumOptions<LoggerType>() + ".");
		public static readonly Option LogIndentLevel = new Option(1, "-indent:", "<integer>", "  {0} Defined indentation value in log");
		public static readonly Option LogLevel = new Option(Log.LogLevel.Normal, "-loglevel:", "<level>", "  {0} Sets the log level to one of " + EnumOptions<Log.LogLevel>() + ", default '" + Log.LogLevel.Normal.ToString() + "'.");
		public static readonly Option UseStdErr = new Option(false, "-usestderr", "  {0} Error records sent to console are written to stderr, otherwise to stdout.");
		public static readonly Option Verbose = new Option(false, "-verbose", "  {0} Sets loglevel to 'Detailed'.");
		public static readonly Option WarningAsError = new Option(new List<string>(), "-wx:", "<id>", "  {0} Forces a warning to be enabled and thrown as an error.");
		public static readonly Option WarningAsErrorLevel = new Option(Log.WarnLevel.Quiet, "-warnaserrorlevel:", "<level>", "  {0} Sets the warning level to one of " + EnumOptions<Log.WarnLevel>() + ", warnings below this level will be thrown as errors.");
		public static readonly Option WarningDisabled = new Option(new List<string>(), "-wd:", "<id>", "  {0} Forces a warning to be disabled.");
		public static readonly Option WarningEnabled = new Option(new List<string>(), "-we:", "<id>", "  {0} Forces a warning to be enabled.");
		public static readonly Option WarningNotAsError = new Option(new List<string>(), "-wn:", "<id>", "  {0} Forces a warning to not be thrown as an error.");
		public static readonly Option WarnLevel = new Option(Log.WarnLevel.Normal, "-warnlevel:", "<level>", "  {0} Sets the warning level to one of " + EnumOptions<Log.WarnLevel>() + ", default '" + Log.WarnLevel.Normal.ToString() + "'.");

		// general options
		public static readonly Option AutoVersion = new Option(false, "-autoversion", "  {0} Allow framework to determine if the running framework version matches the master config. If not, download the config version and pass command line through.");
		public static readonly Option BuildFile = new Option(String.Empty, "-buildfile:", "<file>", "  {0} Use given buildfile.");
		public static readonly Option BuildPackage = new Option(String.Empty, "-buildpackage:", "<package name>", "  {0} Use masterconfig to find buildfile for given package (case insensitive).");
		public static readonly Option BuildRoot = new Option(String.Empty, "-buildroot:", "<path>", "  {0} Use given build root.");
		public static readonly Option CacheDirContents = new Option(false, "-cachedircontents", "  {0} Cache directory contents for faster build performance.");
		public static readonly Option ConfigsOption = new Option(String.Empty, "-configs:", "\"<config> <configs> ...\"", "  {0} A list of one or more configs to use (equivalent to -D:package.configs).");
		public static readonly Option ForceAutobuild = new Option(false, "-forceautobuild", "  {0} Force autobuild for all packages, even those with 'autobuildclean=false'.");
		public static readonly Option ForceAutobuildGroup = new Option(String.Empty, "-forceautobuild:group:", "\"<grouptype name> ...\"", "  {0} Force autobuild for packages in the given grouptypes, even those with 'autobuildclean=false'.");
		public static readonly Option ForceAutobuildPackage = new Option(String.Empty, "-forceautobuild:", "\"<package name> ...\"", "  {0} Force autobuild for packages with given names, even those with 'autobuildclean=false'.");
		public static readonly Option GlobalPropertiesAltOption = new Option(new Dictionary<string, string>(), "-D:", "<property>=<value>", "  {0} Define global property, use value for given property.");
		public static readonly Option GlobalPropertiesOption = new Option(new Dictionary<string, string>(), "-G:", "<property>=<value>", "  {0} Define global property, use value for given property.");
		public static readonly Option MasterConfigFile = new Option(String.Empty, "-masterconfigfile:", "<file>", "  {0} Use specified master configuration file.");
		public static readonly Option NoMetrics = new Option(false, "-nometrics", String.Empty);
		public static readonly Option PackageServerToken = new Option(String.Empty, "-packageservertoken:", "Authentication token to use for interaction with package server. If empty, the credstore shall be used to obtain the value");
		public static readonly Option P4PathExe = new Option(false, "-p4pathfirst", "  {0} Use the instance of p4 found in the path var before any other version, if the version in path exists.");
		public static readonly Option ProgramHelp = new Option(false, "-help", "  {0} Print this message.");
		public static readonly Option PropertiesFile = new Option(new List<string>(), "-propertiesfile:", "<file>", "  {0} Use given properties file.");
		public static readonly Option PropertyCaseSensitivity = new Option(PropertyKey.CaseSensitivityLevel.Insensitive, "-propcasesensitive:", "<level>", "{0} Sets the case sensitivity of properties to one of " + EnumOptions<PropertyKey.CaseSensitivityLevel>() + ", default: '" + PropertyKey.CaseSensitivityLevel.Insensitive.ToString() + "'.");
		public static readonly Option ShowMasterConfig = new Option(false, "-showmasterconfig", "  {0} Print content of masterconfig file including merged fragments.");
		public static readonly Option ShowMasterConfigFiltered = new Option(String.Empty, "-showmasterconfig:", "\"<packagename> <packagename> ...\"", "  {0} Print content of masterconfig file including merged fragments but limited to the specified packages.");
		public static readonly Option MasterConfigFragments = new Option(String.Empty, "-masterconfigfragments:", "\"<path>;<path>...\"", "  {0} Tell framework to manually include the file(s) specified as additional masterconfig fragments when merging its masterconfigs. Separate mulitple paths with ';'.");
		public static readonly Option ShowTime = new Option(false, "-showtime", "  {0} Log current time at the end of the build.");
		public static readonly Option UsePackageServerV1 = new Option(false, "-usePackageServerV1", "Use the old version of the package server"); // to be set to true after confirmation build farm(s) have creds set up
		public static readonly Option VersionHelp = new Option(false, "-version", "  {0} Print version information.");

		// debugging options
		public static readonly Option BackupGeneratedFiles = new Option(false, "-backupgeneratedfiles", "  {0} Save old versions of generated files (ie: .vcxproj/.csproj/.sln/.mak) when writing new ones.");
		public static readonly Option Debug = new Option(false, "-debug", "  {0} Launch nant debugging session immediately after processing arguments.");
		public static readonly Option DumpDebugInfo = new Option(false, "-dumpdebuginfo", "  {0} Creates a zip file containing information that is helpful for the CM team to debug build issues.");
		public static readonly Option EnableCoreDump = new Option(false, "-enablecoredump", "  {0} Enable project instance core dump. Core dump is disabled by default.");
		public static readonly Option NoParallelNant = new Option(false, "-noparallel", "  {0} Disables parallel build script loading and processing. Parallel processing is enabled by default.");
		public static readonly Option TraceEcho = new Option(false, "-trace-echo", "  {0} Displays location of all echo instructions.");
		public static readonly Option TraceProperties = new Option(String.Empty, "-traceprop:", "\"<property name> ...\"", "  {0} Print trace information when property is changed or removed.");
		public static readonly Option TraceOptionSets = new Option(String.Empty, "-traceoptionset:", "\"<optionset name> ...\"", "  {0} Print trace information when optionset is added or modified from <optionset> tasks. Note that this does not trace optionset being modified or added from C# code. Output is 'prettified' so whitespace changes in option will be detected but not correctly displayed.");

		// deprecated options
		public static readonly Option CacheDebug = new Option(false, "-cachedebug", String.Empty, deprecated: true);            // deprecated on 2018/07/30
		public static readonly Option EnableCache = new Option(false, "-enablecache", String.Empty, deprecated: true);          // deprecated on 2018/07/30
		public static readonly Option EnableMetrics = new Option(false, "-enablemetrics", String.Empty, deprecated: true);      // deprecated on 2018/07/30
		public static readonly Option EnableTimingMetrics = new Option(0, "-timingMetrics:", String.Empty, deprecated: true);   // deprecated on 2018/07/30
		public static readonly Option FindBuildFile = new Option(false, "-find", String.Empty, deprecated: true);               // deprecated on 2018/07/30
		public static readonly Option UseOverrides = new Option(false, "-useoverrides", String.Empty, deprecated: true);        // deprecated on 2018/07/30

		// hidden options (intended only for those working on Framework, not for general use) - empty description options do not appear in help
		public static readonly Option DevWarnings = new Option(false, "-devwarnings", String.Empty);
		public static readonly Option BreakOnTraceProp = new Option(false, "-break-on-traceprop", String.Empty);
		public static readonly Option BreakOnDependencies = new Option(String.Empty, "-break-on-dep:", String.Empty);

		// temporary transition options
		public static readonly Option SanitizeFbConfigs = new Option(false, "-sanitizefbconfigs", "  {0} Automatically convert from old fb config names to eaconfig names since the old fb config names are deprecated.");

		/// <summary>The list of all targets that were passed in on the command line</summary>
		internal static readonly List<string> Targets = new List<string>();

		internal static void PrintHelp()
		{
			System.Console.WriteLine();
			System.Console.WriteLine("Usage: nant [options] [target [target2] ... ]");
			System.Console.WriteLine();
			System.Console.WriteLine("Options:");

			Option[] visibleOptions = GetOptionDefinitions().Where(opt => !String.IsNullOrEmpty(opt.Description) && !opt.Deprecated).ToArray();
			int optionsPadding = visibleOptions.Max(opt => opt.Name.Length + opt.ArgumentFormat.Length);
			foreach (Option option in visibleOptions)
			{
				System.Console.WriteLine(option.Description, (option.Name + option.ArgumentFormat).PadRight(optionsPadding));
			}

			System.Console.WriteLine();
			System.Console.WriteLine("A file ending in .build will be used if no buildfile is specified.");
		}

		internal static List<Tuple<string, Option>> ParseCommandLine(string[] args)
		{
			Option[] options = GetOptionDefinitions();
			List<Tuple<string, Option>> invalidOptions = new List<Tuple<string, Option>>();
			foreach (string arg in args)
			{
				if (arg.StartsWith("-"))
				{
					// find matching option
					Option matchingOpt = null;
					Option[] matchingOptions = options.Where(opt => arg.StartsWith(opt.Name)).ToArray();
					if (matchingOptions.Length == 1)
					{
						matchingOpt = matchingOptions.First();
					}
					else if (matchingOptions.Any())
					{
						// this case means we have two options with the same starting string
						// such as -forceautobuild (bool) and -forceautobuild: (list)
						// in which case we want the non-boolean option
						matchingOpt = matchingOptions.FirstOrDefault(opt => !(opt.Value is bool) || opt.Name == arg);
					}

					// process option
					if (matchingOpt == null || matchingOpt.Deprecated)
					{
						invalidOptions.Add(Tuple.Create(arg, matchingOpt));
					}
					else if (matchingOpt.Value is bool)
					{
						ProcessBoolOption(matchingOpt, arg);
					}
					else if (matchingOpt.Value is Dictionary<string, string>)
					{
						ProcessDictionaryOption(matchingOpt, arg);
					}
					else if (matchingOpt.Value is string)
					{
						ProcessStringOption(matchingOpt, arg);
					}
					else if (matchingOpt.Value is List<string>)
					{
						ProcessStringListOption(matchingOpt, arg);
					}
					else if (matchingOpt.Value is Enum)
					{
						ProcessEnumOption(matchingOpt, arg);
					}
					else if (matchingOpt.Value is int)
					{
						ProcessIntOption(matchingOpt, arg);
					}
					else
					{
						throw new Exception("INTERNAL ERROR: No processing done for parameter '" + matchingOpt.Name + "'.");
					}
				}
				else
				{
					// must be a target if not an option
					Targets.Add(arg);
				}
			}
			return invalidOptions;
		}

		// for unit testing only
		internal static void Reset()
		{
			foreach (Option option in GetOptionDefinitions())
			{
				option.Value = option.DefaultValue;

				// collections are special, they are not given a new value but appended to so they need to be reset
				if (option.Value is List<string> asList) 
				{
					asList.Clear();
				}

				if (option.Value is Dictionary<string, string> asDict)
				{
					asDict.Clear();
				}
			}
		}

		private static Option[] GetOptionDefinitions()
		{
			return typeof(Option).GetFields(BindingFlags.Static | BindingFlags.Public)
				.Select(field => field.GetValue(null) as Option)
				.OrderBy(option => option.Name)
				.ToArray();
		}

		private static void ProcessBoolOption(Option option, string arg)
		{
			if (arg.Length <= option.Name.Length)
			{
				option.Value = true;
			}
			else
			{
				string value = arg.Substring(option.Name.Length + 1); // plus one for ':'
				if (!Boolean.TryParse(value, out bool boolVal))
				{
					throw new BuildException(String.Format("Failed to convert option '{0}' value '{1}' to bool.", option.Name, value));
				}
				option.Value = boolVal;
			}
		}

		private static void ProcessDictionaryOption(Option option, string arg)
		{
			Match match = Regex.Match(arg, option.Name + @"(\w+.*?)=(\w*.*)");
			if (match.Success)
			{
				string name = match.Groups[1].Value;
				string value = match.Groups[2].Value;

				Dictionary<string, string> dictionary = (Dictionary<string, string>)option.Value;
				if (dictionary.TryGetValue(name, out string existingValue))
				{
					System.Console.WriteLine("WARNING: duplicate command line definition of the property '{0}' is ignored. '{1}{2}={3}' will be used.", arg, option.Name, name, existingValue);
				}
				else
				{
					dictionary[name] = value;
				}
			}
			else
			{
				String msg = String.Format("Invalid syntax for '{0}' parameter {1}", option.Name, arg);
				throw new BuildException(msg);
			}
		}

		private static void ProcessStringOption(Option option, string arg)
		{
			if (!String.IsNullOrEmpty((string)option.Value))
			{
				throw new BuildException(String.Format("Option '{0}' has already been specified.", option.Name));
			}

			if (arg.Length > option.Name.Length)
			{
				option.Value = arg.Substring(option.Name.Length);
			}

			if (String.IsNullOrEmpty((string)option.Value))
			{
				throw new BuildException(String.Format("Option '{0}' value is missing in the argument.", option.Name));
			}
		}

		private static void ProcessStringListOption(Option option, string arg)
		{
			string value = null;
			if (arg.Length > option.Name.Length)
			{
				value = arg.Substring(option.Name.Length);
			}
			if (String.IsNullOrEmpty(value))
			{
				throw new BuildException(String.Format("Option '{0}' value is missing in the argument.", option.Name));
			}

			List<string> list = (List<string>)option.Value;
			if (list.Contains(value))
			{
				System.Console.WriteLine("WARNING: duplicate command line argument {0}", arg);
			}
			else
			{
				list.Add(value);
			}
		}

		private static void ProcessEnumOption(Option option, string arg)
		{
			string value = null;
			if (arg.Length > option.Name.Length)
			{
				value = arg.Substring(option.Name.Length);
			}

			if (String.IsNullOrEmpty(value))
			{
				throw new BuildException(String.Format("Option '{0}' value is missing in the argument.", option.Name));
			}

			try
			{
				option.Value = Enum.Parse(option.Value.GetType(), value, true);
			}
			catch (Exception ex)
			{
				throw new BuildException(String.Format("Failed to convert Option '{0}' value '{1}' to Enum '{2}', valid values are: '{3}'.", option.Name, value, option.Value.GetType().Name, StringUtil.ArrayToString(Enum.GetValues(option.Value.GetType()), ", ")), ex);
			}
		}

		private static void ProcessIntOption(Option option, string arg)
		{
			string value = null;
			if (arg.Length > option.Name.Length)
			{
				value = arg.Substring(option.Name.Length);
			}
			if (String.IsNullOrEmpty(value))
			{
				throw new BuildException(String.Format("Option '{0}' value is missing in the argument.", option.Name));
			}

			if (!Int32.TryParse(value, out int intval))
			{
				throw new BuildException(String.Format("Failed to convert Option '{0}' value '{1}' to integer.", option.Name, value));
			}
			option.Value = intval;
		}

		private static string EnumOptions<T>()
		{
			return "[" + Enum.GetNames(typeof(T)).ToString("|") + "]";
		}
	}
}