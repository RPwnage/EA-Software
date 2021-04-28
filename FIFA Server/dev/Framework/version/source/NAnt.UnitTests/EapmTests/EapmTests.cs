// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
using System.IO;
using System.Text.RegularExpressions;
using System.Xml;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core;
using NAnt.Core.Util;

namespace EapmTests
{
	[TestClass]
	public class EapmTests : EapmTestBase
	{
		//[TestMethod]
		//public void WebPackageServerInstallExplicitVersionTest() // test installing explicit version from web server
		//{
		//	const string testPackage = "EABase";
		//	const string testVersion = "2.09.04";

		//	string packageVersionDownloadDirectory = Path.Combine(m_testDir, testPackage, testVersion);
		//	Assert.IsFalse(Directory.Exists(packageVersionDownloadDirectory), "Download folder existed before install. Test invalid!");

		//	bool serverProblem = false;
		//	string recordedOutput = null;
		//	try
		//	{
		//		recordedOutput = RunEapm("install", testPackage, testVersion, String.Format("-ondemandroot:{0}", m_testDir));
		//	}
		//	catch (Exception ex)
		//	{
		//		if (ex.Message.Contains("Could not establish trust relationship for the SSL/TLS secure channel."))
		//		{
		//			serverProblem = true;
		//		}
		//		else
		//		{
		//			throw;
		//		}
		//	}

		//	if (!serverProblem)
		//	{
		//		string packageInstalledString = String.Format("Package '{0}-{1}' installed.", testPackage, testVersion);
		//		Assert.IsTrue(recordedOutput.Contains(packageInstalledString), String.Format("Didn't see '{0}' in eapm output:\n{1}", packageInstalledString, recordedOutput));
		//		Assert.IsTrue(Directory.Exists(packageVersionDownloadDirectory), "Package was not installed to expected location. eapm output:\n" + recordedOutput);
		//	}
		//}

		//[TestMethod]
		//public void WebPackageServerInstallLatestTest() // test installing implicit latest version from web server
		//{
		//	const string testPackage = "EABase";

		//	string packageDownloadDirectory = Path.Combine(m_testDir, testPackage);
		//	Assert.IsFalse(Directory.Exists(packageDownloadDirectory), "Download folder existed before install. Test invalid!");

		//	bool serverProblem = false;
		//	string recordedOutput = null;
		//	try
		//	{
		//		recordedOutput = RunEapm("install", testPackage, String.Format("-ondemandroot:{0}", m_testDir));
		//	}
		//	catch (Exception ex)
		//	{
		//		if (ex.Message.Contains("Could not establish trust relationship for the SSL/TLS secure channel."))
		//		{
		//			serverProblem = true;
		//		}
		//		else
		//		{
		//			throw;
		//		}
		//	}

		//	if (!serverProblem)
		//	{
		//		string packageInstalledPattern = String.Format(@"Package '{0}-[0-9\.]+' installed.", testPackage);
		//		Assert.IsTrue(new Regex(packageInstalledPattern).IsMatch(recordedOutput), String.Format("Didn't see pattern '{0}' in eapm output:\n{1}", packageInstalledPattern, recordedOutput));
		//		Assert.IsTrue(Directory.Exists(packageDownloadDirectory), string.Format("Package was not installed to expected location ({0}). eapm output:\n{1}", packageDownloadDirectory, recordedOutput));
		//	}
		//}

//		[TestMethod]
//		public void WebPackageServerInstallMasterconfigTest() // test installing masterconfig version from web server
//		{
//			const string testPackage = "EABase"; // eapm init requires a config package version, if you change this add eaconfig to masterconfig below
//			const string testVersion = "2.09.04";
//			const string onDemandDir = "ondemand";

//			// setup masterconfig with a perforce package uri potint to local test server
//			string masterConfigPath = Path.Combine(m_testDir, "masterconfig.xml");
//			string masterConfigContents = String.Format
//			(
//@"<project>
//<masterversions>
//	<package name=""{0}"" version=""{1}""/>
//</masterversions>
//<ondemand>true</ondemand>
//<ondemandroot>{2}</ondemandroot>
//</project>"
//				,
//				testPackage,
//				testVersion,
//				onDemandDir
//			);
//			File.WriteAllText(masterConfigPath, masterConfigContents);

//			string masterconfigDownloadDirectory = Path.Combine(m_testDir, onDemandDir, testPackage, testVersion);
//			Assert.IsFalse(Directory.Exists(masterconfigDownloadDirectory), "Download folder existed before install. Test invalid!");

//			bool serverProblem = false;
//			string recordedOutput = null;
//			try
//			{
//				recordedOutput = RunEapm("install", testPackage, testVersion, String.Format("-masterconfigfile:{0}", masterConfigPath));
//			}
//			catch (Exception ex)
//			{
//				if (ex.Message.Contains("Could not establish trust relationship for the SSL/TLS secure channel."))
//				{
//					serverProblem = true;
//				}
//				else
//				{
//					throw;
//				}
//			}

//			if (!serverProblem)
//			{
//				string packageInstalledString = String.Format("Package '{0}-{1}' installed.", testPackage, testVersion);
//				Assert.IsTrue(recordedOutput.Contains(packageInstalledString), String.Format("Didn't see '{0}' in eapm output:\n{1}", packageInstalledString, recordedOutput));
//				Assert.IsTrue(Directory.Exists(masterconfigDownloadDirectory), "Package was not installed to expected location. eapm output:\n" + recordedOutput);
//			}
//		}

//		[TestMethod]
//		public void WebPackageServerInstallExplicitVersionMasterConfigTest() // test installing explicit version from web server, but using masterconfig to set on demand root
//		{
//			const string testPackage = "EASTL"; // eapm init requires a config package version, if you change this add eaconfig to masterconfig below
//			const string testVersion = "3.02.00";
//			const string masterConfigVersion = "2.11.01"; // masterconfig specifies different version than we want to download
//			const string onDemandDir = "ondemand";

//			// setup masterconfig with a perforce package uri potint to local test server
//			string masterConfigPath = Path.Combine(m_testDir, "masterconfig.xml");
//			string masterConfigContents = String.Format
//			(
//@"<project>
//<masterversions>
//	<package name=""{0}"" version=""{1}""/>
//</masterversions>
//<ondemand>true</ondemand>
//<ondemandroot>{2}</ondemandroot>
//</project>"
//				,
//				testPackage,
//				masterConfigVersion,
//				onDemandDir
//			);
//			File.WriteAllText(masterConfigPath, masterConfigContents);

//			string packageDownloadDirectory = Path.Combine(m_testDir, onDemandDir, testPackage, testVersion);
//			Assert.IsFalse(Directory.Exists(packageDownloadDirectory), "Download folder existed before install. Test invalid!");

//			bool serverProblem = false;
//			string recordedOutput = null;
//			try
//			{
//				recordedOutput = RunEapm("install", testPackage, testVersion, String.Format("-masterconfigfile:{0}", masterConfigPath));
//			}
//			catch (Exception ex)
//			{
//				if (ex.Message.Contains("Could not establish trust relationship for the SSL/TLS secure channel."))
//				{
//					serverProblem = true;
//				}
//				else
//				{
//					throw;
//				}
//			}

//			// If we detected failure is one of the detected server problem, just ignore validating anything for now.
//			if (!serverProblem)
//			{
//				ValidateInstall(testPackage, testVersion, packageDownloadDirectory, recordedOutput);
//			}
//		}

		// TODO requires P4TestServer to be migrated into our testing utilities
		/*[TestMethod]
		public void PerforceInstallTest() // test perforce install from p4 uri
			{
			using (P4TestServer testServer = P4TestServer.StartNewTestServer())
				{
				Dictionary<string, string> clientMapping = new Dictionary<string, string> { { String.Format("//{0}/...", P4TestServer.DefaultDepot), "//..." } };
				using (P4TestClient testClient = testServer.CreateTestClient(clientMapping))
					{
					const string perforcePackage = "testpackage";
					const string perforcePackageVersion = "dev";

					// create test package on local perforce test server
					string basePackagePath = String.Format("{0}/{1}", perforcePackage, perforcePackageVersion);
					P4TestServer.PopulateWithTestFile
					(
						basePackagePath,
						"Manifest.xml",
						String.Format
						(
@"<package>
<manifestVersion>2</manifestVersion>
<frameworkVersion>2</frameworkVersion>
<buildable>true</buildable>
<versionName>{0}</versionName>
</package>",
							perforcePackageVersion
						),
						testClient.Client
					);

					// setup masterconfig with a perforce package uri potint to local test server
					string perforceMasterConfigPath = Path.Combine(m_testDir, "masterconfig.xml");
					string perforcePackageUri = String.Format("p4://localhost:1999/{0}/{1}?head", P4TestServer.DefaultDepot, basePackagePath);
					string perforceMasterConfigContents = String.Format
					(
@"<project>
<masterversions>
	<package name=""{0}"" version=""{1}"" uri=""{2}""/>
</masterversions>
<ondemand>true</ondemand>
<ondemandroot>{3}</ondemandroot>
<globalproperties>
	nant.warningsuppression=2030
</globalproperties>
<config package=""eaconfig""/>
</project>"
						,
						perforcePackage,
						perforcePackageVersion,
						perforcePackageUri,
						m_testDir
					);
					File.WriteAllText(perforceMasterConfigPath, perforceMasterConfigContents);

					string perforceDownloadDirectory = Path.Combine(m_testDir, perforcePackage, perforcePackageVersion);
					Assert.IsFalse(Directory.Exists(perforceDownloadDirectory), "Download folder existed before install. Test invalid!");

					string recordedOutput = RunEapm("install", perforcePackage, String.Format("-masterconfigfile:{0}", perforceMasterConfigPath));

					string packageInstalledString = String.Format("Package '{0}-{1}' installed.", perforcePackage, perforcePackageVersion);
					Assert.IsTrue(recordedOutput.Contains(packageInstalledString), String.Format("Didn't see '{0}' in eapm output:\n{1}", packageInstalledString, recordedOutput));
					Assert.IsTrue(Directory.Exists(perforceDownloadDirectory), "Package was not installed to expected location. eapm output:\n" + recordedOutput);
					}
			}
		}*/

		/// <summary>Test that running the help command works</summary>
		[TestMethod]
		public void HelpMessage()
		{
			string recordedOutput = RunEapm("help");

			Assert.IsTrue(recordedOutput.Contains("Usage: eapm <command> [arg ...]"));
		}

		/// <summary>Test that eapm where returns the path to a package in the masterconfig</summary>
		[TestMethod]
		public void EapmWhereTest()
		{
			CreatePackage("mypackage", "test");
			CreateMasterconfig(m_testDir, "masterconfig.xml");

			string recordedOutput = RunEapm("where", "mypackage", "-masterconfigfile:" + Path.Combine(m_testDir, "masterconfig.xml"));

			Assert.IsTrue(recordedOutput.Contains(Path.Combine(m_testDir, "mypackage", "test")));
		}

		/// <summary>Test that eapm where fails if the package cannot be found</summary>
		[TestMethod]
		public void EapmWhereTestMissingPackage()
		{
			CreatePackage("mypackage", "test");
			CreateMasterconfig(Path.Combine(m_testDir), "masterconfig.xml");

			string recordedOutput = RunEapm(1, new string[] { "where", "not_my_package", "-masterconfigfile:" + Path.Combine(m_testDir, "masterconfig.xml") });

			Assert.IsTrue(recordedOutput.Contains("Requested package 'not_my_package' was not listed in the masterconfig!"));
		}

		/// <summary>Run eapm prune on an empty ondemand directory</summary>
		[TestMethod]
		public void EapmPruneTestEmptyDirectory()
		{
			string recordedOutput = RunEapm("prune", m_testDir);

			Assert.IsTrue(recordedOutput.Contains("Package Cleanup Complete: 0 deleted, 0 warning, 0 kept"));
			Assert.IsFalse(Directory.Exists(Path.Combine(m_testDir, "ondemand_metadata")));
		}

		/// <summary>
		/// Run eapm prune on a directory with packages but that don't contain metadata needed for pruning.
		/// Should not fail, should not delete any packages but should write metadata files for these packages.
		/// </summary>
		[TestMethod]
		public void EapmPruneTestNoMetadata()
		{
			CreatePackage("packageA", "test");
			CreatePackage("packageB", "test");

			string recordedOutput = RunEapm("prune", m_testDir);

			Assert.IsTrue(recordedOutput.Contains("[warning] The ondemand release 'packageA-test' has no ondemand metadata file, " +
				"cannot determine the last time this release was referenced. " +
				"Generating a new metadata file for this package."));
			Assert.IsTrue(recordedOutput.Contains("[warning] The ondemand release 'packageB-test' has no ondemand metadata file, " +
				"cannot determine the last time this release was referenced. " +
				"Generating a new metadata file for this package."));
			Assert.IsTrue(Directory.Exists(Path.Combine(m_testDir, "ondemand_metadata")));
			Assert.IsTrue(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageA-test.metadata.xml")));
			Assert.IsTrue(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageB-test.metadata.xml")));

			XmlDocument document = new XmlDocument();
			document.Load(Path.Combine(m_testDir, "ondemand_metadata", "packageA-test.metadata.xml"));
			XmlNode packageNode = document.GetChildElementByName("package");
			XmlNode nameNode = packageNode.GetChildElementByName("name");
			XmlNode versionNode = packageNode.GetChildElementByName("version");
			XmlNode directoryNode = packageNode.GetChildElementByName("directory");
			XmlNode referencedNode = packageNode.GetChildElementByName("referenced");

			Assert.AreEqual(nameNode.InnerText, "packageA");
			Assert.AreEqual(versionNode.InnerText, "test");
			Assert.AreEqual(directoryNode.InnerText, Path.Combine(m_testDir, "packageA", "test"));
			Assert.AreEqual(referencedNode.GetAttributeValue("culture"), System.Globalization.CultureInfo.CurrentCulture.Name, "The generated 'culture' referenced node info doesn't match up with current running system's culture name.");
			Assert.IsFalse(referencedNode.InnerText.IsNullOrEmpty());
		}

		/// <summary>
		/// Test that when the packages contain metadata files they are removed if the metadata indicates that they are older than the threshold
		/// </summary>
		[TestMethod]
		public void EapmPruneTestWithMetadata()
		{
			CreatePackage("packageA", "test");
			CreatePackage("packageB", "test");

			Directory.CreateDirectory(Path.Combine(m_testDir, "ondemand_metadata"));
			CreateOndemandMetadata("packageA", "test", DateTime.Now.ToString());
			CreateOndemandMetadata("packageB", "test", DateTime.Now.AddDays(-180).ToString());

			string recordedOutput = RunEapm("prune", m_testDir);

			Assert.IsTrue(Directory.Exists(Path.Combine(m_testDir, "ondemand_metadata")));
			Assert.IsTrue(recordedOutput.Contains("Deleting ondemand release 'packageB-test', it was last referenced 180 days ago..."));
			Assert.IsTrue(recordedOutput.Contains("Package Cleanup Complete: 1 deleted, 0 warning, 1 kept"));
			Assert.IsTrue(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageA-test.metadata.xml")));
			Assert.IsFalse(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageB-test.metadata.xml")));
		}

		/// <summary>
		/// Test to make sure that if there is metadata for a package but it no longer exists it simply deletes the metadata file
		/// </summary>
		[TestMethod]
		public void EapmPruneTestNoLongerExists()
		{
			Directory.CreateDirectory(Path.Combine(m_testDir, "ondemand_metadata"));
			CreateOndemandMetadata("packageA", "test", DateTime.Now.ToString());
			CreateOndemandMetadata("packageB", "test", DateTime.Now.AddDays(-180).ToString());

			string recordedOutput = RunEapm("prune", m_testDir);

			Assert.IsTrue(Directory.Exists(Path.Combine(m_testDir, "ondemand_metadata")));
			Assert.IsTrue(recordedOutput.Contains(string.Format("Directory has already been deleted '{0}', deleting ondemand metadata file",
			Path.Combine(m_testDir, "packageA", "test"))));
			Assert.IsTrue(recordedOutput.Contains(string.Format("Directory has already been deleted '{0}', deleting ondemand metadata file",
			Path.Combine(m_testDir, "packageB", "test"))));
			Assert.IsTrue(recordedOutput.Contains("Package Cleanup Complete: 0 deleted, 0 warning, 0 kept"));
			Assert.IsFalse(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageA-test.metadata.xml")));
			Assert.IsFalse(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageB-test.metadata.xml")));

		}

		/// <summary>
		/// Make sure that when the warn option is used there is a warning printed but the packages are not deleted
		/// </summary>
		[TestMethod]
		public void EapmPruneTestWarn()
		{
			CreatePackage("packageA", "test");
			CreatePackage("packageB", "test");

			Directory.CreateDirectory(Path.Combine(m_testDir, "ondemand_metadata"));
			CreateOndemandMetadata("packageA", "test", DateTime.Now.ToString());
			CreateOndemandMetadata("packageB", "test", DateTime.Now.AddDays(-180).ToString());

			string recordedOutput = RunEapm("prune", m_testDir, "-warn");

			Assert.IsTrue(Directory.Exists(Path.Combine(m_testDir, "ondemand_metadata")));
			Assert.IsTrue(recordedOutput.Contains("[warning] Ondemand release 'packageB-test' hasn't been referenced in 180 days"));
			Assert.IsTrue(recordedOutput.Contains("Package Cleanup Complete: 0 deleted, 1 warning, 1 kept"));
			Assert.IsTrue(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageA-test.metadata.xml")));
			Assert.IsTrue(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageB-test.metadata.xml")));
		}

		/// <summary>
		/// Test that eapm prune works as expected when specifying a custom threshold value (default threshold is assumed to be 90)
		/// </summary>
		[TestMethod]
		public void EapmPruneTestCustomThreshold()
		{
			CreatePackage("packageA", "test");
			CreatePackage("packageB", "test");
			CreatePackage("packageC", "test");

			Directory.CreateDirectory(Path.Combine(m_testDir, "ondemand_metadata"));
			CreateOndemandMetadata("packageA", "test", DateTime.Now.ToString());
			CreateOndemandMetadata("packageB", "test", DateTime.Now.AddDays(-180).ToString());
			CreateOndemandMetadata("packageC", "test", DateTime.Now.AddDays(-100).ToString());

			string recordedOutput = RunEapm("prune", m_testDir, "-threshold:120");

			Assert.IsTrue(Directory.Exists(Path.Combine(m_testDir, "ondemand_metadata")));
			Assert.IsTrue(recordedOutput.Contains("Deleting ondemand release 'packageB-test', it was last referenced 180 days ago..."));
			Assert.IsTrue(recordedOutput.Contains("Package Cleanup Complete: 1 deleted, 0 warning, 2 kept"));
			Assert.IsTrue(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageA-test.metadata.xml")));
			Assert.IsFalse(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageB-test.metadata.xml")));
			Assert.IsTrue(File.Exists(Path.Combine(m_testDir, "ondemand_metadata", "packageC-test.metadata.xml")));
		}
	}
}