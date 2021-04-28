using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Collections.Generic;

namespace NAnt.LangTools.UnitTests
{
	[TestClass]
	public class ParserModuleTests : ParserTestsBase
	{
		/*[TestMethod]
		public void EmptyModule()
		{
			Parser parser = ParsePackage
			(
				"MyPackage",
				Script("<project>",
				"	<package/>",
				"",
				"	<Module name=\"MyModule\"/>",
				"</project>")
			);

		}*/

		[TestMethod]
		public void EmptyLibrary()
		{
			Parser parser = ParsePackage
			(
				"MyPackage",
				Script("<project>",
				"	<package/>",
				"",
				"	<Library name=\"MyModule\"/>",
				"</project>")
			);

			Property.Node runtimeModules = parser.Properties["runtime.buildmodules"];
			Assert.IsInstanceOfType(runtimeModules, typeof(Property.String));
			Assert.AreEqual("MyModule", (runtimeModules as Property.String).Value);

			Property.Node myModuleType = parser.Properties["runtime.MyModule.buildtype"];
			Assert.IsInstanceOfType(myModuleType, typeof(Property.String));
			Assert.AreEqual("Library", (myModuleType as Property.String).Value);
		}

		[TestMethod]
		public void LibraryOverriddenToDllInSharedElement()
		{
			Parser parser = ParsePackage
			(
				"MyPackage",
				Script("<project>",
				"	<package/>",
				"",
				"	<Library name=\"MyModule\">",
				"		<buildtype name=\"DynamicLibrary\" if=\"${Dll}\"/>",
				"	</Library>",
				"</project>")
			);

			Property.Node runtimeModules = parser.Properties["runtime.buildmodules"];
			Assert.IsInstanceOfType(runtimeModules, typeof(Property.String));
			Assert.AreEqual("MyModule", (runtimeModules as Property.String).Value);

			Property.Node myModuleType = parser.Properties["runtime.MyModule.buildtype"];
			Assert.IsInstanceOfType(myModuleType, typeof(Property.Conditional));

			Dictionary<string, ConditionValueSet> expansion = myModuleType.Expand();
			Assert.AreEqual(2, expansion.Count);

			TestUtils.VerifyPropertyExpansionConditionSets
			(
				expansion, "Library",
				new Dictionary<Condition, string>() { { Constants.SharedBuildCondition, "false" } }
			);
			TestUtils.VerifyPropertyExpansionConditionSets
			(
				expansion, "DynamicLibrary",
				new Dictionary<Condition, string>() { { Constants.SharedBuildCondition, "true" } }
			);
		}

		[TestMethod]
		public void LibraryWithIncludeDirs()
		{
			Parser parser = ParsePackage
			(
				"MyPackage",
				Script("<project>",
				"	<package/>",
				"",
				"	<Library name=\"MyModule\">",
				"		<includedirs>",
				"			alpha",
				"			beta",
				"		</includedirs>",
				"	</Library>",
				"</project>")
			);

			Property.Node runtimeModules = parser.Properties["runtime.buildmodules"];
			Assert.IsInstanceOfType(runtimeModules, typeof(Property.String));
			Assert.AreEqual("MyModule", (runtimeModules as Property.String).Value);

			Property.Node myModuleType = parser.Properties["runtime.MyModule.buildtype"];
			Assert.IsInstanceOfType(myModuleType, typeof(Property.String));
			Assert.AreEqual("Library", (myModuleType as Property.String).Value);

			Property.Node includeDirNode = parser.Properties["runtime.MyModule.includedirs"];
			Assert.IsInstanceOfType(includeDirNode, typeof(Property.String));
			string includeDirsString = (includeDirNode as Property.String).Value;

			Assert.IsTrue(includeDirsString.Contains(parser.PackageDirToken + "/alpha"));
			Assert.IsTrue(includeDirsString.Contains(parser.PackageDirToken + "/beta"));
		}

		[TestMethod]
		public void LibraryWithConditionalAppendIncludeDirs()
		{
			Parser parser = ParsePackage
			(
				"MyPackage",
				Script("<project>",
				"	<package/>",
				"",
				"	<property name=\"runtime.MyModule.includedirs\">",
				"		alpha",
				"		beta",
				"	</property>",
				"",
				"	<Library name=\"MyModule\">",
				"		<includedirs append=\"'${config-system}' != 'capilano' and '${config-system}' != 'kettle'\">",
				"			gamma",
				"			delta",
				"			<do if=\"'${config-system}' == 'capilano'\">",
				"				capilano",
				"			</do>",
				"			<do if=\"'${config-system}' == 'pc64'\">",
				"				pc64",
				"			</do>",
				"		</includedirs>",
				"	</Library>",
				"</project>")
			);

			Property.Node includeDirNode = parser.Properties["runtime.MyModule.includedirs"];
			Dictionary<string, ConditionValueSet> expansion = includeDirNode.Expand();
			Assert.AreEqual(4, expansion.Count);
		}
	}
}