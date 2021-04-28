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
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.ExceptionServices;
using System.Text;
using System.Threading;

using EA.PackageSystem.PackageCore;

using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.NuGet;
using NAnt.Shared.Properties;

namespace NAnt.Core.PackageCore
{
	public class PackageMap
	{
		// TODO: back to masterconfig?

		// simple implementation of IPackageSpec for simple web server package installs / special
		// release like Framework / eaconfig / VisualStudio
		public sealed class PackageSpec : MasterConfig.IPackage
		{
			public string Name { get; private set; }
			public string Version { get; private set; }

			public string XmlPath => "<special>";

			public ReadOnlyDictionary<string, string> AdditionalAttributes => s_emptyAttributes;

			public bool? AutoBuildClean => false;
			public string AutoBuildTarget => String.Empty;
			public string AutoCleanTarget => String.Empty;

			public bool? LocalOnDemand => false;

			public bool IsAutoVerison => false;

			public Dictionary<Log.WarningId, MasterConfig.WarningDefinition> Warnings => s_noWarnings;
			public Dictionary<Log.DeprecationId, MasterConfig.DeprecationDefinition> Deprecations => s_noDeprecations;

			private static readonly ReadOnlyDictionary<string, string> s_emptyAttributes = new ReadOnlyDictionary<string, string>(new Dictionary<string, string>());
			private static readonly Dictionary<Log.WarningId, MasterConfig.WarningDefinition> s_noWarnings = new Dictionary<Log.WarningId, MasterConfig.WarningDefinition>();
			private static readonly Dictionary<Log.DeprecationId, MasterConfig.DeprecationDefinition> s_noDeprecations = new Dictionary<Log.DeprecationId, MasterConfig.DeprecationDefinition>();
			private static readonly MasterConfig.GroupType s_defaultGroup = new MasterConfig.GroupType(MasterConfig.GroupType.DefaultMasterVersionsGroupName);

			public PackageSpec(string name, string version)
			{
				Name = name ?? throw new ArgumentNullException(nameof(name));
				Version = version ?? throw new ArgumentNullException(nameof(version));
			}

			public MasterConfig.GroupType EvaluateGroupType(Project project)
			{
				return s_defaultGroup;
			}
		}

		// deprecated packages are special packages that used to exist outside of Framework but are now
		// included - they are still treated as separate packages when references outside of Framework
		// for backwards compatibility
		internal struct DeprecatedPackage
		{
			internal readonly MasterConfig.Package MasterPackage;
			internal readonly string AdditionalDeprecationInformation;

			internal DeprecatedPackage(MasterConfig.Package overridePackage, string additionalDeprecationInformation)
			{
				MasterPackage = overridePackage;
				AdditionalDeprecationInformation = additionalDeprecationInformation;
			}
		}

		// stores a release for given masterconfig condition or - if the release for a given condition
		// could not be resolved - it captures the exception thrown so it can be rethrown again if the
		// same release is requested
		internal struct ReleaseCacheResult
		{
			internal readonly ExceptionDispatchInfo OnDemandException;
			internal readonly ExceptionDispatchInfo NoOnDemandException;
			internal readonly Release Release;

			internal ReleaseCacheResult(ExceptionDispatchInfo exception, bool onDemand)
			{
				if (exception is null)
				{
					throw new ArgumentNullException(nameof(exception));
				}

				OnDemandException = onDemand ? exception : null;
				NoOnDemandException = onDemand ? null : exception;
				Release = null;
			}

			internal ReleaseCacheResult(Release release)
			{
				OnDemandException = null;
				NoOnDemandException = null;
				Release = release ?? throw new ArgumentNullException(nameof(release));
			}

			internal bool TryGetCachedResult(bool onDemand, out Release release)
			{
				// if we cached an installed release, then we just return it
				if (Release != null)
				{
					release = Release;
					return true;
				}

				// if we recorded an execption last time we tried to find this package when on demand was enabled, throw it with orignal callstack
				if (onDemand && OnDemandException != null)
				{
					OnDemandException.Throw();

					// satisfy compiler
					release = null;
					return true;
				}

				// same but for no ondemand case, this separation is because we don't want to throw errors 
				if (!onDemand && NoOnDemandException != null)
				{
					NoOnDemandException.Throw();

					// satisfy compiler
					release = null;
					return true;
				}

				throw new InvalidOperationException("Invalid entry stored in cache");
			}
		}

		// used to compare package specs for the purposes of using the release cache in PackageMap
		internal class PackageCacheComparer : IEqualityComparer<MasterConfig.IPackage>
		{
			public bool Equals(MasterConfig.IPackage x, MasterConfig.IPackage y)
			{
				// most common case is we have retrieved IPackage instance from masterconfig
				// which is always the same instance
				if (ReferenceEquals(x, y))
				{
					return true;
				}

				if (!x.Name.Equals(y.Name, StringComparison.OrdinalIgnoreCase))
				{
					return false;
				}

				if (!x.Version.Equals(y.Version, StringComparison.OrdinalIgnoreCase))
				{
					return false;
				}

				// these last two case are just paranoia - in common api usage we will be given a an equal reference or an object with no
				// attribtes and no local ondemand setting
				if (x.AdditionalAttributes.TryGetValue("uri", out string xUri) != y.AdditionalAttributes.TryGetValue("uri", out string yUri))
				{
					return false;
				}

				if (xUri != yUri)
				{
					return false;
				}

				if (x.LocalOnDemand != y.LocalOnDemand)
				{
					return false;
				}

				return true;
			}

			public int GetHashCode(MasterConfig.IPackage obj)
			{
				return ((obj.Name.GetHashCode() << 5) + obj.Name.GetHashCode()) ^ obj.Version.GetHashCode();
			}
		}

		// this struct just wraps a release discovered when scanning local machine for releases
		// and string which explains why that release was not acceptable for finding the active
		// release - used in error reporting only
		internal struct RejectedRelease
		{
			internal readonly Release Release;
			internal readonly string Reason;

			internal RejectedRelease(Release release, string rejectionReason)
			{
				Release = release;
				Reason = rejectionReason;
			}
		}

		public const string MasterConfigFile = "masterconfig.xml";
		public const string DefaultConfigPackage = "eaconfig";
		public const string UseTopLevelVersionProperty = "nant.usetoplevelversion";

		public static PackageMap Instance
		{
			get
			{
				if (s_instance == null)
				{
					throw new BuildException("PackageMap is not initialized, call Init(Project project) method.");
				}
				return s_instance;
			}
		}

		public string BuildRoot { get; private set; }

		public IList<string> ConfigExcludes { get { return MasterConfig.Config.Excludes; } }

		public IList<string> ConfigIncludes { get { return MasterConfig.Config.Includes; } }
		public string ConfigPackageName { get { return MasterConfig.Config.Package ?? DefaultConfigPackage; } }
		public string DefaultConfig { get { return MasterConfig.Config.Default; } }

		public string ConfigPackageDir;
		public string ConfigDir;
		public string ConfigPackageVersion;

		public readonly List<string> MasterPackages; // TODO: obsoluete - this is just MasterConfig.Packages.Keys
		public readonly PackageRootList PackageRoots = new PackageRootList();

		private bool m_configPackageInitialized = false;

		private readonly List<string> m_unresolvedMetaPackages = new List<string>();

		private readonly bool m_autoBuildAll;
		private readonly HashSet<string> m_autoBuildPackages;
		private readonly HashSet<string> m_autoBuildGroups;

		private readonly bool m_useV1;
		private readonly string m_packageServerToken;

		private Lazy<NugetManager> m_nugetManager;

		public List<string> GetConfigExtensions(Project project, bool evaluate = true)
		{
			if (evaluate == false)
			{
				return MasterConfig.Config.Extensions
					.Select(x => x.Name)
					.Distinct()
					.ToList();
			}

			// lazy evaluate the if statement on the config extension element so that as many properties are available as possible
			// BUT only do this once - we want a consistent set of extensions across Projects
			if (m_evaluatedConfigExtensions == null)
			{
				try
				{
					m_evaluatedConfigExtensions = MasterConfig.Config.Extensions.Where(x => x.IfClause == null || bool.Parse(project.ExpandProperties(x.IfClause)))
						.Select(x => x.Name)
						.Distinct()
						.ToList();
				}
				catch (Exception e)
				{
					throw new BuildException("The if clause of one of the <extension> elements in the masterconfig does not evaluate to a boolean", e);
				}
			}
			return m_evaluatedConfigExtensions;
		}
		private List<string> m_evaluatedConfigExtensions;

		/// <summary>
		/// Should we download missing packages from the package server on demand?
		/// This functionality make it harder than necessary to have a complete
		/// archive in source control, due to silent downloads of packages which
		/// haven't been checked into source control, so is turned off by default.
		/// </summary>
		public bool OnDemand { get; }
		public bool PackageServerOnDemand { get; }
		public bool LocalOnDemand { get; }

		/// <summary>
		/// If set to true, then package map apis will always use the the 
		/// version and location of the given top level release rather than 
		/// information from the masterconfig for that package.
		/// </summary>
		public Release TopLevelRelease { get; private set; }

		/// <summary>Mechanism for specifying versions automatically if we can determine package version to use without a masterconfig version.</summary>
		private readonly ConcurrentDictionary<string, MasterConfig.Package> AutomaticPackageVersions = new ConcurrentDictionary<string, MasterConfig.Package>();

		private readonly Dictionary<string, DeprecatedPackage> m_deprecatedPackages = new Dictionary<string, DeprecatedPackage>();

		/// <summary>
		/// Every time we attempt to locate an installed package or install a package we query this cache. For each IPackage (with custom compare rules) it will return
		/// a cache object that will either contain 1) a found installed release or a release that was installed by last call or 2) an exception (with dispatch info)
		/// that was thrown last time a package was searched / installed. The exception can be valid for remote search (on package server) or just for local searching
		/// so TryGetCachedResult ReleaseCacheResult to validate the right exception is being thrown. This dictionary is concurrent just so multiple entries can be updated
		/// parallel - we never expect to have two threads update the same entry simultaneously.
		/// </summary>
		private readonly ConcurrentDictionary<MasterConfig.IPackage, ReleaseCacheResult> m_packageReleaseCache = new ConcurrentDictionary<MasterConfig.IPackage, ReleaseCacheResult>(new PackageCacheComparer());

		private Release m_executingFrameworkRelease = null;

		private static PackageMap s_instance = null;

		public static bool IsExecutingFramework()
		{
			Assembly entryAssembly = Assembly.GetEntryAssembly();
			return entryAssembly != null && Path.GetFileName(entryAssembly.Location) == "nant.exe";
		}

		/// <summary>
		/// Returns the Framework package release we are currently running or throws exeception. Should only be called
		/// from code paths that are limited to running inside Framework.
		/// </summary>
		public Release GetFrameworkRelease()
		{
			if (m_executingFrameworkRelease != null)
			{
				return m_executingFrameworkRelease;
			}

			// framework release should be one directory above the bin directory
			DirectoryInfo binDirectory = new DirectoryInfo(PathUtil.GetExecutingAssemblyLocation());
			DirectoryInfo releaseDirectory = binDirectory.Parent;

			List<string> attemptedSearchPaths = new List<string>
			{
				releaseDirectory.FullName
			};

			while (releaseDirectory != null)
			{
				Manifest manifest = Manifest.Load(releaseDirectory.FullName);
				if (manifest != Manifest.DefaultManifest)
				{
					PackageSpec frameworkMasterPackage = new PackageSpec("Framework", manifest.Version);
					m_executingFrameworkRelease = new Release(frameworkMasterPackage, releaseDirectory.FullName, isFlattened: releaseDirectory.Name == "Framework", manifest);
					break;
				}
				// for our tests keep looking deeper until we find the root of the package, for our tests nant.exe is in Local/bin/test
				releaseDirectory = releaseDirectory.Parent;
			}

			if (m_executingFrameworkRelease == null)
			{
				throw new ApplicationException(String.Format("Cannot find Framework release! The following search paths has been attempted:{0}{1}", Environment.NewLine, String.Join(Environment.NewLine, attemptedSearchPaths)));
			}

			return m_executingFrameworkRelease;
		}

		[Obsolete("Deprecated. To test if package appears in masterconfig use " + nameof(MasterConfigHasPackage) + ". To get masterconfig package details use " + nameof(TryGetMasterPackage) + ". " +
			"To get the current release use " + nameof(FindOrInstallCurrentRelease) + ".")]
		public Release GetMasterRelease(string packageName, Project project, bool? onDemand = null)
		{
			if (TryGetMasterPackage(project, packageName, out MasterConfig.IPackage masterPackage))
			{
				return FindOrInstallRelease(project, masterPackage, onDemand);
			}
			return null;
		}

		public Release FindOrInstallCurrentRelease(Project project, string packageName, bool? onDemand = null)
		{
			return FindOrInstallRelease(project, GetMasterPackage(project, packageName), onDemand);
		}

		// finds a release in package roots or (including packages already installed to ondemand roots)
		public Release FindInstalledRelease(MasterConfig.IPackage package)
		{
			// test cache for previous result, note we don't use TryGetCachedResult as we don't necessarily want to throw exception if
			// release was not found - instead just test if a release was cached. If not, will search again and throw a new exception
			// or return another null
			if (m_packageReleaseCache.TryGetValue(package, out ReleaseCacheResult releaseCacheResult) && releaseCacheResult.Release != null)
			{
				return releaseCacheResult.Release;
			}

			using (ProcessUtil.WindowsNamedMutexWrapper packageSearchMutex = CreatePackageInstallMutex(package)) // multiprocess safety only, in process we cache results
			{
				if (!packageSearchMutex.HasOwnership)
				{
					packageSearchMutex.WaitOne();
				}

				List<RejectedRelease> rejectedReleases = null;
				Release installedRelease = FindInstalledReleaseInternal(package, ref rejectedReleases);
				if (installedRelease != null)
				{
					// note we don't cache on failure to find an installed release via this api - the situations where this API is designed
					// to be used (vs FindOrInstallRelease) are not one where we expect to constantly querying for a missing installed release
					m_packageReleaseCache[package] = new ReleaseCacheResult(installedRelease);
				}
				return installedRelease;
			}
		}

		[Obsolete("Deprecated. Please be careful using this function - you likely want to use FindOrInstallCurrentRelease which will return (or download) " +
			"the package release specified by your masterconfig. If you really wish to find an explicit version of an installed package use " +
			nameof(FindInstalledRelease) + " passing a " + nameof(PackageSpec) + " with the required name and version.")]
		public Release FindReleaseByNameAndVersion(string packageName, string version)
		{
			return FindInstalledRelease(new PackageSpec(packageName, version));
		}

		// finds a release in package roots or (including packages already installed to ondemand roots), if rejectedReleases
		// is not null it is populated with releases of the correct name but are unsuitable due to location or version
		// should only be called with inside of mutex lock
		private Release FindInstalledReleaseInternal(MasterConfig.IPackage package, ref List<RejectedRelease> rejectedReleases)
		{
			if (package.Name.Equals(TopLevelRelease?.Name, StringComparison.OrdinalIgnoreCase) && package.Version.Equals(TopLevelRelease?.Version))
			{
				return TopLevelRelease;
			}

			// before searching for the package - check if it is a deprecated package (eaconfig / VisualStudio) and just assume
			// it is within Framework package
			string deprecatedPackageName = m_deprecatedPackages.Keys.FirstOrDefault(deprecated => deprecated.Equals(package.Name, StringComparison.OrdinalIgnoreCase)); // do key search rather than a look up																																									// we don't use a case insensitive look up because we want to get the casing used by the key rather than packageSpec
			if (deprecatedPackageName != null)
			{
				DeprecatedPackage deprecatedPackage = m_deprecatedPackages[deprecatedPackageName];
				string nantPath = IsExecutingFramework() ? GetFrameworkRelease().Path : Path.Combine(Project.NantLocation, "..");
				string packagePath = Path.Combine(nantPath, deprecatedPackageName);
				return new Release
				(
					deprecatedPackage.MasterPackage,
					packagePath,
					isFlattened: true,
					Manifest.Load(packagePath)
				);
			}

			foreach (PackageRootList.Root packageRoot in PackageRoots)
			{
				string rawPackageRootDirectory = Path.Combine(packageRoot.Dir.FullName, package.Name);
				string packageRootDirectory = PathNormalizer.Normalize(rawPackageRootDirectory);

				// early out if root does, not exists - warnings for these missing roots were thrown during package map init
				if (!Directory.Exists(packageRootDirectory))
				{
					continue;
				}

				// determine if package release can be found in this package root
				string rejectedReleaseReason = null;
				{
					bool isWorkOnPackage = package.AdditionalAttributes.ContainsKey("switched-from-version");
					if (isWorkOnPackage)
					{
						// work on'd packages can ONLY be found in development root
						if (packageRoot.Type != PackageRootList.RootType.DevelopmentRoot)
						{
							rejectedReleaseReason = "Package has 'switched-from-version' attribute and is not located in the development root.";
						}
					}
					else
					{
						// non work on'd package can be found in ondemand or local ondemand root (as appropriate to whether package is local ondemand) 
						// or in general package roots
						bool isLocalPackage = (package.LocalOnDemand ?? PackageRoots.UseLocalOnDemandRootAsDefault) && LocalOnDemand;
						if (isLocalPackage && packageRoot.Type == PackageRootList.RootType.OnDemandRoot)
						{
							rejectedReleaseReason = "Package is a local ondemand package and not usable from regular ondemand root.";
						}
						else if (!isLocalPackage && packageRoot.Type == PackageRootList.RootType.LocalOnDemandRoot)
						{
							rejectedReleaseReason = "Package is not a local ondemand package and not usable from local ondemand root.";
						}
						else if (packageRoot.Type == PackageRootList.RootType.DevelopmentRoot)
						{
							rejectedReleaseReason = "Package is not a 'switched-from-version' package and not usable outside development root.";
						}
					}
				}

				// if there was reason to reject the release and we're not logging rejected releases then
				// skip scanning this package root
				if (rejectedReleaseReason != null && rejectedReleases == null)
				{
					continue;
				}

				// check for package without version folder, look for <Root>/<PackageName>/<PackageName>.build 
				// or <Root>/<PackageName>//Manifest.xml;
				Manifest flattenedManifest = SafeLoadManifest(packageRootDirectory, out bool corruptManifest);
				string flattenedBuildFile = Path.Combine(packageRootDirectory, package.Name + ".build");
				if (PathUtil.TryGetFilePathFileNameInsensitive(flattenedBuildFile, out _) || flattenedManifest != Manifest.DefaultManifest)
				{
					// first check special "flattened" case - if masterconfig version lists
					// version as flattened we will accept any package with <Root>/<PackageName>/Manifest.xml
					if (package.Version == Release.Flattened || flattenedManifest.Version == package.Version)
					{
						Release release = new Release(package, packageRootDirectory, isFlattened: true, manifest: flattenedManifest);
						if (rejectedReleaseReason == null)
						{
							return release;
						}
						else
						{
							rejectedReleases?.Add(new RejectedRelease(release, rejectedReleaseReason));
						}
					}
					else if (rejectedReleases != null)
					{
						Release mismatchVersionRelease = new Release(new PackageSpec(package.Name, corruptManifest ? "CORRUPT" : flattenedManifest.Version), packageRootDirectory, isFlattened: true, manifest: flattenedManifest);
						rejectedReleases.Add(new RejectedRelease(mismatchVersionRelease, rejectionReason: corruptManifest ? "Corrupt Manifest.xml file." : String.Empty));
					}
				}
				else if (corruptManifest)
				{
					Release corruptRelease = new Release(new PackageSpec(package.Name, "CORRUPT"), packageRootDirectory, isFlattened: true, manifest: flattenedManifest);
					rejectedReleases.Add(new RejectedRelease(corruptRelease, rejectionReason: String.Empty));
				}

				// finally, looks though sub directories of root for
				// <Root>/<PackageName>/<Version>/<PackageName>.build or <Root>/<PackageName>/<Version>/Manifest.xml. Done as case insensitive comparsion
				// for Version regardless of filesystem case sensitivity.
				DirectoryInfo packageDirectoryInfo = new DirectoryInfo(packageRootDirectory);
				foreach (DirectoryInfo subdirectory in packageDirectoryInfo.GetDirectories())
				{
					bool folderVersionMatches = String.Equals(subdirectory.Name, package.Version, StringComparison.OrdinalIgnoreCase);
					if (!folderVersionMatches && rejectedReleases == null)
					{
						continue; // don't scan this folder if we're not scanning for unsuitable packages
					}

					Manifest versionFolderManifest = SafeLoadManifest(subdirectory.FullName, out corruptManifest);
					string versionedBuildFile = Path.Combine(subdirectory.FullName, package.Name + ".build");
					if (PathUtil.TryGetFilePathFileNameInsensitive(versionedBuildFile, out _) || versionFolderManifest != Manifest.DefaultManifest)
					{
						if (folderVersionMatches)
						{
							Release release = new Release(package, subdirectory.FullName, isFlattened: false, manifest: versionFolderManifest);
							if (rejectedReleaseReason == null)
							{
								return release;
							}
							else
							{
								rejectedReleases?.Add(new RejectedRelease(release, rejectedReleaseReason));
							}
						}
						else if (rejectedReleases != null)
						{
							Release mismatchVersionRelease = new Release(new PackageSpec(package.Name, subdirectory.Name), subdirectory.FullName, isFlattened: false, manifest: versionFolderManifest);
							rejectedReleases.Add(new RejectedRelease(mismatchVersionRelease, rejectionReason: String.Empty));
						}
					}
					else if (rejectedReleases != null)
					{
						string reason = corruptManifest ?
							$"This package has a corrupt Manifest.xml file" :
							$"This folder exists but does not contain a Manifest.xml or a {package.Name}.build file and so is not considered a valid package.";

						Release noPackageFilesRelease = new Release(new PackageSpec(package.Name, subdirectory.Name), subdirectory.FullName, isFlattened: false, manifest: versionFolderManifest);
						rejectedReleases.Add(new RejectedRelease(noPackageFilesRelease, rejectionReason: reason));
					}
				}
			}

			return null;
		}

		private static Manifest SafeLoadManifest(string directory, out bool corruptManifest)
		{
			try
			{
				corruptManifest = false;
				return Manifest.Load(directory);
			}
			catch
			{
				if (PathUtil.TryGetFilePathFileNameInsensitive(Path.Combine(directory, "Manifest.xml"), out _))
				{
					corruptManifest = true;
					return Manifest.DefaultManifest;
				}
				else
				{
					throw;
				}
			}
		}

		public void SetTopLevelRelease(Release topLevelRelease)
		{
			if (TopLevelRelease != null)
			{
				throw new InvalidOperationException("Top level release can only be set once!");
			}

			TopLevelRelease = topLevelRelease;
		}

		// finds all the potential packages in path, regardless of active masterconfig
		// only really used for ondemand metadata - needs to be kept in sync with 
		// FindInstalledReleaseInternal logic
		public List<Release> GetReleasesInPath(string path)
		{
			List<Release> releases = new List<Release>();
			DirectoryInfo pathDirectoryInfo = new DirectoryInfo(path);
			foreach (DirectoryInfo packageDirectory in pathDirectoryInfo.GetDirectories())
			{
				string flattenedBuildFile = Path.Combine(packageDirectory.FullName, packageDirectory.Name + ".build");
				if (PathUtil.TryGetFilePathFileNameInsensitive(flattenedBuildFile, out _))
				{
					Manifest flattenedManifest = Manifest.Load(packageDirectory.FullName);
					string version = String.IsNullOrEmpty(flattenedManifest.Version) ? Release.Flattened : flattenedManifest.Version;
					releases.Add(new Release(new PackageSpec(packageDirectory.Name, version), packageDirectory.FullName, isFlattened: true, manifest: flattenedManifest));
				}

				foreach (DirectoryInfo versionDirectory in packageDirectory.GetDirectories())
				{
					string versionedBuildFile = Path.Combine(versionDirectory.FullName, packageDirectory.Name + ".build");
					if (PathUtil.TryGetFilePathFileNameInsensitive(versionedBuildFile, out _))
					{
						// prefer manifest.xml version casing if version folder matches insensitively, otherwise
						// just take version folder - version folder trumps manifest
						string version = versionDirectory.Name;
						Manifest versionFolderManifest = Manifest.Load(versionDirectory.FullName);
						if (versionFolderManifest.Version.Equals(version, StringComparison.OrdinalIgnoreCase))
						{
							version = versionFolderManifest.Version;
						}
						releases.Add(new Release(new PackageSpec(packageDirectory.Name, version), versionDirectory.FullName, isFlattened: false, manifest: versionFolderManifest));
					}
				}
			}

			return releases;
		}

		public MasterConfig.IPackage GetMasterPackage(Project project, string packageName)
		{
			if (!TryGetMasterPackage(project, packageName, out MasterConfig.IPackage package))
			{
				// there was nothing in masterconfig for this package, and nant.usetoplevelversion does not apply so we have to throw an error
				string metaPkgMsg = m_unresolvedMetaPackages.Count > 0 ?
					$" The following metapackages were not resolved during Framework startup and may have contained the version information for this package: {String.Join(",", Instance.m_unresolvedMetaPackages)}." :
					String.Empty;

				string buildFileMsg = !String.IsNullOrEmpty(project.CurrentScriptFile) ? $" Found during the execution of {project.CurrentScriptFile}." : String.Empty;
				throw new BuildException($"A request was made to find the current release for package '{packageName}' but this package was not specified in the masterconfig.{buildFileMsg}{metaPkgMsg}");
			}
			return package;
		}

		[Obsolete("Deprecated. This function can be misleading - you likely want the overload that takes a Project a first argument instead as it will " +
			"return the masterconfig version with all exceptions evaluated. This function may not return the version you expect.")]
		public bool TryGetMasterPackage(string packageName, out MasterConfig.Package masterPackage)
		{
			return TryGetRootMasterPackage(packageName, out masterPackage);
		}

		public bool TryGetMasterPackage(Project project, string packageName, out MasterConfig.IPackage package)
		{
			// see if masterconfig contains an entry for this package
			bool foundMasterPackage = TryGetRootMasterPackage(packageName, out MasterConfig.Package masterPackage);

			// special override property that tells Framework to ignore masterconfig and use version X of a package if X was the .build file framework was invoked upon 
			if (packageName.Equals(TopLevelRelease?.Name, StringComparison.OrdinalIgnoreCase))
			{
				// return a fake masterconfig package as if masterconfig had an entry for top version - inherit attributes and group from actual masterconfig entry if one exists
				package = new MasterConfig.Package
				(
					grouptype: masterPackage?.GroupType ?? MasterConfig.MasterGroup, // don't evaluate here - we can evaluate group type at time of use
					name: TopLevelRelease.Name,
					version: TopLevelRelease.Version,
					attributes: (IDictionary<string, string>)masterPackage?.AdditionalAttributes ?? new Dictionary<string, string>(),
					autoBuildClean: masterPackage?.AutoBuildClean,
					isAutoVersion: false,
					warnings: masterPackage?.Warnings,
					deprecations: masterPackage?.Deprecations
				);
				return true;
			}

			if (!foundMasterPackage)
			{
				package = null;
				return false;
			}

			package = masterPackage.Exceptions.ResolveException(project, masterPackage);
			return true;
		}

		[Obsolete("Deprecated. Please use " + nameof(TryGetMasterPackage) + " instead (and the .Version property of the returned object).")]
		public string GetMasterVersion(string packageName, Project project)
		{
			if (TryGetMasterPackage(project, packageName, out MasterConfig.IPackage package))
			{
				return package.Version;
			}
			return null;
		}

		/// <summary>
		/// get master package of the given package, if none it set
		/// then set an automatic version to use for this package
		/// optionally takes the name of the package that is providing
		/// this version in which case we will inherit group/attributes
		/// </summary>
		public MasterConfig.IPackage GetMasterOrAutoPackage(Project project, string packageName, string autoVersion, string parentPackage = null)
		{
			if (TryGetMasterPackage(project, packageName, out MasterConfig.IPackage masterPackage))
			{
				return masterPackage;
			}

			// inherit parents group type, if parent provided
			MasterConfig.Package parentMasterPackage = null;
			if (parentPackage != null)
			{
				TryGetRootMasterPackage(parentPackage, out parentMasterPackage);
			}

			// add new auto package
			MasterConfig.Package autoPackage = new MasterConfig.Package
			(
				grouptype: parentMasterPackage.GroupType ?? MasterConfig.MasterGroup,
				name: packageName,
				version: autoVersion,
				attributes: (IDictionary<string, string>)parentMasterPackage?.AdditionalAttributes ?? new Dictionary<string, string>(),
				autoBuildClean: parentMasterPackage?.AutoBuildClean,
				localOndemand: parentMasterPackage?.LocalOnDemand,
				isAutoVersion: true
			);

			return AutomaticPackageVersions.GetOrAdd(packageName, autoPackage);
		}

		public string GetBuildGroupRoot(Project project, string packageName)
		{
			if (BuildRoot == null)
			{
				throw new InvalidOperationException($"Build root has not be initialized. Call {nameof(SetBuildRoot)}().");
			}

			// We used to append the "masterconfig" grouptype info to the package's buildroot:
			// ie [BuildRoot]\[Masterconfig_GroupType]\...
			// However, we are having too many long path issues.  So we decided to make this
			// optional. One release of Framework, 6.04.00, had this commented out entirely
			// but this was affecting some users who were relying on the feature.

			if (project.Properties.GetBooleanPropertyOrDefault("eaconfig.includegroupinbuildroot", false))
			{
				MasterConfig.GroupType group = GetMasterGroup(project, packageName);
				string groupBuildRoot = group?.BuildRoot ?? group?.Name;
				if (groupBuildRoot != null)
				{
					return Path.Combine(BuildRoot, groupBuildRoot);
				}
			}
			return BuildRoot;
		}

		[Obsolete("Deprecated. This is an old overload that may be removed in future. By convention "
			+ nameof(PackageMap) + " APIs take a " + nameof(Project) + " as first argument. Use that overload and the .Name member of the returned object.")]
		public string GetMasterGroup(string packageName, Project project)
		{
			return GetMasterGroup(project, packageName).Name;
		}

		// conviennce function when just need the group
		public MasterConfig.GroupType GetMasterGroup(Project project, string packageName)
		{
			if (TryGetRootMasterPackage(packageName, out MasterConfig.Package masterPackage))
			{
				return masterPackage.GroupType.EvaluateGroupType(project);
			}
			return null;
		}

		// more readable convenience name, but also a slight lie - packages like Framework, eaconfig, VisualStudio don't need masterconfig versions
		// but will still return true, as well as auto versions. Really we're asking if *PackageMap* has version data for this package but for
		// most user of this api they conceputally want to know if masterconfig has an entry and anything else is invisible details
		public bool MasterConfigHasPackage(string packageName)
		{
			return TryGetRootMasterPackage(packageName, out _);
		}

		internal bool MasterConfigHasNugetPackage(string packageName)
		{
			return MasterConfig.NuGetPackages.ContainsKey(packageName);
		}

		#region Implementation

		public readonly MasterConfig MasterConfig;
		public readonly Project GlobalInitProject;

		public string MasterConfigFilePath
		{
			get
			{
				return MasterConfig == null ? String.Empty : MasterConfig.MasterconfigFile;
			}
		}

		public static bool IsInitialized
		{
			get { return (s_instance != null); }
		}

		private Release GetMetaPackageRelease(Project project, string packageName)
		{
			try
			{
				return FindOrInstallCurrentRelease(project, packageName);
			}
			catch (Exception e)
			{
				m_unresolvedMetaPackages.Add(packageName);
				project.Log.ThrowWarning
				(
					Log.WarningId.MetaPackageNotFound, Log.WarnLevel.Advise,
					"Failed to load metapackage '" + packageName + "'. " +
					"This may result in Framework being unable to find versions for other packages. " +
					"Reason: " + e.Message
				);
				return null;
			}
		}

		private PackageMap(Project project, MasterConfig masterConfig = null, bool autoBuildAll = false, string autoBuildPackages = null, string autoBuildGroups = null, bool useV1 = false, string token = null)
		{
			MasterConfig = masterConfig ?? MasterConfig.UndefinedMasterconfig;

			m_autoBuildAll = autoBuildAll;
			m_autoBuildPackages = new HashSet<string>(autoBuildPackages.TrimQuotes().ToArray(), StringComparer.OrdinalIgnoreCase);
			m_autoBuildGroups = new HashSet<string>(autoBuildGroups.TrimQuotes().ToArray(), StringComparer.OrdinalIgnoreCase);

			m_useV1 = useV1;
			m_packageServerToken = token;

			GlobalInitProject = project;

			// following block has almost nothing to do with packagemap - this is to do with masterconfig
			// and project / log global state. It's done here however to reduce the amount of 
			// system initialization calls you have to remember to do when creating a project and 
			// package map context and so that we can resolve the ondemand and use top level version
			// properties if they are set in masterconfig
			foreach (MasterConfig.GlobalProperty prop in MasterConfig.GlobalProperties)
			{
				Project.GlobalProperties.Add(prop.Name, prop.Value, prop.Condition);
			}

			// check compatbility for Framework
			Release frameworkRelease = null;
			if (IsExecutingFramework())
			{
				frameworkRelease = GetFrameworkRelease();
				CheckCompatibility(project.Log, frameworkRelease, null);
			}

			// setup deprecated config packages, these packages previously where separate from Framework but are now included. Their existence as separate packages is "faked"\
			// for backwards compatibility
			{
				// for config packages, take version from framework if executing Framework, otherwise use version of FW we were compiled against
				string defaultEaConfigVerison = frameworkRelease?.Version ?? Project.NantVersion;
				m_deprecatedPackages.Add("eaconfig", new DeprecatedPackage(new MasterConfig.Package(MasterConfig.MasterGroup, "eaconfig", defaultEaConfigVerison), " The version used in the masterconfig will be ignored and Framework's version will be used instead."));
				m_deprecatedPackages.Add("android_config", new DeprecatedPackage(new MasterConfig.Package(MasterConfig.MasterGroup, "android_config", defaultEaConfigVerison), " The version used in the masterconfig will be ignored and Framework's version will be used instead."));
			}

			// most of the code block is normally done in Project constructor but for the project used to 
			// init the package map it can't be done until the above block is done
			{
				foreach (Project.GlobalPropertyDef propdef in Project.GlobalProperties.EvaluateExceptions(project))
				{
					if (propdef.InitialValue != null && !project.Properties.Contains(propdef.Name)) // don't overrite propeties already set (such as from command line)
					{
						project.Properties.Add(propdef.Name, propdef.InitialValue);
					}
				}

				// thee ondemand property can be set by above loop and is used in PostPackageMapInit function below - have resolve here
				OnDemand = project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_ONDEMAND, MasterConfig.OnDemand);
				PackageServerOnDemand = project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_PACKAGESERVERONDEMAND, MasterConfig.PackageServerOnDemand);
				LocalOnDemand = MasterConfig.LocalOnDemand;
			}

			SetPackageRoots(project);

			MasterConfig.LoadMetaPackages(project, (_project, _packageName) => GetMetaPackageRelease(_project, _packageName)?.Path);

			MasterPackages = new List<string>(MasterConfig.Packages.Keys);

			// setup deprecated package for VisualStudio, do this after global property init though in case we want to take vsversion from
			// masterconfig based on exceptions
			{
				// for visual studio, we have to jump through a few hoops to make this backwards compatible - first base it off vsverison
				// but then also check visual studio minimum version and take whichever is larger, this should cover all cases where
				// VisualStudio package was used for version checking (instead of config-vs-version, which is the correct way)
				string vsYear = project.Properties.GetPropertyOrDefault("vsversion." + project.Properties["config-system"], project.Properties.GetPropertyOrDefault("vsversion", "2019"));
				string visualStudioVersion = null;
				switch (vsYear)
				{
					case "2017":
						visualStudioVersion = "15.7.27703.2042";
						break;
					case "2019":
						visualStudioVersion = "16.0";
						break;
					default:
						throw new BuildException($"Unrecognised value for property 'vsversion' - '{vsYear}'. Expected a year value (e.g '2017') no greater than 2019.");
				}
				string visualStudioMinVersion = project.Properties["package.VisualStudio.MinimumVersion"];
				if (visualStudioMinVersion != null && (visualStudioMinVersion.StrCompareVersions(visualStudioVersion) > 0))
				{
					visualStudioVersion = visualStudioMinVersion;
				}
				m_deprecatedPackages.Add("VisualStudio", new DeprecatedPackage(new MasterConfig.Package(MasterConfig.MasterGroup, "VisualStudio", visualStudioVersion), " The version used in the masterconfig will be ignored and based of 'vsversion' and 'package.VisualStudio.MinimumVersion' properties."));
			}

			// check if deprecated packages are listed in masterconfig
			{
				foreach (KeyValuePair<string, DeprecatedPackage> deprecatedPackage in m_deprecatedPackages)
				{
					foreach (KeyValuePair<string, MasterConfig.Package> package in MasterConfig.Packages)
					{
						if (package.Key.Equals(deprecatedPackage.Key, StringComparison.OrdinalIgnoreCase))
						{
							project.Log.ThrowWarning(Log.WarningId.DeprecatedPackage, Log.WarnLevel.Normal,
								"Package '{0}' is deprecated and is now included as part of Framework. " +
								"You can remove this package from your masterconfig.{1}", deprecatedPackage.Key, deprecatedPackage.Value.AdditionalDeprecationInformation);
						}
					}
				}
			}

			// init nuget manager (lazy, because we don't want to init until we have the buildroot set)
			Dictionary<string, string> netFrameworkPinnedVerisons = MasterConfig.NuGetPackages.Concat(MasterConfig.NetFrameworkNuGetPackages)
			  .GroupBy(kvp => kvp.Key, kvp => kvp.Value)
			  .ToDictionary(g => g.Key, g => g.Last()).Values
			  .ToDictionary(nugetPackage => nugetPackage.Name, nugetPackage => nugetPackage.Version);
			Dictionary<string, string> netCorePinnedVerisons = MasterConfig.NuGetPackages.Concat(MasterConfig.NetCoreNuGetPackages)
			  .GroupBy(kvp => kvp.Key, kvp => kvp.Value)
			  .ToDictionary(g => g.Key, g => g.Last()).Values
			  .ToDictionary(nugetPackage => nugetPackage.Name, nugetPackage => nugetPackage.Version);
			Dictionary<string, string> netStandardPinnedVerisons = MasterConfig.NuGetPackages.Concat(MasterConfig.NetStandardNuGetPackages)
			  .GroupBy(kvp => kvp.Key, kvp => kvp.Value)
			  .ToDictionary(g => g.Key, g => g.Last()).Values
			  .ToDictionary(nugetPackage => nugetPackage.Name, nugetPackage => nugetPackage.Version);
			m_nugetManager = new Lazy<NugetManager>
			(
				() => new NugetManager
				(
					Path.Combine(PackageRoots.OnDemandRoot.FullName, "Nuget"),
					Path.Combine(BuildRoot, "framework_tmp"),
					MasterConfig.NuGetSources,
					netFrameworkPinnedVerisons,
					netCorePinnedVerisons,
					netStandardPinnedVerisons
				)
			);
		}

		// gets the build file path from a package name using the masterconfig
		// this is needed for -buildpackage from command line or buildpackage="" from
		// <nanttask>
		// DAVE-FUTURE-REFACTOR-TODO: would love to invert this question and get
		// a package from a buildfile (this could be done by combining this function, 
		// PackageTask release verifcation, and CreateReleaseFromBuildFile)
		public string GetPackageBuildFileName(Project project, string packageName)
		{
			Release release = FindOrInstallCurrentRelease(project, packageName);
			string buildFilePath = Path.Combine(release.Path, release.Name + ".build");
			if (PathUtil.TryGetFilePathFileNameInsensitive(buildFilePath, out string existingPath))
			{
				buildFilePath = existingPath;
			}
			return buildFilePath;
		}

		public void InitConfigPackage(Project project)
		{
			if (!m_configPackageInitialized)
			{
				MasterConfig.IPackage currentPackageSpec = GetMasterPackage(project, ConfigPackageName);
				ConfigPackageVersion = GetMasterPackage(project, ConfigPackageName).Version;

				if (String.IsNullOrEmpty(ConfigPackageVersion))
				{
					throw new BuildException("Cannot find master config version for config package '" + ConfigPackageName + "'");
				}

				Release releaseInfo = FindOrInstallRelease(project, currentPackageSpec);

				ConfigPackageDir = releaseInfo.Path;
				ConfigDir = Path.Combine(releaseInfo.Path, "config");

				CheckCompatibility
				(
					project.Log,
					releaseInfo,
					pkg => (pkg == "Framework" && IsExecutingFramework()) ? GetFrameworkRelease() : null // we're very
				);

				m_configPackageInitialized = true;
			}
		}

		public void SetBuildRoot(Project project, string buildRootOverride = "")
		{
			if (BuildRoot != null)
			{
				if (buildRootOverride != String.Empty)
				{
					throw new BuildException("PackageMap build root was already initialized!");
				}

				// if no override was set and BuildRoot was already initialized then this is a lazy init
				// and does nothing
				// DAVE-FUTURE-REFACTOR-TODO messy, should redo this to have packagemap set buildroot to default value
				// at init time but then allow it to be overridden once and once only
			}
			else
			{
				if (MasterConfig.BuildRoot != null)
				{
					if (MasterConfig.BuildRoot.AllowDefault == false && buildRootOverride == String.Empty)
					{
						throw new BuildException("Error: Masterconfig has set <buildroot allow-default='false'> but no buildroot has been provided with the -buildroot: command line argument.");
					}

					MasterConfig.BuildRoot.Path = MasterConfig.BuildRoot.Exceptions.ResolveException(project, MasterConfig.BuildRoot.Path);
				}

				if (!String.IsNullOrEmpty(buildRootOverride))
				{
					BuildRoot = Path.GetFullPath(buildRootOverride);
				}
				else
				{
					string rootBase = String.IsNullOrWhiteSpace(MasterConfig.MasterconfigFile) ?
						Path.GetFullPath(project.BaseDirectory) :
						Path.GetDirectoryName(Path.GetFullPath(MasterConfig.MasterconfigFile));

					BuildRoot = Path.GetFullPath(Path.Combine(rootBase, MasterConfig.BuildRoot.Path ?? "build"));
				}

			}
		}

		// to 100% setup packagemap these 3 things that need to happen, but you may not need all of them so it's broken up into stages // DAVE-FUTURE-REFACTOR-TODO - still a lot of specific 
		// calls that have to be remembered, would be nice to simplify
		//		1 - PackageMap.Init - sets the masterconfig, required if we want to resolve versions
		//		2 - PackageMap.Instance.SetBuildRoot - either takes the buildroot from masterconfig or an override, required if buildroot needs to be resolved // DAVE-FUTURE-REFACTOR-TODO - 
		//			check .BuildRoot isn't accessed until this is done
		//		3 - PackageMap.Instance.SetTopLevelPackage - needed for implicit resolution of command line package, needs to happen before we resolve version but ONLY in cases where we have a package 
		//			we consider to be 'top level'
		public static void Init(Project project, MasterConfig masterConfig = null, bool autobuildAll = false, string autobuildPackages = null, string autobuildGroups = null, bool useV1 = false, string token = null)
		{
			if (s_instance != null)
			{
				throw new BuildException("PackageMap has already been initialized.");
			}
			else
			{
				s_instance = new PackageMap(project, masterConfig, autobuildAll, autobuildPackages, autobuildGroups, useV1, token);
				project.PostPackageMapInit();

				// the project used to init the package won't have had a chance to init configuration props (because it needs package map
				// to resolve configuration packages) so explicitly initialize them here - note these properties are only initialize
				// when project as a build file (see Project constructor that takes a buildfile)
				if (project.CurrentScriptFile != null)
				{
					ConfigPackageLoader.InitializeInitialConfigurationProperties(project);
				}

				foreach (KeyValuePair<Log.WarningId, MasterConfig.WarningDefinition> warningDefinition in s_instance.MasterConfig.Warnings)
				{
					if (warningDefinition.Value.Enabled.HasValue)
					{
						Log.SetGlobalWarningEnabledIfNotSet(project.Log, warningDefinition.Key, warningDefinition.Value.Enabled.Value, Log.SettingSource.MasterConfigGlobal);
					}
					if (warningDefinition.Value.AsError.HasValue)
					{
						Log.SetGlobalWarningAsErrorIfNotSet(project.Log, warningDefinition.Key, warningDefinition.Value.AsError.Value, Log.SettingSource.MasterConfigGlobal);
					}
				}

				foreach (KeyValuePair<Log.DeprecationId, MasterConfig.DeprecationDefinition> deprecationDefinition in s_instance.MasterConfig.Deprecations)
				{
					Log.SetGlobalDeprecationEnabledIfNotSet(project.Log, deprecationDefinition.Key, deprecationDefinition.Value.Enabled, Log.SettingSource.MasterConfigGlobal);
				}

				project.Log.ApplyLegacyProjectSuppressionProperties(project); // update warning settings from legacy properties now we have masterconfig properties
			}
		}

		// for testing only this - doesn't exactly match Init since it resets all warnings and global properties not just the ones from masterconfig - it is assumed tests
		// don't need to preserve setting from other sources
		internal static void Reset()
		{
			if (s_instance != null)
			{
				s_instance.m_packageReleaseCache.Clear();
			}
			s_instance = null;
			Log.ResetGlobalWarningStates();
			Project.ResetGlobalProperties();
		}

		private Release GetReleaseFromPackageServer(Project project, MasterConfig.IPackage packageSpec, IPackageServer packageServer, bool isLocalOnDemand)
		{
			Uri packageUri = null;
			if (packageSpec.AdditionalAttributes.TryGetValue("uri", out string uriString))
			{
				packageUri = new Uri(uriString);
			}

			string installDirectory;
			if (isLocalOnDemand)
			{
				if (!PackageRoots.HasLocalOnDemandRoot)
				{
					throw new BuildException(string.Format("Trying to perform ondemand install for package '{0}' which has set 'localondemand=true' in the masterconfig but no <localondemandroot> has been specified in the masterconfig.", packageSpec.Name));
				}
				if (packageUri != null && packageUri.AbsolutePath.TrimStart('/').StartsWith("SDK/") && !packageSpec.Version.ToLowerInvariant().Contains("dev"))
				{
					project.Log.ThrowWarning(Log.WarningId.LocalOndemandIncorrect, Log.WarnLevel.Minimal,
						"localondemand is set to true for package '{0}' but the URI indicates that this is synced from the SDK depot. SDK packages should not use localondemand so that they are not synced to multiple locations on a users machine because SDK packages are both large and immutable.", packageSpec.Name);
				}
				installDirectory = PackageRoots.LocalOnDemandRoot.FullName;
			}
			else
			{
				if (!PackageRoots.HasOnDemandRoot)
				{
					throw new BuildException("Trying to perform on demand install but no ondemandroot has been set! This should be set by masterconfig or via environment variable FRAMEWORK_ONDEMAND_ROOT.");
				}
				if (packageUri != null && PackageRoots.HasLocalOnDemandRoot && packageSpec.Version.ToLowerInvariant().Contains("dev"))
				{
					project.Log.ThrowWarning(Log.WarningId.LocalOndemandIncorrect, Log.WarnLevel.Minimal,
						"localondemand is set to false for package '{0}', but the version name '{1}' suggests that this is a development version. localondemand should be set to true for all development versions to ensure that local changes are not clobbered when switching between streams.", packageSpec.Name, packageSpec.Version);
				}
				installDirectory = PackageRoots.OnDemandRoot.FullName;
			}

			return packageServer.Install(packageSpec, installDirectory);
		}

		private bool UpdateReleaseFromPackageServer(Project project, Release release, Uri ondemandUri, bool localOndemand, IPackageServer packageServer)
		{
			packageServer.UpdateProgress += new InstallProgressEventHandler(UpdateInstallProgress);

			project.Log.Debug.WriteLine("Ensuring package '{0}-{1}' up-to-date using the protocol package server.", release.Name, release.Version);

			string defaultInstallDirectory = PackageRoots.OnDemandRoot.FullName;
			if (localOndemand)
			{
				defaultInstallDirectory = PackageRoots.LocalOnDemandRoot.FullName;
			}

			packageServer.Update(release, ondemandUri, defaultInstallDirectory);
			return true;
		}

		public void OverrideOnDemandRoot(string onDemandRootOverride)
		{
			PackageRoots.OverrideOnDemandRoot(new PathString(onDemandRootOverride));
		}

		public bool IsPackageAutoBuildClean(Project project, string packageName)
		{
			// auto build all override
			if (m_autoBuildAll)
			{
				return true;
			}

			// specific package override
			if (m_autoBuildPackages.Contains(packageName))
			{
				return true;
			}

			// master group override
			MasterConfig.GroupType group = GetMasterGroup(project, packageName);
			if (group != null && m_autoBuildGroups.Contains(group.Name))
			{
				return true;
			}

			// no overrides were set, check package, then group, then default to true
			if (TryGetMasterPackage(project, packageName, out MasterConfig.IPackage packageSpec))
			{
				return packageSpec.AutoBuildClean ?? group?.AutoBuildClean ?? true;
			}
			return false;
		}

		public Release FindOrInstallRelease(Project project, MasterConfig.IPackage package, bool? allowOnDemand = null)
		{
			// check release cache first
			bool onDemandIsEnabled = allowOnDemand ?? OnDemand;
			if (m_packageReleaseCache.TryGetValue(package, out ReleaseCacheResult releaseCacheResult) && releaseCacheResult.TryGetCachedResult(onDemandIsEnabled, out Release cachedRelease))
			{
				// TryGetCacheResult will throw an exception if we previously failed to find or install release, we reach here then we have a valid release to return
				return cachedRelease;
			}

			using (ProcessUtil.WindowsNamedMutexWrapper packageSearchInstallMutex = CreatePackageInstallMutex(package)) // this servers as both multiprocess and in process safety for this block
			{
				if (!packageSearchInstallMutex.HasOwnership)
				{
					packageSearchInstallMutex.WaitOne();
				}

				// no cache entry perform package search (and optionally install if not found - cache success or failure)
				try
				{
					Release foundOrInstalledRelease = FindOrInstallReleaseInternal(project, package, onDemandIsEnabled);
					m_packageReleaseCache[package] = new ReleaseCacheResult(foundOrInstalledRelease);
					return foundOrInstalledRelease;
				}
				catch (Exception e)
				{
					ReleaseCacheResult capturedFailResult = new ReleaseCacheResult(ExceptionDispatchInfo.Capture(e), onDemandIsEnabled);
					m_packageReleaseCache[package] = capturedFailResult;
					throw;
				}
			}
		}

		// this should only be called from FindOrInstallRelease - it is only factored into a separate function for readability
		private Release FindOrInstallReleaseInternal(Project project, MasterConfig.IPackage package, bool onDemandIsEnabled)
		{
			List<RejectedRelease> rejectedReleases = new List<RejectedRelease>();

			Release installed = FindInstalledReleaseInternal(package, ref rejectedReleases);
			if (installed != null)
			{
				// if this is a uri package and ondemand is enabled we allow an "update" from the uri
				// this is for:
				// p4 uri - check we have the right cl sync'd
				// nuget uri - allow automatic dependency version setting (so user doesn't have to explicit about version for whole nuget dependency chain)
				if (onDemandIsEnabled && !installed.UpToDate) //note: using uptodate check in this way relies on caching results of find installed
				{
					UpdateOnDemandRelease(project, installed, package);
					installed.UpToDate = true;
				}

				return installed;
			}

			bool okToDownload = onDemandIsEnabled;
			if (!package.AdditionalAttributes.TryGetValue("uri", out string uri) && !PackageServerOnDemand)
				okToDownload = false;

			if (onDemandIsEnabled && okToDownload)
			{
				return InstallOnDemandRelease(project, package, rejectedReleases); // this will fail with missing package exception with additional details about remote server
			}
			else
			{
				string message = $"Could not find package '{package.Name}-{package.Version}' in package roots." + Environment.NewLine
								+ "Please note that on demand package downloads are " + (onDemandIsEnabled ? "on " : "off ") 
								+ "and specifically package server on demand downloads are " + (PackageServerOnDemand ? "on." : "off.") + Environment.NewLine
								+ "If you are expecting the package to be downloaded you may want to check your on demand package download settings.";
				return ThrowMissingPackageException(package, message, rejectedReleases: rejectedReleases);
			}
		}

		// TODO does function really belong in package map?
		public static Release CreateReleaseFromBuildFile(Project project, string buildFilePath)
		{
			string buildFilePackageName = Path.GetFileNameWithoutExtension(buildFilePath);
			string buildFileDirectory = Path.GetDirectoryName(buildFilePath);

			Manifest manifest = Manifest.Load(buildFileDirectory, project.Log); // returns "default" manifest if manifest.xml does not exist with an empty version string

			// test for flattened case first
			string buildFileDirectoryName = Path.GetFileName(buildFileDirectory);
			if (buildFilePackageName.Equals(buildFileDirectoryName, StringComparison.OrdinalIgnoreCase))
			{
				// we've found Package/Package.build, assume this is flattnened package
				string version = !manifest.Version.IsNullOrEmpty() ? manifest.Version : Release.Flattened;
				return new Release(new PackageSpec(buildFilePackageName, version), buildFileDirectory, isFlattened: true, manifest: manifest);
			}

			// test for version folder case
			string parentDirectoryName = Path.GetFileName(Path.GetDirectoryName(buildFileDirectory));
			if (buildFilePackageName.Equals(parentDirectoryName, StringComparison.OrdinalIgnoreCase))
			{
				string version = buildFileDirectoryName; // ALWAYS take a version folder is one exists, even if it doesn't agree with manifest version

				// check version folder matches manifest.xml, case insensitively - if it does match take the manifest version just in case it has correct casing vs file system
				if (!manifest.Version.IsNullOrEmpty())
				{
					if (version.Equals(manifest.Version, StringComparison.OrdinalIgnoreCase))
					{
						version = manifest.Version;
					}
					else
					{
						project.Log.ThrowWarning(Log.WarningId.InconsistentVersionName, Log.WarnLevel.Minimal, $"Package manifest '{Path.Combine(buildFileDirectory, "Manifest.xml")}' <versionName> ({manifest.Version}) does not match package folder version ({version}). Package folder version will be used instead.");
					}
				}

				return new Release(new PackageSpec(buildFilePackageName, version), buildFileDirectory, isFlattened: false, manifest: manifest);
			}

			// finally, allow a release to be create even if the folder is wrong, allow top level release to always be accepted
			// even if it doesn't match
			{
				string version = !manifest.Version.IsNullOrEmpty() ? manifest.Version : Release.Flattened;
				return new Release(new PackageSpec(buildFilePackageName, version), buildFileDirectory, isFlattened: true, manifest: manifest);
			}
		}

		// returns the package entry from the masterconfig, but DOES NOT evaluate any exceptions - use with care
		private bool TryGetRootMasterPackage(string packageName, out MasterConfig.Package masterPackage)
		{
			masterPackage = null;

			if (String.IsNullOrEmpty(packageName))
			{
				return false;
			}

			// do name check case insensitively and use the name returned from the dictionary, this gives the correct casing in the very unlikely
			// event package name is being searched for using incorrect case
			string deprecatedPackageName = m_deprecatedPackages.Keys.FirstOrDefault(deprecated => deprecated.Equals(packageName, StringComparison.OrdinalIgnoreCase));
			if (deprecatedPackageName != null)
			{
				masterPackage = m_deprecatedPackages[deprecatedPackageName].MasterPackage;
				return true;
			}

			// attempt to locate an exact case match.
			IDictionary<string, MasterConfig.Package>[] packageLists = new IDictionary<string, MasterConfig.Package>[] { MasterConfig.Packages, AutomaticPackageVersions };
			foreach (IDictionary<string, MasterConfig.Package> dictionary in packageLists)
			{
				if (dictionary.TryGetValue(packageName, out masterPackage))
				{
					return true;
				}
			}

			// otherwise perform a case insensitive match
			bool foundDuplicates = false;
			foreach (IDictionary<string, MasterConfig.Package> dictionary in packageLists)
			{
				foreach (KeyValuePair<string, MasterConfig.Package> nameToPackage in dictionary)
				{
					if (nameToPackage.Key.Equals(packageName, StringComparison.OrdinalIgnoreCase))
					{
						if (masterPackage == null)
						{
							masterPackage = nameToPackage.Value;
						}
						else
						{
							masterPackage = null;
							foundDuplicates = true;
						}
					}
				}
			}

			if (foundDuplicates)
			{
				throw new BuildException(string.Format("Package '{0}' has multiple duplicates in the masterconfig. Please remove duplicate entries.", packageName));
			}

			return masterPackage != null;
		}

		private Release ThrowMissingPackageException(MasterConfig.IPackage packageSpec, string errorMessageHeader, Exception innerException = null, List<RejectedRelease> rejectedReleases = null, IPackageServer packageServer = null, bool listRemoteReleases = false)
		{
			StringBuilder exceptionMessageBuilder = new StringBuilder(errorMessageHeader);
			exceptionMessageBuilder.AppendLine();

			bool searchedPackageRoots = PackageRoots.Any();
			bool searchedRemoteServer = packageServer != null;

			// if both package server and package roots we're searched, give context as why package server was used
			if (searchedPackageRoots && searchedRemoteServer)
			{
				exceptionMessageBuilder.AppendLine($"Attempt to acquire package '{packageSpec.Name}-{packageSpec.Version}' from remote server was made because package could not be found with in package roots.");
			}

			// inform user where we search locally and why we rejected and found releases
			if (searchedPackageRoots)
			{
				// package roots enumeration
				exceptionMessageBuilder.AppendLine("Package roots searched:");
				foreach (PackageRootList.Root root in PackageRoots)
				{
					if (root.Type == PackageRootList.RootType.DevelopmentRoot)
					{
						exceptionMessageBuilder.AppendLine($"\t\t{root.Dir}    (this is the development root, packages will only be found in this root if they have the 'switched-from-version' attribute in the masterconfig)");
					}
					else if (root.Type == PackageRootList.RootType.LocalOnDemandRoot)
					{
						exceptionMessageBuilder.AppendLine($"\t\t{root.Dir}    (this is the local ondemand root, packages will only be found in this root if they have the 'localondemand=\"true\"' (masterconfig is setting '{PackageRoots.UseLocalOnDemandRootAsDefault.ToString().ToLowerInvariant()}' as default value if unspecified) attribute in the masterconfig)");
					}
					else if (root.Type == PackageRootList.RootType.OnDemandRoot)
					{
						exceptionMessageBuilder.AppendLine($"\t\t{root.Dir}    (this is ondemand root, if packages are marked 'localondemand=\"false\"' (masterconfig is setting '{PackageRoots.UseLocalOnDemandRootAsDefault.ToString().ToLowerInvariant()}' as default value if unspecified) they will be downloaded here if masterconfig permits)");
					}
					else
					{
						exceptionMessageBuilder.AppendLine($"\t\t{root.Dir}");
					}
				}

				// list releases we found for this package that didn't match version or were rejected for another reason
				if (rejectedReleases.Any())
				{
					exceptionMessageBuilder.AppendLine("The following versions were found in your package roots:");
					foreach (RejectedRelease rejectedRelease in rejectedReleases)
					{
						string rejectionReasonFormatted = !String.IsNullOrWhiteSpace(rejectedRelease.Reason) ? " (" + rejectedRelease.Reason + ")" : String.Empty; // no reason implies wrong version, which requires no explanation
						exceptionMessageBuilder.AppendLine($"\t\t{rejectedRelease.Release.Name}-{rejectedRelease.Release.Version} Path: {rejectedRelease.Release.Path}{rejectionReasonFormatted}");
					}
				}
			}

			// inform user of versions found on remote server, if we failed to find remote versions (only web server can list remote versions)
			if (searchedRemoteServer && listRemoteReleases)
			{
				// wrap in try, catch we don't want package server failure throwing during error handling
				bool retrievedReleases = false;
				IList<MasterConfig.IPackage> webServerReleases = null;
				try
				{
					retrievedReleases = packageServer.TryGetPackageReleases(packageSpec.Name, out webServerReleases);
				}
				catch { }

				if (retrievedReleases && webServerReleases.Any())
				{
					List<MasterConfig.IPackage> sorted = new List<MasterConfig.IPackage>(webServerReleases);
					sorted.Sort((x, y) => x.Version.CompareTo(y.Version));

					exceptionMessageBuilder.AppendLine("The following versions were found on the package server:");

					foreach (MasterConfig.IPackage webServerRelease in sorted)
					{
						exceptionMessageBuilder.AppendLine($"\t\t{webServerRelease.Name}-{webServerRelease.Version.PadRight(25)}");
					}
				}
			}

			throw new BuildException(exceptionMessageBuilder.ToString(), innerException);
		}

		private Release InstallOnDemandRelease(Project project, MasterConfig.IPackage packageSpec, List<RejectedRelease> rejectedReleases = null)
		{
			IPackageServer packageServer = null;
			try
			{
				bool canBeLocalOnDemand = (packageSpec.LocalOnDemand ?? PackageRoots.UseLocalOnDemandRootAsDefault) && LocalOnDemand; // true if can be found or installed to local ondemand root

				// DAVE-FUTURE-REFACTOR-TODO: might be more logical in this re-work to have the protocol based on uri selection happen here, rather than having a "protocol server" that
				// essentially wraps a another set of package server implementations
				if (packageSpec.AdditionalAttributes.TryGetValue("uri", out string uri))
				{
					packageServer = PackageServerFactory.CreateProtocolPackageServer(project);
				}
				else
				{
					packageServer = PackageServerFactory.CreateWebPackageServer(project, m_useV1, m_packageServerToken);
				}
				return GetReleaseFromPackageServer(project, packageSpec, packageServer, canBeLocalOnDemand);
			}
			catch (Exception e)
			{
				if (e.Message == "The request failed with HTTP status 401: Unauthorized.")
				{
					return ThrowMissingPackageException(packageSpec, CMProperties.PackageServerAuthenticationError, e, rejectedReleases, packageServer);
				}
				else if (e.Message == "The remote server returned an error: (404) Not Found.")
				{
					// special case for web server not being able to find requested package
					return ThrowMissingPackageException(packageSpec, $"Could not find package '{packageSpec.Name}-{packageSpec.Version}' on the selected server.", e, rejectedReleases, packageServer, listRemoteReleases: true);
				}
				else
				{
					return ThrowMissingPackageException(packageSpec, $"Failed to download package '{packageSpec.Name}-{packageSpec.Version}' from the selected server. {e.Message}", e, rejectedReleases, packageServer);
				}
			}
		}

		private void UpdateOnDemandRelease(Project project, Release installed, MasterConfig.IPackage packageSpec)
		{
			bool canBeLocalOnDemand = (packageSpec.LocalOnDemand ?? PackageRoots.UseLocalOnDemandRootAsDefault) && LocalOnDemand; // true if can be found or installed to local ondemand root
			if (packageSpec.AdditionalAttributes.TryGetValue("uri", out string uri))
			{
				IPackageServer protocolServer = PackageServerFactory.CreateProtocolPackageServer(project);
				if (IsOnDemandRelease(installed, packageSpec, protocolServer))
				{
					if (!UpdateReleaseFromPackageServer(project, installed, new Uri(uri), canBeLocalOnDemand, protocolServer))
					{
						throw new BuildException($"Failed to update the package from the URI provided: {uri}. If you wish to continue without checking if package is up-to-date please re-run the build with -D:check-ondemand-up-to-date=false.");
					}
				}
				else
				{
					DoOnce.Execute($"package-root-uri-{packageSpec.Name}-{packageSpec.Version}", () =>
					{
						project.Log.Info.WriteLine("Package {0} is inside your package roots at '{1}' but specifies an on-demand uri in your masterconfig. Framework will skip the on-demand fetch and use the version found in your package roots.", installed.Name, installed.Path);
					});
				}
			}

			if (PackageRoots.HasOnDemandRoot && !canBeLocalOnDemand) // TODO - this only called for updates right now, make it called no matter what
			{
				PackageCleanup.UpdateMetadataTime(project, installed, PackageRoots.OnDemandRoot.FullName);
			}
			else if (PackageRoots.HasLocalOnDemandRoot && canBeLocalOnDemand)
			{
				PackageCleanup.UpdateMetadataTime(project, installed, PackageRoots.LocalOnDemandRoot.FullName);
			}
		}

		private void UpdateInstallProgress(object sender, InstallProgressEventArgs e)
		{
			if (e != null)
			{
				IPackageServer packageServer = sender as IPackageServer;
				if (packageServer != null)
				{
					if (e.LocalProgress < 0)
					{
						packageServer.Project.Log.Minimal.WriteLine(e.Message);
					}
					else
					{
						packageServer.Project.Log.Debug.WriteLine("{0}. Progress - {1}%", e.Message, e.LocalProgress);
					}
				}
			}
		}

		private void SetPackageRoots(Project project)
		{
			// We will first add all the packageroots to _packageroots, and then we will 
			// add the packages underneath them. This ensures we know all the roots when 
			// we add the packages, and we won't add a root as a package by mistake.
			//	1) Add development root, but this root will only be used if the package is marked up in the masterconfig to say it is forked/workon'd
			//	2) Add local on demand root, but this root will only be used when special mark up in the masterconfig indicates to use a local ondemand version
			//	3) Add all roots in masterconfig
			//	4) Add on demand root, after normal package roots so that ondemand packages are only use if the package is not in the normal package roots

			// add development root before explicit roots
			string developmentRootPath = MasterConfig.GetDevelopmentRoot(project);
			if (!String.IsNullOrEmpty(developmentRootPath))
			{
				PathString developmentRoot = MakePathStringFromProperty(developmentRootPath, project);
				try
				{
					if (Directory.Exists(developmentRoot.Path) == false)
					{
						Directory.CreateDirectory(developmentRoot.Path);
					}

					PackageRoots.Add(developmentRoot, PackageRootList.RootType.DevelopmentRoot);
				}
				catch (Exception e)
				{
					project.Log.Warning.WriteLine("Package development root in masterconfig file could not be created: {0}", e.Message);
				}
			}

			string onDemandRootPath = MasterConfig.GetOnDemandRoot(project);
			PathString onDemandRoot = null;
			bool onDemandRootAdded = false;
			if (!string.IsNullOrEmpty(onDemandRootPath))
			{
				onDemandRoot = MakePathStringFromProperty(onDemandRootPath, project);
			}

			// add explict roots
			foreach (MasterConfig.PackageRoot root in MasterConfig.PackageRoots)
			{
				PathString path = MakePathStringFromProperty(root.EvaluatePackageRoot(project), project);
				if (Directory.Exists(path.Path) == false)
				{
					// TODO, this warning doesn't tell you which fragment the package root was defined in
					project.Log.ThrowWarning(Log.WarningId.PackageRootDoesNotExist, Log.WarnLevel.Normal, $"Package root given in masterconfig file does not exist: {path.Path}.");
					continue;
				}

				if (MasterConfig.OnDemand && onDemandRoot != null && path == onDemandRoot)
				{
					PackageRoots.Add(path, PackageRootList.RootType.OnDemandRoot);
					onDemandRootAdded = true;
				}
				else
				{
					PackageRoots.Add(path);
				}
			}

			// add local ondemand root before global ondemand root so the local versions take precendence
			MasterConfig.SpecialRootInfo localOndemandRootPath = MasterConfig.GetLocalOnDemandRoot(project);
			if (localOndemandRootPath != null)
			{
				PathString localOndemandRoot = MakePathStringFromProperty(localOndemandRootPath.Path, project);
				try
				{
					if (Directory.Exists(localOndemandRoot.Path) == false)
					{
						Directory.CreateDirectory(localOndemandRoot.Path);
					}
					string useLocalRootAsDefault = null;
					if (localOndemandRootPath.ExtraAttributes != null)
					{
						IEnumerable<KeyValuePair<string, string>> useAsDefault = localOndemandRootPath.ExtraAttributes.Where(kv => kv.Key == "use-as-default");
						if (useAsDefault.Any())
						{
							// There should be only one.
							if (!String.IsNullOrEmpty(useAsDefault.First().Value))
							{
								useLocalRootAsDefault = String.Format("use-as-default={0}", useAsDefault.First().Value);
							}
						}
					}
					PackageRoots.Add(localOndemandRoot, PackageRootList.RootType.LocalOnDemandRoot, useLocalRootAsDefault);
				}
				catch (Exception e)
				{
					project.Log.Warning.WriteLine("Local On-demand package download package root '{0}' in masterconfig file could not be created: {1}", localOndemandRoot.Path, e.Message);
				}
			}

			// add on demand root AFTER explicit roots
			if (onDemandRoot != null && !onDemandRootAdded)
			{
				try
				{
					if (Directory.Exists(onDemandRoot.Path) == false)
					{
						Directory.CreateDirectory(onDemandRoot.Path);
					}
					PackageRoots.Add(onDemandRoot, PackageRootList.RootType.OnDemandRoot);
				}
				catch (Exception e)
				{
					project.Log.Warning.WriteLine("On-demand package download package root '{0}' in masterconfig file could not be created: {1}", onDemandRoot.Path, e.Message);
				}
			}

			if (project.Log.Level >= Log.LogLevel.Detailed)
			{
				project.Log.Info.WriteLine("Package roots:");
				foreach (var root in PackageRoots)
				{
					project.Log.Info.WriteLine("    " + root.ToString());
				}
				project.Log.Info.WriteLine();
			}
		}

		// returns normalized full path.
		// Relative paths are computed against masterconfig location.
		private PathString MakePathStringFromProperty(string dir, Project project)
		{
			string path = project == null ? dir : project.ExpandProperties(dir);
			if (!String.IsNullOrEmpty(MasterConfigFilePath))
			{
				return PathString.MakeCombinedAndNormalized(Path.GetDirectoryName(Path.GetFullPath(MasterConfigFilePath)), path);
			}
			else if (Path.IsPathRooted(path))
			{
				return PathString.MakeNormalized(dir);
			}
			else
			{
				throw new BuildException("Cannot resolve relative path '{0}' without masterconfig file!");
			}
		}

		private bool IsOnDemandRelease(Release release, MasterConfig.IPackage packageSpec, IPackageServer packageServer) // TODO
		{
			bool isLocalOnDemandRelease = (packageSpec.LocalOnDemand ?? PackageRoots.UseLocalOnDemandRootAsDefault) && LocalOnDemand;

			string root = null;
			if (PackageRoots.HasOnDemandRoot && !isLocalOnDemandRelease)
			{
				root = PackageRoots.OnDemandRoot.FullName;
			}
			else if (PackageRoots.HasLocalOnDemandRoot && isLocalOnDemandRelease)
			{
				root = PackageRoots.LocalOnDemandRoot.FullName;
			}

			if (root != null)
			{
				return packageServer.IsPackageServerRelease(release, root);
			}
			return false; // if there is no ondemand root no release can be on demand
		}

		private static ProcessUtil.WindowsNamedMutexWrapper CreatePackageInstallMutex(MasterConfig.IPackage package)
		{
			// using name / version as the lock, this is inefficent if we have multiple instances trying to install same name + version to different
			// locations, but this is very rare and we care way more about correctness than efficiency for these cases
			return new ProcessUtil.WindowsNamedMutexWrapper($"/{package.Name}/{package.Version}".ToLowerInvariant());
		}

		#endregion		
		
		public IEnumerable<NugetPackage> GetNugetPackages(string frameworkMoniker, IEnumerable<string> packageNames) => m_nugetManager.Value.GetRequiredPackages(frameworkMoniker, packageNames);

		/* 
		#region Nuget Package Management

		// This call converts a set of nuget packages (from the masterconfig) into a resolved set of all the nuget references needed to support those packages
		// It will solve the cross dependencies globally for an entire masterconfig, but will only do this for a single framework type (e.g., netcoreapp31, net5)
		private PackageResolutionResult GetPackageResolution(string frameworkMoniker)
		{
			NuGet.Frameworks.NuGetFramework framework = NugetManager.GetFrameworkString(frameworkMoniker); // NUGET-TODO just take a NugetGetFramework
			Task<PackageResolutionResult> containingTask = null;
			lock (m_nugetResolutionMap)
			{
				string frameworkString = framework.GetFrameworkString();
				if (!m_nugetResolutionMap.TryGetValue(frameworkString, out containingTask))
				{
					GlobalInitProject.Log.Minimal.WriteLine("Resolving nuget graph for framework: {0}", framework.GetShortFolderName());

					// always add official nuget as last source // NUGET-TODO:?
					IEnumerable<string> adjustedNugetSources = MasterConfig.NuGetSources;
					if (!adjustedNugetSources.Contains("https://api.nuget.org/v3/index.json")) // NUGET-TODO: source comparison again
					{
						adjustedNugetSources = adjustedNugetSources.Concat(new string[] { "https://api.nuget.org/v3/index.json" });
					}

					// TEMP HACK:
					adjustedNugetSources = new string[] { "https://api.nuget.org/v3/index.json" };

					if (!PackageResolutionResult.TryLoadFromCacheFile(Path.Combine(Instance.BuildRoot, "framework_tmp"), framework, adjustedNugetSources, MasterConfig.NuGetPackages, out PackageResolutionResult resolution, out string cacheFilePath))
					{
						containingTask = NugetManager.ResolveNugetPackages(MasterConfig.NuGetPackages, framework, adjustedNugetSources, CancellationToken.None)
							.ContinueWith
							(
								(resolveTask) =>
								{
									PackageResolutionResult.WriteCacheFile(cacheFilePath, resolveTask.Result.GlobalPackageList);
									return resolveTask.Result;
								}
							);
					}
					else
					{
						throw new NotImplementedException();
					}
					m_nugetResolutionMap[frameworkString] = containingTask;
				}
			}
			return containingTask.Result;
		}

		// Once we have a globally resolved set of packages, we have to convert explicit nuget references into a set of all packages and dependent packages
		public IEnumerable<PackageContents> GetNugetContents(string frameworkMoniker, IEnumerable<string> nugetReferences)
		{
			// insert extra sources here
			PackageResolutionResult resolvedPackages = GetPackageResolution(frameworkMoniker);
			IEnumerable<NuGet.Packaging.Core.PackageIdentity> packages = GetResolvedPackageIdentities(nugetReferences, resolvedPackages);

			DirectoryInfo nugetDir = PackageRoots.OnDemandRoot.CreateSubdirectory("nuget");
			lock (resolvedPackages.PackageContents) // NUGET-TODO: locking member of another class - move this logic into class
			{
				foreach (NuGet.Packaging.Core.PackageIdentity identity in packages)
				{
					if (!resolvedPackages.PackageContents.TryGetValue(identity, out Task<PackageContents> contentsDownloadTask))
					{
						PackageResolutionResult.PackageIdentityWithSource package = resolvedPackages.GlobalPackageList.First(sourceInfo => sourceInfo.Equals(identity)); // NUGET-TODO: convert to lookup?
						resolvedPackages.PackageContents[identity] = NugetManager.BeginAsyncDownload(package, nugetDir, NugetManager.GetFrameworkString(frameworkMoniker), CancellationToken.None);
					}
				}
			}

			foreach (NuGet.Packaging.Core.PackageIdentity identity in packages)
			{
				yield return resolvedPackages.PackageContents[identity].Result;
			}
		}

		private IEnumerable<NuGet.Packaging.Core.PackageIdentity> GetResolvedPackageIdentities(IEnumerable<string> nugetReferences, PackageResolutionResult resolvedResult)
		{
			HashSet<NuGet.Packaging.Core.PackageIdentity> flattenedPackageList = new HashSet<NuGet.Packaging.Core.PackageIdentity>(NuGet.Packaging.Core.PackageIdentity.Comparer);
			foreach (string package in nugetReferences)
			{
				if (resolvedResult.DependenciesForPackage.TryGetValue(package, out List<NuGet.Packaging.Core.PackageIdentity> deps))
				{
					foreach (NuGet.Packaging.Core.PackageIdentity identity in deps)
					{
						flattenedPackageList.Add(identity);
					}
				}
				else
				{
					if (resolvedResult.NotFoundPackages.Any(x => package.Equals(x.Item1, StringComparison.OrdinalIgnoreCase)))
					{
						throw new NotImplementedException(); // NUGET-TODO unable to find package blah, but we did look
					}
					else
					{
						throw new UnresolvedNugetPackageException(package);
					}
				}
			}
			return flattenedPackageList;
		}

		#endregion*/

		#region Compatibility verification functionality
		private void VerifyCompatibilityData(Log log, CompatibilityData compatibilityData, Release release)
		{
			if (compatibilityData != null && (1 == Interlocked.Increment(ref compatibilityData.Verified)))
			{
				string releaseVersion = !String.IsNullOrWhiteSpace(release.Manifest.Version) ? release.Manifest.Version : release.Version;
				if (compatibilityData.MinCondition != null && compatibilityData.MinCondition.revision != null)
				{
					if (release.Manifest.Compatibility != null && release.Manifest.Compatibility.Revision != null)
					{
						if (0 < compatibilityData.MinCondition.revision.StrCompareVersions(release.Manifest.Compatibility.Revision))
						{
							string msg = String.Format("Package '{0}' requires minimal revision '{1}' of package '{2}', but package '{2}-{3}' ({4}) has revision '{5}' {6}",
								compatibilityData.MinCondition.ConditionSource.Name, compatibilityData.MinCondition.revision, release.Name, release.Version, release.Path, release.Manifest.Compatibility.Revision,
								String.IsNullOrWhiteSpace(compatibilityData.MinCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MinCondition.Message);

							if (compatibilityData.MinCondition.fail)
							{
								throw new BuildException(msg);
							}
							else
							{
								log.ThrowWarning(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, msg);
							}
						}
					}
					else
					{
						string msg = String.Format("Package '{0}' requires minimal revision '{1}' of package '{2}', but Manifest.xml file of package '{2}-{3}' ({4}) does not declare revision number. Can not check compatibility. {5}",
							compatibilityData.MinCondition.ConditionSource.Name, compatibilityData.MinCondition.revision, release.Name, release.Version, release.Path, String.IsNullOrWhiteSpace(compatibilityData.MinCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MinCondition.Message);

						if (compatibilityData.MinCondition.fail)
						{
							throw new BuildException(msg);
						}
						else
						{
							log.ThrowWarning(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, msg);
						}
					}
				}

				if (compatibilityData.MinVersionCondition != null && compatibilityData.MinVersionCondition.revision != null)
				{
					// when checking version compatibility use the verision given in the manifest (release.Manifest.Version) before using 
					// the version we detected the manifest is more likely to have the correct version in terms of compatibility checking. 
					// The release.Version may be Flattened or modified by changing the version folder


					if (0 < compatibilityData.MinVersionCondition.revision.StrCompareVersions(releaseVersion))
					{
						if (releaseVersion == Release.Flattened)
						{
							string msg = String.Format("Package '{0}' requires minimal version '{1}' of package '{2}', but package '{2}' ({3}) is listed as flattened so the correct version cannot be determined. This warning can be resolved by indicating the flattened package's version in the manifest file, see framework docs for details. {4}",
								compatibilityData.MinVersionCondition.ConditionSource.Name, compatibilityData.MinVersionCondition.revision, release.Name, release.Path,
								String.IsNullOrWhiteSpace(compatibilityData.MinVersionCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MinVersionCondition.Message);
							log.ThrowWarning(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, msg);
						}
						else
						{
							string msg = String.Format("Package '{0}' requires minimal version '{1}' of package '{2}', but package '{2}' ({3}) has version '{4}' {5}",
								compatibilityData.MinVersionCondition.ConditionSource.Name, compatibilityData.MinVersionCondition.revision, release.Name, release.Path, release.Version,
								String.IsNullOrWhiteSpace(compatibilityData.MinVersionCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MinVersionCondition.Message);

							if (compatibilityData.MinVersionCondition.fail)
							{
								throw new BuildException(msg);
							}
							else
							{
								log.ThrowWarning(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, msg);
							}
						}
					}
				}

				if (compatibilityData.MaxCondition != null && compatibilityData.MaxCondition.revision != null)
				{
					if (release.Manifest.Compatibility != null && release.Manifest.Compatibility.Revision != null)
					{
						if (0 > compatibilityData.MaxCondition.revision.StrCompareVersions(release.Manifest.Compatibility.Revision))
						{
							string msg = String.Format("Package '{0}' requires maximal revision '{1}' of package '{2}', but package '{2}-{3}' ({4}) has revision '{5}' {6}",
								compatibilityData.MaxCondition.ConditionSource.Name, compatibilityData.MaxCondition.revision, release.Name, release.Version, release.Path, release.Manifest.Compatibility.Revision,
								String.IsNullOrWhiteSpace(compatibilityData.MaxCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MaxCondition.Message);

							if (compatibilityData.MaxCondition.fail)
							{
								throw new BuildException(msg);
							}
							else
							{
								log.ThrowWarning(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, msg);
							}
						}
					}
					else
					{
						string msg = String.Format("Package '{0}' requires maximal revision '{1}' of package '{2}', but Manifest.xml file of package '{2}-{3}' ({4}) does not declare revision number. Can not check compatibility. {5}",
							compatibilityData.MaxCondition.ConditionSource.Name, compatibilityData.MaxCondition.revision, release.Name, release.Version, release.Path, String.IsNullOrWhiteSpace(compatibilityData.MaxCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MaxCondition.Message);

						if (compatibilityData.MaxCondition.fail)
						{
							throw new BuildException(msg);
						}
						else
						{
							log.ThrowWarning(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, msg);
						}
					}
				}

				if (compatibilityData.MaxVersionCondition != null && compatibilityData.MaxVersionCondition.revision != null)
				{
					if (0 > compatibilityData.MaxVersionCondition.revision.StrCompareVersions(releaseVersion))
					{
						if (releaseVersion == Release.Flattened)
						{
							string msg = String.Format("Package '{0}' requires maximal version '{1}' of package '{2}', but package '{2}' ({3}) is listed as flattened so the correct version cannot be determined. This warning can be resolved by indicating the flattened package's version in the manifest file, see framework docs for details. {4}",
								compatibilityData.MaxVersionCondition.ConditionSource.Name, compatibilityData.MaxVersionCondition.revision, release.Name, release.Path,
								String.IsNullOrWhiteSpace(compatibilityData.MaxVersionCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MaxVersionCondition.Message);
							log.ThrowWarning(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, msg);
						}
						else
						{
							string msg = String.Format("Package '{0}' requires maximal version '{1}' of package '{2}', but package '{2}' ({3}) has version '{4}' {5}",
							compatibilityData.MaxVersionCondition.ConditionSource.Name, compatibilityData.MaxVersionCondition.revision, release.Name, release.Path, releaseVersion,
							String.IsNullOrWhiteSpace(compatibilityData.MaxVersionCondition.Message) ? String.Empty : Environment.NewLine + compatibilityData.MaxVersionCondition.Message);

							if (compatibilityData.MaxVersionCondition.fail)
							{
								throw new BuildException(msg);
							}
							else
							{
								log.ThrowWarning(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, msg);
							}
						}
					}
				}
			}
		}

		// DAVE-FUTURE-REFACTOR-TODO: the fact that this takes a string => release callback is very annoying to deal with and conceptually
		// might be unnecessary? in theory it should be possible to just record every release we pass to this function and do compatibility
		// checks against those - unless we end up having multiple releases per package which we need to separate by build graph?
		public void CheckCompatibility(Log log, Release release, Func<string, Release> packages)
		{
			// Check this release against existing conditions:
			if (CompatibilityConditions.TryGetValue(release.Name, out CompatibilityData compatibilityData))
			{
				VerifyCompatibilityData(log, compatibilityData, release);
			}

			Compatibility compatibility = release.Manifest.Compatibility;
			if (compatibility != null)
			{
				foreach (Compatibility.Dependent dependent in compatibility.Dependents)
				{
					CompatibilityData updated = null;

					CompatibilityConditions.AddOrUpdate(dependent.PackageName,
						(key) =>
						{
							updated = new CompatibilityData(dependent, release);
							return updated;
						},
						(key, data) =>
						{
							CompatibilityData.Condition minCondition = null;
							CompatibilityData.Condition minVersionCondition = null;
							CompatibilityData.Condition maxCondition = null;
							CompatibilityData.Condition maxVersionCondition = null;

							if (dependent.MinRevision != null)
							{
								if (data.MinCondition.revision == null || 0 < dependent.MinRevision.StrCompareVersions(data.MinCondition.revision))
								{
									minCondition = new CompatibilityData.Condition(dependent.MinRevision, data.MinCondition.fail ? true : dependent.Fail, dependent.Message, release);
								}
							}
							if (dependent.MinVersion != null)
							{
								if (data.MinVersionCondition.revision == null || 0 < dependent.MinVersion.StrCompareVersions(data.MinVersionCondition.revision))
								{
									minVersionCondition = new CompatibilityData.Condition(dependent.MinVersion, data.MinVersionCondition.fail ? true : dependent.Fail, dependent.Message, release);
								}
							}

							if (dependent.MaxRevision != null)
							{
								if (data.MaxCondition?.revision == null || 0 > dependent.MaxRevision.StrCompareVersions(data.MaxCondition.revision))
								{
									maxCondition = new CompatibilityData.Condition(dependent.MaxRevision, data.MaxCondition.fail ? true : dependent.Fail, dependent.Message, release);
								}
							}
							if (dependent.MaxVersion != null)
							{
								if (data.MaxVersionCondition.revision == null || 0 > dependent.MaxVersion.StrCompareVersions(data.MaxVersionCondition.revision))
								{
									maxVersionCondition = new CompatibilityData.Condition(dependent.MaxRevision, data.MaxVersionCondition.fail ? true : dependent.Fail, dependent.Message, release);
								}
							}

							updated = new CompatibilityData(minCondition ?? new CompatibilityData.Condition(data.MinCondition.revision, data.MinCondition.fail, data.MinCondition.Message, data.MinCondition.ConditionSource),
															minVersionCondition ?? new CompatibilityData.Condition(data.MinVersionCondition.revision, data.MinVersionCondition.fail, data.MinVersionCondition.Message, data.MinVersionCondition.ConditionSource),
															maxCondition ?? new CompatibilityData.Condition(data.MaxCondition.revision, data.MaxCondition.fail, data.MaxCondition.Message, data.MaxCondition.ConditionSource),
															maxVersionCondition ?? new CompatibilityData.Condition(data.MaxVersionCondition.revision, data.MaxVersionCondition.fail, data.MaxVersionCondition.Message, data.MaxVersionCondition.ConditionSource));
							return updated;
						}
				  );

					if (updated != null && packages != null)
					{
						//Verify existing packages against new/updated compatibility condition:
						Release packageRelease = packages(dependent.PackageName);
						if (packageRelease != null)
						{
							VerifyCompatibilityData(log, updated, packageRelease);
						}
					}
				}
			}
		}

		private class CompatibilityData
		{
			internal readonly Condition MinCondition;
			internal readonly Condition MaxCondition;
			internal readonly Condition MinVersionCondition;
			internal readonly Condition MaxVersionCondition;

			internal int Verified = 0;

			internal CompatibilityData(Compatibility.Dependent dependent, Release source)
			{
				MinCondition = new Condition(dependent.MinRevision, dependent.Fail, dependent.Message, source);
				MinVersionCondition = new Condition(dependent.MinVersion, dependent.Fail, dependent.Message, source);
				MaxCondition = new Condition(dependent.MaxRevision, dependent.Fail, dependent.Message, source);
				MaxVersionCondition = new Condition(dependent.MaxVersion, dependent.Fail, dependent.Message, source);
			}

			internal CompatibilityData(Condition minCondition, Condition minVersionCondition, Condition maxCondition, Condition maxVersionCondition)
			{
				MinCondition = minCondition;
				MinVersionCondition = minVersionCondition;
				MaxCondition = maxCondition;
				MaxVersionCondition = maxVersionCondition;
			}

			internal class Condition
			{
				internal string revision;
				internal bool fail;
				internal string Message;
				internal Release ConditionSource;

				internal Condition(string rev, bool isFail, string message, Release source)
				{
					revision = rev;
					fail = isFail;
					Message = message;
					ConditionSource = source;
				}
			}
		}
		/// <summary>Returns a collection of releases.</summary>
		private readonly ConcurrentDictionary<string, CompatibilityData> CompatibilityConditions = new ConcurrentDictionary<string, CompatibilityData>();

		#endregion
	}
}
