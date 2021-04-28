using Microsoft.VisualStudio.TestTools.UnitTesting;
using NAnt.Core.Util;
using NAnt.Tests.Util;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace NAnt.NuGet.Tests
{
	[TestClass]
	public class NuGetManagerTests
	{
		[TestMethod]
		public void NoDependenciesPackageTest()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(), // use default feed (yes, not a unit test anymore...)
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "log4net", "2.0.10" } }
				);

				IEnumerable<NugetPackage> packages = nugetManager.GetRequiredPackages("net472", new string[] { "log4net" });
				Assert.AreEqual(1, packages.Count());
				Assert.AreEqual("log4net", packages.First().Id);

				Assert.AreEqual(packages.First().CompileItems.Count(), 1);
				Assert.IsTrue(packages.First().CompileItems.First().FullName.EndsWith("log4net.dll", StringComparison.OrdinalIgnoreCase));
			}
		}

		[TestMethod]
		public void PackageWithDependenciesTest()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(),
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "NuGet.Client", "4.2.0" } }
				);

				IEnumerable<NugetPackage> packages = nugetManager.GetRequiredPackages("net472", new string[] { "NuGet.Client" });
				Assert.AreEqual(11, packages.Count());
			}
		}

		[TestMethod]
		public void TestCacheReuse()
		{
			using (TestUtil.TestDirectory testDir = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(testDir, "download");
				string testCacheFolder = Path.Combine(testDir, "cache");
				NugetManager nugetManagerOne = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(),
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "NuGet.Client", "4.2.0" } }
				);
				nugetManagerOne.GetRequiredPackages("net472", new string[] { "NuGet.Client" });

				// should be one cache file after getting required packages
				string[] afterOneCacheFiles = Directory.GetFiles(testDir, "*.nucache", SearchOption.AllDirectories);
				Assert.AreEqual(1, afterOneCacheFiles.Count());

				string afterOneCacheFilePath = afterOneCacheFiles.Single();
				DateTime afterOneCacheWriteTime = File.GetLastWriteTime(afterOneCacheFilePath);

				// second nuget manager with same directory should find the cache file
				NugetManager nugetManagerTwo = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(),
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "NuGet.Client", "4.2.0" } }
				);
				nugetManagerTwo.GetRequiredPackages("net472", new string[] { "NuGet.Client" });

				// should still only have on cache file... with the same timestamp
				string[] afterTwoCacheFiles = Directory.GetFiles(testDir, "*.nucache", SearchOption.AllDirectories);
				Assert.AreEqual(1, afterTwoCacheFiles.Count());

				// ... at the same path ...
				string afterTwoCacheFilePath = afterTwoCacheFiles.Single();
				Assert.AreEqual(afterOneCacheFilePath, afterTwoCacheFilePath);

				// ... with the same timestamp
				DateTime afterTwoCacheWriteTime = File.GetLastWriteTime(afterTwoCacheFilePath);
				Assert.AreEqual(afterOneCacheWriteTime, afterTwoCacheWriteTime);
			}
		}

		[TestMethod]
		public void NonexistantPackage()
		{
			using (TestUtil.TestDirectory testDir = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(testDir, "download");
				string testCacheFolder = Path.Combine(testDir, "cache");
				NugetManager nugetManagerOne = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(),
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "NotAPackageThatExists", "3.2.1" } }
				);
				Assert.ThrowsException<MissingNugetPackageException>(() => nugetManagerOne.GetRequiredPackages("net472", new string[] { "NotAPackageThatExists" }));
			}
		}

		[TestMethod]
		public void OnlyErrorsWhenGettingUnresolvable()
		{
			using (TestUtil.TestDirectory testDir = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(testDir, "download");
				string testCacheFolder = Path.Combine(testDir, "cache");
				NugetManager nugetManagerOne = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(),
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "NuGet.Client", "4.2.0" }, { "NotAPackageThatExists", "3.2.1" } }
				);

				// this shoulud succeed, this is a real package
				nugetManagerOne.GetRequiredPackages("net472", new string[] { "NuGet.Client" });

				// this should fail only now when we ask for a package that doesn't exist
				Assert.ThrowsException<MissingNugetPackageException>(() => nugetManagerOne.GetRequiredPackages("net472", new string[] { "NotAPackageThatExists" }));
			}
		}

		[TestMethod]
		public void UnresolvableDependencies()
		{
			using (TestUtil.TestDirectory testDir = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(testDir, "download");
				string testCacheFolder = Path.Combine(testDir, "cache");
				NugetManager nugetManagerOne = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					new string[] { "https://artifactory.ap.ea.com/artifactory/api/nuget/firemonkeysqe-nuget-local" },
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "Devil.Core", "2.0.4-frostbite" } }
				);

				// devil package requires dependencies from public nuget, we haven't specifed public nuget as a source so this should fail to find dependencies
				Assert.ThrowsException<MissingNugetDependenciesException>(() => nugetManagerOne.GetRequiredPackages("net472", new string[] { "Devil.Core" }));
			}
		}

		[TestMethod]
		public void VersionNotCoveredByResolution()
		{
			using (TestUtil.TestDirectory testDir = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(testDir, "download");
				string testCacheFolder = Path.Combine(testDir, "cache");
				NugetManager nugetManagerOne = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(),
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "NuGet.Client", "4.2.0" } }
				);

				// ask for a package that we didn't pin a version for, and wouldn't have been resolved as a dependency
				// of a pinned version
				Assert.ThrowsException<UnresolvedNugetPackageException>(() => nugetManagerOne.GetRequiredPackages("net472", new string[] { "NotAPackageWePinnedNorDependencyOfAPin" }));
			}
		}

		[TestMethod]
		public void InvalidSource()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					new string[] { "https://not.a.real.source.org/v3/index.json" },
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "NuGet.Client", "4.2.0" } }
				);

				Assert.ThrowsException<InvalidNugetSourceException>(() => nugetManager.GetRequiredPackages("net472", new string[] { "NuGet.Client" }));
			}
		}

		[TestMethod]
		public void InvalidOrOfflineUnusedSourceDoesNotBreakResolutionIfValidSourceListedFirst()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					new string[] { "https://api.nuget.org/v3/index.json", "https://not.a.real.source.org/v3/index.json" },
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "NuGet.Client", "4.2.0" } }
				);

				nugetManager.GetRequiredPackages("net472", new string[] { "NuGet.Client" });
			}
		}

		[TestMethod]
		public void UnresolvableDependenciesWhenSourceIsInvalid()
		{
			using (TestUtil.TestDirectory testDir = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(testDir, "download");
				string testCacheFolder = Path.Combine(testDir, "cache");
				NugetManager nugetManagerOne = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					new string[] { "https://artifactory.ap.ea.com/artifactory/api/nuget/firemonkeysqe-nuget-local", "https://not.a.real.source.org/v3/index.json" },
					GetRuntimeGraphPath(),
					new Dictionary<string, string> { { "Devil.Core", "2.0.4-frostbite" } }
				);

				// devil package requires dependencies from public nuget, we haven't specifed public nuget as a source so this should fail to find dependencies
				Assert.ThrowsException<AvailableVersionResolutionFailedException>(() => nugetManagerOne.GetRequiredPackages("net472", new string[] { "Devil.Core" }));
			}
		}

		[TestMethod]
		public void NativeRuntimesExact()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(), // use default feed (yes, not a unit test anymore...)
					GetRuntimeGraphPath(),
					pinnedCorePackageVersions: new Dictionary<string, string> { { "p4api.net.core", "2020.2.207.5590" } }			 
				);

				IEnumerable<NugetPackage> packagesX86 = nugetManager.GetRequiredPackages("net5.0", new string[] { "p4api.net.core" }, new string[] { "win-x86" });

				Assert.AreEqual(1, packagesX86.Count());
				Assert.AreEqual("p4api.net.core", packagesX86.First().Id);
				Assert.AreEqual(1, packagesX86.First().RuntimeItems.Count());
				string expectedX86Ending = PathNormalizer.Normalize("win-x86\\native\\p4bridge.dll", getFullPath: false);
				string recievedX86 = PathNormalizer.Normalize(packagesX86.First().RuntimeItems.First().FullName);
				Assert.IsTrue(recievedX86.EndsWith(expectedX86Ending));

				IEnumerable<NugetPackage> packagesX64 = nugetManager.GetRequiredPackages("net5.0", new string[] { "p4api.net.core" }, new string[] { "win-x64" });

				Assert.AreEqual(1, packagesX64.Count());
				Assert.AreEqual("p4api.net.core", packagesX64.First().Id);
				Assert.AreEqual(1, packagesX64.First().RuntimeItems.Count());
				string expectedX64Ending = PathNormalizer.Normalize("win-x64\\native\\p4bridge.dll", getFullPath: false);
				string recievedX64 = PathNormalizer.Normalize(packagesX64.First().RuntimeItems.First().FullName);
				Assert.IsTrue(recievedX64.EndsWith(expectedX64Ending));
			}
		}

		[TestMethod]
		public void NativeRuntimesFallback()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(), // use default feed (yes, not a unit test anymore...)
					GetRuntimeGraphPath(),
					pinnedCorePackageVersions: new Dictionary<string, string> { { "p4api.net.core", "2020.2.207.5590" } }
				);

				// win10-x64-aot should fall back to win-x64
				IEnumerable<NugetPackage> packages = nugetManager.GetRequiredPackages("net5.0", new string[] { "p4api.net.core" }, new string[] { "win10-x64-aot" });

				Assert.AreEqual(1, packages.Count());
				Assert.AreEqual("p4api.net.core", packages.First().Id);
				Assert.AreEqual(1, packages.First().RuntimeItems.Count());
				string expectedEnding = PathNormalizer.Normalize("win-x64\\native\\p4bridge.dll", getFullPath: false);
				string recieved = PathNormalizer.Normalize(packages.First().RuntimeItems.First().FullName);
				Assert.IsTrue(recieved.EndsWith(expectedEnding));
			}
		}

		[TestMethod]
		public void PinnedDependencyIndirectIncompatbility()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(), // use default feed (yes, not a unit test anymore...)
					GetRuntimeGraphPath(),
					pinnedFrameworkPackageVersions: new Dictionary<string, string>
					{
						{ "Microsoft.CodeAnalysis.CSharp.Workspaces", "1.3.2" },
						{ "Microsoft.CodeAnalysis.Scripting" , "3.6.0" },
						{ "Microsoft.Extensions.Logging", "3.1.4" },
						{ "Microsoft.Extensions.Primitives", "3.1.4" },
						{ "System.Runtime.CompilerServices.Unsafe", "4.6.0" }
					}
				);

				Assert.ThrowsException<UnresolvableNugetDependencyException>(() => nugetManager.GetRequiredPackages("net472", new string[] { "System.Runtime.CompilerServices.Unsafe" }));
			}
		}

		[TestMethod]
		public void PinnedDependencyDirectIncompatibiliy()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(), // use default feed (yes, not a unit test anymore...)
					GetRuntimeGraphPath(),
					pinnedCorePackageVersions: new Dictionary<string, string> 
					{
						{ "Microsoft.CodeAnalysis.CSharp.Workspaces", "1.3.2" },
						{ "Microsoft.CodeAnalysis.Scripting" , "3.6.0" }
					}
				);

				Assert.ThrowsException<UnresolvableNugetDependencyException>(() => nugetManager.GetRequiredPackages("netcoreapp3.1", new string[] { "Microsoft.CodeAnalysis.Scripting" }));
			}
		}

		[TestMethod]
		public void UnsupportedFrameworkWarning()
		{
			using (TestUtil.TestDirectory test = new TestUtil.TestDirectory("nugetManagerTest"))
			{
				string testDownloadFolder = Path.Combine(test, "download");
				string testCacheFolder = Path.Combine(test, "cache");
				NugetManager nugetManager = new NugetManager
				(
					testDownloadFolder,
					testCacheFolder,
					Enumerable.Empty<string>(), // use default feed (yes, not a unit test anymore...)
					GetRuntimeGraphPath(),
					pinnedCorePackageVersions: new Dictionary<string, string>
					{
						{ "AvalonEdit", "5.0.2" },
						{ "System.Runtime", "4.3.0" }
					}
				);

				// should succeed withot exception
				nugetManager.GetRequiredPackages("net5.0", new string[] { "System.Runtime" });

				// should fail
				Assert.ThrowsException<UnsupportFrameworkNugetPackageException>(() => nugetManager.GetRequiredPackages("net5.0", new string[] { "AvalonEdit" }));
			}
		}

		private static string GetRuntimeGraphPath()
		{
			return Path.Combine
			(
				Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
				"NAnt.NuGet\\RuntimeIdentifierGraph.json"
			);
		}
	}
}