using System;
using System.IO;
using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Tests.Util;

using Newtonsoft.Json;

using EA.Eaconfig.Build;
using EA.FrameworkTasks.Model;
using System.Collections.Generic;

namespace EA.Tasks.Tests
{
	[TestClass]
	public class BuildLayoutTests : NAntTestClassWithDirectory
	{
		// this is an intentional duplication of the serialization class used in the writer
		// it's duplicated here so we can validate the contract like an external tool using
		// the json file would
		public sealed class BuildLayout
		{
			[JsonProperty(PropertyName = "version")] public int Version = 2;
			[JsonProperty(PropertyName = "platform", Order = 1)] public string Platform;
			[JsonProperty(PropertyName = "entrypoint", Order = 2)] public Dictionary<string, string> EntryPoint;
			[JsonProperty(PropertyName = "additionalfiles", Order = 3)] public List<string> AdditionalFiles;
			[JsonProperty(PropertyName = "tags", Order = 4)] public Dictionary<string, string> Tags;
		}

		[TestMethod]
		public void BuildLayoutInitializationTest()
		{
			// setup a package with a module that uses buildlayout
			string buildFile = CreatePackage
			(
				"TestPackage", "test",
				FileContents
				(
					"<project>",
					"	<optionset name='config-build-layout-tags-common'>",
					"		<option name='common-tag' value='common-value'/>",
					"	</optionset>",
					"",
					"	<optionset name='config-build-layout-entrypoint-common'>",
					"		<option name='exe' value='%output%'/>",
					"	</optionset>",
					"",
					"	<package name='TestPackage'/>",
					"",
					"	<Program name='TestProgram'>",
					"		<buildlayout enable='true' build-type='type-tag'>",
					"			<tags>",
					"				<option name='additional-tag' value='additional-value'/>",
					"			</tags>",
					"		</buildlayout>",
					"	</Program>",
					"</project>"
				)
			);

			// setup a basic config package
			CreateConfigPackage("TestConfig", "test", configs: "pc-vc-debug");

			// init package map and project
			Project project = new Project(Log.Silent, buildFile);
			InitializePackageMapMasterConfig
			(
				project,
				$"<project>",
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
				buildConfigurations : new string[] { "pc-vc-debug" },
				buildGroupNames : new string[] { "runtime" }
			);

			// check module exists
			IModule testProgram = buildGraph.Modules.Values.FirstOrDefault(m => m.Name == "TestProgram");
			if (testProgram == null)
			{
				Assert.Fail("Couldn't find test program in build graph!");
			}

			BuildLayoutGenerator generator = new BuildLayoutGenerator
			{
				Project = project
			};
			generator.Execute();

			// check a copy as added for the build layout input file
			CopyLocalInfo buildLayoutCopy = testProgram.CopyLocalFiles.FirstOrDefault(copy => copy.AbsoluteSourcePath.EndsWith("TestProgram.buildlayout.in"));
			if (buildLayoutCopy == null)
			{
				Assert.Fail("No copy was added for the build layout file!");
			}

			BuildLayout layout = null;
			using (StreamReader file = File.OpenText(buildLayoutCopy.AbsoluteSourcePath))
			{
				JsonSerializer serializer = new JsonSerializer();
				layout = (BuildLayout)serializer.Deserialize(file, typeof(BuildLayout));
			}

			// check tags and entry point are set correctly
			Assert.IsTrue(layout.EntryPoint.ContainsKey("exe"));
			Assert.IsTrue(layout.EntryPoint["exe"].EndsWith("TestProgram")); // no exe suffix because we're using a test config that does not set one
			Assert.IsTrue(layout.Tags.ContainsKey("common-tag"));
			Assert.AreEqual("common-value", layout.Tags["common-tag"]);
			Assert.IsTrue(layout.Tags.ContainsKey("additional-tag"));
			Assert.AreEqual("additional-value", layout.Tags["additional-tag"]);

			// pc specific
			Assert.IsTrue(layout.AdditionalFiles.Any(file => file.EndsWith("TestProgram.pdb")));
			Assert.IsTrue(layout.AdditionalFiles.Any(file => file.EndsWith("TestProgram.map")));
		}
	}
}