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
using System.IO;
using System.Security.Principal;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Tests.Util;
using NAnt.Win32Tasks.Functions;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class FunctionTests : NAntTestClassBase
	{
		[TestMethod]
		public void AreAllPackagesInMasterConfigTest()
		{
			string testDir = TestUtil.CreateTestDirectory("AreAllPackagesInMasterConfigTest");

			string masterconfigPath = Path.Combine(testDir, "masterconfig1.xml");
			File.WriteAllText(masterconfigPath,
				"<project>" +
					"<masterversions>" +
						"<package name='PackageA' version='dev'/>" +
						"<package name='PackageB' version='dev'/>" +
						"<package name='eaconfig' version='dev'/>" +
					"</masterversions>" +
					// secondary test to see that functions work in the masterconfig
					"<globalproperties>" +
						"<if condition=\"@{PropertyExists('test.exists1')}\">" +
							"test.value1=true" +
						"</if>" +
						"<if condition=\"@{PropertyExists('test.exists2')}\">" +
							"test.value2=true" +
						"</if>" +
					"</globalproperties>" +
					"<packageroots><packageroot>.</packageroot></packageroots>" +
					"<config package='eaconfig'/>" +
				"</project>");
			Directory.CreateDirectory(Path.Combine(testDir, "eaconfig", "dev"));
			File.WriteAllText(Path.Combine(testDir, "eaconfig", "dev", "Manifest.xml"),
				"<package>" +
					"<packageName>eaconfig</packageName>" +
					"<versionName>dev</versionName>" +
					"<frameworkVersion>2</frameworkVersion>" +
					"<buildable>false</buildable>" +
				"</package>");
			File.WriteAllText(Path.Combine(testDir, "eaconfig", "dev", "eaconfig.build"),
				"<package>" +
					"<package name=\"eaconfig\"/>" +
				"</package>");

			Project project = new Project(Log.Silent);
			project.Properties["test.exists1"] = "true";

			PackageMap.Init(project, MasterConfig.Load(project, masterconfigPath));

			// DAVE-FUTURE-REFACTOR-TODO - see comment on SetProjectMasterconfigProperties in ConsoleRunner.cs, fixing it removes need for this block
			foreach (Project.GlobalPropertyDef propdef in Project.GlobalProperties.EvaluateExceptions(project))
			{
				if (propdef.InitialValue != null && !project.Properties.Contains(propdef.Name))
				{
					project.Properties.Add(propdef.Name, propdef.InitialValue);
				}
			}

			Assert.AreEqual("true", EA.FrameworkTasks.Functions.PackageFunctions.AreAllPackagesInMasterconfig(project, "packageA packageB"));
			Assert.AreEqual("false", EA.FrameworkTasks.Functions.PackageFunctions.AreAllPackagesInMasterconfig(project, "packageC packageD"));
			Assert.AreEqual("true", project.Properties["test.value1"]);
			Assert.IsFalse(project.Properties.Contains("test.value2"));
		}

		[TestMethod]
		public void StringCompareVersionsTests()
		{
			Assert.AreEqual(StringFunctions.StrCompareVersions(null, "test-01", "test-1"), "0");
			Assert.AreEqual(StringFunctions.StrCompareVersions(null, "test-10-z", "test-10-z"), "0");
			Assert.AreEqual(StringFunctions.StrCompareVersions(null, "test-1.1", "test-1.01"), "0");
			Assert.AreEqual(StringFunctions.StrCompareVersions(null, "test-1.1", "test-001.01"), "0");

			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-1", "test-11")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-01", "test-11")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-01", "test-02")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-12", "test-1")) > 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-12", "test-01")) > 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-012", "test-01")) > 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test", "test-01")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-01", "test")) > 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-9", "test-10")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-10", "test-9")) > 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-10-abc", "test-10")) > 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-10", "test-10-abc")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-10-z", "test-10-abc")) > 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-10-z", "test-10-0z")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "test-1.1", "test-001-01")) > 0);

			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "dev", "work")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "dev", "work-Igor")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "dev-Igor", "work-Igor")) < 0);
			Assert.IsTrue(int.Parse(StringFunctions.StrCompareVersions(null, "dev-John", "work-Igor")) < 0);
		}

		[TestMethod]
		public void StringFunctionTests()
		{
			Assert.AreEqual(StringFunctions.StrLen(null, string.Empty), "0");
			Assert.AreEqual(StringFunctions.StrLen(null, "nant.exe"), "8");

			Assert.AreEqual(StringFunctions.StrEndsWith(null, "nant.exe", "exe"), bool.TrueString);
			Assert.AreEqual(StringFunctions.StrEndsWith(null, "nant.exe", "nant"), bool.FalseString);

			Assert.AreEqual(StringFunctions.StrStartsWith(null, "nant.exe", "exe"), bool.FalseString);
			Assert.AreEqual(StringFunctions.StrStartsWith(null, "nant.exe", "nant"), bool.TrueString);

			Assert.AreEqual(StringFunctions.StrCompare(null, "nant", "nant"), "0");

			Assert.AreEqual(StringFunctions.StrCompareVersions(null, "1.00.00", "1.00.00"), "0");

			Assert.AreEqual(StringFunctions.StrVersionLess(null, "1.00.00", "1.05.00"), bool.TrueString);
			Assert.AreEqual(StringFunctions.StrVersionLess(null, "1.05.00", "1.00.00"), bool.FalseString);
			Assert.AreEqual(StringFunctions.StrVersionLess(null, "1.05.00", "1.05.00"), bool.FalseString);

			Assert.AreEqual(StringFunctions.StrVersionGreater(null, "1.00.00", "1.05.00"), bool.FalseString);
			Assert.AreEqual(StringFunctions.StrVersionGreater(null, "1.05.00", "1.00.00"), bool.TrueString);
			Assert.AreEqual(StringFunctions.StrVersionGreater(null, "1.05.00", "1.05.00"), bool.FalseString);

			Assert.AreEqual(StringFunctions.StrVersionLessOrEqual(null, "1.00.00", "1.05.00"), bool.TrueString);
			Assert.AreEqual(StringFunctions.StrVersionLessOrEqual(null, "1.05.00", "1.00.00"), bool.FalseString);
			Assert.AreEqual(StringFunctions.StrVersionLessOrEqual(null, "1.05.00", "1.05.00"), bool.TrueString);

			Assert.AreEqual(StringFunctions.StrVersionGreaterOrEqual(null, "1.00.00", "1.05.00"), bool.FalseString);
			Assert.AreEqual(StringFunctions.StrVersionGreaterOrEqual(null, "1.05.00", "1.00.00"), bool.TrueString);
			Assert.AreEqual(StringFunctions.StrVersionGreaterOrEqual(null, "1.05.00", "1.05.00"), bool.TrueString);

			Assert.AreEqual(StringFunctions.StrLastIndexOf(null, "nant.exe.config", "."), "8");

			Assert.AreEqual(StringFunctions.StrIndexOf(null, "nant.exe", "."), "4");

			// Note: contains returns true/false as lower case strings, unlike other functions that return True/False (from the C# ToString)
			// This ideally should be changed but would be a fairly major breaking change
			// for now we need to ensure that the case does not change without us being aware of it since there are possibly things relying on this casing.
			Assert.AreEqual(StringFunctions.StrContains(null, "nant.exe", "ant"), "true");
			Assert.AreEqual(StringFunctions.StrContains(null, "nant.exe", "make"), "false");

			Assert.AreEqual(StringFunctions.StrRemove(null, "nant.exe", 4, 1), "nantexe");

			Assert.AreEqual(StringFunctions.StrReplace(null, "nant.exe", ".exe", ".config"), "nant.config");

			Assert.AreEqual(StringFunctions.StrInsert(null, ".exe", 0, "nant"), "nant.exe");

			Assert.AreEqual(StringFunctions.StrSubstring(null, "nant.exe", 4), ".exe");

			Assert.AreEqual(StringFunctions.StrSubstring(null, "nant.exe", 0, 4), "nant");

			Assert.AreEqual(StringFunctions.StrConcat(null, "nant", ".exe"), "nant.exe");

			Assert.AreEqual(StringFunctions.StrPadLeft(null, "nant", 8), "    nant");

			Assert.AreEqual(StringFunctions.StrPadRight(null, "nant", 8), "nant    ");

			Assert.AreEqual(StringFunctions.StrTrim(null, "  nant  "), "nant");
			Assert.AreEqual(StringFunctions.StrTrimWhiteSpace(null, "  nant  "), "nant");

			Assert.AreEqual(StringFunctions.StrToUpper(null, "nant"), "NANT");
			Assert.AreEqual(StringFunctions.StrToUpper(null, "NAnt"), "NANT");

			Assert.AreEqual(StringFunctions.StrToLower(null, "NANT"), "nant");
			Assert.AreEqual(StringFunctions.StrToLower(null, "NAnt"), "nant");

			Assert.AreEqual(StringFunctions.StrPascalCase(null, "hello world"), "HelloWorld");
			Assert.AreEqual(StringFunctions.StrPascalCase(null, "hElLo WoRlD"), "HelloWorld");

			Assert.AreEqual(StringFunctions.StrEcho(null, "nant"), "nant");

			Assert.AreEqual(StringFunctions.StrIsEmpty(null, string.Empty), bool.TrueString);
			Assert.AreEqual(StringFunctions.StrIsEmpty(null, "nant"), bool.FalseString);

			Assert.AreEqual(StringFunctions.DistinctItems(null, "a b f c d d e f g"), "a b f c d e g");

			Assert.AreEqual(StringFunctions.DistinctItemsCustomSeparator(null, "a b f c d d e f g", "|"), "a|b|f|c|d|e|g");
		}

		[TestMethod]
		public void CharFunctionTests()
		{
			Assert.AreEqual(CharacterFunctions.CharIsDigit(null, "5", 0), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsDigit(null, "a", 0), bool.FalseString);
			Assert.AreEqual(CharacterFunctions.CharIsDigit(null, "asdf5", 4), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsDigit(null, "asdf5", 0), bool.FalseString);

			Assert.AreEqual(CharacterFunctions.CharIsNumber(null, "5", 0), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsNumber(null, "a", 0), bool.FalseString);
			Assert.AreEqual(CharacterFunctions.CharIsNumber(null, "asdf5", 4), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsNumber(null, "asdf5", 0), bool.FalseString);

			Assert.AreEqual(CharacterFunctions.CharIsWhiteSpace(null, " ", 0), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsWhiteSpace(null, "a", 0), bool.FalseString);
			Assert.AreEqual(CharacterFunctions.CharIsWhiteSpace(null, "asdf ", 4), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsWhiteSpace(null, "asdf ", 0), bool.FalseString);

			Assert.AreEqual(CharacterFunctions.CharIsLetter(null, "a", 0), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsLetter(null, "1", 0), bool.FalseString);
			Assert.AreEqual(CharacterFunctions.CharIsLetter(null, "asdfa", 4), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsLetter(null, "asdf5", 4), bool.FalseString);

			Assert.AreEqual(CharacterFunctions.CharIsLetterOrDigit(null, "a", 0), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsLetterOrDigit(null, "1", 0), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsLetterOrDigit(null, "#", 0), bool.FalseString);

			Assert.AreEqual(CharacterFunctions.CharToLower(null, 'A'), "a");
			Assert.AreEqual(CharacterFunctions.CharToUpper(null, 'a'), "A");

			Assert.AreEqual(CharacterFunctions.CharIsLower(null, "a", 0), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsLower(null, "A", 0), bool.FalseString);

			Assert.AreEqual(CharacterFunctions.CharIsUpper(null, "A", 0), bool.TrueString);
			Assert.AreEqual(CharacterFunctions.CharIsUpper(null, "a", 0), bool.FalseString);
		}

		[TestMethod]
		public void TargetFunctionTests()
		{
			Project project = new Project(Log.Silent);

			Target target = new Target();
			target.Name = "build";
			project.Targets.Add(target);

			Assert.AreEqual(TargetFunctions.TargetExists(project, "build"), bool.TrueString);
			Assert.AreEqual(TargetFunctions.TargetExists(project, "not-a-real-target"), bool.FalseString);
		}

		[TestMethod]
		public void PropertyFunctionTests()
		{
			Project project = new Project(Log.Silent);

			Assert.AreEqual(PropertyFunctions.PropertyExists(project, "nant.version"), bool.TrueString);
			Assert.AreEqual(PropertyFunctions.PropertyExists(project, "not-a-real-property"), bool.FalseString);

			project.Properties.Add("is-true", bool.TrueString);
			project.Properties.Add("is-false", bool.FalseString);
			project.Properties.Add("is-value", "asdf");

			Assert.AreEqual(PropertyFunctions.PropertyTrue(project, "is-true"), bool.TrueString);
			Assert.AreEqual(PropertyFunctions.PropertyTrue(project, "is-false"), bool.FalseString);
			Assert.ThrowsException<BuildException>(() => PropertyFunctions.PropertyTrue(project, "not-a-real-property"));
			Assert.ThrowsException<BuildException>(() => PropertyFunctions.PropertyTrue(project, "is-value"));

			Assert.AreEqual(PropertyFunctions.PropertyExists(project, "is-true"), bool.TrueString);
			PropertyFunctions.PropertyUndefine(project, "is-true");
			Assert.AreEqual(PropertyFunctions.PropertyExists(project, "is-true"), bool.FalseString);

			Assert.AreEqual(PropertyFunctions.IsPropertyGlobal(project, "nant.transitive"), bool.TrueString);
			Assert.AreEqual(PropertyFunctions.IsPropertyGlobal(project, "is-value"), bool.FalseString);

			Assert.AreEqual(PropertyFunctions.GetPropertyOrDefault(project, "is-value", "defualt"), "asdf");
			Assert.AreEqual(PropertyFunctions.GetPropertyOrDefault(project, "is-not-value", "default"), "default");
		}

		[TestMethod]
		public void OptionSetFunctionTests()
		{
			Project project = new Project(Log.Silent);

			OptionSet optionset = new OptionSet();
			optionset.Options.Add("optimizations", "true");
			optionset.Options.Add("debugflags", "on");
			optionset.Options.Add("retailflags", "off");
			optionset.Options.Add("pch", "false");
			optionset.Options.Add("filepath", "C:/temp/log.txt");
			project.NamedOptionSets.Add("my-build-options", optionset);

			Assert.AreEqual(OptionSetFunctions.OptionSetGetValue(project, "my-build-options", "debugflags"), "on");
			Assert.AreEqual(OptionSetFunctions.OptionSetGetValue(project, "my-build-options", "asdf"), string.Empty);
			Assert.AreEqual(OptionSetFunctions.OptionSetGetValue(project, "zxcv", "asdf"), string.Empty);

			Assert.AreEqual(OptionSetFunctions.OptionSetExists(project, "my-build-options"), bool.TrueString);
			Assert.AreEqual(OptionSetFunctions.OptionSetExists(project, "zxcv"), bool.FalseString);

			Assert.AreEqual(OptionSetFunctions.OptionSetOptionExists(project, "my-build-options", "debugflags"), bool.TrueString);
			Assert.AreEqual(OptionSetFunctions.OptionSetOptionExists(project, "my-build-options", "asdf"), bool.FalseString);
			Assert.AreEqual(OptionSetFunctions.OptionSetOptionExists(project, "zxcv", "asdf"), bool.FalseString);

			Assert.AreEqual(OptionSetFunctions.OptionSetToString(project, "my-build-options"), 
				"start optionset name=my-build-options\r\n    options\r\n        optimizations=true\r\n        debugflags=on\r\n        retailflags=off\r\n        pch=false\r\n        filepath=C:/temp/log.txt\r\nend optionset\r\n");

			OptionSetFunctions.OptionSetOptionUndefine(project, "my-build-options", "debugflags");
			Assert.AreEqual(OptionSetFunctions.OptionSetOptionExists(project, "my-build-options", "debugflags"), bool.FalseString);

			OptionSetFunctions.OptionSetUndefine(project, "my-build-options");
			Assert.AreEqual(OptionSetFunctions.OptionSetExists(project, "my-build-options"), bool.FalseString);
		}

		[TestMethod]
		public void FileSetFunctionTests()
		{
			Project project = new Project(Log.Silent);

			FileSet fileset = new FileSet(project);
			fileset.BaseDirectory = "D:/packages/testpackage/dev/source";
			fileset.Includes.Add(new FileSetItem(PatternFactory.Instance.CreatePattern("one.txt"), asIs:true));
			fileset.Includes.Add(new FileSetItem(PatternFactory.Instance.CreatePattern("two.txt"), asIs: true));
			fileset.Excludes.Add(new FileSetItem(PatternFactory.Instance.CreatePattern("three.txt"), asIs: true));
			project.NamedFileSets.Add("sourcefiles", fileset);

			Assert.AreEqual(FileSetFunctions.FileSetExists(project, "sourcefiles"), bool.TrueString);
			Assert.AreEqual(FileSetFunctions.FileSetExists(project, "zxcv"), bool.FalseString);

			Assert.AreEqual(FileSetFunctions.FileSetCount(project, "sourcefiles"), "2");
			Assert.ThrowsException<BuildException>(() => FileSetFunctions.FileSetCount(project, "zxcv"));

			Assert.AreEqual(FileSetFunctions.FileSetGetItem(project, "sourcefiles", 0), "one.txt");
			Assert.AreEqual(FileSetFunctions.FileSetGetItem(project, "sourcefiles", 1), "two.txt");
			Assert.ThrowsException<BuildException>(() => FileSetFunctions.FileSetGetItem(project, "zxcv", 0));
			Assert.ThrowsException<BuildException>(() => FileSetFunctions.FileSetGetItem(project, "sourcefiles", 5));

			Assert.AreEqual(FileSetFunctions.FileSetToString(project, "sourcefiles", ";"), "one.txt;two.txt");

			Assert.ThrowsException<BuildException>(() => FileSetFunctions.FileSetDefinitionToXmlString(project, "zxcv"));

			Assert.AreEqual(FileSetFunctions.FileSetGetBaseDir(project, "sourcefiles"), "D:/packages/testpackage/dev/source");

			FileSetFunctions.FileSetUndefine(project, "sourcefiles");
			Assert.AreEqual(FileSetFunctions.FileSetExists(project, "sourcefiles"), bool.FalseString);
		}

		[TestMethod]
		public void ProjectFunctionTests()
		{
			string projectPath = Path.Combine(Environment.CurrentDirectory, "test.build");
			SeedXmlDocumentCache(projectPath, "<project name='temp_project' default='build'/>");

			Project project = new Project(Log.Silent, projectPath);
			Assert.AreEqual(ProjectFunctions.ProjectGetName(project), "temp_project");
		}

		[TestMethod]
		public void FileFunctionTests()
		{
			// set up a build file in order to set BaseDirectory, this is used by
			// PathGetFullPath function
			string projectPath = Path.Combine(Environment.CurrentDirectory, "test.build");
			SeedXmlDocumentCache(projectPath, "<project name='temp_project' default='build'/>");

			Project project = new Project(Log.Silent, projectPath);

			string testDir = TestUtil.CreateTestDirectory("file_function_tests");
			try
			{
				File.WriteAllText(Path.Combine(testDir, "one.txt"), "helloworld");

				Assert.AreEqual(FileFunctions.FileExists(project, Path.Combine(testDir, "one.txt")), bool.TrueString);
				Assert.AreEqual(FileFunctions.FileExists(project, Path.Combine(testDir, "zero.txt")), bool.FalseString);

				Assert.AreEqual(FileFunctions.FileCheckAttributes(project, Path.Combine(testDir, "one.txt"), "Archive"), bool.TrueString);
				Assert.AreEqual(FileFunctions.FileCheckAttributes(project, Path.Combine(testDir, "one.txt"), "ReadOnly"), bool.FalseString);

				Assert.AreEqual(FileFunctions.FileGetAttributes(project, Path.Combine(testDir, "one.txt")), "Archive");

				Assert.AreEqual(DirectoryFunctions.DirectoryExists(project, testDir), bool.TrueString);
				Assert.AreEqual(DirectoryFunctions.DirectoryExists(project, Path.Combine(testDir, "asdf")), bool.FalseString);

				Assert.AreEqual(DirectoryFunctions.DirectoryIsEmpty(project, testDir), bool.FalseString);

				Assert.AreEqual(DirectoryFunctions.DirectoryGetFileCount(project, testDir, "*.txt"), "1");
				Assert.AreEqual(DirectoryFunctions.DirectoryGetFileCount(project, testDir, "*.cpp"), "0");

				Assert.AreEqual(PathFunctions.PathGetFullPath(project, "."), project.BaseDirectory);

				Assert.AreEqual(PathFunctions.PathToUnix(project, "C:\\windows"), "C:/windows");
				Assert.AreEqual(PathFunctions.PathToCygwin(project, "C:\\windows"), "/cygdrive/c/windows");
				Assert.AreEqual(PathFunctions.PathToWindows(project, "/usr/linux"), "\\usr\\linux");
				Assert.AreEqual(PathFunctions.PathToNativeOS(project, "/usr/linux"), "\\usr\\linux");

				Assert.AreEqual(PathFunctions.PathGetDirectoryName(project, "packages\\build"), "packages");
				Assert.AreEqual(PathFunctions.PathGetFileName(project, "packages\\nant.exe"), "nant.exe");
				Assert.AreEqual(PathFunctions.PathGetExtension(project, "nant.exe"), ".exe");
				Assert.AreEqual(PathFunctions.PathGetFileNameWithoutExtension(project, "nant.exe"), "nant");
				Assert.AreEqual(PathFunctions.PathChangeExtension(project, "nant.exe", "cfg"), "nant.cfg");

				Assert.AreEqual(PathFunctions.PathHasExtension(project, "nant.exe"), Boolean.TrueString);
				Assert.AreEqual(PathFunctions.PathHasExtension(project, "nant"), Boolean.FalseString);

				Assert.AreEqual(PathFunctions.PathIsPathRooted(project, "D:\\nant.exe"), Boolean.TrueString);
				Assert.AreEqual(PathFunctions.PathIsPathRooted(project, "nant.exe"), Boolean.FalseString);

				Assert.AreEqual(PathFunctions.PathCombine(project, "packages", "build"), "packages\\build");

				PathFunctions.PathGetTempFileName(project);
				PathFunctions.PathGetTempPath(project);
				PathFunctions.PathGetRandomFileName(project);
			}
			finally
			{
				PathUtil.DeleteDirectory(testDir);
			}
		}

		[TestMethod]
		public void RegistryFunctionTests()
		{
			bool isElevated = false;
			try
			{
				using (WindowsIdentity identity = WindowsIdentity.GetCurrent())
				{
					WindowsPrincipal principal = new WindowsPrincipal(identity);
					isElevated = principal.IsInRole(WindowsBuiltInRole.Administrator);
				}
			}
			catch { }
			if (!isElevated) Assert.Inconclusive("This test can only be run in administrator mode because it needs to make changes to the registry.");

			Project project =  new Project(Log.Silent);
			RegistrySetValueTask.Execute(project, "LocalMachine", "SOFTWARE\\Microsoft", "FWUnitTest", value: "Original");

			Assert.AreEqual("True",	RegistryFunctions.RegistryKeyExists(project, Microsoft.Win32.RegistryHive.LocalMachine, "SOFTWARE\\Microsoft"));
			Assert.AreEqual("True", RegistryFunctions.RegistryValueExists(project, Microsoft.Win32.RegistryHive.LocalMachine, "SOFTWARE\\Microsoft", "FWUnitTest"));
			Assert.AreEqual("Original", RegistryFunctions.RegistryGetValue(project, Microsoft.Win32.RegistryHive.LocalMachine, "SOFTWARE\\Microsoft", "FWUnitTest"));

			RegistrySetValueTask.Execute(project, "LocalMachine", "SOFTWARE\\Microsoft", "FWUnitTest", value: "Updated");

			Assert.AreEqual("True", RegistryFunctions.RegistryKeyExists(project, Microsoft.Win32.RegistryHive.LocalMachine, "SOFTWARE\\Microsoft"));
			Assert.AreEqual("True", RegistryFunctions.RegistryValueExists(project, Microsoft.Win32.RegistryHive.LocalMachine, "SOFTWARE\\Microsoft", "FWUnitTest"));
			Assert.AreEqual("Updated", RegistryFunctions.RegistryGetValue(project, Microsoft.Win32.RegistryHive.LocalMachine, "SOFTWARE\\Microsoft", "FWUnitTest"));

		}

		[TestMethod]
		public void DirectoryMoveTests()
		{
			string testDir = null;
			try
			{
				testDir = TestUtil.CreateTestDirectory("DirectoryMoveTests");

				Project project = new Project(Log.Silent);

				// setup test environment
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder1"));
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder2"));
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder1", "SubFolder1"));
				File.WriteAllText(Path.Combine(testDir, "TestFolder1", "SubFolder1", "file1.txt"), "Test File 1");
				File.WriteAllText(Path.Combine(testDir, "TestFolder1", "SubFolder1", "file2.txt"), "Test File 2");
				File.WriteAllText(Path.Combine(testDir, "TestFolder1", "SubFolder1", "file3.txt"), "Test File 3");

				// do a simple directory move
				DirectoryFunctions.DirectoryMove(project, Path.Combine(testDir, "TestFolder1", "SubFolder1"), Path.Combine(testDir, "TestFolder2", "New_Folder1"));
				Assert.IsTrue(Directory.Exists(Path.Combine(testDir, "TestFolder2", "New_Folder1")));
				Assert.IsFalse(Directory.Exists(Path.Combine(testDir, "TestFolder1", "SubFolder1")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder2", "New_Folder1", "file1.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder2", "New_Folder1", "file2.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder2", "New_Folder1", "file3.txt")));

				// now test my moving sub directories as well
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder3", "SubFolder"));
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder2", "New_Folder1", "New_SubFolder1"));
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder2", "New_Folder1", "New_SubFolder2"));
				File.WriteAllText(Path.Combine(testDir, "TestFolder2", "New_Folder1", "New_SubFolder1", "file4.txt"), "Test File 4");
				File.WriteAllText(Path.Combine(testDir, "TestFolder2", "New_Folder1", "New_SubFolder1", "file5.txt"), "Test File 5");
				File.WriteAllText(Path.Combine(testDir, "TestFolder2", "New_Folder1", "New_SubFolder1", "file6.txt"), "Test File 6");
				File.WriteAllText(Path.Combine(testDir, "TestFolder2", "New_Folder1", "New_SubFolder2", "file7.txt"), "Test File 7");
				File.WriteAllText(Path.Combine(testDir, "TestFolder2", "New_Folder1", "New_SubFolder2", "file8.txt"), "Test File 8");
				File.WriteAllText(Path.Combine(testDir, "TestFolder2", "New_Folder1", "New_SubFolder2", "file9.txt"), "Test File 9");
				DirectoryFunctions.DirectoryMove(project, Path.Combine(testDir, "TestFolder2", "New_Folder1"), Path.Combine(testDir, "TestFolder3", "SubFolder", "Another"));
				Assert.IsTrue(Directory.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another")));
				Assert.IsFalse(Directory.Exists(Path.Combine(testDir, "TestFolder2", "New_Folder1")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "file1.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "file2.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "file3.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "New_SubFolder1", "file4.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "New_SubFolder1", "file5.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "New_SubFolder1", "file6.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "New_SubFolder2", "file7.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "New_SubFolder2", "file8.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder3", "SubFolder", "Another", "New_SubFolder2", "file9.txt")));

				// Now try moving to a folder that already exists
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder4", "ExistingFolder"));
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder5", "SubFolder", "Another"));
				File.WriteAllText(Path.Combine(testDir, "TestFolder5", "SubFolder", "file1.txt"), "Test File 3");
				File.WriteAllText(Path.Combine(testDir, "TestFolder5", "SubFolder", "file2.txt"), "Test File 3");
				File.WriteAllText(Path.Combine(testDir, "TestFolder5", "SubFolder", "Another", "file3.txt"), "Test File 3");
				Assert.ThrowsException<BuildException>(() => DirectoryFunctions.DirectoryMove(project, Path.Combine(testDir, "TestFolder5", "SubFolder"), Path.Combine(testDir, "TestFolder4", "ExistingFolder")));
				Assert.IsFalse(Directory.Exists(Path.Combine(testDir, "TestFolder4", "ExistingFolder", "SubFolder")));

				// Now try moving to a folder that already exists
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder6", "SubFolder", "Another"));
				Directory.CreateDirectory(Path.Combine(testDir, "TestFolder7"));
				File.WriteAllText(Path.Combine(testDir, "TestFolder6", "SubFolder", "file1.txt"), "Test File 3");
				File.WriteAllText(Path.Combine(testDir, "TestFolder6", "SubFolder", "file2.txt"), "Test File 3");
				File.WriteAllText(Path.Combine(testDir, "TestFolder6", "SubFolder", "Another", "file3.txt"), "Test File 3");
				foreach (string file in Directory.GetFiles(Path.Combine(testDir, "TestFolder6", "SubFolder", "Another"), "*.*", SearchOption.AllDirectories))
				{
					File.SetAttributes(file, FileAttributes.ReadOnly);
				}
				DirectoryFunctions.DirectoryMove(project, Path.Combine(testDir, "TestFolder6", "SubFolder"), Path.Combine(testDir, "TestFolder7", "ReadOnlyFolder"));
				Assert.IsFalse(Directory.Exists(Path.Combine(testDir, "TestFolder6", "SubFolder")));
				Assert.IsTrue(Directory.Exists(Path.Combine(testDir, "TestFolder7", "ReadOnlyFolder")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder7", "ReadOnlyFolder", "file1.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder7", "ReadOnlyFolder", "file2.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "TestFolder7", "ReadOnlyFolder", "Another", "file3.txt")));
			}
			finally
			{
				if (testDir != null && Directory.Exists(testDir))
				{
					foreach (string file in Directory.GetFiles(testDir, "*.*", SearchOption.AllDirectories))
					{
						File.SetAttributes(file, FileAttributes.Normal);
					}
					Directory.Delete(testDir, true);
				}
			}
		}
	}
}