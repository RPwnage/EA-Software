using System;
using System.Collections.Generic;
using System.Linq;

using NuGet.Frameworks;
using NuGet.Protocol.Core.Types;
using NuGet.Resolver;

namespace NAnt.NuGet
{
	public abstract class NugetException : Exception
	{
		protected NugetException(string message) : base(message)
		{
		}

		protected NugetException(string message, Exception innerException) : base(message, innerException)
		{
		}
	}

	public sealed class MissingNugetPackageException : NugetException
	{
		internal MissingNugetPackageException(string package, string version, NuGetFramework framework, IEnumerable<NugetSource> sourceRepositories) :
			base($"Unable to find NuGet package '{package} {version}' for .NET target {framework.Framework}. Could not find package from any of the available sources: {String.Join(",", sourceRepositories.Select(source => source.Repository.PackageSource.Source))}.")
		{
		}
	}

	public sealed class UnresolvedNugetPackageException : NugetException
	{
		internal UnresolvedNugetPackageException(string package, NuGetFramework framework) :
			base($"Could not resolve version for NuGet package '{package}' for .NET target {framework.Framework}. It was not a pinned version or an implicit dependency of any pinned versions. Please add a <nugetpackage> entry for this package in your masterconfig.") // technically we shouldn't mention masterconfig here because that's from a higher architecture level, but it avoids catching and rethrowing this
		{
		}
	}

	public sealed class MissingNugetDependenciesException : NugetException
	{
		internal MissingNugetDependenciesException(IEnumerable<string> missingDependencies, NuGetFramework framework, NuGetResolverConstraintException resolveException) :
			base
			(
				$"Unresolvable NuGet package for .NET target {framework.Framework}." +
				(missingDependencies.Any() ?
					$" No matching identities were found for the following dependencies: {String.Join(", ", missingDependencies)}." :
					String.Empty),
				resolveException
			)
		{
		}
	}

	public sealed class DependencyResolutionFailedException : NugetException
	{
		internal DependencyResolutionFailedException(string requestPackageName, string requestPackageVersion, IEnumerable<string> triedSources, NuGetProtocolException protocolException) :
			base
			(
				$"Failed trying to get required dependencies of package {requestPackageName} {requestPackageVersion}." +
				(triedSources.Any() ?
					$" Failed to get package dependency info from any of these sources: {String.Join(", ", triedSources)}." :
					String.Empty),
				protocolException
			)
		{
		}

		internal DependencyResolutionFailedException(string requestPackageName, string requestPackageVersion, IEnumerable<string> triedSources) :
			base
			(
				$"Failed trying to get required dependencies of package {requestPackageName} {requestPackageVersion}." +
				(triedSources.Any() ?
					$" Failed to get package dependency info from any of these sources: {String.Join(", ", triedSources)}." :
					String.Empty)
			)
		{
		}
	}

	public sealed class AvailableVersionResolutionFailedException : NugetException
	{
		internal AvailableVersionResolutionFailedException(string packageName, IEnumerable<string> triedSources, NuGetProtocolException protocolException) :
			base
			(
				$"Failed trying to get available versions of package {packageName}." +
				(triedSources.Any() ?
					$" Failed to get available versions from any of these sources: {String.Join(", ", triedSources)}." :
					String.Empty),
				protocolException
			)
		{
		}
	}

	public sealed class InvalidNugetSourceException : NugetException
	{
		internal InvalidNugetSourceException(string requestPackageName, string requestPackageVersion, IEnumerable<string> triedSources, NuGetProtocolException protocolException) :
			base
			(
				$"Failed trying to resolve package {requestPackageName} {requestPackageVersion}." + 
				(triedSources.Any() ?
					$" Packages not found at the previous sources: {String.Join(", ", triedSources)}." :
					String.Empty),
				protocolException
			)
		{
		}
	}

	public sealed class UnresolvableNugetDependencyException : NugetException
	{
		internal UnresolvableNugetDependencyException(string dependencyPackage, IEnumerable<SourcePackageDependencyInfo> submittedVersions, IEnumerable<string> dependencyChains, NuGetResolverConstraintException resolveException) :
			base 
			(
				$"Failed to find version solution for package {dependencyPackage}. " +
				$"Possible versions were {String.Join(", ", submittedVersions.OrderByDescending(v => v.Version).Select(v => v.Version.OriginalVersion))}. " +
				$"Trying to resolve dependency chains from pinned versions:\n{String.Join("\n", dependencyChains)}",
				resolveException
			)
		{
		}
	}
}