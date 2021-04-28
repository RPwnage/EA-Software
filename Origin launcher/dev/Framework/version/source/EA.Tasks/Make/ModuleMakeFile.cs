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
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Writers;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

using EA.Make.MakeItems;
using EA.Make.MakeTools;

namespace EA.Make
{
	public class ModuleMakeFile : MakeModuleBase, IModuleProcessor
	{
		protected readonly ProcessableModule Module;

		private MakeUniqueStringCollection PHONY;
		private MakeUniqueStringCollection SUFFIXES;
		
		private MakeTarget  BUILD;
		private MakeFragment MacroDefinitions;


		private static string DEFAULT_EMPTY_RECIPY_SUFFEXES = ".H .h .hpp .hxx  .inc .tup .d";


		public ModuleMakeFile(IModule module, PathModeType pathmode, MakeToolWrappers toolWrapers) :  base(module.Package.Project.Log, pathmode, toolWrapers)
		{
			Module = module as ProcessableModule;
		}

		public void GenerateMakeFile()
		{
			MakeFile.HEADER = "  --- Generated makefile for " + Module.ModuleIdentityString();

			//var top_targets = Sections.GetOrAddSection("top_targets", "Top targets");


			MakeFile.LINE = "";

			SUFFIXES = MakeFile.SUFFIXES;
			PHONY = MakeFile.PHONY;

			PHONY += "build clean rebuild lint lintheaders lintsource lintclean".ToArray();

			MakeFile.LINE = "";

			var defaulttarget = MakeFile.Target(Module.Name + "_DefaultTarget");
			PHONY += defaulttarget.Target;

			var env = Sections.GetOrAddSection("environment_settings", "Environment settings" );

			SetMakeEnvironmentVariables(env);

			MacroDefinitions =  Sections.GetOrAddSection("rules_section", "Rules");

			var tools = Sections.GetOrAddSection("tool_programs", "Tools and Settings");

			tools.Variable("MAKEFILE", ToMakeFilePath(Path.Combine(MakeFileDir, MakeFileName)));
			tools.Variable("MKDIR", ToMakeFilePath(Module.Package.Project.GetPropertyValue("nant.mkdir")));
			tools.Variable("RMDIR", ToMakeFilePath(Module.Package.Project.GetPropertyValue("nant.rmdir")));
			tools.Variable("BUILD_EXECUTED", "false");

			Sections.GetOrAddSection("tool_options", "Options");

			BUILD = MakeFile.Target("build");
			defaulttarget.Prerequisites += BUILD.Target;

			MakeFile.LINE = "";

			EmptyRecipes.Suffixes.Add(String.Empty);

			foreach(var s in DEFAULT_EMPTY_RECIPY_SUFFEXES.ToArray())
			{
				EmptyRecipes.Suffixes.Add(s);
			}

			Module.Apply(this);

			MacroDefinitions.Items.Add(EmptyRecipes);
		}


		private void SetMakeEnvironmentVariables(MakeFragment env)
		{
			string path = String.Empty;

			Dictionary<string, string> envKeyOriginalCasing = new Dictionary<string, string>(comparer: PlatformUtil.IsWindows ? StringComparer.OrdinalIgnoreCase : StringComparer.Ordinal);
			foreach (DictionaryEntry entry in Environment.GetEnvironmentVariables().Cast<DictionaryEntry>())
			{
				envKeyOriginalCasing[(string)entry.Key] = (string)entry.Key;
			}

			foreach (Property property in Module.Package.Project.Properties)
			{
				if (property.Prefix == "build.env")
				{
					var suffix = property.Suffix;
					if (suffix.Equals("path", StringComparison.OrdinalIgnoreCase))
					{
						path = property.Value;
					}
					else
					{
						env.ENVIRONMENT.Export(envKeyOriginalCasing.GetValueOrDefault(property.Suffix, property.Suffix), property.Value);
					}
				}
			}
			//Some native programs can set up PATH env variable with different cases. .Net does not like the mix.

			env.ENVIRONMENT.Export("PATH", path + String.Format("{0}$(Path){0}$(PATH)", PlatformUtil.IsWindows ? ";" : ":"));
			env.ENVIRONMENT.UnExport("Path");
		}

		protected override string MakeFileDir
		{
			get
			{
				return Module.IntermediateDir.Path;
			}
		}

		protected override string PackageBuildDir
		{
			get
			{
				return Module.Package.PackageBuildDir.Path;
			}
		}


		protected override string MakeFileName
		{
			get
			{
				return Module.MakeFileName();
			}
		}


		public void Process(Module_Native module)
		{
			SUFFIXES += ".c .cpp .cxx .cc .s .asm".ToArray();

			MakeTarget output_target = null;

			var prebuild = ExecuteSteps(module.BuildSteps, BuildStep.PreBuild, "prebuild", null);
			output_target = prebuild;

			var custombuild = ExecuteBuildTools(module.Tools, prebuild);

			if (custombuild != null)
			{
				AddPrerequisites(custombuild, output_target);

				output_target = custombuild;
			}

			var copylocal = new MakeCopyLocalTarget(module, this);

			// Copy local DLLs (before link?)
			copylocal.AddCopyLocalDependents(module.CopyLocalUseHardLink);

			// TODO make file copy local clean up
			// -dvaliant 2016/05/31
			/* the following code block can probably be removed, determination of copy local files is handled cetnrally by CopyLocalProcessor */
			if (module.Cc != null)
			{
				// CopyLocal slim are set through optionset
				copylocal.AddCopyLocal(module.Cc.Assemblies, CopyLocalType.True == module.CopyLocal, module.CopyLocalUseHardLink);
				copylocal.AddCopyLocal(module.Cc.ComAssemblies, CopyLocalType.True == module.CopyLocal, module.CopyLocalUseHardLink);
			}
			copylocal.GenerateTargets();
			if (copylocal.CopyLocalTarget != null)
			{
				// This target name isn't an output file, so need to add to PHONY list.  Also, we need to make sure this target name is
				// listed in PHONY list so that the AddPrerequisites() function will know that it needs to be added to the OrderOnlyPrerequisites section.
				// Failure to do so will cause incremental build keep doing unnecessary rebuild.
				PHONY += copylocal.CopyLocalTarget.Target;
			}
			
			var compile = new MakeCompileTarget(this, module);

			compile.GenerateTargets();

			var asmCompile = new MakeCompileAsmTarget(this, module);

			asmCompile.GenerateTargets();


			if (compile.CcTarget != null || asmCompile.AsmTarget != null)
			{
				if (output_target != null)
				{
					AddPrerequisites(compile.CcTarget, output_target);

					AddPrerequisites(asmCompile.AsmTarget, output_target);

					output_target = null;
				}

				if(copylocal.CopyLocalTarget != null)
				{
					if (compile.CcTarget != null)
					{
						AddPrerequisites(compile.CcTarget, copylocal.CopyLocalTarget);
					}

					if (asmCompile.AsmTarget != null)
					{
						AddPrerequisites(asmCompile.AsmTarget, copylocal.CopyLocalTarget);
					}
				}
			}

			if (module.Lib != null)
			{
				bool noObjsForLib =
					(compile.CcTarget == null || compile.CcTarget.Target == "$(PCHFILE)") &&
					asmCompile.AsmTarget == null &&
					module.Package.Project.Properties.GetBooleanPropertyOrDefault(
						module.Package.Project.Properties["config-system"] + ".shared-pch-generate-lib",
						true) == false;

				if (noObjsForLib)
				{
					BUILD.Prerequisites += compile.CcTarget.Target;

					AddPrerequisites(compile.CcTarget, output_target);

					output_target = compile.CcTarget;
				}
				else
				{
					MakeLibTarget lib = new MakeLibTarget(this, module);

					lib.GenerateTargets(compile, asmCompile);

					BUILD.Prerequisites += lib.LibTarget.Target;

					AddPrerequisites(lib.LibTarget, output_target);

					output_target = lib.LibTarget;
				}
			}
			else if (module.Link != null)
			{

			  var link = new MakeLinkTarget(this, module);

				link.GenerateTargets(compile, asmCompile);

				AddPrerequisites(link.LinkTarget, output_target);

				if (copylocal.CopyLocalTarget != null)
				{
					AddPrerequisites(link.LinkTarget, copylocal.CopyLocalTarget);
				}

				output_target = link.LinkTarget;

				if(module.PostLink != null)
				{
					output_target = ExecutePostLinkStep(module.PostLink, link.LinkTarget);
				}

				BUILD.Prerequisites += output_target.Target;
			}

			//ExecuteNAntBuildCopyTarget(module);

			var postbuild = ExecuteSteps(module.BuildSteps, BuildStep.PostBuild, "postbuild", output_target);
			if (postbuild != null)
			{
				if (module.GetModuleBooleanPropertyValue("postbuild.skip-if-up-to-date", false))
				{
					postbuild.WrapRulesWithCondition("\"$(BUILD_EXECUTED)\" == \"true\"");
				}

				AddPrerequisites(postbuild, output_target);

				BUILD.Prerequisites += postbuild.Target;
			}

			AddCleanTarget();
			AddRebuildTarget();
		}

		public void Process(Module_DotNet module)
		{
			throw new BuildException("MakeFile generation for Module type 'Module_DotNet' is not yet supported: can't generate makefile for: " + module.ModuleIdentityString());
		}

		public void Process(Module_Utility module)
		{
			MakeTarget output_target = null;

			var prebuild = ExecuteSteps(module.BuildSteps, BuildStep.PreBuild, "prebuild", null);
			output_target = prebuild;

			var custombuild = ExecuteBuildTools(module.Tools, prebuild);

			if (custombuild != null)
			{
				AddPrerequisites(custombuild, output_target);

				output_target = custombuild;
			}

			var prelink = ExecuteSteps(module.BuildSteps, BuildStep.PreLink, "prelink", output_target);

			if (prelink != null)
			{
				AddPrerequisites(prelink, output_target);

				output_target = prelink;
			}

			var buildstep = ExecuteSteps(module.BuildSteps, BuildStep.Build, "buildstep", output_target);

			if (buildstep != null)
			{
				AddPrerequisites(buildstep, output_target);

				output_target = buildstep;
			}

			var postbuild = ExecuteSteps(module.BuildSteps, BuildStep.PostBuild, "postbuild", output_target);

			if (postbuild != null)
			{
				AddPrerequisites(postbuild, output_target);

				output_target = postbuild;
			}

			var copylocal = new MakeCopyLocalTarget(module, this);
			copylocal.AddCopyLocalDependents(false);
			copylocal.GenerateTargets();
			if (copylocal.CopyLocalTarget != null)
			{
				PHONY += copylocal.CopyLocalTarget.Target;
			}

			if (copylocal.CopyLocalTarget != null)
			{
				AddPrerequisites(copylocal.CopyLocalTarget, output_target);

				output_target = copylocal.CopyLocalTarget;

			}
			if (output_target != null)
			{
				AddPrerequisites(BUILD, output_target);
			}

			var fileCopyStep = new MakeFileCopyStepTarget(module, this);
			fileCopyStep.GenerateTargets();

			if (fileCopyStep.FileCopyStepsTarget != null)
			{
				PHONY += fileCopyStep.FileCopyStepsTarget.Target;
				AddPrerequisites(fileCopyStep.FileCopyStepsTarget, output_target);
				output_target = fileCopyStep.FileCopyStepsTarget;
			}

			if (output_target != null)
			{
				AddPrerequisites(BUILD, output_target);
			}

			AddCleanTarget();
			AddRebuildTarget();
		}

		public void Process(Module_MakeStyle module)
		{
			var makebuild = ExecuteSteps(module.BuildSteps, BuildStep.Build, "makebuild", null);

			AddPrerequisites(BUILD, makebuild);

			var makeclean = ExecuteSteps(module.BuildSteps, BuildStep.Clean, "makeclean", null);

			var cleanTarget = MakeFile.Target("clean");
			PHONY += cleanTarget.Target;

			AddPrerequisites(cleanTarget, makeclean);

			var makerebuild = ExecuteSteps(module.BuildSteps, BuildStep.ReBuild, "makerebuild", null);

			var rebuildTarget = MakeFile.Target("rebuild");
			PHONY += rebuildTarget.Target;

			AddPrerequisites(rebuildTarget, makerebuild);
		}

		public void Process(Module_ExternalVisualStudio module)
		{
			throw new BuildException("MakeFile generation for Module type 'Module_ExternalVisualStudio' is not supported: can't generate makefile for: " + module.ModuleIdentityString());
		}

		public void Process(Module_UseDependency module)
		{

		}

		public void Process(Module_Python module)
		{
			throw new BuildException("MakeFile generation for Module type 'Module_Python' is not supported: can't generate makefile for: " + module.ModuleIdentityString());
		}

		public void Process(Module_Java module)
		{
			throw new BuildException("MakeFile generation for Module type 'Module_Java' is not supported: can't generate makefile for: " + module.ModuleIdentityString());
		}

		private MakeTarget ExecutePostLinkStep(PostLink postlink, MakeTarget prerequisite)
		{
			
			var postlinkstep = new MakePostLinkTarget(this, Module, postlink);

			postlinkstep.GenerateTarget();

			AddPrerequisites(postlinkstep.StepTargets.First(), prerequisite);

			return postlinkstep.StepTargets.First();
		}


		private MakeTarget ExecuteSteps(IEnumerable<BuildStep> steps, uint mask, string stepname, MakeTarget prerequisite)
		{
			MakeTarget target = null; 
			if (steps != null)
			{
				var selected = steps.Where(s => s.IsKindOf(mask));

				if(selected.Any(s=> (s.InputDependencies != null && s.InputDependencies.Count > 0)))
				{
					target = new MakeTarget(stepname);

					var targetnames = new Dictionary<string, int>();
					targetnames.Add(target.Label, 0);

					int nonEmptyCommands = 0;

					foreach(var step in selected)
					{
						var buildstep = new MakeBuildStepTarget(this, Module, step);

						int commandsNo = buildstep.GenerateTargets(stepname, targetnames);

						if (commandsNo > 0)
						{
							AddPrerequisites(target, buildstep.StepTargets.First().Label);
							AddPrerequisites(buildstep.StepTargets.First(), prerequisite);
						}

						nonEmptyCommands += commandsNo;
					}

					if(nonEmptyCommands > 0)
					{
						MakeFile.Items.Add(target);
						MakeFile.PHONY += target.Target;
					}
					else
					{
						target = null;
					}
				}
				else if (!selected.IsNullOrEmpty())
				{
					target = new MakeTarget(stepname);

					int nonEmptyCommands = 0;

					foreach(var step in selected)
					{
						var buildstep = new MakeBuildStepTarget(this, Module, step);

						nonEmptyCommands += buildstep.GenerateTargets(target);
					}
					if (nonEmptyCommands > 0)
					{
						MakeFile.Items.Add(target);
						MakeFile.PHONY += target.Target;
					}
					else
					{
						target = null;
					}

				}
			}

			return target;
		}

		private MakeTarget ExecuteBuildTools(IEnumerable<Tool> tools, MakeTarget prerequisite)
		{
			const string default_target_name = "custom-build";
			MakeTarget target = null;
			if (tools != null)
			{
				var targetnames = new Dictionary<string, int>();

				target = new MakeTarget(default_target_name);
				targetnames.Add(target.Label, 0);

				foreach (var tool in tools)
				{
					var buildtool = tool as BuildTool;

					if (buildtool != null && buildtool.IsKindOf(Tool.TypeBuild | Tool.TypePreCompile) && !buildtool.IsKindOf(Tool.ExcludedFromBuild))
					{
						var buildstep = new MakeBuildToolTarget(this, Module, buildtool);

						buildstep.GenerateTargets(default_target_name, targetnames);

						target.Prerequisites += buildstep.StepTargets.First().Label;

						AddPrerequisites(buildstep.StepTargets.First(), prerequisite);
					}
				}

				if(target.Prerequisites.Count + target.OrderOnlyPrerequisites.Count > 0)
				{
					MakeFile.Items.Add(target);
					PHONY += target.Target;
				}
				else
				{
					target = null;
				}
			}

			return target;
		}

		private MakeTarget AddCleanTarget()
		{
			var cleanTarget = MakeFile.Target("clean");
			PHONY += cleanTarget.Target;

			//var rspfile = WriteCleanResponseFile();
			//var toolPrograms = Sections["tool_programs"];
			//var delvar = toolPrograms.Variable("DEL", Module.Package.Project.GetPropertyValue("nant.delete"));

			cleanTarget.AddRuleCommentLine("[{0}] Clean", Module.Name);
			foreach (var path in FilesToClean.OrderBy(p => p))
			{
				cleanTarget.AddRuleLine(path.DelFile());
			}
			return cleanTarget;
		}

		private MakeTarget AddRebuildTarget()
		{
			var rebuildTarget = MakeFile.Target("rebuild");

			rebuildTarget.AddRuleLine("$(MAKE) $(MAKEFLAGS) -f$(MAKEFILE) clean");
			rebuildTarget.AddRuleLine("$(MAKE) $(MAKEFLAGS) -f$(MAKEFILE) build");

			return rebuildTarget;
		}


		private PathString WriteCleanResponseFile()
		{
			var rspfile = PathString.MakeCombinedAndNormalized(Module.IntermediateDir.Path, "clean_target.rsp");
			using (var writer = new MakeWriter())
			{
				writer.FileName = rspfile.Path;
				writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnCleanResponseFileUpdate);
				foreach (var path in FilesToClean.OrderBy(p => p))
				{
					writer.WriteLine(path);
				}
			}
			return rspfile;
		}

		protected void OnCleanResponseFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} clean target response file for '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Module.Name);
				if (e.IsUpdatingFile)
					Module.Package.Project.Log.Minimal.WriteLine(message);
				else
					Module.Package.Project.Log.Status.WriteLine(message);
			}
		}
	}


}
