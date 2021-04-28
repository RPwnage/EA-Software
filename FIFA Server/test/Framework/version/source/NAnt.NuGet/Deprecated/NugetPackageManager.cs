using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

using NuGet.Common;
using NuGet.Frameworks;
using NuGet.Packaging;
using NuGet.Packaging.Core;
using NuGet.Protocol;
using NuGet.Protocol.Core.Types;
using NuGet.Versioning;

namespace NAnt.NuGet.Deprecated
{
	// this class is just a wrapper so that our dependencies don't need to know about nuget types
	public class NugetPackage
	{
		public class NugetVersionRequirement
		{
			private readonly VersionRange m_versionRange;

			internal NugetVersionRequirement(VersionRange range)
			{
				m_versionRange = range;
			}

			public string GetBestAutoVersion()
			{
				// "best" version determination must meet following criteria
				// 1) uses a concrete version - we cannot use non-inclusive ranges
				// 2) is deterministic - i.e if no bound is provided we cannot simply ask nuget for latest version because latest may change which means our build determinism is dependent on state of external server
				// 3) cannot involve nuget calls at all - this is run for every nuget package - even those already downloaded, calling external services is simply too slow
				string minVersion = m_versionRange.IsMinInclusive && m_versionRange.HasLowerBound ? m_versionRange.MinVersion.ToString() : null; // we cannot use min version if range is not inclusive
				string maxVersion = m_versionRange.IsMaxInclusive && m_versionRange.HasUpperBound ? m_versionRange.MaxVersion.ToString() : null; // we cannot use max version if range is not inclusive
				return maxVersion ?? minVersion;
			}

			public bool Satisfies(string version)
			{
				return m_versionRange.Satisfies(new NuGetVersion(version));
			}

			public string GetVersionRequirementDescription()
			{
				string versionRequirements = null;
				if (m_versionRange.HasLowerBound)
				{
					versionRequirements = m_versionRange.IsMinInclusive ? String.Format("At least {0}", m_versionRange.MinVersion) : String.Format("Greater than {0}", m_versionRange.MinVersion);
				}
				if (m_versionRange.HasUpperBound)
				{
					if (versionRequirements != null)
					{
						versionRequirements = String.Format(m_versionRange.IsMaxInclusive ? "{0}, at most {1}" : "{0}, less than {1}", versionRequirements, m_versionRange.MaxVersion);
					}
					else
					{
						versionRequirements = String.Format(m_versionRange.IsMaxInclusive ? "At most {1}" : "Less than {1}", m_versionRange.MaxVersion);
					}
				}
				return versionRequirements;
			}
		}

		public class FrameworkItems
		{
			public readonly string FrameworkVersion;

			public readonly ReadOnlyCollection<string> Assemblies;
			public readonly ReadOnlyCollection<string> ContentFiles;
			public readonly ReadOnlyCollection<string> Dependencies;
			public readonly ReadOnlyCollection<string> InitFiles;
			public readonly ReadOnlyCollection<string> InstallFiles;
			public readonly ReadOnlyCollection<string> PropsFiles;
			public readonly ReadOnlyCollection<string> TargetsFiles;
			public readonly ReadOnlyCollection<string> FrameworkAssemblies;

			internal FrameworkItems(string version, IEnumerable<string> assemblies, IEnumerable<string> content, IEnumerable<string> dependencies, IEnumerable<string> initFiles, IEnumerable<string> installFiles,
				IEnumerable<string> propsFiles, IEnumerable<string> targetFiles, IEnumerable<string> frameworkAssemblies)
			{
				FrameworkVersion = version;

				Assemblies = new ReadOnlyCollection<string>(assemblies.ToArray());
				ContentFiles = new ReadOnlyCollection<string>(content.ToArray());
				Dependencies = new ReadOnlyCollection<string>(dependencies.ToArray());
				InitFiles = new ReadOnlyCollection<string>(initFiles.ToArray());
				InstallFiles = new ReadOnlyCollection<string>(installFiles.ToArray());
				PropsFiles = new ReadOnlyCollection<string>(propsFiles.ToArray());
				TargetsFiles = new ReadOnlyCollection<string>(targetFiles.ToArray());
				FrameworkAssemblies = new ReadOnlyCollection<string>(frameworkAssemblies.ToArray());
			}
		}

		public string Name => m_reader.GetIdentity().Id;

		public string Version => m_reader.GetIdentity().Version.ToString();

		public string LicenseUrl => m_reader.NuspecReader.GetLicenseUrl();
		public string Copyright => m_reader.NuspecReader.GetCopyright();

		public readonly string PackageRelativeDir;

		private readonly PackageReaderBase m_reader;

		private static readonly NuGetFramework[] NetFrameworkFrameworks = new NuGetFramework[]
		{
			FrameworkConstants.CommonFrameworks.Net11,
			FrameworkConstants.CommonFrameworks.Net2,
			FrameworkConstants.CommonFrameworks.Net35,
			FrameworkConstants.CommonFrameworks.Net4,
			FrameworkConstants.CommonFrameworks.Net45,
			FrameworkConstants.CommonFrameworks.Net451,
			FrameworkConstants.CommonFrameworks.Net452,
			FrameworkConstants.CommonFrameworks.Net46,
			FrameworkConstants.CommonFrameworks.Net461,
			FrameworkConstants.CommonFrameworks.Net462,
			FrameworkConstants.CommonFrameworks.Net463,
			new NuGetFramework(FrameworkConstants.FrameworkIdentifiers.Net, new Version(4, 7, 0, 0)),
			new NuGetFramework(FrameworkConstants.FrameworkIdentifiers.Net, new Version(4, 7, 1, 0)),
			new NuGetFramework(FrameworkConstants.FrameworkIdentifiers.Net, new Version(4, 7, 2, 0))
		};

		internal NugetPackage(string baseDirectory, PackageReaderBase packageReaderBase)
		{
			m_reader = packageReaderBase;
			PackageRelativeDir = baseDirectory;
		}

		public FrameworkItems GetSharedItems()
		{
			IEnumerable<PackageDependencyGroup> dependencies = m_reader.GetPackageDependencies();
			IEnumerable<FrameworkSpecificGroup> toolFiles = m_reader.GetToolItems();
			IEnumerable<FrameworkSpecificGroup> buildfiles = m_reader.GetBuildItems();
			IEnumerable<FrameworkSpecificGroup> content = m_reader.GetContentItems();
			IEnumerable<FrameworkSpecificGroup> frameworkAssemblies = m_reader.GetFrameworkItems();
			IEnumerable<FrameworkSpecificGroup> references = m_reader.GetReferenceItems();

			// assemblies appear to be a bit of wierd case - if only "any" assemblies are specified then we should use them, but if 
			// framework specific are specified then we should ignore
			IEnumerable<string> assemblies = Enumerable.Empty<string>();
			FrameworkSpecificGroup anyGroup = references.FirstOrDefault(group => group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any);
			if (anyGroup != null && !references.Any(group => group.TargetFramework.Framework != FrameworkConstants.SpecialIdentifiers.Any))
			{
				assemblies = anyGroup.Items;
			}

			return new FrameworkItems
			(
				version: null,
				assemblies: assemblies,
				content: content.FirstOrDefault(group => group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any)?.Items ?? Enumerable.Empty<string>(),
				dependencies: dependencies.FirstOrDefault(group => group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any)?.Packages.Select(package => package.Id) ?? Enumerable.Empty<string>(),
				initFiles: toolFiles.FirstOrDefault(group => group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any)?.Items.Where(item => Path.GetFileName(item).Equals("init.ps1", StringComparison.OrdinalIgnoreCase)) ?? Enumerable.Empty<string>(),
				installFiles: toolFiles.FirstOrDefault(group => group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any)?.Items.Where(item => Path.GetFileName(item).Equals("install.ps1", StringComparison.OrdinalIgnoreCase)) ?? Enumerable.Empty<string>(),
				propsFiles: buildfiles.FirstOrDefault(group => group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any)?.Items.Where(item => Path.GetExtension(item) == ".props") ?? Enumerable.Empty<string>(),
				targetFiles: buildfiles.FirstOrDefault(group => group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any)?.Items.Where(item => Path.GetExtension(item) == ".targets") ?? Enumerable.Empty<string>(),
				frameworkAssemblies: frameworkAssemblies.FirstOrDefault(group => group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any)?.Items ?? Enumerable.Empty<string>()
			);
		}

		// get item that need to be exposed from the package grouped by .net framework version
		public List<FrameworkItems> GetFrameworkItems(out string highestUnsupportedVersion)
		{
			// get supported frameworks
			IEnumerable<NuGetFramework> supportedFrameworks = m_reader.GetSupportedFrameworks().Where(framework => framework.Framework != FrameworkConstants.SpecialIdentifiers.Any);

			// reduce supported frameworks to the closest matching .net framework, also
			// track the highest .net framework version that *isn't* supported - if there
			// isn't one then it will be null
			FrameworkReducer frameworkReducer = new FrameworkReducer();
			NuGetFramework highestUnsupported = null;
			OrderedDictionary supportedToNearestDotNetFramework = new OrderedDictionary();
			foreach (NuGetFramework netFramework in NetFrameworkFrameworks.Reverse())
			{
				NuGetFramework nearest = frameworkReducer.GetNearest(netFramework, supportedFrameworks);
				if (nearest != null)
				{
					supportedToNearestDotNetFramework[nearest] = netFramework;
				}
				else
				{
					highestUnsupported = highestUnsupported ?? netFramework;
				}
			}

			// output highest unsupported for minimum checks, null if not applicable
			highestUnsupportedVersion = highestUnsupported != null ? FrameworkAsVersionString(highestUnsupported) : null;

			// convert to a more convenient types and reverse the mapping
			KeyValuePair<NuGetFramework, NuGetFramework>[] maximumDotNetFrameworkToSupported = supportedToNearestDotNetFramework.Keys
				.OfType<NuGetFramework>()
				.Select(key => new KeyValuePair<NuGetFramework, NuGetFramework>((NuGetFramework)supportedToNearestDotNetFramework[key], key))
				.Reverse()
				.ToArray();

			// get all the file items we supported from the package
			IEnumerable<PackageDependencyGroup> dependencies = m_reader.GetPackageDependencies();
			IEnumerable<FrameworkSpecificGroup> references = m_reader.GetReferenceItems();
			IEnumerable<FrameworkSpecificGroup> toolFiles = m_reader.GetToolItems();
			IEnumerable<FrameworkSpecificGroup> buildfiles = m_reader.GetBuildItems();
			IEnumerable<FrameworkSpecificGroup> content = m_reader.GetContentItems();
			IEnumerable<FrameworkSpecificGroup> frameworkAssemblies = m_reader.GetFrameworkItems();

			// batch up all the information we need by .net framework support version
			List<FrameworkItems> frameworkItems = new List<FrameworkItems>();
			foreach (KeyValuePair<NuGetFramework, NuGetFramework> maxToSupported in maximumDotNetFrameworkToSupported)
			{
				frameworkItems.Add
				(
					new FrameworkItems
					(
						version: FrameworkAsVersionString(maxToSupported.Key),
						assemblies: references.FirstOrDefault(group => group.TargetFramework == maxToSupported.Value)?.Items ?? Enumerable.Empty<string>(),
						content: content.FirstOrDefault(group => group.TargetFramework == maxToSupported.Value)?.Items ?? Enumerable.Empty<string>(),
						dependencies: dependencies.FirstOrDefault(group => group.TargetFramework == maxToSupported.Value)?.Packages.Select(package => package.Id) ?? Enumerable.Empty<string>(),
						initFiles: toolFiles.FirstOrDefault(group => group.TargetFramework == maxToSupported.Value)?.Items.Where(item => Path.GetFileName(item).Equals("init.ps1", StringComparison.OrdinalIgnoreCase)) ?? Enumerable.Empty<string>(),
						installFiles: toolFiles.FirstOrDefault(group => group.TargetFramework == maxToSupported.Value)?.Items.Where(item => Path.GetFileName(item).Equals("install.ps1", StringComparison.OrdinalIgnoreCase)) ?? Enumerable.Empty<string>(),
						propsFiles: buildfiles.FirstOrDefault(group => group.TargetFramework == maxToSupported.Value)?.Items.Where(item => Path.GetExtension(item) == ".props") ?? Enumerable.Empty<string>(),
						targetFiles: buildfiles.FirstOrDefault(group => group.TargetFramework == maxToSupported.Value)?.Items.Where(item => Path.GetExtension(item) == ".targets") ?? Enumerable.Empty<string>(),
						frameworkAssemblies: frameworkAssemblies.FirstOrDefault(group => group.TargetFramework == maxToSupported.Value)?.Items ?? Enumerable.Empty<string>()
					)
				);
			}

			return frameworkItems;
		}

		public Dictionary<string, NugetVersionRequirement> GetAutoDependencyVersions(string dotNetVersion)
		{
			// find the framework closest to our current that is supported by the package
			NuGetFramework currentFramework = new NuGetFramework(FrameworkConstants.FrameworkIdentifiers.Net, new Version(dotNetVersion));
			IEnumerable<NuGetFramework> supportedFrameworks = m_reader.GetSupportedFrameworks().Where(framework => framework.Framework != FrameworkConstants.SpecialIdentifiers.Any);
			FrameworkReducer frameworkReducer = new FrameworkReducer();
			NuGetFramework closestFramework = frameworkReducer.GetNearest(currentFramework, supportedFrameworks);

			// find all dependencies for that framework
			IEnumerable<PackageDependency> dependencies = m_reader.GetPackageDependencies()
				.Where(group => group.TargetFramework == closestFramework || group.TargetFramework.Framework == FrameworkConstants.SpecialIdentifiers.Any)
				.SelectMany(group => group.Packages);

			return dependencies.ToDictionary(package => package.Id, package => new NugetVersionRequirement(package.VersionRange));
		}

		private static string FrameworkAsVersionString(NuGetFramework framework)
		{
			return framework.Version.Major.ToString() +
				"." + framework.Version.Minor.ToString() +
				(framework.Version.Build != 0 ? "." + framework.Version.Build.ToString() : String.Empty);
		}
	}

	public static class NugetPackageManager
	{
		public static string ContentFolder = PackagingConstants.Folders.Content;

		private const bool UseSideBySidePaths = false;

		// create a repository cache - I have no basis for doing this other than I saw it done in some example code and not doing it seems to cause
		// massive slow downloads to the point of timing out when lots of downloads are down in quick succession
		private static readonly ConcurrentDictionary<string, Lazy<SourceRepository>> s_remoteRepositoryCache = new ConcurrentDictionary<string, Lazy<SourceRepository>>();

		// check if a package exists - search a local location before query remote
		public static bool PackageExists(string package, string version, string remoteSource)
		{
			PackageIdentity identity = new PackageIdentity(package, new NuGetVersion(version));

			SourceRepository sourceRepository = s_remoteRepositoryCache.GetOrAdd
			(
				remoteSource,
				new Lazy<SourceRepository>(() => Repository.Factory.GetCoreV3(remoteSource), LazyThreadSafetyMode.ExecutionAndPublication)
			).Value;

			PackageMetadataResource metadataResource = sourceRepository.GetResource<PackageMetadataResource>();
			using (SourceCacheContext cacheContext = new SourceCacheContext())
			{
				IPackageSearchMetadata metadataResult = metadataResource.GetMetadataAsync(identity, cacheContext, NullLogger.Instance, CancellationToken.None).Result;
				return metadataResult != null;
			}
		}

		public static NugetPackage DownloadRemotePackage(string package, string version, string remoteSource, string downloadFolder)
		{
			PackageIdentity identity = new PackageIdentity(package, new NuGetVersion(version));

			// download remote package to specified folder
			SourceRepository sourceRepository = s_remoteRepositoryCache.GetOrAdd
			(
				remoteSource,
				new Lazy<SourceRepository>(() => Repository.Factory.GetCoreV3(remoteSource), LazyThreadSafetyMode.ExecutionAndPublication)
			)
			.Value;

			DownloadResource downloadResource = sourceRepository.GetResource<DownloadResource>();
			NugetPackage nugetPackage = null;

			using (SourceCacheContext cacheContext = new SourceCacheContext())
			{
				DownloadResourceResult downloadResult = downloadResource.GetDownloadResourceResultAsync
				(
					identity,
					new PackageDownloadContext(cacheContext, downloadFolder, directDownload: true),
					downloadFolder,
					NullLogger.Instance,
					CancellationToken.None
				).Result;

				using (downloadResult)
				{
					if (downloadResult.Status != DownloadResourceResultStatus.Available)
					{
						throw new IOException(String.Format("Nuget package download failed with status {0}.", downloadResult.Status));
					}

					PackagePathResolver pathResolver = new PackagePathResolver(downloadFolder, UseSideBySidePaths);
					PackageArchiveReader archiveReader = new PackageArchiveReader(downloadResult.PackageStream);
					PackageExtractor.ExtractPackageAsync
					(
						remoteSource,
						archiveReader,
						pathResolver,
						new PackageExtractionContext
						(
							PackageSaveMode.Files | PackageSaveMode.Nuspec,
							XmlDocFileSaveMode.None,
							clientPolicyContext: null,
							logger: NullLogger.Instance
						),
						CancellationToken.None
					).Wait();

					string baseDir = pathResolver.GetPackageDirectoryName(identity);
					PackageFolderReader folderReader = new PackageFolderReader(Path.Combine(downloadFolder, baseDir));
					nugetPackage = new NugetPackage
					(
						pathResolver.GetPackageDirectoryName(identity),
						folderReader
					);

					return nugetPackage;
				}
			}
		}

		public static NugetPackage GetLocalPackage(string package, string version, string localSource)
		{
			PackageIdentity identity = new PackageIdentity(package, new NuGetVersion(version));

			PackagePathResolver pathResolver = new PackagePathResolver(localSource, UseSideBySidePaths);
			return new NugetPackage
			(
				pathResolver.GetPackageDirectoryName(identity),
				new PackageFolderReader(pathResolver.GetInstallPath(identity))
			);
		}
	}
}