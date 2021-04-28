using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

namespace NAnt.Core.PackageCore
{
	public static class ConfigPackageLoader
	{
		private const string CONFIGURATION_DEFINITION_OPTIONSET_NAME = "configuration-definition";

		// cache the config file names in each config (extension) package so we don't have to rescan // TODO - just cache the config names!
		private static readonly ConcurrentDictionary<string, string[]> s_configFilesCache = new ConcurrentDictionary<string, string[]>();

		// initializes the configuration up to the point of having set the configuration properties
		// (config, config-name, config-system) but does not include any of the wiring for the 
		// config, that is done in InitConfiguration
		public static void InitializeInitialConfigurationProperties(Project project)
		{
			if (!PackageMap.IsInitialized)
			{
				// if package map isn't setup yet then this is probably the project used to 
				// init the package map - package mapp init will run this explicitly for this
				// project
				return; 
			}
			PackageMap.Instance.InitConfigPackage(project); // lazy init, right before we need config package

			Location configPropertyInitLocation = Location.UnknownLocation; // DAVE-FUTURE-REFACTOR-TODO - this is now run very early in project init which doesn't give us a good reporting location, maybe we could just use .build script root location?
			string config = project.Properties[PackageProperties.ConfigNameProperty];

			// make sure configuration package is installed (this should have already been done by package map init)
			Release mainConfigRelease = PackageMap.Instance.FindOrInstallCurrentRelease(project, PackageMap.Instance.ConfigPackageName);

			// make sure all active config extensions are installed
			List<string> configExtensionNames = PackageMap.Instance.GetConfigExtensions(project);
			Release[] configExtensions = configExtensionNames
				.Select(extensionName => PackageMap.Instance.FindOrInstallCurrentRelease(project, extensionName))
				.ToArray();
			project.Properties["nant.active-config-extensions"] = $"{PackageMap.Instance.ConfigPackageName} {String.Join(" ", configExtensionNames.ToArray())}";

			// set package.configs property, do this before validating config as the config packages may want to query this
			{
				// create a list of possible directories config .xml files can exists
				List<string> configurationDirs = new List<string>();
				{
					// main config package, config dir
					string mainConfigDirectory = PackageMap.Instance.ConfigDir ??
						throw new Exception("Unable to determine the path to the configuration package.");
					configurationDirs.Add(mainConfigDirectory);

					// config extension config dirs
					foreach (Release extension in configExtensions)
					{
						string extensionConfigDirectory = Path.Combine(extension.Path, "config");
						configurationDirs.Add(extensionConfigDirectory);
					}
				}

				// scan directories for a list of possible configs
				HashSet<string> allConfigs = new HashSet<string>();
				foreach (string configDir in configurationDirs)
				{
					if (!Directory.Exists(configDir))
					{
						continue;
					}

					foreach (string fileName in s_configFilesCache.GetOrAdd(configDir, cdp => Directory.GetFiles(cdp)))
					{
						if (Path.GetExtension(fileName).ToLower() != PackageProperties.ConfigFileExtension)
						{
							continue;
						}
						string foundConfig = Path.GetFileNameWithoutExtension(fileName);
						allConfigs.Add(foundConfig);
					}
				}

				// set property recording all possible configs
				project.Properties.UpdateReadOnly(PackageProperties.PackageAllConfigsPropertyName, String.Join(" ", allConfigs.OrderBy(s => s)));

				// setup package.configs property - if property is already set the just use
				// existing value (but set it readonly from here on out)
				string activeConfigs = project.Properties[PackageProperties.PackageConfigsPropertyName];
				if (activeConfigs is null)
				{
					// filter all configs by exclude or includes if provided by masterconfig
					// note: no error checking for both includes and excludes here, masterconfig loading errors
					// if both are set so we can assume only one is > 0
					IList<string> includes = PackageMap.Instance.ConfigIncludes;
					IList<string> excludes = PackageMap.Instance.ConfigExcludes;
					HashSet<string> defaultActiveConfigs = null;
					if (excludes.Count > 0)
					{
						defaultActiveConfigs = allConfigs;  // note: no copy here, modifying reference in place
						foreach (string excludeString in excludes)
						{
							int wildCardIndex = excludeString.IndexOf('*');
							if (wildCardIndex != -1 && wildCardIndex != excludeString.Length - 1)
							{
								// legacy code allowed this even though it is misleading to user, so for compatbility make this a warning only
								project.Log.ThrowWarning(Log.WarningId.InvalidWildCardConfigPattern, Log.WarnLevel.Normal,
									"Exclude string in masterconfig contains invalid wild card '*' placement ({0}). Wild cards can only appear at the end of the string. Anything after the wild card will be ignored.",
									excludeString
								);
							}
							defaultActiveConfigs.RemoveWhere
							(
								c => wildCardIndex != -1 ?
									c.StartsWith(excludeString.Substring(0, wildCardIndex)) :
									c == excludeString
							);
						}
					}
					else if (includes.Count > 0)
					{
						defaultActiveConfigs = new HashSet<string>();
						foreach (string includeString in includes)
						{
							int wildCardIndex = includeString.IndexOf('*');
							if (wildCardIndex != -1 && wildCardIndex != includeString.Length - 1)
							{
								// legacy code allowed this even though it is misleading to user, so for compatbility make this a warning only
								project.Log.ThrowWarning(Log.WarningId.InvalidWildCardConfigPattern, Log.WarnLevel.Quiet,
									"Include string in masterconfig contains invalid wild card '*' placement ({0}). Wild cards can only appear at the end of the string. Anything after the wild card will be ignored.",
									includeString
								);
							}
							defaultActiveConfigs.UnionWith
							(
								allConfigs.Where
								(
									c => wildCardIndex != -1 ?
										c.StartsWith(includeString.Substring(0, includeString.Length - 1)) :
										c == includeString
								)
							);
						}
					}
					else
					{
						defaultActiveConfigs = allConfigs;
					}

					List<string> orderedActiveConfigs = defaultActiveConfigs.OrderBy(s => s).ToList();

					// add the active config at the start of the list if not already in the list
					if (!String.IsNullOrEmpty(config) && !defaultActiveConfigs.Contains(config))
					{
						project.Log.Warning.WriteLine("List of all included configurations does not contain active configuration defined by property {0}='{1}'. Adding active configuration to the list of included configurations.", PackageProperties.PackageConfigPropertyName, config);
						orderedActiveConfigs.Insert(0, config);
					}

					activeConfigs = String.Join(" ", orderedActiveConfigs);
				}
				project.Properties.UpdateReadOnly(PackageProperties.PackageConfigsPropertyName, String.Join(" ", activeConfigs));
			}

			// init configuration properties from init.xml from each configuration package and extensipn - these get to jump in ahead of everything else
			{
				// load main configuration package init.xml - but don't fully init, we don't want to load initialize.xml just yet
				string topLevelGlobalInit = Path.Combine(PackageMap.Instance.ConfigDir, "global", "init.xml");
				if (LineInfoDocument.IsInCache(topLevelGlobalInit) || File.Exists(topLevelGlobalInit))
				{
					project.IncludeBuildFileDocument(topLevelGlobalInit, configPropertyInitLocation);
				}

				// load init.xml from all configuration packages
				foreach (Release extension in configExtensions)
				{		
					string initFile = Path.Combine(extension.Path, "config", "global", "init.xml");
					if (LineInfoDocument.IsInCache(initFile) || File.Exists(initFile))
					{
						project.IncludeBuildFileDocument(initFile, configPropertyInitLocation);
					}
				}
			}

			// handle config property - don't error if is unset for backwards compatibility for pure nant scripts that don't run <package> task ever
			// also init all config derived properites, config-name, config-system, etc
			{
				// if config property not explicitly set by this point then fall back to masterconfig defaults
				if (config == null)
				{
					// fall back to masterconfig default config
					config = PackageMap.Instance.DefaultConfig;

					// remap the default config if necessary - this should only happen for top level packages, dependencies
					// will inherit already remapped config string
					if (config != null && Project.SanitizeConfigString(config, out string remappedConfig))
					{
						project.Log.Warning.WriteLine($"Automatically remapping masterconfig default config string from '{config}' to '{remappedConfig}'. Please update your masterconfig to use the new config names.");
						config = remappedConfig;
					}
				}

				// allow config to be null at this stage - fail later if second half of config init is run
				if (config != null)
				{
					project.Properties.AddReadOnly(PackageProperties.ConfigNameProperty, config);

					// see if any config extension contain this config, we will check main config later but we always run this search
					// so we can warn about any conflicts
					string firstConfigExtensionFile = null;
					List<string> extensionsWithThisConfig = null;
					foreach (Release extension in configExtensions)
					{
						string extensionConfigFile = Path.Combine(extension.Path, "config", config + ".xml");
						if (LineInfoDocument.IsInCache(extensionConfigFile) || File.Exists(extensionConfigFile))
						{
							extensionsWithThisConfig = extensionsWithThisConfig ?? new List<string>();
							extensionsWithThisConfig.Add(extension.Name);
							firstConfigExtensionFile = firstConfigExtensionFile ?? extensionConfigFile;
						}
					}

					// load from central config package if config is available, if not load from first config extension to contain this config
					// throw warnings if config is duplicated between main config package or config extensions
					string configFile;
					string configSourcePackage;
					{
						string configPackageConfigFile = Path.Combine(PackageMap.Instance.ConfigDir, config + ".xml");
						bool configFileInConfigPackage = LineInfoDocument.IsInCache(configPackageConfigFile) || File.Exists(configPackageConfigFile);
						if (configFileInConfigPackage)
						{
							// config comes from main config package
							configSourcePackage = PackageMap.Instance.ConfigPackageName;
							configFile = configPackageConfigFile;

							if (extensionsWithThisConfig != null)
							{
								project.Log.ThrowWarning
								(
									Log.WarningId.DuplicateConfigInExtension, Log.WarnLevel.Normal, new string[] { $"{config}-extension-duplicate" },
									"Root configuration package {0} contains a definition for {1}, duplicate config from extensions ({2}) will be ignored.",
									PackageMap.Instance.ConfigPackageName, config, String.Join(", ", extensionsWithThisConfig)
								);
							}
						}
						else if (extensionsWithThisConfig != null)
						{
							// config comes from extension
							configSourcePackage = extensionsWithThisConfig.First();
							configFile = firstConfigExtensionFile;

							// mark the extension as the "platform config package" this is *only* done for extension for backwards compatiblity 
							// with how msbuild options file are loaded where overriding the msbuild options file with a different file
							// is relative the platform config package - which in modern terms is a config extension
							// TODO: this could be cleaned up
							project.Properties.UpdateReadOnly("nant.platform-config-package-name", configSourcePackage); 

							if (extensionsWithThisConfig.Count > 1)
							{
								project.Log.ThrowWarning
								(
									Log.WarningId.DuplicateConfigInExtension, Log.WarnLevel.Normal, new string[] { $"{config}-extension-duplicate" },
									"Configuration extension {0} contains a definition for {1}, duplicate config from extensions ({2}) will be ignored.",
									configSourcePackage, config, String.Join(", ", extensionsWithThisConfig.Skip(1))
								);
							}
						}
						else
						{
							throw new BuildException($"Could not find the file '{config}.xml' in any of the config packages listed in the masterconfig file's <config> block.", location: configPropertyInitLocation);
						}
					}

					// record which package was the source of the config file
					project.Properties.UpdateReadOnly("nant.config-source-package", configSourcePackage);

					// load configuration initial property file
					ProcessConfigFile(project, configPropertyInitLocation, configFile, configSourcePackage);
				}
			}

			// now config properties are processed - recompute global properties in case any are based on configuration
			project.RecomputeGlobalPropertyExceptions();
		}

		internal static void InitializePrecombineConfiguration(Project project, Location location)
		{
			Release mainConfigRelease = PackageMap.Instance.FindOrInstallCurrentRelease(project, PackageMap.Instance.ConfigPackageName);
			Release[] configExtensions = PackageMap.Instance.GetConfigExtensions(project)
				.Select(extensionName => PackageMap.Instance.FindOrInstallCurrentRelease(project, extensionName))
				.ToArray();

			// config packages are allowed to rewrite templates for things like
			// package.builddir in init.xml so recompute these now we're loading real config
			PackageInitializer.AddPackageProperties(project, mainConfigRelease);
			foreach (Release extension in configExtensions)
			{
				PackageInitializer.AddPackageProperties(project, extension);
			}

			// Initialize each package fully, loading its initialize.xml now the package properties are finished being setup
			PackageInitializer.IncludePackageInitializeXml(project, mainConfigRelease, location);
			foreach (Release extension in configExtensions)
			{
				PackageInitializer.IncludePackageInitializeXml(project, extension, location);
			}

			string configSourcePackage = project.GetPropertyOrFail("nant.config-source-package");

			// config-common needs to be loaded after the config- properties have been set
			// it is executed for both the optionset and deprecated manual formats
			IncludeExtensionScript(project, location, Path.Combine("platform", "config-common.xml"), configSourcePackage);

			// try to include the appropriate platform file from eaconfig, platform config packages include this automatically in their initialize.xml
			// although now these steps are disabled when the new config system property is used.
			// NOTE: for backwards compatibility config-platform may not include processor, but we always load platform file using processor so this is comprised of platform+processor+compiler
			string platform = $"{project.Properties.GetPropertyOrFail("config-system")}-{project.Properties.GetPropertyOrFail("config-processor")}-{project.Properties.GetPropertyOrFail("config-compiler")}";
			IncludeExtensionScript(project, location, Path.Combine("platform", platform + ".xml"), configSourcePackage, optional: false);

			string configSet = project.Properties.GetPropertyOrFail("config-set");
			IncludeExtensionScript(project, location, Path.Combine("configset", configSet + ".xml"), configSourcePackage, optional: false);

			// Any steps that need to happen after loading the config file but before running the combine step
			IncludeExtensionScript(project, location, Path.Combine("global", "pre-combine.xml"), configSourcePackage);

			// Execute Game team specific config setting override file if it exists before converting config optionsets to real option list.
			// If game teams need to make custom override changes to build optionsets like config-options-common for 
			// any configs(including capilano, kettle, etc), they can provide this file and write their own overrides
			// in this file instead of making a divergence to the config packages.
			LoadExtraConfigOverrides(project, location);
		}

		internal static void InitializePostCombineConfiguration(Project project, Location location)
		{
			string configSourcePackage = project.GetPropertyOrFail("nant.config-source-package");

			// Any steps that need to happen after running the combine step
			IncludeExtensionScript(project, location, Path.Combine("global", "post-combine.xml"), configSourcePackage);
			IncludeExtensionScript(project, location, Path.Combine("targets", "targets.xml"), configSourcePackage);

			// Developer config validation
			if (project.Log.DeveloperWarningEnabled)
			{
				if (!project.Properties.GetBooleanPropertyOrDefault("eaconfig.used-build-tool-tasks", false, failOnNonBoolValue: true))
				{
					string configSystem = project.Properties.GetPropertyOrFail("config-system");
					DoOnce.Execute($"eaconfig.{configSystem}.used-build-tool-tasks", () => project.Log.DeveloperWarning.WriteLine($"Config system '{configSystem}' isn't set up with build tool tasks."));
				}
			}
		}

		private static void LoadExtraConfigOverrides(Project project, Location location)
		{
			string optionsOverrideFileOriginal = project.GetPropertyOrDefault("eaconfig.options-override.file", null);
			if (!optionsOverrideFileOriginal.IsNullOrEmpty())
			{
				project.Log.ThrowDeprecation
				(
					Log.DeprecationId.EaConfigOptionsOverrideFile, Log.DeprecateLevel.Normal, new string[] { "eaconfig.options-override.file" },
					"You are using the eaconfig.options-override.file property to extend the config packages. This field is deprecated. " +
					"We have redesigned package loading so that you can easily put these files in their own config package which will extend your other config packages. " +
					"Adding the name of your config package in the masterconfig as an <extension> element within the <config> block. " +
					"See the Framework documentation for more details."
				);

				string optionsOverrideFileExpanded = project.ExpandProperties(optionsOverrideFileOriginal);

				// relative path is combined with masterconfig file directory
				string masterconfigDir = Path.GetDirectoryName(PackageMap.Instance.MasterConfigFilePath);
				optionsOverrideFileExpanded = Path.GetFullPath(Path.Combine(masterconfigDir, optionsOverrideFileExpanded));

				string optionsOverridePath = Path.GetDirectoryName(optionsOverrideFileExpanded);
				string optionsOverrideFile = Path.GetFileNameWithoutExtension(optionsOverrideFileExpanded);
				string optionsOverrideExt = Path.GetExtension(optionsOverrideFileExpanded);

				string platformOverrides = Path.Combine(optionsOverridePath, optionsOverrideFile + "." + project.Properties["config-platform"] + optionsOverrideExt);
				string systemOverrides = Path.Combine(optionsOverridePath, optionsOverrideFile + "." + project.Properties["config-system"] + optionsOverrideExt);
				string defaultOverrides = Path.Combine(optionsOverridePath, optionsOverrideFile + optionsOverrideExt);

				if (LineInfoDocument.IsInCache(platformOverrides) || File.Exists(platformOverrides))
				{
					project.IncludeBuildFileDocument(platformOverrides, location);
				}
				else if (LineInfoDocument.IsInCache(systemOverrides) || File.Exists(systemOverrides))
				{
					project.IncludeBuildFileDocument(systemOverrides, location);
				}
				else if (LineInfoDocument.IsInCache(defaultOverrides) || File.Exists(defaultOverrides))
				{
					project.IncludeBuildFileDocument(defaultOverrides, location);
				}
				else
				{
					project.Log.Warning.WriteLine("Property eaconfig.options-override.file='${0}' but none of the files: '${1}' or '${2}' or '${3}' exists.", optionsOverrideFileOriginal, platformOverrides, systemOverrides, defaultOverrides);
				}
			}
		}

		private static void IncludeExtensionScript(Project project, Location location, string name, string configSourcePackage, bool optional = true)
		{
			bool found = false;
			string topLevelScript = Path.Combine(PackageMap.Instance.ConfigDir, name);
			if (LineInfoDocument.IsInCache(topLevelScript) || File.Exists(topLevelScript))
			{
				project.IncludeBuildFileDocument(topLevelScript, location);
				found = true;
			}

			foreach (string extension in PackageMap.Instance.GetConfigExtensions(project))
			{
				Release extensionRelease = PackageMap.Instance.FindOrInstallCurrentRelease(project, extension);
				string extensionScript = Path.Combine(extensionRelease.Path, "config", name);
				if (LineInfoDocument.IsInCache(extensionScript) || File.Exists(extensionScript))
				{
					project.IncludeBuildFileDocument(extensionScript, location);
					found = true;
				}
			}

			if (!optional && !found)
			{
				throw new BuildException
				(
					"Could not find platform config script '" + name + "' in main configuration package or any extension. It should likely be found inside the '" + configSourcePackage + "' package.",
					location
				);
			}
		}

		private static void ValidateConfigOptionWasSet(Project project, Location location, string optionName, string configFile, string configSourcePackage)
		{
			if (!project.Properties.Contains(optionName))
			{
				throw new BuildException($"Configuration file '{Path.GetFileName(configFile)}' in package '{configSourcePackage}' does not define required option '{optionName}' in its '{CONFIGURATION_DEFINITION_OPTIONSET_NAME}'.", location);
			}
		}

		private static void ProcessConfigFile(Project project, Location location, string configFile, string configSourcePackage)
		{
			project.IncludeBuildFileDocument(configFile, location);

			OptionSet platformDef = project.NamedOptionSets["configuration-definition"] ??
				throw new BuildException($"Configuration file '{Path.GetFileName(configFile)}' in package '{configSourcePackage}' does not define required optionset '{CONFIGURATION_DEFINITION_OPTIONSET_NAME}'.");

			Dictionary<string, bool> configsetopt = new Dictionary<string, bool>()
			{
				{ "debug", false },
				{ "optimization", false },
				{ "profile", false },
				{ "retail", false },
				{ "dll", false }
			};
			string explicitConfigSet = null;

			foreach (KeyValuePair<string, string> op in platformDef.Options)
			{
				string name = op.Key as string;
				string val = op.Value as string;

				switch (name)
				{
					case "config-name":
					case "config-system":
					case "config-compiler":
					case "config-platform":
					case "config-processor":
					case "config-os":
					case "config-isposix":
						{
							project.Properties[name] = val;
						}
						break;
					case "debug":
					case "optimization":
					case "profile":
					case "retail":
					case "dll":
						{
							if (val == "on") configsetopt[name] = true;
						}
						break;
					case "configset":
						explicitConfigSet = val;
						break;
					default:
						{
							project.Log.Warning.WriteLine($"{location}Configuration set '{CONFIGURATION_DEFINITION_OPTIONSET_NAME}' in file '{Path.GetFileName(configFile)}' contains unknown option '{name}={val}'.");
						}
						break;
				}
			}

			ValidateConfigOptionWasSet(project, location, "config-name", configFile, configSourcePackage);
			ValidateConfigOptionWasSet(project, location, "config-system", configFile, configSourcePackage);
			ValidateConfigOptionWasSet(project, location, "config-compiler", configFile, configSourcePackage);
			ValidateConfigOptionWasSet(project, location, "config-platform", configFile, configSourcePackage);
			ValidateConfigOptionWasSet(project, location, "config-processor", configFile, configSourcePackage);
			ValidateConfigOptionWasSet(project, location, "config-os", configFile, configSourcePackage);

			// set a default value for the dll property, we need to check because it could be set in the config file
			if (!project.Properties.Contains("config-isDll"))
			{
				project.Properties["config-isDll"] = configsetopt["dll"] ? "true" : "false";
			}

			// set a default value for the config-isposix property if it wasn't set (likely on non posix platforms)
			if (!project.Properties.Contains("config-isposix"))
			{
				project.Properties["config-isposix"] = "false";
			}

			if (explicitConfigSet != null)
			{
				project.Properties["config-set"] = explicitConfigSet;
				return;
			}

			// This constructed configset name needs to match up with what is currently in 
			// eaconfig's config\configset\*.xml because it is used to include one of those files.
			StringBuilder configSetString = new StringBuilder();
			if (configsetopt["dll"])
			{
				configSetString.Append("dll-");
			}
			if (configsetopt["debug"])
			{
				configSetString.Append("debug-");
			}
			if (configsetopt["optimization"])
			{
				configSetString.Append("opt-");
			}
			if (configsetopt["profile"])
			{
				configSetString.Append("profile-");
			}
			if (configsetopt["retail"])
			{
				configSetString.Append("retail-");
			}
			string configSet = configSetString.ToString().TrimEnd('-');

			if (configSet == "dll")
			{
				throw new BuildException($"None of the configuration options 'debug, optimization, profile, retail' is defined in '{Path.GetFileName(configFile)}'.");
			}

			project.Properties["config-set"] = configSet;
		}
	}
}
