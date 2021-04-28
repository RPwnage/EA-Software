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
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Structured;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Backends;


namespace EA.Eaconfig.Build
{
	public static class Extensions
	{
		public static BuildType CreateBuildType(this Project project, string groupname)
		{
			string buildTypeName = project.Properties[groupname + ".buildtype"];
			if (buildTypeName == null)
			{
				throw new BuildException($"Property '{groupname + ".buildtype"}' is not set. Default 'Program' build type is no longer supported.", stackTrace: BuildException.StackTraceType.XmlOnly);
			}

			// Init common common properties required for the build:
			return GetModuleBaseType.Execute(project, buildTypeName);
		}

		public static void ExecuteProcessSteps(this Project project, ProcessableModule module, OptionSet buildTypeOptionSet, IEnumerable<string> steps, BufferedLog log)
		{
			Chrono timer = (log == null) ? null : new Chrono();
			int count = 0;

			if (log != null)
			{
				log.IndentLevel += 6;
			}

			foreach (string step in steps)
			{
				Task task = project.TaskFactory.CreateTask(step, project);
				if (task != null)
				{
					IBuildModuleTask bmt = task as IBuildModuleTask;
					if (bmt != null)
					{
						bmt.Module = module;
						bmt.BuildTypeOptionSet = buildTypeOptionSet;
					}

					task.Execute();

					if (log != null)
					{
						log.Info.WriteLine("task   '{0}'", step);
						count++;
					}

				}
				else
				{
					if (project.ExecuteTargetIfExists(step, force: true))
					{
						if (log != null)
						{
							log.Info.WriteLine("target '{0}'", step);
							count++;
						}
					}
					else
					{
						project.Log.Warning.WriteLine("No task or target that matches process step '{0}'. Package {1}-{2} ({3}) {4}", step, module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup + "." + module.Name);
					}
				}
			}

			if (log != null)
			{
				log.Info.WriteLine("Completed {0} step(s) {1}", count, timer.ToString());
				if (count > 0)
				{
					log.Flush();
				}
				log.IndentLevel -= 6;
			}

		}

		public static int ExecuteGlobalProjectSteps(this Project project, IEnumerable<string> steps, BufferedLog log)
		{
			Chrono timer = (log == null) ? null : new Chrono();
			int count = 0;

			if (log != null)
			{
				log.IndentLevel += 6;
			}

			foreach (string step in steps.OrderedDistinct())
			{
				var task = project.TaskFactory.CreateTask(step, project);
				if (task != null)
				{
					task.Execute();

					count++;
					if (log != null)
					{
						log.Info.WriteLine("task   '{0}'",  step);
					}
				}
				else
				{
					if (project.ExecuteTargetIfExists(step, force: true))
					{
						count++;
						if (log != null)
						{
							log.Info.WriteLine("target '{0}'", step);
						}
					}
					else
					{
							project.Log.Warning.WriteLine("Task or target target '{0}' not found. (note: only tasks loaded from assemblies using taskdef are supported, not ones created using <createtask>)", step);
					}
				}
			}

			if (log != null)
			{
				if (count > 0)
				{
					log.Info.WriteLine("Completed {0} step(s) {1}", count, timer.ToString());
					log.Flush();
				}

				log.IndentLevel -= 6;
			}
			return count;
		}

		public static void ExecuteBuildTool(this Project project, Tool tool, bool failonerror = true)
		{
			BuildTool buildtool = tool as BuildTool;
			if (buildtool != null)
			{
				project.ExecuteCommand(buildtool, failonerror, buildtool.ToolName);
			}
			else
			{
				project.Log.Error.WriteLine("Tool [{0}] can not be executed.  Tool type is '{1}', it must be 'BuildTool' to be actionable.", tool.ToolName, tool.GetType().FullName);
			}
		}

		public static Tool GetTool(this FileItem fileitem)
		{
			  FileData filedata = fileitem.Data as FileData;
			  return (filedata != null) ? filedata.Tool: null;
		}

		public static CopyLocalType GetCopyLocal(this FileItem fileitem, IModule module)
		{
			var copyLocal = CopyLocalType.Undefined;

			if (fileitem != null && !String.IsNullOrEmpty(fileitem.OptionSetName))
			{
				// Check if assembly has optionset attached.
				OptionSet options;
				if (module.Package.Project != null && module.Package.Project.NamedOptionSets.TryGetValue(fileitem.OptionSetName, out options))
				{
					var option = options.GetOptionOrDefault("copylocal", String.Empty);

					if (option.Equals("on", StringComparison.OrdinalIgnoreCase) || option.Equals("true", StringComparison.OrdinalIgnoreCase))
					{
						copyLocal = CopyLocalType.True;
					}
					else if (option.Equals("off", StringComparison.OrdinalIgnoreCase) || option.Equals("false", StringComparison.OrdinalIgnoreCase))
					{
						copyLocal = CopyLocalType.False;
					}
				}
				else if (fileitem.OptionSetName.Equals("copylocal", StringComparison.OrdinalIgnoreCase))
				{
					copyLocal = CopyLocalType.True;
				}
			}

			return copyLocal;
		}

		#region FileData Extensions
		// ---- FileData extensions -----------------------------------------------------

		public static FileSet SetFileDataFlags(this FileSet fs, uint flags)
		{
			if (fs != null)
			{
				foreach (var fsi in fs.Includes)
				{
					fsi.SetFileDataFlags(flags);
				}
			}
			return fs;
		}

		public static FileSetItem SetFileDataFlags(this FileSetItem fi, uint flags)
		{
			if (fi != null)
			{
				if (fi.Data == null)
				{
					fi.Data = new FileSetData(null, flags: flags);
				}
				else if (fi.Data is FileSetData)
				{
					(fi.Data as FileSetData).SetType(flags);
				}
			}
			return fi;
		}

		public static FileItem SetFileDataFlags(this FileItem fi, uint flags)
		{
			if (fi != null)
			{
				if (fi.Data == null)
				{
					fi.Data = new FileData(null, flags: flags);
				}
				else if (fi.Data is FileSetData)
				{
					(fi.Data as FileSetData).SetType(flags);
				}
			}
			return fi;
		}

		public static FileSetItem ClearFileDataFlags(this FileSetItem fi, uint flags)
		{
			if (fi != null)
			{
				if (fi.Data == null)
				{
					fi.Data = new FileSetData(null, flags: flags);
				}
				else if (fi.Data is FileSetData)
				{
					(fi.Data as FileSetData).ClearType(flags);
				}
			}
			return fi;
		}

		public static FileItem ClearFileDataFlags(this FileItem fi, uint flags)
		{
			if (fi != null)
			{
				if (fi.Data == null)
				{
					fi.Data = new FileData(null, flags: flags);
				}
				else if (fi.Data is FileSetData)
				{
					(fi.Data as FileSetData).ClearType(flags);
				}
			}
			return fi;
		}

		internal static void SetFileData(this FileItem fi, Tool tool, PathString objectfile = null, uint flags=0)
		{
			if (fi != null)
			{
				var olddata = fi.Data as FileSetData;
				uint _flags = flags;
				if (olddata != null)
				{
					_flags |= olddata.Type;
				}
				fi.Data = new FileData(tool, objectfile, _flags);
			}
		}

		internal static void SetFileData(this FileSetItem fi, Tool tool, uint flags = 0)
		{
			if (fi != null)
			{
				var olddata = fi.Data as FileSetData;
				uint _flags = flags;
				if (olddata != null)
				{
					_flags |= olddata.Type;
				}
				fi.Data = new FileSetData(tool, _flags);
			}
		}

		public static bool IsKindOf(this FileSetItem fi, uint flags)
		{
			if (fi != null)
			{
				var mask = fi.Data as BitMask;
				if (mask != null)
				{
					return mask.IsKindOf(flags);
				}
			}
			return false;
		}

		public static bool IsKindOf(this FileItem fi, uint flags)
		{
			if (fi != null)
			{
				var mask = fi.Data as BitMask;
				if (mask != null)
				{
					return mask.IsKindOf(flags);
				}
			}
			return false;
		}

		public static uint Flags(this FileItem fi)
		{
			uint flags = 0;
			if (fi != null)
			{
				var mask = fi.Data as BitMask;
				if (mask != null)
				{
					return flags = mask.Type;
				}
			}
			return flags;
		}
		#endregion

		#region Module extensions

		public static string LinkerOutputName(this IModule imodule)
		{
			Module module = imodule as Module;

			if (module != null && module.Tools != null)
			{
				Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

				if (link != null)
				{
					return link.OutputName + link.OutputExtension;
				}
			}

			return null;
		}


		public static PathString DynamicLibOrAssemblyOutput(this IModule imodule)
		{
			Module module = imodule as Module;

			if (module != null && module.Tools != null)
			{
				if (module.IsKindOf(Module.DynamicLibrary))
				{
					Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

					if (link != null)
					{
						return PathString.MakeNormalized(Path.Combine(link.LinkOutputDir.Path, link.OutputName + link.OutputExtension));
					}
				}
				if (module.IsKindOf(Module.DotNet))
				{
					var dotnetmodule = module as Module_DotNet;
					if (dotnetmodule != null && dotnetmodule.Compiler != null)
					{
						return PathString.MakeNormalized(Path.Combine(module.OutputDir.Path, dotnetmodule.Compiler.OutputName + dotnetmodule.Compiler.OutputExtension));
					}
				}
			}
			return null;
		}


		public static PathString LibraryOutput(this IModule imodule)
		{
			Module module = imodule as Module;

			if (module != null && module.Tools != null)
			{
				Librarian lib = module.Tools.SingleOrDefault(t => t.ToolName == "lib") as Librarian;

				if (lib != null)
				{
					return PathString.MakeNormalized(Path.Combine(module.OutputDir.Path, lib.OutputName + lib.OutputExtension));
				}

				Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

				if (link != null)
				{
					return link.ImportLibFullPath;
				}
			}

			return null;
		}

		public static string PackagingOutput(this IModule imodule, string outputType)
		{
			// DAVE-FUTURE-REFACTOR-TODO: this is a little disgusting right now but it works,
			// previously things have been set up to follow project output but with packaging 
			// extension - more explicit control and templating would be good in future
			// also VisualStudioFunctions is a misleading namespace - this should apply to xcode
			// etc as well
			if (imodule.Package.Project.TargetExists(imodule.Configuration.System + ".get-packaging-output") && imodule.IsKindOf(Module.Program))
			{
				imodule.Package.Project.Properties["eaconfig.build.group"] = imodule.BuildGroup.ToString();
				imodule.Package.Project.Properties["groupname"] = imodule.BuildGroup.ToString() + "." + imodule.Name;
				imodule.Package.Project.Properties["build.module"] = imodule.Name;
				imodule.Package.Project.Properties[imodule.Configuration.System + ".get-packaging-output.output-type"] = outputType;
				imodule.Package.Project.ExecuteTarget(imodule.Configuration.System + ".get-packaging-output", force: true);
				return imodule.Package.Project.Properties[imodule.Configuration.System + ".get-packaging-output.resolved-path"];
			}
			else if (imodule.IsKindOf(Module.Apk) || imodule.IsKindOf(Module.Program))
			{
				string packagingSuffix = imodule.Package.Project.Properties["packaging-suffix"];
				if (packagingSuffix != null)
				{
					string apkSuffix = imodule.Package.Project.Properties.GetPropertyOrDefault(imodule.PropGroupName("android-apk-suffix"), null);
					if (apkSuffix != null)
						packagingSuffix = apkSuffix + packagingSuffix;

					return Path.Combine(imodule.OutputDir.Path, imodule.Name + packagingSuffix);
				}
			}
			return imodule.PrimaryOutput();
		}

		public static string PrimaryOutput(this IModule imodule)
		{
			Module module = imodule as Module;

			if (module != null && module.Tools != null)
			{
				if (module.Tools.SingleOrDefault(t => t.ToolName == "link") is Linker link)
				{
					return Path.Combine(link.LinkOutputDir.Path, link.OutputName + link.OutputExtension);
				}

				if (module.Tools.SingleOrDefault(t => t.ToolName == "lib") is Librarian lib)
				{
					return Path.Combine(module.OutputDir.Path, lib.OutputName + lib.OutputExtension);
				}

				if (module is Module_DotNet dotNetMod && dotNetMod.Compiler != null)
				{
					return Path.Combine(module.OutputDir.Path, dotNetMod.Compiler.OutputName + dotNetMod.Compiler.OutputExtension);
				}
			}

			if (module.OutputDir == null)
			{
				return null;
			}

			return Path.Combine(module.OutputDir.Path, module.OutputName ?? module.Name);
		}

		// note, only returns linker-esque symbols files, lib symbols file are not represented anywhere currently
		public static string PrimaryDebugSymbolsOutput(this IModule module)
		{
			if (module.GetModuleOptions().Options.TryGetValue("linkoutputpdbname", out string linkOutputPdbName))
			{
				return MacroMap.Replace(linkOutputPdbName, module.Macros, additionalErrorContext: $". Trying to expand 'linkoutputpdbname' from module {module.Name} optionset.");
			}
			return null;
		}

		public static string PrimaryOutputName(this IModule imodule)
		{
			Module module = imodule as Module;

			if (module != null && module.Tools != null)
			{
				if (module.Tools.SingleOrDefault(t => t.ToolName == "link") is Linker link)
				{
					return link.OutputName;
				}

				if (module.Tools.SingleOrDefault(t => t.ToolName == "lib") is Librarian lib)
				{
					return lib.OutputName;
				}

				if (module is Module_DotNet dotNetMod && dotNetMod.Compiler != null)
				{
					return dotNetMod.Compiler.OutputName;
				}
			}

			return module.OutputName ?? module.Name;
		}

		public static void PrimaryOutput(this IModule imodule, out string outputdir, out string name, out string extension)
		{
			Module module = imodule as Module;

			if (module != null && module.Tools != null)
			{
				if (module.Tools.SingleOrDefault(t => t.ToolName == "link") is Linker link)
				{
					outputdir = link.LinkOutputDir.Path;
					name = link.OutputName;
					extension = link.OutputExtension;
					return;
				}

				if (module.Tools.SingleOrDefault(t => t.ToolName == "lib") is Librarian lib)
				{
					outputdir = module.OutputDir.Path;
					name = lib.OutputName;
					extension = lib.OutputExtension;
					return;
				}

				if (module is Module_DotNet dotNetMod && dotNetMod.Compiler != null)
				{
					outputdir = module.OutputDir.Path;
					name = dotNetMod.Compiler.OutputName;
					extension = dotNetMod.Compiler.OutputExtension;
					return;
				}
			}
			outputdir = module.OutputDir.Path;
			name = module.OutputName ?? module.Name;
			extension = String.Empty;
		}

		public static string ModuleIdentityString(this IModule imodule)
		{
			return imodule != null ? String.Format("{0}/{1}.{2} ({3})", imodule.Package.Name, imodule.BuildGroup, imodule.Name, imodule.Configuration.Name) : String.Empty;
		}

		public static string ReplaceDir(this string input, string template, string val)
		{
			if (String.IsNullOrEmpty(val))
			{
				input = input.Replace("\\" + template + "\\", "\\").Replace("/" + template + "/", "/");
			}

			return input.Replace(template, val);
		}

		// gets the intermediate folder
		public static PathString GetVSTmpFolder(this IModule module, string additionalFolder = "v")
		{
			if (module.Package.Project.Properties.Contains("visualstudio.use-versioned-vstmp"))
			{
				DoOnce.Execute(module.Package.Project, "visualstudio.use-versioned-vstmp", () =>
				{
					module.Package.Project.Log.Warning.WriteLine("Property 'visualsudio.use-versioned-vstmp' is deprecated and has no effect.");
				});
			}
			return PathString.MakeCombinedAndNormalized(module.IntermediateDir.Path, additionalFolder);
		}

		public static PathString GetVSTLogLocation(this IModule module)
		{
			return module.GetVSTmpFolder("v.tlog");
		}

		public static void CheckForUnsupportedNugetFeatures(this Module_DotNet module)
		{
			foreach (Module_UseDependency useDependency in module.Dependents.Select(dep => dep.Dependent).OfType<Module_UseDependency>()) // nuget packages have no modules and so are always use dependency types
			{
				CheckForUnsupportedNugetFeatures(module.Package.Project, useDependency.Package.Name);
			}
		}

		public static bool GenerateAutoBindingRedirects(this Module_DotNet module)
		{
			bool bindingRedirectsDefault = module.IsKindOf(Module.Program) || module.GetModuleBooleanPropertyValue(module.GetFrameworkPrefix() + ".unittest", false);
			return module.GetModuleBuildOptionSet().GetBooleanOptionOrDefault("autogeneratebindingredirects", bindingRedirectsDefault);
		}

		public static bool IsBulkBuild(this IModule module)
		{
			if (module.Package.Project == null)
			{
				return false;
			}

			bool bulkBuild = module.Package.Project.Properties.GetBooleanProperty(FrameworkProperties.BulkBuildPropertyName);
			string alwaysBulkBuilt = module.Package.Project.Properties[FrameworkProperties.BulkBuildAlwaysPropertyName];
			string alwaysLooseBuilt = module.Package.Project.Properties[FrameworkProperties.BulkBuildExcludedPropertyName];

			try
			{
				bulkBuild = bulkBuild || module.IsListed(alwaysBulkBuilt);
				bulkBuild = bulkBuild && !module.IsListed(alwaysLooseBuilt);
			}
			catch (BuildException e)
			{
				throw new BuildException("Cannot apply always-bulkbuild/bulkbuild-exclusion settings: " + e.Message);
			}

			return bulkBuild;
		}

		// return true if module is listed in modulesList where module list follows the Package, Package/Module, Package/group/Module
		// convention. Note that in contrast to most place where this is used Package implies ALL modules in the package rather than all runtime
		// modules - this is because this function is used with settings such as bulkbuild overrides where a whole Package shorthand syntax makes
		// more sense
		// DependencyDeclaration.cs comments cover the syntax in more detail
		public static bool IsListed(this IModule module, string modulesList)
		{
			DependencyDeclaration parsedList = new DependencyDeclaration(modulesList);
			DependencyDeclaration.Package package = parsedList.Packages.FirstOrDefault(p => p.Name == module.Package.Name);
			if (package != null)
			{
				if (package.FullDependency)
				{
					return true;
				}

				DependencyDeclaration.Group group = package.Groups.FirstOrDefault(g => g.Type == module.BuildGroup);
				if (group != null)
				{
					if (group.Modules.Contains(module.Name))
					{
						return true;
					}
				}
			}
			return false;
		}

		private static void CheckForUnsupportedNugetFeatures(Project project, string packageName)
		{
			List<string> fileSetNames = new List<string>()
			{
				String.Format("package.{0}.nuget-target-files", packageName),
				String.Format("package.{0}.nuget-prop-files", packageName)
			};

			if (!project.Properties.GetBooleanPropertyOrDefault(String.Format("package.{0}.suppress-nuget-install", packageName), false))
			{
				fileSetNames.Insert(0, String.Format("package.{0}.nuget-install-scripts", packageName));
			}

			if (!project.Properties.GetBooleanPropertyOrDefault(String.Format("package.{0}.suppress-nuget-init", packageName), false))
			{
				fileSetNames.Insert(0, String.Format("package.{0}.nuget-init-scripts", packageName));
			}

			foreach (string fsName in fileSetNames)
			{
				FileSet fs = null;
				if (project.NamedFileSets.TryGetValue(fsName, out fs))
				{
					if (fs.FileItems.Any())
					{
						throw new BuildException(String.Format("Build dependent on NuGet fileset '{0}'. This feature is dependent on Visual Studio. Please use Visual Studio build.", fsName));
					}
				}
			}

			string dependenciesProperty = project.Properties[String.Format("package.{0}.nuget-dependencies", packageName)] ?? String.Empty;
			foreach (string dependency in dependenciesProperty.ToArray()) // ToArray is framework extension that splits property strings on whitespace
			{
				CheckForUnsupportedNugetFeatures(project, dependency);
			}
		}
	}

	#endregion
}