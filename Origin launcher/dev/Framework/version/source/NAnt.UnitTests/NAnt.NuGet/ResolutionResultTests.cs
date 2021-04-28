using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NuGet.Configuration;
using NuGet.Frameworks;
using NuGet.Packaging.Core;
using NuGet.Protocol.Core.Types;
using NuGet.Versioning;

using NAnt.Tests.Util;

namespace NAnt.NuGet.Tests
{
	[TestClass]
	public class ResolutionResultTests
	{
		[TestMethod]
		public void SerializeDeserialize()
		{
			PackageIdentity testIdentity = new PackageIdentity("Test", new NuGetVersion("1.0.0"));
			PackageIdentity dependencyIdentity = new PackageIdentity("Dependency", new NuGetVersion("1.0.0"));
			NugetSource testSource = new NugetSource(new SourceRepository(new PackageSource("testsource"), new INuGetResourceProvider[] { }));
			NugetSource[] testRepositories = new NugetSource[] { testSource };
			NuGetFramework framework = NuGetFramework.Parse("net472");

			ResolutionResult serialize = new ResolutionResult
			(
				framework,
				testRepositories,
				new Dictionary<PackageIdentity, NugetSource>
				{ 
					{ testIdentity, testSource },
					{ dependencyIdentity, testSource }
				},
				new Dictionary<string, List<PackageIdentity>> { { "Test", new List<PackageIdentity> { testIdentity, dependencyIdentity } } },
				new (string, string)[] { ("Missing", "1.0.0") }
			);

			using (TestUtil.TestDirectory testCacheDir = new TestUtil.TestDirectory("nugetResolutionResultTest"))
			{
				string testCacheFile = Path.Combine(testCacheDir, Path.GetRandomFileName() + ".nucache");
				serialize.WriteToCacheFile(testCacheFile);

				Assert.IsTrue(ResolutionResult.TryLoadFromCacheFile(framework, testCacheFile, testRepositories, out ResolutionResult deserialize));

				IEnumerable<PackageIdentity> identities = deserialize.GetResolvedPackageIdentities(new string[] { "Test" });
				Assert.IsTrue(identities.Contains(testIdentity));
				Assert.IsTrue(identities.Contains(dependencyIdentity));

				Assert.AreEqual(testSource, deserialize.GetSourceforIdentity(testIdentity));
				Assert.AreEqual(testSource, deserialize.GetSourceforIdentity(dependencyIdentity));

				Assert.ThrowsException<MissingNugetPackageException>(() => deserialize.GetResolvedPackageIdentities(new string[] { "Missing" }));
				Assert.ThrowsException<UnresolvedNugetPackageException>(() => deserialize.GetResolvedPackageIdentities(new string[] { "NeverRequested" }));
			}
		}

		[TestMethod]
		public void DerializeEmptyDoesntExcept()
		{
			using (TestUtil.TestDirectory testCacheDir = new TestUtil.TestDirectory("nugetResolutionResultTest"))
			{
				string testCacheFile = Path.Combine(testCacheDir, Path.GetRandomFileName() + ".nucache");
				File.WriteAllText(testCacheFile, String.Empty);

				Assert.IsFalse(ResolutionResult.TryLoadFromCacheFile(NuGetFramework.Parse("net472"), testCacheFile, Enumerable.Empty<NugetSource>(), out ResolutionResult deserialize));
			}
		}

		[TestMethod]
		public void DerializeCorruptDoesntExcept()
		{
			using (TestUtil.TestDirectory testCacheDir = new TestUtil.TestDirectory("nugetResolutionResultTest"))
			{
				string testCacheFile = Path.Combine(testCacheDir, Path.GetRandomFileName() + ".nucache");
				File.WriteAllText(testCacheFile, @"{""identitiesToSources"":[{""id"":""Test"",""version"":""1.0.0"",""source"":""testsource""},{""id"":""Dependency"",""version"":""1.0.0"",""source"":""testsource""}],");

				Assert.IsFalse(ResolutionResult.TryLoadFromCacheFile(NuGetFramework.Parse("net472"), testCacheFile, Enumerable.Empty<NugetSource>(), out ResolutionResult deserialize));
			}
		}
	}
}
