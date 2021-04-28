using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

using NuGet.Common;
using NuGet.Frameworks;
using NuGet.Packaging.Core;
using NuGet.Protocol.Core.Types;
using NuGet.Versioning;

namespace NAnt.NuGet
{
	internal class NugetSource
	{
		internal readonly SourceRepository Repository;

		private readonly Lazy<Task<FindPackageByIdResource>> m_findPackageByIdResource;
		private readonly Lazy<Task<DependencyInfoResource>> m_dependencyInfoResource;

		internal NugetSource(SourceRepository sourceRepository)
		{
			Repository = sourceRepository;

			m_findPackageByIdResource = new Lazy<Task<FindPackageByIdResource>>(() => Repository.GetResourceAsync<FindPackageByIdResource>());
			m_dependencyInfoResource = new Lazy<Task<DependencyInfoResource>>(() => Repository.GetResourceAsync<DependencyInfoResource>());
		}

		internal async Task<PackageIdentity> GetRemoteIdentity(string package, NuGetVersion version, IEnumerable<string> previouslyTriedSources, SourceCacheContext cacheContext, CancellationToken cancellationToken)
		{
			FindPackageByIdResource finder = await GetResourceWithCatchSpecificOrThrow
			(
				m_findPackageByIdResource,
				(NuGetProtocolException protocolError) => new InvalidNugetSourceException(package, version.OriginalVersion, previouslyTriedSources, protocolError)
			);
			FindPackageByIdDependencyInfo depInfo = await finder.GetDependencyInfoAsync(package, version, cacheContext, NullLogger.Instance, cancellationToken);
			return depInfo?.PackageIdentity;
		}

		internal async Task<SourcePackageDependencyInfo> GetDependencyInfo(PackageIdentity provokingPackage, NuGetFramework framework, IEnumerable<string> triedSources, SourceCacheContext cacheContext, CancellationToken cancellationToken)
		{
			DependencyInfoResource infoResource = await GetResourceWithCatchSpecificOrThrow
			(
				m_dependencyInfoResource,
				(NuGetProtocolException protocolError) => new DependencyResolutionFailedException(provokingPackage.Id, provokingPackage.Version.OriginalVersion, triedSources)
			);

			// can return null
			return await infoResource.ResolvePackage(provokingPackage, framework, cacheContext, NullLogger.Instance, cancellationToken);
		}

		internal async Task<IEnumerable<SourcePackageDependencyInfo>> GetDependencyInfos(string package, NuGetFramework framework, IEnumerable<string> triedSources, SourceCacheContext cacheContext, CancellationToken cancellationToken)
		{
			DependencyInfoResource infoResource = await GetResourceWithCatchSpecificOrThrow
			(
				m_dependencyInfoResource,
				(NuGetProtocolException protocolError) => new AvailableVersionResolutionFailedException(package, triedSources, protocolError)
			);

			// can return null
			return await infoResource.ResolvePackages(package, framework, cacheContext, NullLogger.Instance, cancellationToken);
		}

		private async Task<TResult> GetResourceWithCatchSpecificOrThrow<TResult, TCatchException, THandleException>(Lazy<Task<TResult>> lazyResource, Func<TCatchException, THandleException> handleError) where TCatchException : Exception where THandleException : Exception
		{
			TResult result;
			try
			{
				result = await lazyResource.Value;
			}
			catch (TCatchException protocolError)
			{
				throw handleError(protocolError);
			}
			catch (AggregateException aggregateEx)
			{
				// add some extra context data if this fails to resolve remote source
				if (aggregateEx.Flatten().InnerExceptions.FirstOrDefault(ex => ex is TCatchException) is TCatchException protocolError)
				{
					throw handleError(protocolError);
				}
				throw;
			}
			return result;
		}
	}
}
