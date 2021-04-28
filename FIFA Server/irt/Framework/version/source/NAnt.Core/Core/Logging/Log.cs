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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

using NAnt.Core.PackageCore;

namespace NAnt.Core.Logging
{
	// DAVE-FUTURE-REFACTOR-TODO - be good to derive Log from an ILog and then
	// implement BufferedLog from ILog rather than log and it make capable of
	// recording any logging api call - this would make logging initialization
	// easier in ConsoleRunner since we could just accumulate events then when
	// log is fully set up then we replay all the events to the final log
	// (though this would mean changing almost everything to take an ILog,
	// probably better to make Log abstract and derive)

	// logger types that can be specifed on the command line
	public enum LoggerType : byte
	{
		Console = 1 << 0,
		File = 1 << 1,
	};

	// ILogListeners are object tht actually do something
	// with the incoming logging information such as 
	// writing to console or writing to file
	public interface ILogListener
	{
		void WriteLine(string arg);
		void Write(string arg);
	}

	// ILogs represent various logging levels / channels // TODO ILogWriter would be a much better name, but ILog is used in a couple of taskdefs in the wild
	// on a Log object, for example:
	//		Log.Debug.WriteLine()
	//		Log.Status.WriteLine()
	// Debug and Status are both ILogs, which may have
	// actual or null implementations depending on whether
	// that channel is active
	public interface ILog
	{
		void WriteLine();
		void WriteLine(string arg);
		void WriteLine(string format, params object[] args);
		void Write(string arg);
		void Write(string format, params object[] args);
	}

	public class Log
	{
		public enum LogLevel
		{
			Quiet,		// Error
			Minimal,	// Error, Warning, Minimal
			Normal,		// Error, Warning, Minimal, Status
			Detailed,	// Error, Warning, Minimal, Status, Info			(aka Verbose)
			Diagnostic	// Error, Warning, Minimal, Status, Info, Debug
		};

		public enum WarnLevel
		{
			Quiet,		// no warnings
			Minimal,	// no most common warnings (package roots, duplicate packages)
			Normal,		// warnings on conditions that can affect results
			Advise		// warnings on inconsistencies in build scripts from which framework can auto recover.
		};

		public enum DeprecateLevel
		{
			Quiet,		// no deprecation messages
			Minimal,	// only critical deprecations, typically apis that have not been removed but no longer do anything
			Normal,		// features that will be removed in future versions of Framework
			Advise		// a deprecated feature that we'd like to remove at some point but are in no rush to do so
		}

		// track where override settings for warnings / deprecations were previously set
		public enum SettingSource
		{
			CommandLine,
			MasterConfigGlobal,
			MasterConfigPackage,
			LegacyProperties,
			LegacyPackageProperties
		}

		// warning ids that can be enabled / disabled / set as / not as error
		public enum WarningId
		{
			NoId = 0,
			SyntaxError = 2002,							// Build script syntax error warnings (unknown / unrecognized elements / attributes, etc)
			ManifestFileError = 2003,					// Missing version info, mismatch version info, etc.
			DuplicateReleaseFound = 2004,				// Found duplicate package release in package roots.
			MasterconfigError = 2005,					// Warning messages from masterconfig file (packageroot not found, etc)
			InitializeAssumption = 2006,				// Warning messages for setup that is not properly initialized and we need to make assumptions.
			PackageStructure = 2007,					// Unable to decide FW1 vs FW2 vs FW3, missing manifest.xml, etc
			TaskCancelled = 2008,						// No build modules found, no buildable configs, cannot generate solution, etc.
			BuildScriptSetupError = 2009,				// Detected build script has conflicting info.
			LongPathWarning = 2010,						// Warnings about path being too long
			InconsistentVersionName = 2011,				// Warning about inconsistent top level package version name field in the manifest
			PackageRootDoesNotExist = 2012,				// Package root given in masterconfig file does not exist
			DuplicateReleaseInside = 2013,				// Duplicate package release found in sub-directory of flattened package
			PackageCompatibilityWarn = 2014,			// Incompatible package versions being used.
			InvalidModuleDepReference = 2015,			// Invalid module dependency reference
			InvalidTargetReference = 2016,				// Reference of non existing target
			InconsistentInitializeXml = 2017,			// Inconsistent data between build script and Initialize.xml
			StructuredXmlNotUsed = 2018,				// Warnings suggesting users should switch to Structured XML
			UsingImproperIncludes = 2019,				// Source file is using includes that is not properly setup in build script.
			FoundUnusedIncludes = 2020,					// The build for specific source file contain unused includes.
			MasterconfigURIHead = 2021,					// Warning about a masterconfig URI specify the head revision
			InvalidPublicDependencies = 2022,			// Warnings where public dependencies don't match actual dependencies
			ModuleSelfDependency = 2023,				// Module depends upon itself
			DuplicateCopyLocalDestination = 2024,		// Module copying same file to different locations in output, usually indicates a mistake in declaration.
			PublicDataLibMismatch = 2025,				// Warn that module's public data exports libary that does not exist and is not built by this package
			PublicDataLibMissing = 2026,				// A module has a dependency on another module that builds a library but that library is not exported
			PublicDataDllMismatch = 2027,				// Warn that module's public data exports dll that does not exist and is not built by this package
			PublicDataDllMissing = 2028,				// A module has a dependency on another module that builds a dll but that dll is not exported
			PublicDataAssemblyMismatch = 2029,			// Warn that module's public data exports assembly that does not exist and is not built by this package
			PublicDataAssemblyMissing = 2030,			// A module has a dependency on another module that builds an assembly but that assembly is not exported
			CannotDetermineVersion = 2031,				// Unable to determine package version
			PublicBuildDependency = 2032,
			ProxyMapUnrecognizedCentralServer = 2035,	// warning about server in URI not being listed in the proxy map file
			ProxyMapAlternateCentralServerName = 2036,	// warning that the server in the URI is not the same as the one in the proxy map
			InconsistentMasterconfigAttribute = 2037,	// user has the same version of a package in two masterconfig fragments, but with different attribute settings - this is valid but typically unintentional
			LocalOndemandIncorrect = 2038,				// warning throw when framework detects that localondemand is set to the wrong state based on a package's URI or version name
			RemovedConfigPackageInMasterconfig = 2039,	// when a platform config package that was merged into eaconfig is found in the masterconfig
			MetaPackageNotFound = 2040,					// when a metapackage in the masterconfig is not found and cannot be downloaded ondemand
			DependentCaseMismatch = 2041,				// <dependent name="mypackage"/> doesn't match masterconfig casing <package name="MyPackage"/>
			LegacyGenerateBuildLayoutTask = 2041,       // warn when the legacy generateBuildLayout task is being used
			DuplicateModuleInBuildModulesProp = 2042,   // warn when a user specifies a module twice in a buildmodules property
			ModuleInInitializeButNotBuildScript = 2043, // warn when a module is declared in the initialize.xml build data but is not declared or declared in the wrong group in the build script
			PackageServerErrors = 5001,					// Package download errors (using perforce, web service, etc)
			SystemIOError = 5002,						// Unable to create directory, delete directory, etc.
			PackageCompatibility = 5003,				// Incompatible package versions being used.
			BuildFailure = 5004,						// General build failure message, task fail on error message. (We currently don't throw build exception with build failure!)
			FoundIncludeIrregularities = 5005,			// Final build error with if warning messages UsingRelativeIncludes or FoundUnusedIncludes have been triggered.
			UndefinedBehavior = 5006,					// User is relying on undefined behavior which may bite them.
			CSharpWarnings = 5007,						// warnigns were thrown from script or taskdef code
			NAntTargetAsBuildStep = 5008,				// using targets rather than commands for build steps - these are generally unsafe and slow
			NAntTargetAsBuildStepPortable = 5009,		// using targets rather than commands in portable setup - this only works if Framework is correctly relative to the shipped portable project
			LegacyCommandPropertiesOptions = 5010,		// nant task is using the old misspelt command line optionset name
			MissingVisualStudioComponents = 5011,       // Visual Studio detection notice there are missing components that might be needed!
			DeprecatedPackage = 5012,                   // Deprecated package that is now part of Framework still listed in masterconfig
			GroupBuildTypeSpecifiedWithModules = 5013,  // buildtype is specified for package rather than module even when modules are declared
			LegacyConfigFileFormat = 5014,              // user is using legacy style config that doesn't define optionset and must manually include needed files
            VisualStudioPreviewNotSupported = 5015,     // Preview versions of Visual Studio only exist for the most recent release
			VisualStudioTargetWithNonProxyTools = 5016, // calling visual studio target when using msbuild non-proxy - will probably have incorrect behaviour
			DuplicateConfigInExtension = 5017,          // Config extension is trying to override a config from a prior extension or main config package
			InvalidWildCardConfigPattern = 5018			// masterconfig includes / excludes have in valid wild card format
		}

		public enum DeprecationId
		{
			NoId = 0,
			ScriptTaskLanguageParameter = 1,	// script task now only supports C#
			MetricTask = 2,						// metric task does nothing
			ProjectEvents = 3,					// project events do nothing
			OldLogWriteAPI = 4,					// calling old logging functions we want to deprecate
			PackageInitializeSelf = 5,			// packages always initializeself now
			// OnInitializeXmlLoad = 6,			// the on-initializexml-load target is deprecated (this was un-deprecated - see TODO in PackageInitializercs)
			OldP4ConnectAPI = 7,				// old p4connect api being used
			ProjectDisposal = 8,				// project doesn't need to be IDisposable anymore
			NAntTaskDisposal = 9,				// nanttask doesn't need to be IDisposable anymore
			ErrorSuppression = 10,				// no longer support suppressing error codes
			OldOptionSetAPI = 11,				// old optionset api
			OldWarningSuppression = 12,			// old way of suppressing warnings via properties
			ModifyingTargetStyle = 13,			// TargetStyle shouldn't be modified, if you need to change it do so in a subproject scope
			DependentVersion = 14,				// dependent task ignores version parameter	
			GetModuleBaseTypeName = 15,			// getmodulebasetype name parameter is deprecated
			PublicDataDefaults = 16,			// add-defaults for module or package public data is deprecated - there is nicer syntax now
			ScriptInit = 17,					// ScriptInit task is deprecated in favour of public data
			OldWarningsAsErrors = 18,			// old ways of converting all enabled warnings to errors
			SysInfo = 19,						// sysinfo task is no longer needed, env vars are always turned into properties at project init
			EaConfigOptionsOverrideFile = 20,	// using eaconfig.options-override.file property to override config settings
			LegacyScriptCompilerRefernces = 21, // using the override property that uses legacy reference resolution for taskdef and script
			ModulelessPackage = 22,				// package uses old style syntax for declaring a package which is implicitly converted do dmouel
			PublicJavaElements = 23,			// module is exporting java publicdata - java modules don't need public data as everthing is handled in Gradle build
			PrivateJavaElements = 24,           // module has private java sourcefiles
			ConfigTargetOverrides = 25,         // user is defining the old 'config.targetoverrides' optionsnet
			InternalDependencies = 26,          // internal dependencies were never used
			ModuleTargetFrameworkVersion = 27   // trying to set target framework version on per module level, this is only settable by DotNet or DotNetCoreSdk package versions
		}

		private abstract class SuppressableMessage<TMessageId> /*where TMessageId : Enum*/ // constraint needs C# 7.3 or greater 
		{
			// this dictionary tracks, for each id, a list of "key arrays" which each indicate a set of circumstance in which that message has already been thrown,
			// if a message id is thrown with a key array that has been thrown before then it is suppressed to avoid spamming
			protected static readonly ConcurrentDictionary<TMessageId, List<object[]>> s_throwOnceKeys = new ConcurrentDictionary<TMessageId, List<object[]>>();

			protected struct MessageState
			{
				internal bool StateValue;
				internal SettingSource Source;
			}

			protected readonly Log m_log;

			// Map of codes to their override enabled/disabled error/not-as-error state
			// this is a concurrent dictionary out of paranoia more than anything - the apis for modifying status
			// are public and while generally won't be called from multiple threads since we expect them to be set at log
			// creation (local) / program statrup (global), it doesn't cost us much to use a concurrent container here
			protected readonly ConcurrentDictionary<TMessageId, MessageState> m_localEnabledState;

			// same as above but global
			protected readonly static ConcurrentDictionary<TMessageId, MessageState> s_globalEnabledState = new ConcurrentDictionary<TMessageId, MessageState>();

			protected SuppressableMessage(Log log)
			{
				m_log = log;
				m_localEnabledState = new ConcurrentDictionary<TMessageId, MessageState>();
			}

			protected SuppressableMessage(Log log, SuppressableMessage<TMessageId> other)
			{
				m_log = log;
				m_localEnabledState = other.m_localEnabledState;
			}

			protected static void AttemptUpdateSetting(Log log, ConcurrentDictionary<TMessageId, MessageState> updateDictionary, TMessageId id, bool settingValue, SettingSource settingSource, string setTrueDescription, string setFalseDescription, string codePrefix, string codeDescription)
			{
				bool featureEnabled = (codePrefix == "D" ? log.DeprecationEnabled : log.WarningEnabled);
				ILog logger = (codePrefix == "D" ? log.Deprecation : log.Warning);
				updateDictionary.AddOrUpdate
				(
					id,
					(newId) => new MessageState { StateValue = settingValue, Source = settingSource },
					(existingId, existingState) =>
					{
						if (existingState.StateValue != settingValue && featureEnabled)
						{
							string existingStateDesc = existingState.StateValue ? setTrueDescription : setFalseDescription;
							string originalSourceDesc = GetSourceDescription(existingState.Source);
							string attemptSourceDesc = GetSourceDescription(settingSource);
							logger.WriteLine($"{codeDescription} {codePrefix}{StringifyId(id)} already explicitly {existingStateDesc} from {originalSourceDesc}, setting from {attemptSourceDesc} ignored.");
						}
						return existingState;
					}
				);
			}

			protected static string StringifyId(TMessageId id)
			{
				return ((int)(object)id).ToString().PadLeft(4, '0'); // this casting grossness is get around the fact we can't set an enum constraint on TMessageId yet
			}

			protected static string GetSourceDescription(SettingSource source)
			{
				switch (source)
				{
					case SettingSource.MasterConfigGlobal:
						return "masterconfig";
					case SettingSource.MasterConfigPackage:
						return "masterconfig package";
					case SettingSource.LegacyProperties:
						return "legacy properties";
					case SettingSource.LegacyPackageProperties:
						return "legacy package properties";
					case SettingSource.CommandLine:
					default:
						return "command line";
				}
			}

			protected static bool KeyAlreadyThrownForId(IEnumerable<object> key, TMessageId id)
			{
				// see if this deprecation has been thrown before with the same key, in which case abort
				if (key != null && key.Any())
				{
					List<object[]> objectKeyArrays = s_throwOnceKeys.GetOrAdd(id, (newId) => new List<object[]>());
					lock (objectKeyArrays)
					{
						object[] keyArray = key.ToArray();
						if (KeyAlreadyUsed(keyArray, objectKeyArrays))
						{
							return true;
						}

						// if we made it here then this is a new key
						objectKeyArrays.Add(keyArray);
					}
				}
				return false;
			}

			private static bool KeyAlreadyUsed(object[] keyArray, List<object[]> objectKeyArrays)
			{
				foreach (object[] existingKeyArray in objectKeyArrays)
				{
					if (keyArray.Length != existingKeyArray.Length)
					{
						continue;
					}

					bool equal = true;
					for (int i = 0; i < keyArray.Length; ++i)
					{
						if (!keyArray[i].Equals(existingKeyArray[i]))
						{
							equal = false;
							break;
						}
					}

					if (equal)
					{
						return true;
					}
				}

				return false;
			}
		}

		private sealed class WarningMessage : SuppressableMessage<WarningId>
		{
			// Map of warnings code to their override error/not-as-error state
			// this is a concurrent dictionary out of paranoia more than anything - the apis for modifying warnings status
			// are public and while generally won't be called from multiple threads since we expect them to be set at log
			// creation (local) / program statrup (global), it doesn't cost us much to use a concurrent container here
			private readonly ConcurrentDictionary<WarningId, MessageState> m_localAsErrorState;

			// same as above but global
			private static readonly ConcurrentDictionary<WarningId, MessageState> s_globalAsErrorState = new ConcurrentDictionary<WarningId, MessageState>();

			// avoid repeating warning about invalid warning ids, value is unused - concurent hashset is not readily available however
			private static readonly ConcurrentDictionary<string, bool> s_invalidWarningWarningsIssued = new ConcurrentDictionary<string, bool>();

			// used by legacy warning properties - can be removed if they are deprecated
			private bool m_suppressAll;

			internal WarningMessage(Log log)
				: base(log)
			{
				m_suppressAll = false;
				m_localAsErrorState = new ConcurrentDictionary<WarningId, MessageState>();
			}

			internal WarningMessage(Log log, WarningMessage other)
				: base(log, other)
			{
				m_suppressAll = other.m_suppressAll;
				m_localAsErrorState = other.m_localAsErrorState;
			}

			internal void ThrowWarning(WarningId id, WarnLevel level, string format, params object[] args)
			{
				ThrowWarning(id, level, null, format, args);
			}

			internal void ThrowWarning(WarningId id, WarnLevel level, IEnumerable<object> key, string format, params object[] args)
			{
				if (!IsWarningEnabled(id, level, out bool asError))
				{
					return;
				}

				if (KeyAlreadyThrownForId(key, id))
				{
					return;
				}

				// throw the warning
				if (asError)
				{
					string warningAsErrorAdditional = $" \r\nThis error can be demoted to warning via the <warnings> section in the masterconfig or via command line option -wn:{StringifyId(id)}.";
					string message = $"[W{StringifyId(id)} WARNING-AS-ERROR] {String.Format(format, args)}{warningAsErrorAdditional}";

					throw new BuildException(message);
				}
				else if (m_log.WarningEnabled)
				{
					m_log.Warning.WriteLine($"[W{StringifyId(id)}] {String.Format(format, args)}");
				}
			}

			internal bool IsWarningEnabled(WarningId id, WarnLevel level, out bool asError)
			{
				// suppress all option trumps everything
				if (m_suppressAll)
				{
					asError = false;
					return false;
				}

				bool enabled = false;

				// determine if this warning should be convert to error
				if (!m_localAsErrorState.TryGetValue(id, out MessageState asErrorOverrideState) && !s_globalAsErrorState.TryGetValue(id, out asErrorOverrideState))
				{
					asError = level <= m_log.WarningAsErrorLevel; // if not explicitly an error then only throw as error if it's below the maximum warning as error level
				}
				else
				{
					asError = asErrorOverrideState.StateValue; // error is determine by override value if one exists
					enabled = asErrorOverrideState.StateValue; // if explicity set to error force enabled
				}

				// determine if warning is even enabled
				if (!enabled) // is this was force enabled by explicitly being set as error skip this check
				{
					if (!m_localEnabledState.TryGetValue(id, out MessageState enabledOverrideState) && !s_globalEnabledState.TryGetValue(id, out enabledOverrideState))
					{
						enabled = level <= m_log.WarningLevel;
					}
					else
					{
						enabled = enabledOverrideState.StateValue;
					}
				}

				return enabled;
			}

			// sets the enabled state override for this log instance
			internal void SetWarningEnabledIfNotSet(WarningId id, bool enabled, SettingSource enabledSettingSource)
			{
				AttemptUpdateSetting
				(
					m_log,
					m_localEnabledState,
					id,
					enabled,
					enabledSettingSource,
					setTrueDescription: "enabled",
					setFalseDescription: "disabled",
					codePrefix: "W",
					codeDescription: "Warning"
				);
			}

			// sets the warning as error override for this log instance
			internal void SetWarningAsErrorIfNotSet(WarningId id, bool asError, SettingSource asErrorSettingSource)
			{
				AttemptUpdateSetting
				(
					m_log,
					m_localAsErrorState,
					id,
					asError,
					asErrorSettingSource,
					setTrueDescription: "set as error",
					setFalseDescription: "set never to be error",
					codePrefix: "W",
					codeDescription: "Warning"
				);
			}

			// sets the global enabled state for a warning id - note this is still overridden by log's local
			// overrides
			internal static void SetGlobalWarningEnabledIfNotSet(Log log, WarningId id, bool enabled, SettingSource enabledSettingSource)
			{
				AttemptUpdateSetting
				(
					log,
					s_globalEnabledState,
					id,
					enabled,
					enabledSettingSource,
					setTrueDescription: "enabled",
					setFalseDescription: "disabled",
					codePrefix: "W",
					codeDescription: "Warning"
				);
			}

			// sets the global enabled state for a warning id - note this is still overridden by log's local
			// overrides
			internal static void SetGlobalWarningAsErrorIfNotSet(Log log, WarningId id, bool asError, SettingSource asErrorSettingSource)
			{
				AttemptUpdateSetting
				(
					log,
					s_globalAsErrorState,
					id,
					asError,
					asErrorSettingSource,
					setTrueDescription: "set as error",
					setFalseDescription: "set never to be error",
					codePrefix: "W",
					codeDescription: "Warning"
				);
			}

			internal void ApplyLegacyProjectSuppressionProperties(Project project)
			{
				if (project.Properties.GetBooleanPropertyOrDefault("nant.warningsaserrors", false))
				{
					m_log.ThrowDeprecation // deprecated 2018/07/30
					(
						DeprecationId.OldWarningsAsErrors, DeprecateLevel.Advise, new object[] { "nant.warningsaserrors" }, // TODO keeping this at advise level for now, but should be brought down over time
						"The 'nant.warningsaserrors' property is a older method for enabling warnings as errors. Use -warnaserrorlevel option instead."
					);
					m_log.WarningAsErrorLevel = m_log.WarningLevel;
				}

				if (project.Properties.GetBooleanPropertyOrDefault("nant.warningsuppression.all", false))
				{
					m_log.ThrowDeprecation // deprecated 2018/07/30
					(
						DeprecationId.OldWarningSuppression, DeprecateLevel.Advise, new object[] { "nant.warningsuppression.all" }, // TODO keeping this at advise level for now, but should be brought down over time
						"The 'nant.warningsuppression' property is a older method for suppressing warnings. Instead you can manipulate warning settings via <warnings> section in the masterconfig or command line options."
					);
					m_suppressAll = true;
				}

				// set warning suppression individually even if all may have been suppressed above - this settings can later
				// be overidden by package but it may only set some of them
				string warningSuppression = project.Properties.GetPropertyOrDefault("nant.warningsuppression", null);
				if (warningSuppression != null)
				{
					m_log.ThrowDeprecation // deprecated 2018/07/30
					(
						DeprecationId.OldWarningSuppression, DeprecateLevel.Advise, new object[] { "nant.warningsuppression" }, // TODO keeping this at advise level for now, but should be brought down over time
						"The 'nant.warningsuppression' property is a older method for suppressing warnings. Instead you can manipulate warning settings via <warnings> section in the masterconfig or command line options."
					);
					UpdateSuppressionList(warningSuppression, SettingSource.LegacyProperties);
				}

				string errorSuppression = project.Properties.GetPropertyOrDefault("nant.errorsuppression", null);
				if (errorSuppression != null)
				{
					m_log.ThrowDeprecation // deprecated 2018/07/30
					(
						DeprecationId.ErrorSuppression, DeprecateLevel.Minimal, new object[] { "nant.errorsuppression" }, 
						"Suppressing errors via the 'nant.errorsuppression' property is no longer supported."
					); 
				}
			}

			internal void ApplyLegacyPackageSuppressionProperties(Project project, string packageName)
			{
				if (project.Properties.GetBooleanPropertyOrDefault($"package.{packageName}.nant.warningsuppression.all", false))
				{
					m_suppressAll = true;
				}

				string warningSuppression = project.Properties.GetPropertyOrDefault($"package.{packageName}.nant.warningsuppression", null);
				if (warningSuppression != null)
				{
					m_log.ThrowDeprecation // deprecated 2018/08/07
					(
						DeprecationId.OldWarningSuppression, DeprecateLevel.Advise, new object[] { $"package.{packageName}.nant.warningsuppression" },
						"The 'nant.{0}.errorsuppression' property is a older method for suppressing warnings. Instead you can manipulate warning settings via <warnings> section in the masterconfig or command line options.",
						packageName 
					);
					UpdateSuppressionList(warningSuppression, SettingSource.LegacyPackageProperties);
				}

				string errorSuppression = project.Properties.GetPropertyOrDefault($"package.{packageName}.nant.errorsuppression", null);
				if (errorSuppression != null)
				{
					m_log.ThrowDeprecation // deprecated 2018/07/30
					(
						DeprecationId.ErrorSuppression, DeprecateLevel.Minimal, 
						new object[] { packageName }, "Suppressing errors via the 'package.{0}.nant.errorsuppression' property is no longer supported.", packageName
					); 
				}
			}

            // for testing only
            internal static void ResetGlobalWarningStates()
            {
                s_globalEnabledState.Clear();
                s_globalAsErrorState.Clear();
				s_invalidWarningWarningsIssued.Clear();
            }

			private void UpdateSuppressionList(string warningSuppression, SettingSource settingSource)
			{
				foreach (string id in warningSuppression.ToArray(new char[] { '\x0009', '\x000a', '\x000d', '\x0020', ',' }))
				{
					int val = Convert.ToInt32(id);
					if (Enum.IsDefined(typeof(WarningId), val))
					{
						// don't set using the normal Set..IfNotSet api - these properties get to stomp directly in order to preserve old behaviour
						m_localEnabledState.AddOrUpdate
						(
							(WarningId)val,
							(newId) => new MessageState() { StateValue = false, Source = settingSource },
							(existingId, existingState) =>
							{
								existingState.Source = settingSource;
								existingState.StateValue = false;
								return existingState;
							}
						);

						// diable from being as an erorr
						m_localAsErrorState.AddOrUpdate
						(
							(WarningId)val,
							(newId) => new MessageState() { StateValue = false, Source = settingSource },
							(existingId, existingState) =>
							{
								existingState.Source = settingSource;
								existingState.StateValue = false;
								return existingState;
							}
						);
					}
					else
					{
						if (!s_invalidWarningWarningsIssued.ContainsKey(id))
						{
							s_invalidWarningWarningsIssued[id] = true;
							m_log.Warning.WriteLine("Nant warning suppression id {0} does not exist! Suppressed from {1}.", id, GetSourceDescription(settingSource));
						}
					}
				}
			}
		}

		private sealed class DeprecationMessage : SuppressableMessage<DeprecationId>
		{
			internal DeprecationMessage(Log log)
				: base(log)
			{
			}

			internal DeprecationMessage(Log log, DeprecationMessage other)
				: base(log, other)
			{
			}

			internal void ThrowDeprecation(DeprecationId id, DeprecateLevel level, string format, params object[] args)
			{
				ThrowDeprecation(id, level, null, format, args);
			}

			internal void ThrowDeprecation(DeprecationId id, DeprecateLevel level, IEnumerable<object> key, string format, params object[] args)
			{
				if (!IsDeprecationEnabled(id, level))
				{
					return;
				}

				if (!m_log.DeprecationEnabled)
				{
					return;
				}

				if (KeyAlreadyThrownForId(key, id))
				{
					return;
				}

				// throw the deprecation
				m_log.Deprecation.WriteLine($"[D{StringifyId(id)}] {String.Format(format, args)}");
			}

			internal bool IsDeprecationEnabled(DeprecationId id, DeprecateLevel level)
			{
				if (!m_localEnabledState.TryGetValue(id, out MessageState enabledOverrideState) && !s_globalEnabledState.TryGetValue(id, out enabledOverrideState))
				{
					return level <= m_log.DeprecationLevel;
				}
				else
				{
					return enabledOverrideState.StateValue;
				}
			}

			// sets the enabled deprecation override for this log instance
			internal void SetDeprecationOverrideEnabledIfNotSet(DeprecationId id, bool enabled, SettingSource enabledSettingSource)
			{
				AttemptUpdateSetting
				(
					m_log,
					m_localEnabledState,
					id,
					enabled,
					enabledSettingSource,
					setTrueDescription: "enabled",
					setFalseDescription: "disabled",
					codePrefix: "D",
					codeDescription: "Deprecation"
				);
			}

			// sets the global override level for a warning id - note this is still overridden by log's local
			// overrides,
			internal static void SetGlobalDeprecationEnabledIfNotSet(Log log, DeprecationId id, bool enabled, SettingSource enabledSettingSource)
			{
				AttemptUpdateSetting
				(
					log,
					s_globalEnabledState,
					id,
					enabled,
					enabledSettingSource,
					setTrueDescription: "enabled",
					setFalseDescription: "disabled",
					codePrefix: "D",
					codeDescription: "Deprecation"
				);
			}

			// for testing only
			internal static void ResetGlobalDeprecationStates()
			{
				s_globalEnabledState.Clear();
			}
		}

		public int IndentLevel
		{
			get { return m_indentLevel; }
			set
			{
				if (value >= 0)
				{
					m_indentLevel = value;
					Padding = String.Empty.PadRight(IndentLength * m_indentLevel);
				}
			}
		}

		public string Padding { get; private set; }
		public int IndentSize { get { return IndentLength; } } // TODO - deprecate API, use IndentLength constant instead - can't just make this a constant due to it being used as instance member (e,g, myLog.IndentSize)

		public ReadOnlyCollection<ILogListener> Listeners { get { return Array.AsReadOnly(m_listeners); } }
		public ReadOnlyCollection<ILogListener> ErrorListeners { get { return Array.AsReadOnly(m_errorListeners); } }

		public const int IndentLength = 4;
		public const string DefaultLogFileName = "BuildLog.txt";

		public WarnLevel WarningAsErrorLevel; // this can't be readonly because it can be modified by project properties - if we deprecate that mechanism then this can readonly

		public readonly WarnLevel WarningLevel;
		public readonly DeprecateLevel DeprecationLevel;

		public readonly string Id;
		public readonly LogLevel Level;

		public readonly ILog Error;
		public readonly ILog Warning;
		public readonly ILog Deprecation;
		public readonly ILog Minimal;
		public readonly ILog Status;
		public readonly ILog Info;
		public readonly ILog Debug;
		public readonly ILog DeveloperWarning;

		public readonly bool DeprecationEnabled;
		public readonly bool WarningEnabled;
		public readonly bool MinimalEnabled;
		public readonly bool StatusEnabled;
		public readonly bool InfoEnabled;
		public readonly bool DebugEnabled;
		public readonly bool DeveloperWarningEnabled;

		private readonly ILogListener[] m_listeners;
		private readonly ILogListener[] m_errorListeners;
		private readonly WarningMessage m_warningMessage;
		private readonly DeprecationMessage m_deprecationMessage;

		// the Default log is design to be used outside of contexts where logging parameters are taken into account
		// i.e. any application that needs a Log that isn't Framework
		public static readonly Log Default = new Log
		(
			listeners: new ILogListener[] { new ConsoleListener() },
            errorListeners: new ILogListener[] { new ConsoleStdErrListener() }
		);

		// silent log if for where you want to suppress logging where a log instance is required, useful for unit 
		// testing and applications that should not produce output (note, even though the level is quiet, quiet
		// level does still process error - but there are no listerners so it doesn't go anywhere)
		public static readonly Log Silent = new Log
		(
			level: LogLevel.Quiet,
			warningLevel: WarnLevel.Quiet,
			deprecationLevel: DeprecateLevel.Quiet
		);

		private int m_indentLevel;

		// constructor for creating a new log
		public Log(LogLevel level = LogLevel.Normal, WarnLevel warningLevel = WarnLevel.Normal, WarnLevel warningAsErrorLevel = WarnLevel.Quiet, DeprecateLevel deprecationLevel = DeprecateLevel.Normal,
			int indentLevel = 0, string id = "", IEnumerable<ILogListener> listeners = null, IEnumerable<ILogListener> errorListeners = null, bool enableDeveloperWarnings = false)
			: this
			(
				level: level,
				warningLevel: warningLevel,
				warningAsErrorLevel: warningAsErrorLevel,
				deprecationLevel: deprecationLevel,
				indentLevel: indentLevel >= 0 ? indentLevel : 0,
				id: id,
				listeners: (listeners ?? Enumerable.Empty<ILogListener>()).ToArray(),
				errorListeners: (errorListeners ?? Enumerable.Empty<ILogListener>()).ToArray(),
				warningMessage: null, // pass null then set in consructor body - need this to be fully constructed before we can pass it to WarningMessage constructor
				deprecationMessage: null,
				enableDeveloperWarnings: enableDeveloperWarnings
			)
		{
			m_warningMessage = new WarningMessage(this);
			m_deprecationMessage = new DeprecationMessage(this);
		}

		// constructor for copying a log but optionally overriding certain internals
		public Log(Log log, LogLevel? level = null, string id = null, IEnumerable<ILogListener> listeners = null, IEnumerable<ILogListener> errorListeners = null)
			: this
			(
				level: level ?? log.Level,
				warningLevel: log.WarningLevel,
				warningAsErrorLevel: log.WarningAsErrorLevel,
				deprecationLevel: log.DeprecationLevel,
				indentLevel: log.IndentLevel,
				id: id ?? log.Id,
				listeners: listeners?.ToArray() ?? log.m_listeners,
				errorListeners: errorListeners?.ToArray() ?? log.m_errorListeners,
				warningMessage : log.m_warningMessage,
				deprecationMessage : log.m_deprecationMessage,
				enableDeveloperWarnings : log.DeveloperWarningEnabled
			)
		{
		}

		// internal base constructor
		private Log(LogLevel level, WarnLevel warningLevel, WarnLevel warningAsErrorLevel, DeprecateLevel deprecationLevel, int indentLevel, string id, ILogListener[] listeners, ILogListener[] errorListeners, WarningMessage warningMessage, DeprecationMessage deprecationMessage, bool enableDeveloperWarnings = false)
		{
			Id = id;
			Level = level;
			WarningLevel = warningLevel;
			WarningAsErrorLevel = warningAsErrorLevel;
			DeprecationLevel = deprecationLevel;
			IndentLevel = indentLevel;

			m_listeners = listeners;
			m_errorListeners = errorListeners;

			m_warningMessage = warningMessage;
			m_deprecationMessage = deprecationMessage;

			DeprecationEnabled = level > LogLevel.Quiet && DeprecationLevel > DeprecateLevel.Quiet;
			WarningEnabled = level > LogLevel.Quiet && WarningLevel > WarnLevel.Quiet;
			MinimalEnabled = level >= LogLevel.Minimal;
			StatusEnabled = level >= LogLevel.Normal;
			InfoEnabled = level >= LogLevel.Detailed;
			DebugEnabled = level >= LogLevel.Diagnostic;
			DeveloperWarningEnabled = enableDeveloperWarnings;

			Error = new LogWriter(this, m_errorListeners, " [error] ");
			Warning = WarningEnabled ? (ILog)new LogWriter(this, m_listeners, " [warning] ") : new NullLogWriter();
			Deprecation = DeprecationEnabled ? (ILog)new LogWriter(this, m_listeners, " [deprecated] ") : new NullLogWriter();
			Minimal = MinimalEnabled ? (ILog)new LogWriter(this, m_listeners, String.Empty) : new NullLogWriter();
			Status = StatusEnabled ? (ILog)new LogWriter(this, m_listeners, String.Empty) : new NullLogWriter();
			Info = InfoEnabled ? (ILog)new LogWriter(this, m_listeners, String.Empty) : new NullLogWriter();
			Debug = DebugEnabled ? (ILog)new LogWriter(this, m_listeners, " [debug] ") : new NullLogWriter();
			DeveloperWarning = enableDeveloperWarnings ? (ILog)new LogWriter(this, m_listeners, " [DEVELOPER WARNING] ") : new NullLogWriter();
		}

		[Obsolete("Calling Write on Log object directly is deprecated. Use the specific logging channel e.g Log.Status.Write(...).")]
		public void Write(string format, params object[] args)
		{
			ThrowDeprecation(DeprecationId.OldLogWriteAPI, DeprecateLevel.Normal, "Calling Write on Log object directly is deprecated.Use the specific logging channel e.g Log.Status.Write(...).");
			Status.Write(format, args);
		}

		[Obsolete("Calling WriteLine on Log object directly is deprecated. Use the specific logging channel e.g Log.Status.WriteLine(...).")]
		public void WriteLine(string format, params object[] args)
		{
			ThrowDeprecation(DeprecationId.OldLogWriteAPI, DeprecateLevel.Normal, "Calling WriteLine on Log object directly is deprecated.Use the specific logging channel e.g Log.Status.WriteLine(...).");
			Status.WriteLine(format, args);
		}

		[Obsolete("Calling WriteIf on Log object directly is deprecated. Use the specific logging channel e.g Log.Status.Write(...).")]
		public void WriteIf(bool condition, string format, params object[] args)
		{
			ThrowDeprecation(DeprecationId.OldLogWriteAPI, DeprecateLevel.Normal, "Calling WriteIf on Log object directly is deprecated.Use the specific logging channel e.g Log.Status.Write(...).");
			if (condition)
			{
				Status.Write(format, args);
			}
		}

		[Obsolete("Calling WriteLineIf on Log object directly is deprecated. Use the specific logging channel e.g Log.Status.WriteLine(...) within an if statement.")]
		public void WriteLineIf(bool condition, string format, params object[] args)
		{
            ThrowDeprecation(DeprecationId.OldLogWriteAPI, DeprecateLevel.Normal, "Calling WriteLineIf on Log object directly is deprecated .Use the specific logging channel e.g Log.Status.WriteLine(...) within an if statement.");
            if (condition)
			{
				Status.WriteLine(format, args);
			}
		}

		public void ThrowWarning(WarningId id, WarnLevel level, string format, params object[] args)
		{
			m_warningMessage.ThrowWarning(id, level, format, args);
		}

		public void ThrowWarning(WarningId id, WarnLevel level, IEnumerable<object> key, string format, params object[] args)
		{
			m_warningMessage.ThrowWarning(id, level, key, format, args);
		}

		public void ThrowDeprecation(DeprecationId id, DeprecateLevel level, string format, params object[] args)
		{
			m_deprecationMessage.ThrowDeprecation(id, level, format, args);
		}

		public void ThrowDeprecation(DeprecationId id, DeprecateLevel level, IEnumerable<object> key, string format, params object[] args)
		{
			m_deprecationMessage.ThrowDeprecation(id, level, key, format, args);
		}

		public bool IsWarningEnabled(WarningId id, WarnLevel level)
		{
			return m_warningMessage.IsWarningEnabled(id, level, out bool asError);
		}

		public bool IsWarningEnabled(WarningId id, WarnLevel level, out bool asError)
		{
			return m_warningMessage.IsWarningEnabled(id, level, out asError);
		}

		public bool IsDeprecationEnabled(DeprecationId id, DeprecateLevel level)
		{
			return m_deprecationMessage.IsDeprecationEnabled(id, level);
		}

		// sets the warning level override for this log instance
		public void SetWarningEnabledIfNotSet(WarningId id, bool enabled, SettingSource enabledSettingSource)
		{
			m_warningMessage.SetWarningEnabledIfNotSet(id, enabled, enabledSettingSource);
		}

		// sets the warning as error override for this log instance
		public void SetWarningAsErrorIfNotSet(WarningId id, bool asError, SettingSource asErrorSettingSource)
		{
			m_warningMessage.SetWarningAsErrorIfNotSet(id, asError, asErrorSettingSource);
		}

		// sets the deprecation level override for this log instance
		public void SetDeprecationEnabledIfNotSet(DeprecationId id, bool enabled, SettingSource enabledSettingSource)
		{
			m_deprecationMessage.SetDeprecationOverrideEnabledIfNotSet(id, enabled, enabledSettingSource);
		}

		public void ApplyPackageSuppressions(Project project, string packageName)
		{
			if (PackageMap.Instance.TryGetMasterPackage(project, packageName, out MasterConfig.IPackage masterPackage))
			{
				foreach (KeyValuePair<WarningId, MasterConfig.WarningDefinition> warning in masterPackage.Warnings)
				{
					if (warning.Value.Enabled.HasValue)
					{
						SetWarningEnabledIfNotSet(warning.Key, warning.Value.Enabled.Value, SettingSource.MasterConfigPackage);
					}
					if (warning.Value.AsError.HasValue)
					{
						SetWarningAsErrorIfNotSet(warning.Key, warning.Value.AsError.Value, SettingSource.MasterConfigPackage);
					}
				}
				foreach (KeyValuePair<DeprecationId, MasterConfig.DeprecationDefinition> deprecation in masterPackage.Deprecations)
				{
					SetDeprecationEnabledIfNotSet(deprecation.Key, deprecation.Value.Enabled, SettingSource.MasterConfigPackage);
				}
			}
		}

		public void ApplyLegacyProjectSuppressionProperties(Project project)
		{
			m_warningMessage.ApplyLegacyProjectSuppressionProperties(project);
		}

		public void ApplyLegacyPackageSuppressionProperties(Project project, string packageName)
		{
			m_warningMessage.ApplyLegacyPackageSuppressionProperties(project, packageName);
		}

		// sets the global override level for a warning id - note this is still overridden by log's local
		// overrides,
		public static void SetGlobalWarningEnabledIfNotSet(Log log, WarningId id, bool enabled, SettingSource enabledSettingSource)
		{
			WarningMessage.SetGlobalWarningEnabledIfNotSet(log, id, enabled, enabledSettingSource);
		}

		// sets the global override level for a warning id - note this is still overridden by log's local
		// overrides,
		public static void SetGlobalWarningAsErrorIfNotSet(Log log, WarningId id, bool asError, SettingSource asErrorSettingSource)
		{
			WarningMessage.SetGlobalWarningAsErrorIfNotSet(log, id, asError, asErrorSettingSource);
		}

		public static void SetGlobalDeprecationEnabledIfNotSet(Log log, DeprecationId id, bool enabled, SettingSource enabledSettingSource)
		{
			DeprecationMessage.SetGlobalDeprecationEnabledIfNotSet(log, id, enabled, enabledSettingSource);
		}

        // for testing only
        internal static void ResetGlobalWarningStates()
        {
            WarningMessage.ResetGlobalWarningStates();
        }

		// for testing only
		internal static void ResetGlobalDeprecationStates()
		{
			DeprecationMessage.ResetGlobalDeprecationStates();
		}
	}
}