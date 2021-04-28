// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;

using EA.FrameworkTasks.Model;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Metrics;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Perforce;
using NAnt.Shared.Properties;

namespace NAnt.Console
{
	internal sealed class ConsoleRunner : IDisposable
	{
		internal enum ReturnCode : int
		{
			Ok = 0,
			Error = 1,
			InternalError = 2
		}

		private bool m_disposed = false;
		private static ConsoleCtrl m_consoleControl = null;

		public void Dispose()
		{
			if (!m_disposed)
			{
				m_disposed = true;
				if (m_consoleControl != null)
				{
					m_consoleControl.Dispose();
				}
			}
		}

		private static int Main(string[] args)
		{
			// theoretically allows "profile guided jit'ing" where each run creates a .profile which is then used to kick
			// off a background thread to pre-jit as much as possible next run - hasn't shown any notable performance difference
			// but perfview does show some jit'ing being offloaded to background
			System.Runtime.ProfileOptimization.SetProfileRoot(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location));
			System.Runtime.ProfileOptimization.StartProfile(Path.GetFileName(Assembly.GetEntryAssembly().Location) + ".profile");

			if (IsEnvironmentVariableSet("FRAMEWORK_DEBUGONSTART"))
			{
				Debugger.Launch();
			}

			using (ConsoleRunner runner = new ConsoleRunner())
			{
				return runner.Start(args);
			}
		}

		internal int Start(string[] args)
		{
			try
			{
				// add the console event handler if we are on Windows
				if (PlatformUtil.IsWindows && m_consoleControl == null)
				{
					m_consoleControl = new ConsoleCtrl();
					m_consoleControl.ControlEvent += new ConsoleCtrl.ControlEventHandler(ConsoleEventHandler);
				}

				List<Tuple<string, Option>> invalidOptions = Option.ParseCommandLine(args);

				bool showMasterConfigMode = false;
				string showMasterConfigFilteredValue = Option.ShowMasterConfigFiltered.Value as String;
				if ((bool)Option.ShowMasterConfig.Value || !String.IsNullOrEmpty(showMasterConfigFilteredValue))
				{
					showMasterConfigMode = true;
				}
				
				List<string> masterConfigFragments = null;
				string masterConfigFragmentsValue = Option.MasterConfigFragments.Value as String;
				if (!String.IsNullOrEmpty(masterConfigFragmentsValue))
				{
					masterConfigFragments = masterConfigFragmentsValue.ToArray(new char[] { ';' }).ToList();
				}

				// setup logging as soon as we can so logging setting are applied for the rest of execution
				Log topLevelLog = null;
				if (showMasterConfigMode || (bool)Option.ProgramHelp.Value || (bool)Option.VersionHelp.Value)
				{
					topLevelLog = Log.Silent; // suppress all normal logging if we are performing special action, we just want output from that action
											  // (or output of autoversion nant if option is passed through to that)
				}
				else
				{
					List<ILogListener> listeners = new List<ILogListener>();
					List<ILogListener> errorListeners = new List<ILogListener>();

					LoggerType selectedTypes = 0;
					foreach (string loggertype in ((IEnumerable<string>)Option.Logger.Value))
					{
						LoggerType parsedType;
						if (!Enum.TryParse(loggertype, ignoreCase: true, result: out parsedType))
						{
							throw new BuildException("Unknown log type '" + loggertype + "'.");
						}
						selectedTypes |= parsedType;
					}
					selectedTypes = (selectedTypes == 0) ? LoggerType.Console : selectedTypes;

					if ((selectedTypes & LoggerType.Console) == LoggerType.Console)
					{
						ConsoleListener consoleListener = new ConsoleListener();
						listeners.Add(consoleListener);
						if ((bool)Option.UseStdErr.Value)
						{
							errorListeners.Add(new ConsoleStdErrListener());
						}
						else
						{
							errorListeners.Add(consoleListener);
						}
					}

					if ((selectedTypes & LoggerType.File) == LoggerType.File)
					{
						FileListener fileListener = new FileListener(Log.DefaultLogFileName, (bool)Option.LogAppend.Value);
						listeners.Add(fileListener);
						errorListeners.Add(fileListener);
					}

					topLevelLog = new Log
					(
						level: ((bool)Option.Verbose.Value && (Log.LogLevel)Option.LogLevel.Value < Log.LogLevel.Detailed) ? Log.LogLevel.Detailed : (Log.LogLevel)Option.LogLevel.Value,
						warningLevel: (Log.WarnLevel)Option.WarnLevel.Value,
						warningAsErrorLevel: (Log.WarnLevel)Option.WarningAsErrorLevel.Value,
						deprecationLevel: (Log.DeprecateLevel)Option.DeprecationLevel.Value,
						indentLevel: (int)Option.LogIndentLevel.Value,
						listeners: listeners,
						errorListeners: errorListeners,
						enableDeveloperWarnings: (bool)Option.DevWarnings.Value
					);

					// setup warnings, disables trump enambles, not as error trumps as error
					foreach (Log.WarningId code in ParseWarningCodes(Option.WarningDisabled))
					{
						Log.SetGlobalWarningEnabledIfNotSet(topLevelLog, code, false, Log.SettingSource.CommandLine);
					}
					foreach (Log.WarningId code in ParseWarningCodes(Option.WarningEnabled))
					{
						Log.SetGlobalWarningEnabledIfNotSet(topLevelLog, code, true, Log.SettingSource.CommandLine);
					}
					foreach (Log.WarningId code in ParseWarningCodes(Option.WarningNotAsError))
					{
						Log.SetGlobalWarningAsErrorIfNotSet(topLevelLog, code, false, Log.SettingSource.CommandLine);
					}
					foreach (Log.WarningId code in ParseWarningCodes(Option.WarningAsError))
					{
						Log.SetGlobalWarningAsErrorIfNotSet(topLevelLog, code, true, Log.SettingSource.CommandLine);
					}

					// same for deprecates
					foreach (Log.DeprecationId code in ParseDeprecationCodes(Option.DeprecationDisabled))
					{
						Log.SetGlobalDeprecationEnabledIfNotSet(topLevelLog, code, false, Log.SettingSource.CommandLine);
					}
					foreach (Log.DeprecationId code in ParseDeprecationCodes(Option.DeprecationEnabled))
					{
						Log.SetGlobalDeprecationEnabledIfNotSet(topLevelLog, code, true, Log.SettingSource.CommandLine);
					}
				}

				// set misc global settings, do these early as they have no setup requirements
				PropertyKey.CaseSensitivity = (PropertyKey.CaseSensitivityLevel)Option.PropertyCaseSensitivity.Value;
				CachedWriter.SetBackupGeneratedFilesPolicy((bool)Option.BackupGeneratedFiles.Value || IsEnvironmentVariableSet("FRAMEWORK_BACKUPGENERATEDFILES"));
				ImplicitPattern.SetDirectoryContentsCachingPolicy((bool)Option.CacheDirContents.Value);
				P4GlobalOptions.P4ExeSearchPathFirst = (bool)Option.P4PathExe.Value; // Set perforce to first use the version found in the path before any other version.
				Project.EnableCoreDump = (bool)Option.EnableCoreDump.Value;
				Project.SanitizeFbConfigs = (bool)Option.SanitizeFbConfigs.Value;

#if NANT_CONCURRENT
				Project.NoParallelNant = (bool)Option.NoParallelNant.Value;
#else
				if ((bool)Option.NoParallelNant.Value)
				{
					topLevelLog.Warning.WriteLine("Nant was built in DEBUG configuration so parallel processing is disabled.");
				}
#endif
				PropertyDictionary.SetTraceProperties(Option.TraceProperties.Value.ToString().ToArray(), (bool)Option.BreakOnTraceProp.Value);
				OptionSetDictionary.SetTraceOptionsets(Option.TraceOptionSets.Value.ToString().ToArray());
				DependentCollection.SetBreakOnDependencies(Option.BreakOnDependencies.Value.ToString().ToArray());

				// get properties from the command line or from property files - this include the -configs: option
				Dictionary<string, string> commandLineProperties = GetCommandLineProperties(topLevelLog);
				Project.InitialCommandLineProperties = commandLineProperties;
				foreach (KeyValuePair<string, string> commandLineProp in commandLineProperties)
				{
					Project.GlobalProperties.Add(commandLineProp.Key, commandLineProp.Value);
				}

				// create a project for resolving properties etc for masterconfig file before we create the actual build file project
				Project globalInitProject = new Project(topLevelLog, commandLineProperties);
				globalInitProject.Log.ApplyLegacyProjectSuppressionProperties(globalInitProject); // update warning settings from legacy properties now we have command line properties

				// create variables for build file and masterconfig - these are entwined because we try to find the masterconfig relative to the build file -
				// BUT if the user specified buildpackage rather than build file we need to find the masterconfig first to resolve the path to 
				// the package - this part done later when we actually need to have a build file
				MasterConfig masterConfig = null;
				string buildFilePath = null;

				// handle auto version before processing invalid command line options - they may be valid for the version we are targeting
				if ((bool)Option.AutoVersion.Value)
				{
					masterConfig = InitMasterConfig(globalInitProject, out buildFilePath, safe: true, customFragments: masterConfigFragments);
					if (masterConfig == null)
					{
						topLevelLog.Warning.WriteLine($"Ignoring {Option.AutoVersion.Name}, {Option.AutoVersion.Name} requires masterconfig file.");
					}
					else
					{
						topLevelLog.Info.WriteLine($"Option {Option.AutoVersion.Name} is enabled, determining if an automatic version change should happen.");
						if (ProcessAutoVersion(globalInitProject, masterConfig, args, out int autoVersionReturn))
						{
							return autoVersionReturn;
						}
					}
				}

				// launch debugger if requested, delayed until now so that we trigger debugging in autoversion rather than this version if needed
				if ((bool)Option.Debug.Value)
				{
					Debugger.Launch();
				}

				// if we made it here now we can complain about invalid options
				if (invalidOptions.Any())
				{
					string firstInvalidArg = invalidOptions.First().Item1;
					Option firstInvalidOption = invalidOptions.First().Item2;
					if (firstInvalidOption?.Deprecated ?? false)
					{
						throw new BuildException($"The {firstInvalidOption.Name} option is no longer supported.");
					}
					else
					{
						throw new BuildException($"Unknown command line option '{firstInvalidArg}'.");
					}
				}

				// we're not forwarding to autoversion, handle special options that short circuit target execution and perform a different function
				if ((bool)Option.VersionHelp.Value)
				{
					ShowBanner();
					return (int)ReturnCode.Ok;
				}
				else if ((bool)Option.ProgramHelp.Value)
				{
					ShowBanner();
					Option.PrintHelp();
					return (int)ReturnCode.Ok;
				}

				// if we have arrived here and packagemap isn't initialized - either because autoversion wasn't set or missing masterconfig, attempt to initialze with safe = false
				// if we previously failed because of missing masterconfig this will error
				if (masterConfig == null)
				{
					masterConfig = InitMasterConfig(globalInitProject, out buildFilePath, safe: false, customFragments: masterConfigFragments);
				}

				// we don't want any any errors caused by this call to stop -version or -help
				// but we do want this set in order to have the resolved buildroot in showmasterconfig
				PackageMap.Instance.SetBuildRoot(globalInitProject, ((string)Option.BuildRoot.Value).TrimWhiteSpace().TrimQuotes());

				// another special option, dumps resolved masterconfig to console out then exits
				if (showMasterConfigMode)
				{
					List<string> packagesFilter = null;
					if (!String.IsNullOrEmpty(showMasterConfigFilteredValue))
					{
						packagesFilter = showMasterConfigFilteredValue.ToArray().ToList();
					}

					ShowMasterconfig(globalInitProject, masterConfig, packagesFilter);
					return (int)ReturnCode.Ok;
				}

				// print process environment (debuging log level only)
				PrintProcessEnvironment(topLevelLog);

				// disable if needed telemetry before first telemetry call
				if ((bool)Option.NoMetrics.Value || IsEnvironmentVariableSet("FRAMEWORK_NOMETRICS"))
				{
					Telemetry.Enabled = false;
				}

				// track telemetry now auto version, debug properties and special options are dealt with
				// - before now we might get a double entry due to auto version
				Telemetry.TrackInvocation(args);

				// if buildpackage was set and build file is not yet resolved use the package map to find the package
				string buildPackageArgValue = Option.BuildPackage.Value as String;
				Release topLevelRelease = null;
				if (!String.IsNullOrEmpty(buildPackageArgValue))
				{
					if (buildFilePath != null)
					{
						// build was already resolve from explicit buildfile arguments
						topLevelLog.Warning.WriteLine("Argument '{0}' ignored because '{1}' was specified.", Option.BuildPackage.Name, Option.BuildFile.Name);
					}
					else
					{
						topLevelRelease = PackageMap.Instance.FindOrInstallCurrentRelease(globalInitProject, buildPackageArgValue);
						buildFilePath = Path.Combine(topLevelRelease.Path, topLevelRelease.Name + ".build");
					}
				}
				else if (buildFilePath == null)
				{
					// no explicit buildfile or buildpackage, assume build file is in working dir
					// find first file ending in .build
					DirectoryInfo directoryInfo = new DirectoryInfo(Project.NantProcessWorkingDirectory);
					FileInfo[] buildFiles = directoryInfo.GetFiles("*.build");

					if (buildFiles.Length == 1)
					{
						buildFilePath = Path.Combine(Project.NantProcessWorkingDirectory, buildFiles.First().Name);

					}
					else if (buildFiles.Length == 0)
					{
						throw new BuildException($"Could not find a '.build' file in '{Project.NantProcessWorkingDirectory}'. Use -buildpackage:<package> to use a specific package from masterconfig.");
					}
					else
					{
						throw new BuildException($"More than one '.build' file found in '{Project.NantProcessWorkingDirectory}'. Use -buildfile:<file> to specify a specific file, or -buildpackage:<package> to use a specific package from masterconfig.");
					}

					topLevelRelease = PackageMap.CreateReleaseFromBuildFile(globalInitProject, buildFilePath);
				}
				else
				{
					topLevelRelease = PackageMap.CreateReleaseFromBuildFile(globalInitProject, buildFilePath);
				}

				// now that top level release is resolved, set it in package map
				PackageMap.Instance.SetTopLevelRelease(topLevelRelease);

				// resolve the build targets for top level package
				List<string> buildTargets = new List<string>();
				if ((bool)Option.DumpDebugInfo.Value)
				{
					// debug dump info hijacks the build target call the dump-debug-info targets, the actual targets are stored
					// in a property for the target to retrieve and call
					Project.GlobalProperties.Add("dumpdebuginfo.targets", initialValue: String.Join(" ", Option.Targets));
					buildTargets.Add("dump-debug-info");
				}
				else
				{
					buildTargets = Option.Targets;
				}

				// now create and execute a project for the top level build file
				Project topLevelBuildFileProject = new Project(topLevelLog, buildFilePath, buildTargets, topLevel: true, commandLineProperties: commandLineProperties);
				if (!topLevelBuildFileProject.Run())
				{
					return (int)ReturnCode.Error;
				}
				if (PropertyKey.CaseSensitivity == PropertyKey.CaseSensitivityLevel.InsensitiveWithTracking ||
					PropertyKey.CaseSensitivity == PropertyKey.CaseSensitivityLevel.SensitiveWithTracking)
				{
					LogPropertyCaseMismatches(topLevelLog);
				}
			}
			catch (Exception e)
			{
				return ProcessTopLevelException(e);
			}
			finally
			{
				Telemetry.Shutdown();
			}

			return (int)ReturnCode.Ok;
		}

		// initialize masterconfig and package map - note build file path might also be resolved at this time since default path
		// for masterconfig is beside build file
		// the 'safe' parameter will not throw an error if no masterconfig is available
		private static MasterConfig InitMasterConfig(Project project, out string buildFilePath, bool safe = false, List<string> customFragments = null)
		{
			MasterConfig masterConfig = null;
			string masterConfigFilePath = null;

			buildFilePath = null; // this may or may not be resolved at this time, depending on cmd line option sets

			// explicit masterconfig file path
			string masterConfigArgValue = Option.MasterConfigFile.Value as String;
			if (!String.IsNullOrEmpty(masterConfigArgValue))
			{
				masterConfigFilePath = Path.IsPathRooted(masterConfigArgValue) ? masterConfigArgValue : Path.Combine(Environment.CurrentDirectory, masterConfigArgValue);
			}

			// explicit build file path
			string buildFileArg = Option.BuildFile.Value as String;
			if (!String.IsNullOrEmpty(buildFileArg))
			{
				buildFilePath = Path.IsPathRooted(buildFileArg) ? buildFileArg : Path.Combine(Environment.CurrentDirectory, buildFileArg);

				// if masterconfig file wasn't explicitly set, assume it is next to resolved build file path
				if (masterConfigFilePath == null)
				{
					masterConfigFilePath = Path.Combine(Path.GetDirectoryName(buildFilePath), "masterconfig.xml");
				}
			}
			else if (masterConfigFilePath == null)
			{
				// no explicit masterconfig and no explicit build file - let's just look in current directory for masterconfig
				masterConfigFilePath = Path.Combine(Environment.CurrentDirectory, "masterconfig.xml");
			}

			// load the masterconfig and init package map 
			if (!safe || File.Exists(masterConfigFilePath) || LineInfoDocument.IsInCache(masterConfigFilePath)) // don't fail if this is missing when running "safe", we don't need it for '-help' for example
			{
				masterConfig = MasterConfig.Load(project, masterConfigFilePath, customFragments);
				PackageMap.Init(project, masterConfig, autobuildAll: (bool)Option.ForceAutobuild.Value, autobuildPackages: (string)Option.ForceAutobuildPackage.Value, autobuildGroups: (string)Option.ForceAutobuildGroup.Value, token: (string)Option.PackageServerToken.Value, useV1: (bool)Option.UsePackageServerV1.Value);
				
				// normally configuration properties are only initialized when there is a .build file associated witht the project
				// however when running full nant (as opposed to eapm, tests, etc) we want to initialize the config for the project
				// used to setup masterconfig because will we use to resolve -buildpackage which we want to respect any config based
				// exceptions
				ConfigPackageLoader.InitializeInitialConfigurationProperties(project);
			}

			return masterConfig;
		}

		private static IEnumerable<Log.WarningId> ParseWarningCodes(Option option)
		{
			HashSet<Log.WarningId> codes = new HashSet<Log.WarningId>();
			foreach (string warningCodeString in (List<string>)option.Value)
			{
				if (!Enum.TryParse(warningCodeString, out Log.WarningId id) || id == Log.WarningId.NoId)
				{
					throw new BuildException($"Invalid warning code for option {option.Name}: '{warningCodeString}'.");
				}
				codes.Add(id);
			}
			return codes;
		}

		private static IEnumerable<Log.DeprecationId> ParseDeprecationCodes(Option option)
		{
			HashSet<Log.DeprecationId> codes = new HashSet<Log.DeprecationId>();
			foreach (string deprecationCodeString in (List<string>)option.Value)
			{
				if (!Enum.TryParse(deprecationCodeString, out Log.DeprecationId id) || id == Log.DeprecationId.NoId)
				{
					throw new BuildException($"Invalid deprecation code for option {option.Name}: '{deprecationCodeString}'.");
				}
				codes.Add(id);
			}
			return codes;
		}

		private static void ConsoleEventHandler(ConsoleCtrl.ConsoleEvent consoleEvent)
		{
			// allow NAnt to run as a service
			if (consoleEvent == ConsoleCtrl.ConsoleEvent.CTRL_LOGOFF)
			{
				return;
			}

			// commit ... SODUKO! (also kill our child process aggressively then kill ourselves)
			using (Process process = Process.GetCurrentProcess())
			{
				if (process != null)
				{
					ProcessUtil.KillChildProcessTree(process.Id, killInstantly: true);
					process.Kill();
				}
			}
		}

		private static Dictionary<string, string> GetCommandLineProperties(Log log)
		{
			StringComparer keyComparer = PropertyKey.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase;
			Dictionary<string, string> properties = new Dictionary<string, string>(keyComparer);
			{
				IEnumerable<KeyValuePair<string, string>> commandLineProperties =
					((Dictionary<string, string>)Option.GlobalPropertiesAltOption.Value)
					.Concat((Dictionary<string, string>)Option.GlobalPropertiesOption.Value);

				foreach (KeyValuePair<string, string> pair in commandLineProperties)
				{
					if (properties.TryGetValue(pair.Key, out string existingValue) && existingValue != pair.Value)
					{
						log.Warning.WriteLine
						(
							"Property '{0}' specifed via {1} was already specified via {2}. Using value '{3}'.",
							pair.Key, $"{Option.GlobalPropertiesAltOption} option", $"{Option.GlobalPropertiesOption} option", existingValue
						);
					}
					else
					{
						properties[pair.Key] = pair.Value;
					}
				}
			}

			foreach (string file in (List<string>)Option.PropertiesFile.Value)
			{
				PropertiesFileLoader.LoadPropertiesFromFile(log, file, out Dictionary<string, string> fileLocalProperties, out Dictionary<string, string> fileGlobalProperties);

				IEnumerable<KeyValuePair<string, string>> propertyFileProperties =
					fileLocalProperties
					.Concat(fileGlobalProperties);

				foreach (KeyValuePair<string, string> pair in propertyFileProperties)
				{
					if (properties.TryGetValue(pair.Key, out string existingValue) && existingValue != pair.Value)
					{
						log.Warning.WriteLine
						(
							"Property '{0}' specifed via {1} was already specified via {2}. Using value '{3}'.",
							pair.Key, file, "command line or another file", existingValue
						);
					}
					else
					{
						properties[pair.Key] = pair.Value;
					}
				}
			}

			// if configs command line option is set it stomps any value from other sources
			string configOptionValue = Option.ConfigsOption.Value as String;
			if (!String.IsNullOrEmpty(configOptionValue))
			{
				properties["package.configs"] = Option.ConfigsOption.Value.ToString();
			}

			// if package.configs property is specified and config property is not specified, set config equal to first value in package.configs
			if (!properties.ContainsKey("config") )
			{
				if (properties.TryGetValue("package.configs", out string configsValue))
				{
					string config = configsValue.TrimWhiteSpace().ToArray().FirstOrDefault();
					properties["config"] = config;
				}
			}

			Project.SanitizeConfigProperties(log, ref properties);

			return properties;
		}

		private static void ShowBanner()
		{
			// Get version information directly from assembly.  This takes more work but keeps the version 
			// numbers being displayed in sync with what the assembly is marked with.
			Version version = Assembly.GetExecutingAssembly().GetName().Version;

			// Framework is typically versioned in the format x.yy.zz-w with leading zeroes, e.g. 7.15.06, or 8.03.00-1.
			// The assembly versions are ints so the zeroes get dropped - re-add them before we print.
			string versionString = String.Format("{0}.{1}.{2}", version.Major, version.Minor.ToString("00"), version.Build.ToString("00"));
			if (version.Revision != 0)
			{
				versionString = String.Format("{0}-{1}", versionString, version.Revision.ToString());
			}

			string title = Assembly.GetExecutingAssembly().GetName().Name;
			object[] titleAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyTitleAttribute), true);
			if (titleAttribute.Length > 0)
			{
				title = ((AssemblyTitleAttribute)titleAttribute[0]).Title;
			}

			string copyright = "";
			object[] copyRightAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCopyrightAttribute), true);
			if (copyRightAttribute.Length > 0)
			{
				copyright = ((AssemblyCopyrightAttribute)copyRightAttribute[0]).Copyright;
			}

			string link = "";
			object[] companyAttribute = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCompanyAttribute), true);
			if (companyAttribute.Length > 0)
			{
				link = ((AssemblyCompanyAttribute)companyAttribute[0]).Company;
			}

			System.Console.WriteLine("{0}-{1} {2}", title, versionString, copyright);
			if (!link.IsNullOrEmpty())
			{
				System.Console.WriteLine(link);
			}
			System.Console.WriteLine();
		}

		// returns true if auto version was used, out parameter contains return code from subprocess
		private static bool ProcessAutoVersion(Project project, MasterConfig masterConfig, string[] args, out int returnCode)
		{
			MasterConfig.Package masterPackage = masterConfig.GetMasterPackageCaseInsensitively(project, "framework");
			if (masterPackage == null)
			{
				throw new BuildException("No version of Framework found in the masterconfig. This must be set to use '" + Option.AutoVersion.Name + "'.");
			}
			string masterConfigVersion = masterPackage.Exceptions.ResolveException(project, masterPackage).Version; // ask masterconfig for version

			// determine if we want to use autoversion based on the following logic
			// in the normal case (masterconfig targets released version, we are released version):
			//      if the masterconfig version does not match current manifest version:
			//          use autoversion
			//      if the masterconfig is the same version but in *different location* than we expect:
			//          use autoversion
			//      else
			//          execute normally
			// if the masterconfig is targeting dev however we might well be the dev version so we can't trust assembly version
			// so we skip straight to checking location mistmatch
			Release frameworkRelease = PackageMap.Instance.GetFrameworkRelease(); // this returns the framework that is executing, not the one the masterconfig references
			Release release = PackageMap.Instance.FindOrInstallCurrentRelease(project, masterPackage.Name, onDemand: true);

			bool useAutoVersion = false;
			bool masterConfigTargetsThisVersion = StringExtensions.StrCompareVersions(frameworkRelease.Version, masterConfigVersion) == 0;
			if (masterConfigTargetsThisVersion)
			{
				useAutoVersion = release.Path != frameworkRelease.Path;
			}
			else
			{
				useAutoVersion = true;
			}

			if (useAutoVersion)
			{
				StringBuilder builder = new StringBuilder();
				foreach (string value in args)
				{
					if (value.StartsWith(Option.AutoVersion.Name))
					{
						continue;
					}

					builder.Append(value.UnescapeForCommandLine());
					builder.Append(' ');
				}

				try
				{
					// LogAppend option only exists in 8.xx.xx and above.
					// Version.Parse can't handle proper semantic versioning, e.g. 8.00.00-pr6, so just check the major version manually
					if (!(bool)Option.LogAppend.Value &&
						Convert.ToInt32(masterConfigVersion.Substring(0, masterConfigVersion.IndexOf('.'))) >= 8)
					{
						builder.Append(Option.LogAppend.Name.UnescapeForCommandLine());
					}
				}
				catch (Exception)
				{
					// If for some reason the Framework autoversion specified isn't a properly recognizable version string, just don't bother
				}

				project.Log.Minimal.WriteLine("Executing framework [{0}] using command line arguments: {1}.", masterConfigVersion, builder.ToString());

				Process process = new Process();
				process.StartInfo.FileName = release.Path + "\\bin\\nant.exe";
				process.StartInfo.Arguments = builder.ToString();

				using (ProcessRunner processRunner = new ProcessRunner(process, redirectOutput: false, redirectError: false, redirectIn: false))
				{
					process.StartInfo.CreateNoWindow = false; /* this correspond to the CREATE_NO_WINDOW process creation flag which is documented thus:

						The process is a console application that is being run without a console window. Therefore, the console handle for the application is not set.
						This flag is ignored if the application is not a console application, or if it is used with either CREATE_NEW_CONSOLE or DETACHED_PROCESS.

						The practical upshot of this is if this is true (default from ProcessRunner), then the nant subprocess we launch becomes detached from our
						(launching processes) console window - this prevents output from being automatically piped and console specific api such as Console.ReadKey
						(which boils down to ReadConsoleInput win32api function) from being usable in subprocess
					*/

					processRunner.StartAsync();
					processRunner.WaitForExit();
					project.Log.Info.WriteLine("Framework version [{0}] completed executing with command line {1}.", masterConfigVersion, process.StartInfo.Arguments);
					returnCode = processRunner.ExitCode;
					return true;
				}
			}

			returnCode = default(int);
			return false;
		}

		private static void ShowMasterconfig(Project project, MasterConfig masterConfig, List<string> packagesFilter)
		{
			XmlFormat xmlFormat = new XmlFormat
			(
				identChar: ' ',
				identation: 2,
				newLineOnAttributes: false,
				encoding: new UTF8Encoding(false, false)
			);

			using (XmlWriter writer = new XmlWriter(xmlFormat))
			{
				MasterConfigWriter msw = new MasterConfigWriter(writer, masterConfig, project, packagesFilter);

				msw.Write();
				writer.Flush();
				string generatedMasterconfig = new UTF8Encoding(false).GetString(writer.Internal.ToArray());

				// use console here, we absolutely want to dump this straight to console without any consideration
				// of log level or 
				System.Console.WriteLine(generatedMasterconfig);
			}
		}

		private static int ProcessTopLevelException(Exception e)
		{
			TextWriter outWriter = (bool)Option.UseStdErr.Value ? System.Console.Error : System.Console.Out;
			if (e is BuildException) // build exceptions are our own custom type, when we throw these in console runner ot's usually user error
			{
				Exception current = e;
				string offset = String.Empty;
				while (current != null)
				{
					if (current.Message.Length > 0)
					{
						if (current is BuildException && ((BuildException)current).Location != Location.UnknownLocation)
						{
							outWriter.WriteLine("[error] {0}error at {1} {2}", offset, ((BuildException)current).Location.ToString(), current.Message);
						}
						else
						{
							outWriter.WriteLine("{0}{1}", offset, current.Message);
						}
						offset += "  ";
					}
					current = current.InnerException;
				}
				System.Console.Out.WriteLine("Try 'nant -help' for more information"); // always use out for this line
				return (int)ReturnCode.Error;
			}
			else // treat anything else as internal logic bug
			{
				outWriter.WriteLine("[error] INTERNAL ERROR");
				outWriter.WriteLine("[error] " + e.ToString());
				outWriter.WriteLine();
				outWriter.WriteLine("[error] Please seek help via {0}.", CMProperties.CMSupportChannel);
				return (int)ReturnCode.InternalError;
			}
		}

		// Writes to console any incidents of mismatched keys detected (along with how many instances of each key variation)
		private static void LogPropertyCaseMismatches(Log log)
		{
			Dictionary<string, Dictionary<string, int>> mismatches = PropertyKey.GetMismatchTracker();

			var mismatchedKeys = mismatches.Where(x => x.Value.Count > 1);

			if (mismatchedKeys.Count() == 0)
			{
				log.Warning.WriteLine("No case mismatches detected in property keys.");
			}
			else
			{
				log.Warning.WriteLine("The following properties had multiple cases specified:");
				foreach (var mismatchedKey in mismatchedKeys)
				{
					log.Warning.WriteLine(mismatchedKey.Key + ":");
					foreach (var keyVariation in mismatchedKey.Value)
					{
						log.Warning.WriteLine("     " + keyVariation.Key + " : " + keyVariation.Value + " keys created");

					}
				}
			}
		}

		private static void PrintProcessEnvironment(Log log)
		{
			if (log.DebugEnabled)
			{
				try
				{
					log.Debug.WriteLine("--------------------------------------------------------------------------------------------------------------");
					log.Debug.WriteLine("Environment:");
					log.Debug.WriteLine("--------------------------------------------------------------------------------------------------------------");
					log.Debug.WriteLine(Environment.CommandLine);
					log.Debug.WriteLine("");
					log.Debug.WriteLine("CurrentDirectory=\"{0}\"", Environment.CurrentDirectory);
					log.Debug.WriteLine("");

					foreach (DictionaryEntry env in Environment.GetEnvironmentVariables())
					{
						log.Debug.WriteLine("\t{0}={1}", env.Key, env.Value);
					}

					log.Debug.WriteLine("");
					log.Debug.WriteLine("--------------------------------------------------------------------------------------------------------------");
					log.Debug.WriteLine("");
				}
				catch (Exception ex)
				{
					log.Debug.WriteLine("Unable to retrieve process environment: {0}", ex.Message);
				}
			}
		}

		private static bool IsEnvironmentVariableSet(string envVar)
		{
			string environmentValue = Environment.GetEnvironmentVariable(envVar);
			return environmentValue != null && (environmentValue == "1" || environmentValue.Equals("true", StringComparison.OrdinalIgnoreCase));
		}
	}
}