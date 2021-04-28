using System.Collections.Generic;
using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace NAnt.LangTools.UnitTests
{
	[TestClass]
	public class ParserPublicDataTests : ParserTestsBase
	{
		[TestMethod]
		public void EmptyPublicData()
		{
			ParsePackage
			(
				"MyPackage",
				buildScript: Script("<project>",
				"	<package/>",
				"</project>"),
				initializeXml: Script("<project>",
				"	<publicdata packagename=\"MyPackage\"/>",
				"</project>")
			);
		}

		[TestMethod]
		public void EmptyPublicDataGroups()
		{
			ParsePackage
			(
				"MyPackage",
				buildScript: Script("<project>",
				"	<package/>",
				"</project>"),
				initializeXml: Script("<project>",
				"	<publicdata packagename=\"MyPackage\">",
				"		<runtime/>",
				"		<tests/>",
				"		<examples/>",
				"		<tools/>",
				"	</publicdata>",
				"</project>")
			);
		}

		[TestMethod]
		public void EmptyModules()
		{
			Parser parser = ParsePackage
			(
				"MyPackage",
				buildScript: Script("<project>",
				"	<package/>",
				"</project>"),
				initializeXml: Script("<project>",
				"	<publicdata packagename=\"MyPackage\">",
				"		<module name=\"ImplicitRuntime\"/>",
				"		<runtime>",
				"			<module name=\"ExplicitRuntime\"/>",
				"		</runtime>",
				"		<tests>",
				"			<module name=\"Test\"/>",
				"		</tests>",
				"		<examples>",
				"			<module name=\"Example\"/>",
				"		</examples>",
				"		<tools>",
				"			<module name=\"Tool\"/>",
				"		</tools>",
				"	</publicdata>",
				"</project>")
			);

			Property.Node runtime = parser.Properties["package.MyPackage.runtime.buildmodules"];
			Assert.IsInstanceOfType(runtime, typeof(Property.String));
			Assert.AreEqual("\nImplicitRuntime\nExplicitRuntime", (runtime as Property.String).Value);

			Property.Node test = parser.Properties["package.MyPackage.test.buildmodules"];
			Assert.IsInstanceOfType(test, typeof(Property.String));
			Assert.AreEqual("\nTest", (test as Property.String).Value);

			Property.Node example = parser.Properties["package.MyPackage.example.buildmodules"];
			Assert.IsInstanceOfType(example, typeof(Property.String));
			Assert.AreEqual("\nExample", (example as Property.String).Value);

			Property.Node tool = parser.Properties["package.MyPackage.tool.buildmodules"];
			Assert.IsInstanceOfType(tool, typeof(Property.String));
			Assert.AreEqual("\nTool", (tool as Property.String).Value);
		}

		[TestMethod]
		public void SimpleDefinesAndIncludes()
		{
			Parser parser = ParsePackage
			(
				"MyPackage",
				buildScript: Script("<project>",
				"	<package/>",
				"</project>"),
				initializeXml: Script("<project>",
				"	<publicdata packagename=\"MyPackage\">",
				"		<module name=\"MyModule\">",
				"			<defines>",
				"				ONE=1",
				"				TWO=2",
				"			</defines>",
				"			<includedirs>",
				"				alpha",
				"				beta",
				"			</includedirs>",
				"		</module>",
				"	</publicdata>",
				"</project>")
			);

			Property.Node includeDirNode = parser.Properties["package.MyPackage.MyModule.includedirs"];
			Assert.IsInstanceOfType(includeDirNode, typeof(Property.String));
			string includeDirsString = (includeDirNode as Property.String).Value;

			Assert.IsTrue(includeDirsString.Contains(parser.PackageDirToken + "/alpha"));
			Assert.IsTrue(includeDirsString.Contains(parser.PackageDirToken + "/beta"));

			Property.Node definesNode = parser.Properties["package.MyPackage.MyModule.defines"];
			Assert.IsInstanceOfType(definesNode, typeof(Property.String));
			string definesString = (definesNode as Property.String).Value;

			Assert.IsTrue(definesString.Contains("ONE=1"));
			Assert.IsTrue(definesString.Contains("TWO=2"));
		}
	}
}