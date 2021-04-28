using System;
using System.Collections.Generic;
using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Tests.Util;

using EA.FrameworkTasks.Model;

namespace EA.Tasks.Tests
{
	[TestClass]
	public class GlobalPropertyTests : NAntTestClassWithDirectory
	{
		[TestMethod]
		public void MasterconfigGlobalPropertyInitTest()
		{
			// setup a package with a module that uses buildlayout
			string buildFile = CreatePackage
			(
				"TestPackage", "test",
				FileContents
				(
					"<project>",
					"",
					"	<package name='TestPackage'/>",
					"",
					"	<MakeStyle name='TestMakeStyleModule'>",
					"		<MakeBuildCommand>",
					"			echo make build config:${config}, TestGlobalProperty:${TestGlobalProperty}",
					"		</MakeBuildCommand>",
					"		<MakeCleanCommand>",
					"			echo make clean config:${config}, TestGlobalProperty:${TestGlobalProperty}",
					"		</MakeCleanCommand>",
					"	</MakeStyle>",
					"",
					"</project>"
				)
			);

			string[] testConfigs = new string[] { "pc-vc-debug", "pc64-vc-opt" };

			// setup a basic config package
			CreateConfigPackage("TestConfig", "test", configs: testConfigs);

			// init package map and project
			Project project = new Project(Log.Silent, buildFile);
			InitializePackageMapMasterConfig
			(
				project,
				$"<project>",
				$"	<globalproperties>",
				$"		TestGlobalProperty=DefaultValue",
				$"		<if condition=\"${{config-system}}==pc64\">",
				$"			TestGlobalProperty=OverrideValue",
				$"		</if>",
				$"	</globalproperties>",
				$"	<masterversions>",
				$"		<package name='TestPackage' version='test'/>",
				$"		<package name='TestConfig' version='test'/>",
				$"	</masterversions>",
				$"	<packageroots>",
				$"		<packageroot>{PackageRoot()}</packageroot>",
				$"	</packageroots>",
				$"	<config package='TestConfig' default='pc-vc-debug'/>",
				$"</project>"
			);

			project.Run(); // run the module definiion

			// create build graph
			BuildGraph buildGraph = CreateBuildGraph
			(
				project,
				buildConfigurations: testConfigs,
				buildGroupNames: new string[] { "runtime" }
			);

			// check module exists
			List<IModule> testModules = buildGraph.Modules.Values.Where(m => m.Name == "TestMakeStyleModule").ToList();
			Assert.IsTrue(testModules != null, "Couldn't find TestMakeStyleModule in build graph!");
			Assert.IsTrue(testModules.Count() == 2, "Expecting to see 2 TestMakeStyleModule modules (one for each config) but don't see 2 modules.");

			// Now test the global property is indeed what we expected
			foreach (string config in testConfigs)
			{
				IModule configModule = testModules.Where(m => m.Configuration.Name == config).FirstOrDefault();
				Assert.IsTrue(configModule != null, String.Format("Couldn't find module for config {0} in build graph", config));
				switch (config)
				{
					case "pc-vc-debug":
						Assert.IsTrue(configModule.Package.Project.Properties["TestGlobalProperty"] == "DefaultValue");
						break;
					case "pc64-vc-opt":
						Assert.IsTrue(configModule.Package.Project.Properties["TestGlobalProperty"] == "OverrideValue");
						break;
					default:
						Assert.Fail("Test case setup error for MasterconfigGlobalPropertyInitTest");
						break;
				}
			}
		}
	}
}