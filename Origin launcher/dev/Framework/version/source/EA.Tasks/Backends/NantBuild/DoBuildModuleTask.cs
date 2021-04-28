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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Writers;

using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;

namespace EA.Eaconfig.Build
{
	[TaskName("nant-build-module")]
	public class DoBuildModuleTask : Task, IBuildModuleTask, IModuleProcessor
	{
		public DoBuildModuleTask(ConcurrentDictionary<string,bool> copyLocalSync = null ) : base("nant-build-module")
		{
			CopyLocalSync = copyLocalSync;
		}

		public ProcessableModule Module { get; set; }

		// Property Not used in this context
		public Tool Tool
		{
			get { return null; }
			set { }
		}

		public OptionSet BuildTypeOptionSet 
		{
			get { return null; }
			set { }
		}

		protected override void ExecuteTask()
		{
			Directory.CreateDirectory(Module.OutputDir.Path);
			Directory.CreateDirectory(Module.IntermediateDir.Path);

			Module.Apply(this);
		}

		internal bool collectCompilerStatistic = false;
		internal bool generateDependencyOnly = false;
		 
		public void Process(Module_Native module)
		{
			HashSet<string> tempOptionsets = new HashSet<string>();
			try
			{
				ExecuteSteps(module.BuildSteps, BuildStep.PreBuild);

				foreach (var tool in module.Tools)
				{
					var buildtool = tool as BuildTool;
					if (buildtool != null && buildtool.IsKindOf(Tool.TypeBuild | Tool.TypePreCompile) && !buildtool.IsKindOf(Tool.ExcludedFromBuild))
					{
						Project.ExecuteBuildTool(buildtool);
					}
				}

				EA.CPlusPlusTasks.BuildTask build = new EA.CPlusPlusTasks.BuildTask();
				build.Project = Project;

				build.OptionSetName = "__private_build.buildtype.nativenant";

				tempOptionsets.Add(build.OptionSetName);

				module.ToOptionSet(Project, build.OptionSetName);

				Project.Properties["build.outputdir.override"] = module.OutputDir.Path;

				//IMTODO: specify intermediate and output dir!
				build.BuildDirectory = module.IntermediateDir.Normalize().Path;

				//IMTODO: Append causes another expansion of the same file set. Assign fileset instead!
				//-- CC ---
				if (module.Cc != null)
				{
					build.IncludeDirectories.Value = module.Cc.IncludeDirs.ToNewLineString();
					build.Defines.Value = module.Cc.Defines.ToNewLineString() + Environment.NewLine + "EA_NANT_BUILD=1";
					build.Sources.IncludeWithBaseDir(module.Cc.SourceFiles);
					build.Dependencies.IncludeWithBaseDir(module.Cc.InputDependencies);
					build.UsingDirectories.Value = module.Cc.UsingDirs.ToNewLineString();
					build.ForceUsingAssemblies.IncludeWithBaseDir(module.Cc.Assemblies);
					// Set custom options for each source file:
					foreach (var fileitem in build.Sources.FileItems)
					{
						if (!String.IsNullOrWhiteSpace(fileitem.OptionSetName))
						{
							var optionsetname = "___private.cc.build." + fileitem.OptionSetName;

							if (!tempOptionsets.Contains(optionsetname))
							{
								if ((fileitem.GetTool() as CcCompiler).ToOptionSet(module, Project, optionsetname))
								{
									tempOptionsets.Add(optionsetname);
								}
							}

							fileitem.OptionSetName = optionsetname;
						}
					}
				}
				//-- AS ---
				if (module.Asm != null)
				{
					build.AsmIncludePath.Value = module.Asm.IncludeDirs.ToNewLineString();
					build.AsmDefines.Value = module.Asm.Defines.ToNewLineString() + Environment.NewLine + "EA_NANT_BUILD=1";
					build.AsmSources.IncludeWithBaseDir(module.Asm.SourceFiles);
					build.Dependencies.IncludeWithBaseDir(module.Asm.InputDependencies);

					// Set custom options for each source file:
					foreach (var fileitem in build.AsmSources.FileItems)
					{
						if (!String.IsNullOrWhiteSpace(fileitem.OptionSetName))
						{
							var optionsetname = "___private.as.build." + fileitem.OptionSetName;

							if (!tempOptionsets.Contains(optionsetname))
							{
								if ((fileitem.GetTool() as AsmCompiler).ToOptionSet(module, Project, optionsetname))
								{
									tempOptionsets.Add(optionsetname);
								}
							}
							fileitem.OptionSetName = optionsetname;
						}
					}
				}

				if (module.Link != null)
				{
					if (module.Link.ImportLibFullPath != null && !String.IsNullOrWhiteSpace(module.Link.ImportLibFullPath.Path))
					{
						string importlibdir = Path.GetDirectoryName(module.Link.ImportLibFullPath.Path);
						if (!String.IsNullOrWhiteSpace(importlibdir))
						{
							Directory.CreateDirectory(importlibdir);
						}
					}

					build.BuildName = module.Link.OutputName;
					build.Objects.IncludeWithBaseDir(module.Link.ObjectFiles);
					build.Libraries.IncludeWithBaseDir(module.Link.Libraries);
					build.Dependencies.IncludeWithBaseDir(module.Link.InputDependencies);
					build.PrimaryOutputExtension = module.Link.OutputExtension;
				}
				else if (module.Lib != null)
				{
					build.BuildName = module.Lib.OutputName;
					build.Dependencies.IncludeWithBaseDir(module.Lib.InputDependencies);
					build.Objects.IncludeWithBaseDir(module.Lib.ObjectFiles);
					build.PrimaryOutputExtension = module.Lib.OutputExtension;
				}

				// copy all non-postbuild copy local files
				ExecuteCopyLocal(module, module.CopyLocalUseHardLink);

				build.CollectCompilationTime = collectCompilerStatistic;
				build.GenerateDependencyOnly = generateDependencyOnly;
				if (collectCompilerStatistic || generateDependencyOnly)
				{
					build.FailOnError = false;
				}

				build.Execute();

				ExecuteBuildCopy(module);

				if (build.BuildExecuted || !module.GetModuleBooleanPropertyValue("postbuild.skip-if-up-to-date", false))
				{
					ExecuteSteps(module.BuildSteps, BuildStep.PostBuild);
				}
			}
			finally
			{
				foreach(var tempset in tempOptionsets)
				{
					Project.NamedOptionSets.Remove(tempset);
				}
			}
		}

		public void Process(Module_DotNet module)
		{
			if (module.Configuration.System != "pc" && module.Configuration.System != "pc64" 
				&& module.Configuration.System != "unix" && module.Configuration.System != "unix64"
				&& module.Configuration.System != "osx")
			{
				return;
			}

			module.CheckForUnsupportedNugetFeatures();

			ExecuteSteps(module.BuildSteps, BuildStep.PreBuild);

			NAnt.DotNetTasks.CompilerBase compiler;
			if (module.Compiler is CscCompiler)
			{
				NAnt.DotNetTasks.CscTask csc = new NAnt.DotNetTasks.CscTask
				{
					Project = Project,
					useDebugProperty = false
				};
				compiler = csc;

				csc.Win32Icon = (module.Compiler as CscCompiler).Win32icon.Path;
				csc.Win32Manifest = (module.Compiler as CscCompiler).Win32manifest.Path;
				csc.TargetFrameworkFamily = module.TargetFrameworkFamily.ToString();
				csc.ReferenceAssemblyDirs = (module.Compiler as CscCompiler).ReferenceAssemblyDirs;
				csc.LanguageVersion = (module.Compiler as CscCompiler).LanguageVersion;

				csc.ArgSet.Arguments.AddRange(module.Compiler.Options);

				if (module.Compiler.GenerateDocs)
				{
					csc.Doc = module.Compiler.DocFile.Path;
				}
				csc.ArgSet.Arguments.Add(module.Compiler.AdditionalArguments);
			}
			else
			{
				throw new BuildException($"{LogPrefix} Unknown compiler.", Location);
			}
			
			compiler.Compiler = module.Compiler.Executable.Path;
			compiler.Resgen = Project.ExpandProperties(Project.Properties["build.resgen.program"]);

			compiler.OutputTarget = module.Compiler.Target.ToString().ToLowerInvariant();
			
			compiler.Output = Path.Combine(module.OutputDir.Path, module.Compiler.OutputName + module.Compiler.OutputExtension);
			compiler.Define = module.Compiler.Defines.ToString(" ") + " EA_NANT_BUILD";

			compiler.References.IncludeWithBaseDir(module.Compiler.DependentAssemblies);
			compiler.References.IncludeWithBaseDir(module.Compiler.Assemblies);

			compiler.Resources= module.Compiler.Resources;

			compiler.Modules = module.Compiler.Modules;
			compiler.Sources = module.Compiler.SourceFiles;

			if (module.Compiler.GenerateDocs)
			{
				Directory.CreateDirectory(Path.GetDirectoryName(module.Compiler.DocFile.Path));
			}

			// copy all non-postbuild copy local files
			ExecuteCopyLocal(module, module.CopyLocalUseHardLink);

			compiler.Execute();

			if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Core)
			{
				// If current module is a exe target, we should execute extra step to create the exe from dll build output
				if (compiler.OutputTarget == DotNetCompiler.Targets.Exe.ToString().ToLowerInvariant())
				{
					// run RunCreateAppHostMsbuildTask task from DotNetSdk package
					bool useWPF = Project.Properties.GetBooleanPropertyOrDefault(module.GroupName + ".usewpf", false);
					bool useWindowsForms = Project.Properties.GetBooleanPropertyOrDefault(module.GroupName + ".usewindowsforms", false);
					TaskUtil.ExecuteScriptTask
					(
						Project, "RunCreateAppHostMsbuildTask",
						parameters: new Dictionary<string, string>
						{
							{ "AppHostDestinationPath", Path.ChangeExtension(compiler.Output, ".exe") },
							{ "AppBinaryName", Path.GetFileName(compiler.Output) },
							{ "IntermediateAssembly", compiler.Output },
							{ "UseWindowsGraphicalUserInterface", (useWPF || useWindowsForms) ? "True" : "False" },
							{ "TargetFramework", module.TargetFrameworkVersion }
						}
					);

					// run CreateBasicRuntimeConfigsJson task from DotNetSdk package
					List<string> usedFrameworks = new List<string>() { "Microsoft.NETCore.App" };
					if ((module.Configuration.System == "pc" || module.Configuration.System == "pc64") && (useWPF || useWindowsForms))
					{
						usedFrameworks.Add("Microsoft.WindowsDesktop.App");
					}
					if (((uint)module.ProjectTypes & (uint)DotNetProjectTypes.WebApp) != 0)
					{
						usedFrameworks.Add("Microsoft.AspNetCore.App");
					}
					TaskUtil.ExecuteScriptTask
					(
						Project, "CreateBasicRuntimeConfigsJson",
						parameters: new Dictionary<string, string>
						{
							{ "UsedFrameworks", String.Join(";", usedFrameworks) },
							{ "OutputFile", Path.ChangeExtension(compiler.Output, ".runtimeconfig.json") }
						}
					);

					// generate .deps.json file
					{
						string runtimeTarget = module.Package.Project.GetPropertyOrFail("package.DotNetSdk.runtimetarget.core");
						IEnumerable<string> refDependencies = compiler.References.FileItems
							.Where(item => !item.AsIs)
							.Select(item => Path.GetFileName(item.FullPath))
							.OrderBy(fileName => fileName);

						JProperty targets = new JProperty
						(
							"targets",
							new JObject {
								{ runtimeTarget, new JObject(
									new JProperty[] {
										new JProperty($"{Module.Name}/1.0.0", new JObject {
											{ "dependencies", new JObject(
												refDependencies.Select(refDep => new JProperty($"{Path.GetFileNameWithoutExtension(refDep)}-runtime", "1.0.0")).ToArray()
											)},
											{ "runtime", new JObject {
												{ Path.GetFileName(module.PrimaryOutput()), new JObject() }
											}}
										})
									}
									.Concat
									(
										refDependencies.Select
										(
											refDep => new JProperty($"{Path.GetFileNameWithoutExtension(refDep)}-runtime/1.0.0", new JObject(
													new JProperty("runtime", new JObject {
													{ refDep, new JObject() }
												})
											))
										)
									)
								)}
							}
						);

						JProperty libraries = new JProperty
						(
							"libraries",
							new JObject {
								new JProperty[] {
									new JProperty($"{Module.Name}/1.0.0", new JObject {
										{ "type", "project" },
										{ "serviceable", false },
										{ "sha512", "" }
									})
								}
								.Concat
								(
									refDependencies.Select
									(
										refDep => new JProperty($"{Path.GetFileNameWithoutExtension(refDep)}-runtime/1.0.0", new JObject {
											{ "type", "project" },
											{ "serviceable", false },
											{ "sha512", "" }
										})
									)
								)
							}
						);

						JObject jObject = new JObject {
							{ "runtimeTarget", new JObject {
								{ "name", runtimeTarget }, 
								{ "signature", "" }
							}},
							{ "compilationOptions", new JObject() }, // visual studio doesn't fill this out, so we don't either
							targets,
							libraries
						};

						using (MakeWriter depsWriter = new MakeWriter()
						{
							FileName = Path.ChangeExtension(compiler.Output, ".deps.json")
						})
						using (StreamWriter streamWriter = new StreamWriter(depsWriter.Internal, Encoding.Default, bufferSize: 1024, leaveOpen: true))
						using (JsonTextWriter jsonTextWriter = new JsonTextWriter(streamWriter)
						{
							Formatting = Formatting.Indented
						})
						{
							depsWriter.CacheFlushed += (sender, e) =>
							{
								string message = string.Format("{0}{1} .NET Core dependencies file  '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
								if (e.IsUpdatingFile)
								{
									Log.Minimal.WriteLine(message);
								}
								else
								{
									Log.Status.WriteLine(message);
								}
							};
							jObject.WriteTo(jsonTextWriter);
						}				
					}
				}
			}

			if (compiler.CompileExecuted || !module.GetModuleBooleanPropertyValue("postbuild.skip-if-up-to-date", false))
			{
				ExecuteSteps(module.BuildSteps, BuildStep.PostBuild);
			}
		}

		public void Process(Module_Utility module)
		{
			ExecuteSteps(module.BuildSteps, BuildStep.PreBuild);

			foreach (var tool in module.Tools)
			{
				var buildtool = tool as BuildTool;
				if (buildtool != null && buildtool.IsKindOf(Tool.TypeBuild | Tool.TypePreCompile) && !buildtool.IsKindOf(Tool.ExcludedFromBuild))
				{
					Project.ExecuteBuildTool(buildtool);
				}
			}

			ExecuteSteps(module.BuildSteps, BuildStep.PreLink);

			ExecuteSteps(module.BuildSteps, BuildStep.Build);

			ExecuteSteps(module.BuildSteps, BuildStep.PostBuild);

			ExecuteCopyLocal(module, false);

			ExecuteFileCopyStep(module);
		}

		public void Process(Module_MakeStyle module)
		{
			ExecuteSteps(module.BuildSteps, BuildStep.Build);
			ExecuteCopyLocal(module, false);
		}

		public void Process(Module_Python module)
		{
			// python modules do not need to be built, so do nothing here
		}

		public void Process(Module_ExternalVisualStudio module)
		{
			throw new BuildException($"NAnt build for module type '{nameof(Module_ExternalVisualStudio)}' is not supported.");
		}

		public void Process(Module_Java module)
		{
			throw new BuildException($"NAnt build for module type '{nameof(Module_Java)}' is not supported.");
		}

		public void Process(Module_UseDependency module)
		{
			throw new BuildException($"Trying to process use dependency module. This is most likely a bug in {Name} task.");
		}

		protected void ExecuteTools(Module module, uint type)
		{
			foreach (Tool tool in module.Tools)
			{
				if (tool.IsKindOf(type))
				{
					Project.ExecuteBuildTool(tool);
				}
			}
		}

		private void ExecuteSteps(IEnumerable<BuildStep> steps, uint mask)
		{
			if (steps != null)
			{
				foreach (BuildStep step in steps)
				{
					if (step.IsKindOf(mask))
					{
						Project.ExecuteBuildStep(step, moduleName: Module.Name);
					}
				}
			}
		}

		private void ExecuteCopyLocal(Module module, bool useHardLinkIfPossible)
		{
			foreach (CopyLocalInfo copyLocalInfo in module.CopyLocalFiles)
			{
				if (copyLocalInfo.PostBuildCopyLocal)
				{
					continue; // skip postbuild copy as it serves no purpose in nant build
				}

				bool useHardLink = useHardLinkIfPossible ? PathUtil.TestAllowCopyWithHardlinkUsage(module.Package.Project, copyLocalInfo.AbsoluteSourcePath, copyLocalInfo.AbsoluteDestPath) : false;

				CopyTask task = new CopyTask();
				task.Project = Project;
				task.FailOnError = copyLocalInfo.NonCritical == false;
				task.SourceFile = copyLocalInfo.AbsoluteSourcePath;
				task.ToFile = copyLocalInfo.AbsoluteDestPath;
				task.HardLink = useHardLink;

				if (CopyLocalSync != null && CopyLocalSync.TryAdd(task.ToFile, true))
				{
					Log.Info.WriteLine(LogPrefix + "{0}: copylocal '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);
					task.Execute();
				}
			}
		}

		// This is implemented for Utility module only.  It is used for executing the "filecopystep" element.
		private void ExecuteFileCopyStep(Module_Utility module)
		{
			int copystepIndex=0;
			string propName = string.Format("filecopystep{0}",copystepIndex);
			string tofile = module.PropGroupValue(string.Format("{0}.tofile", propName), null);
			string todir = module.PropGroupValue(string.Format("{0}.todir", propName), null);
			
			while (tofile != null || todir != null)
			{
				if (tofile != null)
				{
					string srcFile = module.PropGroupValue(string.Format("{0}.file", propName), null);
					if (srcFile == null)
					{
						string modulePropName = module.PropGroupName(string.Format("{0}", propName));
						Log.Error.WriteLine("{0}.tofile property was defined but missing {0}.file property.", modulePropName);
						break;
					}

					CopyTask task = new CopyTask();
					task.Project = Project;
					task.SourceFile = srcFile;
					task.ToFile = tofile;
					task.Clobber = true;

					Log.Info.WriteLine(LogPrefix + "{0}: copy '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);
					task.Execute();
				}
				else if (todir != null)
				{
					FileSet srcFileset = module.PropGroupFileSet(string.Format("{0}.fileset", propName));
					string srcFile = module.PropGroupValue(string.Format("{0}.file", propName), null);
					if (srcFileset != null)
					{
						foreach (var item in srcFileset.FileItems)
						{
							var srcf = item.Path.NormalizedPath;
							string destFile = Path.GetFileName(srcf);

							CopyTask task = new CopyTask();
							task.Project = Project;
							task.SourceFile = srcf;
							task.ToFile = Path.Combine(todir, destFile);
							task.Clobber = true;

							Log.Info.WriteLine(LogPrefix + "{0}: copy '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);
							task.Execute();
						}
					}
					else if (srcFile != null)
					{
						string destFile = Path.GetFileName(srcFile);

						CopyTask task = new CopyTask();
						task.Project = Project;
						task.SourceFile = srcFile;
						task.ToFile = Path.Combine(todir, destFile);
						task.Clobber = true;

						Log.Info.WriteLine(LogPrefix + "{0}: copy '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);
						task.Execute();
					}
					else
					{
						string modulePropName = module.PropGroupName(string.Format("{0}", propName));
						Log.Error.WriteLine("{0}.todir property was defined but missing {0}.fileset or {0}.file.", modulePropName);
						break;
					}
				}
				else
				{
					break;
				}
				copystepIndex++;
				propName = string.Format("filecopystep{0}", copystepIndex);
				tofile = module.PropGroupValue(string.Format("{0}.tofile", propName), null);
				todir = module.PropGroupValue(string.Format("{0}.todir", propName), null);
			}
		}


		// This is implementation of a legacy build-copy target.
		// The default eaconfig implementation of this target is not used anymore,
		// But there may be still config specific or optionset specific implementations.
		private void ExecuteBuildCopy(Module module)
		{
			// Just for backward compatibility 
			var targetname = "build-copy-${build.buildtype}";
			if (Project.ExecuteTargetIfExists(targetname, force: true))
			{
				Log.Info.WriteLine(LogPrefix + "{0}: execute '{1}' target", module.ModuleIdentityString(), targetname);
				return;
			}

			// Execute PlatformSpecific target
			if (!module.IsKindOf(ProcessableModule.DotNet))
			{
				var targettype = "library";

				if (module.IsKindOf(ProcessableModule.Program))
				{
					targettype = "program";
				}
				else if (module.IsKindOf(ProcessableModule.DynamicLibrary))
				{
					targettype = "dynamiclibrary";
				}

				targetname = String.Format("build-copy-{0}-{1}", targettype, Project.Properties["config-platform-load-name"] ?? Project.Properties["config-platform"]);

				if (Project.ExecuteTargetIfExists(targetname, force: true))
				{
					Log.Info.WriteLine(LogPrefix + "{0}: execute '{1}' target", module.ModuleIdentityString(), targetname);
				}
			}

		}

		private readonly ConcurrentDictionary<string, bool> CopyLocalSync;
	}
}
