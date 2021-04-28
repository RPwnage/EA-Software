using System;
using System.IO;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using NAnt.Core.Logging;
using NAnt.Tests.Util;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class ProjectTests : NAntTestClassBase
	{
		[TestMethod]
		public void ProjectBaseDirTest()
		{
			string buildDirectory = Path.Combine(Environment.CurrentDirectory, "build");
			string buildFile = Path.Combine(buildDirectory, "buildfile.build");
			string include1Directory = Path.Combine(Environment.CurrentDirectory, "include1");
			string include1File = Path.Combine(include1Directory, "include1.build");
			SeedXmlDocumentCache
			(
				buildFile,
				$"<project>" +
				$"	<echo message='BaseDirectory 1: ${{nant.project.basedir}}'/>" +
				$"	<include file='{include1File}'/>" +
				$"	<echo message='BaseDirectory 7: ${{nant.project.basedir}}'/>" +
				$"</project>"
			);
			string include2Directory = Path.Combine(Environment.CurrentDirectory, "include2");
			string include2File = Path.Combine(include2Directory, "include2.xml");
			SeedXmlDocumentCache
			(
				include1File,
				$"<project>" +
				$"	<echo message='BaseDirectory 2: ${{nant.project.basedir}}'/>" +
				$"	<include file='{include2File}'/>" +
				$"	<echo message='BaseDirectory 6: ${{nant.project.basedir}}'/>" +
				$"</project>"
			);
			string include3Directory = Path.Combine(Environment.CurrentDirectory, "include3");
			string include3File = Path.Combine(include3Directory, "include3.xml");
			SeedXmlDocumentCache
			(
				include2File,
				$"<project>" +
				$"	<echo message='BaseDirectory 3: ${{nant.project.basedir}}'/>" +
				$"	<include file='{include3File}'/>" +
				$"	<echo message='BaseDirectory 5: ${{nant.project.basedir}}'/>" +
				$"</project>"
			);
			SeedXmlDocumentCache
			(
				include3File,
				$"<project>" +
				$"	<echo message='BaseDirectory 4: ${{nant.project.basedir}}'/>" +
				$"</project>"
			);

			StringBuilder builder = new StringBuilder();
			BufferedListener listener = new BufferedListener(builder);
			Log echoLog = new Log(level: Log.LogLevel.Normal, listeners: new ILogListener[] { listener });

			Project project = new Project(echoLog, buildFile);
			project.Run();

			string echoLogText = builder.ToString();
			Assert.IsTrue(echoLogText.Contains($"BaseDirectory 1: {buildDirectory}"));
			Assert.IsTrue(echoLogText.Contains($"BaseDirectory 2: {include1Directory}"));
			Assert.IsTrue(echoLogText.Contains($"BaseDirectory 3: {include2Directory}"));
			Assert.IsTrue(echoLogText.Contains($"BaseDirectory 4: {include3Directory}"));
			Assert.IsTrue(echoLogText.Contains($"BaseDirectory 5: {include2Directory}"));
			Assert.IsTrue(echoLogText.Contains($"BaseDirectory 6: {include1Directory}"));
			Assert.IsTrue(echoLogText.Contains($"BaseDirectory 7: {buildDirectory}"));
		}
	}
}