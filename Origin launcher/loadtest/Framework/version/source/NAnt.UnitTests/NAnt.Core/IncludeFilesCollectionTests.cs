using System;
using System.IO;
using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Logging;
using NAnt.Tests.Util;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class IncludeFilesCollectionTests : NAntTestClassBase
	{
		[TestMethod]
		public void IncludeCollectionTests()
		{
			string buildFile = Path.Combine(Environment.CurrentDirectory, "buildfile.build");
			string include1 = "include1.xml";
			string include2 = "include2.xml";
			SeedXmlDocumentCache
			(
				buildFile,
				$"<project>" +
				$"	<include file='{include1}'/>" +
				$"	<include file='{include2}'/>" +
				$"</project>"
			);
			string include1Path = Path.Combine(Environment.CurrentDirectory, include1);
			SeedXmlDocumentCache
			(
				include1Path,
				$"<project/>"
			);
			string include2Path = Path.Combine(Environment.CurrentDirectory, include2);
			string include3 = "include3.xml";
			SeedXmlDocumentCache
			(
				include2Path,
				$"<project>" +
				$"	<include file='{include3}'/>" +
				$"</project>"
			);
			string include3Path = Path.Combine(Environment.CurrentDirectory, include3);
			SeedXmlDocumentCache
			(
				include3Path,
				$"<project/>"
			);

			Project project = new Project(Log.Silent, buildFile);
			project.Run(); // have to run the project in order to start including files

			Assert.IsTrue(project.IncludedFiles.Contains(buildFile));
			Assert.AreEqual(2, project.IncludedFiles.GetIncludedFiles(buildFile).Count());
			Assert.AreEqual(3, project.IncludedFiles.GetIncludedFiles(buildFile, recursive: true).Count());

			Assert.IsTrue(project.IncludedFiles.Contains(include1Path));
			Assert.AreEqual(0, project.IncludedFiles.GetIncludedFiles(include1Path).Count());
			Assert.AreEqual(0, project.IncludedFiles.GetIncludedFiles(include1Path, recursive: true).Count());

			Assert.IsTrue(project.IncludedFiles.Contains(include2Path));
			Assert.AreEqual(1, project.IncludedFiles.GetIncludedFiles(include2Path).Count());
			Assert.AreEqual(1, project.IncludedFiles.GetIncludedFiles(include2Path, recursive: true).Count());

			Assert.IsTrue(project.IncludedFiles.Contains(include3Path));
			Assert.AreEqual(0, project.IncludedFiles.GetIncludedFiles(include3Path).Count());
			Assert.AreEqual(0, project.IncludedFiles.GetIncludedFiles(include3Path, recursive: true).Count());
		}
	}
}