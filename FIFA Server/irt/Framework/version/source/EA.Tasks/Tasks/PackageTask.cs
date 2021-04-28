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
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Metrics;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

using EA.Eaconfig;
using EA.Eaconfig.Build;
using EA.FrameworkTasks.Functions;
using EA.FrameworkTasks.Model;

using NAnt.Core.PackageCore;

namespace EA.FrameworkTasks
{
	/// <summary>Declares a package and loads configuration. Also automatically loads the package's Initialize.xml.</summary>
	/// <remarks>
	/// <para>
	/// This task should be called only once per package build file.
	/// </para>
	/// <para>
	/// The task declares the following properties:
	/// </para>
	/// <list type='table'>
	/// <listheader><term>Property</term><description>Description</description></listheader>
	/// <item><term>${package.name}</term><description>The name of the package.</description></item>
	/// <item><term>${package.targetversion}</term><description>The version number of the package, determined from the <b>targetversion</b> attribute</description></item>
	/// <item><term>${package.version}</term><description>The version number of the package, determined from the <b>path</b> to the package</description></item>
	/// <item><term>${package.${package.name}.version}</term><description>Same as <b>${package.version}</b> but the property name includes the package name.</description></item>
	/// <item><term>${package.config}</term><description>The configuration to build.</description></item>
	/// <item><term>${package.configs}</term><description>For a Framework 1 package, it's a space delimited list 
	/// of all the configs found in the config folder.  For a Framework 2 package, this property excludes any configs
	/// specified by &lt;config excludes> in masterconfig.xml</description></item>
	/// <item><term>${package.dir}</term><description>The directory of the package build file (depends on packageroot(s) but path should end in <b>/${package.name}/${package.version}</b>.</description></item>
	/// <item><term>${package.${package.name}.dir}</term><description>The directory of the package build file (depends on packageroot(s) but path should end in <b>/${package.name}/${package.version}</b>.</description></item>
	/// <item><term>${package.builddir}</term><description>The current package build directory (depends on <b>buildroot</b> but path should end in <b>/${package.name}/${package.version}</b>.</description></item>
	/// <item><term>${package.${package.name}.builddir}</term><description>Same as <b>${package.builddir}</b> but the property name includes the package name.</description></item>
	/// <item><term>${package.${package.name}.buildroot}</term><description>The directory for building binaries (can be set in masterconfig.xml via <b>buildroot</b> and <b>grouptype</b>) .</description></item>
	/// <item><term>${package.${package.name}.parent}</term><description>Name of this package's parent package (applies only when this package is set to autobuildclean).</description></item>
	/// </list>
	/// <para>
	/// Configuration properties are not loaded until this task has been executed in your .build file.
	/// </para>
	/// </remarks>
	/// <include file='Examples/Package/Package.example' path='example'/>
	[TaskName("package")]
	public class PackageTask : Task
	{
		const string _configDirectoryName = "config";

		/// <summary>Obsolete. Package name is now derived from the name of the .build file.</summary>
		[TaskAttribute("name", Required = false)]
		public string PackageName { get; set; }

		/// <summary>The version of the package in development. This parameter is optional.
		/// If it is not specified, Framework will use the actual version.</summary>
		[TaskAttribute("targetversion", Required = false)] // TODO - can we get rid of this? might still be used in packaging targets
		public string TargetVersion { get; set; }

		/// <summary>Obsolete. Initialize</summary>
		[TaskAttribute("initializeself", Required = false)]
		public bool? InitializeSelf { get; set; }

		public PackageTask(Project project)
			: base()
		{
			Project = project;
		}

		public PackageTask() : base() { }

		protected override void ExecuteTask()
		{
			// immediately check config property is set when executing package task, it should have been set
			// by ConfigPackageLoader.InitConfigurationProperties at project init
			string configName = Project.Properties["config"];
			if (configName == null)
			{
				throw new Exception("No configuration was specified on command line or as default in masterconfig.");
			}

			if (Project.BaseDirectory == null)
			{
				throw new BuildException($"{LogPrefix}Project base directory not set. Cannot initalized package without a directory.", location: Location);
			}

			if (PackageMap.Instance.MasterConfig == MasterConfig.UndefinedMasterconfig)
			{
				throw new Exception($"{LogPrefix}Cannot invoke {Name} task if masterconfig file is not defined.");
			}

			// check for multiple packages
			if (Project.Properties.Contains(PackageProperties.PackageNamePropertyName) &&
				Project.Properties.Contains(PackageProperties.PackageTargetVersionPropertyName))
			{
				throw new BuildException(String.Format("Duplicate package {0}-{1} detected.  The <package> task can only be executed once per package build.", PackageName, TargetVersion));
			}

			// determine the release this <package> corresponds to based on packagemap
			Release packageMapRelease;
			bool executingDoesNotMatchPackageMap = false;
			string executingDoesNotMatchPackageMapMsg = null;
			{
				// get the release based on the location this task is executing
				Release executingRelease = PackageMap.CreateReleaseFromBuildFile(Project, Project.CurrentScriptFile);
				if (PackageName != null)
				{
					PackageName = PackageName.Trim();
					if (!executingRelease.Name.Equals(PackageName, StringComparison.OrdinalIgnoreCase))
					{
						throw new BuildException($"The 'name' parameter of the <package> task ('{PackageName}') does not match the file where it is executing '{Project.CurrentScriptFile}'. " +
							$"In Framework 8.1 the <package> task must executed in the a .build file named after the package (e.g MyPackage.build). The 'name' parameter of the package task must match this name " +
							$"or simply be omitted.");
					}
				}

				// due to Framework's architecture we lose any context about the package we located via masterconfig when resolving dependencies
				// so we have to look at where we are executing the package task and see if it matches what we expected from masterconfig - note
				// we need to support this path for now because we can arrive here via -buildfile on command line or buildfile="" in <nant> task
				// which can just drop us into a package context outside of PackageMap's control
				// executing a packagetask in a location packagemap doesn't think that package should exist is an error
				bool isInMasterConfig = PackageMap.Instance.TryGetMasterPackage(Project, executingRelease.Name, out MasterConfig.IPackage packageSpec);
				if (isInMasterConfig)
				{
					packageMapRelease = PackageMap.Instance.FindInstalledRelease(packageSpec);
					if (packageMapRelease == null)
					{
						// we couldn't find the package - yet we're running it's package task
						throw new BuildException("Invalid <package> task. Cannot run outside of package roots specified in masterconfig.");
					}
				}
				else
				{
					// there is ONE valid case where we can allow <package> to run if there is no masterconfig entry - if this is the package
					// user provided via -buildfile or started in the working dir of the package, force use top level version
					packageMapRelease = PackageMap.Instance.TopLevelRelease;
					if (packageMapRelease == null || !executingRelease.Name.Equals(packageMapRelease.Name, StringComparison.OrdinalIgnoreCase))
					{
						throw new BuildException("Invalid <package> task. Cannot run <package> task for a package not in masterconfig.");
					}
				}

				string executingPath = PathNormalizer.Normalize(executingRelease.Path.EnsureTrailingSlash());
				string packageMapPath = PathNormalizer.Normalize(packageMapRelease.Path.EnsureTrailingSlash());

				if (!executingPath.Equals(packageMapPath, PathUtil.PathStringComparison))
				{
					executingDoesNotMatchPackageMap = true; // don't throw error right away - proceed for now and throw after checking if this was
															// caused by an exception property that was set too late
					 
					executingDoesNotMatchPackageMapMsg = $"The release of the package '{PackageName}-{executingRelease.Version}' you are building ({executingPath}) does not match the one listed in the masterconfig '{PackageName}-{packageMapRelease.Version}' ({packageMapPath}). " +
						$"You seem to have a different release indicated in your masterconfig for {PackageName} than the release you are trying to build. If this package is the package you are building at the top level the property 'nant.usetoplevelversion=true' can be set to force " +
						$"Framework to accept the build file nant was invoked on rather than the one the masterconfig indicates.";
				}
			}

			// check if manifest version does not match the version we have chosen, do this here since this doesn't matter that much until
			// we initialize package properties
			if (!String.IsNullOrEmpty(packageMapRelease.Manifest.Version) && !packageMapRelease.Version.Equals(packageMapRelease.Manifest.Version, StringComparison.OrdinalIgnoreCase))
			{
				// using "flattened" version in masterconfig even when package has a version is fairly common - in this case only throw at advise level
				Log.WarnLevel versionNameWarningLevel = packageMapRelease.Version == Release.Flattened ? Log.WarnLevel.Advise : Log.WarnLevel.Minimal;

				Log.ThrowWarning(Log.WarningId.InconsistentVersionName, versionNameWarningLevel, $"{Location} Package was initialized using masterconfig version '{packageMapRelease.Version}', " +
					$"but package Manifest.xml specified version '{packageMapRelease.Manifest.Version}'. It is recommended to keep versions consistent.");
			}

			// validate target version
			TargetVersion = TargetVersion?.Trim() ?? packageMapRelease.Version;
			if (!NAntUtil.IsPackageVersionStringValid(TargetVersion, out string versionInvalidReason))
			{
				throw new BuildException($"[build file] Package targetversion name {TargetVersion} is invalid. {versionInvalidReason}");
			}

			// this may be a top level package, so check compatibility 
			// TODO avoid reundant checks 
			// dvaliant 2017/05/29 
			/* Dependency checking is done at <dependent> task time as well as here because we want to check compatibility even if dependent is a use style load 
			(aka, initialize.xml only). However compatibility checking has to be done here also, since if this package is a top level package then this is the first  
			time we have access to the release for it and because it is top level, <dependent> task never gets called for it. Doing this in both places means however 
			that dependency checking is done twice */
			PackageMap.Instance.CheckCompatibility(Log, packageMapRelease, (pkg) =>
			{
				if (PackageMap.Instance.TryGetMasterPackage(Project, pkg, out MasterConfig.IPackage pkgSpec))
				{
					string key = Package.MakeKey(pkg, (pkg == "Framework") ? PackageMap.Instance.GetFrameworkRelease().Version : pkgSpec.Version, configName);
					if (Project.BuildGraph().Packages.TryGetValue(key, out IPackage package))
					{
						return package.Release;
					}
				}
				return null;
			});

			// add package.* properties
			SetPackageProperty(PackageProperties.PackageNamePropertyName, packageMapRelease.Name);
			SetPackageProperty(PackageProperties.PackageVersionPropertyName, packageMapRelease.Version);
			SetPackageProperty(PackageProperties.PackageTargetVersionPropertyName, TargetVersion);
			SetPackageProperty(PackageProperties.PackageDirectoryPropertyName, Project.BaseDirectory);
			SetPackageProperty(PackageProperties.PackageRootPropertyName, PackageFunctions.GetPackageRoot(Project, packageMapRelease.Name));

			// setup build folder properties
			string buildRoot = PackageMap.Instance.GetBuildGroupRoot(Project, packageMapRelease.Name);
			SetPackageProperty($"package.{packageMapRelease.Name}.buildroot", PathNormalizer.Normalize(buildRoot));

			// add package.name.* properties
			SetPackageProperty($"package.{packageMapRelease.Name}.dir", Project.BaseDirectory);
			SetPackageProperty($"package.{packageMapRelease.Name}.version", packageMapRelease.Version);
            SetPackageProperty($"package.{packageMapRelease.Name}.root", Project.Properties[PackageProperties.PackageRootPropertyName]);

			AddImplicitFrameworkDependency();
            IncludeConfiguration(configName, packageMapRelease);

            // Set package.configbindir and package.configlibdir if not already set, otherwise set as readonly
            // must executed here as ModulePath.Defaults relies on package.<packagename>.builddir being set
            // and config property being set
            Project.Properties.Add(PackageProperties.PackageConfigBinDir, Project.Properties[PackageProperties.PackageConfigBinDir] ?? ModulePath.Defaults.GetDefaultConfigBinDir(Project, packageMapRelease.Name));
            Project.Properties.Add(PackageProperties.PackageConfigLibDir, Project.Properties[PackageProperties.PackageConfigLibDir] ?? ModulePath.Defaults.GetDefaultConfigLibDir(Project, packageMapRelease.Name));

			Project.RecomputeGlobalPropertyExceptions(); // global properties which came from the masterconfig are allow to be updated once config is loaded in case they are conditional on config package settings
			Project.Log.ApplyLegacyProjectSuppressionProperties(Project);
			Project.Log.ApplyLegacyPackageSuppressionProperties(Project, packageMapRelease.Name);
			Project.Log.ApplyPackageSuppressions(Project, packageMapRelease.Name); // DAVE-FUTURE-REFACTOR-TODO, we could potentially apply these earlier (ConsoleRunner.cs for top level, NantTask for dependents since we 90% of the time know the package name ahead of the PackageTask being run)
			IncludeInitializeScript(packageMapRelease);

			string postConfigLoadVersion = PackageMap.Instance.GetMasterPackage(Project, packageMapRelease.Name).Version;
			if (postConfigLoadVersion != packageMapRelease.Version)
			{
				throw new BuildException($"Package '{packageMapRelease.Name}' resolved to version '{packageMapRelease.Version}' prior to loading but to version" +
					$" '{postConfigLoadVersion}' after config initialization. You likely have a masterconfig exception evaluating a property that is not set early enough.");
			}
			else if (executingDoesNotMatchPackageMap)
			{
				throw new BuildException(executingDoesNotMatchPackageMapMsg);
			}

			string testBuildRoot = PackageMap.Instance.GetBuildGroupRoot(Project, packageMapRelease.Name);

			if (!String.Equals(buildRoot, testBuildRoot, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
			{
				Log.Warning.WriteLine("!!!! Package '{0}' buildroot evaluates to different values before and after config package is loaded,{1}                !!!! This may cause unpredictable build results.{1}                !!!! First  value: {2}{1}                !!!! Second value: {3}",
					packageMapRelease.Name, Environment.NewLine, buildRoot, testBuildRoot);
			}

			// Add global defines from masterconfig file to top level project // DAVE-FUTURE-REFACTOR-TODO: why is this done here and not in project init?
			foreach (KeyValuePair<string, string> prop in GetGlobalDefinesProperties(Project))
			{
				Project.Properties.Add(prop.Key, prop.Value, readOnly: false, deferred: false, local: false);
			}
		}

		/// <summary>Make an implicit dependency on the Framework package we are using to build with.</summary>
		private void AddImplicitFrameworkDependency()
		{
			Release frameworkRelease = PackageMap.Instance.GetFrameworkRelease();
			PackageInitializer.AddPackageProperties(Project, frameworkRelease);
			Project.BuildGraph().GetOrAddPackage
			(
				frameworkRelease,
				Project.Properties[PackageProperties.ConfigNameProperty] ?? PackageMap.Instance.DefaultConfig
			);

			if (PackageMap.Instance.TryGetMasterPackage(Project, "Framework", out MasterConfig.IPackage masterConfigFrameworkPackage))
			{
				if (frameworkRelease.Version != masterConfigFrameworkPackage.Version)
				{
					if (frameworkRelease.Version.StartsWith("dev") || frameworkRelease.Version == "stable" || frameworkRelease.Version.StartsWith("work"))
					{
						// Allow development versions of Framework to be dropped into any project and not trigger
						// version mismatch exceptions.
						Log.Warning.WriteLine("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
						Log.Warning.WriteLine("Masterconfig file specifies Framework-{0}, but we are using Framework-{1}. As Framework-{1} is a development version we are assuming you are making a conscious choice to test this yet unreleased version and will allow the build to continue.", masterConfigFrameworkPackage.Version, frameworkRelease.Version);
						Log.Warning.WriteLine("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
					}
					else
					{
						string msg = String.Format("Masterconfig file specifies Framework-{0}, but we are using Framework-{1}. Please re-invoke the build using the specified version of Framework.", masterConfigFrameworkPackage.Version, frameworkRelease.Version);
						throw new BuildException(msg, Location);
					}
				}
			}
		}

		public class GlobalDefineDef : Project.GlobalPropertyDef
		{
			public readonly bool DotNet;
			public readonly string Packages;

			public GlobalDefineDef(string name, string initialValue = null, string condition = null,
				bool dotnet = false, string packages = "")
				: base(name, initialValue, condition)
			{
				DotNet = dotnet;
				Packages = packages;
			}
		}

		private Dictionary<string, string> GetGlobalDefinesProperties(Project project)
		{
			Dictionary<string, string> globaldefinesdict = new Dictionary<string, string>
			{
				{ "global.defines", "" },
				{ "global.dotnet.defines", "" }
			};

			Project.GlobalPropertyBag propertybag = new Project.GlobalPropertyBag();
			foreach (var prop in PackageMap.Instance.MasterConfig.GlobalDefines)
			{
				propertybag.Add(new GlobalDefineDef(prop.ToString(), String.Empty, prop.Condition, prop.DotNet, prop.Packages));
			}

			foreach (Project.GlobalPropertyDef propdef in propertybag.EvaluateExceptions(project))
			{
				GlobalDefineDef defdef = ((GlobalDefineDef)propdef);
				if (defdef.InitialValue != null)
				{
					string define_value = string.Format("{0}\n", defdef.Name);
					string globaldefinesprop = (defdef.DotNet ? "global.dotnet.defines" : "global.defines");
					if (defdef.Packages.IsNullOrEmpty())
					{
						globaldefinesdict[globaldefinesprop] += define_value;
					}
					else
					{
						// add package specific global defines
						foreach (string package in defdef.Packages.ToArray())
						{
							string propname = string.Format("{0}.{1}", globaldefinesprop, package);
							if (globaldefinesdict.ContainsKey(propname))
							{
								globaldefinesdict[propname] += define_value;
							}
							else
							{
								globaldefinesdict.Add(propname, define_value);
							}
						}
					}
				}
			}
			return globaldefinesdict;
		}

		private void IncludeConfiguration(string configName, Release release)
		{
			// config.targetoverrides is an optionset mechansim that used to be set before config loading
			// throw deprecation if it is set before processing config
			if (Project.NamedOptionSets.TryGetValue("config.targetoverrides", out OptionSet deprecatedTargetOverrides))
			{
				// use location of optionset declaration in warning if available, if not use the location of this task as it's usually close by
				Location optSetLocation = deprecatedTargetOverrides.Location != Location.UnknownLocation ? deprecatedTargetOverrides.Location : Location;

				Log.ThrowDeprecation
				(
					Log.DeprecationId.ConfigTargetOverrides, Log.DeprecateLevel.Advise, 
					"{0} Declaring 'config.targetoverrides' optionset is no longer required. All targets are now defined by default in eaconfig but with the 'allowoverride=\"true\"' attribute set. " +
					"You can remove this optionset or, if you wish to maintain backwards compatibility with older Framework versions, add condition 'unless=\"@{{StrCompareVersions('${{nant.version}}', '8.0.0')}} gte 0\"' to it's declaration to avoid this warning. " +
					"Any targets that replace the default targets must be declared with 'override=true'." ,
					optSetLocation.ToString()
				);
			}

			StringBuilder targetString = new StringBuilder();
			if (Project.BuildTargetNames.Count > 0)
			{
				foreach (string name in Project.BuildTargetNames)
				{
					targetString.AppendFormat(" {0}", name);
				}
			}

			if (!String.IsNullOrEmpty(configName))
			{
				// Verify input path values:
				PathVerifyValid(PackageMap.Instance.ConfigDir, "PackageMap.Instance.ConfigDir");
				PathVerifyValid(configName, "configName");
				PathVerifyValid(PackageProperties.ConfigFileExtension, "PackageProperties.ConfigFileExtension");

				if (Log.StatusEnabled)
				{
					Log.Status.WriteLine($"{LogPrefix}{release.Name}-{release.Version} ({configName}) {targetString.ToString()}");
				}
				LogLoadPackageTelemetry(release, configName);

				// set the package.config property // TODO, we really need property if we have 'config'?
				Project.Properties.AddReadOnly(PackageProperties.PackageConfigPropertyName, configName);

				// set package directory properties based of of templates (or default templates)
				{
					// %version% in replacement templates is replaced with package version EXCEPT for when
					// package has the special version of "flattened" in which case it is replace by empty string
					string replacementVersion = Project.Properties[PackageProperties.PackageVersionPropertyName];
					replacementVersion = replacementVersion == Release.Flattened ? String.Empty : replacementVersion;

					MacroMap rootTemplatesMap = new MacroMap()
					{
						{ "buildroot", PackageMap.Instance.GetBuildGroupRoot(Project, release.Name) },
						{ "package.name", release.Name },
						{ "package.version", replacementVersion },
						{ "package.root", PackageFunctions.GetPackageRoot(Project, release.Name) }
					};

					string buildDirTemplate = Project.Properties["package.builddir.template"] ?? "%buildroot%/%package.name%/%package.version%";
					string buildDir = PathNormalizer.Normalize(MacroMap.Replace(buildDirTemplate, rootTemplatesMap, additionalErrorContext: " from property 'package.builddir.template'"));
					SetPackageProperty(PackageProperties.PackageBuildDirectoryPropertyName, buildDir);
					SetPackageProperty(String.Format("package.{0}.builddir", release.Name), buildDir);

					string genDirTemplate = Project.Properties["package.gendir.template"] ?? "%buildroot%/%package.name%/%package.version%";
					string genDir = PathNormalizer.Normalize(MacroMap.Replace(genDirTemplate, rootTemplatesMap, additionalErrorContext: " from property 'package.gendir.template'"));
					SetPackageProperty(PackageProperties.PackageGenDirectoryPropertyName, genDir);
					SetPackageProperty(String.Format("package.{0}.gendir", release.Name), genDir);

					// DAVE-FUTURE-REFACTOR-TODO should evaluate why these properties could already exist - in theory this is the only place
					// it makes sense to set them
					if (!Project.Properties.Contains(PackageProperties.PackageConfigBuildDir))
					{
						Project.Properties.Add(PackageProperties.PackageConfigBuildDir,
							PathNormalizer.Normalize(Path.Combine(buildDir, configName, "build")));
					}
					if (!Project.Properties.Contains(PackageProperties.PackageConfigGenDir))
					{
						Project.Properties.Add(PackageProperties.PackageConfigGenDir,
							PathNormalizer.Normalize(Path.Combine(genDir, configName, "build"))); // default is 'build' rather than 'gen' because we want to have this by the same value as builddir by default - in future we may not though
					}
					if (!Project.Properties.Contains(PackageProperties.PackageConfigBinDir))
					{
						Project.Properties.Add(PackageProperties.PackageConfigBinDir,
							PathNormalizer.Normalize(Path.Combine(buildDir, configName, "bin")));
					}
					if (!Project.Properties.Contains(PackageProperties.PackageConfigLibDir))
					{
						Project.Properties.Add(PackageProperties.PackageConfigLibDir,
							PathNormalizer.Normalize(Path.Combine(buildDir, configName, "lib")));
					}
				}

				if (!Project.TrySetPackage(release, Project.Properties[PackageProperties.PackageConfigPropertyName], out IPackage package))
				{
					throw new BuildException($"<package> task is called twice for {release.Name}-{release.Version} [{Project.Properties[PackageProperties.PackageConfigPropertyName]}] in the build file, or Framework internal error.");
				}
				else
				{
					package.SetType(release.Manifest.Buildable ? Package.Buildable : BitMask.None);
				}

				// load all of the configuration files from configuration packages
				// DAVE-FUTURE-REFACTOR-TODO: ideally this would be one ConfigPackageLoader call, but right now avoiding the work of moving Combine down
				// to NAnt.Core - boundary between NAnt.XXX and EA.XXX was always blurry but now really needs a re-think
				ConfigPackageLoader.InitializePrecombineConfiguration(Project, Location);
				Combine.Execute(Project);
				ConfigPackageLoader.InitializePostCombineConfiguration(Project, Location);

				// Set package properties that are possibly defined in the configuration package.
				if (package.PackageConfigBuildDir == null)
				{
					if (Properties.Contains("package.configbuilddir"))
					{
						package.PackageConfigBuildDir = PathString.MakeNormalized(Properties["package.configbuilddir"]);
					}
					else
					{
						package.PackageConfigBuildDir = package.PackageBuildDir;
					}
				}

				if (package.PackageConfigGenDir == null)
				{
					if (Properties.Contains("package.configgendir"))
					{
						package.PackageConfigGenDir = PathString.MakeNormalized(Properties["package.configgendir"]);
					}
					else
					{
						package.PackageConfigBuildDir = package.PackageBuildDir;
					}
				}

				// add config dir property
				Project.Properties.AddReadOnly(PackageProperties.PackageConfigDirPropertyName, _configDirectoryName);
			}
		}

		private void LogLoadPackageTelemetry(Release release, string configName)
		{
			// Add some extra information about the environment, unfortunately this is very Frostbite specific, but the rest of EA doesn't have any common environment settings like this we can use
			// In non-frostbite environments this will all just be empty.
			Dictionary<string,string> extraData = new Dictionary<string,string>();
			try
			{
				extraData["licensee"] = Environment.GetEnvironmentVariable("LICENSEE_ID") ?? string.Empty;
				extraData["studioname"] = Environment.GetEnvironmentVariable("STUDIO_NAME") ?? string.Empty;
				extraData["branchname"] = Environment.GetEnvironmentVariable("BRANCH_NAME") ?? string.Empty;
				extraData["exemainname"] = Environment.GetEnvironmentVariable("EXE_MAIN_NAME") ?? string.Empty;
				extraData["branchid"] = Environment.GetEnvironmentVariable("FB_BRANCH_ID") ?? string.Empty;
				extraData["stream"] = Environment.GetEnvironmentVariable("FB_STREAM") ?? string.Empty;
				extraData["autobuild"] = Environment.GetEnvironmentVariable("FB_AUTOBUILD") ?? Boolean.FalseString;
			}
			catch { } // Exception can be thrown if user doesn't have permission to perform that above operation. Silently continue.

			// Commenting this out for now as it's incredibly noisy, and is accounting for more than half of Azure telemetry we gather across Frostbite.
			// Revisit this at some point with some sort of batching scheme.
			//Telemetry.TrackLoadPackage(release.Name, configName, release.Version, release.Manifest.DriftVersion, extraData);
		}

		private void IncludeInitializeScript(Release release)
		{
			if (InitializeSelf.HasValue)
			{
				if (!InitializeSelf.Value)
				{
					Log.ThrowDeprecation(Log.DeprecationId.PackageInitializeSelf, Log.DeprecateLevel.Minimal, "{0}<package> task declared with 'initializeself=\"false\"'. This is no longer supported, including Initialize.xml.", Location);
				}
				else
				{
					Log.ThrowDeprecation(Log.DeprecationId.PackageInitializeSelf, Log.DeprecateLevel.Advise, "{0}<package> task declared with deprecated attribute 'initializeself'. This attribute is ignored and Initialize.xml is automatically included.", Location);
				}
			}

			PackageInitializer.IncludePackageInitializeXml(Project, release, Location, warningOnMissing: false);
		}

        private void SetPackageProperty(string name, string value)
        {
			bool foundProperty = false;
			Project.Properties.SetOrAdd(name, (oldValue) =>
			{
				foundProperty = oldValue != null;
				return value;
			}, true, false, null);

			if (foundProperty)
				Project.Properties.ForceNonLocal(name);
        }

		// throws InvalidArgument exception if path is invalid
		private void PathVerifyValid(string path, string parameterName)
		{
			if (path != null)
			{
				if (-1 < path.IndexOfAny(Path.GetInvalidPathChars()))
				{
					throw new ArgumentException("Illegal characters in path (" + parameterName + ")='" + path + "'", parameterName);
				}
			}
		}
	}
}
