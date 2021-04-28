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
using System.Linq;
using System.Text;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Tests.Util;

using EA.FrameworkTasks;
using EA.FrameworkTasks.Functions;

namespace EA.Tasks.Tests
{
	[TestClass]
	public class ConfigLoadingTests : NAntTestClassWithDirectory
	{
		private string m_testPackageBuildFile;

		[TestInitialize]
		public override void TestSetup()
		{
			base.TestSetup();

			m_testPackageBuildFile = CreatePackage("mypackage", "test");

			CreateConfigPackage("testconfig", "test", createTopLevelInit: false); // indivual test cases will set up top level init in different ways as needed
	
			// BACKWARD COMPATIBLITY: load.xml is used in the backward compatibility cases, we also want to verify that it is not used in the non backward compatibility cases.
			Directory.CreateDirectory(Path.Combine(m_testDir, "testconfig", "test", "config", "platformloader"));
			File.WriteAllText
			(
				Path.Combine(m_testDir, "testconfig", "test", "config", "platformloader", "load.xml"),
				String.Join
				(
					Environment.NewLine,
					$"<project>",
					$"	<include file='../global/init.xml'/>",
					$"	<property name='test.loadxml' value='true'/>",
					$"	<LoadPlatformConfig/>",
					$"	<Combine/>",
					$"</project>"
				)
			);
			Directory.CreateDirectory(Path.Combine(m_testDir, "testconfig", "test", "config", "global"));
			WriteFile(Path.Combine(PackageDirectory("testconfig", "test"), "config", "global", "init.xml"), "<project/>"); // blank init by default, specific tests will override this with the full version

			CreateConfigPackage("FBConfig", "test", createTopLevelInit: false);
			CreateConfigPackage("custom_config", "test", createTopLevelInit: false);
		}

		/// <summary>This test ensure that config extensions can be defined in masterconfig fragments
		/// and that they are appended to the list of configs in the correct order.</summary>
		[TestMethod]
		public void ExtensionsFromFragmentsTest()
		{
			CreateTopLevelConfigGlobalInit("testconfig", "test");

			CreateExtensionFragment("fifa_config");
			CreateConfigPackage("fifa_config", "test");
			CreateCompleteGlobalInit("fifa_config");

			CreateExtensionFragment("madden_config");
			CreateConfigPackage("madden_config", "test");
			CreateCompleteGlobalInit("madden_config");

			CreateConfig("fifa_config", "test", "pc-fifa-debug");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask(project, "pc-fifa-debug", useExtensions: true);

			Assert.IsTrue(PackageFunctions.ConfigExtensionEnabled(project, "custom_config") == "true");
			Assert.IsTrue(PackageFunctions.ConfigExtensionEnabled(project, "FBConfig") == "true");
			Assert.IsTrue(PackageFunctions.ConfigExtensionEnabled(project, "fifa_config") == "true");
			Assert.IsTrue(PackageFunctions.ConfigExtensionEnabled(project, "asdf_config") == "false");

			Assert.IsTrue(PackageMap.Instance.MasterConfig.Config.Extensions.Count == 4);
			Assert.IsTrue(PackageMap.Instance.MasterConfig.Config.Extensions[0].Name == "custom_config");
			Assert.IsTrue(PackageMap.Instance.MasterConfig.Config.Extensions[1].Name == "FBConfig");
			Assert.IsTrue(PackageMap.Instance.MasterConfig.Config.Extensions[2].Name == "fifa_config");
			Assert.IsTrue(PackageMap.Instance.MasterConfig.Config.Extensions[3].Name == "madden_config");

			Assert.IsTrue(project.Properties["test.fifa_config.init-loaded"] == "true");
			Assert.IsTrue(project.Properties["test.madden_config.init-loaded"] == "true");

			VerifyLoadOrder(project, "testconfig", "fifa_config", "madden_config");
		}

		// Test that Framework fails if it can't find a config file that matches the name
		[TestMethod]
		public void MissingConfigFileTest()
		{
			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			BuildException exception = Assert.ThrowsException<BuildException>
			(
				() => ExecutePackageTask(project, "pc-vc-debug")
			);
			Assert.IsFalse(project.Properties.Contains("test.loadxml"));
			Assert.IsTrue(exception.GetBaseException().Message.Contains("Can not load configuration 'pc-vc-debug'")
				|| exception.GetBaseException().Message.Contains("Cannot find configuration file")
				|| exception.GetBaseException().Message.Contains("Could not find the file 'pc-vc-debug.xml' in any of the config packages")); // TODO: this is to match the old message, it should be changed in framework 8
		}

		// above but with extensions enabled
		[TestMethod]
		public void MissingConfigFileWithExtensionsTest()
		{
			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			BuildException exception = Assert.ThrowsException<BuildException>
			(
				() => ExecutePackageTask(project, "pc-vc-debug", useExtensions: true)
			);
			Assert.IsFalse(project.Properties.Contains("test.loadxml"));
			Assert.IsTrue(exception.GetBaseException().Message.Contains("Can not load configuration 'pc-vc-debug'")
				|| exception.GetBaseException().Message.Contains("Cannot find configuration file")
				|| exception.GetBaseException().Message.Contains("Could not find the file 'pc-vc-debug.xml' in any of the config packages")); // TODO: this is to match the old message, it should be changed in framework 8
		}

		[TestMethod]
		public void MissingConfigFileMismatchTest()
		{
			// create config files that don't match the target
			WriteFile(Path.Combine(PackageDirectory("testconfig", "test"), "config", "pc-vc-opt.xml"), "<project/>");
			WriteFile(Path.Combine(PackageDirectory("FBConfig", "test"), "config", "pc64-vc-debug.xml"), "<project/>");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			BuildException exception = Assert.ThrowsException<BuildException>
			(
				() => ExecutePackageTask(project, "pc-vc-debug")
			);
			Assert.IsFalse(project.Properties.Contains("test.loadxml"));
			Assert.IsTrue(exception.GetBaseException().Message.Contains("Can not load configuration 'pc-vc-debug'")
				|| exception.GetBaseException().Message.Contains("Cannot find configuration file")
				|| exception.GetBaseException().Message.Contains("Could not find the file 'pc-vc-debug.xml' in any of the config packages")); // TODO: this is to match the old message, it should be changed in framework 8
		}

		[TestMethod]
		public void MissingConfigFileMismatchWithExtensionsTest()
		{
			// create config files that don't match the target
			WriteFile(Path.Combine(PackageDirectory("testconfig", "test"), "config", "pc-vc-opt.xml"), "<project/>");
			WriteFile(Path.Combine(PackageDirectory("FBConfig", "test"), "config", "pc64-vc-debug.xml"), "<project/>");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			BuildException exception = Assert.ThrowsException<BuildException>
			(
				() => ExecutePackageTask(project, "pc-vc-debug", useExtensions: true)
			);
			Assert.IsFalse(project.Properties.Contains("test.loadxml"));
			Assert.IsTrue(exception.GetBaseException().Message.Contains("Can not load configuration 'pc-vc-debug'")
				|| exception.GetBaseException().Message.Contains("Cannot find configuration file")
				|| exception.GetBaseException().Message.Contains("Could not find the file 'pc-vc-debug.xml' in any of the config packages")); // TODO: this is to match the old message, it should be changed in framework 8
		}

		// Test that if the only thing that is in the config package is an empty config file it fails because configuration optionset is missing
		[TestMethod]
		public void EmptyConfigFileTest()
		{
			WriteFile(Path.Combine(PackageDirectory("testconfig", "test"), "config", "pc-vc-debug.xml"), "<project/>");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			BuildException exception = Assert.ThrowsException<BuildException>
			(
				() => ExecutePackageTask(project, "pc-vc-debug")
			);
			Assert.IsFalse(project.Properties.Contains("test.loadxml"));
			Assert.AreEqual("Configuration file 'pc-vc-debug.xml' in package 'testconfig' does not define required optionset 'configuration-definition'.", exception.GetBaseException().Message);
		}

		[TestMethod]
		public void EmptyConfigFileWithExtensionsTest()
		{
			WriteFile(Path.Combine(PackageDirectory("testconfig", "test"), "config", "pc-vc-debug.xml"), "<project/>");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			BuildException exception = Assert.ThrowsException<BuildException>
			(
				() => ExecutePackageTask(project, "pc-vc-debug", useExtensions: true)
			);
			Assert.IsFalse(project.Properties.Contains("test.loadxml"));
			Assert.AreEqual("Configuration file 'pc-vc-debug.xml' in package 'testconfig' does not define required optionset 'configuration-definition'.", exception.GetBaseException().Message);
		}

		[TestMethod]
		public void EmptyConfigFileInExtensionTest()
		{
			// create an empty config file in the platform config package
			string customConfigDir = Path.Combine(m_testDir, "custom_config", "test", "config");
			Directory.CreateDirectory(customConfigDir);
			File.WriteAllText
			(
				Path.Combine(customConfigDir, "custom-vc-debug.xml"),
				"<project/>"
			);

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			BuildException exception = Assert.ThrowsException<BuildException>
			(
				() => ExecutePackageTask(project, "custom-vc-debug", useExtensions: true)
			);
			Assert.IsTrue(exception.GetBaseException().Message.Contains("does not define required optionset 'configuration-definition'"));
		}

		// Test the Minimal set of information Framework needs to successfully load a config package
		[TestMethod]
		public void MinimalConfigPackageTest()
		{
			// write minimum config settings needed to successfully load the config package
			CreateTopLevelConfigGlobalInit("testconfig", "test");
			CreateConfig("testconfig", "test", "pc-vc-debug");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask(project, "pc-vc-debug");

			Assert.IsFalse(project.Properties.Contains("test.loadxml"));
			Assert.IsTrue(project.NamedOptionSets.ContainsKey("config-options-common"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.init-loaded", false));
		}

		[TestMethod]
		public void MinimalConfigPackageWithExtensionsTest()
		{
			// write minimum config settings needed to successfully load the config package
			CreateTopLevelConfigGlobalInit("testconfig", "test");
			CreateConfig("testconfig", "test", "pc-vc-debug");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask(project, "pc-vc-debug", useExtensions: true);

			Assert.IsFalse(project.Properties.Contains("test.loadxml"));
			Assert.IsTrue(project.NamedOptionSets.ContainsKey("config-options-common"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.init-loaded", false));
		}

		[TestMethod]
		public void CompleteConfigPackageGlobalInitTest()
		{
			CreateCompleteGlobalInit("testconfig");
			CreateConfig("testconfig", "test", "pc-vc-debug");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask(project, "pc-vc-debug");

			Assert.IsTrue(project.NamedOptionSets.ContainsKey("config-options-common"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.config-common-loaded", false));
			Assert.IsTrue(project.Targets.ContainsKey("testconfig-test-target"));
		}

		[TestMethod]
		public void MissingConfigSetFileTest()
		{
			CreateConfig("testconfig", "test", "pc-vc-debug", writeConfigSetFile: false);
			CreateCompleteGlobalInit("testconfig");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ContextCarryingException exception = Assert.ThrowsException<ContextCarryingException>
			(
				() => ExecutePackageTask(project, "pc-vc-debug")
			);
			Assert.AreEqual("Could not find platform config script 'configset\\debug.xml' in main configuration package or any extension. It should likely be found inside the 'testconfig' package.", exception.GetBaseException().Message);
		}

		[TestMethod]
		public void CompleteConfigPackageOptionSetFormatTest()
		{
			CreateConfig("testconfig", "test", "pc-vc-debug");
			CreateCompleteGlobalInit("testconfig");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask(project, "pc-vc-debug");

			Assert.IsTrue(project.NamedOptionSets.ContainsKey("config-options-common"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.config-common-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.configset-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.platform-loaded", false));
			Assert.IsTrue(project.Properties["config-system"] == "pc");
			Assert.IsTrue(project.Properties["config-compiler"] == "vc");
			Assert.IsTrue(project.Targets.ContainsKey("testconfig-test-target"));
		}

		[TestMethod]
		public void ExtensionConfigLoadTest()
		{
			CreateCompleteGlobalInit("testconfig");
			CreateCompleteGlobalInit("custom_config");
			CreateConfig("custom_config", "test", "custom-x86-clang-debug", writeConfigSetFile: false);
			CreateConfigSetFile("testconfig", "test", "debug");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask(project, "custom-x86-clang-debug", useExtensions: true);

			Assert.IsTrue(project.NamedOptionSets.ContainsKey("config-options-common"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.config-common-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.configset-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.platform-loaded", false));
			Assert.IsTrue(project.Targets.ContainsKey("testconfig-test-target"));
			Assert.IsTrue(project.Properties["config-system"] == "custom");
			Assert.IsTrue(project.Properties["config-processor"] == "x86");
			Assert.IsTrue(project.Properties["config-compiler"] == "clang");
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.settings-included-by-initialize", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.targets-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.config-common-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.platform-loaded", false));
			Assert.IsTrue(project.Targets.ContainsKey("custom_config-test-target"));
		}

		[TestMethod]
		public void PlatformConfigAsExtensionTest()
		{
			CreateCompleteGlobalInit("testconfig");
			CreateCompleteGlobalInit("custom_config");
			CreateCompleteGlobalInit("FBConfig");
			CreateConfig("custom_config", "test", "custom-x86-clang-debug", writeConfigSetFile: false);
			CreateConfigSetFile("testconfig", "test", "debug");
			CreatePlatformFile("FBConfig", "test", "custom-x86-clang");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask(project, "custom-x86-clang-debug", useExtensions: true);

			Assert.IsTrue(project.NamedOptionSets.ContainsKey("config-options-common"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.config-common-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.configset-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.platform-loaded", false));
			Assert.IsTrue(project.Targets.ContainsKey("testconfig-test-target"));
			Assert.IsTrue(project.Properties["config-system"] == "custom");
			Assert.IsTrue(project.Properties["config-processor"] == "x86");
			Assert.IsTrue(project.Properties["config-compiler"] == "clang");
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.settings-included-by-initialize", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.targets-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.config-common-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.platform-loaded", false));
			Assert.IsTrue(project.Targets.ContainsKey("custom_config-test-target"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.targets-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.config-common-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.platform-loaded", false));
			Assert.IsTrue(project.Targets.ContainsKey("FBConfig-test-target"));

			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.initialize-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.initialize-loaded", false));
		}

		// this time fbconfig will be disabled using the masterconfig property, so fbconfig should not be loaded and not override custom_config
		[TestMethod]
		public void DisableConfigOverrideTest()
		{
			CreateCompleteGlobalInit("testconfig");
			CreateCompleteGlobalInit("custom_config");
			CreateCompleteGlobalInit("FBConfig");
			CreateConfig("custom_config", "test", "custom-x86-clang-debug", writeConfigSetFile: false);
			CreateConfigSetFile("testconfig", "test", "debug");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask
			(
				project,
				"custom-x86-clang-debug",
				useExtensions: true,
				additionalProperties: new Dictionary<string, string>
				{
					{  "config-use-FBConfig", "false" }
				}
			);

			Assert.IsTrue(PackageFunctions.ConfigExtensionEnabled(project, "testconfig") == "true");
			Assert.IsTrue(PackageFunctions.ConfigExtensionEnabled(project, "custom_config") == "true");
			Assert.IsTrue(PackageFunctions.ConfigExtensionEnabled(project, "FBConfig") == "false");
			Assert.IsTrue(PackageFunctions.ConfigExtensionEnabled(project, "asdf_config") == "false");

			Assert.IsTrue(project.NamedOptionSets.ContainsKey("config-options-common"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.config-common-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.configset-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.platform-loaded", false));
			Assert.IsTrue(project.Targets.ContainsKey("testconfig-test-target"));
			Assert.IsTrue(project.Properties["config-system"] == "custom");
			Assert.IsTrue(project.Properties["config-processor"] == "x86");
			Assert.IsTrue(project.Properties["config-compiler"] == "clang");
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.settings-included-by-initialize", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.pre-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.post-combine-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.init-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.targets-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.config-common-loaded", false));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.platform-loaded", false));
			Assert.IsTrue(project.Targets.ContainsKey("custom_config-test-target"));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.pre-combine-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.post-combine-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.init-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.targets-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.config-common-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.platform-loaded", false));
			Assert.IsFalse(project.Targets.ContainsKey("FBConfig-test-target"));
			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.custom_config.initialize-loaded", false));
			Assert.IsFalse(project.Properties.GetBooleanPropertyOrDefault("test.FBConfig.initialize-loaded", false));
		}

		// verify that it don't fail if you specify an overrides file in but none exists
		[TestMethod]
		public void MissingOverridesFileNoFailTest()
		{
			CreateCompleteGlobalInit("testconfig");
			CreateConfig("testconfig", "test", "pc-vc-debug");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask
			(
				project, "pc-vc-debug", 
				additionalProperties: new Dictionary<string, string>()
				{
					{ "testconfig.options-override.file", "overrides_options/does-not-exist.xml" }
				}
			);
		}

		// verify overrides file is loaded when it does exist
		[TestMethod]
		public void OverridesFileTest()
		{
			CreateCompleteGlobalInit("testconfig");
			CreateConfig("testconfig", "test", "pc-vc-debug");

			string overridesDir = Path.Combine(m_testDir, "overrides_options");
			Directory.CreateDirectory(overridesDir);
			File.WriteAllText
			(
				Path.Combine(overridesDir, "asdf.xml"),
				"<package>" +
					"<property name='test.overrides_options.base' value='true'/>" +
				"</package>"
			);

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask
			(
				project, "pc-vc-debug",
				additionalProperties: new Dictionary<string, string>()
				{
					{ "eaconfig.options-override.file", "overrides_options/asdf.xml" }
				}
			);

			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.overrides_options.base", false));
		}

		[TestMethod]
		public void PlatformOverridesFileTest()
		{
			CreateCompleteGlobalInit("testconfig");
			CreateConfig("testconfig", "test", "pc-vc-debug");

			string overridesDir = Path.Combine(m_testDir, "overrides_options");
			Directory.CreateDirectory(overridesDir);
			File.WriteAllText
			(
				Path.Combine(overridesDir, "asdf.pc.xml"),
				"<package>" +
					"<property name='test.overrides_options.pc' value='true'/>" +
				"</package>"
			);

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask
			(
				project, "pc-vc-debug",
				additionalProperties: new Dictionary<string, string>()
				{
					{ "eaconfig.options-override.file", "overrides_options/asdf.xml" }
				}
			);

			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.overrides_options.pc", false));
		}

		[TestMethod]
		public void ExplicitConfigSetTest()
		{
			CreateCompleteGlobalInit("testconfig");
			CreateConfigSetFile("testconfig", "test", "dev-custom");
			File.WriteAllText
			(
				Path.Combine(m_testDir, "testconfig", "test", "config", "pc-vc-dev-custom.xml"),
				"<package>" +
					"<optionset name='configuration-definition'>" +
						"<option name='config-system' value='pc'/>" +
						"<option name='config-compiler' value='vc'/>" +
						"<option name='configset' value='dev-custom'/>" +
						"<option name='config-processor' value='x86'/>" +
						"<option name='config-name' value='dev-custom'/>" +
						"<option name='config-platform' value='pc-vc'/>" + // config-platform doesn't have to include processor for legacy reasons
						"<option name='config-os' value='testOS'/>" +
					"</optionset>" +
				"</package>"
			);
			CreatePlatformFile("testconfig", "test", "pc-x86-vc");

			Project project = new Project(Log.Silent, m_testPackageBuildFile);
			ExecutePackageTask(project, "pc-vc-dev-custom", useExtensions: true);

			Assert.IsTrue(project.Properties.GetBooleanPropertyOrDefault("test.testconfig.configset-loaded", false));
		}

		protected override void CreateConfigPackage(string package, string version, string buildFileContents = null, string initializeFileContents = null, bool createTopLevelInit = true, params string[] configs)
		{
			base.CreateConfigPackage
			(
				package,
				version,
				buildFileContents: buildFileContents,
				initializeFileContents: FileContents
				(
					"<package>",
					"	<property name='test." + package + ".initialize-loaded' value='true'/>",
					"</package>"
				),
				createTopLevelInit: createTopLevelInit,
				configs: configs
			);
		}

		// TODO: remember to remove useLegacyConfigSystem when that system is removed
		private void ExecutePackageTask(Project project, string config, bool useExtensions = false, Dictionary<string, string> additionalProperties = null)
		{
			// setup project
			project.Properties["config"] = config;
			if (additionalProperties?.Any() ?? false)
			{
				foreach (KeyValuePair<string, string> kvp in additionalProperties)
				{
					project.Properties[kvp.Key] = kvp.Value;
				}
			}

			// create masterconfig and init package map
			StringBuilder masterConfigBuilder = new StringBuilder();
			masterConfigBuilder.AppendLine
			(
				String.Join
				(
					Environment.NewLine,
					$"<project>",
					$"	<masterversions>",
					$"		<package name='custom_config' version='test'/>",
					$"		<package name='testconfig' version='test'/>",
					$"		<package name='FBConfig' version='test'/>",
					$"		<package name='mypackage' version='test'/>",
					$"	</masterversions>",
					$"	<packageroots>",
					$"		<packageroot>{PackageRoot()}</packageroot>",
					$"	</packageroots>",
					$"	<ondemand>false</ondemand>"
				)
			);
			if (useExtensions)
			{
				masterConfigBuilder.AppendLine
				(
					String.Join
					(
						Environment.NewLine,
						"	<config package='testconfig' default='pc-vc-debug' extra-config-dir='extra_configs'>",
						"		<extension>custom_config</extension>",
						"		<extension if='${config-use-FBConfig??true}'>FBConfig</extension>",
						"	</config>"
					)
				);
			}
			else
			{
				masterConfigBuilder.AppendLine($"	<config package='testconfig' default='pc-vc-debug' extra-config-dir='extra_configs'/>");
			}
			masterConfigBuilder.AppendLine
			(
				String.Join
				(
					Environment.NewLine,
					"	<fragments>",
					"		<include name='./*_fragment.xml'/>",
					"	</fragments>"
				)
			);
			masterConfigBuilder.Append("</project>");
			InitializePackageMapMasterConfig(project, masterConfigBuilder.ToString());

			PackageTask packageTask = new PackageTask(project)
			{
				PackageName = "mypackage",
				InitializeSelf = false
			};
			packageTask.Execute();
		}

		private void CreateExtensionFragment(string extension)
		{
			File.WriteAllText
			(
				Path.Combine(m_testDir, $"{extension}_fragment.xml"),
				String.Join
				(
					Environment.NewLine,
					$"<project>",
					$"	<masterversions>",
					$"		<package name='{extension}' version='test'/>",
					$"	</masterversions>",
					$"	<config>",
					$"		<extension>{extension}</extension>",
					$"	</config>",
					$"</project>"
				)
			);
		}

		protected override void CreateTopLevelConfigGlobalInit(string configPackage, string configPackageVersion)
		{
			WriteFile
			(
				Path.Combine(PackageDirectory(configPackage, configPackageVersion), "config", "global", "init.xml"),
				$"<project>",
				$"	<property name=\"test.testconfig.init-loaded\" value=\"true\"/>",
				$"	<property name='test.load-order' value='${{property.value}} testconfig'/>",
				$"	<optionset name='config-options-common'>",
				$"		<option name=\"buildset.protobuildtype\" value=\"true\"/>",
				$"	</optionset>",
				$"	<optionset name='config-options-program'>",
				$"		<option name=\"buildset.name\" value=\"StdProgram\"/>",
				$"		<option name=\"buildset.tasks\" value=\"asm cc link\"/>",
				$"	</optionset>",
				$"	<optionset name='config-options-library'>",
				$"		<option name=\"buildset.name\" value=\"StdLibrary\"/>",
				$"		<option name=\"buildset.tasks\" value=\"asm cc lib\"/>",
				$"	</optionset>",
				$"	<optionset name='config-options-dynamiclibrary'>",
				$"		<option name=\"generatedll\" value=\"on\"/>",
				$"		<option name=\"buildset.name\" value=\"DynamicLibrary\"/>",
				$"		<option name=\"buildset.tasks\" value=\"asm cc link\"/>",
				$"	</optionset>",
				$"</project>"
			);
		}

		private void CreateCompleteGlobalInit(string package)
		{
			string globalDir = Path.Combine(m_testDir, package, "test", "config", "global");
			if (package != "testconfig")
			{
				WriteFile
				(
					Path.Combine(globalDir, "init.xml"),
					$"<project>",
					$"	<property name='test.{package}.init-loaded' value='true'/>",
					$"	<property name='test.load-order' value='${{property.value}} {package}'/>",
					$"</project>"
				);
			}
			else
			{
				// testconfig has special global init because it has to setup the basic optionsets
				CreateTopLevelConfigGlobalInit(package, "test");
			}

			WriteFile
			(
				Path.Combine(m_testDir, package, "test", "config", "platform", "config-common.xml"),
				$"<project>",
				$"	<property name='test.{package}.config-common-loaded' value='true'/>",
				$"</project>"
			);
			WriteFile
			(
				Path.Combine(m_testDir, package, "test", "config", "targets", "targets.xml"),
				$"<project>",
				$"	<property name='test.{package}.targets-loaded' value='true'/>",
				$"	<fail message='targets.xml was not loaded after post-combine.xml' unless='@{{PropertyExists(\"test.{package}.post-combine-loaded\")}}'/>",
				$"	<target name='{package}-test-target'/>",
				$"</project>"
			);
			WriteFile
			(
				Path.Combine(globalDir, "pre-combine.xml"),
				$"<project>",
				$"<fail message='pre-combine.xml was not loaded after init.xml' unless='@{{PropertyExists(\"test.{package}.init-loaded\")}}'/>",
				$"	<property name='test.{package}.pre-combine-loaded' value='true'/>",
				$"</project>"
			);
			WriteFile
			(
				Path.Combine(globalDir, "post-combine.xml"),
				$"<project>",
				$"	<property name='test.{package}.post-combine-loaded' value='true'/>",
				$"	<fail message='pre-combine.xml was not loaded before post-combine.xml' unless='@{{PropertyExists(\"test.{package}.pre-combine-loaded\")}}'/>",
				$"</project>"
			);
		}

		protected override void CreateConfigSetFile(string configPackage, string configPackageVersion, string configName)
		{
			WriteFile
			(
				Path.Combine(PackageDirectory(configPackage, configPackageVersion), "config", "configset", $"{configName}.xml"),
				$"<project>",
				$"	<property name='test.{configPackage}.configset-loaded' value='true'/>",
				$"</project>"
			);
		}

		protected override void CreatePlatformFile(string configPackage, string configPackageVersion, string platformFile)
		{
			WriteFile
			(
				Path.Combine(PackageDirectory(configPackage, configPackageVersion), "config", "platform", $"{platformFile}.xml"),
				$"<project>" +
				$"	<fail message='platform was not loaded before the configset.xml' if='@{{PropertyExists(\"test.{configPackage}.configset-loaded\")}}'/>" +
				$"	<property name='test.{configPackage}.platform-loaded' value='true'/>" +
				$"</project>"
			);
		}

		private void VerifyLoadOrder(Project project, params string[] packages)
		{
			int index = -1;
			string loadOrder = project.Properties["test.load-order"] ?? String.Empty;
			foreach (string package in packages)
			{
				int newIdx = loadOrder.IndexOf(package);
				if (newIdx == -1)
				{
					Assert.Fail($"Package {package} was not loaded!");
				}
				else if (newIdx <= index)
				{
					Assert.Fail($"Package {package} was not loaded in correct order!");
				}
				else
				{
					index = newIdx;
				}
			}
		}
	}
}