using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.ExceptionServices;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

using NuGet.Common;
using NuGet.Configuration;
using NuGet.Frameworks;
using NuGet.Packaging;
using NuGet.Packaging.Core;
using NuGet.Packaging.Signing;
using NuGet.Protocol;
using NuGet.Protocol.Core.Types;
using NuGet.Resolver;
using NuGet.Versioning;

namespace NAnt.NuGet
{
	public class NugetManager
	{
		// package name, version pairs that user has requested to use
		private readonly Dictionary<string, string> m_pinnedFrameworkPackageVersions;
		private readonly Dictionary<string, string> m_pinnedCorePackageVersions;
		private readonly Dictionary<string, string> m_pinnedStandardPackageVersions;

		private readonly NugetSource[] m_sources; // all repositories we will search, created froom user passed in list of during constuctor
		private readonly string m_nugetPackageFolder; // the folder packages will ultimately be placed in after download and unpacking
		private readonly string m_cacheFolder; // location to store cache resolution files, greatly speed up resolution if nothing has change from last run

		private readonly Dictionary<NuGetFramework, Task<ResolutionResult>> m_nugetResolutionMap = new Dictionary<NuGetFramework, Task<ResolutionResult>>(NuGetFramework.Comparer);
		
		private readonly ConcurrentDictionary<NuGetFramework, Dictionary<PackageIdentity, Task<NugetPackage>>> m_packageContents = new ConcurrentDictionary<NuGetFramework, Dictionary<PackageIdentity, Task<NugetPackage>>>(NuGetFramework.Comparer);
		
		// maps identities to downloaded packages path and reader class, lazy prevents multiple downloads being kicked off
		private readonly ConcurrentDictionary<PackageIdentity, Lazy<Task<(string, PackageFolderReader)>>> m_downloadedPackages = new ConcurrentDictionary<PackageIdentity, Lazy<Task<(string, PackageFolderReader)>>>(PackageIdentity.Comparer);

		public NugetManager(string downloadFolder, string cacheFolder, IEnumerable<string> nugetSources, Dictionary<string, string> pinnedFrameworkPackageVersions = null, Dictionary<string, string> pinnedCorePackageVersions = null, Dictionary<string, string> pinnedStandardPackageVersions = null)
		{
			m_pinnedFrameworkPackageVersions = pinnedFrameworkPackageVersions != null ? new Dictionary<string, string>(pinnedFrameworkPackageVersions) : new Dictionary<string, string>();
			m_pinnedCorePackageVersions = pinnedCorePackageVersions != null ? new Dictionary<string, string>(pinnedCorePackageVersions) : new Dictionary<string, string>();
			m_pinnedStandardPackageVersions = pinnedStandardPackageVersions != null ? new Dictionary<string, string>(pinnedStandardPackageVersions) : new Dictionary<string, string>();

			// create repository objects
			if (!nugetSources.Any())
			{
				// use default source if not specified
				nugetSources = new string[] { "https://api.nuget.org/v3/index.json" };
			}
			m_sources = nugetSources.Select(source => new NugetSource(Repository.Factory.GetCoreV3(source))).ToArray();

			m_nugetPackageFolder = downloadFolder;
			m_cacheFolder = cacheFolder;
		}

		// for a given target .net family, download and return all the required packages for a given set of package names (i.e. requested packages and all dependencies)
		public IEnumerable<NugetPackage> GetRequiredPackages(string netFamilyMoniker, IEnumerable<string> packageNames)
		{
			try
			{
				// currently we don't support cancellation, but this token is passed all the way down where nuget allows for future use
				CancellationToken cancellationToken = CancellationToken.None;

				// convert packages name to the nuget identities that match names and versions we resolved for this net family
				NuGetFramework framework = ParseNugetFramework(netFamilyMoniker);
				ResolutionResult resolvedPackages = GetPackageResolution(framework, cancellationToken);
				IEnumerable<PackageIdentity> packageIdentities = resolvedPackages.GetResolvedPackageIdentities(packageNames);

				Dictionary<PackageIdentity, Task<NugetPackage>> familySpecificContents = m_packageContents.GetOrAdd(framework, (key) => new Dictionary<PackageIdentity, Task<NugetPackage>>(PackageIdentity.Comparer));
				List<Task<NugetPackage>> packageContentsTasks = new List<Task<NugetPackage>>();
				lock (familySpecificContents)
				{
					foreach (PackageIdentity identity in packageIdentities)
					{
						if (!familySpecificContents.TryGetValue(identity, out Task<NugetPackage> nugetPackageContentsTask))
						{
							// kick off a find/download task for this identity or retrieve on if it has already
							// been launched for another .net family
							Task<(string, PackageFolderReader)> downloadTask = m_downloadedPackages.GetOrAdd
							(
								identity,
								new Lazy<Task<(string, PackageFolderReader)>>(() => FindOrDownload(identity, resolvedPackages.GetSourceforIdentity(identity), m_nugetPackageFolder, framework, cancellationToken))
							)
							.Value;

							// continue the download task with task to convert the download package reader into the a NugetPackage object for the requested
							// net family
							nugetPackageContentsTask = downloadTask.ContinueWith(readerTask => new NugetPackage(identity.Id, readerTask.Result.Item1, readerTask.Result.Item2, framework));
							familySpecificContents[identity] = nugetPackageContentsTask;
						}
						packageContentsTasks.Add(nugetPackageContentsTask);
					}
				}

				return packageContentsTasks.Select(task => task.Result).ToArray(); // ToArray to force eval
			}
			catch (AggregateException ae)
			{
				ae = ae.Flatten();
				if (ae.InnerExceptions.Count == 1)
				{
					ExceptionDispatchInfo.Capture(ae.InnerExceptions.First()).Throw();
				}
				throw;
			}
		}

		private ResolutionResult GetPackageResolution(NuGetFramework framework, CancellationToken cancellationToken)
		{
			Task<ResolutionResult> getResolutionTask = null;
			lock (m_nugetResolutionMap)
			{
				if (!m_nugetResolutionMap.TryGetValue(framework, out getResolutionTask))
				{
					//GlobalInitProject.Log.Minimal.WriteLine("Resolving nuget graph for framework: {0}", framework.GetShortFolderName());
					string cacheFilePath = GetCacheFilePath(framework);
					IEnumerable<string> repositoryNames = m_sources.Select(source => source.Repository.PackageSource.Source);
					if (!ResolutionResult.TryLoadFromCacheFile(framework, cacheFilePath, m_sources, out ResolutionResult resolution))
					{
						getResolutionTask =
							Task.Run(async () =>
							{
								return await ResolveDependencyGraph(framework, cancellationToken);
							})
							.ContinueWith
							(
								(resolveTask) =>
								{
									resolveTask.Result.WriteToCacheFile(cacheFilePath);
									return resolveTask.Result;
								}
							);
					}
					else
					{
						getResolutionTask = Task.FromResult(resolution);
					}
					m_nugetResolutionMap[framework] = getResolutionTask;
				}
			}
			return getResolutionTask.Result;
		}

		private NuGetFramework ParseNugetFramework(string netFamilyMoniker)
		{
			NuGetFramework netFamily = NuGetFramework.Parse(netFamilyMoniker);
			if (netFamily.IsUnsupported)
			{
				// if we have a leading v, assume it's a net framework version
				if (netFamilyMoniker.StartsWith("v") && Version.TryParse(netFamilyMoniker.Substring(1), out Version version))
				{
					return new NuGetFramework(FrameworkConstants.FrameworkIdentifiers.Net, version);
				}
				else
				{
					throw new FormatException($"Could not parse .NET family moniker '{netFamilyMoniker}'.");
				}
			}
			else
			{
				return netFamily;
			}
		}

		private string GetCacheFilePath(NuGetFramework netFramework)
		{
			uint StringHash(string s)
			{
				unchecked
				{
					uint h = 17;
					foreach (char c in s)
					{
						h = h * 31 + c;
					}
					return h;
				}
			}

			// hash the inputs, for use as a file name to quick check if we've done this resolve before
			uint hash = 17;
			unchecked
			{
				hash = hash * 31 + StringHash(netFramework.ToString());
				foreach (string source in m_sources.Select(repo => repo.Repository.PackageSource.Source)) // NUGET-TODO check this is normalized
				{
					hash = hash * 31 + StringHash(source);
				}
				foreach (KeyValuePair<string, string> pinnedVersion in GetPinnedVersionsForFramework(netFramework))
				{
					uint innerHash = 17;
					innerHash = innerHash * 31 + StringHash(pinnedVersion.Key);
					innerHash = innerHash * 31 + StringHash(pinnedVersion.Value);
					hash += innerHash;
				}
			}

			return Path.Combine(m_cacheFolder, $"{hash}.nucache");
		}

		private async Task<ResolutionResult> ResolveDependencyGraph(NuGetFramework framework, CancellationToken cancellationToken)
		{
			using (SourceCacheContext cacheContext = new SourceCacheContext())
			{
				// step 1: try to locate the pinned packages from the nuget servers
				Dictionary<string, string> inputPinnedVersions = GetPinnedVersionsForFramework(framework);
				List<Task<((string, string), PackageIdentity)>> inputIdentities = new List<Task<((string, string), PackageIdentity)>>();
				foreach (KeyValuePair<string, string> pinned in inputPinnedVersions)
				{
					inputIdentities.Add(Task.Run(async () =>
					{
						string packageId = pinned.Key;
						NuGetVersion packageVersion = new NuGetVersion(pinned.Value);
						List<string> successfulRepositories = new List<string>(); // track name of repositories we successfully connected to, but didn't find package on (for clearer errors)
						foreach (NugetSource source in m_sources)
						{
							PackageIdentity identity = await source.GetRemoteIdentity(packageId, packageVersion, successfulRepositories, cacheContext, cancellationToken);
							if (identity != null)
							{
								return ((packageId, pinned.Value), identity);
							}
							successfulRepositories.Add(source.Repository.PackageSource.Source);
						}
						return ((packageId, pinned.Value), null);
					}));
				}

				// step 2: determine dependencies remotely by using a dependency framework constraint
				// accumulate all packages which are referenced into a gigantic has set
				Dictionary<PackageIdentity, Task<SourcePackageDependencyInfo>> consideredSet = new Dictionary<PackageIdentity, Task<SourcePackageDependencyInfo>>(PackageIdentityComparer.Default);
				Dictionary<string, Task<IEnumerable<SourcePackageDependencyInfo>>> availableVersions = new Dictionary<string, Task<IEnumerable<SourcePackageDependencyInfo>>>(StringComparer.OrdinalIgnoreCase); // NUGET TODO - do I gain anything by caching this? cachecontext may handle this more or less for us

				List<Task> continuations = new List<Task>();
				List<string> unsatisfiedDependencies = new List<string>();
				List<(string, string)> missingPackages = new List<(string, string)>();

				foreach (Task<((string, string) Request, PackageIdentity Identity)> identityTask in inputIdentities)
				{
					continuations.Add(identityTask.ContinueWith(async (parent) =>
					{
						if (identityTask.Result.Identity?.HasVersion ?? false)
						{
							// we found the package, keep begin a deep dependency search
							await GetDependenciesRecursive(identityTask.Result.Identity, framework, cacheContext, consideredSet, availableVersions, unsatisfiedDependencies, cancellationToken);
						}
						else
						{
							// missing dependency
							lock (missingPackages)
							{
								missingPackages.Add(identityTask.Result.Request);
							}
						}
					}).Unwrap());
				}

				// await all results
				await Task.WhenAll(continuations.ToArray());
				continuations.Clear(); // we reuse this later

				// resolve will throw exception if given no input indentities, we want defered errors so early out here
				IEnumerable<Task<((string, string), PackageIdentity)>> resolvedPinnedPackages = inputIdentities.Where(x => x.Result.Item2?.HasVersion == true);
				if (!resolvedPinnedPackages.Any())
				{
					return new ResolutionResult
					(
						framework,
						m_sources,
						new Dictionary<PackageIdentity, NugetSource>(),
						new Dictionary<string, List<PackageIdentity>>(),
						missingPackages
					);
				}

				IEnumerable<SourcePackageDependencyInfo> computedSourceDependencies = consideredSet.Values.Select(x => x.Result);

				// resolve globally across all dependencies
				IEnumerable<PackageIdentity> resolvedIdentities;
				try
				{
					PackageResolverContext resolverContext = new PackageResolverContext
					(
						DependencyBehavior.Lowest,
						resolvedPinnedPackages.Select(pinned => pinned.Result.Item2.Id),
						resolvedPinnedPackages.Select(pinned => pinned.Result.Item2.Id),
						Enumerable.Empty<PackageReference>(),
						resolvedPinnedPackages.Select(pinned => pinned.Result.Item2),
						computedSourceDependencies,
						m_sources.Select(source => source.Repository.PackageSource),
						NullLogger.Instance
					);
					PackageResolver resolver = new PackageResolver();
					resolvedIdentities = resolver.Resolve(resolverContext, CancellationToken.None);
				}
				catch (NuGetResolverConstraintException resolveException)
				{
					Match match = Regex.Match(resolveException.Message, "Unable to find a version of '([^']*)'");
					if (match.Success)
					{
						IEnumerable<PackageIdentity> pinnedIdentities = resolvedPinnedPackages.Select(resolved => resolved.Result.Item2);

						string packageOfInterest = match.Groups[1].Value;

						// find all version we submitted to resolution
						IEnumerable<SourcePackageDependencyInfo> submittedVersions = computedSourceDependencies.Where(depInfo => depInfo.Id == packageOfInterest);

						// find all dependencies on this packages
						IEnumerable<(SourcePackageDependencyInfo, PackageDependency)> dependenciesOnPackage = computedSourceDependencies.SelectMany(depInfo => depInfo.Dependencies.Where(dep => dep.Id == packageOfInterest).Select(dep  => (depInfo, dep)));
						// find unqiue dependencies, i.e same version range
						IEnumerable<(SourcePackageDependencyInfo, PackageDependency)> uniqueDependencies = dependenciesOnPackage.GroupBy(depOnPackage => depOnPackage.Item2).Select(group => group.First());
						// grab the first matching chain of dependencies from a pinned version
						IEnumerable<string> dependencyChains = uniqueDependencies.Select
						(
							((SourcePackageDependencyInfo Dependent, PackageDependency Dependency) uniqueDependency) =>
							{
								SourcePackageDependencyInfo search = uniqueDependency.Dependent;
								List<SourcePackageDependencyInfo> chain = new List<SourcePackageDependencyInfo>();
								while (!pinnedIdentities.Contains(search))
								{
									chain.Add(search);
									search = computedSourceDependencies.First(depInfo => depInfo.Dependencies.Any(dep => dep.Id == search.Id && dep.VersionRange.Satisfies(search.Version)));
								}
								chain.Add(search);
								return $"{String.Join(" => ", chain.Reverse<SourcePackageDependencyInfo>().Select(depInfo => $"{depInfo.Id} {depInfo.Version}"))} => {uniqueDependency.Dependency}";
							}
						);
						throw new UnresolvableNugetDependencyException(packageOfInterest, submittedVersions, dependencyChains, resolveException);
					}
					throw new MissingNugetDependenciesException(unsatisfiedDependencies, framework, resolveException);
				}

				IEnumerable<SourcePackageDependencyInfo> allPackagesWithDependencyInfo = resolvedIdentities.Select(p => computedSourceDependencies.Single(x => PackageIdentityComparer.Default.Equals(x, p)));
				Dictionary<string, List<PackageIdentity>> perPackageDependencies = new Dictionary<string, List<PackageIdentity>>(StringComparer.OrdinalIgnoreCase);
				foreach (Task<((string Name, string Version) Pinned, PackageIdentity Identity)> resolvePinnedTask in resolvedPinnedPackages)
				{
					continuations.Add(Task.Run(() =>
					{
						// using the newly globally solved set of "packages to install", we now ask the dependency solver for just one package's flattened dependency graph
						PackageResolverContext resolverContextMinimal = new PackageResolverContext
						(
							DependencyBehavior.Lowest,
							new[] { resolvePinnedTask.Result.Identity.Id },
							Enumerable.Empty<string>(),
							Enumerable.Empty<PackageReference>(),
							new[] { resolvePinnedTask.Result.Identity },
							allPackagesWithDependencyInfo,
							m_sources.Select(source => source.Repository.PackageSource),
							NullLogger.Instance
						);

						// locally solve the dependencies, note: this is a flattened graph include the top node
						PackageResolver resolverSub = new PackageResolver();
						IEnumerable<SourcePackageDependencyInfo> thisProjectOnly = resolverSub.Resolve(resolverContextMinimal, CancellationToken.None)
							.Select(p => allPackagesWithDependencyInfo.Single(t => PackageIdentityComparer.Default.Equals(t, p)));

						lock (perPackageDependencies)
						{
							perPackageDependencies.Add(resolvePinnedTask.Result.Pinned.Name, thisProjectOnly.Cast<PackageIdentity>().ToList());
						}
					}));
				}

				await Task.WhenAll(continuations);

				return new ResolutionResult
				(
					framework,
					m_sources,
					allPackagesWithDependencyInfo.ToDictionary<SourcePackageDependencyInfo, PackageIdentity, NugetSource>
					(
						sourceInfo => sourceInfo, 
						sourceInfo => m_sources.First(source => sourceInfo.Source == source.Repository)
					),
					perPackageDependencies,
					missingPackages
				);
			}
		}

		private async Task GetDependenciesRecursive(PackageIdentity provokingPackage, NuGetFramework framework, SourceCacheContext cacheContext, Dictionary<PackageIdentity, Task<SourcePackageDependencyInfo>> consideredSet, Dictionary<string, Task<IEnumerable<SourcePackageDependencyInfo>>> availableVersions, List<string> unsatisfiedDependencies, CancellationToken cancellationToken)
		{
			Task<SourcePackageDependencyInfo> innerTask = null;
			lock (consideredSet)
			{
				if (!consideredSet.TryGetValue(provokingPackage, out innerTask))
				{
					innerTask = Task.Run(async () =>
					{
						List<string> successfulRepositories = new List<string>(); // track name of repositories we successfully connected to, but didn't find package on (for clearer errors)
						SourcePackageDependencyInfo dependencyInfo = null;
						foreach (NugetSource source in m_sources)
						{
							dependencyInfo = await source.GetDependencyInfo(provokingPackage, framework, successfulRepositories, cacheContext, cancellationToken);
							if (dependencyInfo != null)
							{
								break;
							}
							successfulRepositories.Add(source.Repository.PackageSource.Source);
						}
						if (dependencyInfo == null)
						{
							// very unlikely to fail to get dependency info since we're already working with identities we got
							// back successfully from remote but paranoia safety here anyways
							throw new DependencyResolutionFailedException(provokingPackage.Id, provokingPackage.Version.OriginalVersion, successfulRepositories);
						}

						foreach (PackageDependency dependency in dependencyInfo.Dependencies)
						{
							PackageIdentity dependencyIdentity = await GetMinimumIdentityForDependency(dependency, framework, cacheContext, availableVersions, cancellationToken);
							if (dependencyIdentity == null)
							{
								unsatisfiedDependencies.Add($"{dependency.Id}-{dependency.VersionRange}");
							}
							else
							{
								await GetDependenciesRecursive(dependencyIdentity, framework,
									cacheContext, consideredSet, availableVersions, unsatisfiedDependencies, cancellationToken);
							}
						}
						return dependencyInfo;
					});

					consideredSet[provokingPackage] = innerTask;
				}
			}
			await innerTask;
			return;
		}

		private async Task<PackageIdentity> GetMinimumIdentityForDependency(PackageDependency dependency, NuGetFramework framework, SourceCacheContext cacheContext, Dictionary<string, Task<IEnumerable<SourcePackageDependencyInfo>>> availableVersions, CancellationToken cancellationToken)
		{
			Task<IEnumerable<SourcePackageDependencyInfo>> innerTask = null;
			lock (availableVersions)
			{
				if (!availableVersions.TryGetValue(dependency.Id, out innerTask))
				{
					innerTask = Task.Run(async () =>
					{
						List<string> successfulRepositories = new List<string>(); // track name of repositories we successfully connected to, but didn't find package on (for clearer errors)
						foreach (NugetSource source in m_sources)
						{
							IEnumerable<SourcePackageDependencyInfo> sourceDependencyInfos = await source.GetDependencyInfos(dependency.Id, framework, successfulRepositories, cacheContext, cancellationToken);
							if (sourceDependencyInfos?.Any() ?? false)
							{
								// note: we're returning here as soon as we have a set of version from a single source rather than
								// aggregating the results from all sources
								return sourceDependencyInfos;
							}
							successfulRepositories.Add(source.Repository.PackageSource.Source);
						}
						return null;
					});

					availableVersions[dependency.Id] = innerTask;
				}
			}

			IEnumerable<SourcePackageDependencyInfo> dependencyInfos = await innerTask;
			if (dependencyInfos is null)
			{
				// no info found for this dependency, higher level code can determine if this is an error or not
				return null;
			}

			IOrderedEnumerable<SourcePackageDependencyInfo> availableDependencyInfosInAscendingVersionOrder = innerTask.Result.OrderBy(depInfo => depInfo.Version); 
			foreach (SourcePackageDependencyInfo availableDependencyInfo in availableDependencyInfosInAscendingVersionOrder)
			{
				if (dependency.VersionRange.Satisfies(availableDependencyInfo.Version))
				{
					return new PackageIdentity(availableDependencyInfo.Id, availableDependencyInfo.Version);
				}
			}

			return null;
		}

		private static Task<(string installedPath, PackageFolderReader reader)> FindOrDownload(PackageIdentity package, NugetSource sourceRepository, string downloadDirectory, NuGetFramework framework, CancellationToken cancellationToken)
		{
			DirectoryInfo downloadFolderInfo = new DirectoryInfo(downloadDirectory).CreateSubdirectory("Downloads");

			ISettings settings = Settings.LoadDefaultSettings(null);
			ClientPolicyContext clientPolicy = ClientPolicyContext.GetClientPolicy(settings, NullLogger.Instance);

			PackagePathResolver packagePathResolver = new PackagePathResolver(downloadDirectory);
			PackageExtractionContext packageExtractionContext = new PackageExtractionContext(
				PackageSaveMode.Defaultv3,
				XmlDocFileSaveMode.None,
				clientPolicy,
				NullLogger.Instance);

			List<Task<PackageFolderReader>> subtasks = new List<Task<PackageFolderReader>>();

			SourceCacheContext cache = new SourceCacheContext(); // NUGET-TODO make a global / static (and dispose on exit / reset?)
			PackageDownloadContext downloaderContext = new PackageDownloadContext(cache, downloadFolderInfo.FullName, true);

			return Task.Run(async () =>
			{
				string installedPath = packagePathResolver.GetInstalledPath(package);
				if (installedPath == null)
				{
					//TODO: log.Minimal.WriteLine("Downloading nuget {0} {1}...", package.Identity.Id, package.Identity.Version);
					DownloadResource downloadResource = await sourceRepository.Repository.GetResourceAsync<DownloadResource>(cancellationToken);
					using (DownloadResourceResult downloadResult = await downloadResource.GetDownloadResourceResultAsync(
						package,
						downloaderContext,
						downloadFolderInfo.FullName,
						NullLogger.Instance, cancellationToken))
					{
						await PackageExtractor.ExtractPackageAsync(
							downloadResult.PackageSource,
							downloadResult.PackageStream,
							packagePathResolver,
							packageExtractionContext,
							CancellationToken.None);
					}

					installedPath = packagePathResolver.GetInstalledPath(package);
				}
				return (installedPath, new PackageFolderReader(installedPath));
			});
		}

		private Dictionary<string, string> GetPinnedVersionsForFramework(NuGetFramework nuGetFramework)
		{
			switch (nuGetFramework.Framework)
			{
				case FrameworkConstants.FrameworkIdentifiers.Net:
					return m_pinnedFrameworkPackageVersions;
				case FrameworkConstants.FrameworkIdentifiers.NetCore:
				case FrameworkConstants.FrameworkIdentifiers.NetCoreApp:
					return m_pinnedCorePackageVersions;
				case FrameworkConstants.FrameworkIdentifiers.NetStandard:
				case FrameworkConstants.FrameworkIdentifiers.NetStandardApp:
					return m_pinnedStandardPackageVersions;
				default:
					throw new ArgumentException($"Cannot determine input identities for {nuGetFramework.Framework}!");
			}
		}
	}
}