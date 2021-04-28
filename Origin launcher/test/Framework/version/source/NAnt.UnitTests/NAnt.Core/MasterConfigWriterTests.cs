using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Tests.Util;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class MasterConfigWriterTests : NAntTestClassBase
	{
		/// <summary>
		/// This is a test for a bug that we noticed in our unit test merging code
		/// Where the masterconfig would be evaluated to use version 1.8 but the uri would still indicate 1.7
		/// </summary>
		[TestMethod]
		public void UriTest()
		{
			string testDir = TestUtil.CreateTestDirectory("uritest");
			try
			{
				Project project = new Project(Log.Silent);

				MasterConfig masterconfig = new MasterConfig(Path.Combine(testDir, "masterconfig.xml"));

				MasterConfig.GroupType grouptype = new MasterConfig.GroupType("masterversions");
				MasterConfig.Package masterPackage = new MasterConfig.Package
				(
					grouptype, "jdk", "1.7", 
					attributes: new Dictionary<string, string>() { { "uri", "1.7" } }
				);
				MasterConfig.Exception<MasterConfig.IPackage> exception = new MasterConfig.Exception<MasterConfig.IPackage>("use_jdk_1_8");
				MasterConfig.PackageCondition condition = new MasterConfig.PackageCondition
				(
					masterPackage, "true", "1.8", 
					attributes: new Dictionary<string, string>() { { "uri", "1.8" } }
				);
				exception.Add(condition);
				masterPackage.Exceptions.Add(exception);
				masterconfig.Packages.Add("jdk", masterPackage);

				project.Properties["use_jdk_1_8"] = "true";

				PackageMap.Init(project, masterconfig);

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig, project)
					{
						// The masterconfig that is written already has all of the fragments merged together so we don't need the fragments section
						HideFragments = true,
						// Resolve the various package roots to get their full roots since the relative paths will be broken
						ResolveRoots = true,
						// Evaluate the master versions based on the startupProperties
						EvaluateMasterVersions = true,
						// Need to preserve the global properties the way they are in the masterconfig
						ResolveGlobalProperties = false,
						// Need to make sure that group types are not evaluated ahead of time
						EvaluateGroupTypes = false,
						// Turn off attribute added to show which fragment a package/property came from
						ShowFragmentSources = false,
						// Remove metapackage attributes
						RemoveMetaPackages = true
					};

					masterconfigWriter.Write();
					xmlWriter.Flush();
					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					Assert.IsTrue(generatedMasterconfig.Contains("name=\"jdk\" version=\"1.8\" uri=\"1.8\""));
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
		public void ConfigInfoTest()
		{
			string testDir = TestUtil.CreateTestDirectory("ConfigInfoTest");
			try
			{
				Project project = new Project(Log.Silent);
				MasterConfig masterconfig = new MasterConfig(Path.Combine(testDir, "masterconfig.xml"));

				Exception e = Assert.ThrowsException<BuildException>(() => new MasterConfig.ConfigInfo("eaconfig", "android-gcc-debug", "includes", "excludes"));
				Assert.IsTrue(e.Message.Contains("<config> cannot have excludes and includes both in masterconfig file"));

				masterconfig.Config = new MasterConfig.ConfigInfo("eaconfig", "android-gcc-debug", "unix-vc-retail", null);

				PackageMap.Init(project, masterconfig);

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig, project);
					masterconfigWriter.Write();
					xmlWriter.Flush();
					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					Assert.IsTrue(generatedMasterconfig.Contains("package=\"eaconfig\""));
					Assert.IsTrue(generatedMasterconfig.Contains("default=\"android-gcc-debug\""));
					Assert.IsTrue(generatedMasterconfig.Contains("includes=\"unix-vc-retail\""));
				}

				MasterConfig masterconfig2 = new MasterConfig(Path.Combine(testDir, "masterconfig.xml"));

				List<MasterConfig.ConfigExtension> extensions = new List<MasterConfig.ConfigExtension>();
				extensions.Add(new MasterConfig.ConfigExtension("android_config", null));
				extensions.Add(new MasterConfig.ConfigExtension("FBConfig", "${config-use-FBConfig??false}"));
				masterconfig2.Config = new MasterConfig.ConfigInfo("eaconfig", "android-gcc-debug", null, "unix-vc-retail", extensions);

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig2, project);
					masterconfigWriter.Write();
					xmlWriter.Flush();
					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					Assert.IsTrue(generatedMasterconfig.Contains("package=\"eaconfig\""));
					Assert.IsTrue(generatedMasterconfig.Contains("default=\"android-gcc-debug\""));
					Assert.IsTrue(generatedMasterconfig.Contains("excludes=\"unix-vc-retail\""));
					Assert.IsTrue(generatedMasterconfig.Contains("<extension>android_config</extension>"));
					Assert.IsTrue(generatedMasterconfig.Contains("<extension if=\"${config-use-FBConfig??false}\">FBConfig</extension>"));
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
		public void PackageRootTest()
		{
			string testDir = TestUtil.CreateTestDirectory("PackageRootTest");
			try
			{
				Project project = new Project(Log.Silent);
				MasterConfig masterconfig = new MasterConfig(Path.Combine(testDir, "masterconfig.xml"));

				masterconfig.PackageRoots.Add(new MasterConfig.PackageRoot("./packageroot1"));
				masterconfig.PackageRoots.Add(new MasterConfig.PackageRoot("../packageroot2", 1, 2));
				masterconfig.PackageRoots.Add(new MasterConfig.PackageRoot("C:/packageroot3"));

				PackageMap.Init(project, masterconfig);

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig, project)
					{
						ResolveRoots = false
					};
					masterconfigWriter.Write();
					xmlWriter.Flush();

					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					Assert.IsTrue(generatedMasterconfig.Contains("<packageroot>./packageroot1</packageroot>"));
					Assert.IsTrue(generatedMasterconfig.Contains("<packageroot minlevel=\"1\" maxlevel=\"2\">../packageroot2</packageroot>"));
					Assert.IsTrue(generatedMasterconfig.Contains("<packageroot>C:/packageroot3</packageroot>"));
				}

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig, project)
					{
						ResolveRoots = true
					};
					masterconfigWriter.Write();
					xmlWriter.Flush();

					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					Assert.IsTrue(generatedMasterconfig.Contains("<packageroot>" + Path.Combine(testDir, "packageroot1") + "</packageroot>"));
					Assert.IsTrue(generatedMasterconfig.Contains("<packageroot minlevel=\"1\" maxlevel=\"2\">" + Path.Combine(Path.GetDirectoryName(testDir), "packageroot2") + "</packageroot>"));
					Assert.IsTrue(generatedMasterconfig.Contains("<packageroot>C:\\packageroot3</packageroot>"));
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
		public void GlobalPropertiesAndDefinesTest()
		{
			string testDir = TestUtil.CreateTestDirectory("GlobalPropertiesTest");
			try
			{
				Project project = new Project(Log.Silent);

				MasterConfig masterconfig = new MasterConfig(Path.Combine(testDir, "masterconfig.xml"));

				masterconfig.GlobalProperties.Add(new MasterConfig.GlobalProperty("my-prop", "my-prop-value", null));
				masterconfig.GlobalProperties.Add(new MasterConfig.GlobalProperty("my-quoted-prop", "my-prop-value", null, quoted: true));
				masterconfig.GlobalProperties.Add(new MasterConfig.GlobalProperty("my-valueless-prop", null, null));
				masterconfig.GlobalProperties.Add(new MasterConfig.GlobalProperty("my-xml-prop", "my-xml-prop-value", null, useXmlElement: true));
				masterconfig.GlobalProperties.Add(new MasterConfig.GlobalProperty("my-xml-condition-prop", "my-xml-prop-value", "${my-prop} eq 'my-prop-value'", useXmlElement: true));
				masterconfig.GlobalProperties.Add(new MasterConfig.GlobalProperty("my-conditional-prop", "my-prop-value", "${my-prop} eq 'my-prop-value'"));

				masterconfig.GlobalDefines.Add(new MasterConfig.GlobalDefine("my-define", "some-value", null, false));
				masterconfig.GlobalDefines.Add(new MasterConfig.GlobalDefine("my-dotnet-define", null, null, true));

				PackageMap.Init(project, masterconfig);

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig, project)
					{
						ResolveGlobalProperties = false
					};
					masterconfigWriter.Write();
					xmlWriter.Flush();
					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					XmlDocument doc = new XmlDocument();
					doc.LoadXml(generatedMasterconfig);

					XmlNode projectNode = doc.GetChildElementByName("project");
					XmlNode globalDefinesNode = projectNode.GetChildElementByName("globaldefines");
					XmlNode globalDotNetDefinesNode = projectNode.GetChildElementByName("globaldotnetdefines");
					XmlNode globalPropertiesNode = projectNode.GetChildElementByName("globalproperties");
					XmlNode globalDefine = globalDefinesNode.GetChildElementByName("globaldefine");
					XmlNode globalDotNetDefine = globalDotNetDefinesNode.GetChildElementByName("globaldefine");
					XmlNode propCondition = globalPropertiesNode.GetChildElementByName("if");
					XmlNode xmlProp = globalPropertiesNode.GetChildElementsByName("globalproperty").Where(x => x.GetAttributeValue("condition", null) == null).First();
					XmlNode xmlConditionProp = globalPropertiesNode.GetChildElementsByName("globalproperty").Where(x => x.GetAttributeValue("condition", null) != null).First();

					Assert.IsTrue(globalPropertiesNode.InnerText.Contains("my-prop=my-prop-value"));
					Assert.IsTrue(globalPropertiesNode.InnerText.Contains("my-quoted-prop=\"my-prop-value\""));
					Assert.IsTrue(globalPropertiesNode.InnerText.Contains("my-valueless-prop"));
					Assert.IsFalse(globalPropertiesNode.InnerText.Contains("my-valueless-prop="));
					Assert.AreEqual(propCondition.GetAttributeValue("condition"), "${my-prop} eq 'my-prop-value'");
					Assert.AreEqual(propCondition.InnerText, "my-conditional-prop=my-prop-value");
					Assert.AreEqual(xmlProp.InnerText, "my-xml-prop-value");
					Assert.AreEqual(xmlConditionProp.GetAttributeValue("condition"), "${my-prop} eq 'my-prop-value'");
					Assert.AreEqual(xmlConditionProp.InnerText, "my-xml-prop-value");

					Assert.AreEqual(globalDefine.GetAttributeValue("name"), "my-define");
					Assert.AreEqual(globalDefine.GetAttributeValue("value"), "some-value");
						
					Assert.AreEqual(globalDotNetDefine.GetAttributeValue("name"), "my-dotnet-define");
					Assert.AreEqual(globalDotNetDefine.GetAttributeValue("value", null), null);
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
		public void FragmentTest()
		{
			string testDir = TestUtil.CreateTestDirectory("FragmentTest");
			try
			{
				Project project = new Project(Log.Silent);
				MasterConfig masterconfig = new MasterConfig(Path.Combine(testDir, "masterconfig.xml"));

				MasterConfig.FragmentDefinition fragment = new MasterConfig.FragmentDefinition();
				fragment.MasterConfigPath = "C:/masterconfig_fragment.xml";
				MasterConfig.FragmentDefinition conditionalFragment = new MasterConfig.FragmentDefinition();
				conditionalFragment.MasterConfigPath = "C:/condition1/masterconfig_fragment.xml";
				MasterConfig.Exception<string> exception = new MasterConfig.Exception<string>("my-prop");
				MasterConfig.StringCondition condition = new MasterConfig.StringCondition("my-value", "C:/condition2/masterconfig_fragment.xml");
				exception.Add(condition);
				conditionalFragment.Exceptions.Add(exception);
				masterconfig.FragmentDefinitions.Add(fragment);
				masterconfig.FragmentDefinitions.Add(conditionalFragment);

				PackageMap.Init(project, masterconfig);

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig, project);
					masterconfigWriter.ResolveGlobalProperties = false;
					masterconfigWriter.Write();
					xmlWriter.Flush();

					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					XmlDocument doc = new XmlDocument();
					doc.LoadXml(generatedMasterconfig);

					XmlNode projectNode = doc.GetChildElementByName("project");
					XmlNode fragmentsNode = projectNode.GetChildElementByName("fragments");
					XmlNode fragmentNode = fragmentsNode.GetChildElementsByName("include").Where(x => !x.HasChildNodes).First();
					XmlNode conditionalFragmentNode = fragmentsNode.GetChildElementsByName("include").Where(x => x.HasChildNodes).First();
					XmlNode exceptionNode = conditionalFragmentNode.GetChildElementByName("exception");
					XmlNode conditionNode = exceptionNode.GetChildElementByName("condition");

					Assert.AreEqual(fragmentNode.GetAttributeValue("name"), "C:/masterconfig_fragment.xml");
					Assert.AreEqual(conditionalFragmentNode.GetAttributeValue("name"), "C:/condition1/masterconfig_fragment.xml");
					Assert.AreEqual(exceptionNode.GetAttributeValue("propertyname"), "my-prop");
					Assert.AreEqual(conditionNode.GetAttributeValue("value"), "my-value");
					Assert.AreEqual(conditionNode.GetAttributeValue("name"), "C:/condition2/masterconfig_fragment.xml");
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
		public void GroupTypeTest()
		{
			string testDir = TestUtil.CreateTestDirectory("GroupTypeTest");
			try
			{
				Project project = new Project(Log.Silent);
				MasterConfig masterconfig = new MasterConfig(Path.Combine(testDir, "masterconfig.xml"));

				MasterConfig.GroupType grouptype = new MasterConfig.GroupType("sdks");
				masterconfig.GroupMap["sdks"] = grouptype;
				MasterConfig.Package masterPackage = new MasterConfig.Package
				(
					grouptype, "jdk", "1.7",
					attributes: new Dictionary<string, string>() { { "uri", "1.7" } }
				);

				MasterConfig.Exception<MasterConfig.IPackage> exception = new MasterConfig.Exception<MasterConfig.IPackage>("use_jdk_1_8");
				MasterConfig.PackageCondition condition = new MasterConfig.PackageCondition
				(
					masterPackage, "true", "1.8",
					attributes: new Dictionary<string, string>()
					{
						{ "uri", "1.8" }
					}
				);

				exception.Add(condition);
				masterPackage.Exceptions.Add(exception);
				masterconfig.Packages.Add("jdk", masterPackage);

				PackageMap.Init(project, masterconfig);

				using (Writers.XmlWriter xmlWriter = new Writers.XmlWriter(XmlFormat.Default))
				{
					MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig, project)
					{
						ResolveGlobalProperties = false
					};
					masterconfigWriter.Write();
					xmlWriter.Flush();
					string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());
					XmlDocument doc = new XmlDocument();
					doc.LoadXml(generatedMasterconfig);

					XmlNode projectNode = doc.GetChildElementByName("project");
					XmlNode masterVersionsNode = projectNode.GetChildElementByName("masterversions");
					XmlNode groupNode = masterVersionsNode.GetChildElementByName("grouptype");
					XmlNode packageNode = groupNode.GetChildElementByName("package");

					Assert.AreEqual("sdks", groupNode.GetAttributeValue("name"));
					Assert.AreEqual("jdk", packageNode.GetAttributeValue("name"));
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
	}
}
