// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Backends.Text
{
	/// <summary>The task used by the buildinfo target to generate the build info file</summary>
	[TaskName("WriteBuildInfo")]
	public class WriteBuildInfo : Task
	{
		public WriteBuildInfo() : base("WriteBuildInfo") { }

		[TaskAttribute("outputdir", Required = false)]
		public string OutputDir
		{
			get { return !String.IsNullOrEmpty(_outputdir) ? _outputdir : Path.Combine(Project.GetFullPath(Project.ExpandProperties(Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT])), "buildinfo", "txt"); }
			set { _outputdir = value; }
		}
		private string _outputdir;
		private ITextWriter _writer;

		protected override void ExecuteTask()
		{
			var timer = new Chrono();

			foreach (IGrouping<string, IModule> allModules in Project.BuildGraph().Modules.GroupBy(m => m.Value.Configuration.Name, m => m.Value))
			{
				foreach (var module in allModules.Where(m => !(m is Module_UseDependency)))
				{
					WriteModule(module as Module, PathString.MakeCombinedAndNormalized(OutputDir, allModules.Key));
				}
			}

			Log.Minimal.WriteLine(LogPrefix + "  {0} buildinfo directory : {1}", timer.ToString(), OutputDir.Quote());
		}

		public void WriteOneConfig(IEnumerable<IModule> topmodules, PathString dir)
		{
			var allmodules = topmodules.Union(topmodules.SelectMany(m => m.Dependents.FlattenAll(), (m, d) => d.Dependent).Where(m => !(m is Module_UseDependency)));

			foreach (var module in allmodules)
			{
				WriteModule(module as Module, dir);
			}
		}

		private void WriteModule(Module module, PathString dir)
		{
			using (var writer = new TextWriterWithIndent(writeBOM:false))
			{
				var processableModule = module as ProcessableModule;
				writer.FileName = Path.Combine(dir.Path, GetModuleFileName(module));
				writer.IndentSize = 2;

				_writer = writer;

				var buildOptions = module.GetModuleOptions();

				writer.WriteLine("# This is a valid -*- YAML -*- file, feed your Python with it");
				writer.WriteLine("name: {0}.{1}", module.BuildGroup, module.Name);
				writer.WriteLine("package: {0}-{1}", module.Package.Name, module.Package.Version);
				writer.WriteLine("packageName: {0}", module.Package.Name);
				writer.WriteLine("packageVersion: {0}", module.Package.Version);
				writer.WriteLine("config: {0}", module.Configuration.Name);
				writer.WriteLine("type: {0}", ModuleTypeToString(module));
				WritePath("output_dir", module.OutputDir);
				WritePath("intermediate_dir", module.IntermediateDir);
				if (!Project.Properties.GetBooleanPropertyOrDefault("buildinfo.hide-sxml-differences", false))
				{
					WritePath("script_file", module.ScriptFile);
				}
				if (processableModule != null)
				{
					writer.WriteLine("buildtype: {0}", processableModule.BuildType.Name);
					writer.WriteLine("basebuildtype: {0}", processableModule.BuildType.BaseName);
				}
				writer.WriteLine("deploy_assets: {0}", module.DeployAssets);
				writer.WriteLine();
				WriteProcessSteps(module, buildOptions, "preprocess");
				writer.WriteLine();
				WriteProcessSteps(module, buildOptions, "postprocess");
				writer.WriteLine();
				writer.WriteLine("dependencies:");
				Indented(() =>
				{
					foreach (var dep in module.Dependents)
					{
						writer.WriteLine(GetDependencyInfoString(dep));
					}
				});

				writer.WriteLine();

				writer.WriteLine("tools:");
				Indented(() => {
					foreach (var tool in module.Tools)
					{
						writer.WriteLine("-");
						PushIndent();
						{
							WriteTool(tool);
						}
						PopIndent();
					}
				});

				writer.WriteLine();
				writer.WriteLine("build_steps:");
				Indented(() =>
				{
					WriteBuildSteps(module.BuildSteps, BuildStep.PreBuild);
					WriteBuildSteps(module.BuildSteps, BuildStep.PreLink);
					WriteBuildSteps(module.BuildSteps, BuildStep.PostBuild);
					// These are set if module is makestyle:
					WriteBuildSteps(module.BuildSteps, BuildStep.Build);
					WriteBuildSteps(module.BuildSteps, BuildStep.Clean);
					WriteBuildSteps(module.BuildSteps, BuildStep.ReBuild);
				});

				writer.WriteLine();
				writer.WriteLine("module_files:");
				Indented(() =>
				{
					WriteFiles("- excludedfrombuild_sources", module.ExcludedFromBuild_Sources);
					WriteFiles("- excludedfrombuild_files", module.ExcludedFromBuild_Files);
					WriteFiles("- assets", module.Assets);
				});
			}
		}

		void WriteMultiLineList(IEnumerable<string> xs)
		{
			foreach (string x in xs)
			{
				_writer.WriteLine("- {0}", QuoteListVal(x));
			}
		}

		private void WriteProcessSteps(Module module, OptionSet buildOptions, string type)
		{
			_writer.WriteLine("{0}_steps:", type);
			Indented(() =>
			{
				_writer.WriteLine("build_options: {0}", InlineList(buildOptions.Options[type.ToLowerInvariant()].LinesToArray()));
				_writer.WriteLine("eaconfig.postprocess: {0}", InlineList(module.Package.Project.Properties["eaconfig." + type.ToLowerInvariant()].LinesToArray()));
				_writer.WriteLine(module.GroupName + ".postprocess: {0}", InlineList(module.Package.Project.Properties["eaconfig." + type.ToLowerInvariant()].LinesToArray()));
			});
		}

		private void WriteTool(Tool tool, bool writefiles = true, FileItem file=null)
		{
			if (tool is BuildTool)
			{
				WriteTool(tool as BuildTool, writefiles);
			}
			else if (tool is CcCompiler)
			{
				WriteTool(tool as CcCompiler, writefiles, file);
			}
			else if (tool is AsmCompiler)
			{
				WriteTool(tool as AsmCompiler, writefiles, file);
			}
			else if (tool is Linker)
			{
				WriteTool(tool as Linker, writefiles);
			}
			else if (tool is Librarian)
			{
				WriteTool(tool as Librarian, writefiles);
			}
			else if (tool is DotNetCompiler)
			{
				WriteTool(tool as DotNetCompiler, writefiles);
			}
			else if (tool is PostLink)
			{
				WriteTool(tool as PostLink, writefiles);
			}
			else if (tool is AppPackageTool)
			{
				WriteTool(tool as AppPackageTool, writefiles);
			}
			else
			{
				WriteToolGeneric(tool, writefiles);
			}
		}

		private void WriteTool(BuildTool tool, bool writefiles = true)
		{
			_writer.WriteLine("name: {0}", tool.ToolName);
			_writer.WriteLine("type: {0}", ToolTypeToString(tool));
			_writer.WriteLine("description: {0}", QuoteMapVal(tool.Description));
			_writer.WriteLine("excludedfrombuild: {0}", tool.IsKindOf(BuildTool.ExcludedFromBuild));
			if (tool.IsKindOf(BuildTool.Program))
			{
				WritePath("executable", tool.Executable);
				WriteList("options", tool.Options);
			}
			else
			{
				_writer.WriteLine("script: {0}", "'" + EscapeSingleQuotes(tool.Script) + "'");
			}
			WritePath("outputdir", tool.OutputDir);
			WritePath("intermediatedir", tool.IntermediateDir);
			WriteFiles("files", tool.Files);
			WriteFiles("inputdependencies", tool.InputDependencies);
			WriteFiles("outputdependencies", tool.OutputDependencies);
			WriteList("createdirectories", tool.CreateDirectories);
			WritePath("workingdir", tool.WorkingDir);
			if (tool.Env != null)
			{
				_writer.WriteLine("environment:");
				Indented(() =>
				{
					foreach (var e in tool.Env)
					{
						_writer.WriteLine("{0}: {1}", e.Key, e.Value);
					}
				});
			}
		}

		private void WriteTool(CcCompiler cc, bool writefiles = true, FileItem file= null)
		{
			_writer.WriteLine("name: {0}", cc.ToolName);
			_writer.WriteLine("type: {0}", ToolTypeToString(cc));

			string optionset = file==null? null : file.OptionSetName;

			if(!String.IsNullOrEmpty(optionset))
			{
				_writer.WriteLine("build_optionset: {0}", optionset);
			}
			_writer.WriteLine("excludedfrombuild: {0}", cc.IsKindOf(BuildTool.ExcludedFromBuild));
			WritePath("executable", cc.Executable);
			_writer.WriteLine("precompiledheaders: {0}", cc.PrecompiledHeaders.ToString());
			WriteList("options", cc.Options);
			WriteList("defines", cc.Defines);
			WriteList("compilerinternaldefines", cc.CompilerInternalDefines);
			WriteList("includedirs", cc.IncludeDirs);
			WriteList("usingdirs", cc.UsingDirs);
			
			WriteFiles("assemblies", cc.Assemblies, nonEmptyOnly: true);
			WriteFiles("comassemblies", cc.ComAssemblies, nonEmptyOnly: true);

			if (writefiles)
			{
				_writer.WriteLine("sourcefiles:");
				Indented(() => WriteSources(cc.SourceFiles));
			}
		}

		private void WriteSources(FileSet sources)
		{
			foreach (var fi in sources.FileItems)
			{
				_writer.WriteLine("-");
				Indented(() => WriteSingleSource(fi));
			}
		}

		private void WriteSingleSource(FileItem fi)
		{
			_writer.WriteLine("src: {0}", fi.Path.Path);
			FileData filedata = fi.Data as FileData;
			if (filedata != null)
			{
				if (filedata.ObjectFile != null)
				{
					_writer.WriteLine("obj: {0}", filedata.ObjectFile.Path);
				}
				if (filedata.Tool != null)
				{
					_writer.WriteLine("tool:");
					Indented(() => WriteTool(filedata.Tool, writefiles: false, file: fi));
				}
			}
		}

		private void WriteTool( AsmCompiler asm, bool writefiles = true, FileItem file=null)
		{
			_writer.WriteLine("name: {0}", asm.ToolName);
			_writer.WriteLine("type: {0}", ToolTypeToString(asm));
			string optionset = file == null ? null : file.OptionSetName;

			if (!String.IsNullOrEmpty(optionset))
			{
				_writer.WriteLine("build_optionset: {0}", optionset);
			}

			_writer.WriteLine("excludedfrombuild: {0}", asm.IsKindOf(BuildTool.ExcludedFromBuild));
			WritePath("executable", asm.Executable);
			
			WriteList("options", asm.Options);
			WriteList("defines", asm.Defines);
			WriteList("includedirs", asm.IncludeDirs);

			if (writefiles)
			{
				_writer.WriteLine("sourcefiles:");
				Indented(() => WriteSources(asm.SourceFiles));
			}
		}

		private void WriteTool( Linker link, bool writefiles = true)
		{
			_writer.WriteLine("name: {0}", link.ToolName);
			_writer.WriteLine("type: {0}", ToolTypeToString(link));
			_writer.WriteLine("excludedfrombuild: {0}", link.IsKindOf(BuildTool.ExcludedFromBuild));
			WritePath("executable", link.Executable);

			WritePath("outputdir", link.LinkOutputDir);
			_writer.WriteLine("outputname: {0}", link.OutputName);
			_writer.WriteLine("outputextension: {0}", link.OutputExtension);
			WritePath("importlibrary", link.ImportLibFullPath, nonEmptyOnly : true);
			_writer.WriteLine("uselibrarydependencyinputs: {0}", link.UseLibraryDependencyInputs);
			
			WriteList("options", link.Options);

			WriteList("librarydirs", link.LibraryDirs);
			WriteFiles("libraries", link.Libraries);
			WriteFiles("assemblies", link.DynamicLibraries);
			_writer.WriteLine("# additional to compiler output obj files, aka libs");
			WriteFiles("objectfiles", link.ObjectFiles);
		}

		private void WriteTool( Librarian lib, bool writefiles = true)
		{
			_writer.WriteLine("name: {0}", lib.ToolName);
			_writer.WriteLine("type: {0}", ToolTypeToString(lib));
			_writer.WriteLine("excludedfrombuild: {0}", lib.IsKindOf(BuildTool.ExcludedFromBuild));
			WritePath("executable", lib.Executable);

			_writer.WriteLine("outputname: {0}", lib.OutputName);
			_writer.WriteLine("outputextension: {0}", lib.OutputExtension);

			WriteList("options", lib.Options);
			_writer.WriteLine("# additional to compiler output obj files");
			WriteFiles("objectfiles", lib.ObjectFiles);
		}

		private void WriteTool(DotNetCompiler dotnet, bool writefiles = true)
		{
			_writer.WriteLine("name: {0}", dotnet.ToolName);
			_writer.WriteLine("type: {0}", ToolTypeToString(dotnet));
			_writer.WriteLine("excludedfrombuild: {0}", dotnet.IsKindOf(BuildTool.ExcludedFromBuild));
			WritePath("executable", dotnet.Executable);

			_writer.WriteLine("target: {0}", dotnet.Target);
			WriteList("options", dotnet.Options);
			WriteList("defines", dotnet.Defines);
			WritePath("docfile", dotnet.DocFile, nonEmptyOnly:true);
			_writer.WriteLine("generatedocs: {0}", dotnet.GenerateDocs);

			if (writefiles)
			{
				WriteFiles("sourcefiles", dotnet.SourceFiles);
				WriteFiles("assemblies", dotnet.Assemblies);
				WriteFiles("dependentassemblies", dotnet.DependentAssemblies);
				WriteFiles("resources", dotnet.Resources);
				WriteFiles("nonembeddedresources", dotnet.NonEmbeddedResources);
				WriteFiles("contentfiles", dotnet.ContentFiles);
				WriteFiles("modules", dotnet.Modules);
				WriteFiles("comassemblies", dotnet.ComAssemblies);
			}
		}

		private void WriteTool(AppPackageTool apppackage, bool writefiles = true)
		{
			_writer.WriteLine("apppackagetool: WRITETOOL NOT IMPLEMENTED");
		}

		private void WriteToolGeneric(Tool tool, bool writefiles = true)
		{
			_writer.WriteLine("name: {0}", tool.ToolName);
			_writer.WriteLine("type: {0}", ToolTypeToString(tool));
			_writer.WriteLine("description: {0}", QuoteMapVal(tool.Description));
			_writer.WriteLine("excludedfrombuild: {0}", tool.IsKindOf(BuildTool.ExcludedFromBuild));
			if (tool.IsKindOf(BuildTool.Program))
			{
				WritePath("executable", tool.Executable);
				WriteList("options", tool.Options);
			}
			else
			{
				_writer.WriteLine("script: {0}", "'" + EscapeSingleQuotes(tool.Script) + "'"); 
			}

			WriteFiles("inputdependencies", tool.InputDependencies);
			WriteFiles("outputdependencies", tool.OutputDependencies);
			WriteList("createdirectories", tool.CreateDirectories);
			WritePath("workingdir", tool.WorkingDir);

			if (tool.Env != null)
			{
				_writer.WriteLine("environment:");
				Indented(() =>
				{
					foreach (var e in tool.Env)
					{
						_writer.WriteLine("{0}: {1}", e.Key, e.Value);
					}
				});
			}
		}

		private void WriteTool(PostLink postlink, bool writefiles = true)
		{
			WriteToolGeneric(postlink, writefiles);
		}

		private void WriteBuildSteps( IEnumerable<BuildStep> steps, uint mask)
		{
			if (steps != null)
			{
				foreach (BuildStep step in steps)
				{
					if (step.IsKindOf(mask))
					{
						_writer.WriteLine("-");
						Indented(() => WriteBuildStep(step));
					}
				}
			}
		}

		private void WriteBuildStep(BuildStep step)
		{
			_writer.WriteLine("name: {0}", step.Name);
			_writer.WriteLine("type: {0}", BuildStepTypeToString(step));

			WriteList("nant_targets", step.TargetCommands.Select(t => t.Target));
			_writer.WriteLine("commands:");
			Indented(() =>
			{
				foreach (var command in step.Commands)
				{
					_writer.WriteLine("-");
					Indented(() => WriteCommand(command));
				}
			});
			WriteList("inputdependencies", step.InputDependencies);
			WriteList("outputdependencies", step.OutputDependencies);
		}

		private void WriteCommand(Command command)
		{
			if(command.IsKindOf(Command.ExcludedFromBuild))
			{
				_writer.WriteLine("excludedfrombuild: true");
			}

			_writer.WriteLine("description: {0}", QuoteMapVal(command.Description));

			if(command.IsKindOf(Command.ShellScript))
			{
				_writer.WriteLine("type: ShellScript");
				WriteList("shell_lines", command.Script.LinesToArray());
			}
			if(command.IsKindOf(Command.Program))
			{
				WritePath("program", command.Executable);
				WriteList("options", command.Options);
			}
			WriteList("createdirectories", command.CreateDirectories);
			WritePath("workingdir", command.WorkingDir, nonEmptyOnly: true);
			if (command.Env != null)
			{
				_writer.WriteLine("environment:");
				Indented(() =>
				{
					foreach (var e in command.Env)
					{
						_writer.WriteLine("{0}: {1}", e.Key, e.Value);
					}
				});
			}
		}

		private string GetDependencyInfoString(Dependency<IModule> dependency)
		{
			var module = dependency.Dependent;
			var rhs = InlineMapping(
				new[] { "type", "package", "packageName", "packageVersion" },
				new[] { DependencyTypes.ToString(dependency.Type), module.Package.Name + "-" + module.Package.Version, module.Package.Name, module.Package.Version });
			return string.Format("{0}.{1}.{2}: {3}", module.BuildGroup, module.Package.Name, module.Name, rhs);
		}

		private void WriteList(string label, IEnumerable<string> values, bool nonEmptyOnly = false)
		{
			if (nonEmptyOnly && values.IsNullOrEmpty())
			{
				return;
			}
			_writer.WriteLine("{0}: ",label);
			if (!values.IsNullOrEmpty())
			{
				Indented(() => WriteMultiLineList(values));
			}
		}

		private void WriteList(string label, IEnumerable<PathString> values, bool nonEmptyOnly = false)
		{
			WriteList(label, values?.Select(p => p.Path));
		}

		private void WritePath( string label, PathString path, bool nonEmptyOnly = false)
		{
			string pathStr = path?.Path;
			if (nonEmptyOnly && pathStr.IsNullOrEmpty())
			{
				return;
			}
			_writer.WriteLine("{0}: {1}", label, pathStr == null ? "~" /* null designator in YAML */ : pathStr);
		}

		private void WriteFiles( string label, FileSet fs, bool nonEmptyOnly = false)
		{
			var fileItems = fs?.FileItems;
			bool isEmpty = fileItems.IsNullOrEmpty();
			if (nonEmptyOnly && isEmpty)
			{
				return;
			}
			_writer.WriteLine("{0}: ", label);
			if (!isEmpty)
			{
				var listItems = fileItems.Select(fi =>
				{
					string typeStr = ((fi.Data as FileData)?.Type)?.ToString();
					//************************************************************************************************************************************************
					// There are helper extensions to test type mask that can be associated with FileItem:
					//
					// uint flags = (fileitem.IsKindOf(FileData.Header | FileData.Resource) || module.IsKindOf(Module.MakeStyle)) ? 0 : FileEntry.ExcludedFromBuild; 
					//
					//************************************************************************************************************************************************
					if (String.IsNullOrWhiteSpace(typeStr))
					{
						return fi.Path.Path;
					}
					else
					{
						return InlineMapping(new[] { "type", "path" }, new[] { typeStr, fi.Path.Path });
					}
				});
				Indented(() => WriteMultiLineList(listItems));
			}
		}

		private string GetModuleFileName(IModule module)
		{
			return String.Format("{0}-{1}-{2}.{3}.txt", module.Package.Name, module.Package.Version, module.BuildGroup, module.Name);
		}

		private string ModuleTypeToString(Module module)
		{
			var names = GetBitNames(module.Type,
				new[] { Module.Native, Module.DotNet, Module.Utility, Module.MakeStyle, Module.Managed, Module.CSharp, Module.Program, Module.Library, Module.DynamicLibrary },
				new[] { "Native", "DotNet", "Utility", "MakeStyle", "Managed", "CSharp", "Program", "Library", "DynamicLibrary" });
			return InlineList(names);
		}

		private string ToolTypeToString(Tool tool)
		{
			var names = GetBitNames(tool.Type,
				new[] { Tool.TypeBuild, Tool.TypePreCompile, Tool.TypePreLink, Tool.TypePostLink },
				new[] {     "TypeBuild",    "TypePreCompile",    "TypePreLink",    "TypePostLink" });
			return InlineList(names);
		}

		private string BuildStepTypeToString(BuildStep step)
		{
			var names = GetBitNames(step.Type,
				new[] { BuildStep.PreBuild, BuildStep.PreLink, BuildStep.PostBuild, BuildStep.Build, BuildStep.Clean, BuildStep.ReBuild, BuildStep.ExecuteAlways },
				new[] {          "PreBuild",         "PreLink",         "PostBuild",         "Build",         "Clean",         "ReBuild",         "ExecuteAlways" });
			return InlineList(names);
		}

		private void PushIndent()
		{
			_writer.Indent += 1;
		}
		private void PopIndent()
		{
			_writer.Indent -= 1;
		}
		private void Indented(Action f)
		{
			_writer.Indent += 1;
			f();
			_writer.Indent -= 1;
		}

		static string InlineList(IEnumerable<object> xs)
		{
			if (xs == null)
			{
				return "~";
			}
			if (!xs.Any())
			{
				return "[]";
			}
			StringBuilder sb = new StringBuilder("[ ");
			string separator = "";
			foreach (var x in xs)
			{
				sb.Append(separator);
				sb.Append(x.ToString());
				separator = ", ";
			}
			sb.Append(" ]");
			return sb.ToString();
		}

		static string EscapeSingleQuotes(string v)
		{
			return v.Replace("'", "''");
		}

		static string QuoteMapVal(string v)
		{
			if (v.Contains(':'))
				return "'" + EscapeSingleQuotes(v) + "'";
			else
				return v;
		}
		static string QuoteListVal(string v)
		{
			if (v.StartsWith("\"") || v.StartsWith("%") || v.StartsWith("@") || v.StartsWith("&"))
			{
				return "'" + v + "'";
			}
			return v;
		}
		static string InlineMapping(IEnumerable<string> xs, IEnumerable<string> ys)
		{
			if (xs == null || ys == null)
			{
				return "~";
			}
			var pairs = xs.Zip(ys, (x,y) => Tuple.Create(x,y));
			if (pairs.Any())
			{
				StringBuilder sb = new StringBuilder("{ ");
				string separator = "";
				foreach(var xy in pairs)
				{
					sb.AppendFormat("{0}{1}: {2}", separator, xy.Item1, QuoteMapVal(xy.Item2));
					separator = ", ";
				}
				sb.Append(" }");
				return sb.ToString();
			}
			else
			{
				return "{}";
			}
		}
		private static IEnumerable<string> GetBitNames(uint bits, uint[] masks, string[] names)
		{
			return names.Where((name, i) => (bits & masks[i]) != 0);
		}
	}
}
