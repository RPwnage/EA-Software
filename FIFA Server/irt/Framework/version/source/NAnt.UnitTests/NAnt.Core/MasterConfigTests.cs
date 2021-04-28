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
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Tests.Util;
using NAnt.Core.Writers;
using System.Text;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class MasterConfigTest : NAntTestClassBase
	{
		[TestMethod]
		public void MasterConfigConfigBlockTest()
		{
			string testDir = TestUtil.CreateTestDirectory("MasterConfigConfigBlockTest");

			try
			{
				// create a masterconfig with the old style config element
				{
					Project project = new Project(Log.Silent);

					string masterconfigPath = Path.Combine(testDir, "masterconfig1.xml");
					XmlDocument doc = new XmlDocument();
					XmlNode rootNode = doc.CreateElement("project");
					XmlNode configNode = doc.CreateElement("config");
					configNode.SetAttribute("package", "eaconfig");
					configNode.SetAttribute("default", "pc-vc-debug");
					rootNode.AppendChild(configNode);
					doc.AppendChild(rootNode);
					doc.Save(masterconfigPath);

					MasterConfig masterconfig = MasterConfig.Load(project, masterconfigPath);
					Assert.AreEqual(masterconfig.Config.Extensions.Count, 0);
				}

				// create a masterconfig with config extensions
				try
				{
					Project project = new Project(Log.Silent);

					string masterconfigPath = Path.Combine(testDir, "masterconfig2.xml");
					XmlDocument doc = new XmlDocument();
					XmlNode rootNode = doc.CreateElement("project");
					XmlNode configNode = doc.CreateElement("config");
					configNode.SetAttribute("package", "eaconfig");
					configNode.SetAttribute("default", "pc-vc-debug");
					XmlNode extensionNode1 = doc.CreateElement("extension");
					extensionNode1.SetAttribute("if", "${config-use-FBConfig??true}");
					extensionNode1.InnerText = "FBConfig";
					XmlNode extensionNode2 = doc.CreateElement("extension");
					extensionNode2.InnerText = "android_config";
					configNode.AppendChild(extensionNode1);
					configNode.AppendChild(extensionNode2);
					rootNode.AppendChild(configNode);
					doc.AppendChild(rootNode);
					doc.Save(masterconfigPath);

					MasterConfig masterconfig = MasterConfig.Load(project, masterconfigPath);
					PackageMap.Init(project, masterconfig);

					Assert.AreEqual(masterconfig.Config.Extensions.Count, 2);

					project.Properties["config-use-FBConfig"] = "false";
					Assert.AreEqual(PackageMap.Instance.GetConfigExtensions(project).Count, 1);
				}
				finally
				{
					PackageMap.Reset();
				}

				try
				{
					Project project = new Project(Log.Silent);

					// check that invalid boolean in the if field throws an error
					string masterConfigPath = Path.Combine(testDir, "masterconfig3.xml");
					XmlDocument doc = new XmlDocument();
					XmlNode rootNode = doc.CreateElement("project");
					XmlNode configNode = doc.CreateElement("config");
					configNode.SetAttribute("package", "eaconfig");
					configNode.SetAttribute("default", "pc-vc-debug");
					XmlNode extensionNode1 = doc.CreateElement("extension");
					extensionNode1.SetAttribute("if", "asdf");
					extensionNode1.InnerText = "FBConfig";
					XmlNode extensionNode2 = doc.CreateElement("extension");
					extensionNode2.InnerText = "android_config";
					configNode.AppendChild(extensionNode1);
					configNode.AppendChild(extensionNode2);
					rootNode.AppendChild(configNode);
					doc.AppendChild(rootNode);
					doc.Save(masterConfigPath);

					MasterConfig masterConfig = MasterConfig.Load(project, masterConfigPath);
					PackageMap.Init(project, masterConfig);

					Assert.AreEqual(masterConfig.Config.Extensions.Count, 2);

					Assert.ThrowsException<BuildException>(() => PackageMap.Instance.GetConfigExtensions(project));
				}
				finally
				{
					PackageMap.Reset();
				}
			}
			finally
			{
				if (Directory.Exists(testDir))
				{
					Directory.Delete(testDir, true);
				}
			}
		}

		[TestMethod]
		public void GetMasterPackageCaseInsensitivelyTest()
		{
			string testDir = TestUtil.CreateTestDirectory("caseInstenstiveTest");

			try
			{
				string masterconfigPath = Path.Combine(testDir, "masterconfig_caseinsensitive_test.xml");

				// create a temporary masterconfig file
				XmlDocument doc = new XmlDocument();
				XmlNode rootNode = doc.CreateElement("project");

				XmlNode masterVersionsNode = doc.CreateElement("masterversions");
				masterVersionsNode.AppendChild(CreatePackageNode(doc, "TestPackage", "3.00.00"));
				rootNode.AppendChild(masterVersionsNode);

				XmlNode devRootNode = doc.CreateElement("developmentroot");
				devRootNode.InnerText = testDir;
				rootNode.AppendChild(devRootNode);

				XmlNode configNode = doc.CreateElement("config");
				configNode.SetAttribute("package", "eaconfig");
				configNode.SetAttribute("default", "pc-vc-debug");
				rootNode.AppendChild(configNode);

				doc.AppendChild(rootNode);
				doc.Save(masterconfigPath);

				Project project = new Project(Log.Silent);
				{
					MasterConfig masterconfig = MasterConfig.Load(project, masterconfigPath);

					// lookup using the correct case
					MasterConfig.Package masterPackage1 = masterconfig.GetMasterPackageCaseInsensitively(project, "TestPackage");
					Assert.IsTrue(masterPackage1 != null);

					// lookup using all lower case
					MasterConfig.Package masterPackage2 = masterconfig.GetMasterPackageCaseInsensitively(project, "testpackage");
					Assert.IsTrue(masterPackage2 != null);

					// lookup using alternating letter case
					MasterConfig.Package masterPackage3 = masterconfig.GetMasterPackageCaseInsensitively(project, "TeStPaCkAgE");
					Assert.IsTrue(masterPackage3 != null);

					// lookup package not in the masterconfig
					MasterConfig.Package masterPackage4 = masterconfig.GetMasterPackageCaseInsensitively(project, "missingpackage");
					Assert.IsTrue(masterPackage4 == null);
				}
			}
			finally
			{
				if (Directory.Exists(testDir)) Directory.Delete(testDir, true);
			}
		}

		// Tests the evaluation of various grouptype names
		[TestMethod]
		public void GroupTypeNameTest()
		{
			string testDir = TestUtil.CreateTestDirectory("localOndemandTest");

			try
			{
				string masterconfigPath = Path.Combine(testDir, "masterconfig_grouptype_test.xml");

				XmlDocument doc = new XmlDocument();
				XmlNode rootNode = doc.CreateElement("project");

				XmlNode masterVersionsNode = doc.CreateElement("masterversions");

				XmlNode groupNode = doc.CreateElement("grouptype");
				groupNode.SetAttribute("name", "a");

				XmlNode exceptionNode = doc.CreateElement("exception");
				exceptionNode.SetAttribute("propertyname", "grpname");

				Dictionary<string, string> conditions = new Dictionary<string, string>();
				conditions.Add("1", "qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890aaaaaaaaa");
				conditions.Add("i2", "a\b");
				conditions.Add("i3", "a/b");
				conditions.Add("4", "a:b");
				conditions.Add("5", "a*b");
				conditions.Add("6", "-mate");
				conditions.Add("7", "a&quot;b");
				conditions.Add("8", "a&lt;b");
				conditions.Add("9", "a&gt;b");
				conditions.Add("10", "a|b");
				conditions.Add("i11", ".a");
				conditions.Add("i12", "b.");
				conditions.Add("i13", "a/b\\c");
				conditions.Add("i14", "a\\/");
				conditions.Add("i15", "/\\s");
				conditions.Add("i16", "\\/\\/00+");
				conditions.Add("17", "valid");
				conditions.Add("18", "w00t.w00t");
				conditions.Add("19", "hello_mate");
				conditions.Add("20", "wtf-mate");
				conditions.Add("21", "a?b");
				conditions.Add("22", "]");
				conditions.Add("23", "asfgvccefreiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
				conditions.Add("24", "a");
				conditions.Add("25", "-");
				conditions.Add("26", "+");
				conditions.Add("27", "=");
				conditions.Add("28", "~");
				conditions.Add("29", "_");
				conditions.Add("30", "{");
				conditions.Add("31", "}");
				conditions.Add("32", "[");
				conditions.Add("33", "_dude");
				conditions.Add("34", "(");
				conditions.Add("35", ")");
				conditions.Add("36", "(__|__)");
				conditions.Add("37", "( o )");
				conditions.Add("38", "qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890aaaaaaa");
				conditions.Add("39", "79h12890qwiopaslzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890aaaaaaa");
				conditions.Add("i40", " ");

				foreach (KeyValuePair<string, string> condition in conditions)
				{
					XmlNode conditionNode = doc.CreateElement("condition");
					conditionNode.SetAttribute("value", condition.Key);
					conditionNode.SetAttribute("name", condition.Value);
					exceptionNode.AppendChild(conditionNode);
				}

				groupNode.AppendChild(exceptionNode);
				groupNode.AppendChild(CreatePackageNode(doc, "packageB", "lib"));
				masterVersionsNode.AppendChild(groupNode);
				rootNode.AppendChild(masterVersionsNode);

				XmlNode configNode = doc.CreateElement("config");
				configNode.SetAttribute("package", "eaconfig");
				configNode.SetAttribute("default", "pc-vc-debug");
				rootNode.AppendChild(configNode);

				doc.AppendChild(rootNode);
				doc.Save(masterconfigPath);

				Project project = new Project(Log.Silent);
				{
					foreach (string key in conditions.Keys)
					{
						project.Properties["grpname"] = key;
						try
						{
							MasterConfig masterconfig = MasterConfig.Load(project, masterconfigPath);
							masterconfig.GetMasterPackageCaseInsensitively(project, "packageB").EvaluateGroupType(project);
							if (key.StartsWith("i"))
							{
								throw new Exception("Test " + key + " Failed! (succeeded but was expected to fail)");
							}
							else
							{
								System.Console.WriteLine("Test " + key + " Succeeded!");
							}
						}
						catch
						{
							if (key.StartsWith("i"))
							{
								System.Console.WriteLine("Test " + key + " Succeeded!");
							}
							else
							{
								throw new Exception("Test " + key + " Failed!");
							}
						}
					}
				}
			}
			finally
			{
				if (Directory.Exists(testDir)) Directory.Delete(testDir, true);
			}
		}

		[TestMethod]
		public void GroupTypeAdditionalAttributeTest()
		{
			string testDir = TestUtil.CreateTestDirectory("localOndemandTest");

			try
			{
				string masterconfigPath = Path.Combine(testDir, "masterconfig_grouptype_test.xml");

				File.WriteAllText
				(
					masterconfigPath,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\" test-discovery=\"true\">",
						$"			<package name=\"test-package-a1\" version=\"test\"/>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"	<fragments>",
						$"		<include name=\"masterconfig_fragment.xml\" />",
						$"	</fragments>",
						$"</project>"
					)
				);

				string fragmentFile = Path.Combine(testDir, "masterconfig_fragment.xml");
				File.WriteAllText
				(
					fragmentFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\">",
						$"			<package name=\"test-package-b1\" version=\"test\"/>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"</project>"
					)
				);

				Project project = new Project(Log.Silent);
				MasterConfig masterConfig = MasterConfig.Load(project, masterconfigPath);
				PackageMap.Init(project, masterConfig);

				Assert.IsTrue(masterConfig.GroupMap["default-group"].AdditionalAttributes["test-discovery"] == "true");

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterConfig, project);

					masterconfigWriter.Write();
					xmlWriter.Flush();
					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					Assert.IsTrue(generatedMasterconfig.Contains("grouptype name=\"default-group\" test-discovery=\"true\""));
				}
			}
			finally
			{
				if (Directory.Exists(testDir)) Directory.Delete(testDir, true);
			}
		}

		[TestMethod]
		public void GroupTypeAdditionalAttributeConflictTest()
		{
			string testDir = TestUtil.CreateTestDirectory("localOndemandTest");

			try
			{
				string masterconfigPath = Path.Combine(testDir, "masterconfig_grouptype_test.xml");

				File.WriteAllText
				(
					masterconfigPath,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\" test-discovery=\"true\">",
						$"			<package name=\"test-package-a1\" version=\"test\"/>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"	<fragments>",
						$"		<include name=\"masterconfig_fragment.xml\" />",
						$"	</fragments>",
						$"</project>"
					)
				);

				string fragmentFile = Path.Combine(testDir, "masterconfig_fragment.xml");
				File.WriteAllText
				(
					fragmentFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\"  test-discovery=\"false\">",
						$"			<package name=\"test-package-b1\" version=\"test\"/>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"</project>"
					)
				);

				Project project = new Project(Log.Silent);
				MasterConfig masterConfig = MasterConfig.Load(project, masterconfigPath);
				PackageMap.Init(project, masterConfig);

				Assert.Fail("The test was supposed to fail because of the attribute value conflict but didn't.");
			}
			catch (BuildException e)
			{
				Assert.IsTrue(e.Message.Contains("is defined in multiple fragments with conflicting values of the 'test-discovery' property."));
			}
			finally
			{
				if (Directory.Exists(testDir)) Directory.Delete(testDir, true);
			}
		}

		// test to check that the package is found in the correct package root
		[TestMethod]
		public void SetMasterconfigFindInPackageRoots()
		{
			string testDir = TestUtil.CreateTestDirectory("localOndemandTest");

			string oldFrameworkOnDemandRootEnvVar = Environment.GetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT");
			string oldFrameworkDevelopmentRootEnvVar = Environment.GetEnvironmentVariable("FRAMEWORK_DEVELOPMENT_ROOT");

			try
			{
				string masterconfigPath = Path.Combine(testDir, "masterconfig_packageroots_test.xml");

				string localPackageRoot = Path.Combine(testDir, "LocalPackages");
				string regularPackageRoot = Path.Combine(testDir, "RegularPackages");
				string ondemandPackageRoot = Path.Combine(testDir, "OndemandPackages");
				string developmentPackageRoot = Path.Combine(testDir, "DevelopmentPackages");

				// If the machine has environment variables set, we need to change them so that they
				// match this test case's requirement.
				if (oldFrameworkOnDemandRootEnvVar != null)
				{
					Environment.SetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT", ondemandPackageRoot);
				}
				if (oldFrameworkDevelopmentRootEnvVar != null)
				{
					Environment.SetEnvironmentVariable("FRAMEWORK_DEVELOPMENT_ROOT", developmentPackageRoot);
				}

				CreateDummyPackage("LocalOndemandPackage", "dev", localPackageRoot);
				CreateDummyPackage("LocalOndemandPackage", "dev", ondemandPackageRoot);
				CreateDummyPackage("LocalOndemandPackage", "dev", developmentPackageRoot);

				CreateDummyPackage("DevelopmentPackage", "dev", localPackageRoot);
				CreateDummyPackage("DevelopmentPackage", "dev", regularPackageRoot);
				CreateDummyPackage("DevelopmentPackage", "dev", ondemandPackageRoot);
				CreateDummyPackage("DevelopmentPackage", "dev", developmentPackageRoot);

				CreateDummyPackage("DevelopmentPackageInOndemandRoot", "dev", ondemandPackageRoot);
				CreateDummyPackage("DevelopmentPackageInOndemandRoot", "dev", regularPackageRoot);
				CreateDummyPackage("DevelopmentPackageInOndemandRoot", "dev", localPackageRoot);

				CreateDummyPackage("RegularPackage", "dev", regularPackageRoot);
				CreateDummyPackage("RegularPackage", "dev", localPackageRoot);
				CreateDummyPackage("RegularPackage", "dev", developmentPackageRoot);

				CreateDummyPackage("OndemandPackage", "dev", ondemandPackageRoot);
				CreateDummyPackage("OndemandPackage", "dev", localPackageRoot);
				CreateDummyPackage("OndemandPackage", "dev", developmentPackageRoot);

				CreateDummyPackage("OndemandPackageInRegularRoot", "dev", ondemandPackageRoot);
				CreateDummyPackage("OndemandPackageInRegularRoot", "dev", regularPackageRoot);
				CreateDummyPackage("OndemandPackageInRegularRoot", "dev", localPackageRoot);
				CreateDummyPackage("OndemandPackageInRegularRoot", "dev", developmentPackageRoot);

				// create a temporary masterconfig file
				XmlDocument doc = new XmlDocument();
				XmlNode rootNode = doc.CreateElement("project");

				XmlNode masterVersionsNode = doc.CreateElement("masterversions");

				XmlNode packageNode = CreatePackageNode(doc, "LocalOndemandPackage", "dev");
				packageNode.SetAttribute("localondemand", "true");
				masterVersionsNode.AppendChild(packageNode);

				XmlNode packageNode2 = CreatePackageNode(doc, "RegularPackage", "dev");
				masterVersionsNode.AppendChild(packageNode2);

				XmlNode packageNode3 = CreatePackageNode(doc, "OndemandPackage", "dev");
				masterVersionsNode.AppendChild(packageNode3);

				XmlNode packageNode4 = CreatePackageNode(doc, "DevelopmentPackage", "dev");
				packageNode4.SetAttribute("switched-from-version", "1.00.00");
				masterVersionsNode.AppendChild(packageNode4);

				XmlNode packageNode9 = CreatePackageNode(doc, "DevelopmentPackageInOndemandRoot", "dev");
				packageNode9.SetAttribute("switched-from-version", "1.00.00");
				masterVersionsNode.AppendChild(packageNode9);

				XmlNode packageNode5 = CreatePackageNode(doc, "OndemandPackageInRegularRoot", "dev");
				masterVersionsNode.AppendChild(packageNode5);

				XmlNode packageNode6 = CreatePackageNode(doc, "MissingRegularPackage", "dev");
				masterVersionsNode.AppendChild(packageNode6);

				XmlNode packageNode7 = CreatePackageNode(doc, "MissingLocalPackage", "dev");
				packageNode7.SetAttribute("localondemand", "true");
				masterVersionsNode.AppendChild(packageNode7);

				XmlNode packageNode8 = CreatePackageNode(doc, "MissingDevelopmentPackage", "dev");
				packageNode8.SetAttribute("switched-from-version", "1.00.00");
				masterVersionsNode.AppendChild(packageNode8);

				rootNode.AppendChild(masterVersionsNode);

				XmlNode packageRootsNode = doc.CreateElement("packageroots");
				XmlNode packageRootItemNode = doc.CreateElement("packageroot");
				packageRootItemNode.InnerText = regularPackageRoot;
				packageRootsNode.AppendChild(packageRootItemNode);
				rootNode.AppendChild(packageRootsNode);

				XmlNode ondemandElement = doc.CreateElement("ondemand");
				ondemandElement.InnerText = "true";
				rootNode.AppendChild(ondemandElement);

				XmlNode ondemandRootNode = doc.CreateElement("ondemandroot");
				ondemandRootNode.InnerText = ondemandPackageRoot;
				rootNode.AppendChild(ondemandRootNode);

				XmlNode localRootNode = doc.CreateElement("localondemandroot");
				localRootNode.InnerText = localPackageRoot;
				rootNode.AppendChild(localRootNode);

				XmlNode devRootNode = doc.CreateElement("developmentroot");
				devRootNode.InnerText = developmentPackageRoot;
				rootNode.AppendChild(devRootNode);

				XmlNode configNode = doc.CreateElement("config");
				configNode.SetAttribute("package", "eaconfig");
				configNode.SetAttribute("default", "pc-vc-debug");
				rootNode.AppendChild(configNode);

				doc.AppendChild(rootNode);
				doc.Save(masterconfigPath);

				Project project = new Project(Log.Silent);
				{
					MasterConfig masterConfig = MasterConfig.Load(project, masterconfigPath);

					// check the masterconfig loader parses the local flag in the masterconfig and sets the default correctly for elements that dont have this flag
					MasterConfig.Package masterPackage1 = masterConfig.GetMasterPackageCaseInsensitively(project, "LocalOndemandPackage");
					Assert.IsTrue(masterPackage1.LocalOnDemand.Value);
					MasterConfig.Package masterPackage2 = masterConfig.GetMasterPackageCaseInsensitively(project, "RegularPackage");
					Assert.IsFalse(masterPackage2.LocalOnDemand.HasValue);
					try
					{
						PackageMap.Init(project, masterConfig);

						// this package is an local ondemand package and does not appear in the regular package roots so it should use the local ondemand root
						Release release1 = PackageMap.Instance.FindOrInstallCurrentRelease(project, "LocalOndemandPackage");
						Assert.IsTrue(release1.Path.StartsWith(localPackageRoot));

						// this package is a development package so it should used the development root even though it appears in all package root types
						Release release2 = PackageMap.Instance.FindOrInstallCurrentRelease(project, "DevelopmentPackage");
						Assert.IsTrue(release2.Path.StartsWith(developmentPackageRoot));

						// this package has the switched-from-version attribute but only exists in package roots other than the development root
						// we should only be looking for this package in the development root
						Assert.ThrowsException<BuildException>(() => PackageMap.Instance.FindOrInstallCurrentRelease(project, "DevelopmentPackageInOndemandRoot"));

						// this package is an ondemand package and does not appear in the regular package roots so it should use the ondemand root
						Release release3 = PackageMap.Instance.FindOrInstallCurrentRelease(project, "OndemandPackage");
						Assert.IsTrue(release3.Path.StartsWith(ondemandPackageRoot));

						// this package is a regular package so it should be taken from the regular package roots, eventhough it appears in the dev and localondemand roots
						Release release4 = PackageMap.Instance.FindOrInstallCurrentRelease(project, "RegularPackage");
						Assert.IsTrue(release4.Path.StartsWith(regularPackageRoot));

						// this package is in both the ondemand and regular package roots, but the regular package root should take precedence
						Release release5 = PackageMap.Instance.FindOrInstallCurrentRelease(project, "OndemandPackageInRegularRoot");
						Assert.IsTrue(release5.Path.StartsWith(regularPackageRoot));

						// these packages do not exist and should return error
						Assert.ThrowsException<BuildException>(() => PackageMap.Instance.FindOrInstallCurrentRelease(project, "MissingRegularPackage"));
						Assert.ThrowsException<BuildException>(() => PackageMap.Instance.FindOrInstallCurrentRelease(project, "MissingLocalPackage"));
						Assert.ThrowsException<BuildException>(() => PackageMap.Instance.FindOrInstallCurrentRelease(project, "MissingDevelopmentPackage"));
					}
					finally
					{
						PackageMap.Reset();
					}
				}
			}
			finally
			{
				if (Directory.Exists(testDir)) Directory.Delete(testDir, true);
				if (oldFrameworkOnDemandRootEnvVar != null)
				{
					Environment.SetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT", oldFrameworkOnDemandRootEnvVar);
				}
				if (oldFrameworkDevelopmentRootEnvVar != null)
				{
					Environment.SetEnvironmentVariable("FRAMEWORK_DEVELOPMENT_ROOT", oldFrameworkDevelopmentRootEnvVar);
				}
			}
		}

		[TestMethod]
		public void MetaPackageTest()
		{
			string testDir = TestUtil.CreateTestDirectory("metapackagetest");

			try
			{
				string packageRoot = Path.Combine(testDir, "Packages");

				// create meta package
				{
					CreateDummyPackage("TestMetaPackage", "1.00.00", packageRoot);

					string metaPackageMasterconfigPath = Path.Combine(packageRoot, "TestMetaPackage", "1.00.00", "masterconfig.meta.xml");

					XmlDocument doc = new XmlDocument();
					XmlNode rootNode = doc.CreateElement("project");

					XmlNode hosts = doc.CreateElement("hosts");
					XmlNode hostNode = doc.CreateElement("host");
					hostNode.SetAttribute("name", "sdk");
					hostNode.SetAttribute("root", "//packages");
					hostNode.SetAttribute("server", "frostbite.perforce.server");
					hosts.AppendChild(hostNode);
					rootNode.AppendChild(hosts);

					XmlNode globaldefinesNode = doc.CreateElement("globaldefines");
					globaldefinesNode.InnerText = "TEST_DEFINE=1";
					rootNode.AppendChild(globaldefinesNode);

					XmlNode globalDotNetDefinesNode = doc.CreateElement("globaldotnetdefines");
					globalDotNetDefinesNode.InnerText = "TEST_DOTNET_DEFINE=2";
					rootNode.AppendChild(globalDotNetDefinesNode);

					XmlNode globalpropertiesNode = doc.CreateElement("globalproperties");
					globalpropertiesNode.InnerText = "test.property=true";
					rootNode.AppendChild(globalpropertiesNode);

					XmlNode masterVersionsNode = doc.CreateElement("masterversions");
					XmlNode groupNode = doc.CreateElement("grouptype");
					groupNode.SetAttribute("name", "a");
					groupNode.AppendChild(CreatePackageNode(doc, "TestPackage", "1.00.00"));
					groupNode.AppendChild(CreatePackageNode(doc, "TestPackage1", "dev"));
					masterVersionsNode.AppendChild(groupNode);
					rootNode.AppendChild(masterVersionsNode);

					XmlNode configNode = doc.CreateElement("config");
					configNode.SetAttribute("package", "eaconfig");
					configNode.SetAttribute("default", "android-clang-debug");
					rootNode.AppendChild(configNode);

					doc.AppendChild(rootNode);
					doc.Save(metaPackageMasterconfigPath);
				}

				string masterconfigPath = Path.Combine(testDir, "masterconfig_metapackagetest_test.xml");

				// create a temporary masterconfig file
				{
					XmlDocument doc = new XmlDocument();
					XmlNode rootNode = doc.CreateElement("project");

					XmlNode globalpropertiesNode = doc.CreateElement("globalproperties");
					globalpropertiesNode.InnerText = "test2.property=false";
					rootNode.AppendChild(globalpropertiesNode);

					XmlNode masterVersionsNode = doc.CreateElement("masterversions");
					XmlNode groupNode = doc.CreateElement("grouptype");
					groupNode.SetAttribute("name", "b");
					groupNode.AppendChild(CreatePackageNode(doc, "TestPackage", "3.00.00", allowOverride: "true"));
					groupNode.AppendChild(CreatePackageNode(doc, "TestMetaPackage", "1.00.00", isMetaPackage: true));
					masterVersionsNode.AppendChild(groupNode);
					rootNode.AppendChild(masterVersionsNode);

					XmlNode devRootNode = doc.CreateElement("developmentroot");
					devRootNode.InnerText = testDir;
					rootNode.AppendChild(devRootNode);

					XmlNode packageRootsNode = doc.CreateElement("packageroots");
					XmlNode packageRootItemNode = doc.CreateElement("packageroot");
					packageRootItemNode.InnerText = packageRoot;
					packageRootsNode.AppendChild(packageRootItemNode);
					rootNode.AppendChild(packageRootsNode);

					XmlNode configNode = doc.CreateElement("config");
					configNode.SetAttribute("package", "eaconfig");
					configNode.SetAttribute("default", "pc-vc-debug");
					rootNode.AppendChild(configNode);

					doc.AppendChild(rootNode);
					doc.Save(masterconfigPath);
				}

				Project project = new Project(Log.Silent);
				{
					MasterConfig masterConfig = MasterConfig.Load(project, masterconfigPath);

					PackageMap.Init(project, masterConfig);

					// lookup using the correct case
					MasterConfig.Package testPackage = PackageMap.Instance.MasterConfig.GetMasterPackageCaseInsensitively(project, "TestPackage");
					Assert.IsTrue(testPackage != null);
					Assert.IsTrue(testPackage.Version == "3.00.00");
					Assert.IsTrue(testPackage.GroupType.Name == "b");

					MasterConfig.Package testMetaPackage = PackageMap.Instance.MasterConfig.GetMasterPackageCaseInsensitively(project, "TestMetaPackage");
					Assert.IsTrue(testMetaPackage.Version == "1.00.00");
					Assert.IsTrue(testMetaPackage.GroupType.Name == "b");

					MasterConfig.Package testPackage1 = PackageMap.Instance.MasterConfig.GetMasterPackageCaseInsensitively(project, "TestPackage1");
					Assert.IsTrue(testPackage1.Version == "dev");
					Assert.IsTrue(testPackage1.GroupType.Name == "b");

					Assert.IsTrue(PackageMap.Instance.MasterConfig.GlobalProperties.Any(gp => gp.Name == "test.property" && gp.Value == "true"));
					Assert.IsTrue(PackageMap.Instance.MasterConfig.GlobalProperties.Any(gp => gp.Name == "test2.property" && gp.Value == "false"));
					Assert.IsTrue(PackageMap.Instance.MasterConfig.GlobalDefines.Any(gd => gd.Name == "TEST_DEFINE" && gd.Value == "1" && gd.DotNet == false));
					Assert.IsTrue(PackageMap.Instance.MasterConfig.GlobalDefines.Any(gd => gd.Name == "TEST_DOTNET_DEFINE" && gd.Value == "2" && gd.DotNet == true));
				}
			}
			finally
			{
				if (Directory.Exists(testDir)) Directory.Delete(testDir, true);
			}
		}

		[TestMethod]
		public void WarningSectionTest()
		{
			string testDir = TestUtil.CreateTestDirectory("warningsSectionTest");
			try
			{
				string masterConfgFile = Path.Combine(testDir, "warnings_masterconfig.xml");
				string warningFragment = Path.Combine(testDir, "warning_fragments.xml");
				File.WriteAllText
				(
					masterConfgFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<warnings>",
						$"		<warning id='{Log.WarningId.ManifestFileError.ToString("d")}' enabled='false' as-error='false'/>",
						$"		<warning id='{Log.WarningId.DuplicateReleaseFound.ToString("d")}' enabled='false' as-error='true'/>",
						$"		<warning id='{Log.WarningId.BuildFailure.ToString("d")}' enabled='true' as-error='false'/>",
						$"		<warning id='{Log.WarningId.InitializeAssumption.ToString("d")}' enabled='true' as-error='true'/>",
						$"		<warning id='{Log.WarningId.TaskCancelled.ToString("d")}' enabled='false'/>",
						$"		<warning id='{Log.WarningId.BuildScriptSetupError.ToString("d")}' as-error='true'/>",
						$"		<warning id='{Log.WarningId.LongPathWarning.ToString("d")}'/>",
						$"		<warning id='{Log.WarningId.InconsistentVersionName.ToString("d")}' enabled='true' as-error='true'/>",
						$"		<warning id='{Log.WarningId.PackageRootDoesNotExist.ToString("d")}' enabled='false' as-error='false'/>",
						$"		<warning id='{Log.WarningId.DuplicateReleaseInside.ToString("d")}' enabled='true' as-error='false'/>",
						$"	</warnings>",
						$"	<masterversions>",
						$"		<package name='test' version='test'>",
						$"			<warnings>",
						$"				<warning id='{Log.WarningId.PackageCompatibilityWarn.ToString("d")}' enabled='false' as-error='false'/>",
						$"				<warning id='{Log.WarningId.InvalidModuleDepReference.ToString("d")}' enabled='true' as-error='true'/>",
						$"			</warnings>",
						$"		</package>",
						$"	</masterversions>", 
						$"	<fragments>",
						$"		<include name='{warningFragment}' asis='true'/>",
						$"	</fragments>",
						$"	<config package='config-package'/>",
						$"</project>"
					)
				);
				File.WriteAllText
				(
					warningFragment,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<warnings>",
						$"		<warning id='{Log.WarningId.PackageRootDoesNotExist.ToString("d")}' enabled='true'/>",
						$"		<warning id='{Log.WarningId.DuplicateReleaseInside.ToString("d")}' as-error='true'/>",
						$"	</warnings>",
						$"</project>"
					)
				);

				// set up context
				Log warningLog = new Log(warningLevel: Log.WarnLevel.Normal);
				Project project = new Project(warningLog);

				// check masterconfig is parsing correctly
				MasterConfig masterConfig = MasterConfig.Load(project, masterConfgFile);
				Assert.IsFalse(masterConfig.Warnings[Log.WarningId.ManifestFileError].Enabled.Value);
				Assert.IsFalse(masterConfig.Warnings[Log.WarningId.ManifestFileError].AsError.Value);
				Assert.AreEqual(masterConfgFile, masterConfig.Warnings[Log.WarningId.ManifestFileError].SettingLocation.FileName);
				Assert.IsFalse(masterConfig.Warnings[Log.WarningId.DuplicateReleaseFound].Enabled.Value);
				Assert.IsTrue(masterConfig.Warnings[Log.WarningId.DuplicateReleaseFound].AsError.Value);
				Assert.AreEqual(masterConfgFile, masterConfig.Warnings[Log.WarningId.DuplicateReleaseFound].SettingLocation.FileName);
				Assert.IsTrue(masterConfig.Warnings[Log.WarningId.BuildFailure].Enabled.Value);
				Assert.IsFalse(masterConfig.Warnings[Log.WarningId.BuildFailure].AsError.Value);
				Assert.AreEqual(masterConfgFile, masterConfig.Warnings[Log.WarningId.BuildFailure].SettingLocation.FileName);
				Assert.IsTrue(masterConfig.Warnings[Log.WarningId.InitializeAssumption].Enabled.Value);
				Assert.IsTrue(masterConfig.Warnings[Log.WarningId.InitializeAssumption].AsError.Value);
				Assert.AreEqual(masterConfgFile, masterConfig.Warnings[Log.WarningId.InitializeAssumption].SettingLocation.FileName);

				Assert.IsFalse(masterConfig.Warnings[Log.WarningId.TaskCancelled].Enabled.Value);
				Assert.AreEqual(null, masterConfig.Warnings[Log.WarningId.TaskCancelled].AsError);
				Assert.IsTrue(masterConfig.Warnings[Log.WarningId.BuildScriptSetupError].AsError.Value);
				Assert.AreEqual(null, masterConfig.Warnings[Log.WarningId.BuildScriptSetupError].Enabled);

				Assert.IsFalse(masterConfig.Warnings.ContainsKey(Log.WarningId.LongPathWarning)); // this sets nothing so should have no entry

				// check fragment overides base
				Assert.IsTrue(masterConfig.Warnings[Log.WarningId.PackageRootDoesNotExist].Enabled.Value);
				Assert.IsFalse(masterConfig.Warnings[Log.WarningId.PackageRootDoesNotExist].AsError.Value);
				Assert.AreEqual(warningFragment, masterConfig.Warnings[Log.WarningId.PackageRootDoesNotExist].SettingLocation.FileName);
				Assert.IsTrue(masterConfig.Warnings[Log.WarningId.DuplicateReleaseInside].Enabled.Value);
				Assert.IsTrue(masterConfig.Warnings[Log.WarningId.DuplicateReleaseInside].AsError.Value);
				Assert.AreEqual(warningFragment, masterConfig.Warnings[Log.WarningId.DuplicateReleaseInside].SettingLocation.FileName);

				// check package specific warning settings
				Assert.IsFalse(masterConfig.Packages["test"].Warnings[Log.WarningId.PackageCompatibilityWarn].Enabled.Value);
				Assert.IsFalse(masterConfig.Packages["test"].Warnings[Log.WarningId.PackageCompatibilityWarn].AsError.Value);
				Assert.AreEqual(masterConfgFile, masterConfig.Packages["test"].Warnings[Log.WarningId.PackageCompatibilityWarn].SettingLocation.FileName);
				Assert.IsTrue(masterConfig.Packages["test"].Warnings[Log.WarningId.InvalidModuleDepReference].Enabled.Value);
				Assert.IsTrue(masterConfig.Packages["test"].Warnings[Log.WarningId.InvalidModuleDepReference].AsError.Value);
				Assert.AreEqual(masterConfgFile, masterConfig.Packages["test"].Warnings[Log.WarningId.InvalidModuleDepReference].SettingLocation.FileName);

				// before we initialize warning setting from masterconfig, set warnings as if from command line - this should not be overriden by masterconfig
				Log.SetGlobalWarningEnabledIfNotSet(warningLog, Log.WarningId.InconsistentVersionName, false, Log.SettingSource.CommandLine);
				Log.SetGlobalWarningAsErrorIfNotSet(warningLog, Log.WarningId.InconsistentVersionName, false, Log.SettingSource.CommandLine);

				// check log settings - note test mixes using minimal and advise at call site, if warning is force disabled then
				// minimal is checked because this should be enabled by default. Reverse applis to advise
				PackageMap.Init(project, masterConfig); // package map init sets global warnings from masterconfig

				// explicit disable even though it's a minimal level warning, not error
				Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.ManifestFileError, Log.WarnLevel.Minimal, out bool manifestFileErrorAsError));
				Assert.IsFalse(manifestFileErrorAsError);

				// this isn't explicitly enabled but is explicitly set as error therefore is implicitly enabled
				Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.DuplicateReleaseFound, Log.WarnLevel.Advise, out bool duplicateReleaseAsError));
				Assert.IsTrue(duplicateReleaseAsError);

				// advise warning level isn't default enabled - but this specific warning should be force enabled, should not be error
				Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Advise, out bool buildFailureAsError));
				Assert.IsFalse(buildFailureAsError);

				// force enabled and forced as error
				Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.InitializeAssumption, Log.WarnLevel.Advise, out bool initializeAssumptionAsError));
				Assert.IsTrue(initializeAssumptionAsError);

				// forced enabled, no explicit as error value so should default to false
				Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.TaskCancelled, Log.WarnLevel.Minimal, out bool taskCancelledAsError));
				Assert.IsFalse(taskCancelledAsError);

				// no enabled setting - but this was forced as error therefore is enabled and is error
				Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.BuildScriptSetupError, Log.WarnLevel.Advise, out bool buildScriptSetupAsError));
				Assert.IsTrue(buildScriptSetupAsError);

				// this was disabled and not-error'd as if from command line - should be disabled despite being set true and as error in masterconfig
				Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.InconsistentVersionName, Log.WarnLevel.Minimal, out bool inconsistentVersionNameAsError));
				Assert.IsFalse(inconsistentVersionNameAsError);

				// fragments enabled this, but does not set as error
				Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.PackageRootDoesNotExist, Log.WarnLevel.Advise, out bool packageRootDoesNotExistAsError));
				Assert.IsFalse(packageRootDoesNotExistAsError);

				// this was enabled in base, but fragment also make it an error
				Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.DuplicateReleaseInside, Log.WarnLevel.Advise, out bool duplicateReleaseInsideAsError));
				Assert.IsTrue(duplicateReleaseInsideAsError);

				// check these two warnings are working as default
				Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, out bool packageCompatibilityWarnAsError));
				Assert.IsFalse(packageCompatibilityWarnAsError);
				Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Advise, out bool invalidModuleDepReferenceAsError));
				Assert.IsFalse(invalidModuleDepReferenceAsError);

				// apply the package specific warning settings for 'test' package
				warningLog.ApplyPackageSuppressions(project, "test");

				// perform test as before, but the results should have changed based on package specific warning settings
				Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.PackageCompatibilityWarn, Log.WarnLevel.Minimal, out packageCompatibilityWarnAsError));
				Assert.IsFalse(packageCompatibilityWarnAsError);
				Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.InvalidModuleDepReference, Log.WarnLevel.Advise, out invalidModuleDepReferenceAsError));
				Assert.IsTrue(invalidModuleDepReferenceAsError);
			}
			finally
			{
				PackageMap.Reset();
				if (Directory.Exists(testDir)) 
				{
					Directory.Delete(testDir, true);
				}
			}
		}

		[TestMethod]
		public void DeprecationSectionTest()
		{
			string testDir = TestUtil.CreateTestDirectory("deprecationsSectionTest");
			try
			{
				string masterConfgFile = Path.Combine(testDir, "deprecations_masterconfig.xml");
				string deprecationFragment = Path.Combine(testDir, "deprecation_fragments.xml");
				File.WriteAllText
				(
					masterConfgFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<deprecations>",
						$"		<deprecation id='{Log.DeprecationId.SysInfo.ToString("d")}' enabled='false'/>",
						$"		<deprecation id='{Log.DeprecationId.ScriptTaskLanguageParameter.ToString("d")}' enabled='false'/>",
						$"		<deprecation id='{Log.DeprecationId.PackageInitializeSelf.ToString("d")}' enabled='true'/>",
						$"		<deprecation id='{Log.DeprecationId.OldLogWriteAPI.ToString("d")}' enabled='false'/>",
						$"		<deprecation id='{Log.DeprecationId.ModulelessPackage.ToString("d")}' enabled='true'/>",
						$"	</deprecations>",
						$"	<masterversions>",
						$"		<package name='test' version='test'>",
						$"			<deprecations>",
						$"				<deprecation id='{Log.DeprecationId.DependentVersion.ToString("d")}' enabled='false'/>",
						$"				<deprecation id='{Log.DeprecationId.ScriptInit.ToString("d")}' enabled='true'/>",
						$"			</deprecations>",
						$"		</package>",
						$"	</masterversions>",
						$"	<fragments>",
						$"		<include name='{deprecationFragment}' asis='true'/>",
						$"	</fragments>",
						$"	<config package='config-package'/>",
						$"</project>"
					)
				);
				File.WriteAllText
				(
					deprecationFragment,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<deprecations>",
						$"		<deprecation id='{Log.DeprecationId.OldLogWriteAPI.ToString("d")}' enabled='true'/>",
						$"		<deprecation id='{Log.DeprecationId.ModulelessPackage.ToString("d")}' enabled='false'/>",
						$"	</deprecations>",
						$"</project>"
					)
				);

				// set up context
				Log deprecationLog = new Log(deprecationLevel: Log.DeprecateLevel.Normal);
				Project project = new Project(deprecationLog);

				// check masterconfig is parsing correctly
				MasterConfig masterConfig = MasterConfig.Load(project, masterConfgFile);
				Assert.IsFalse(masterConfig.Deprecations[Log.DeprecationId.SysInfo].Enabled);
				Assert.AreEqual(masterConfgFile, masterConfig.Deprecations[Log.DeprecationId.SysInfo].SettingLocation.FileName);
				Assert.IsFalse(masterConfig.Deprecations[Log.DeprecationId.ScriptTaskLanguageParameter].Enabled);
				Assert.AreEqual(masterConfgFile, masterConfig.Deprecations[Log.DeprecationId.ScriptTaskLanguageParameter].SettingLocation.FileName);
				Assert.IsTrue(masterConfig.Deprecations[Log.DeprecationId.PackageInitializeSelf].Enabled);
				Assert.AreEqual(masterConfgFile, masterConfig.Deprecations[Log.DeprecationId.PackageInitializeSelf].SettingLocation.FileName);

				// check fragment overides base
				Assert.IsTrue(masterConfig.Deprecations[Log.DeprecationId.OldLogWriteAPI].Enabled);
				Assert.AreEqual(deprecationFragment, masterConfig.Deprecations[Log.DeprecationId.OldLogWriteAPI].SettingLocation.FileName);
				Assert.IsFalse(masterConfig.Deprecations[Log.DeprecationId.ModulelessPackage].Enabled);
				Assert.AreEqual(deprecationFragment, masterConfig.Deprecations[Log.DeprecationId.ModulelessPackage].SettingLocation.FileName);

				// check package specific deprecation settings
				Assert.IsFalse(masterConfig.Packages["test"].Deprecations[Log.DeprecationId.DependentVersion].Enabled);
				Assert.AreEqual(masterConfgFile, masterConfig.Packages["test"].Deprecations[Log.DeprecationId.DependentVersion].SettingLocation.FileName);
				Assert.IsTrue(masterConfig.Packages["test"].Deprecations[Log.DeprecationId.ScriptInit].Enabled);
				Assert.AreEqual(masterConfgFile, masterConfig.Packages["test"].Deprecations[Log.DeprecationId.ScriptInit].SettingLocation.FileName);

				// before we initialize warning setting from masterconfig, set deprecations as if from command line - this should not be overriden by masterconfig
				Log.SetGlobalDeprecationEnabledIfNotSet(deprecationLog, Log.DeprecationId.PackageInitializeSelf, false, Log.SettingSource.CommandLine);
				Log.SetGlobalDeprecationEnabledIfNotSet(deprecationLog, Log.DeprecationId.SysInfo, true, Log.SettingSource.CommandLine);

				// check log settings - note test mixes using minimal and advise at call site, if warning is force disabled then
				// minimal is checked because this should be enabled by default. Reverse applis to advise
				PackageMap.Init(project, masterConfig); // package map init sets global warnings from masterconfig

				// Verify the settings after package map is initialized with the masterconfigs AND taken into account of
				// command line influence.
				// - these from from command line influence
				Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Minimal));
				Assert.IsFalse(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.PackageInitializeSelf, Log.DeprecateLevel.Minimal));
				// - these are from masterconfig
				Assert.IsFalse(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.ScriptTaskLanguageParameter, Log.DeprecateLevel.Minimal));
				Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.OldLogWriteAPI, Log.DeprecateLevel.Minimal));
				Assert.IsFalse(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.ModulelessPackage, Log.DeprecateLevel.Minimal));

				// check these two package specific deprecations should have the default setting (which is enabled by default)
				Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.DependentVersion, Log.DeprecateLevel.Minimal));
				Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.ScriptInit, Log.DeprecateLevel.Minimal));

				// apply the package specific deprecation settings for 'test' package
				deprecationLog.ApplyPackageSuppressions(project, "test");

				// perform test as before, but the results should have changed based on package specific warning settings
				Assert.IsFalse(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.DependentVersion, Log.DeprecateLevel.Minimal));
				Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.ScriptInit, Log.DeprecateLevel.Minimal));
			}
			finally
			{
				PackageMap.Reset();
				if (Directory.Exists(testDir))
				{
					Directory.Delete(testDir, true);
				}
			}
		}

		[TestMethod]
		public void MasterGroupExceptionsTest()
		{
			string testDir = TestUtil.CreateTestDirectory("deprecationsSectionTest");
			try
			{
				string masterConfigFile = Path.Combine(testDir, "groups_masterconfig.xml");
				File.WriteAllText
				(
					masterConfigFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\" outputname-map-options=\"default-output-options\" autobuildclean=\"false\">",
						$"			<exception propertyname=\"exception-property\">",
						$"				<condition value=\"special\" name=\"special-group\"/>",
						$"			</exception>",
						$"			<exception propertyname=\"exception-property\">",
						$"				<condition value=\"loose\" name=\"loose-group\"/>",
						$"			</exception>",
						$"			<package name=\"test-package\" version=\"test\"/>",
						$"		</grouptype>",
						$"		<grouptype name=\"special-group\" outputname-map-options=\"special-output-options\" autobuildclean=\"true\">",
						$"			<exception propertyname=\"bounce-property\">",
						$"				<condition value=\"true\" name=\"bounce-group\"/>",
						$"			</exception>",
						$"		</grouptype>",
						$"		<grouptype name=\"bounce-group\" outputname-map-options=\"bounce-output-options\" autobuildclean=\"false\">",
						$"			<exception propertyname=\"circular\">",
						$"				<condition value=\"true\" name=\"special-group\"/>",
						$"			</exception>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"</project>"
					)
				);

				Project project = new Project(Log.Silent);
				MasterConfig masterConfig = MasterConfig.Load(project, masterConfigFile);
				PackageMap.Init(project, masterConfig);

				// check defaults are all set correctly
				MasterConfig.GroupType defaultGroup = PackageMap.Instance.GetMasterPackage(project, "test-package").EvaluateGroupType(project);
				Assert.AreEqual("default-group", defaultGroup.Name);
				Assert.AreEqual("default-output-options", defaultGroup.OutputMapOptionSet);
				Assert.IsFalse(defaultGroup.AutoBuildClean.Value);

				// if we apply special property we should get the special group - with attributes inherited fromt the outside group
				project.Properties["exception-property"] = "special";
				MasterConfig.GroupType specialGroup = PackageMap.Instance.GetMasterPackage(project, "test-package").EvaluateGroupType(project);
				Assert.AreEqual("special-group", specialGroup.Name);
				Assert.AreEqual("special-output-options", specialGroup.OutputMapOptionSet);
				Assert.IsTrue(specialGroup.AutoBuildClean.Value);

				// if the name override refers to a group that wasn't declared separately then it should inherit parent options
				project.Properties["exception-property"] = "loose";
				MasterConfig.GroupType looseGroup = PackageMap.Instance.GetMasterPackage(project, "test-package").EvaluateGroupType(project);
				Assert.AreEqual("loose-group", looseGroup.Name);
				Assert.AreEqual("default-output-options", defaultGroup.OutputMapOptionSet);
				Assert.IsFalse(defaultGroup.AutoBuildClean.Value);

				// check if we redirect to a different group with an exception we also follow that group's exception
				project.Properties["exception-property"] = "special";
				project.Properties["bounce-property"] = "true";
				MasterConfig.GroupType bounceGroup = PackageMap.Instance.GetMasterPackage(project, "test-package").EvaluateGroupType(project);
				Assert.AreEqual("bounce-group", bounceGroup.Name);
				Assert.AreEqual("bounce-output-options", bounceGroup.OutputMapOptionSet);
				Assert.IsFalse(bounceGroup.AutoBuildClean.Value);

				// check if get a failure if we have a circular reference loop
				project.Properties["circular"] = "true";
				Assert.ThrowsException<BuildException>(() => PackageMap.Instance.GetMasterPackage(project, "test-package").EvaluateGroupType(project));
			}
			finally
			{
				PackageMap.Reset();
				if (Directory.Exists(testDir))
				{
					Directory.Delete(testDir, true);
				}
			}
		}

		[TestMethod]
		public void MergeGroupsTest()
		{
			string testDir = TestUtil.CreateTestDirectory("MergeGroupsTest");
			try
			{
				string masterConfigFile = Path.Combine(testDir, "groups_masterconfig.xml");
				File.WriteAllText
				(
					masterConfigFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\">",
						$"			<package name=\"test-package-a1\" version=\"test\"/>",
						$"		</grouptype>",
						$"		<grouptype name=\"matching-group\" autobuildclean=\"false\" buildroot=\"\" outputname-map-options=\"outputmap\">",
						$"			<package name=\"test-package-a2\" version=\"test\"/>",
						$"		</grouptype>",
						$"		<grouptype name=\"base-set-group\" autobuildclean=\"false\" buildroot=\"\" outputname-map-options=\"outputmap\">",
						$"			<package name=\"test-package-a3\" version=\"test\"/>",
						$"		</grouptype>",
						$"		<grouptype name=\"fragment-set-group\">",
						$"			<package name=\"test-package-a4\" version=\"test\"/>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"	<fragments>",
						$"		<include name=\"masterconfig_fragment.xml\" />",
						$"	</fragments>",
						$"</project>"
					)
				);

				string fragmentFile = Path.Combine(testDir, "masterconfig_fragment.xml");
				File.WriteAllText
				(
					fragmentFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\">",
						$"			<package name=\"test-package-b1\" version=\"test\"/>",
						$"		</grouptype>",
						$"		<grouptype name=\"matching-group\" autobuildclean=\"false\" buildroot=\"\">",
						$"			<package name=\"test-package-b2\" version=\"test\"/>",
						$"		</grouptype>",
						$"		<grouptype name=\"base-set-group\">",
						$"			<package name=\"test-package-b3\" version=\"test\"/>",
						$"		</grouptype>",
						$"		<grouptype name=\"fragment-set-group\" autobuildclean=\"false\" buildroot=\"\" outputname-map-options=\"outputmap\">",
						$"			<package name=\"test-package-b4\" version=\"test\"/>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"</project>"
					)
				);

				Project project = new Project(Log.Silent);
				MasterConfig masterConfig = MasterConfig.Load(project, masterConfigFile);
				PackageMap.Init(project, masterConfig);

				// check defaults are all set correctly
				MasterConfig.GroupType defaultGroup = PackageMap.Instance.GetMasterPackage(project, "test-package-a1").EvaluateGroupType(project);
				Assert.AreEqual(null, defaultGroup.AutoBuildClean);
				Assert.AreEqual(null, defaultGroup.BuildRoot);
				Assert.AreEqual(null, defaultGroup.OutputMapOptionSet);

				MasterConfig.GroupType defaultGroup2 = PackageMap.Instance.GetMasterPackage(project, "test-package-a2").EvaluateGroupType(project);
				Assert.AreEqual(false, defaultGroup2.AutoBuildClean);
				Assert.AreEqual("", defaultGroup2.BuildRoot);
				Assert.AreEqual("outputmap", defaultGroup2.OutputMapOptionSet);

				MasterConfig.GroupType defaultGroup3 = PackageMap.Instance.GetMasterPackage(project, "test-package-a3").EvaluateGroupType(project);
				Assert.AreEqual(false, defaultGroup3.AutoBuildClean);
				Assert.AreEqual("", defaultGroup3.BuildRoot);
				Assert.AreEqual("outputmap", defaultGroup3.OutputMapOptionSet);

				MasterConfig.GroupType defaultGroup4 = PackageMap.Instance.GetMasterPackage(project, "test-package-a4").EvaluateGroupType(project);
				Assert.AreEqual(false, defaultGroup4.AutoBuildClean);
				Assert.AreEqual("", defaultGroup4.BuildRoot);
				Assert.AreEqual("outputmap", defaultGroup4.OutputMapOptionSet);
			}
			finally
			{
				PackageMap.Reset();
				if (Directory.Exists(testDir))
				{
					Directory.Delete(testDir, true);
				}
			}
		}

		[TestMethod]
		public void MergeGroupsConflictTest()
		{
			string testDir = TestUtil.CreateTestDirectory("MergeGroupsTest");
			try
			{
				string masterConfigFile = Path.Combine(testDir, "groups_masterconfig.xml");
				File.WriteAllText
				(
					masterConfigFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\" autobuildclean=\"false\">",
						$"			<package name=\"test-package-a1\" version=\"test\"/>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"	<fragments>",
						$"		<include name=\"masterconfig_fragment.xml\" />",
						$"	</fragments>",
						$"</project>"
					)
				);

				string fragmentFile = Path.Combine(testDir, "masterconfig_fragment.xml");
				File.WriteAllText
				(
					fragmentFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						$"		<grouptype name=\"default-group\" autobuildclean=\"true\">",
						$"			<package name=\"test-package-b1\" version=\"test\"/>",
						$"		</grouptype>",
						$"	</masterversions>",
						$"</project>"
					)
				);

				Project project = new Project(Log.Silent);
				MasterConfig masterConfig = MasterConfig.Load(project, masterConfigFile);
				PackageMap.Init(project, masterConfig);

				Assert.Fail("Test is supposed to fail because of a conflict merging");
			}
			catch (BuildException e)
			{
				Assert.IsTrue(e.Message.Contains("is defined in multiple fragments with conflicting values"));
			}
			finally
			{
				PackageMap.Reset();
				if (Directory.Exists(testDir))
				{
					Directory.Delete(testDir, true);
				}
			}
		}

		[TestMethod]
		public void DuplicateOndemandRootTest()
		{
			string testDir = TestUtil.CreateTestDirectory("DuplicateOndemandRoot");
			string ondemandEnvOverride = Environment.GetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT");
			Environment.SetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT", null);
			try
			{
				string masterConfigFile = Path.Combine(testDir, "groups_masterconfig.xml");
				File.WriteAllText
				(
					masterConfigFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						// should respect the localondemand settings despite it also being in the 
						// ondemand root which appears first
						$"		<package name=\"PackageA\" version=\"dev\" localondemand=\"true\"/>",
						// even though local ondemand is true it will find it in on of the other packageroots first
						$"		<package name=\"PackageB\" version=\"dev\" localondemand=\"true\"/>",
						// package will be found in Code since that appears before the ondemand root
						$"		<package name=\"PackageC\" version=\"dev\" localondemand=\"false\"/>",
						// package is in Code2 and ondemandpackages but the version in ondemandpackages
						// will be used because it appears first in the package root order
						$"		<package name=\"PackageD\" version=\"dev\" localondemand=\"false\"/>",
						// cases where the package is not found in any of the package roots
						$"		<package name=\"PackageE\" version=\"dev\" localondemand=\"false\" uri=\"stub://server/" + testDir + "/server\"/>",
						$"		<package name=\"PackageF\" version=\"1.00.00\" localondemand=\"true\" uri=\"stub://server/" + testDir + "/server\"/>",
						$"	</masterversions>",
						$"	<packageroots>",
						$"		<packageroot>.</packageroot>",
						$"		<packageroot>.\\Code</packageroot>",
						$"		<packageroot>.\\ondemandpackages</packageroot>",
						$"		<packageroot>.\\Code2</packageroot>",
						$"	</packageroots>",
						$"	<ondemand>true</ondemand>",
						$"	<ondemandroot>.\\ondemandpackages</ondemandroot>",
						$"	<localondemandroot>.\\localpackages</localondemandroot>",
						$"</project>"
					)
				);

				CreateDummyPackage("PackageA", "dev", Path.Combine(testDir, "localpackages"));
				CreateDummyPackage("PackageA", "dev", Path.Combine(testDir, "ondemandpackages"));

				CreateDummyPackage("PackageB", "dev", Path.Combine(testDir, "localpackages"));
				CreateDummyPackage("PackageB", "dev", Path.Combine(testDir, "Code"));

				CreateDummyPackage("PackageC", "dev", Path.Combine(testDir, "localpackages"));
				CreateDummyPackage("PackageC", "dev", Path.Combine(testDir, "Code"));
				CreateDummyPackage("PackageC", "dev", Path.Combine(testDir, "ondemandpackages"));

				CreateDummyPackage("PackageD", "dev", Path.Combine(testDir, "localpackages"));
				CreateDummyPackage("PackageD", "dev", Path.Combine(testDir, "Code2"));
				CreateDummyPackage("PackageD", "dev", Path.Combine(testDir, "ondemandpackages"));

				CreateDummyPackage("PackageE", "dev", Path.Combine(testDir, "server"));
				CreateDummyPackage("PackageF", "1.00.00", Path.Combine(testDir, "server"));

				Project project = new Project(Log.Silent);
				MasterConfig masterConfig = MasterConfig.Load(project, masterConfigFile);
				PackageMap.Init(project, masterConfig);

				string packageAPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageA").Path;
				string packageBPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageB").Path;
				string packageCPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageC").Path;
				string packageDPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageD").Path;
				string packageEPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageE").Path;
				string packageFPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageF").Path;

				Assert.AreEqual(Path.Combine(testDir, "localpackages", "PackageA", "dev"), packageAPath);
				Assert.AreEqual(Path.Combine(testDir, "Code", "PackageB", "dev"), packageBPath);
				Assert.AreEqual(Path.Combine(testDir, "Code", "PackageC", "dev"), packageCPath);
				Assert.AreEqual(Path.Combine(testDir, "ondemandpackages", "PackageD", "dev"), packageDPath);
				Assert.AreEqual(Path.Combine(testDir, "ondemandpackages", "PackageE", "dev"), packageEPath);
				Assert.AreEqual(Path.Combine(testDir, "localpackages", "PackageF", "1.00.00"), packageFPath);

				// change PackageE to a local ondemand and make sure that the one that
				// was previously downloaded to the ondemand folder is not used
				// eventhough ondemand appears first in the packageroot list
				masterConfig.Packages["PackageE"].LocalOnDemand = true;

				project = new Project(Log.Silent);
				PackageMap.Reset();
				PackageMap.Init(project, masterConfig);

				string packageE2Path = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageE").Path;

				Assert.AreEqual(Path.Combine(testDir, "localpackages", "PackageE", "dev"), packageE2Path);
			}
			finally
			{
				Environment.SetEnvironmentVariable("FRAMEWORK_ONDEMAND_ROOT", ondemandEnvOverride);
				PackageMap.Reset();
				if (Directory.Exists(testDir))
				{
					Directory.Delete(testDir, true);
				}
			}
		}

		[TestMethod]
		public void DuplicateOndemandRootOffTest()
		{
			string testDir = TestUtil.CreateTestDirectory("DuplicateOndemandRoot2");
			try
			{
				string masterConfigFile = Path.Combine(testDir, "groups_masterconfig.xml");
				File.WriteAllText
				(
					masterConfigFile,
					String.Join
					(
						Environment.NewLine,
						$"<project>",
						$"	<masterversions>",
						// unlike in the other test, because ondemand is off this time,
						// the localondemand flag will be ignored
						$"		<package name=\"PackageA\" version=\"dev\" localondemand=\"true\"/>",
						// The other examples will behave the same as in the first test
						$"		<package name=\"PackageB\" version=\"dev\" localondemand=\"true\"/>",
						$"		<package name=\"PackageC\" version=\"dev\" localondemand=\"false\"/>",
						$"		<package name=\"PackageD\" version=\"dev\" localondemand=\"false\"/>",
						$"	</masterversions>",
						$"	<packageroots>",
						$"		<packageroot>.</packageroot>",
						$"		<packageroot>.\\Code</packageroot>",
						$"		<packageroot>.\\ondemandpackages</packageroot>",
						$"		<packageroot>.\\Code2</packageroot>",
						$"	</packageroots>",
						$"	<ondemand>false</ondemand>",
						$"	<ondemandroot>.\\ondemandpackages</ondemandroot>",
						$"	<localondemandroot>.\\localpackages</localondemandroot>",
						$"</project>"
					)
				);

				CreateDummyPackage("PackageA", "dev", Path.Combine(testDir, "localpackages"));
				CreateDummyPackage("PackageA", "dev", Path.Combine(testDir, "ondemandpackages"));

				CreateDummyPackage("PackageB", "dev", Path.Combine(testDir, "localpackages"));
				CreateDummyPackage("PackageB", "dev", Path.Combine(testDir, "Code"));

				CreateDummyPackage("PackageC", "dev", Path.Combine(testDir, "localpackages"));
				CreateDummyPackage("PackageC", "dev", Path.Combine(testDir, "Code"));
				CreateDummyPackage("PackageC", "dev", Path.Combine(testDir, "ondemandpackages"));

				CreateDummyPackage("PackageD", "dev", Path.Combine(testDir, "localpackages"));
				CreateDummyPackage("PackageD", "dev", Path.Combine(testDir, "Code2"));
				CreateDummyPackage("PackageD", "dev", Path.Combine(testDir, "ondemandpackages"));

				Project project = new Project(Log.Silent);
				MasterConfig masterConfig = MasterConfig.Load(project, masterConfigFile);
				PackageMap.Init(project, masterConfig);

				string packageAPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageA").Path;
				string packageBPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageB").Path;
				string packageCPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageC").Path;
				string packageDPath = PackageMap.Instance.FindOrInstallCurrentRelease(project, "PackageD").Path;

				Assert.AreEqual(Path.Combine(testDir, "ondemandpackages", "PackageA", "dev"), packageAPath);
				Assert.AreEqual(Path.Combine(testDir, "Code", "PackageB", "dev"), packageBPath);
				Assert.AreEqual(Path.Combine(testDir, "Code", "PackageC", "dev"), packageCPath);
				Assert.AreEqual(Path.Combine(testDir, "ondemandpackages", "PackageD", "dev"), packageDPath);
			}
			finally
			{
				PackageMap.Reset();
				if (Directory.Exists(testDir))
				{
					Directory.Delete(testDir, true);
				}
			}
		}

		#region Helper Functions
		private void CreateDummyPackage(string name, string version, string root)
		{
			string rootFolder = Path.Combine(root, name, version);
			Directory.CreateDirectory(rootFolder);

			XmlDocument manifestDoc = new XmlDocument();
			XmlNode manifestRootNode = manifestDoc.CreateElement("package");
			manifestDoc.AppendChild(manifestRootNode);
			manifestDoc.Save(Path.Combine(rootFolder, name + ".build"));

			XmlDocument initializeDoc = new XmlDocument();
			XmlNode initializeRootNode = initializeDoc.CreateElement("project");
			initializeDoc.AppendChild(initializeRootNode);
			initializeDoc.Save(Path.Combine(rootFolder, "Initialize.xml"));

			XmlDocument buildDoc = new XmlDocument();
			XmlNode buildRootNode = buildDoc.CreateElement("project");
			buildDoc.AppendChild(buildRootNode);
			buildDoc.Save(Path.Combine(rootFolder, name + ".xml"));
		}

		private XmlNode CreatePackageNode(XmlDocument doc, string name, string version, string allowOverride = null, bool isMetaPackage = false, string isLocalOnDemand = null)
		{
			XmlNode packageNode = null;
			if (isMetaPackage)
			{
				packageNode = doc.CreateElement("metapackage");
			}
			else
			{
				packageNode = doc.CreateElement("package");
			}

			packageNode.SetAttribute("name", name);
			packageNode.SetAttribute("version", version);
			if (!String.IsNullOrEmpty(allowOverride))
				packageNode.SetAttribute("allowoverride", allowOverride);
			if (!String.IsNullOrEmpty(isLocalOnDemand))
				packageNode.SetAttribute("localondemand", isLocalOnDemand);
			return packageNode;
		}
		#endregion
	}
}
 