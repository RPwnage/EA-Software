using System;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Console;
using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Metrics;
using NAnt.Core.PackageCore;
using NAnt.Tests.Util;

namespace ConsoleRunner.Tests
{
	[TestClass]
    public class ConsoleRunnerTests : NAntTestClassBase
    {
		internal string CapturedOutput { get { return m_capturedOutput.ToString(); } }

		private TextWriter m_storedConsoleOut;
		private StringWriter m_capturedOutput;

		[TestInitialize]
		public override void TestSetup()
		{
			Telemetry.Enabled = false; // don't try and log telemetry for these invocations

			m_storedConsoleOut = Console.Out;
			m_capturedOutput = new StringWriter();
			Console.SetOut(m_capturedOutput);

			Option.Reset();

			base.TestSetup();
		}

		[TestCleanup]
		public override void TestCleanUp()
		{
			base.TestCleanUp();

			Console.SetOut(m_storedConsoleOut);
			m_capturedOutput.Dispose();
		}

		[TestMethod]
		public void HelpTest()
		{
			int returnCode = RunNant("-help");

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Ok, returnCode);
			Assert.IsTrue(CapturedOutput.Contains("Usage: nant"));
		}

		[TestMethod]
		public void InvalidArgumentTest()
		{
			int returnCode = RunNant("-asdf");

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Error, returnCode);
			Assert.IsTrue(CapturedOutput.Contains("Unknown command line option '-asdf'"));
		}

		[TestMethod]
		public void VersionTest()
		{
			int returnCode = RunNant("-version");

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Ok, returnCode);
			Assert.IsTrue(Regex.IsMatch(CapturedOutput, @"NAnt-[\d\.]+ Copyright \(C\) \d+-\d+ Electronic Arts"));
		}

		[TestMethod]
		public void ShowMasterConfigMissingMasterConfigTest()
		{
			int returnCode = RunNant("-showmasterconfig", "-masterconfigfile:Z:\\not-a-file-that-exists-probably.xml");

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Error, returnCode);
			Assert.IsTrue(CapturedOutput.Contains("Could not find"));
		}

		[TestMethod]
		public void ShowMasterConfigTest()
		{
			string masterConfigPath = Path.Combine(Environment.CurrentDirectory, "masterconfig.xml");
			SeedXmlDocumentCache
			(
				masterConfigPath,
				"<project>",
				"	<masterversions>",
				"		<package name='test_package' version='test'/>",
				"	</masterversions>",
				"	<config package='eaconfig'/>",
				"</project>"
			);

			int returnCode = RunNant("-showmasterconfig", $"-masterconfigfile:{masterConfigPath}");

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Ok, returnCode);

			// test the output is a valid masterconfig by attempting to load it (ensures that nothing but the masterconfig was output)
			string capturedOutput = CapturedOutput;
			MasterConfig masterConfig = MasterConfig.Load(new Project(Log.Silent), masterConfigPath);

			Assert.IsTrue(masterConfig.Packages.ContainsKey("test_package"));
			Assert.AreEqual("eaconfig", masterConfig.Config.Package);
		}

		[TestMethod]
		public void PropertiesFileTest()
		{
			string propertiesFile = Path.Combine(Environment.CurrentDirectory, "properties.xml");
			SeedXmlDocumentCache
			(
				propertiesFile,
				"<project>",
				"	<properties>",
				"		<property name='propertyOne' value='propertyOneFromFile'/>",
				"		<property name='propertyTwo' value='propertyTwoFromFile'/>",
				"	</properties>",
				"	<globalproperties>",
				"		<property name='propertyThree' value='propertyThreeFromFile'/>",
				"	</globalproperties>",
				"</project>"
			);
			string masterConfigPath = Path.Combine(Environment.CurrentDirectory, "masterconfig.xml");
			SeedXmlDocumentCache
			(
				masterConfigPath,
				"<project>",
				"	<globalproperties>",
				"		propertyOne=propertyOneFromMasterConfig",
				"		propertyTwo=propertyTwoFromMasterConfig",
				"		propertyThree=propertyThreeFromMasterConfig",
				"		propertyFour=propertyFourFromMasterConfig",
				"		propertyFive=propertyFiveFromMasterConfig",
				"	</globalproperties>",
				"	<config package='eaconfig'/>",
				"</project>"
			);
			string buildFile = Path.Combine(Environment.CurrentDirectory, "test.build");
			SeedXmlDocumentCache
			(
				buildFile,
				"<project>",
				"	<property name='propertyOne' value='propertyOneFromBuild'/>",
				"	<property name='propertyTwo' value='propertyTwoFromBuild'/>",
				"	<property name='propertyThree' value='propertyThreeFromBuild'/>",
				"	<property name='propertyFour' value='propertyFourFromBuild'/>",
				// skip five
				"	<property name='propertySix' value='propertySixFromBuild'/>",
				"	<echo message='propertyOne final: ${propertyOne}'/>",
				"	<echo message='propertyTwo final: ${propertyTwo}'/>",
				"	<echo message='propertyThree final: ${propertyThree}'/>",
				"	<echo message='propertyFour final: ${propertyFour}'/>",
				"	<echo message='propertyFive final: ${propertyFive}'/>",
				"	<echo message='propertySix final: ${propertySix}'/>",
				"</project>"
			);

			int returnCode = RunNant
			(
				$"-buildfile:{buildFile}", 
				$"-masterconfigfile:{masterConfigPath}", 
				$"-propertiesfile:{propertiesFile}",
				$"-D:propertyOne=propertyOneFromCommandLine"
			);

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Ok, returnCode);

			Assert.IsTrue(CapturedOutput.Contains($"Property 'propertyOne' specifed via {propertiesFile} was already specified via command line or another file. Using value 'propertyOneFromCommandLine'."));

			Assert.IsTrue(CapturedOutput.Contains("propertyOne final: propertyOneFromCommandLine"));	// command line trumps everything else
			Assert.IsTrue(CapturedOutput.Contains("propertyTwo final: propertyTwoFromFile"));			// property file comes next
			Assert.IsTrue(CapturedOutput.Contains("propertyThree final: propertyThreeFromFile"));		// property file global
			Assert.IsTrue(CapturedOutput.Contains("propertyFour final: propertyFourFromBuild"));		// global property from masterconfig but there are writable in top level so build file wins
			Assert.IsTrue(CapturedOutput.Contains("propertyFive final: propertyFiveFromMasterConfig"));	// nothing else sets this, so it comes from masterconfig
			Assert.IsTrue(CapturedOutput.Contains("propertySix final: propertySixFromBuild"));			// set only in build file
		}

		[TestMethod]
		public void DuplicateLocalGlobalPropertyFileTest()
		{
			string propertiesFile = Path.Combine(Environment.CurrentDirectory, "properties.xml");
			SeedXmlDocumentCache
			(
				propertiesFile,
				"<project>",
				"	<properties>",
				"		<property name='diffProperty' value='LocalVersion'/>",
				"		<property name='sameProperty' value='SameVersion'/>",
				"	</properties>",
				"	<globalproperties>",
				"		<property name='diffProperty' value='GlobalVersion'/>",
				"		<property name='sameProperty' value='SameVersion'/>",
				"	</globalproperties>",
				"</project>"
			);
			string masterConfigPath = Path.Combine(Environment.CurrentDirectory, "masterconfig.xml");
			SeedXmlDocumentCache
			(
				masterConfigPath,
				"<project>",
				"	<config package='eaconfig'/>",
				"</project>"
			);
			string buildFile = Path.Combine(Environment.CurrentDirectory, "test.build");
			SeedXmlDocumentCache(buildFile, "<project/>");

			int returnCode = RunNant
			(
				$"-buildfile:{buildFile}",
				$"-masterconfigfile:{masterConfigPath}",
				$"-propertiesfile:{propertiesFile}"
			);

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Ok, returnCode);
		}

		[TestMethod]
		public void TracePropTest()
		{
			string masterConfigPath = Path.Combine(Environment.CurrentDirectory, "masterconfig.xml");
			SeedXmlDocumentCache
			(
				masterConfigPath,
				"<project>",
				"	<config package='eaconfig'/>",
				"</project>"
			);
			string buildFile = Path.Combine(Environment.CurrentDirectory, "test.build");
			SeedXmlDocumentCache
			(
				buildFile,
				"<project>",
				"	<property name='propertyFromBuild' value='propertyFromBuild'/>",
				"</project>"
			);

			int returnCode = RunNant
			(
				$"-buildfile:{buildFile}",
				$"-masterconfigfile:{masterConfigPath}",
				//$"-propcasesensitive:Sensitive",
				$"-D:propertyFromCommandLine=propertyFromCommandLine",
				$"-traceprop:propertyFromBuild propertyFromCommandLine"
			);

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Ok, returnCode);

			Assert.IsTrue(CapturedOutput.Contains("TRACE set property propertyfromcommandline=propertyFromCommandLine"));
			Assert.IsTrue(CapturedOutput.Contains($"TRACE set property propertyfrombuild=propertyFromBuild at {buildFile}"));
		}

		[TestMethod]
		public void DuplicateMasterConfigFileTest()
		{
			string masterConfigPath = Path.Combine(Environment.CurrentDirectory, "masterconfig.xml");
			SeedXmlDocumentCache
			(
				masterConfigPath,
				"<project>",
				"	<config package='eaconfig'/>",
				"</project>"
			);
			string buildFile = Path.Combine(Environment.CurrentDirectory, "test.build");
			SeedXmlDocumentCache(buildFile, "<project/>");

			int returnCode = RunNant
			(
				$"-buildfile:{buildFile}",
				$"-masterconfigfile:{masterConfigPath}",
				$"-masterconfigfile:{masterConfigPath}"
			);

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Error, returnCode);

			Assert.IsTrue(CapturedOutput.Contains("Option '-masterconfigfile:' has already been specified."));
		}

		[TestMethod]
		public void InvalidPropertyArugmentTest()
		{
			string masterConfigPath = Path.Combine(Environment.CurrentDirectory, "masterconfig.xml");
			SeedXmlDocumentCache
			(
				masterConfigPath,
				"<project>",
				"	<config package='eaconfig'/>",
				"</project>"
			);
			string buildFile = Path.Combine(Environment.CurrentDirectory, "test.build");
			SeedXmlDocumentCache(buildFile, "<project/>");

			int returnCode = RunNant
			(
				$"-buildfile:{buildFile}",
				$"-masterconfigfile:{masterConfigPath}",
				$"-D:config:shouldhavebeenequals"
			);

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Error, returnCode);

			Assert.IsTrue(CapturedOutput.Contains("Invalid syntax for '-D:' parameter -D:config:shouldhavebeenequals"));
		}

		[TestMethod]
		public void WarnLevelMissingArugmentTest()
		{
			int returnCode = RunNant("-warnlevel:");

			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Error, returnCode);
			Assert.IsTrue(CapturedOutput.Contains("Option '-warnlevel:' value is missing in the argument."));
		}

		[TestMethod]
		public void WarnLevelInvalidArgumentTest()
		{
			int returnCode = RunNant("-warnlevel:not_a_valid_level");
			Assert.AreEqual((int)NAnt.Console.ConsoleRunner.ReturnCode.Error, returnCode);

			Assert.IsTrue(CapturedOutput.Contains("Failed to convert Option '-warnlevel:' value 'not_a_valid_level' to Enum 'WarnLevel'"));
		}
	}
}
