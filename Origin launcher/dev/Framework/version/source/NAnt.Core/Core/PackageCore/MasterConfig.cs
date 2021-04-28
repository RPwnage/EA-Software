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
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;

namespace NAnt.Core.PackageCore
{
	public sealed partial class MasterConfig
	{
		public class ConfigExtension
		{
			public readonly string Name;
			public readonly string IfClause;

			public ConfigExtension(string name, string ifClause)
			{
				Name = name;
				IfClause = ifClause;
			}
		}

		public class ConfigInfo
		{
			public readonly IList<string> ActiveExcludes;
			public readonly IList<string> ActiveIncludes;
			public readonly List<ConfigExtension> Extensions;

			public readonly string ActiveDefault;
			public readonly string Package;

			// Accepted keys are "generic", "osx", and "unix"
			public readonly Dictionary<string, IList<string>> ExcludesByHost;
			public readonly Dictionary<string, IList<string>> IncludesByHost;
			public readonly Dictionary<string, string> DefaultByHost;

			internal static readonly ConfigInfo DefaultConfigInfo = new ConfigInfo();
			internal static readonly string[] SupportedConfigHosts = new string[] { "generic", "osx", "unix" };

			public ConfigInfo(string package = null,
				string defaultConfig = null, string includes = "", string excludes = "",
				List<ConfigExtension> extensions = null,
				string defaultConfigOsx = null, string includesOsx = "", string excludesOsx = "",
				string defaultConfigUnix = null, string includesUnix = "", string excludesUnix = ""
				)
			{
				if (!String.IsNullOrEmpty(excludes) && !String.IsNullOrEmpty(includes) ||
					!String.IsNullOrEmpty(excludesOsx) && !String.IsNullOrEmpty(includesOsx) ||
					!String.IsNullOrEmpty(excludesUnix) && !String.IsNullOrEmpty(includesUnix))
				{
					throw new BuildException("Error: <config> cannot have excludes and includes both in masterconfig file");
				}

				// If platform specific settings input is null, it means they are not defined.
				// Then we just take the input from "generic" as default.
				// We are counting on the fact that 
				DefaultByHost = new Dictionary<string, string>();
				DefaultByHost["generic"] = defaultConfig;
				DefaultByHost["osx"] = defaultConfigOsx;
				DefaultByHost["unix"] = defaultConfigUnix; ;

				IncludesByHost = new Dictionary<string, IList<string>>();
				IncludesByHost["generic"] = StringUtil.ToArray(includes);
				IncludesByHost["osx"] = StringUtil.ToArray(includesOsx);
				IncludesByHost["unix"] = StringUtil.ToArray(includesUnix);

				ExcludesByHost = new Dictionary<string, IList<string>>();
				ExcludesByHost["generic"] = StringUtil.ToArray(excludes);
				ExcludesByHost["osx"] = StringUtil.ToArray(excludesOsx);
				ExcludesByHost["unix"] = StringUtil.ToArray(excludesUnix);

				if (PlatformUtil.IsOSX)
				{
					ActiveDefault = string.IsNullOrEmpty(defaultConfigOsx) ? DefaultByHost["generic"] : DefaultByHost["osx"];
					ActiveIncludes = string.IsNullOrEmpty(includesOsx) ? IncludesByHost["generic"] : IncludesByHost["osx"];
					ActiveExcludes = string.IsNullOrEmpty(excludesOsx) ? ExcludesByHost["generic"] : ExcludesByHost["osx"];
				}
				else if (PlatformUtil.IsUnix)
				{
					ActiveDefault = string.IsNullOrEmpty(defaultConfigUnix) ? DefaultByHost["generic"] : DefaultByHost["unix"];
					ActiveIncludes = string.IsNullOrEmpty(includesUnix) ? IncludesByHost["generic"] : IncludesByHost["unix"];
					ActiveExcludes = string.IsNullOrEmpty(excludesUnix) ? ExcludesByHost["generic"] : ExcludesByHost["unix"];
				}
				else
				{
					ActiveDefault = DefaultByHost["generic"];
					ActiveIncludes = IncludesByHost["generic"];
					ActiveExcludes = ExcludesByHost["generic"];
				}

				Extensions = extensions ?? new List<ConfigExtension>();
				Package = package;
			}
		}

		public class BuildRootInfo
		{
			public string Path { get; set; }
			public bool AllowDefault { get; set; } = true;

			public readonly ExceptionCollection<string> Exceptions = new ExceptionCollection<string>();
		}

		public class SpecialRootInfo
		{
			public string Path { get; set; }
			public string IfClause { get; set; }
			// We couldn't evaluate the if clause during masterconfig loading because we don't
			// have the "project" object in the MasterConfigLoader.  So we evaluate it in later stage.
			public string EvaluatedIfClause { get; set; }
			// At present only "localondemandroot" uses this field to store 'use-as-default="true/false"' attribute setting.
			public Dictionary<string,string> ExtraAttributes { get; set; }

			internal SpecialRootInfo(string ifClause, string path, Dictionary<string, string> extraAttributes = null)
			{
				Path = path;
				IfClause = ifClause;
				EvaluatedIfClause = null;
				ExtraAttributes = extraAttributes;
			}
		}

		public class PackageRoot
		{
			public readonly string Path;

			public readonly int? MinLevel;
			public readonly int? MaxLevel;

			public readonly ExceptionCollection<string> Exceptions = new ExceptionCollection<string>();

			internal PackageRoot(string path, int? minlevel = null, int? maxlevel = null)
			{
				Path = path;
				MinLevel = minlevel;
				MaxLevel = maxlevel;
				if (minlevel < 0 || maxlevel < 0)
				{
					throw new BuildException("Error: <packageroot> element in masterconfig file cannot have negative 'minlevel' or 'maxlevel' attribute values.");
				}
				if (minlevel > maxlevel)
				{
					throw new BuildException("Error: <packageroot> element in masterconfig file cannot have 'minlevel' greater than 'maxlevel' attribute value.");
				}
			}

			public string EvaluatePackageRoot(Project project)
			{
				return Exceptions.ResolveException(project, Path);
			}
		}

		public class GlobalProperty
		{
			public GlobalProperty(string name, string value, string condition, bool useXmlElement = false, bool quoted = false)
			{
				Name = name;
				Value = value;
				Condition = condition;
				UseXmlElement = useXmlElement;
				Quoted = quoted;
			}

			// for merging, determine if the two items have matching keys so that the values can be reconciled
			public bool ConditionsMatch(GlobalProperty other)
			{
				return Name == other.Name
					&& Condition == other.Condition;
			}

			public readonly string Name;
			public string Value;
			public readonly string Condition;

			// Indicates that the loaded masterconfig loaded define this property using a multi-line xml block <globalproperty/>
			// and is used to determine if the xml block style should be used when writing
			public bool UseXmlElement;

			// Indicates if the non-xml property was quoted in the original masterconfig so that we know to write it as quoted if we save the masterconfig
			public bool Quoted;
		}

		public class GlobalDefine
		{
			public GlobalDefine(string name, string value, string condition, bool dotnet, string packages = "")
			{
				Name = name;
				Value = value;
				Condition = condition;
				DotNet = dotnet;
				Packages = packages;
			}

			public readonly string Name;
			public string Value;
			public readonly string Condition;
			public readonly bool DotNet;
			public readonly string Packages;

			// for merging, determine if the two items have matching keys so that the values can be reconciled
			public bool ConditionsMatch(GlobalDefine other)
			{
				return Name == other.Name
					&& Condition == other.Condition
					&& DotNet == other.DotNet
					&& Packages == other.Packages;
			}

			public override string ToString()
			{
				if (Value.IsNullOrEmpty())
				{
					return Name;
				}
				return Name + "=" + Value;
			}
		}

		public struct DeprecationDefinition
		{
			public bool Enabled { get; internal set; }
			public Location SettingLocation { get; internal set; }

			internal DeprecationDefinition(bool enabled, Location settingLocation)
			{
				Enabled = enabled;
				SettingLocation = settingLocation;
			}
		}

		public struct WarningDefinition
		{
			public bool? Enabled { get; internal set; }
			public bool? AsError { get; internal set; }
			public Location SettingLocation { get; internal set; }

			internal WarningDefinition(bool? enabled, bool? asError, Location settingLocation)
			{
				Enabled = enabled;
				AsError = asError;
				SettingLocation = settingLocation;
			}
		}

		public class FragmentDefinition
		{
			public string MasterConfigPath;
			public string LoadCondition = null;

			public readonly ExceptionCollection<string> Exceptions = new ExceptionCollection<string>();
		}

		public class NuGetPackage
		{
			public readonly string Name;
			public readonly string Version;

			public NuGetPackage(string name, string version)
			{
				Name = name;
				Version = version;
			}
		}

		public bool OnDemand { get { return m_onDemand == null ? false : (bool)m_onDemand; } private set { m_onDemand = value; } }
		public bool PackageServerOnDemand { get { return m_packageServerOnDemand == null ? true : (bool)m_packageServerOnDemand; } private set { m_packageServerOnDemand = value; } }
		public bool LocalOnDemand { get { return m_localOnDemand == null ? true : (bool)m_localOnDemand; } }

		public GroupType MasterGroup { get; private set; }

		public ConfigInfo Config { get; set; } = ConfigInfo.DefaultConfigInfo;

		public /* readonly */ List<FragmentDefinition> FragmentDefinitions = new List<FragmentDefinition>(); // TODO the only thing that stops this being a readonly member is merging code in test_nant that reassigns it

		public readonly string MasterconfigFile;

		public readonly BuildRootInfo BuildRoot = new BuildRootInfo();
		public readonly Dictionary<Log.WarningId, WarningDefinition> Warnings = new Dictionary<Log.WarningId, WarningDefinition>();
		public readonly Dictionary<Log.DeprecationId, DeprecationDefinition> Deprecations = new Dictionary<Log.DeprecationId, DeprecationDefinition>();
		public Dictionary<string, Package> Packages { get; private set; } = new Dictionary<string, Package>();
		public List<GlobalDefine> GlobalDefines { get; private set; } = new List<GlobalDefine>();
		public List<GlobalProperty> GlobalProperties { get; private set; } = new List<GlobalProperty>();
		public ReadOnlyCollection<string> CustomFragmentPaths { get; private set; }
		public Dictionary<string, NuGetPackage> NuGetPackages { get; private set; } = new Dictionary<string, NuGetPackage>(StringComparer.OrdinalIgnoreCase);
		public Dictionary<string, NuGetPackage> NetCoreNuGetPackages { get; private set; } = new Dictionary<string, NuGetPackage>(StringComparer.OrdinalIgnoreCase);
		public Dictionary<string, NuGetPackage> NetFrameworkNuGetPackages { get; private set; } = new Dictionary<string, NuGetPackage>(StringComparer.OrdinalIgnoreCase);
		public Dictionary<string, NuGetPackage> NetStandardNuGetPackages { get; private set; } = new Dictionary<string, NuGetPackage>(StringComparer.OrdinalIgnoreCase);
		public List<string> NuGetSources { get; private set; } = new List<string>();

		// used to be a List<GroupType> - maintaining a IEnumerable type for some backward compatbility
		// but making readonly since manipulating this naively is going to cause errors
		public ReadOnlyCollection<GroupType> PackageGroups => new ReadOnlyCollection<GroupType>(GroupMap.Values.ToList());

		public readonly List<MasterConfig> Fragments = new List<MasterConfig>();
		public readonly List<PackageRoot> PackageRoots = new List<PackageRoot>();
		public static readonly MasterConfig UndefinedMasterconfig = new MasterConfig();

		internal Dictionary<string, GroupType> GroupMap = new Dictionary<string, GroupType>(); // ideally this would be read only but merging currently relies on it being exposed

		internal delegate string GetPackagePathDelegate(Project project, string packageName);

		private bool? m_onDemand;
		private bool? m_packageServerOnDemand;
		private bool? m_localOnDemand;
		private List<SpecialRootInfo> m_developmentRoots;
		private List<SpecialRootInfo> m_localOnDemandRoots;
		private List<SpecialRootInfo> m_onDemandRoots;

		public MasterConfig(string file = "")
		{
			MasterconfigFile = file;

			if (!String.IsNullOrEmpty(file))
			{
				MasterconfigFile = Path.GetFullPath(file);
			}

			MasterGroup = new GroupType(GroupType.DefaultMasterVersionsGroupName);

			m_developmentRoots = new List<SpecialRootInfo>();
			m_localOnDemandRoots = new List<SpecialRootInfo>();
			m_onDemandRoots = new List<SpecialRootInfo>();
		}

		/// <summary>Returns the resolved ondemand root. Checks for the environment variable, evaluates conditions on the masterconfig elements and expands properties.</summary>
		/// <param name="project">If provided, nant properties in the root will be expanded.</param>
		public string GetOnDemandRoot(Project project = null)
		{
			return ResolveRoot("OnDemandRoot", m_onDemandRoots, project, "FRAMEWORK_ONDEMAND_ROOT")?.Path;
		}

		/// <summary>Returns the resolved development root. Checks for the environment variable, evaluates conditions on the masterconfig elements and expands properties.</summary>
		/// <param name="project">If provided, nant properties in the root will be expanded.</param>
		public string GetDevelopmentRoot(Project project = null)
		{
			return ResolveRoot("DevelopmentRoot", m_developmentRoots, project, "FRAMEWORK_DEVELOPMENT_ROOT")?.Path;
		}

		/// <summary>Returns the resolved local ondemand root. Evaluates conditions on the masterconfig elements and expands properties.
		/// This property does not have an environment variable because it should be different for each build environment.</summary>
		/// <param name="project">If provided, nant properties in the root will be expanded.</param>
		public SpecialRootInfo GetLocalOnDemandRoot(Project project = null)
		{
			return ResolveRoot("LocalOnDemandRoot", m_localOnDemandRoots, project);
		}

		// do a case insensitive lookup of the package name in the masterconfig
		public Package GetMasterPackageCaseInsensitively(Project project, string package)
		{
			IEnumerable<Package> matchingPackages = Packages.Where(kvp => kvp.Key.Equals(package, StringComparison.OrdinalIgnoreCase)).Select(kvp => kvp.Value);
			if (matchingPackages.Any())
			{
				if (matchingPackages.Count() > 1)
				{
					// if we have multiple case insensitive matches for a package name fall back to exact case match, if there is no exact
					// case match then fail
					Package masterPackage = matchingPackages.FirstOrDefault(mcp => mcp.Name.Equals(package, StringComparison.Ordinal));
					if (masterPackage == null)
					{
						throw new BuildException(String.Format("Cannot resolve package name '{0}' case insensitively - too many matches: {1}.", package, String.Join(", ", matchingPackages.Select(mp => mp.Name))));
					}
					return masterPackage;
				}
				return matchingPackages.First();
			}
			return null;
		}

		public static MasterConfig Load(Project project, string filePath, List<string> customFragments = null)
		{
			MasterConfigLoader loader = new MasterConfigLoader(project.Log);
			MasterConfig loadedMC = loader.Load(project, filePath);

			// We should do all the common initialization during load.
			loadedMC.MergeFragments(project, loader, customFragments);
			loadedMC.ExpandSpecialRootsProperties(project);

			return loadedMC;
		}

		internal void LoadMetaPackages(Project project, GetPackagePathDelegate getPath)
		{
			MasterConfig combinedMasterConfig = null;

			IEnumerable<KeyValuePair<string, Package>> metaPackages = Packages.Where(kv => kv.Value.IsMetaPackage);
			foreach (KeyValuePair<string, Package> metaPackage in metaPackages)
			{
				string packagePath = getPath(project, metaPackage.Key);
				if (packagePath == null)
				{
					continue; // we failed to find/download the metapackage and printed a warning, continue on to the next one
				}
				string masterConfigFilePath = Path.Combine(packagePath, "masterconfig.meta.xml");
				if (!File.Exists(masterConfigFilePath))
				{
					throw new BuildException("Cannot find masterconfig.meta.xml in meta-package '" + metaPackage.Key + "'");
				}

				MasterConfig newMasterConfig = Load(project, PathString.MakeNormalized(masterConfigFilePath).ToString());

				// apply some of the attributes to the packages in the metapackage
				List<string> grouptypeOverrideMessages = new List<string>();
				foreach (KeyValuePair<string, Package> package in newMasterConfig.Packages)
				{
					if (metaPackage.Value.GroupType != MasterGroup)
					{
						grouptypeOverrideMessages.Add(string.Format("'{0}' grouptype: '{1}' ==> '{2}'", package.Key, package.Value.GroupType.Name, metaPackage.Value.GroupType.Name));
						package.Value.GroupType = metaPackage.Value.GroupType;
					}

					package.Value.LocalOnDemand = package.Value.LocalOnDemand ?? metaPackage.Value.LocalOnDemand;
				}

				// print all the messages at the end so that they can be formatted more concisely
				if (grouptypeOverrideMessages.Any())
				{
					project.Log.Status.WriteLine("Meta-package fragment '{0}' changing masterconfig values:", masterConfigFilePath);
					foreach (string message in grouptypeOverrideMessages)
					{
						project.Log.Status.WriteLine(message);
					}
				}

				if (combinedMasterConfig != null)
				{
					MergeFragment(combinedMasterConfig, project, newMasterConfig, false);
				}
				else
				{
					combinedMasterConfig = newMasterConfig;
				}
			}

			// the original masterconfig trumps everything in the combined meta masterconfigs. Note if no metapackages could be resolved then combinedMasterCofnig will be null
			if (combinedMasterConfig != null)
			{
				MergeFragment(combinedMasterConfig, project, this);
				Packages = combinedMasterConfig.Packages;
				GroupMap = combinedMasterConfig.GroupMap;
				GlobalDefines = combinedMasterConfig.GlobalDefines;
				GlobalProperties = combinedMasterConfig.GlobalProperties;
			}
		}

		private void MergeFragments(Project project, MasterConfigLoader loader, List<string> customFragments = null)
		{
			CustomFragmentPaths = new ReadOnlyCollection<string>(customFragments ?? Enumerable.Empty<string>().ToList());

			if (String.IsNullOrEmpty(MasterconfigFile))
			{
				// The package doesn't have a masterconfig file.  No need to continue.
				// Also, the following GetDirectoryName function will crash if input 
				// is an empty string.
				return;
			}
			string baseDir = Path.GetDirectoryName(MasterconfigFile);

			List<FragmentDefinition> combinedFragmentsList;
			if (customFragments != null)
			{
				combinedFragmentsList = new List<FragmentDefinition>();
				combinedFragmentsList.AddRange(FragmentDefinitions);
				foreach (string fragment in customFragments)
				{
					combinedFragmentsList.Add(new FragmentDefinition { MasterConfigPath = fragment });
				}
			}
			else
			{
				combinedFragmentsList = FragmentDefinitions;
			}	

			foreach (FragmentDefinition fragmentDefinition in combinedFragmentsList)
			{
				bool loadThisFragment = true;
				if (!String.IsNullOrEmpty(fragmentDefinition.LoadCondition))
				{
					// Note that global properties in masterconfig has not been loaded at this point.  So any
					// properties listed here must be passed in from command line.
					// We need "project" to evaluate properties.  But if input has null project, we are
					// not using masterconfig under normal build workflow.  In which case, we'll just skip
					// loading this fragment.
					loadThisFragment = project != null && Expression.Evaluate(project.ExpandProperties(fragmentDefinition.LoadCondition));
				}

				if (loadThisFragment)
				{
					// evaluate exception to determine which fragment we are loading
					string path = fragmentDefinition.Exceptions.ResolveException(project, fragmentDefinition.MasterConfigPath);
					if (path != null) // default is to load nothing (no name specified)
					{
						var fragments = loader.LoadFragments(project, path, baseDir);
						foreach (MasterConfig fragment in fragments)
						{
							Fragments.Add(fragment);
							MergeFragment(this, project, fragment);
						}
					}
				}
			}
		}

		private static void MergeFragment(MasterConfig masterconfig, Project project, MasterConfig fragment, bool allowOverriding = true)
		{
			//--- merge packages ---
			List<string> packageVersionChangeMessages = new List<string>(); // Keep track of package change messages and print them all at the end
			foreach (KeyValuePair<string, Package> nameToPackage in fragment.Packages)
			{
				if (masterconfig.Packages.ContainsKey(nameToPackage.Key))
				{
					//IMTODO: Need to merge (name+exceptions) instead of evaluating exceptions right away
					if (project != null)
					{
						bool allowOverride = false;
						if (nameToPackage.Value.AdditionalAttributes.TryGetValue("allowoverride", out string allowOverrideStr))
						{
							if (allowOverrideStr.ToLowerInvariant() == "true")
							{
								allowOverride = true;
							}
						}

						if (!allowOverriding && allowOverride)
							throw new BuildException(String.Format("allowoverride is not allowed in '{0}' for '{1}'", fragment.MasterconfigFile, nameToPackage.Value.Name));

						// check if version is being overridden (this is not allowed unless "allowoverride" is set on the *overriding* version)
						IPackage oldPackage = masterconfig.Packages[nameToPackage.Key].Exceptions.ResolveException(project, masterconfig.Packages[nameToPackage.Key]);
						string oldVersion = oldPackage.Version;
						string newVersion = nameToPackage.Value.Exceptions.ResolveException(project, nameToPackage.Value).Version;
						if (newVersion != oldVersion)
						{
							if (allowOverride)
							{
								packageVersionChangeMessages.Add(string.Format("\t'{0}' version: '{1}' ==> '{2}'", nameToPackage.Value.Name, oldVersion, newVersion));
							}
							else
							{
								throw new BuildException
								(
									$"Masterconfig xml '{fragment.MasterconfigFile}' contains a definition of package '{nameToPackage.Value.Name}' with a different version '{newVersion}' than in '{oldPackage.XmlPath}' which uses version '{oldVersion}'. " +
									"If the file is expected to override the version number, please add attribute 'allowoverride=\"true\"' to the <package> element in the overriding file."
								);
							}
						}
						else
						{
							foreach (KeyValuePair<string, string> attribute in nameToPackage.Value.AdditionalAttributes)
							{
								if (attribute.Key == "allowoverride") continue;

								string newValue = attribute.Value;
								if (!masterconfig.Packages[nameToPackage.Key].AdditionalAttributes.TryGetValue(attribute.Key, out string oldValue) || oldValue != newValue)
								{
									if (allowOverride)
									{
										packageVersionChangeMessages.Add(string.Format("\t'{0}' {3}: '{1}' ==> '{2}'", nameToPackage.Value.Name, oldValue, newValue, attribute.Key));
									}
									else
									{
										project.Log.ThrowWarning(Log.WarningId.InconsistentMasterconfigAttribute, Log.WarnLevel.Minimal, "Fragment '{0}' does not override the version of package '{1}' but has a different {2} setting, if this is intentional please add attribute 'allowoverride=\"true\"'.", fragment.MasterconfigFile, nameToPackage.Value.Name, attribute.Key);
									}
								}
							}
						}

						masterconfig.Packages[nameToPackage.Key] = nameToPackage.Value;
					}
				}
				else
				{
					masterconfig.Packages.Add(nameToPackage.Key, nameToPackage.Value);
				}
			}

			// print all of the fragment override messages
			DoOnce.Execute(project, string.Format("fragment-override-warning.{0}.do-once", fragment.MasterconfigFile.GetHashCode()), () =>
			{
				if (packageVersionChangeMessages.Count() > 0)
				{
					project.Log.Status.WriteLine("Fragment '{0}' changing masterconfig values:", fragment.MasterconfigFile);
					foreach (string message in packageVersionChangeMessages)
					{
						project.Log.Status.WriteLine(message);
					}
				}
			});

			//--- merge groups ---
			foreach (KeyValuePair<string, GroupType> nameToGroup in fragment.GroupMap)
			{
				if (!masterconfig.GroupMap.TryGetValue(nameToGroup.Key, out GroupType existingGroup))
				{
					masterconfig.GroupMap[nameToGroup.Key] = nameToGroup.Value;
				}
				else
				{
					existingGroup.Merge(nameToGroup.Value);
				}
			}
			foreach (GroupType groupType in masterconfig.GroupMap.Values)
			{
				groupType.UpdateRefs(masterconfig.GroupMap);
			}

			//--- merge package roots ---
			foreach (var root in fragment.PackageRoots)
			{
				// Compute absolute package root path because relative paths would be recomputed against different masterconfig locations:
				string path = root.EvaluatePackageRoot(project);
				path = PathString.MakeCombinedAndNormalized(Path.GetFullPath(Path.GetDirectoryName(fragment.MasterconfigFile)), path).Path;
				var newRoot = new PackageRoot(path, root.MinLevel, root.MaxLevel);

				masterconfig.PackageRoots.Add(newRoot);
			}

			//IMTODO: Detect same property with different values?
			if (fragment.GlobalProperties != null)
			{
				if (masterconfig.GlobalProperties == null)
					masterconfig.GlobalProperties = new List<GlobalProperty>();

				masterconfig.GlobalProperties.AddRange(fragment.GlobalProperties);
			}

			if (fragment.GlobalDefines != null)
			{
				if (masterconfig.GlobalDefines == null)
					masterconfig.GlobalDefines = new List<GlobalDefine>();

				masterconfig.GlobalDefines.AddRange(fragment.GlobalDefines);
			}

			// Merge config extensions - fragements may list the same extension multiple times (with different conditions), we add all entries here
			// and will reduce to distinct once conditions are evaluated
			masterconfig.Config.Extensions.AddRange(fragment.Config.Extensions);

			// Merge warnings info
			foreach (KeyValuePair<Log.WarningId, WarningDefinition> warning in fragment.Warnings)
			{
				AddOrMergeWarning(project.Log, masterconfig.Warnings, warning.Key, warning.Value);
			}

			// Merge deprecations info
			foreach (KeyValuePair<Log.DeprecationId, DeprecationDefinition> deprecation in fragment.Deprecations)
			{
				AddOrMergeDeprecation(project.Log, masterconfig.Deprecations, deprecation.Key, deprecation.Value);
			}

			// Merge ondemand settings
			if (fragment.m_onDemand != null)
				masterconfig.OnDemand = fragment.OnDemand;
			if (fragment.m_packageServerOnDemand != null)
				masterconfig.PackageServerOnDemand = fragment.PackageServerOnDemand;

			// NUGET-TODO merge nuget packages and sources
		}

		private void ExpandSpecialRootsProperties(Project project)
		{
			foreach (List<SpecialRootInfo> rootsList in new List<SpecialRootInfo>[] { m_onDemandRoots, m_developmentRoots, m_localOnDemandRoots })
			{
				foreach (SpecialRootInfo odroot in rootsList)
				{
					if (odroot.IfClause != null)
					{
						// At present, we can only handle nant's system properties (we can't use config properties like config-system
						// as they are not initialized yet).
						string expression = project.ExpandProperties(odroot.IfClause);
						odroot.EvaluatedIfClause = Expression.Evaluate(expression) ? "true" : "false";
					}
					if (PlatformUtil.IsUnix || PlatformUtil.IsOSX)
					{
						// Nant currently couldn't handle path with '~'.  We have to convert it.
						if (odroot.Path.StartsWith("~"))
						{
							odroot.Path = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile) + odroot.Path.Substring(1);
						}
					}
				}

				// Do some error checking with root list. We should not have multiple "true" items.
				IEnumerable<SpecialRootInfo> trueConditionRoots = rootsList.Where(r => r.EvaluatedIfClause == "true");
				if (trueConditionRoots.Count() > 1)
				{
					StringBuilder msg = new StringBuilder();
					msg.AppendLine("ERROR: Masterconfig file cannot not have multiple root conditions being evaluated to true. The following conditions are being evaluated to true:");
					foreach (SpecialRootInfo info in trueConditionRoots)
					{
						msg.AppendLine(String.Format("    {0}", info.IfClause));
					}
					throw new BuildException(msg.ToString());
				}
			}
		}

		/// <summary>Resolves special roots, such as the development and ondemand roots, returning the value of the environment variable if set 
		/// or the appropriate root in the masterconfig after evaluating conditions.</summary>
		/// <param name="rootDescription">The name of the root, used for printing helpful log messages</param>
		/// <param name="rootInfoList">The list of roots in the masterconfig file</param>
		/// <param name="project">When provided, nant properties in the root will be expanded</param>
		/// <param name="environmentVariableOverride">The name of the environment variable that can be used to override the masterconfig file roots</param>
		private SpecialRootInfo ResolveRoot(string rootDescription, List<SpecialRootInfo> rootInfoList, Project project = null, string environmentVariableOverride = null)
		{
			// check for environment override first
			if (environmentVariableOverride != null)
			{
				string envOverrideValue = null;
				try
				{
					envOverrideValue = Environment.GetEnvironmentVariable(environmentVariableOverride);
				}
				catch { } // Exception can be throw if user doesn't have permission to perform that above operation. Silently continue.

				if (!String.IsNullOrEmpty(envOverrideValue))
				{
					if (project != null)
					{
						DoOnce.Execute
						(
							project, String.Format("{0}_USING_ENV_VAR_MSG", environmentVariableOverride), () =>
							{
								project.Log.Status.WriteLine("{0} path is being overridden by environment variable '{1}'. It is currently set to: {2}", rootDescription, environmentVariableOverride, envOverrideValue);
							},
							DoOnce.DoOnceContexts.global
						);
					}
					if (project != null && envOverrideValue != null)
					{
						envOverrideValue = project.ExpandProperties(envOverrideValue);
					}
					return new SpecialRootInfo(null, envOverrideValue);
				}
			}

			SpecialRootInfo defaultPath = null;
			SpecialRootInfo outPath = null;
			foreach (SpecialRootInfo info in rootInfoList)
			{
				if (info.IfClause == null)
				{
					if (defaultPath == null)
					{
						defaultPath = new SpecialRootInfo(null, info.Path, info.ExtraAttributes);
						continue;
					}
					else
					{
						// We shouldn't get here!  We should have thrown an error much earlier on.  
						// But we should still catch this error in case we made changes in future and something got missed.
						throw new BuildException(String.Format("NANT ERROR: masterconfig {0} has multiple default paths:\n  {1}\n  {2}", rootDescription.ToLowerInvariant(), defaultPath, info.Path));
					}
				}
				else if (info.EvaluatedIfClause == null)
				{
					// We shouldn't get here!  But we should still catch this error in case we made changes in future and something got missed.
					throw new BuildException(String.Format("NANT ERROR: masterconfig {0} 'if' clause has not been evaluated: {1}", rootDescription.ToLowerInvariant(), info.IfClause));
				}
				else if (info.EvaluatedIfClause == "true")
				{
					outPath = new SpecialRootInfo(info.EvaluatedIfClause, info.Path, info.ExtraAttributes);
					break;
				}
			}
			SpecialRootInfo specialRoot = (outPath == null ? defaultPath : outPath);
			if (project != null && specialRoot != null)
			{
				specialRoot.Path = project.ExpandProperties(specialRoot.Path);
			}
			return specialRoot;
		}

		private static void AddOrMergeWarning(Log log, Dictionary<Log.WarningId, WarningDefinition> warningDict, Log.WarningId id, WarningDefinition warningDefinition)
		{
			if (warningDict.TryGetValue(id, out WarningDefinition existing))
			{
				if (warningDefinition.Enabled != null)
				{
					if (existing.Enabled != warningDefinition.Enabled)
					{
						log.Status.WriteLine("Warning enabled setting from {0} overridden by setting at {1}.", existing.SettingLocation, warningDefinition.SettingLocation);
					}
				}
				else
				{
					warningDefinition.Enabled = existing.Enabled;
				}
				if (warningDefinition.AsError != null)
				{
					if (existing.AsError != warningDefinition.AsError)
					{
						log.Status.WriteLine("Warning as-error setting from {0} overridden by setting at {1}.", existing.SettingLocation, warningDefinition.SettingLocation);
					}
				}
				else
				{
					warningDefinition.AsError = existing.AsError;
				}
			}
			warningDict[id] = warningDefinition;
		}

		private static void AddOrMergeDeprecation(Log log, Dictionary<Log.DeprecationId, DeprecationDefinition> deprecationDict, Log.DeprecationId id, DeprecationDefinition deprecationDefinition)
		{
			if (deprecationDict.TryGetValue(id, out DeprecationDefinition existing))
			{
				if (existing.Enabled != deprecationDefinition.Enabled)
				{
					log.Status.WriteLine("Deprecation enabled setting from {0} overridden by setting at {1}.", existing.SettingLocation, deprecationDefinition.SettingLocation);
				}
			}
			deprecationDict[id] = deprecationDefinition;
		}
	}
}