// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Core.Events;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;

namespace FBConfig.Tasks
{
	[TaskName("audio-spu-jobs-to-make")]
	class AudioSpuJobsToMake : Task
	{
		const string packagename = "EAAudioCore";

		protected override void ExecuteTask()
		{
			var jobModules = new List<Module>();
			
			foreach (var mod in Project.BuildGraph().Modules.Where(e => e.Value.Package.Name == packagename && e.Value.Configuration.System=="ps3" && e.Value.Configuration.SubSystem==".spu").Select(e => e.Value).Cast<Module>())
			{
				mod.SetType(Module.ExcludedFromBuild);
				jobModules.Add(mod);
			}
			if (jobModules.Count > 0)
			{
				Console.WriteLine(LogPrefix + "Converting SPU jobs to MakeStyle module: {0}", jobModules.ToString("; ", m => m.Name));

				foreach (var group in jobModules.GroupBy(m => m.Configuration))
				{
					if (group.Count() > 0)
					{
						var makeModule = CreateMakeModule(group);

						foreach (var topmod in Project.BuildGraph().TopModules.Where(m => m.Configuration == makeModule.Configuration))
						{
							topmod.Dependents.Add(makeModule, DependencyTypes.Build);
						}

						foreach (var audioCoreModule in Project.BuildGraph().Modules.Where(e => e.Value.Package.Name == packagename && e.Value.Configuration == group.First().Configuration && e.Value.Configuration.System == "ps3" && e.Value.Configuration.SubSystem != ".spu").Select(e => e.Value).Cast<Module>())
						{
							audioCoreModule.Dependents.Add(makeModule, DependencyTypes.Build);
						}
					}
				}
			}
		}

		private Module_MakeStyle CreateMakeModule(IEnumerable<Module> jobs)
		{
			var first = jobs.First();
			var name = first.Package.Name + ".SpuJobs";
			var groupname = first.BuildGroup + name;

			var makeModule = new Module_MakeStyle(name, groupname, String.Empty, first.Configuration, first.BuildGroup, first.Package);

			makeModule.OutputDir = first.Package.PackageConfigBuildDir;
			makeModule.IntermediateDir = first.Package.PackageConfigBuildDir;

			using (IMakeWriter writer = new MakeWriter())
			{
				var makefile = String.Format("{0}_SpuJobs.mak", packagename);

				writer.FileName = Path.Combine(makeModule.IntermediateDir.Path, makefile);
				writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnMakeFileUpdate);

				makeModule.ExcludedFromBuild_Files.Include(writer.FileName, failonmissing: false);

				WriteMakeFileHeader(writer, makeModule, first as Module_Native);

				var targets = new List<string>();

				foreach (var job in jobs)
				{
					var jobtarget = JobToMake(writer, makeModule, job as Module_Native);

					if(!String.IsNullOrEmpty(jobtarget))
					{
						targets.Add(jobtarget);
					}
				}

				WriteAllTarget(writer, makeModule, targets);

				AddBuildSteps(makeModule, writer.FileName);
			}

			return makeModule;
		}

		private void WriteMakeFileHeader(IMakeWriter writer, Module_MakeStyle makeModule, Module_Native ajob)
		{
			writer.WriteHeader("generated for 'nmake' makefile for {0} SPU jobs. {1}", makeModule.Package.Name, makeModule.Configuration.Name);
			writer.WriteLine();
			writer.WriteLine(".SUFFIXES: .c .cpp .cxx .cc .s .asm");
			writer.WriteLine(".PHONY : all welcome goodbye");
			writer.WriteLine();
		}

		private void WriteAllTarget(IMakeWriter writer, Module_MakeStyle makeModule, IEnumerable<string> targets)
		{
			writer.WriteLine();
			writer.WriteLine();
			writer.WriteLine("welcome:");
			writer.WriteTabLine("@echo Building Plugin SPU binaries...");

			var outputFolders = from t in targets select Path.GetDirectoryName(t);
			outputFolders.Distinct().ToList().ForEach(f => writer.WriteTabLine("@IF NOT EXIST {0} MKDIR {0}", f));

			writer.WriteLine();
			writer.WriteLine("goodbye:");
			writer.WriteTabLine("@echo Done Building Plugin SPU binaries.");
			writer.WriteLine();
			writer.WriteLine("all: welcome {0} goodbye", targets.ToString(" "));
			writer.WriteLine();
			writer.WriteLine("clean:");
			writer.WriteTabLine("@echo Clean Plugin SPU binaries.");
			var cleanTargets = from t in targets select String.Format("@IF EXIST {0} DEL {0}", t);
			cleanTargets.ToList().ForEach(s => writer.WriteTabLine(s));
		}


		private string JobToMake(IMakeWriter writer, Module_MakeStyle makeModule, Module_Native job)
		{
			var jobtarget = String.Empty;
			if (job != null)
			{
				makeModule.ExcludedFromBuild_Sources.Include(job.ExcludedFromBuild_Sources);
				makeModule.ExcludedFromBuild_Files.Include(job.ExcludedFromBuild_Files);
				makeModule.ExcludedFromBuild_Files.Include(job.Cc.SourceFiles);

				var JNAME = job.Name.ToUpperInvariant().Replace('.', '_');
				var compiler = job.Cc.Executable;
				var linker = job.Link.Executable;

				// Write to make file:
				writer.WriteLine();
				writer.WriteHeader("Job {0}", job.Name);
				writer.WriteLine();

				writer.WriteLine("CC_{0}={1}", JNAME, compiler.Path);
				writer.WriteLine("LINK_{0}={1}", JNAME, linker.Path);

				var cc_options = WriteOptions(writer, "CC_OPTIONS", JNAME, job.Cc.Options);

				var defineTemplate = job.Cc.Templates["cc.template.define"];
				var cc_defines = WriteOptions(writer, "CC_DEFINES", JNAME, job.Cc.Defines, (d)=>defineTemplate.Replace("%define%", d));

				var inclTemplate = job.Cc.Templates["cc.template.includedir"];
				var cc_includedir = WriteOptions(writer, "CC_INCLUDES", JNAME, job.Cc.IncludeDirs, (inc)=>inclTemplate.Replace("%includedir%", inc.Path.Quote()));

				var objects = new List<string>();

				var cc_template = PrepareCommandlineTemplate(job, "cc.template.commandline", defaultTemplate:"%defines%  %options%  %includedirs% -o \"%objectfile%\" \"%sourcefile%\"");

				foreach (var fi in job.Cc.SourceFiles.FileItems)
				{
					writer.WriteLine();

					string objectfile = ObjectFile(job, fi); 
					var cc_command_args = cc_template.Replace("%defines%", "$(" + cc_defines + ")")
												  .Replace("%options%", "$(" + cc_options + ")")
												  .Replace("%includedirs%", "$(" + cc_includedir + ")")
												  .Replace("%objectfile%", objectfile)
												  .Replace("%sourcefile%", fi.Path.Path);

					writer.WriteLine("{0}: {1}", objectfile, fi.Path.Path.Quote());
					writer.WriteTabLine("@IF NOT EXIST {0} MKDIR {0}", Path.GetDirectoryName(objectfile).Quote());
					//WriteBuildEvents(writer, job, "prebuild", BuildStep.PreBuild);
					writer.WriteTabLine("@ECHO {0}", Path.GetFileName(fi.Path.Path));
					writer.WriteTabLine("@$(CC_{0}) {1}", JNAME, cc_command_args);

					objects.Add(objectfile);
				}
				writer.WriteLine();

				foreach(var fi in job.Link.ObjectFiles.FileItems)
				{
					objects.Add(fi.Path.Path);
				}

				var link_options = WriteOptions(writer, "LINK_OPTIONS", JNAME, job.Link.Options);
				var libTemplate = PrepareToolTemplate(job.Link.Templates, "link.template.libraryfile", "\"%libraryfile%\"");
				var link_libraries = WriteOptions(writer, "LINK_LIBS", JNAME, job.Link.Libraries.FileItems, fi=>libTemplate.Replace("%libraryfile%",fi.Path.Path));
				var objTemplate = PrepareToolTemplate(job.Link.Templates, "link.template.objectfile", "\"%objectfile%\"");
				var link_objects = WriteOptions(writer, "LINK_OBJS", JNAME, objects, obj => objTemplate.Replace("%objectfile%", obj));
				var libdirTemplate = PrepareToolTemplate(job.Link.Templates, "link.template.librarydir", "-L \"%librarydir%\"");
				var link_lib_dirs = WriteOptions(writer, "LINK_LIB_DIRS", JNAME, job.Link.LibraryDirs, dir => libdirTemplate.Replace("%librarydir%",dir.Path));

				var link_template = PrepareCommandlineTemplate(job, "link.template.commandline", defaultTemplate:"%options% %objectfiles% %librarydirs% -Xlinker --start-group %libraryfiles% -Xlinker --end-group");

				var link_command_args = link_template.Replace("%options%", "$(" + link_options + ")")
													 .Replace("%objectfiles%", "$(" + link_objects + ")")
													 .Replace("%librarydirs%", "$(" + link_lib_dirs + ")")
													 .Replace("%libraryfiles%", "$(" + link_libraries + ")");

				var linkoutput = PathString.MakeCombinedAndNormalized(job.Link.LinkOutputDir.Path, job.Link.OutputName + job.Link.OutputExtension);
				writer.WriteLine("{0}: $({1}) $({2}) $(MAKEFILE)", linkoutput, link_objects, link_libraries);
				writer.WriteTabLine("@$(LINK_{0}) {1}", JNAME, link_command_args);
				WriteBuildEvents(writer, job, "postbuild", BuildStep.PostBuild);
				writer.WriteLine();

				jobtarget = linkoutput.Path;
			}
			return jobtarget;
		}

		private string PrepareToolTemplate(IDictionary<string,string> tools, string templatename, string defaultTemplate)
		{
			string template;
			if(!(tools != null && tools.TryGetValue(templatename, out template)))
			{
				template = defaultTemplate;
			}
			return template;
		}


		private string PrepareCommandlineTemplate(IModule module, string templatename, string defaultTemplate)
		{
			return module.Package.Project.Properties.GetPropertyOrDefault(templatename, defaultTemplate).LinesToArray().ToString(" ");
		}

		private string ObjectFile(IModule mod, FileItem fi)
		{
			var objectfile = String.Empty;
			FileData filedata = fi.Data as FileData;
			if (filedata != null && filedata.ObjectFile != null)
			{
				objectfile = filedata.ObjectFile.Path;
			}
			else
			{
				objectfile = Path.Combine(mod.IntermediateDir.Path, Path.GetFileName(fi.Path.Path) + ".obj");
			}

			return objectfile;
		}

		private string WriteOptions<T>(IMakeWriter writer, string name, string modulename, ICollection<T> options, Func<T, string> convert = null)
		{
			var varName = string.Format("{0}_{1}", name, modulename);
			writer.WriteLine();
			writer.WriteLine(@"{0}=\", varName);

			int maxInd = options.Count - 1;
			int i = 0;
			foreach (var option in options)
			{
				if (i < maxInd)
				{
					writer.WriteLine(@"{0}\", convert == null? option.ToString() : convert(option));
				}
				else
				{
					writer.WriteLine(@"{0}", convert == null ? option.ToString() : convert(option));
				}
				i++;
			}
			writer.WriteLine();
			return varName;
		}

		private void WriteBuildEvents(IMakeWriter writer, Module_Native job, string toolname, uint type)
		{
			 foreach (var command in job.BuildSteps.Where(step => step.IsKindOf(type) && step.Name == toolname).SelectMany(step => step.Commands))
			 {
				 foreach (var line in GetCommandScriptWithCreateDirectories(command).LinesToArray())
				 {
					 var clean_line = line.Replace("$(OutDir)", job.Link.LinkOutputDir.Path.EnsureTrailingSlash()).Replace("$(TargetName)", job.Link.OutputName).Replace("$(TargetExt)", job.Link.OutputExtension);
					 writer.WriteTabLine(clean_line);
				 }
			 }
		}

		private string GetCommandScriptWithCreateDirectories(Command command)
		{
			var commandstring = new StringBuilder();

			if (command.CreateDirectories != null && command.CreateDirectories.Count() > 0)
			{

				string format = "@if not exist \"{0}\" mkdir \"{0}\"";

				switch (Environment.OSVersion.Platform)
				{
					case PlatformID.MacOSX:
					case PlatformID.Unix:
						format = "NOT IMPLEMENTED";
						break;
				}

				foreach (var createdir in command.CreateDirectories)
				{
					commandstring.AppendFormat(format, createdir.Normalize(PathString.PathParam.NormalizeOnly));
					commandstring.AppendLine();
				}
			}

			commandstring.AppendLine(command.CommandLine);
			return commandstring.ToString();
		}



		private void AddBuildSteps(Module_MakeStyle make_module, string makefile)
		{
			var buildstep = new BuildStep("prebuild", BuildStep.Build | BuildStep.ExecuteAlways);
			buildstep.Commands.Add(new Command(CreateMakeCommand(makefile, "all")));
			make_module.BuildSteps.Add(buildstep);

			buildstep = new BuildStep("prebuild", BuildStep.ReBuild | BuildStep.ExecuteAlways);
			buildstep.Commands.Add(new Command(CreateMakeCommand(makefile, "clean all")));
			make_module.BuildSteps.Add(buildstep);

			buildstep = new BuildStep("prebuild", BuildStep.Clean | BuildStep.ExecuteAlways);
			buildstep.Commands.Add(new Command(CreateMakeCommand(makefile, "clean")));
			make_module.BuildSteps.Add(buildstep);
		}

		private string CreateMakeCommand(string makefile, string targets)
		{
			// Get location of nmake and add it to path:
			Properties["package.VisualStudio.exportbuildsettings"] = "false";
			TaskUtil.Dependent(Project, "VisualStudio", Project.TargetStyleType.Use);

			string processorSubFolder = String.Empty;
			if (Properties["config-processor"] == "x64")
			{
				processorSubFolder = System.Environment.Is64BitOperatingSystem ? "amd64" : "x86_amd64";
			}

			PathString vcbinpath = PathString.MakeNormalized(Path.Combine(Properties["package.VisualStudio.appdir"], @"VC\BIN\" + processorSubFolder));

			var cmd = new StringBuilder();

			cmd.AppendFormat("SET PATH=%PATH%;{0}{1}", vcbinpath.Path, EOL);            
			cmd.AppendFormat("SET MAKEFLAGS={0}", EOL);
			cmd.AppendFormat("nmake {0} -f {1}", targets, makefile);
			return cmd.ToString();
		}

		protected void OnMakeFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				Log.Status.WriteLine("{0}{1} make file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
			}
		}


		const string EOL = "&& ";

	}
}
