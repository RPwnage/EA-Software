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
using System.Reflection;
using System.Runtime.InteropServices.ComTypes;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Tests.Util;

using EA.Eaconfig;
using NAnt.Intellisense;
using EA.CPlusPlusTasks;
using EA.Eaconfig.Structured;
using NAnt.Core.PackageCore;

namespace EA.Tasks.Tests
{
	[TestClass]
	public class EATaskTests
	{
		[TestMethod]
		public void ResponseFileTest()
		{
			string testDir = TestUtil.CreateTestDirectory("ResponseFileTest");

			Project project = new Project(Log.Silent);

			project.Properties["cc"] = "cc.exe";
			project.Properties["lib"] = "lib.exe";
			project.Properties["as"] = "as.exe";
			project.Properties["link"] = "link.exe";

			OptionSet options = new OptionSet();
			OptionSetUtil.AppendOption(options, "cc", "lib.exe");
			OptionSetUtil.AppendOption(options, "cc.options", "-rs");
			OptionSetUtil.AppendOption(options, "cc.template.objectfile", "%objectfile%");
			OptionSetUtil.AppendOption(options, "cc.useresponsefile", "true");
			OptionSetUtil.AppendOption(options, "cc.template.includedir", "");
			OptionSetUtil.AppendOption(options, "cc.template.responsefile", "mycctemplate_response_file_command_line @%responsefile%");
			OptionSetUtil.AppendOption(options, "cc.template.responsefile.commandline", "mycctemplate_response_file_contents %options% %liboutputname% %objectfiles%");
			OptionSetUtil.AppendOption(options, "lib", "lib.exe");
			OptionSetUtil.AppendOption(options, "lib.options", "-rs");
			OptionSetUtil.AppendOption(options, "lib.template.objectfile", "%objectfile%");
			OptionSetUtil.AppendOption(options, "lib.useresponsefile", "true");
			OptionSetUtil.AppendOption(options, "lib.template.responsefile", "mylibtemplate_response_file_command_line @%responsefile%");
			OptionSetUtil.AppendOption(options, "lib.template.responsefile.commandline", "mylibtemplate_response_file_contents %options% %liboutputname% %objectfiles%");
			OptionSetUtil.AppendOption(options, "as", "lib.exe");
			OptionSetUtil.AppendOption(options, "as.options", "-rs");
			OptionSetUtil.AppendOption(options, "as.template.includedir", "");
			OptionSetUtil.AppendOption(options, "as.template.objectfile", "%objectfile%");
			OptionSetUtil.AppendOption(options, "as.useresponsefile", "true");
			OptionSetUtil.AppendOption(options, "as.template.responsefile", "myastemplate_response_file_command_line @%responsefile%");
			OptionSetUtil.AppendOption(options, "as.template.responsefile.commandline", "myastemplate_response_file_contents %options% %liboutputname% %objectfiles%");
			OptionSetUtil.AppendOption(options, "link", "lib.exe");
			OptionSetUtil.AppendOption(options, "link.options", "-rs");
			OptionSetUtil.AppendOption(options, "link.template.librarydir", "");
			OptionSetUtil.AppendOption(options, "link.template.libraryfile", "");
			OptionSetUtil.AppendOption(options, "link.template.objectfile", "%objectfile%");
			OptionSetUtil.AppendOption(options, "link.useresponsefile", "true");
			OptionSetUtil.AppendOption(options, "link.template.responsefile", "mylinktemplate_response_file_command_line @%responsefile%");
			OptionSetUtil.AppendOption(options, "link.template.responsefile.commandline", "mylinktemplate_response_file_contents %options% %liboutputname% %objectfiles%");
			project.NamedOptionSets.Add("testoptions", options);

			CcTask cc = new CcTask();
			cc.Project = project;
			cc.Sources.Include("D:/asdf/file.cpp", true);
			cc.OutputDir = testDir;
			cc.OutputName = "output.obj";
			cc.OptionSetName = "testoptions";

			LibTask lib = new LibTask();
			lib.Project = project;
			lib.Objects.Include("D:/asdf/file.obj", true);
			lib.OutputDir = testDir;
			lib.OutputName = "output.lib";
			lib.OptionSetName = "testoptions";

			AsTask assemble = new AsTask();
			assemble.Project = project;
			assemble.Sources.Include("D:/asdf/file.cpp", true);
			assemble.OutputDir = testDir;
			assemble.OutputName = "output.obj";
			assemble.OptionSetName = "testoptions";

			LinkTask link = new LinkTask();
			link.Project = project;
			link.Libraries.Include("D:/asdf/file.lib", true);
			link.OutputDir = testDir;
			link.OutputName = "output.obj";
			link.OptionSetName = "testoptions";

			cc.InitializeBuildOptions();
			string ccCommandLine = cc.GetCompilerCommandLine(cc.Sources.FileItems[0]);
			string ccResponseFileCommandLine = cc.GenerateResponseCommandLine("output.obj", ccCommandLine);
			Assert.IsTrue(ccResponseFileCommandLine.Contains("mycctemplate_response_file_command_line"));

			assemble.InitializeBuildOptions();
			string asCommandLine = assemble.GetCompilerCommandLine(assemble.Sources.FileItems[0]);
			string asResponseFileCommandLine = assemble.GenerateResponseCommandLine("output.obj", asCommandLine);
			Assert.IsTrue(asResponseFileCommandLine.Contains("myastemplate_response_file_command_line"));

			lib.InitializeBuildOptions();
			string libResponseFileContents = lib.GetCommandLine(out string libResponseFileCommandLine);
			Assert.IsTrue(libResponseFileContents.Contains("mylibtemplate_response_file_contents"));
			Assert.IsTrue(libResponseFileCommandLine.Contains("mylibtemplate_response_file_command_line"));

			link.InitializeBuildOptions();
			string linkResponseFileContents = link.GetCommandLine(out string linkResponseFileCommandLine);
			Assert.IsTrue(linkResponseFileContents.Contains("mylinktemplate_response_file_contents"));
			Assert.IsTrue(linkResponseFileCommandLine.Contains("mylinktemplate_response_file_command_line"));
		}

		[TestMethod]
		public void ResponseFileTest_BuildTools()
		{
			string testDir = TestUtil.CreateTestDirectory("ResponseFileTest2");

			Project project = new Project(Log.Silent);
			project.Properties["config-system"] = "kettle";
			project.Properties["config-compiler"] = "clang";
			project.Properties["config-platform"] = "kettle-clang";

			project.Properties["cc.useresponsefile"] = "true";
			project.Properties["as.useresponsefile"] = "true";
			project.Properties["lib.useresponsefile"] = "true";
			project.Properties["link.useresponsefile"] = "true";

			PackageMap.Init(project);

			CompilerTask CompilerBuildTool = new CompilerTask();
			CompilerBuildTool.Project = project;
			CompilerBuildTool.PathCLanguage = "compiler";
			CompilerBuildTool.Version = "1.0.0";
			CompilerBuildTool.Path = "compiler";
			CompilerBuildTool.Templates.CommandLine.Value = "%options%";
			CompilerBuildTool.Templates.IncludeDir.Value = "-I %includedir%";
			CompilerBuildTool.Templates.SystemIncludeDir.Value = "-I %includedir%";
			CompilerBuildTool.Templates.Define.Value = "-D %define%";
			CompilerBuildTool.Templates.Responsefile.CommandLine.Value = "cc_buildtool_commandline";
			CompilerBuildTool.Templates.Responsefile.Contents.Value = "cc_buildtool_contents";
			CompilerBuildTool.Execute();

			LibrarianTask LibrarianBuildTool = new LibrarianTask();
			LibrarianBuildTool.Project = project;
			LibrarianBuildTool.Version = "1.0.0";
			LibrarianBuildTool.Path = "lib";
			LibrarianBuildTool.Templates.ObjectFile.Value = "%objectfile%";
			LibrarianBuildTool.Templates.CommandLine.Value = "%options%";
			LibrarianBuildTool.Templates.Responsefile.CommandLine.Value = "lib_buildtool_commandline";
			LibrarianBuildTool.Templates.Responsefile.Contents.Value = "lib_buildtool_contents";
			LibrarianBuildTool.Execute();

			AssemblerTask AssemblerBuildTool = new AssemblerTask();
			AssemblerBuildTool.Project = project;
			AssemblerBuildTool.Version = "1.0.0";
			AssemblerBuildTool.Path = "assembler";
			AssemblerBuildTool.Templates.CommandLine.Value = "%options%";
			AssemblerBuildTool.Templates.IncludeDir.Value = "-I %includedir%";
			AssemblerBuildTool.Templates.SystemIncludeDir.Value = "-I %includedir%";
			AssemblerBuildTool.Templates.Define.Value = "-D %define%";
			AssemblerBuildTool.Templates.Responsefile.CommandLine.Value = "as_buildtool_commandline";
			AssemblerBuildTool.Templates.Responsefile.Contents.Value = "as_buildtool_contents";
			AssemblerBuildTool.Execute();

			LinkerTask LinkerBuildTool = new LinkerTask();
			LinkerBuildTool.Project = project;
			LinkerBuildTool.Version = "1.0.0";
			LinkerBuildTool.Path = "linker";
			LinkerBuildTool.Templates.CommandLine.Value = "%options%";
			LinkerBuildTool.Templates.LibraryDir.Value = "%librarydir%";
			LinkerBuildTool.Templates.LibraryFile.Value = "%libraryfile%";
			LinkerBuildTool.Templates.ObjectFile.Value = "%objectfile%";
			LinkerBuildTool.Templates.Responsefile.CommandLine.Value = "link_buildtool_commandline";
			LinkerBuildTool.Templates.Responsefile.Contents.Value = "link_buildtool_contents";
			LinkerBuildTool.Execute();

			OptionSet config_options_common = new OptionSet();
			OptionSetUtil.AppendOption(config_options_common, "buildset.protobuildtype", "true");
			project.NamedOptionSets.Add("config-options-common", config_options_common);

			OptionSet config_options_program = new OptionSet();
			OptionSetUtil.AppendOption(config_options_program, "buildset.name", "StdProgram");
			OptionSetUtil.AppendOption(config_options_program, "buildset.name", "asm cc link");
			project.NamedOptionSets.Add("config-options-program", config_options_program);

			OptionSet config_options_library = new OptionSet();
			OptionSetUtil.AppendOption(config_options_library, "buildset.name", "StdLibrary");
			OptionSetUtil.AppendOption(config_options_library, "buildset.name", "asm cc lib");
			project.NamedOptionSets.Add("config-options-library", config_options_library);

			OptionSet config_options_dynamiclibrary = new OptionSet();
			OptionSetUtil.AppendOption(config_options_dynamiclibrary, "buildset.name", "DynamicLibrary");
			OptionSetUtil.AppendOption(config_options_dynamiclibrary, "buildset.name", "asm cc link");
			project.NamedOptionSets.Add("config-options-dynamiclibrary", config_options_dynamiclibrary);

			Combine.Execute(project);

			CcTask cc = new CcTask();
			cc.Project = project;
			cc.Sources.Include("D:/asdf/file.cpp", true);
			cc.OutputDir = testDir;
			cc.OutputName = "output.obj";
			cc.OptionSetName = "testoptions";

			LibTask lib = new LibTask();
			lib.Project = project;
			lib.Objects.Include("D:/asdf/file.obj", true);
			lib.OutputDir = testDir;
			lib.OutputName = "output.lib";
			lib.OptionSetName = "testoptions";

			AsTask assemble = new AsTask();
			assemble.Project = project;
			assemble.Sources.Include("D:/asdf/file.cpp", true);
			assemble.OutputDir = testDir;
			assemble.OutputName = "output.obj";
			assemble.OptionSetName = "testoptions";

			LinkTask link = new LinkTask();
			link.Project = project;
			link.Libraries.Include("D:/asdf/file.lib", true);
			link.OutputDir = testDir;
			link.OutputName = "output.obj";
			link.OptionSetName = "testoptions";

			cc.InitializeBuildOptions();
			string ccCommandLine = cc.GetCompilerCommandLine(cc.Sources.FileItems[0]);
			string ccResponseFileCommandLine = cc.GenerateResponseCommandLine("output.obj", ccCommandLine);
			Assert.IsTrue(ccResponseFileCommandLine.Contains("cc_buildtool_commandline"));

			assemble.InitializeBuildOptions();
			string asCommandLine = assemble.GetCompilerCommandLine(assemble.Sources.FileItems[0]);
			string asResponseFileCommandLine = assemble.GenerateResponseCommandLine("output.obj", asCommandLine);
			Assert.IsTrue(asResponseFileCommandLine.Contains("as_buildtool_commandline"));

			lib.InitializeBuildOptions();
			string libResponseFileContents = lib.GetCommandLine(out string libResponseFileCommandLine);
			Assert.IsTrue(libResponseFileContents.Contains("lib_buildtool_contents"));
			Assert.IsTrue(libResponseFileCommandLine.Contains("lib_buildtool_commandline"));

			link.InitializeBuildOptions();
			string linkResponseFileContents = link.GetCommandLine(out string linkResponseFileCommandLine);
			Assert.IsTrue(linkResponseFileContents.Contains("link_buildtool_contents"));
			Assert.IsTrue(linkResponseFileCommandLine.Contains("link_buildtool_commandline"));
		}

		[TestMethod]
		public void GenerateBuildOptionsetTest()
		{
			string testDir = TestUtil.CreateTestDirectory("GenerateBuildOptionsetTest");

			Project project = new Project(Log.Silent);

			// optionset must have a buildset.name field
			OptionSet os1 = new OptionSet
			{
				Project = project
			};
			os1.Options.Add("myoption", "on");
			project.NamedOptionSets["config-options-no-name"] = os1;
			ContextCarryingException exception1 = Assert.ThrowsException<ContextCarryingException>(() => GenerateBuildOptionset.Execute(project, "config-options-no-name"));
			Assert.IsTrue(exception1.GetBaseException().Message.Contains("must have option 'buildset.name'"));
			project.NamedOptionSets.Remove("config-options-no-name");

			// check that the final buildtype has the option that we provide
			OptionSet os2 = new OptionSet
			{
				Project = project
			};
			os2.Options.Add("buildset.name", "MyLibrary");
			os2.Options.Add("myoption", "on");
			project.NamedOptionSets["config-options-my-library"] = os2;
			GenerateBuildOptionset.Execute(project, "config-options-my-library");
			Assert.IsTrue(project.NamedOptionSets.Contains("MyLibrary"));
			// potentially could change in the future, but we should probably be aware of the change
			Assert.IsTrue(project.NamedOptionSets["MyLibrary"].Options.Count == 4); 
			Assert.IsTrue(project.NamedOptionSets["MyLibrary"].Options["name"] == "MyLibrary");
			Assert.IsTrue(project.NamedOptionSets["MyLibrary"].Options["myoption"] == "on");
			// I think these two values are just informational, maybe for debugging purposes
			Assert.IsTrue(project.NamedOptionSets["MyLibrary"].Options["buildset.finalbuildtype"] == "true");
			Assert.IsTrue(project.NamedOptionSets["MyLibrary"].Options["buildset.protobuildtype.name"] == "config-options-my-library");

			// Test that options can selectively be applied to specific files in a fileset
			File.WriteAllText(Path.Combine(testDir, "test.cpp"), "");
			File.WriteAllText(Path.Combine(testDir, "test.cxx"), "");

			FileSet myfileset = new FileSet
			{
				Project = project,
				BaseDirectory = testDir
			};
			myfileset.Include("*.cpp");
			myfileset.Include("*.cxx", optionSetName: "MyLibrary");
			Assert.IsTrue(myfileset.FileItems.Count == 2);
			Assert.IsTrue(myfileset.FileItems[0].OptionSetName == null);
			Assert.IsTrue(myfileset.FileItems[1].OptionSetName == "MyLibrary");

			// test that generate optionset removes buildset from the option name
			OptionSet os3 = new OptionSet
			{
				Project = project
			};
			os3.Options.Add("buildset.name", "MySecondLibrary");
			os3.Options.Add("buildset.myoption", "on");
			project.NamedOptionSets["config-options-my-second-library"] = os3;
			GenerateBuildOptionset.Execute(project, "config-options-my-second-library");
			Assert.IsTrue(project.NamedOptionSets.Contains("MySecondLibrary"));
			Assert.IsTrue(project.NamedOptionSets["MySecondLibrary"].Options.Contains("myoption"));
			Assert.IsTrue(project.NamedOptionSets["MySecondLibrary"].Options["myoption"] == "on");

			// test that correct thing happens when we set the clanguage flag to on
			project.Properties["cc-clanguage.template.commandline"] = "cc-clanguage-template";
			project.Properties["as-clanguage.template.commandline"] = "as-clanguage-template";
			project.Properties["lib-clanguage.template.commandline"] = "lib-clanguage-template";
			project.Properties["link-clanguage.template.commandline"] = "link-clanguage-template";
			project.Properties["cc-clanguage.template.responsefile.commandline"] = "cc-clanguage-template-responsefile";
			project.Properties["as-clanguage.template.responsefile.commandline"] = "as-clanguage-template-responsefile";
			project.Properties["lib-clanguage.template.responsefile.commandline"] = "lib-clanguage-template-responsefile";
			project.Properties["link-clanguage.template.responsefile.commandline"] = "link-clanguage-template-responsefile";

			OptionSet os4 = new OptionSet
			{
				Project = project
			};
			os4.Options.Add("buildset.name", "MyCLibrary");
			os4.Options.Add("clanguage", "on");
			project.NamedOptionSets["config-options-my-c-library"] = os4;
			GenerateBuildOptionset.Execute(project, "config-options-my-c-library");
			Assert.IsTrue(project.NamedOptionSets.Contains("MyCLibrary"));
			Assert.IsTrue(project.NamedOptionSets["MyCLibrary"].Options["cc.template.commandline"] == "cc-clanguage-template");
			Assert.IsTrue(project.NamedOptionSets["MyCLibrary"].Options["as.template.commandline"] == "as-clanguage-template");
			Assert.IsTrue(project.NamedOptionSets["MyCLibrary"].Options["lib.template.commandline"] == "lib-clanguage-template");
			Assert.IsTrue(project.NamedOptionSets["MyCLibrary"].Options["link.template.commandline"] == "link-clanguage-template");
			Assert.IsTrue(project.NamedOptionSets["MyCLibrary"].Options["cc.template.responsefile.commandline"] == "cc-clanguage-template-responsefile");
			Assert.IsTrue(project.NamedOptionSets["MyCLibrary"].Options["as.template.responsefile.commandline"] == "as-clanguage-template-responsefile");
			Assert.IsTrue(project.NamedOptionSets["MyCLibrary"].Options["lib.template.responsefile.commandline"] == "lib-clanguage-template-responsefile");
			Assert.IsTrue(project.NamedOptionSets["MyCLibrary"].Options["link.template.responsefile.commandline"] == "link-clanguage-template-responsefile");
		}

		[TestMethod]
		public void ExcludeDirTaskTest()
		{
			Project project = new Project(Log.Silent);

			string testDir = TestUtil.CreateTestDirectory("ExcludeDirTest");
			project.Properties["package.dir"] = Path.Combine(testDir, "package", "dev");

			Directory.CreateDirectory(Path.Combine(project.Properties["package.dir"], "source", "lib"));
			File.WriteAllText("test.cpp", Path.Combine(project.Properties["package.dir"], "source"));
			File.WriteAllText("test2.cpp", Path.Combine(project.Properties["package.dir"], "source", "lib"));

			FileSet myfileset = new FileSet();
			myfileset.Include(project.Properties["package.dir"] + "source/**");
			project.NamedFileSets["sourcefiles"] = myfileset;
			Assert.IsTrue(myfileset.Includes.Count == 1);
			Assert.IsTrue(myfileset.Excludes.Count == 0);

			ExcludeDirTask excludedir = new ExcludeDirTask(project: project, fileset: "sourcefiles", directory: Path.Combine(project.Properties["package.dir"], "source", "lib"));
			excludedir.Execute();
			Assert.IsTrue(myfileset.Includes.Count == 1);
			Assert.IsTrue(myfileset.Excludes.Count == 1);
		}

		[TestMethod]
		public void GetTlbimpExeTest()
		{
			Project project = new Project(Log.Silent);

			Assert.AreEqual(GenerateModuleInteropAssembliesTask.GetTlbimpExe(project), null);
			project.Properties.Remove("tlbimp.exe");
			project.Properties["package.WindowsSDK.appdir"] = "appdir";
			Assert.AreEqual(GenerateModuleInteropAssembliesTask.GetTlbimpExe(project), "appdir/bin/tlbimp.exe");
			project.Properties.Remove("tlbimp.exe");
			project.Properties["package.WindowsSDK.MajorVersion"] = "10";
			project.Properties["package.WindowsSDK.dotnet.tools.dir"] = "toolsdir";
			Assert.AreEqual(GenerateModuleInteropAssembliesTask.GetTlbimpExe(project), "toolsdir/tlbimp.exe");
			project.Properties.Remove("tlbimp.exe");
			project.Properties["package.WindowsSDK.tools.tlbimp"] = "tools";
			Assert.AreEqual(GenerateModuleInteropAssembliesTask.GetTlbimpExe(project), "tools");
			project.Properties.Remove("tlbimp.exe");
			project.Properties["tlbimp.exe"] = "tlbimp.exe";
			Assert.AreEqual(GenerateModuleInteropAssembliesTask.GetTlbimpExe(project), "tlbimp.exe");
		}

		[TestMethod]
		public void GetComLibraryNameTaskTest()
		{
			Project project = new Project(Log.Silent);

			string testDir = TestUtil.CreateTestDirectory("GetComLibraryNameTaskTest");
			Assert.ThrowsException<NullReferenceException>(() => new TestableGetComLibraryNameTask().Execute());
			string libPath = Path.Combine(testDir, "mylib.lib");
			File.WriteAllText(libPath, "");
			new TestableGetComLibraryNameTask(project, libPath, "resultprop").Execute();
			Assert.AreEqual(project.Properties["resultprop"], "testName");
		}

		[TestMethod]
		public void GenerateWebReferencesTaskTest()
		{
			Project project = new Project(Log.Silent);

			string testDir = TestUtil.CreateTestDirectory("GenerateWebReferencesTest");

			Assert.AreEqual(GenerateModuleWebReferencesTask.GetWsdlExe(project), null);
			project.Properties.Remove("wsdl.exe");
			project.Properties["package.WindowsSDK.appdir"] = "appdir";
			Assert.AreEqual(GenerateModuleWebReferencesTask.GetWsdlExe(project), "appdir/bin/wsdl.exe");
			project.Properties.Remove("wsdl.exe");
			project.Properties["package.WindowsSDK.MajorVersion"] = "10";
			project.Properties["package.WindowsSDK.dotnet.tools.dir"] = "toolsdir";
			Assert.AreEqual(GenerateModuleWebReferencesTask.GetWsdlExe(project), "toolsdir/wsdl.exe");
			project.Properties.Remove("wsdl.exe");
			project.Properties["package.WindowsSDK.tools.wsdl"] = "tools";
			Assert.AreEqual(GenerateModuleWebReferencesTask.GetWsdlExe(project), "tools");
			project.Properties.Remove("wsdl.exe");
			project.Properties["wsdl.exe"] = Path.Combine(testDir, "wsdl.exe");
			Assert.AreEqual(GenerateModuleWebReferencesTask.GetWsdlExe(project), Path.Combine(testDir, "wsdl.exe"));

			File.WriteAllText(Path.Combine(testDir, "wsdl.exe"), ""); // create the file so the file existance check doesn fail
			project.Properties["package.builddir"] = testDir;

			OptionSet csbuildtype = new OptionSet();
			csbuildtype.Options.Add("build.tasks", "csc");
			csbuildtype.Options.Add("csc.target", "library");
			project.NamedOptionSets["CSharpLibrary"] = csbuildtype;

			OptionSet cppbuildtype = new OptionSet();
			cppbuildtype.Options.Add("build.tasks", "link");
			cppbuildtype.Options.Add("cc.options", "clr");
			cppbuildtype.Options.Add("link.options", "-dll");
			project.NamedOptionSets["ManagedCppAssembly"] = cppbuildtype;

			project.Properties["runtime.mycsmodule.buildtype"] = "CSharpLibrary";
			project.Properties["test.mycppmodule.buildtype"] = "ManagedCppAssembly";

			OptionSet myreferences = new OptionSet();
			myreferences.Options.Add("option1", "value1");
			project.NamedOptionSets["runtime.mycsmodule.webreferences"] = myreferences;
			project.NamedOptionSets["test.mycppmodule.webreferences"] = myreferences;
			project.NamedOptionSets["test.myfsmodule.webreferences"] = myreferences;

			new TestableGenerateModuleWebReferencesTask(project, "mycsmodule", output: "myoutput").Execute();
			Assert.IsTrue(project.NamedFileSets.Contains("myoutput"));
			Assert.IsFalse(project.Properties.Contains("myoutput"));
			Assert.AreEqual(project.Properties["webref.lang"], "CS");
			Assert.AreEqual(project.Properties["webcodedir"], Path.Combine(project.Properties["package.builddir"], "mycsmodule", "webreferences"));

			new TestableGenerateModuleWebReferencesTask(project, "mycppmodule", group: "test", output: "myoutput").Execute();
			Assert.AreEqual(project.Properties["webcodedir"], Path.Combine(project.Properties["package.builddir"], "mycppmodule", "webreferences"));
			Assert.AreEqual(project.Properties["myoutput"], Path.Combine(project.Properties["package.builddir"], "mycppmodule", "webreferences"));
			Assert.AreEqual(project.Properties["webref.lang"], "CPP");

			Assert.ThrowsException<ContextCarryingException>(() => new TestableGenerateModuleWebReferencesTask(project, "myfsmodule", group: "test", output: "myoutput").Execute());
		}

		/// <summary>
		/// A test to verify that the build script intellisense generation code works as expected
		/// </summary>
		[TestMethod]
		public void IntellisenseTest()
		{
			Project project = new Project(Log.Silent);

			string testDir = TestUtil.CreateTestDirectory("IntellisenseTest");

			string schemaFile = Path.Combine(testDir, "framework3.xsd");

			Assert.IsFalse(File.Exists(schemaFile));

			FileSet assemblies = new FileSet();
			assemblies.Include(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "NAnt.Core.dll"));
			assemblies.Include(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "NAnt.Tasks.dll"));
			assemblies.Include(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "EA.Tasks.dll"));

			NAntSchemaTask task = new NAntSchemaTask();
			task.Project = project;
			task.OutputFile = schemaFile;
			task.AssemblyFileSet = assemblies;
			task.Execute();

			Assert.IsTrue(File.Exists(schemaFile));
		}

		#region Testing Stubs
		public class TestableGetComLibraryNameTask : GetComLibraryNameTask
		{
			public TestableGetComLibraryNameTask() : base() { }
			public TestableGetComLibraryNameTask(Project p, string lib, string result) : base(p, lib, result) { }
			protected override ITypeLib LoadTypeLib(string strTypeLibName, RegKind regKind) { return new TestingTypeLib(); }
		}

		public class TestingTypeLib : ITypeLib
		{
			public int GetTypeInfoCount() { throw new NotImplementedException(); }
			public void GetTypeInfo(int index, out ITypeInfo ppTI) { throw new NotImplementedException(); }
			public void GetTypeInfoType(int index, out TYPEKIND pTKind) { throw new NotImplementedException(); }
			public void GetTypeInfoOfGuid(ref Guid guid, out ITypeInfo ppTInfo) { throw new NotImplementedException(); }
			public void GetLibAttr(out IntPtr ppTLibAttr) { throw new NotImplementedException(); }
			public void GetTypeComp(out ITypeComp ppTComp) { throw new NotImplementedException(); }
			public void GetDocumentation(int index, out string strName, out string strDocString, out int dwHelpContext, out string strHelpFile) {
				strName = "testName";
				strDocString = "testDocString";
				dwHelpContext = 0;
				strHelpFile = "testHelpFile";
			}
			public bool IsName(string szNameBuf, int lHashVal) { throw new NotImplementedException(); }
			public void FindName(string szNameBuf, int lHashVal, ITypeInfo[] ppTInfo, int[] rgMemId, ref short pcFound) { throw new NotImplementedException(); }
			public void ReleaseTLibAttr(IntPtr pTLibAttr) { throw new NotImplementedException(); }
		}

		// override the certain methods with stubs
		public class TestableGenerateModuleWebReferencesTask : GenerateModuleWebReferencesTask
		{
			public TestableGenerateModuleWebReferencesTask(Project project, string module, string group = "runtime", string output = null)
				: base(project, module, group, output) { }

			protected override void RunWsdl(string exe, string directory, string name, string value, string lang)
			{
			}
		}
		#endregion
	}
}