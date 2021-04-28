/*
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

using NuGet.Common;
using NuGet.Frameworks;
using NuGet.Packaging;
using NuGet.Packaging.Core;
using NuGet.Packaging.Signing;
using NuGet.Protocol;
using NuGet.Protocol.Core.Types;
using NuGet.Resolver;
using NuGet.Versioning;

using Newtonsoft.Json;

using NAnt.Core.PackageCore;

namespace NAnt.Core.Nuget
{
    public class PackageResolutionResult
    {
        public struct PackageIdentityWithSource
        {
            public readonly PackageIdentity Identity;
            public readonly SourceRepository Source;

			public PackageIdentityWithSource(PackageIdentity identity, SourceRepository source)
			{
                Identity = identity;
                Source = source;
			}
		}

        private struct SerializableIdentityWithSource
		{
            public string Id;
            public string Version;
            public string Source;
		}

        public readonly Dictionary<string, List<PackageIdentity>> DependenciesForPackage = new Dictionary<string, List<PackageIdentity>>(StringComparer.OrdinalIgnoreCase);
        public List<PackageIdentityWithSource> GlobalPackageList { get; set; } = new List<PackageIdentityWithSource>();
        public List<(string, string)> NotFoundPackages { get; } = new List<(string, string)>();
        public List<(string, string)> DependenciesNotSatisfied { get; } = new List<(string, string)>();
        public Dictionary<PackageIdentity, Task<PackageContents>> PackageContents { get; } = new Dictionary<PackageIdentity, Task<PackageContents>>(PackageIdentityComparer.Default);

		internal static bool TryLoadFromCacheFile(string cacheFolder, NuGetFramework inputFramework, IEnumerable<string> adjustedNugetSources, Dictionary<string, MasterConfig.NuGetPackage> inputPackages, out PackageResolutionResult resolution, out string cacheFilePath)
		{
            int StringHash(string s)
            {
                int h = 17;
                foreach (char x in s)
                {
                    h = h * 31 + x;
                }
                return h;
            }

            // hash the inputs, for use as a file name to quick check if we've done this resolve before
            int hash = 17;
            unchecked
            {
                hash = hash * 31 + StringHash(inputFramework.ToString());
                foreach (string source in adjustedNugetSources)
                {
                    hash = hash * 31 + StringHash(source);
                }
                foreach (KeyValuePair<string, MasterConfig.NuGetPackage> package in inputPackages)
                {
                    int innerHash = 17;
                    innerHash = innerHash * 31 + StringHash(package.Value.Name);
                    innerHash = innerHash * 31 + StringHash(package.Value.Version);
                    hash += innerHash;
                }
            }

            // load the from cache if exists
            cacheFilePath = Path.Combine(cacheFolder, $"{hash}.nucache");
            if (File.Exists(cacheFilePath))
            {
				resolution = new PackageResolutionResult
				{
					GlobalPackageList = JsonConvert.DeserializeObject<SerializableIdentityWithSource[]>(File.ReadAllText(cacheFilePath))
					.Select(serialized => new PackageIdentityWithSource(new PackageIdentity(serialized.Id, NuGetVersion.Parse(serialized.Version)), null))
					.ToList()
				};
                return true;
			}
            else
			{
                resolution = null;
                return false;
			}
        }

        internal static void WriteCacheFile(string cacheFilePath, List<PackageIdentityWithSource> resolvedIndentitiesWithSource)
        {
            // TODO: noramlize sources to look-up to avoid repeated string
            string json = JsonConvert.SerializeObject
            (
                resolvedIndentitiesWithSource.Select
                (
                    package => new SerializableIdentityWithSource { Id = package.Identity.Id, Version = package.Identity.Version.ToNormalizedString(), Source = package.Source.PackageSource.Source }
                )
                .ToArray()
            );
            File.WriteAllText(cacheFilePath, json);
		}
    }

    public class NugetManagerLewis
    {
        public static NuGetFramework GetFrameworkString(string moniker)
        {
            NuGetFramework fw = NuGetFramework.Parse(moniker);
            if (fw.IsUnsupported)
            {
                // try harder
                if (moniker.StartsWith("v") && Version.TryParse(moniker.Substring(1), out var ver))
                {
                    fw = new NuGetFramework(FrameworkConstants.FrameworkIdentifiers.Net, ver);
                }
            }
            else
            {
                // maybe dnx?
                return fw;
            }
            return fw;
        }

        public static Task<PackageResolutionResult> ResolveNugetPackages(IEnumerable<KeyValuePair<string, MasterConfig.NuGetPackage>> inputsToPin, NuGetFramework frameworkConstraint, IEnumerable<string> sources, CancellationToken cancellationToken)
        {
            KeyValuePair<string, MasterConfig.NuGetPackage>[] inputs = inputsToPin.ToArray();
            return System.Threading.Tasks.Task.Run(async () =>
            {
                return await ResolveDependencyGraph(inputs, frameworkConstraint, sources, cancellationToken);
            });
        }

        public static Task<PackageContents> BeginAsyncDownload(PackageResolutionResult.PackageIdentityWithSource package, DirectoryInfo cacheDirectory, NuGetFramework frameworkConstraint, CancellationToken cancellationToken)
        {
            var downloadFolderInfo = cacheDirectory.CreateSubdirectory("Downloads");

            var settings = NuGet.Configuration.Settings.LoadDefaultSettings(null);
            var clientPolicy = ClientPolicyContext.GetClientPolicy(settings, NullLogger.Instance);

            var packagePathResolver = new PackagePathResolver(cacheDirectory.FullName);
            var packageExtractionContext = new PackageExtractionContext(
                PackageSaveMode.Defaultv3,
                XmlDocFileSaveMode.None,
                clientPolicy,
                NullLogger.Instance);

            var frameworkReducer = new FrameworkReducer();
            List<Task<PackageContents>> subtasks = new List<Task<PackageContents>>();

            SourceCacheContext cache = new SourceCacheContext(); // NUGET-TODO make a global / static (and dispose on exit / reset?)
            
            var downloaderContext = new PackageDownloadContext(cache, downloadFolderInfo.FullName, true);

            return System.Threading.Tasks.Task.Run(async () =>
            {
                var installedPath = packagePathResolver.GetInstalledPath(package.Identity);

                PackageReaderBase packageReader;

                if (installedPath == null)
                {
                    //TODO: log.Minimal.WriteLine("Downloading nuget {0} {1}...", package.Identity.Id, package.Identity.Version);
                    var downloadResource = await package.Source.GetResourceAsync<DownloadResource>(CancellationToken.None);
                    var downloadResult = await downloadResource.GetDownloadResourceResultAsync(
                        package.Identity,
                        downloaderContext,
                        cacheDirectory.FullName,
                        NullLogger.Instance, cancellationToken);

                    await PackageExtractor.ExtractPackageAsync(
                            downloadResult.PackageSource,
                            downloadResult.PackageStream,
                            packagePathResolver,
                            packageExtractionContext,
                            CancellationToken.None);

                    downloadResult.Dispose();

                    installedPath = packagePathResolver.GetInstalledPath(package.Identity);
                    packageReader = new PackageFolderReader(installedPath);
                }
                else
                {
                    packageReader = new PackageFolderReader(installedPath);
                }

                IEnumerable<string> ItemPatternMatch(IEnumerable<FrameworkSpecificGroup> items)
				{
					NuGetFramework matchingGroup = frameworkReducer.GetNearest(frameworkConstraint, items.Select(x => x.TargetFramework));
					return items.Where(x => x.TargetFramework.Equals(matchingGroup)).SelectMany(x => x.Items);
				}

                PackageContents contents = new PackageContents(package.Identity.Id, new DirectoryInfo(installedPath));
                contents.References.AddRange(ItemPatternMatch(packageReader.GetReferenceItems()));
                contents.Libraries.AddRange(ItemPatternMatch(packageReader.GetLibItems()));
                contents.Content.AddRange(ItemPatternMatch(packageReader.GetContentItems()));
                contents.BuildItems.AddRange(ItemPatternMatch(packageReader.GetBuildItems()));
                contents.Runtimes.AddRange(packageReader.GetFiles("runtimes"));

                packageReader.Dispose();

                return contents;
            });
        }

        static async Task<bool> GetDependenciesRecursive(PackageIdentity provokingPackage, NuGetFramework frameworkConstraint, SourceCacheContext cache, ILogger logger, IEnumerable<DependencyInfoResource> dependencyInfoResources, Dictionary<PackageIdentity, Task<SourcePackageDependencyInfo>> consideredSet, Dictionary<string, Task<IEnumerable<SourcePackageDependencyInfo>>> availableVersions, CancellationToken cancellation)
        {
            Task<SourcePackageDependencyInfo> innerTask = null;
            lock (consideredSet)
            {
                if (!consideredSet.TryGetValue(provokingPackage, out innerTask))
                {
                    innerTask = System.Threading.Tasks.Task.Run(async () =>
                    {
                        foreach (DependencyInfoResource dependencyInfoResource in dependencyInfoResources)
                        {
							SourcePackageDependencyInfo dependencyInfo = await dependencyInfoResource.ResolvePackage(provokingPackage, frameworkConstraint, cache, logger, cancellation);
                            if (dependencyInfo != null)
                            {
                                foreach (PackageDependency dependency in dependencyInfo.Dependencies)
                                {
                                    PackageIdentity dependencyIdentity = GetMinimumIdentityForDependency(dependency, frameworkConstraint, dependencyInfoResources, cache, logger, availableVersions, cancellation);

									bool success = await GetDependenciesRecursive(dependencyIdentity,
                                        frameworkConstraint, cache, logger, dependencyInfoResources, consideredSet, availableVersions, cancellation);

                                    if (success == false)
                                    {
                                        return null; // NUGET TODO
                                    }
                                }

                                return dependencyInfo;
                            }
                        }
                        return null; // MUGET TODO
                    });

                    consideredSet[provokingPackage] = innerTask;
                }
            }

            var result = await innerTask;
            return result != null;
        }

		private static PackageIdentity GetMinimumIdentityForDependency(PackageDependency dependency, NuGetFramework frameworkConstraint, IEnumerable<DependencyInfoResource> dependencyInfoResources, SourceCacheContext cache, ILogger logger, Dictionary<string, Task<IEnumerable<SourcePackageDependencyInfo>>> availableVersions, CancellationToken cancellation)
		{
            // NUGET-TODO - we could try asking for just minimum version from dependency here, and skip the below block

            Task<IEnumerable<SourcePackageDependencyInfo>> innerTask = null;
            lock (availableVersions)
            {
                if (!availableVersions.TryGetValue(dependency.Id, out innerTask))
                {
                    innerTask = System.Threading.Tasks.Task.Run(async () =>
                     {
                         HashSet<SourcePackageDependencyInfo> combinedVersionsFromAllSources = new HashSet<SourcePackageDependencyInfo>();
                         foreach (DependencyInfoResource nugetServerDependencyInfoSource in dependencyInfoResources)
                         {
                             IEnumerable<SourcePackageDependencyInfo> dependencyInfos = await nugetServerDependencyInfoSource.ResolvePackages(dependency.Id, frameworkConstraint, cache, logger, cancellation);
                             combinedVersionsFromAllSources.UnionWith(dependencyInfos);
                         }
                         return (IEnumerable<SourcePackageDependencyInfo>)combinedVersionsFromAllSources;
                     });

                    availableVersions[dependency.Id] = innerTask;
                }
            }

			IOrderedEnumerable<SourcePackageDependencyInfo> availableDependencyInfosInAscendingVersionOrder = innerTask.Result.OrderBy(depInfo => depInfo.Version); // NUGET-TODO should order when we cache
            foreach (SourcePackageDependencyInfo availableDependencyInfo in availableDependencyInfosInAscendingVersionOrder) 
            {
                if (dependency.VersionRange.Satisfies(availableDependencyInfo.Version))
				{
                    return new PackageIdentity(availableDependencyInfo.Id, availableDependencyInfo.Version);
                }
            }

            throw new Exception("no package found!"); // NUGET-TODO
		}

		private static async Task<PackageResolutionResult> ResolveDependencyGraph(IEnumerable<KeyValuePair<string, MasterConfig.NuGetPackage>> inputs, NuGetFramework frameworkConstraint, IEnumerable<string> sources, CancellationToken cancellationToken)
        {
            ILogger logger = NullLogger.Instance;

            IEnumerable<SourceRepository> repositories = sources.Select(x => Repository.Factory.GetCoreV3(x));

            using (SourceCacheContext cache = new SourceCacheContext())
            {
                var findByIdResource = repositories.Select(async x => await x.GetResourceAsync<FindPackageByIdResource>()).Select(x => x.Result).ToList();

                // This is all TPL because nuget is very TPL and (also) very slow!

                // Step 1: Try to locate the pinned packages from the nuget catalog server
                List<Task<((string, string) Request, PackageIdentity Identity)>> inputIdentities = new List<Task<((string, string), PackageIdentity)>>();
                foreach (KeyValuePair<string, MasterConfig.NuGetPackage> requests in inputs)
                {
                    inputIdentities.Add(System.Threading.Tasks.Task.Run(async () =>
                    {
                        string packageId = requests.Key;
                        NuGetVersion packageVersion = new NuGetVersion(requests.Value.Version);
                        foreach (FindPackageByIdResource finder in findByIdResource)
                        {
							FindPackageByIdDependencyInfo depInfo = await finder.GetDependencyInfoAsync(requests.Value.Name, packageVersion, cache, logger, cancellationToken);
                            if (depInfo?.PackageIdentity != null)
                            {
                                return ((packageId, requests.Value.Version), depInfo?.PackageIdentity);
                            }
                        }
                        return ((packageId, requests.Value.Version), null);
                    }));
                }

				// Step 2: Determine dependencies remotely by using a dependency framework constraint
				// Accumulate all packages which are referenced into a gigantic has set
				Dictionary<PackageIdentity, Task<SourcePackageDependencyInfo>> consideredSet = new Dictionary<PackageIdentity, Task<SourcePackageDependencyInfo>>(PackageIdentityComparer.Default);
                Dictionary<string, Task<IEnumerable<SourcePackageDependencyInfo>>> availableVersions = new Dictionary<string, Task<IEnumerable<SourcePackageDependencyInfo>>>(StringComparer.OrdinalIgnoreCase); // NUGET TODO - do I gain anything by caching this? cachecontext may handle this more or less for us

                PackageResolutionResult result = new PackageResolutionResult();

                List<System.Threading.Tasks.Task> continuations = new List<System.Threading.Tasks.Task>();
                var dependencyInfoResources = repositories.Select(async x => await x.GetResourceAsync<DependencyInfoResource>()).Select(x => x.Result).ToList();

                foreach (var identityTask in inputIdentities)
                {
                    continuations.Add(identityTask.ContinueWith(async (parent) =>
                    {
                        if (identityTask.Result.Identity?.HasVersion ?? false)
                        {
                            // we found the package, keep begin a deep dependency search
                            var dependencyScanResult = await GetDependenciesRecursive(identityTask.Result.Identity, frameworkConstraint, cache, logger, dependencyInfoResources, consideredSet, availableVersions, cancellationToken);
                            if (dependencyScanResult == false)
                            {
                                lock (result.DependenciesNotSatisfied)
                                {
                                    result.DependenciesNotSatisfied.Add(identityTask.Result.Request);
                                }
                            }
                        }
                        else
                        {
                            // missing dependency
                            lock (result.NotFoundPackages)
                            {
                                result.NotFoundPackages.Add(identityTask.Result.Request);
                            }
                        }
                    }).Unwrap());
                }

                // await all results
                await System.Threading.Tasks.Task.WhenAll(continuations.ToArray());
                var resolvedPinnedPackages = inputIdentities.Where(x => x.Result.Identity?.HasVersion == true);
                var computedSourceDependencies = consideredSet.Values.Select(x => x.Result);

				// resolve globally across all dependencies
				PackageResolverContext resolverContext = new PackageResolverContext(
                    DependencyBehavior.Lowest,
                    resolvedPinnedPackages.Select(x => x.Result.Identity.Id),
                    resolvedPinnedPackages.Select(x => x.Result.Identity.Id),
                    Enumerable.Empty<PackageReference>(),
                    resolvedPinnedPackages.Select(x => x.Result.Identity),
                    computedSourceDependencies,
                    repositories.Select(x => x.PackageSource),
                    NullLogger.Instance);

                var resolver = new PackageResolver();
				IEnumerable<PackageIdentity> resolvedIdentities = resolver.Resolve(resolverContext, CancellationToken.None);
				IEnumerable<SourcePackageDependencyInfo> packagesToInstall = resolvedIdentities.Select(p => computedSourceDependencies.Single(x => PackageIdentityComparer.Default.Equals(x, p)));

                // determine reference set for each input identity
                result.GlobalPackageList = packagesToInstall.Select(package => new PackageResolutionResult.PackageIdentityWithSource(package, package.Source)).ToList();

                // more async
                continuations.Clear();

                foreach (Task<((string, string) Request, PackageIdentity Identity)> resolvePinnedTask in resolvedPinnedPackages)
                {
                    continuations.Add(System.Threading.Tasks.Task.Run(() =>
                    {
						// using the newly globally solved set of "packages to install", we now ask the dependency solver for just one package's flattened dependency graph
						PackageResolverContext resolverContextMinimal = new PackageResolverContext
                        (
                            DependencyBehavior.Lowest,
                            new[] { resolvePinnedTask.Result.Identity.Id },
                            Enumerable.Empty<string>(),
                            Enumerable.Empty<PackageReference>(),
                            new[] { resolvePinnedTask.Result.Identity },
                            packagesToInstall,
                            repositories.Select(r => r.PackageSource),
                            NullLogger.Instance
                        );

                        // locally solve the dependencies
                        var resolverSub = new PackageResolver();
                        var thisProjectOnly = resolverSub.Resolve(resolverContextMinimal, CancellationToken.None)
                            .Select(p => packagesToInstall.Single(t => PackageIdentityComparer.Default.Equals(t, p)));

                        lock (result)
                        {
                            result.DependenciesForPackage.Add(resolvePinnedTask.Result.Request.Item1, thisProjectOnly.Cast<PackageIdentity>().ToList());
                        }
                    }));
                }

                await System.Threading.Tasks.Task.WhenAll(continuations);
                return result;
            }
        }
    }
}
*/