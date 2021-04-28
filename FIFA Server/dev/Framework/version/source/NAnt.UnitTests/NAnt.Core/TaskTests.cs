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
using System.Xml;
using EA.FrameworkTasks.Model;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Tests.Util;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class TaskTests
	{
		[TestMethod]
		public void PropertyTaskTest()
		{
			Project project = new Project(Log.Silent);
			{
				string testDir = TestUtil.CreateTestDirectory("propertytest");

				// Using the property task
				new PropertyTask(project, name:"my-prop", value: "my-value").Execute();
				Assert.AreEqual(project.Properties["my-prop"], "my-value");

				// Reading in a property from a file
				string prop1_file = Path.Combine(testDir, "prop1.txt");
				File.WriteAllText(prop1_file, "value from file");
				new PropertyTask(project, name: "file-prop", fromFile: prop1_file).Execute();
				Assert.AreEqual(project.Properties["file-prop"], "value from file");
			}
		}

		[TestMethod]
		public void ScriptTaskTest()
		{
			string testDir = TestUtil.CreateTestDirectory("ScriptTaskTest");

			string masterconfigpath = Environment.GetEnvironmentVariable("masterconfig_path");

			if (masterconfigpath.IsNullOrEmpty() || !File.Exists(masterconfigpath))
			{
				masterconfigpath = string.Join(testDir, "masterconfig.xml");
				File.WriteAllText(masterconfigpath,
					string.Join(
						Environment.NewLine,
						"<project>",
						"  <masterversions>",
						"    <package name=\"MSBuildTools\" version=\"dev16\" uri=\"p4://dice-p4-one.dice.ad.ea.com:2001/SDK/MSBuildTools/dev16\" />",
						"  </masterversions>",
						"  <ondemand>true</ondemand>",
						"  <ondemandroot>ondemand</ondemandroot>",
						"  <config default=\"pc-vc-dev\" includes=\"pc-vc-debug\"/>",
						"</project>"
						));
			}
			
			Project project = new Project(Log.Silent);
			MasterConfig masterconfigFile = MasterConfig.Load(project, masterconfigpath);

			// Use msbuildtools, specifically for when this runs on the buildfarm where visual studio isn't installed
			project.Properties["package.VisualStudio.use-non-proxy-build-tools"] = "true";

			PackageMap.Init(project, masterconfigFile, buildRootOverride: TestUtil.CreateTestDirectory("scripttest"));
			{
				string code = @"public static void ScriptMain(Project project) {
	DataTable table = new DataTable(""TestTable"");

	DataColumn column = new DataColumn();
	column.DataType = System.Type.GetType(""System.Int32"");
	column.ColumnName = ""ID"";
	column.ReadOnly = true;
	column.Unique = true;
	table.Columns.Add(column);
	column = new DataColumn();
	column.DataType = System.Type.GetType(""System.String"");
	column.ColumnName = ""Name"";
	column.AutoIncrement = false;
	column.Caption = ""Caption Name"";
	column.ReadOnly = false;
	column.Unique = false;
	table.Columns.Add(column);

	DataColumn[] PrimaryKeyColumns = new DataColumn[1];
	PrimaryKeyColumns[0] = table.Columns[""ID""];
	table.PrimaryKey = PrimaryKeyColumns;

	System.Random rand = new System.Random(System.DateTime.Now.Millisecond);
	int numRows = rand.Next(10) + 1;
	for (int i = 0; i < numRows; ++i)
	{
		DataRow row = table.NewRow();
		row[""ID""] = i;
		row[""Name""] = ""Test Name "" + i;
		table.Rows.Add(row);
	}

	Console.WriteLine(""\t\tCreated table with {0} columns and {1} rows."", table.Columns.Count, table.Rows.Count);
}";

				List<string> imports = new List<string>();
				imports.Add("System");
				imports.Add("System.Net.Mail");
				imports.Add("System.Data");
				imports.Add("System.Data.OleDb");
				imports.Add("System.Collections");
				imports.Add("System.IO");

				FileSet references = new FileSet();
				references.Include("System.Data.dll", asIs: true);

				ScriptTask task = new ScriptTask(project, code: code, imports: imports, references: references);
				task.Execute();
			}
		}

		[TestMethod]
		public void MailTaskTest()
		{
			Project project = new Project(Log.Silent);
			{
				string testDir = TestUtil.CreateTestDirectory("mailtest");
				File.WriteAllText(Path.Combine(testDir, "test1.txt"), "test attachment 1");
				File.WriteAllText(Path.Combine(testDir, "test2.txt"), "test attachment 2");
				File.WriteAllText(Path.Combine(testDir, "test3.txt"), "test attachment 3");
				File.WriteAllText(Path.Combine(testDir, "test4.txt"), "test attachment 4");

				string from = "nonexistentguy0@ea.com";
				string toList = "nonexistentguy1@ea.com; nonexistentguy2@ea.com, nonexistentguy3@ea.com";
				string ccList = "nonexistentguy4@ea.com; nonexistentguy5@ea.com, nonexistentguy6@ea.com";
				string bccList = "nonexistentguy7@ea.com, nonexistentguy8@ea.com; nonexistentguy9@ea.com";
				string subject = "Framework sendmail test";
				string mailhost = "mailhost.ea.com";

				string[] mailFormats = { "Html", "Text" };
				foreach (string mailformat in mailFormats)
				{
					string br = "";
					if (mailformat == "Html") br = "<br>";

					string files = Path.Combine(testDir, "test1.txt") + ";" + Path.Combine(testDir, "test2.txt");
					string attachments = Path.Combine(testDir, "test.txt") + ";" + Path.Combine(testDir, "test2.txt");

					string message1 = string.Format(@"
TO:{0}{4}
CC:{1}{4}
BCC:{2}{4}
Testing all required fields.{4}
Please let me know when you get this :){4}
This email should be in {3}
format.{4}
You'll get one in HTML and one in TEXT format.{4}
Message files: {5}{4}
Attachments: {6}{4}
Thanks{4}
{4}", toList, ccList, bccList, mailformat, br, files, attachments);

					string message2 = string.Format(@"
TO:{0}{2}
CC:{2}
BCC:{2}
Test skipping not required fields.{2}
Please let me know when you get this :){2}
This email should be in {1}
format.{2}
You'll get one in HTML and one in TEXT format.{2}
Thanks{2}
{2}", toList, mailformat, br);

					MailTask mailTask1 = new MailTask(project, from: from, format: mailformat,
						toList: toList, ccList: ccList, bccList: bccList, subject: subject, message: message1,
						mailhost: mailhost, files: files, attachments: attachments, async: true);
					mailTask1.Execute();

					MailTask mailTask2 = new MailTask(project, from: from, format: mailformat,
						toList: toList, message: message2, mailhost: mailhost, async: true);
					mailTask2.Execute();

					MailTask mailTask3 = new MailTask(project, from: from, format: mailformat,
						toList: toList, subject: "Test skipping CC and BCC (DT 26607)", message: message2,
						mailhost: mailhost, files: files, attachments: attachments, async: true);
					mailTask3.Execute();
				}
			}
		}

		[TestMethod]
		public void RegexMatchTaskTest()
		{
			Project project = new Project(Log.Silent);
			{
				string testString = "Test string input abc-def-ghi 00.476.57 1.03.65 Clang version 3.4 More test string";

				RegexMatchTask.Execute(project, testString, "Clang version ([0-9])\\.([0-9])", "Result1;Result2;Result3;Result4");
				Assert.AreEqual(project.Properties["Result1"], "Clang version 3.4");
				Assert.AreEqual(project.Properties["Result2"], "3");
				Assert.AreEqual(project.Properties["Result3"], "4");
				Assert.AreEqual(project.Properties["Result4"], "");

				RegexMatchTask.Execute(project, testString, "([a-z]+)-(.+)-([a-z])", "Result1;Result2;Result3");
				Assert.AreEqual(project.Properties["Result1"], "abc-def-g");
				Assert.AreEqual(project.Properties["Result2"], "abc");
				Assert.AreEqual(project.Properties["Result3"], "def");

				RegexMatchTask.Execute(project, testString, "(\\d)\\.(\\d{2})\\.(\\d{2})", "Result1");
				Assert.AreEqual(project.Properties["Result1"], "1.03.65");

				RegexMatchTask.Execute(project, testString, "([0-9]+)\\.([0-9]+)\\.([0-9]+)", "Result1");
				Assert.AreEqual(project.Properties["Result1"], "00.476.57");

				testString = "Ubuntu clang version 3.4-1ubuntu3 (tags/RELEASE_34/final) (based on LLVM 3.4)\n" +
					"Target: x86_64-pc-linux-gnu\n" +
					"Thread model: posix";

				RegexMatchTask.Execute(project, testString, "clang version (\\d)\\.(\\d)", "ClangVersionString;ClangMajorVer;ClangMinorVer");
				Assert.AreEqual(project.Properties["ClangVersionString"], "clang version 3.4");
				Assert.AreEqual(project.Properties["ClangMajorVer"], "3");
				Assert.AreEqual(project.Properties["ClangMinorVer"], "4");

				RegexMatchTask.Execute(project, testString, "Expecting no match (\\d)\\.(\\d)\\.(\\d)", "MatchResult");
				Assert.AreEqual(project.Properties["MatchResult"], "");
			}
		}

		[TestMethod]
		public void CreateTaskBasicTest()
		{
			Project project = new Project(Log.Silent);
			{
				CreateTaskTask createTask = new CreateTaskTask();
				createTask.Project = project;
				createTask.TaskName = "BasicTask";

				LineInfoDocument doc = new LineInfoDocument();
				doc.LoadXml("<createtask name='BasicTask'>" +
					"<code>" +
						"<property name='test.result' value='asdf'/>" +
					"</code>" +
				"</createtask>");
				XmlNode taskNode = doc.GetChildElementByName("createtask");
				XmlNode codeNode = taskNode.GetChildElementByName("code");

				project.UserTasks["BasicTask"] = codeNode;

				createTask.Execute();
				Assert.IsTrue(project.Properties.Contains("Task.BasicTask"));

				RunTask runTask = new RunTask();
				runTask.Project = project;
				runTask.TaskName = "BasicTask";
				runTask.Initialize(taskNode);
				Assert.IsFalse(project.Properties.Contains("test.result"));
				runTask.Execute();
				Assert.IsTrue(project.Properties["test.result"] == "asdf");
			}
		}

		[TestMethod]
		public void CreateTaskParameterTest()
		{
			Project project = new Project(Log.Silent);
			{
				CreateTaskTask createTask = new CreateTaskTask();
				createTask.Project = project;
				createTask.TaskName = "ParameterTask";

				LineInfoDocument doc = new LineInfoDocument();
				doc.LoadXml("<createtask name='ParameterTask'>" +
					"<parameters>" +
						"<option name='SetPropertyTo'/>" +
					"</parameters>" +
					"<code>" +
						"<property name='test.result' value='${ParameterTask.SetPropertyTo}'/>" +
					"</code>" +
				"</createtask>");
				XmlNode taskNode = doc.GetChildElementByName("createtask");
				XmlNode codeNode = taskNode.GetChildElementByName("code");

				project.UserTasks["ParameterTask"] = codeNode;

				createTask.Execute();
				Assert.IsTrue(project.Properties.Contains("Task.ParameterTask"));

				RunTask runTask = new RunTask();
				runTask.Project = project;
				runTask.TaskName = "ParameterTask";
				runTask.ParameterValues.Options.Add("SetPropertyTo", "zxcv");
				runTask.Initialize(taskNode);
				Assert.IsFalse(project.Properties.Contains("test.result"));
				runTask.Execute();
				Assert.IsTrue(project.Properties["test.result"] == "zxcv");
			}
		}

		[TestMethod]
		public void CreateTaskFailTest()
		{
			Project project = new Project(Log.Silent);
			{
				CreateTaskTask createTask = new CreateTaskTask();
				createTask.Project = project;
				createTask.TaskName = "FailTask";

				LineInfoDocument doc = new LineInfoDocument();
				doc.LoadXml("<createtask name='FailTask'>" +
					"<code>" +
						"<fail message='failing from inside createtask'/>" +
					"</code>" +
				"</createtask>");
				XmlNode taskNode = doc.GetChildElementByName("createtask");
				XmlNode codeNode = taskNode.GetChildElementByName("code");

				project.UserTasks["FailTask"] = codeNode;

				createTask.Execute();
				Assert.IsTrue(project.Properties.Contains("Task.FailTask"));

				RunTask runTask = new RunTask();
				runTask.Project = project;
				runTask.TaskName = "FailTask";
				runTask.Initialize(taskNode);
				ContextCarryingException ex = Assert.ThrowsException<ContextCarryingException>(() => runTask.Execute());
				Assert.IsTrue(ex.GetBaseException().Message.Contains("failing from inside createtask"));
			}
		}

		[TestMethod]
		public void CreateTaskTryCatchTest()
		{
			Project project = new Project(Log.Silent);
			{
				CreateTaskTask createTask = new CreateTaskTask();
				createTask.Project = project;
				createTask.TaskName = "TryCatchTestTask";

				LineInfoDocument doc = new LineInfoDocument();
				doc.LoadXml("<createtask name='TryCatchTestTask'>" +
						"<parameters>" +
							"<option name='MakeFailInTry'/>" +
							"<option name='MakeFailInCatch'/>" +
							"<option name='MakeFailInFinally'/>" +
							"<option name='SecondLevelFail'/>" +
						"</parameters>" +
						"<code>" +
							"<trycatch>" +
								"<try>" +
									"<fail message='failed inside createtask try' if='${TryCatchTestTask.MakeFailInTry??false}'/>" +
								"</try>" +
								"<catch>" +
									"<task name='FailTask' if='${TryCatchTestTask.SecondLevelFail??false}'/>" +
									"<fail message='failed inside createtask catch' if='${TryCatchTestTask.MakeFailInCatch??false}'/>" +
								"</catch>" +
								"<finally>" +
									"<fail message='failed inside createtask finally' if='${TryCatchTestTask.MakeFailInFinally??false}'/>" +
								"</finally>" +
							"</trycatch>" +
						"</code>" +
					"</createtask>");
				XmlNode taskNode = doc.GetChildElementByName("createtask");
				XmlNode codeNode = taskNode.GetChildElementByName("code");

				CreateTaskTask createFailTask = new CreateTaskTask();
				createFailTask.Project = project;
				createFailTask.TaskName = "FailTask";

				LineInfoDocument doc2 = new LineInfoDocument();
				doc2.LoadXml("<createtask name='FailTask'>" +
					"<code>" +
						"<fail message='failed in second level'/>" +
					"</code>" +
				"</createtask>");
				XmlNode failTaskNode = doc2.GetChildElementByName("createtask");
				XmlNode failCodeNode = failTaskNode.GetChildElementByName("code");

				project.UserTasks["TryCatchTestTask"] = codeNode;
				project.UserTasks["FailTask"] = failCodeNode;

				createTask.Execute();
				createFailTask.Execute();
				Assert.IsTrue(project.Properties.Contains("Task.TryCatchTestTask"));
				Assert.IsTrue(project.Properties.Contains("Task.FailTask"));

				RunTask runTask = new RunTask();
				runTask.Project = project;
				runTask.TaskName = "TryCatchTestTask";
				runTask.Initialize(taskNode);
				// Test that nothing fails
				runTask.Execute();
				// Test that if it fails in the try it is caught
				runTask.ParameterValues.Options.Add("MakeFailInTry", "true");
				runTask.Execute();
				// Test that if it fails in the catch it fails
				runTask.ParameterValues.Options.Add("MakeFailInCatch", "true");
				ContextCarryingException ex1 = Assert.ThrowsException<ContextCarryingException>(() => runTask.Execute());
				Assert.IsTrue(ex1.GetBaseException().Message.Contains("failed inside createtask catch"));
				// Test that if it fails in the finally it fails
				runTask.ParameterValues.Options.Clear();
				runTask.ParameterValues.Options.Add("MakeFailInFinally", "true");
				ContextCarryingException ex2 = Assert.ThrowsException<ContextCarryingException>(() => runTask.Execute());
				Assert.IsTrue(ex2.GetBaseException().Message.Contains("failed inside createtask finally"));
				// very that the options are indeed cleared and it passes again
				runTask.ParameterValues.Options.Clear();
				runTask.Execute();
				// test that a failure of a second level task is thrown
				runTask.ParameterValues.Options.Add("MakeFailInTry", "true");
				runTask.ParameterValues.Options.Add("SecondLevelFail", "true");
				ContextCarryingException ex3 = Assert.ThrowsException<ContextCarryingException>(() => runTask.Execute());
				Assert.IsTrue(ex3.GetBaseException().Message.Contains("failed in second level"));
			}
		}

		[TestMethod]
		public void ZipTaskTest()
		{
			string testDir = TestUtil.CreateTestDirectory("ZipTaskTest");
			File.WriteAllText(Path.Combine(testDir, "test1.txt"), "test1");
			File.WriteAllText(Path.Combine(testDir, "test2.txt"), "test2");

			Project project = new Project(Log.Silent);
			{
				ZipTasks.ZipTask ziptask = new ZipTasks.ZipTask
				{
					Project = project
				};
				ziptask.ZipFileSet.BaseDirectory = testDir;
				ziptask.ZipFileSet.Include("*.txt");
				ziptask.ZipFileName = Path.Combine(testDir, "test.zip");
				ziptask.Execute();
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "test.zip")));

				ZipTasks.UnZipTask unziptask = new ZipTasks.UnZipTask();
				unziptask.Project = project;
				unziptask.ZipFileName = Path.Combine(testDir, "test.zip");
				unziptask.OutDir = Path.Combine(testDir, "unzipped");
				unziptask.Execute();
				Assert.IsTrue(Directory.Exists(Path.Combine(testDir, "unzipped")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "unzipped", "test1.txt")));
				Assert.IsTrue(File.Exists(Path.Combine(testDir, "unzipped", "test2.txt")));
			}
		}

	}
}