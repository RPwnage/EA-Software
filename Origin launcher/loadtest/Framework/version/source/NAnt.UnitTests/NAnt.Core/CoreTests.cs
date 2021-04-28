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

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Tests.Util;

namespace NAnt.Core.Tests
{
	// Tests for misc parts of NAnt.Core functionality to boost code coverage
	[TestClass]
	public class CoreTests
	{
		[TestMethod]
		public void LocationTests()
		{
			Location location = new Location("myfile.txt", 5, 25);
			Assert.AreEqual(location.ToString(), "myfile.txt(5,25):");

			Location locationDefault = new Location("myfile.txt");
			Assert.AreEqual(locationDefault.ToString(), "myfile.txt:");
		}

		[TestMethod]
		public void PropertyKeyTests()
		{
			Assert.AreEqual(PropertyKey.Create("a").AsString(), "a");
			Assert.AreEqual(PropertyKey.Create("a", ".b").AsString(), "a.b");
			Assert.AreEqual(PropertyKey.Create("a", ".b", ".c").AsString(), "a.b.c");
			Assert.AreEqual(PropertyKey.Create("a", ".b", ".c", ".d").AsString(), "a.b.c.d");
			Assert.AreEqual(PropertyKey.Create("a", ".b", ".c", ".d", ".e").AsString(), "a.b.c.d.e");
			Assert.AreEqual(PropertyKey.Create("a", ".b", ".c", ".d", ".e", ".f").AsString(), "a.b.c.d.e.f");

			PropertyKey.CaseSensitivity = PropertyKey.CaseSensitivityLevel.Insensitive;
			Assert.AreNotEqual(PropertyKey.Create("UPPERCASEKEY").AsString(), "UPPERCASEKEY");
			Assert.AreEqual(PropertyKey.Create("UPPERCASEKEY").AsString(), "uppercasekey");

			PropertyKey.CaseSensitivity = PropertyKey.CaseSensitivityLevel.Sensitive;
			Assert.AreEqual(PropertyKey.Create("UPPERCASEKEY").AsString(), "UPPERCASEKEY");
			Assert.AreNotEqual(PropertyKey.Create("UPPERCASEKEY").AsString(), "uppercasekey");
		}

		[TestMethod]
		public void PropertyTests()
		{
			Project project = new Project(Log.Silent);
			{
				Assert.AreEqual(project.Properties.GetPropertyOrDefault("not-a-property", "default"), "default");
				Assert.AreEqual(project.GetPropertyOrDefault("not-a-property", "default"), "default");
				Assert.AreEqual(project.Properties.GetBooleanPropertyOrDefault("not-a-property", false), false);
				Assert.AreEqual(project.Properties.GetBooleanPropertyOrDefault("not-a-property", true), true);

				project.Properties["is-true"] = "true";
				project.Properties["is-false"] = "false";
				project.Properties["is-asdf"] = "asdf";

				Assert.AreEqual(project.Properties.GetBooleanProperty("is-true"), true);
				Assert.AreEqual(project.Properties.GetBooleanPropertyOrDefault("is-true", false), true);
				Assert.AreEqual(project.Properties.GetBooleanProperty("is-false"), false);
				Assert.AreEqual(project.Properties.GetBooleanPropertyOrDefault("is-false", true), false);
				Assert.AreEqual(project.Properties.GetPropertyOrDefault("is-asdf", "zxcv"), "asdf");

				Assert.AreEqual(project.Properties.GetPropertyOrFail("is-asdf"), "asdf");
				Assert.AreEqual(project.GetPropertyOrFail("is-asdf"), "asdf");
				Assert.ThrowsException<BuildException>(() => project.Properties.GetPropertyOrFail("not-a-property"));
				Assert.ThrowsException<BuildException>(() => project.GetPropertyOrFail("not-a-property"));
			}
		}

		[TestMethod]
		public void StringTests()
		{
			Assert.AreEqual("".IsOptionBoolean(), false);
			Assert.AreEqual("asdf".IsOptionBoolean(), false);
			Assert.AreEqual("on".IsOptionBoolean(), true);
			Assert.AreEqual("off".IsOptionBoolean(), true);
			Assert.AreEqual("true".IsOptionBoolean(), true);
			Assert.AreEqual("false".IsOptionBoolean(), true);
			Assert.AreEqual("   true  ".IsOptionBoolean(), true);

			Assert.AreEqual("".OptionToBoolean(), false);
			Assert.AreEqual("asdf".OptionToBoolean(), false);
			Assert.AreEqual("on".OptionToBoolean(), true);
			Assert.AreEqual("off".OptionToBoolean(), false);
			Assert.AreEqual("true".OptionToBoolean(), true);
			Assert.AreEqual("false".OptionToBoolean(), false);
			Assert.AreEqual("   true  ".OptionToBoolean(), true);

			Assert.AreEqual("".ToBoolean(), false);
			Assert.AreEqual("asdf".ToBoolean(), false);
			Assert.AreEqual("on".ToBoolean(), false);
			Assert.AreEqual("off".ToBoolean(), false);
			Assert.AreEqual("true".ToBoolean(), true);
			Assert.AreEqual("false".ToBoolean(), false);
			Assert.AreEqual("   true  ".ToBoolean(), true);

			Assert.AreEqual("".ToBooleanOrDefault(false), false);
			Assert.AreEqual("  false  ".ToBooleanOrDefault(true), false);
			Assert.AreEqual("  true  ".ToBooleanOrDefault(false), true);

			Assert.AreEqual("asdf".Quote(), "\"asdf\"");
			Assert.AreEqual("\"asdf\"".TrimQuotes(), "asdf");

			Assert.AreEqual("file.txt".TrimExtension(), "file");
			Assert.AreEqual("".TrimExtension(), "");
			Assert.AreEqual("file".TrimExtension(), "file");

			Assert.AreEqual("one two three four".ToArray().ToString(","), new List<string>() { "one", "two", "three", "four" }.ToString(","));
			Assert.AreEqual("one|two|three|four".ToArray("|").ToString(","), new List<string>() { "one", "two", "three", "four" }.ToString(","));
			Assert.AreEqual("one\ntwo\nthree\nfour".LinesToArray().ToString(","), new List<string>() { "one", "two", "three", "four" }.ToString(","));

			Assert.AreEqual("C:\\asdf\\zxcv".PathToUnix(), "C:/asdf/zxcv");
			Assert.AreEqual("C:/asdf/zxcv".PathToWindows(), "C:\\asdf\\zxcv");
		}

		[TestMethod]
		public void StringMinimumEditDistanceTest()
		{
			Assert.AreEqual(0, StringUtil.MinimumEditDistance(null, null));
			Assert.AreEqual(0, StringUtil.MinimumEditDistance("", ""));
			Assert.AreEqual(0, StringUtil.MinimumEditDistance(null, ""));
			Assert.AreEqual(0, StringUtil.MinimumEditDistance("", null));

			Assert.AreEqual(4, StringUtil.MinimumEditDistance("asdf", ""));
			Assert.AreEqual(4, StringUtil.MinimumEditDistance("asdf", null));
			Assert.AreEqual(4, StringUtil.MinimumEditDistance("", "asdf"));
			Assert.AreEqual(4, StringUtil.MinimumEditDistance(null, "asdf"));

			Assert.AreEqual(0, StringUtil.MinimumEditDistance("asdf", "asdf"));
			Assert.AreEqual(4, StringUtil.MinimumEditDistance("asdf", "zxcv"));
			Assert.AreEqual(8, StringUtil.MinimumEditDistance("asdf", "zxcv", 2));

			Assert.AreEqual(5, StringUtil.MinimumEditDistance("intention", "execution"));
			Assert.AreEqual(8, StringUtil.MinimumEditDistance("intention", "execution", 2));
		}

		[TestMethod]
		public void OptionSetTest()
		{
			OptionSet optionset = new OptionSet();
			optionset.Options["is-asdf"] = "asdf";
			optionset.Options["is-true"] = "true";
			optionset.Options["is-false"] = "false";

			Assert.AreEqual(optionset.GetBooleanOption("is-true"), true);
			Assert.AreEqual(optionset.GetBooleanOption("is-false"), false);
			Assert.AreEqual(optionset.GetBooleanOption("is-asdf"), false);
			Assert.AreEqual(optionset.GetBooleanOption("not-an-option"), false);

			Assert.AreEqual(optionset.GetBooleanOptionOrDefault("is-true", false), true);
			Assert.AreEqual(optionset.GetBooleanOptionOrDefault("is-false", true), false);
			Assert.AreEqual(optionset.GetBooleanOptionOrDefault("is-asdf", true), false);
			Assert.AreEqual(optionset.GetBooleanOptionOrDefault("not-an-option", true), true);

			Assert.AreEqual(optionset.GetOptionOrDefault("is-asdf", "default"), "asdf");
			Assert.AreEqual(optionset.GetOptionOrDefault("not-an-option", "default"), "default");

			OptionSet clone = new OptionSet(optionset, true);
			Assert.AreEqual(clone.ContentEquals(optionset), true);

			OptionSet set1 = new OptionSet();
			set1.Options["op1"] = "asdf";
			OptionSet set2 = new OptionSet();
			set2.Options["op2"] = "zxcv";
			set1.Append(set2);
			Assert.AreEqual(set1.Options.Count, 2);
			Assert.AreEqual(set2.Options.Count, 1);
			Assert.AreEqual(set1.Options.ContainsKey("op1"), true);
			Assert.AreEqual(set1.Options.ContainsKey("op2"), true);
			Assert.AreEqual(set2.Options.ContainsKey("op2"), true);
		}

		[TestMethod]
		public void OptionSetTaskTest()
		{
			System.Xml.XmlDocument xmlDoc = new System.Xml.XmlDocument();
			System.Xml.XmlElement optionsetNode = xmlDoc.CreateElement("optionset");
			optionsetNode.SetAttribute("name", "TestOptionset");
			System.Xml.XmlNode goodOptionTest = optionsetNode.GetOrAddElement("option");
			goodOptionTest.SetAttribute("name", "goodOptionTest");
			goodOptionTest.InnerXml =
				"test1" + Environment.NewLine +
				"<do if=\"${test-config}==debug\">" + Environment.NewLine +
				"	<!-- Dummy Debug Comment -->" + Environment.NewLine +
				"	debug1" + Environment.NewLine +
				"	debug2" + Environment.NewLine +
				"</do>" + Environment.NewLine +
				"<do if=\"${test-config}==release\">" + Environment.NewLine +
				"	<!-- Dummy Release Comment -->" + Environment.NewLine +
				"	release1" + Environment.NewLine +
				"	release2" + Environment.NewLine +
				"</do>" + Environment.NewLine +
				"test2";

			Project testProject = new Project(Log.Silent);
			testProject.Properties["test-config"] = "debug";

			Tasks.OptionSetTask optsetTask = new Tasks.OptionSetTask();
			optsetTask.Project = testProject;
			optsetTask.OptionSetName = "OptionsetTest";
			optsetTask.Initialize(optionsetNode);
			optsetTask.Execute();

			string evaluatedOptionValue = Functions.OptionSetFunctions.OptionSetGetValue(testProject, "TestOptionset", "goodOptionTest");
			Assert.IsTrue(evaluatedOptionValue.Contains("test1"));
			Assert.IsTrue(evaluatedOptionValue.Contains("debug1"));
			Assert.IsTrue(evaluatedOptionValue.Contains("debug2"));
			Assert.IsTrue(evaluatedOptionValue.Contains("test2"));
			Assert.IsFalse(evaluatedOptionValue.Contains("release1"));
			Assert.IsFalse(evaluatedOptionValue.Contains("release2"));

			testProject.Properties["test-config"] = "release";
			optsetTask.Initialize(optionsetNode);
			optsetTask.Execute();
			evaluatedOptionValue = Functions.OptionSetFunctions.OptionSetGetValue(testProject, "TestOptionset", "goodOptionTest");
			Assert.IsTrue(evaluatedOptionValue.Contains("test1"));
			Assert.IsTrue(evaluatedOptionValue.Contains("release1"));
			Assert.IsTrue(evaluatedOptionValue.Contains("release2"));
			Assert.IsTrue(evaluatedOptionValue.Contains("test2"));
			Assert.IsFalse(evaluatedOptionValue.Contains("debug1"));
			Assert.IsFalse(evaluatedOptionValue.Contains("debug2"));

			System.Xml.XmlNode badOptionTest = optionsetNode.GetOrAddElement("option");
			badOptionTest.SetAttribute("name", "badOptionTest");
			badOptionTest.InnerXml =
				"test1" + Environment.NewLine +
				"<do if=\"${test-config}==debug\">" + Environment.NewLine +
				"	debug" + Environment.NewLine +
				"	<echo message=\"Should get build exception on this line unless didn't pass 'do' block!\"/>" + Environment.NewLine +
				"</do>" + Environment.NewLine +
				"<do if=\"${test-config}==release\">" + Environment.NewLine +
				"	release" + Environment.NewLine +
				"	<echo message=\"Should get build exception on this line unless didn't pass 'do' block!\"/>" + Environment.NewLine +
				"</do>" + Environment.NewLine +
				"test2";

			Exception receivedException = null;
			try
			{
				optsetTask.Initialize(optionsetNode);
				optsetTask.Execute();
			}
			catch (Exception ex)
			{
				receivedException = ex;
			}
			Assert.IsTrue(receivedException != null);

			badOptionTest.InnerXml =
				"test1" + Environment.NewLine +
				"<echo message=\"Should get build exception on this line unless didn't pass 'do' block!\"/>" + Environment.NewLine +
				"test2";

			receivedException = null;
			try
			{
				optsetTask.Initialize(optionsetNode);
				optsetTask.Execute();
			}
			catch (Exception ex)
			{
				receivedException = ex;
			}
			Assert.IsTrue(receivedException != null);
		}

		[TestMethod]
		public void FileSetTest()
		{
			Project project = new Project(Log.Silent);
			{
				string testDir = TestUtil.CreateTestDirectory("filesettest");
				File.WriteAllText(Path.Combine(testDir, "file1.cpp"), String.Empty);
				File.WriteAllText(Path.Combine(testDir, "file2.cpp"), String.Empty);
				Directory.CreateDirectory(Path.Combine(testDir, "dir1"));
				File.WriteAllText(Path.Combine(testDir, "dir1", "file3.cs"), String.Empty);
				Directory.CreateDirectory(Path.Combine(testDir, "dir2", "dir3"));
				File.WriteAllText(Path.Combine(testDir, "dir2", "dir3", "file4.cs"), String.Empty);
				File.WriteAllText(Path.Combine(testDir, "file5.cs"), String.Empty);
				File.WriteAllText(Path.Combine(testDir, "file1_xb1.cpp"), String.Empty);

				FileSet fileset = new FileSet(project);
				fileset.BaseDirectory = testDir;
				fileset.Include("*.cpp");
				fileset.Include("**.cs");
				fileset.Exclude("*_xb1.cpp");

				Assert.AreEqual(fileset.FileItems.Count, 5);
				Assert.AreEqual(fileset.FileItems[0].FileName, Path.Combine(testDir, "file1.cpp"));
				Assert.AreEqual(fileset.FileItems[1].FileName, Path.Combine(testDir, "file2.cpp"));
				Assert.AreEqual(fileset.FileItems[2].FileName, Path.Combine(testDir, "dir1", "file3.cs"));
				Assert.AreEqual(fileset.FileItems[3].FileName, Path.Combine(testDir, "dir2", "dir3", "file4.cs"));
				Assert.AreEqual(fileset.FileItems[4].FileName, Path.Combine(testDir, "file5.cs"));
			}
		}

		[TestMethod]
		public void ChronoTest()
		{
			Chrono timer = new Chrono();
			TimeSpan time = timer.GetElapsed();
			Assert.IsNotNull(time);
			Assert.IsTrue(timer.ToString().Contains("millisec"));
		}

		private bool CompareFastAndSlow(string path)
		{
			bool useFast = !PlatformUtil.IsMonoRuntime;
			return PathNormalizer.Normalize(path, true, useFast) == PathNormalizer.Normalize(path, true, false);
		}

		[TestMethod]
		public void PathNormalizerTests()
		{
			Assert.IsTrue(CompareFastAndSlow(@"d:\hello"));
			Assert.IsTrue(CompareFastAndSlow(@"d:/hello"));
			Assert.IsTrue(CompareFastAndSlow(@"d:/hello\.\you"));
			Assert.IsTrue(CompareFastAndSlow(@"d:/hello\..\you"));
			Assert.IsTrue(CompareFastAndSlow(@"d:/hello\../.\you"));
			Assert.IsTrue(CompareFastAndSlow(@"d:/hello\../.\.you"));
			Assert.IsTrue(CompareFastAndSlow(@"\hello"));
			Assert.IsTrue(CompareFastAndSlow(@"\\server\share"));
			Assert.IsTrue(CompareFastAndSlow(@"hello"));
			Assert.IsTrue(CompareFastAndSlow(@".hello"));
			Assert.IsTrue(CompareFastAndSlow(@"D:/hello/hello2/../.."));
		}

		[TestMethod]
		public void PathUtilRelativePathTests()
		{
			Assert.AreEqual("", PathUtil.RelativePath(new PathString("D:\\one\\two"), new PathString("D:\\one\\two")));
			Assert.AreEqual("two", PathUtil.RelativePath(new PathString("D:\\one\\two"), new PathString("D:\\one")));
			Assert.AreEqual("two\\", PathUtil.RelativePath(new PathString("D:\\one\\two\\"), new PathString("D:\\one\\")));
			Assert.AreEqual(".\\two\\", PathUtil.RelativePath(new PathString("D:\\one\\two\\"), new PathString("D:\\one\\"), addDot: true));
			Assert.AreEqual("..\\..\\one", PathUtil.RelativePath(new PathString("D:\\one"), new PathString("D:\\one\\two")));
			Assert.AreEqual("..", PathUtil.RelativePath(new PathString("D:\\one\\"), new PathString("D:\\one\\two\\")));
			Assert.AreEqual("..\\..", PathUtil.RelativePath(new PathString("D:\\one\\"), new PathString("D:\\one\\two\\three\\")));
			Assert.AreEqual("..\\..\\four\\five", PathUtil.RelativePath(new PathString("D:\\one\\four\\five\\"), new PathString("D:\\one\\two\\three\\")));

			// Testing the behavior when being run on Unix Style paths
			// note: under mono it will automatically flip the slashes the opposite way
			Assert.AreEqual("..\\..\\four\\five", PathUtil.RelativePath(new PathString("D:/one/four/five"), new PathString("D:/one/two/three")));
			Assert.AreEqual("..\\..\\four\\five", PathUtil.RelativePath(new PathString("D:/one/four/five/"), new PathString("D:/one/two/three/")));
			Assert.AreEqual("../../four/five", PathUtil.RelativePath(new PathString("D:/one/four/five"), new PathString("D:/one/two/three"), separatorOptions: DirectorySeparatorOptions.Preserve));
			Assert.AreEqual("../../four/five", PathUtil.RelativePath(new PathString("D:/one/four/five/"), new PathString("D:/one/two/three/"), separatorOptions: DirectorySeparatorOptions.Preserve));
		}
	}
}