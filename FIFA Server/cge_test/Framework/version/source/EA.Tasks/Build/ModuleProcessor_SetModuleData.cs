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
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Threading;
using NAnt.Core.Util;
using NAnt.NuGet;
using NAnt.Shared.Properties;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Structured;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;

namespace EA.Eaconfig.Build
{
	internal class ModuleProcessor_SetModuleData : ModuleProcessorBase, IModuleProcessor, IDisposable
	{
		internal ModuleProcessor_SetModuleData(ProcessableModule module, string logPrefix)
			: base(module, logPrefix)
		{
			ModuleOutputNameMapping = ModulePath.Private.ModuleOutputMapping(module.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString());
		}

		private readonly OptionSet ModuleOutputNameMapping = null;

		public void Process(ProcessableModule module)
		{
			BufferedLog log = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
			Chrono timer = (log == null) ? null : new Chrono();

			if(log != null && log.InfoEnabled)
			{
				log.Info.WriteLine(LogPrefix + "processing module {0} - {1}", module.GetType().Name, module.ModuleIdentityString());
			}

			module.IntermediateDir = GetIntermediateDir(module);
			module.GenerationDir = GetGenerationDir(module);
			module.OutputDir = module.IntermediateDir;

			module.Macros.Add("intermediatedir", module.IntermediateDir.Path);
			module.Macros.Add("gendir", module.GenerationDir.Path);
			module.Macros.Add("outputdir", module.OutputDir.Path);
			module.Macros.Add("output", Path.Combine(module.OutputDir.Path, module.OutputName));

			SetPublicData(module);

			module.Apply(this);

			SetModuleBuildSteps(module);

			foreach (Tool t in module.Tools)
			{
				SetToolTemplates(t);
			}

			if (log != null)
			{
				log.IndentLevel += 6;
				log.Info.WriteLine("tools:       {0}", module.Tools.ToString(", ", t=> t.ToolName.Quote()));
				if (module.BuildSteps != null && module.BuildSteps.Count > 0)
				{
					log.Info.WriteLine("build steps: {0}", module.BuildSteps.ToString(", ", s => s.Name.Quote()));
				}
				log.Info.WriteLine("completed    {0}", timer.ToString());
				log.Flush();
			}
		}

		public void Process(Module_Native module)
		{
			var headerfiles = AddModuleFiles(module, ".headerfiles", "${package.dir}/include/**/*.h", "${package.dir}/" + module.GroupSourceDir + "/**/*.h");
			headerfiles.SetFileDataFlags(FileData.Header);

			module.ExcludedFromBuild_Files.AppendWithBaseDir(headerfiles);

			module.UsingSharedPchModule = false;
			module.UseForcedIncludePchHeader = false;
			module.PrecompiledFile = new PathString(String.Empty);
			module.PrecompiledHeaderFile = String.Empty;
			module.PrecompiledHeaderDir = new PathString(String.Empty);

			module.SetCopyLocal(ConvertUtil.ToEnum<CopyLocalType>(PropGroupValue("copylocal", "false")));
			module.SetCopyLocalUseHardLink(GetUseHardLinkIfPossible(module));

			module.ImportMSBuildProjects = GetModulePropertyValue("importmsbuildprojects",
				Project.Properties["eaconfig.importmsbuildprojects"]);

			module.RootNamespace = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(".rootnamespace"), "");

			FileSet custombuildObjects = null;
			var custombuildsources = new FileSet();
			List<PathString> custombuildIncludeDirs = null;

			if (!String.IsNullOrEmpty(GetModulePropertyValue(".vcproj.custom-build-source").TrimWhiteSpace()))
			{
				var msg = String.Format("Support for old syntax to define custom build files through property {0} was removed. Use new syntax, see Framework Guide (Reference/Packages/build scripts/Native (C/C++) modules/Custom Build Steps on Individual Files)", PropGroupName(".vcproj.custom-build-source"));
				throw new BuildException(msg);
			}
			var custombuildfiles_temp = AddModuleFiles(module, ".custombuildfiles");

			if (custombuildfiles_temp != null)
			{
				custombuildObjects = new FileSet();
				custombuildIncludeDirs = new List<PathString>();

				var basedir = PathString.MakeNormalized(custombuildfiles_temp.BaseDirectory);

				foreach (var fileitem in custombuildfiles_temp.FileItems)
				{
					module.AddTool(CreateCustomBuildTools(module, fileitem, basedir, custombuildsources, custombuildObjects, custombuildIncludeDirs));
				}
			}

			module.CustomBuildRuleFiles = PropGroupFileSet("vcproj.custom-build-rules");
			Project.NamedOptionSets.TryGetValue(PropGroupName(".vcproj.custom-build-rules-options"), out module.CustomBuildRuleOptions);

			foreach (string buildtask in StringUtil.ToArray(OptionSetUtil.GetOptionOrFail(BuildOptionSet, "build.tasks")))
			{
				switch (buildtask)
				{
					case "asm":
						{
							FileSet asmsources = AddModuleFiles(module, ".asmsourcefiles");

							if (asmsources != null)
							{
								module.Asm = CreateAsmTool(BuildOptionSet, module);

								var commonroots = new CommonRoots<FileItem>(asmsources.FileItems.OrderBy(e => e.Path.Path), (fe) => 
								fe.Path.Path,
								new string[] { PackageMap.Instance.BuildRoot, module.Package.Dir.Path });

								var duplicateObjFiles = new HashSet<string>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

								module.Asm.SourceFiles.Append(asmsources);

								// Set file specific options:
								foreach (var fileitem in module.Asm.SourceFiles.FileItems)
								{
									Tool tool = null;
									if (!String.IsNullOrEmpty(fileitem.OptionSetName))
									{
										OptionSet fileOptions;
										if (Project.NamedOptionSets.TryGetValue(fileitem.OptionSetName, out fileOptions))
										{
											tool = CreateAsmTool(fileOptions, module);

											SetToolTemplates(tool, fileOptions);

											if (custombuildIncludeDirs != null)
											{
												(tool as AsmCompiler).IncludeDirs.AddUnique(custombuildIncludeDirs);
											}

											ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["preprocess"].ToArray());
											ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["postprocess"].ToArray());
										}
									}
									uint nameConflictFlag = 0;
									var objFile = GetObjectFile(fileitem, tool ?? module.Asm, module, commonroots, duplicateObjFiles, out nameConflictFlag);

									fileitem.SetFileData(tool, objFile, nameConflictFlag);
								}
								if (custombuildIncludeDirs != null)
								{
									module.Asm.IncludeDirs.AddUnique(custombuildIncludeDirs);
								}
							}
						}
						break;
					case "cc":
						{
							// Set up the module.PrecompiledHeader info first as the CreateCcTool() will use those info to setup module.Cc object.
							string usingSharedPchPropName = PropGroupName("using-shared-pch");
							bool usingSharedPch = module.Package.Project.Properties.GetBooleanPropertyOrDefault(usingSharedPchPropName, false);
							if (usingSharedPch)
							{
								module.UsingSharedPchModule = true;
							}

							string targetPlatformPathStyle = module.Package.Project.GetPropertyOrDefault(CPlusPlusTasks.BuildTask.PathStyleProperty, null);

							CcCompiler.PrecompiledHeadersAction pchAction = GetModulePchAction(BuildOptionSet, module);
							if (pchAction == CcCompiler.PrecompiledHeadersAction.NoPch)
							{
								if (usingSharedPch)
								{
									string pchHdrPropName = PropGroupName("pch-header-file");
									string pchHeader = module.Package.Project.Properties.GetPropertyOrDefault(pchHdrPropName, null);
									if (String.IsNullOrEmpty(pchHeader))
									{
										throw new BuildException(String.Format("[Build Script Error] Module {0} is setup to use precompiled header but property {1} is not defined!", module.Name, pchHdrPropName));
									}
									module.PrecompiledHeaderFile = PathNormalizer.Normalize(pchHeader.TrimWhiteSpace(), false);

									string useForcedIncPropName = PropGroupName("pch-use-forced-include");
									module.UseForcedIncludePchHeader = module.Package.Project.Properties.GetPropertyOrDefault(useForcedIncPropName, "false").ToLower() == "true";

									string pchHdrDirPropName = PropGroupName("pch-header-file-dir");
									string pchHdrDir = module.Package.Project.Properties.GetPropertyOrDefault(pchHdrDirPropName, null);
									if (String.IsNullOrEmpty(pchHdrDir) && module.UseForcedIncludePchHeader)
									{
										throw new BuildException(string.Format("[Build Script Error] Module {0} is setup with {1} set to true, but did not provide property {2}.", module.Name, useForcedIncPropName, pchHdrDirPropName));
									}
									pchHdrDir = pchHdrDir.TrimWhiteSpace();
									module.PrecompiledHeaderDir = new PathString(CPlusPlusTasks.BuildTask.LocalizePath(pchHdrDir, targetPlatformPathStyle));
								}
							}
							else // if (pchAction != CcCompiler.PrecompiledHeadersAction.NoPch)
							{
								// Note that if the "pch-file" setup contains %filereldir% info, we will replace that at a later stage below
								// after the "commonroots" info is setup and the compile output OBJ path is setup (which is based on the commonroots info).
								// The obj's output relative path isn't exactly relative path of source file to package root.  It is relative to the
								// commonroot of this project to avoid overly long path issue.
								module.PrecompiledFile = PathString.MakeNormalized
								(
									MacroMap.Replace
									(
										PropGroupValue("pch-file", "%intermediatedir%\\stdPCH.pch"),
										new MacroMap(module.Macros)
										{
											{ "filereldir", "%filereldir%" }
										}
									),
									PathString.PathParam.NormalizeOnly
								);

								if (!Path.IsPathRooted(module.PrecompiledFile.Path))
								{
									module.PrecompiledFile = PathString.MakeCombinedAndNormalized(module.IntermediateDir.Path, module.PrecompiledFile.Path, PathString.PathParam.NormalizeOnly);
								}

								// We need to make sure that PrecompiledFile is in a format for the target platform.  We will be using this directly for %pchfile% token replacement.
								module.PrecompiledFile = new PathString(CPlusPlusTasks.BuildTask.LocalizePath(module.PrecompiledFile.Path, targetPlatformPathStyle));

								bool isPortable = module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false);
								if (isPortable)
								{
									module.PrecompiledFile = new PathString(PathUtil.RelativePath(module.PrecompiledFile.Path, module.Package.PackageBuildDir.Path));
								}

								string pchHdrPropName = PropGroupName("pch-header-file");
								string pchHeader = module.Package.Project.Properties.GetPropertyOrDefault(pchHdrPropName, null);
								if (String.IsNullOrEmpty(pchHeader))
								{
									throw new BuildException(String.Format("[Build Script Error] Module {0} is setup to use precompiled header but property {1} is not defined!", module.Name, pchHdrPropName));
								}
								module.PrecompiledHeaderFile = PathNormalizer.Normalize(pchHeader.TrimWhiteSpace(), false);

								string useForcedIncPropName = PropGroupName("pch-use-forced-include");
								module.UseForcedIncludePchHeader = module.Package.Project.Properties.GetPropertyOrDefault(useForcedIncPropName, "false").ToLower() == "true";

								string pchHdrDirPropName = PropGroupName("pch-header-file-dir");
								string pchHdrDir = module.Package.Project.Properties.GetPropertyOrDefault(pchHdrDirPropName, null);
								if (String.IsNullOrEmpty(pchHdrDir) && module.UseForcedIncludePchHeader)
								{
									throw new BuildException(string.Format("[Build Script Error] Module {0} is setup with {1} set to true, but did not provide property {2}.", module.Name, useForcedIncPropName, pchHdrDirPropName));
								}
								pchHdrDir = pchHdrDir.TrimWhiteSpace();
								module.PrecompiledHeaderDir = new PathString(CPlusPlusTasks.BuildTask.LocalizePath(pchHdrDir, targetPlatformPathStyle));
							}

							module.Cc = CreateCcTool(BuildOptionSet, module, BuildOptionSet);

							SetDebuglevelObsolete(module, module.Cc);

							if (custombuildIncludeDirs != null)
							{
								module.Cc.IncludeDirs.AddUnique(custombuildIncludeDirs);
							}

							// load source files file set if it exists, if not use /**/*.cpp as default pattern but include it from
							// another fileset using IncludeWithBaseDir - this will enumerate the pattern and only add files if they
							// exist - this is important to downstream logic which will interpret user intention based on number of 
							// includes in the fileset - we want their to be 0 if user added none and default pattern matched none
							FileSet cc_sources = new FileSet();
							int added = AddModuleFiles(cc_sources, module, ".sourcefiles");
							if (added == 0)
							{
								FileSet defaultSources = new FileSet();
								defaultSources.Include(Path.Combine(module.Package.Dir.Path, module.GroupSourceDir) + "/**/*.cpp");
								cc_sources.IncludeWithBaseDir(defaultSources);
							}

							// some packages add header files into set of source file. Clean this up:
							cc_sources = RemoveHeadersFromSourceFiles(module, cc_sources);

							FileSet excludedSourcefiles;

							FileSet sources = SetupBulkbuild(module, module.Cc, cc_sources, custombuildsources, out excludedSourcefiles);

							var commonroots = new CommonRoots<FileItem>(sources.FileItems.OrderBy(e => e.Path.Path), (fe) =>
								fe.Path.Path,
								new string[] { PackageMap.Instance.BuildRoot, module.Package.Dir.Path });

							var duplicateObjFiles = new HashSet<string>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

							var tools = new Dictionary<string, Tool>();

							module.Cc.SourceFiles.FailOnMissingFile = cc_sources.FailOnMissingFile;
							module.Cc.SourceFiles.Append(sources);

							// Set file specific options:
							foreach (var fileitem in module.Cc.SourceFiles.FileItems)
							{
								Tool tool = null;
								bool isCreatePchFile = false;

								if (!String.IsNullOrEmpty(fileitem.OptionSetName))
								{
									OptionSet fileOptions = GetFileBuildOptionSet(fileitem);

									isCreatePchFile = false;

									if (fileOptions != null)
									{
										if (!tools.TryGetValue(fileitem.OptionSetName, out tool))
										{
											tool = CreateCcTool(fileOptions, module, BuildOptionSet);
											SetToolTemplates(tool, fileOptions);
											tools.Add(fileitem.OptionSetName, tool);

											if (custombuildIncludeDirs != null)
											{
												(tool as CcCompiler).IncludeDirs.AddUnique(custombuildIncludeDirs);
											}
										}

										isCreatePchFile = fileOptions.Options["create-pch"] == "on";
									}
								}

								uint nameConflictFlag = 0;
								var objFile = GetObjectFile(fileitem, tool ?? module.Cc, module, commonroots, duplicateObjFiles, out nameConflictFlag);
								int tokenIndex = module.PrecompiledFile.Path.IndexOf("%filereldir%");
								if (isCreatePchFile && tokenIndex >= 0)
								{
									string pchRootDir = tokenIndex > 0 ? module.PrecompiledFile.Path.Substring(0, tokenIndex).TrimEnd(new char[] { '\\', '/' }) : "";
									string objFilePath = Path.GetDirectoryName(objFile.Path).TrimEnd(new char[] { '\\', '/' });
									string relDir = PathUtil.RelativePath(PathString.MakeNormalized(objFilePath), PathString.MakeNormalized(pchRootDir));
									if (String.IsNullOrEmpty(relDir))
									{
										module.PrecompiledFile = new PathString(
											module.PrecompiledFile.Path.Replace("%filereldir%\\","").Replace("%filereldir%/", "")
											);
									}
									else
									{
										relDir = new PathString(CPlusPlusTasks.BuildTask.LocalizePath(relDir, targetPlatformPathStyle)).Path;
										module.PrecompiledFile = new PathString(
											module.PrecompiledFile.Path.Replace("%filereldir%", relDir)
											);
									}
								}

								if (isCreatePchFile 
									&& module.Configuration.System == "android"
									&& module.Package.Project.Properties.GetPropertyOrFail("package.android_config.build-system").StartsWith("msvs-android"))
								{
									/* MSVS uses PrecompiledHeaderFile as the path to the file to compile into the pch, so it needs to be a full path; see FixupCLCompileOptions in Microsoft.Cpp.Clang.targets */
									module.PrecompiledHeaderFile = fileitem.Path.NormalizedPath;
								}

								if (isCreatePchFile)
								{
									module.Macros.Add("pchfile", module.PrecompiledFile.Path);
									module.Macros.Add("pchheaderfile", module.PrecompiledHeaderFile.Quote());
								}

								fileitem.SetFileData(tool, objFile, nameConflictFlag);
							}

							if (module.PrecompiledFile != null && !String.IsNullOrEmpty(module.PrecompiledFile.Path) && !module.Macros.ContainsKey("pchfile"))
							{
								module.Macros.Add("pchfile", module.PrecompiledFile.Path);
							}
							if (!String.IsNullOrEmpty(module.PrecompiledHeaderFile) && !module.Macros.ContainsKey("pchheaderfile"))
							{
								module.Macros.Add("pchheaderfile", module.PrecompiledHeaderFile.Quote());
							}

							if (excludedSourcefiles != null)
							{
								var emptyCc = new CcCompiler(module.Cc.Executable, description: "#global_context_settings");

								var commonExcludedroots = new CommonRoots<FileItem>(excludedSourcefiles.FileItems.OrderBy(e => e.Path.Path), (fe) => fe.Path.Path, new string[] { PackageMap.Instance.BuildRoot, module.Package.Dir.Path });

								foreach (var fileitem in excludedSourcefiles.FileItems)
								{
									Tool tool = emptyCc;

									if (!String.IsNullOrEmpty(fileitem.OptionSetName))
									{
										var fileOptions = GetFileBuildOptionSet(fileitem);

										if (fileOptions != null)
										{
											if (!tools.TryGetValue(fileitem.OptionSetName, out tool))
											{
												tool = CreateCcTool(fileOptions, module, BuildOptionSet);
												SetToolTemplates(tool, fileOptions);
												tools.Add(fileitem.OptionSetName, tool);
											}
										}
									}

									uint nameConflictFlag = 0;
									var objFile = GetObjectFile(fileitem, tool, module, commonExcludedroots, duplicateObjFiles, out nameConflictFlag);

									fileitem.SetFileData(tool, objFile, nameConflictFlag);
								}
								module.ExcludedFromBuild_Sources.IncludeWithBaseDir(excludedSourcefiles);
							}
						}
						break;
					case "link":
						module.Link = CreateLinkTool(module);
						module.PostLink = CreatePostLinkTool(BuildOptionSet, module, module.Link);

						if (custombuildObjects != null)
						{
							module.Link.ObjectFiles.Include(custombuildObjects);
						}
						break;
					case "lib":
						module.Lib = CreateLibTool(module);

						if (custombuildObjects != null)
						{
							module.Lib.ObjectFiles.Include(custombuildObjects);
						}
						break;
					default:
						Log.Warning.WriteLine(LogPrefix + "Custom tasks can not be processed by Framework 2: BuildType='{0}', build task='{1}' [{2}].", module.BuildType.Name, buildtask, BuildOptionSet.Options["build.tasks"]);
						break;
				}
			}

			// bonus java?
			FileSet javasourcefiles = new FileSet();
			if (AddModuleFiles(javasourcefiles, module, ".javasourcefiles") > 0)
			{
				module.JavaSourceFiles = javasourcefiles;
			}

			ReplaceAllTemplates(module);

			module.ExcludedFromBuild_Files.IncludeWithBaseDir(PropGroupFileSet("vcproj.excludedbuildfiles"));

			module.ResourceFiles = AddModuleFiles(module, ".resourcefiles", "${package.dir}/" + module.GroupSourceDir + "/**/*.ico");

			SetupNatvisFiles(module);

			module.SnDbsRewriteRulesFiles.AppendIfNotNull(module.GetPublicFileSet(module, "sndbs-rewrite-rules-files"));

			//Set module types.
			if (module.Link != null)
			{
				if (!module.IsKindOf(Module.Program | Module.DynamicLibrary))
				{
					Log.Warning.WriteLine("package {0}-{1} module {2} [{3}] has buildtype {4} ({5}) with 'link' task but module kind is not set to 'Program' or 'DynamicLibrary'.", module.Package.Name, module.Package.Version, module.BuildGroup + "." + module.Name, module.Configuration.Name, BuildType.Name, BuildType.BaseName);
				}
			}
			else if (module.Lib != null)
			{
				if (!module.IsKindOf(Module.Library))
				{
					Log.Warning.WriteLine("package {0}-{1} module {2} [{3}] has buildtype {4} ({5}) with 'lib' task but module kind is not set to 'Library'.", module.Package.Name, module.Package.Version, module.BuildGroup + "." + module.Name, module.Configuration.Name, BuildType.Name, BuildType.BaseName);
				}
			}

			// We use a flag file to detect during incremental gensln if we are switching between
			// shared pch binary vs not.  If we detected a switch, we need to remove the pdb file and all
			// local *.obj to force a rebuild of this module.  Otherwise, the build can fail reporting
			// pdb was built using a different pch.  This is only needed for VC compiler as other compilers
			// don't have pdb files.
			string sharedPchPdbCopyFlagFile = Path.Combine(module.IntermediateDir.NormalizedPath, "HasSharedPchPdbCopy.flag");
			bool hasSharedPchPdbCopy = module.GetModuleBooleanPropertyValue("has-sharedpch-pdb-copy", false);
			// If module build setting just changed from using shared pch binary to not use shared pch binary (or vice versa)
			// under VC compiler, we need to delete old obj and pdb files to force a rebuild during incremental build.
			bool pdbCopyStateChanged = 
						(File.Exists(sharedPchPdbCopyFlagFile) && !hasSharedPchPdbCopy) ||
						(!File.Exists(sharedPchPdbCopyFlagFile) && hasSharedPchPdbCopy);
			if (pdbCopyStateChanged)
			{
				// Delete all obj files in intermediate dir
				DirectoryInfo dirInfo = new DirectoryInfo(module.IntermediateDir.NormalizedPath);
				if (dirInfo.Exists)
				{
					IEnumerable<FileInfo> allObjFiles = dirInfo.EnumerateFiles("*.obj", SearchOption.AllDirectories);
					foreach (FileInfo file in allObjFiles)
					{
						file.Delete();
					}
					// Delete the compiler pdb
					if (module.Lib != null)
					{
						string libOutput = module.LibraryOutput().Path;
						if (File.Exists(libOutput))
							File.Delete(libOutput);
						string pdbfile = Path.ChangeExtension(libOutput, ".pdb");
						if (File.Exists(pdbfile))
							File.Delete(pdbfile);
					}
					else if (module.Link != null)
					{
						string compilerPdb = Path.Combine(module.IntermediateDir.NormalizedPath, module.PrimaryOutputName() + ".pdb");
						if (File.Exists(compilerPdb))
							File.Delete(compilerPdb);
					}
				}
			}
			if (File.Exists(sharedPchPdbCopyFlagFile) && !hasSharedPchPdbCopy)
			{
				File.Delete(sharedPchPdbCopyFlagFile);
			}
			else if (!File.Exists(sharedPchPdbCopyFlagFile) && hasSharedPchPdbCopy)
			{
				if (!Directory.Exists(module.IntermediateDir.NormalizedPath))
				{
					Directory.CreateDirectory(module.IntermediateDir.NormalizedPath);
				}
				File.WriteAllText(sharedPchPdbCopyFlagFile, "");
			}

			// Create prebuild step to ensure all output directories exist as some tool will fail if they don't
			List<PathString> createdirs = new List<PathString>(
				new PathString[] {module.OutputDir, 
								  module.IntermediateDir, 
								  (module.Link != null) ? module.Link.LinkOutputDir : null, 
								  (module.Link != null) ? module.Link.ImportLibOutputDir : null} 
			);
			if (module.Asm != null && module.Asm.SourceFiles.FileItems.Any())
			{
				// Microsoft's asm compiler couldn't handle outputing obj to a folder that doesn't already exists and
				// we encounter situation where we have obj file path contain sub-folder from common intermediate dir.
				foreach (FileItem itm in module.Asm.SourceFiles.FileItems)
				{
					createdirs.Add(new PathString(Path.GetDirectoryName((itm.Data as FileData).ObjectFile.Path)));
				}
			}
			BuildStep prebuild = new BuildStep("prebuild", BuildStep.PreBuild);
			prebuild.Commands.Add(new Command(String.Empty, String.Empty, createdirectories: createdirs.Where(d => d != null).OrderedDistinct()));

			AddBuildStepToModule(module, prebuild);

			// For some modules on specific configs, we may want to build using makefiles under Visual Studio.  We set a flag to indicate
			// this if the following property exists.
			string slnForceMakeStyle = module.Package.Project.GetPropertyValue(
						String.Format("eaconfig.build.sln.{0}.force-makestyle", module.Configuration.System));
			if (!String.IsNullOrEmpty(slnForceMakeStyle))
			{
				if (slnForceMakeStyle.ToLowerInvariant() == "true")
				{
					module.SetType(Module.ForceMakeStyleForVcproj);
				}
			}

			if (module.IsKindOf(Module.Managed))
			{
				module.NuGetReferences = PropGroupFileSet("nugetreferences");
				if (module.NuGetReferences != null)
				{
					module.NuGetReferences.Append(PropGroupFileSet("additional-nuget-references"));
				}
				else
				{
					module.NuGetReferences = PropGroupFileSet("additional-nuget-references");
				}

				SetupGeneratedAssemblyInfo(module);

				ResolveNugetReferences(module, module.NuGetReferences);
			}
		}

		public void Process(Module_DotNet module)
		{
			ThrowIfBulkBuildPropertiesSet(module);

			module.SetCopyLocal(ConvertUtil.ToEnum<CopyLocalType>(PropGroupValue("copylocal", "false")));
			module.SetCopyLocalUseHardLink(GetUseHardLinkIfPossible(module));

			module.LanguageVersion = PropGroupValue("languageversion", null);

			module.RootNamespace = PropGroupValue(GetFrameworkPrefix(module) + ".rootnamespace", module.Name);

			module.ApplicationManifest = PropGroupValue(GetFrameworkPrefix(module) + ".application-manifest", PropGroupValue("application-manifest"));
			module.AppDesignerFolder = PathString.MakeNormalized(PropGroupValue(GetFrameworkPrefix(module) + ".appdesigner-folder", PropGroupValue("appdesigner-folder")), PathString.PathParam.NormalizeOnly);

			module.DisableVSHosting = module.GetModuleBooleanPropertyValue(GetFrameworkPrefix(module) + ".disablevshosting");

			foreach (DotNetProjectTypes type in Enum.GetValues(typeof(DotNetProjectTypes)))
			{
				if (module.GetModuleBooleanPropertyValue(GetFrameworkPrefix(module) + "." + type.ToString().ToLowerInvariant()))
				{
					module.ProjectTypes |= type;
				}
			}

			module.GenerateSerializationAssemblies = ConvertUtil.ToEnum<DotNetGenerateSerializationAssembliesTypes>(GetModulePropertyValue(".generateserializationassemblies", "None"));

			module.ImportMSBuildProjects = GetModulePropertyValue(GetFrameworkPrefix(module) + ".importmsbuildprojects", 
				Project.Properties["eaconfig." + GetFrameworkPrefix(module) + ".importmsbuildprojects"]);

			Project.NamedOptionSets.TryGetValue(PropGroupName(".webreferences"), out module.WebReferences);

			if (module.Package.Project.UseNugetResolution)
			{
				module.NuGetReferences = PropGroupFileSet("nugetreferences");
				if (module.NuGetReferences != null)
				{
					module.NuGetReferences.Append(PropGroupFileSet("additional-nuget-references"));
				}
				else
				{
					module.NuGetReferences = PropGroupFileSet("additional-nuget-references");
				}

				if (module.GetModuleBooleanPropertyValue(GetFrameworkPrefix(module) + ".unittest", false))
				{
					if (module.NuGetReferences == null)
					{
						module.NuGetReferences = new FileSet();
					}
					if (module.TargetFrameworkFamily != DotNetFrameworkFamilies.Framework)
					{
						module.NuGetReferences.Include("Microsoft.NET.Test.Sdk", asIs: true);
					}
					module.NuGetReferences.Include("MSTest.TestFramework", asIs: true);
					module.NuGetReferences.Include("MSTest.TestAdapter", asIs: true);
				}
			}

			module.RunPostBuildEventCondition = module.GetPlatformModuleProperty(GetFrameworkPrefix(module) + ".runpostbuildevent", "OnOutputUpdated");

			module.ExcludedFromBuild_Files.IncludeWithBaseDir(PropGroupFileSet(GetFrameworkPrefix(module) + ".excludedbuildfiles"));

			foreach (string buildtask in StringUtil.ToArray(OptionSetUtil.GetOptionOrFail(BuildOptionSet, "build.tasks")))
			{
				DotNetCompiler compiler = null;
				switch (buildtask)
				{
					case "csc":

						if(!module.IsKindOf(Module.CSharp))
						{
							Log.Warning.WriteLine(LogPrefix + "package {0} module {1} is declared as C# module, but does not have csc task in BuildType '{2}", module.Package.Name, module.Name, module.BuildType.Name);
						}
						CscCompiler csc = null;
						if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Framework)
						{
							if (PlatformUtil.IsUnix || PlatformUtil.IsOSX)
							{
								string comp = Project.Properties["package.mono.tools.compiler"];
								csc = new CscCompiler(new PathString(comp), ConvertUtil.ToEnum<DotNetCompiler.Targets>(GetOptionString("csc.target", BuildOptionSet.Options)));
							}
							else
							{
								// if defined DotNet.csc.exe will pick csc based on non-proxy settings - if it doesn't exist VisualStudio.csc.exe
								// refers to the csc installed by Visual Studio always
								string comp = Project.Properties["package.DotNet.csc.exe"] ?? Project.Properties["package.VisualStudio.csc.exe"] ?? throw new BuildException("Neither property 'package.DotNet.csc.exe' or 'package.VisualStudio.csc.exe' were set - unable to initialize .NET framework compiler.");
								if (comp == null)
								{
									// backwards compatibility to older VisualStudio package versions - this is technically incorrect but in most cases
									// we rely on other mechansims so set csc path anyway (i.e building through VisualStudio)
									string dotNetBinFolder = Project.Properties["package.DotNet.bindir"] ?? Project.Properties["package.DotNet.appdir"] ?? "";
									comp = Path.Combine(dotNetBinFolder, "csc.exe");
								}
								csc = new CscCompiler(PathString.MakeNormalized(comp, PathString.PathParam.NormalizeOnly),
																ConvertUtil.ToEnum<DotNetCompiler.Targets>(GetOptionString("csc.target", BuildOptionSet.Options)));
							}
						}
						else
						{
							InternalQuickDependency.Dependent(Project, "DotNetCoreSdk");
							string comp = Project.Properties["package.DotNetCoreSdk.dotnet.exe"] ?? throw new BuildException("Property 'package.DotNetCoreSdk.dotnet.exe' was not set - unable to initialize .NET Core compiler.");
							csc = new CscCompiler(PathString.MakeNormalized(comp, PathString.PathParam.NormalizeOnly),
															ConvertUtil.ToEnum<DotNetCompiler.Targets>(GetOptionString("csc.target", BuildOptionSet.Options)))
							{
								TargetFrameworkFamily = module.TargetFrameworkFamily
							};

							List<string> assemblyRefDirs = new List<string>();
							if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Standard)
							{
								assemblyRefDirs.Add(PathString.MakeNormalized(Project.Properties["package.DotNetCoreSdk.NETStandard.Library.ref.dir"]).Path);
							}
							else if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Core)
							{
								assemblyRefDirs.Add(PathString.MakeNormalized(Project.Properties["package.DotNetCoreSdk.Microsoft.NETCore.App.ref.dir"]).Path);
								if (((uint) module.ProjectTypes & (uint) DotNetProjectTypes.WebApp) != 0)
								{
									assemblyRefDirs.Add(PathString.MakeNormalized(Project.Properties["package.DotNetCoreSdk.Microsoft.AspNetCore.App.ref.dir"]).Path);
								}
								if (module.Configuration.System=="pc" || module.Configuration.System == "pc64")
								{
									// All WinForms, WPF, etc assemblies reside here.  But it is only available
									// for PC.
									bool useWPF = Project.Properties.GetBooleanPropertyOrDefault(module.GroupName + ".usewpf", false);
									bool useWindowsForms = Project.Properties.GetBooleanPropertyOrDefault(module.GroupName + ".usewindowsforms", false);
									if (useWindowsForms || useWPF)
									{
										assemblyRefDirs.Add(PathString.MakeNormalized(Project.Properties["package.DotNetCoreSdk.Microsoft.WindowsDesktop.App.ref.dir"]).Path);
									}
								}
							}
							if (assemblyRefDirs.Any())
							{
								csc.ReferenceAssemblyDirs = String.Join(",", assemblyRefDirs);
							}
						}

						module.Compiler = compiler = csc;

						// 1607 - Suppress Assembly generation - Referenced assembly 'System.Data.dll' targets a different processor 
						// NU1605 - Suppress Detected package downgrade. This will be revisited to have a more sophisticated approach where framework hard-set the right package versions
						// 1702 - Assuming assembly reference 'Xxx.xxx, Version=X.X.X.X, Culture=neutral, PublicKeyToken=xxxxxxxx' used by 'Yyy.Yyy' matches identity 'Xxx.Xxx, Version=Z.Z.Z.Z Culture=neutral, PublicKeyToken=xxxxxxxx' of 'Xxx.Xxx', you may need to supply runtime policy
						csc.Options.Add("/nowarn:1607,NU1605,1701,1702");
						csc.Win32icon = PathString.MakeNormalized(GetModulePropertyValue(".win32icon"));
						csc.Win32manifest = PathString.MakeNormalized(module.ApplicationManifest);
						csc.Modules.AppendWithBaseDir(AddModuleFiles(module, ".modules", "${package.dir}/" + module.GroupSourceDir + "/**/*.module"));
						csc.LanguageVersion = GetModulePropertyValue(".languageversion");
						break;
					case "fsc":

						if(!module.IsKindOf(Module.FSharp))
						{
							Log.Warning.WriteLine(LogPrefix + "package {0} module {1} is declared as F# module, but does not have fsc task in BuildType '{2}", module.Package.Name, module.Name, module.BuildType.Name);
						}

						string fscBinFolder = Project.Properties["package.DotNet.bindir"] ?? Project.Properties["package.DotNet.appdir"] ?? "";
						FscCompiler fsc = new FscCompiler(PathString.MakeNormalized(Path.Combine(fscBinFolder, "fsc.exe")),
														  ConvertUtil.ToEnum<DotNetCompiler.Targets>(GetOptionString("fsc.target", BuildOptionSet.Options)));

						module.Compiler = compiler = fsc;

						break;

					default:
						Log.Warning.WriteLine(LogPrefix + "Custom tasks can not be processed by Framework 2: BuildType='{0}', build task='{1}' [{2}].", module.BuildType.Name, buildtask, BuildOptionSet.Options["build.tasks"]);
						break;
				}

				if (compiler == null)
				{
					throw new BuildException($"{module.LogPrefix}Module does not have valid compiler task defined. Tasks: '{module.Tools.Select(tool => tool.ToolName).ToString(" ")}'.");
				}

				// ---- Add common csc/fsc settings ---

				string outputname;
				string replacename;
				string outputextension;
				PathString linkoutputdir;
				SetLinkOutput(module, out outputname, out outputextension, out linkoutputdir, out replacename); // Redefine output dir and output name based on linkoutputname property.
				compiler.OutputName = outputname;
				compiler.OutputExtension = outputextension;

				string optimization = GetOptionString("optimization", BuildOptionSet.Options, "optimization", useOptionProperty:false);
				string debugflags = GetOptionString("debugflags", BuildOptionSet.Options, "debugflags", useOptionProperty: false);
				string trace = GetOptionString("trace", BuildOptionSet.Options, "trace", useOptionProperty: false);
				string debugsymbols = GetOptionString("debugsymbols", BuildOptionSet.Options, "debugsymbols", useOptionProperty: false);
				string retailflags = GetOptionString("retailflags", BuildOptionSet.Options, "retailflags", useOptionProperty: false);

				if (optimization == "on" || optimization == "true") // DAVE-FUTURE-REFACTOR-TODO: be nice to clean up option handling across fw, esp with regard with true/false/on/off/custom standards - for the time being don't want to introduce any risk so only optimizations gets special true/false handling here
				{
					compiler.Options.Add("/optimize+");
				}
				else if (optimization == "off" || optimization == "false")
				{
					compiler.Options.Add("/optimize-");
				}

				if (debugsymbols == "on")
				{
					if (compiler.HasOption("debug") == false)
					{
						if (debugflags == "on" || debugflags == "custom")
						{
							compiler.Options.Add("/debug:full");
						}
						else
						{
							compiler.Options.Add("/debug:pdbonly");
						}
					}
				}

				bool nodefaultdefines = GetOptionString("nodefaultdefines", BuildOptionSet.Options, "nodefaultdefines", useOptionProperty: false).OptionToBoolean();
				if (nodefaultdefines == false)
				{
					compiler.Defines.Add("EA_DOTNET2");

					if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Core)
					{
						// trimwhitespace converts null to empty string
						string dotnetVersionDefineVS = Project.Properties["package.DotNetCoreSDK.versionDefineVS.core"].TrimWhiteSpace();
						if (!String.IsNullOrEmpty(dotnetVersionDefineVS))
						{
							compiler.Defines.Add(dotnetVersionDefineVS);
						}
						string dotnetVersionDefineEA = Project.Properties["package.DotNetCoreSDK.versionDefineEA.core"].TrimWhiteSpace();
						if (!String.IsNullOrEmpty(dotnetVersionDefineEA))
						{
							compiler.Defines.Add(dotnetVersionDefineEA);
						}
						compiler.Defines.Add("NETCOREAPP");
					}
					else if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Standard)
					{
						// trimwhitespace converts null to empty string
						string dotnetVersionDefineVS = Project.Properties["package.DotNetCoreSDK.versionDefineVS.standard"].TrimWhiteSpace();
						if (!String.IsNullOrEmpty(dotnetVersionDefineVS))
						{
							compiler.Defines.Add(dotnetVersionDefineVS);
						}
						string dotnetVersionDefineEA = Project.Properties["package.DotNetCoreSDK.versionDefineEA.standard"].TrimWhiteSpace();
						if (!String.IsNullOrEmpty(dotnetVersionDefineEA))
						{
							compiler.Defines.Add(dotnetVersionDefineEA);
						}
						compiler.Defines.Add("NETSTANDARD");
					}
					else // .net framework
					{
						// trimwhitespace converts null to empty string
						string dotnetVersionDefine = Project.Properties["package.DotNet.versionDefine"].TrimWhiteSpace(); 
						if (!String.IsNullOrEmpty(dotnetVersionDefine))
						{
							compiler.Defines.Add(dotnetVersionDefine);
						}
					}

					if (debugflags == "on" || debugflags == "custom")
					{
						compiler.Defines.Add("DEBUG");
					}

					if (String.IsNullOrEmpty(trace) || trace == "on")
					{
						compiler.Defines.Add("TRACE");
					}

					if (retailflags == "on")
					{
						compiler.Defines.Add("EA_RETAIL");
					}
				}

				compiler.Defines.AddRange(GetOptionString(compiler.ToolName + ".defines", BuildOptionSet.Options, ".defines").LinesToArray());
				compiler.Defines.AddRange(Project.Properties["global.dotnet.defines"].LinesToArray());
				compiler.Defines.AddRange(Project.Properties["global.dotnet.defines." + module.Package.Name].LinesToArray());

				compiler.SourceFiles.IncludeWithBaseDir(AddModuleFiles(module, ".sourcefiles"));
				compiler.Assemblies.AppendWithBaseDir(AddModuleFiles(module, ".assemblies"));
				compiler.Resources.IncludeWithBaseDir(AddModuleFiles(module, ".resourcefiles", "${package.dir}/" + module.GroupSourceDir + "/**/*.ico"));
				compiler.Resources.SetFileDataFlags(FileData.EmbeddedResource);
				compiler.NonEmbeddedResources.IncludeWithBaseDir(AddModuleFiles(module, ".resourcefiles.notembed"));

				compiler.Resources.BaseDirectory = GetModulePropertyValue(".resourcefiles.basedir", compiler.SourceFiles.BaseDirectory??compiler.Resources.BaseDirectory);
				compiler.Resources.Prefix = GetModulePropertyValue(".resourcefiles.prefix", module.Name);

				compiler.ContentFiles.IncludeWithBaseDir(AddModuleFiles(module, ".contentfiles"));
				compiler.ContentFiles.SetFileDataFlags(FileData.ContentFile);

				// TODO overiding optionset
				// -dvaliant 2014/04/05
				/* should deprecate .contentfiles.action - a property than can blank override a fileitem's optionset could be
				potentially confusing for user */
				var content_action = GetModulePropertyValue(".contentfiles.action", null).TrimWhiteSpace().ToLowerInvariant();
				if (!String.IsNullOrEmpty(content_action))
				{
					if (content_action == "copy-always" || content_action == "do-not-copy" || content_action == "copy-if-newer")
					{
						foreach (var fsItem in compiler.ContentFiles.Includes)
						{
							fsItem.OptionSet = content_action;
						}
					}
					else
					{
						Log.Warning.WriteLine("package {0} module {1}: invalid value of '{2}'= '{3}', valid values are: {4}.", module.Package.Name, module.Name, PropGroupValue(".contentfiles.action"), content_action, "copy-always, do-not-copy, copy-if-newer");
					}
				}

				// ---- options -----

				char[] OPTIONS_DELIM = new char[] { ' ', ';', ',', '\n', '\t' };

				if (!AddOptionOrPropertyValue(compiler, "warningsaserrors.list", "/warnaserror:", val => val.ToArray(OPTIONS_DELIM).ToString(",")))
				{
					// It doesn't make sense to turn on warnings with errors when there is a list of specific warnings defined
					// Therefore, if the list is not defined, only then do we decide to check the value of warningsaserrors
					if (AddOptionOrPropertyValue(compiler, "warningsaserrors", "/warnaserror"))
					{
						// it only makes sense to disable specific warningsaserrors if warningsaserrors is turned on
						AddOptionOrPropertyValue(compiler, "warningsnotaserrors.list", "/warnaserror-:", val => val.ToArray(OPTIONS_DELIM).ToString(","));
					}
				}

				AddOptionOrPropertyValue(compiler, "suppresswarnings", "/nowarn:", val => val.ToArray(OPTIONS_DELIM).ToString(","));

				AddOptionOrPropertyValue(compiler, "allowunsafe", "/unsafe", val => val.OptionToBoolean() ? "+" : "-");

				AddOptionOrPropertyValue(compiler, "keyfile", "/keyfile:", val => PathNormalizer.Normalize(val, false).Quote());

				AddOptionOrPropertyValue(compiler, "deterministic", "/deterministic");

				AddOptionOrPropertyValue(compiler, "platform", "/platform:", (val) => ProcessModuleDotNetPlatform(val, module, compiler));

				AddAdditionalOptions(compiler);

				// delete remove options
				string removeOptionKey = "remove." + compiler.ToolName + ".options";
				IList<string> removecscoptions = ((BuildOptionSet.Options[removeOptionKey] ?? String.Empty) + Environment.NewLine + GetModulePropertyValue(removeOptionKey)).LinesToArray();
				if (removecscoptions.Count > 0)
				{
					compiler.Options.RemoveAll(x => removecscoptions.Contains(x));
				}

				// A [output].config will be auto generated under Visual Studio if this file is listed in the project. and this
				// This .config file may be needed for proper execution of the assembly if people make the effort to create this file.
				if (compiler.Resources.FileItems.Any(f => Path.GetFileName(f.FileName).ToLower() == "app.config") ||
					compiler.NonEmbeddedResources.FileItems.Any(f => Path.GetFileName(f.FileName).ToLower() == "app.config") ||
					module.GenerateAutoBindingRedirects())
				{
					// It is potentially feasible for other C# modules to build depend on this C# program module and user added
					// the C# exe to assemblies fileset.  In which case, we may need to fix the CopyLocalProcessor as well.
					// For now, we're ignoring this as we don't really have dependent exe that generates a .config file.
					string appConfigOutputName = module.PrimaryOutput() + ".config";
					module.RuntimeDependencyFiles.Add(appConfigOutputName);
				}

				// If its an executable, add its pdb file to the runtime dependency files (ensures it comes through in the generated file manifest)
				if (compiler.Target == DotNetCompiler.Targets.Exe || compiler.Target == DotNetCompiler.Targets.WinExe)
				{
					string pdbName = Path.Combine(module.OutputDir.Path, module.OutputName + ".pdb");
					module.RuntimeDependencyFiles.Add(pdbName);
				}

				SetGenerateDocs(module, compiler);

				compiler.ApplicationIcon = PathString.MakeNormalized(GetModulePropertyValue(".win32icon"));

				SetupGeneratedAssemblyInfo(module);

				SetupComAssembliesAssemblies(compiler, module);

				SetupWebReferences(compiler, module);

				SetDebuglevelObsolete(module, compiler);

				SetupCustomBuildTools(module, compiler);
			}

			ResolveNugetReferences(module, module.NuGetReferences);
		}

		private void ResolveNugetReferences(Module module, FileSet nuGetReferences)
		{
			if (module.Package.Project.UseNugetResolution)
			{ 
				if (nuGetReferences != null)
				{
					NugetPackage[] flattenedContents;
					try
					{
						flattenedContents = PackageMap.Instance.GetNugetPackages(module.Package.Project.BuildGraph().GetTargetNetVersion(module.Package.Project), nuGetReferences.FileItems.Select(x => x.FileName)).ToArray();
					}
					catch (NugetException nugetEx)
					{
						throw new BuildException($"Error while resolving NuGet dependencies of module '{module.Name}'.", nugetEx);
					}
					foreach (NugetPackage packageContents in flattenedContents)
					{
						// need to add target support
						if (module is Module_DotNet mod)
						{
							foreach (FileInfo x in packageContents.ReferenceItems)
							{
								mod.Compiler.Assemblies.Include(new FileItem(new PathString(x.FullName, PathString.PathState.FullPath)));
							}
							/*foreach (FileInfo x in packageContents.TargetItems)
							{
								if (!string.IsNullOrEmpty(mod.ImportMSBuildProjects))
								{
									mod.ImportMSBuildProjects += "\n";
								}
								mod.ImportMSBuildProjects += x;
							}*/
						}
						if (module is Module_Native native)
						{
							foreach (var x in packageContents.ReferenceItems)
							{
								native.Cc.Assemblies.Include(new FileItem(new PathString(x.FullName, PathString.PathState.FullPath)));
							}
							/*foreach (var x in packageContents.TargetItems)
							{
								if (!string.IsNullOrEmpty(native.ImportMSBuildProjects))
									native.ImportMSBuildProjects += "\n";
								native.ImportMSBuildProjects += x;
							}*/
						}
					}
				}
			}
		}

		public void Process(Module_Utility module)
		{
			AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".asmsourcefiles");
			AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".sourcefiles");

			module.SetCopyLocal(ConvertUtil.ToEnum<CopyLocalType>(PropGroupValue("copylocal", "false")));
			module.SetCopyLocalUseHardLink(GetUseHardLinkIfPossible(module));

			var headerfiles = AddModuleFiles(module, ".headerfiles", "${package.dir}/include/**/*.h", "${package.dir}/" + module.GroupSourceDir + "/**/*.h");
			headerfiles.SetFileDataFlags(FileData.Header);
			module.ExcludedFromBuild_Files.AppendWithBaseDir(headerfiles);

			module.ExcludedFromBuild_Files.IncludeWithBaseDir(PropGroupFileSet("vcproj.excludedbuildfiles"));

			// Add custom build tools				
			var custombuildfiles = AddModuleFiles(module, ".custombuildfiles");
			if (custombuildfiles != null)
			{
				var basedir = PathString.MakeNormalized(custombuildfiles.BaseDirectory);

				foreach (var fileitem in custombuildfiles.FileItems)
				{
					module.AddTool(CreateCustomBuildTools(module, fileitem, basedir, module.ExcludedFromBuild_Sources));
				}
			}
		}

		public void Process(Module_MakeStyle module)
		{
			if (module.BuildSteps.Count > 0)
			{
				throw new BuildException($"{module.LogPrefix}MakesStyle module does not support build steps. {module.BuildSteps.Count} buildsteps were defined.");
			}

			// Initialize with system environment variables first.
			SortedDictionary<string, string> cmdEnvironment = new SortedDictionary<string, string>(Project.EnvironmentVariables);
			foreach (Property property in module.Package.Project.Properties)
			{
				// If we have any build environment override, re-assign or add those!
				if (property.Prefix == "build.env")
				{
					cmdEnvironment[property.Suffix] = property.Value;
				}
			}

			var command = PropGroupValue("MakeBuildCommand").TrimWhiteSpace();

			if (!String.IsNullOrEmpty(command))
			{
				var buildstep = new BuildStep("MakeStyle-Build", BuildStep.Build | BuildStep.ExecuteAlways);
				buildstep.Commands.Add(new Command(command, workingDir: PathString.MakeNormalized(Project.Properties["package.builddir"]), env: cmdEnvironment));
				module.BuildSteps.Add(buildstep);
			}

			command = PropGroupValue("MakeRebuildCommand").TrimWhiteSpace();

			if (!String.IsNullOrEmpty(command))
			{
				var buildstep = new BuildStep("MakeStyle-ReBuild", BuildStep.ReBuild | BuildStep.ExecuteAlways);
				buildstep.Commands.Add(new Command(command, workingDir: PathString.MakeNormalized(Project.Properties["package.builddir"]), env: cmdEnvironment));
				module.BuildSteps.Add(buildstep);
			}

			command = PropGroupValue("MakeCleanCommand").TrimWhiteSpace();

			if (!String.IsNullOrEmpty(command))
			{
				var buildstep = new BuildStep("MakeStyle-Clean", BuildStep.Clean | BuildStep.ExecuteAlways);
				buildstep.Commands.Add(new Command(command, workingDir: PathString.MakeNormalized(Project.Properties["package.builddir"]), env: cmdEnvironment));
				module.BuildSteps.Add(buildstep);
			}

			AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".asmsourcefiles");
			AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".sourcefiles");
			AddModuleFiles(module.ExcludedFromBuild_Sources, module, ".vcproj.excludedbuildfiles");

			var headerfiles = AddModuleFiles(module, ".headerfiles", "${package.dir}/include/**/*.h", "${package.dir}/" + module.GroupSourceDir + "/**/*.h");
			headerfiles.SetFileDataFlags(FileData.Header);
			module.ExcludedFromBuild_Files.AppendWithBaseDir(headerfiles);
		}

		public void Process(Module_Python module)
		{
			module.SetType(Module.Python);
			module.SearchPath = PropGroupValue("searchpaths", "");
			module.StartupFile = PropGroupValue("startupfile", "");
			module.WorkDir = PropGroupValue("workdir", ".");
			module.ProjectHome = PropGroupValue("projecthome", ".");
			module.IsWindowsApp = PropGroupValue("windowsapp", "false");
			module.Environment = new OptionSet();
			string envOptionSetName = PropGroupName("environment");
			if (module.Package.Project.NamedOptionSets.Contains(envOptionSetName))
			{
				module.Environment = module.Package.Project.NamedOptionSets[envOptionSetName];
			}

			// todo: only activepython is checked, should be updated for alternate python interpreters
			// note: python interpreter is currently only used to store file names
			string pythonexe = Project.Properties.GetPropertyOrDefault("package.ActivePython.exe", "");
			PythonInterpreter interpreter = new PythonInterpreter("python", 
				PathString.MakeNormalized(pythonexe, PathString.PathParam.NormalizeOnly));

			interpreter.SourceFiles.IncludeWithBaseDir(AddModuleFiles(module, ".sourcefiles"));
			interpreter.ContentFiles.IncludeWithBaseDir(AddModuleFiles(module, ".contentfiles"));
			module.Interpreter = interpreter;
		}

		public void Process(Module_ExternalVisualStudio module)
		{
			module.VisualStudioProject = PathString.MakeNormalized(PropGroupValue("project-file").TrimWhiteSpace());
			module.VisualStudioProjectConfig = PropGroupValue("project-config").TrimWhiteSpace();
			module.VisualStudioProjectPlatform = PropGroupValue("project-platform").TrimWhiteSpace();

			if (!Guid.TryParse(PropGroupValue("project-guid").TrimWhiteSpace(), out module.VisualStudioProjectGuild))
			{
				var msg = String.Format("Property '{0}={1}' can not be converted to guid", PropGroupName("project-guid"), PropGroupValue("project-guid").TrimWhiteSpace());
				throw new BuildException(msg);
			}

			var ext = Path.GetExtension(module.VisualStudioProject.Path).ToLowerInvariant();
			switch (ext)
			{
				case ".csproj":
					module.SetType(Module.DotNet | Module.CSharp);
					break;
				case ".fsproj":
					module.SetType(Module.DotNet | Module.FSharp);
					break;
				case ".vcxproj":
				default:
					if (Project.Properties.GetBooleanPropertyOrDefault(PropGroupName("project-managed"), false))
					{
						module.SetType(Module.Managed);
					}
					break;
			}

			if (module.IsKindOf(Module.DotNet))
			{
				foreach (DotNetProjectTypes type in Enum.GetValues(typeof(DotNetProjectTypes)))
				{
					if (module.GetModuleBooleanPropertyValue(type.ToString().ToLowerInvariant()))
					{
						module.DotNetProjectTypes |= type;
					}
				}
			}
		}

		public void Process(Module_Java module)
		{
			module.GradleDirectory = GetModulePropertyValue("gradle-directory");
			module.GradleProject = GetModulePropertyValue("gradle-project");

			if (module.IsKindOf(Module.Apk))
			{
				ModulePath binPath = ModulePath.Private.GetModuleBinPath(module, binarySuffix: ".apk");
				module.OutputDir = new PathString(binPath.OutputDir);
			}
			else if (module.IsKindOf(Module.Aar))
			{
				ModulePath libPath = ModulePath.Private.GetModuleLibPath(module, librarySuffix: ".aar");
				module.OutputDir = new PathString(libPath.OutputDir);
			}
			module.ExcludedFromBuild_Files.IncludeWithBaseDir(PropGroupFileSet("javasourcefiles"));
		}

		public void Process(Module_UseDependency module)
		{
			throw new BuildException($"Trying to process use dependency module. This is most likely a bug.");
		}

		#region all modules

		private bool GetUseHardLinkIfPossible(Module module)
		{
			string useHardLink = PropGroupValue("copylocal_usehardlinkifpossible", null);
			if (useHardLink == null)
				return module.Package.Project.Properties.GetBooleanPropertyOrDefault("nant.copylocal-default-use-hardlink", false);
			return useHardLink.ToLowerInvariant() == "true";
		}

		private void SetPublicData(ProcessableModule module)
		{
			int errors = 0;

#if NANT_CONCURRENT
			// Under mono most parallel execution is slower than consecutive. Until this is resolved use consecutive execution 
			bool parallel = (PlatformUtil.IsMonoRuntime == false) && (Project.NoParallelNant == false);
#else
			bool parallel = false;
#endif
			try
			{
				if(parallel)
				{
					Parallel.ForEach(module.Dependents, (d) =>
					{
						if (0 == Interlocked.CompareExchange(ref errors, 0, 0))
						{
							SetDependentPublicData(module, d);
						}
						else
						{
							return;
						}
					});
				}
				else
				{
					foreach (var d in module.Dependents)
					{
							if (0 == Interlocked.CompareExchange(ref errors, 0, 0))
							{
								SetDependentPublicData(module, d);
							}
							else
							{
								break;
							}
					}
				}
			}
			catch (System.Exception e)
			{
				if (1 == Interlocked.Increment(ref errors))
				{
					ThreadUtil.RethrowThreadingException(
						String.Format(LogPrefix + "Error processing dependents (ModuleProcessor.DoPackageDependents) of [{0}-{1}], module '{2}'", module.Package.Name, module.Package.Version, module.Name),
						Location.UnknownLocation, e);
				}
			}
		}

		private void SetDependentPublicData(ProcessableModule module, Dependency<IModule> d)
		{
			OptionSet outputmapOptionset = null;
			// Populate public include dirs, public libraries and assemblies.                    
			PublicData packagePublicData = new PublicData();

			string propertyname;
			outputmapOptionset = ModulePath.Private.ModuleOutputMapping(d.Dependent.Package.Project ?? module.Package.Project, d.Dependent.Package.Name, d.Dependent.Name, d.Dependent.BuildGroup.ToString());

			if (d.IsKindOf(DependencyTypes.Interface))
			{
				propertyname = "package." + d.Dependent.Package.Name + ".defines";
				packagePublicData.Defines = Project.Properties[propertyname].LinesToArray();

				propertyname = "package." + d.Dependent.Package.Name + ".includedirs";
				packagePublicData.IncludeDirectories = Project.Properties[propertyname].LinesToArray()
													.Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>();
				propertyname = "package." + d.Dependent.Package.Name + ".usingdirs";
				packagePublicData.UsingDirectories = MapDependentBuildOutputs.MapDependentOutputDir(Project,
													Project.Properties[propertyname].LinesToArray()
													.Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>(),
													outputmapOptionset, d.Dependent.Package.Name, "link");
			}
			if (d.IsKindOf(DependencyTypes.Link) || (d.Dependent is Module_Native)) // To account for transitive propagation
			{
				propertyname = "package." + d.Dependent.Package.Name + ".libdirs";
				packagePublicData.LibraryDirectories = MapDependentBuildOutputs.MapDependentOutputDir(Project,
													Project.Properties[propertyname].LinesToArray()
													.Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>(),
													outputmapOptionset, d.Dependent.Package.Name, "lib");

				packagePublicData.Libraries = MapDependentBuildOutputs.RemapModuleLibraryPaths(d.Dependent as ProcessableModule, module, Project.GetFileSet("package." + d.Dependent.Package.Name + ".libs"))
													.SafeClone()
													.AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + ".libs" + ".external"));

				packagePublicData.DynamicLibraries = MapDependentBuildOutputs.RemapModuleBinaryPaths(d.Dependent as ProcessableModule, module, Project.GetFileSet("package." + d.Dependent.Package.Name + ".dlls"), Project.Properties["dll-suffix"])
													.SafeClone()
													.AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + ".dlls" + ".external"));
			}
			if (d.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
			{
				FileSet dllRemappedAssemblies = MapDependentBuildOutputs.RemapModuleBinaryPaths(d.Dependent as ProcessableModule, module, Project.GetFileSet("package." + d.Dependent.Package.Name + ".assemblies"), ".dll");
				FileSet exeRemappedAssemblies = MapDependentBuildOutputs.RemapModuleBinaryPaths(d.Dependent as ProcessableModule, module, dllRemappedAssemblies, ".exe");
				packagePublicData.Assemblies = exeRemappedAssemblies.SafeClone()
													.AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + ".assemblies" + ".external"));

				packagePublicData.ContentFiles = Project.GetFileSet("package." + d.Dependent.Package.Name + ".contentfiles")
													.SafeClone();
			}

			if (packagePublicData.Defines == null)
			{
				packagePublicData.Defines = new String[0];
			}
			if (packagePublicData.IncludeDirectories == null)
			{
				packagePublicData.IncludeDirectories = new PathString[0];
			}
			if (packagePublicData.UsingDirectories == null)
			{
				packagePublicData.UsingDirectories = new PathString[0];
			}
			if (packagePublicData.LibraryDirectories == null)
			{
				packagePublicData.LibraryDirectories = new PathString[0];
			}
			if (packagePublicData.Libraries == null)
			{
				packagePublicData.Libraries = new FileSet();
			}
			if (packagePublicData.DynamicLibraries == null)
			{
				packagePublicData.DynamicLibraries = new FileSet();
			}
			if (packagePublicData.Assemblies == null)
			{
				packagePublicData.Assemblies = new FileSet();
			}
			if (packagePublicData.ContentFiles == null)
			{
				packagePublicData.ContentFiles = new FileSet();
			}

			// Not going to support package level .programs, .sndbs-global-options property or .sndbs-rewrite-rules-files fileset.
			packagePublicData.Programs = new FileSet();
			packagePublicData.SnDbsRewriteRulesFiles = new FileSet();

			packagePublicData.Libraries.FailOnMissingFile = false;
			packagePublicData.DynamicLibraries.FailOnMissingFile = false;
			packagePublicData.Assemblies.FailOnMissingFile = false;
			packagePublicData.Programs.FailOnMissingFile = false;

			var dependentModule = d.Dependent;
			{

				PublicData modulePublicData = new PublicData();

				if (d.IsKindOf(DependencyTypes.Interface))
				{
					string moduleProp = "package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".defines";
					string moduleincludesProp = "package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".includedirs";
					string moduleusingdirsProp = "package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".usingdirs";

					string moduledefines;
					string moduleincludes;
					string moduleusingdirs;

					using (var scope = Project.Properties.ReadScope())
					{
						moduledefines = scope[moduleProp];
						moduleincludes = scope[moduleincludesProp];
						moduleusingdirs = scope[moduleusingdirsProp];
					}

					modulePublicData.Defines = (moduledefines != null) ? moduledefines.LinesToArray() : packagePublicData.Defines;
					modulePublicData.IncludeDirectories = (moduleincludes != null) ? moduleincludes.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>() : packagePublicData.IncludeDirectories;

					modulePublicData.UsingDirectories = moduleusingdirs != null ?
									MapDependentBuildOutputs.MapDependentOutputDir(Project, moduleusingdirs.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>(), outputmapOptionset, d.Dependent.Package.Name, "lib")
									: packagePublicData.UsingDirectories;

					FileSet moduleSnDbsRewriteRulesFiles = Project.NamedFileSets["package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".sndbs-rewrite-rules-files"];
					modulePublicData.SnDbsRewriteRulesFiles = moduleSnDbsRewriteRulesFiles.SafeClone();
				}
				if (d.IsKindOf(DependencyTypes.Link) || (d.Dependent is Module_Native)) // To account for transitive propagation
				{
					string modulelibdirs = Project.Properties["package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".libdirs"];
					modulePublicData.LibraryDirectories = modulelibdirs != null ?
									MapDependentBuildOutputs.MapDependentOutputDir(Project, modulelibdirs.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).ToList<PathString>(), outputmapOptionset, d.Dependent.Package.Name, "lib")
									: packagePublicData.LibraryDirectories;

					FileSet moduleLibraries = MapDependentBuildOutputs.RemapModuleLibraryPaths(d.Dependent as ProcessableModule, module, Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".libs"));
					modulePublicData.Libraries = (moduleLibraries ?? packagePublicData.Libraries)
														.SafeClone()
														.AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".libs" + ".external"));

					FileSet moduleSharedLibraries = MapDependentBuildOutputs.RemapModuleBinaryPaths(d.Dependent as ProcessableModule, module, Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".dlls"), Project.Properties["dll-suffix"]);
					modulePublicData.DynamicLibraries = (moduleSharedLibraries ?? packagePublicData.DynamicLibraries)
														.SafeClone()
														.AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".dlls" + ".external"));

					FileSet moduleNatvisFiles = Project.NamedFileSets["package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".natvisfiles"];
					modulePublicData.NatvisFiles = moduleNatvisFiles.SafeClone();
				}
				else if (!d.IsKindOf(DependencyTypes.Link) && d.IsKindOf(DependencyTypes.CopyLocal) && (d.Dependent is Module_Utility))
				{
					// We have dependency with module like Core/Engine.BuildInfo where it is explicitly marked with build dependency
					// with do not link but with copylocal set to true.  Under normal situation, the module is a "Native" module and the
					// above block will still get the dlls fileset.  However, under outsourcing workflow, the native module get turned
					// into Utility module wrapper.  So we need to account for that.
					FileSet moduleSharedLibraries = MapDependentBuildOutputs.RemapModuleBinaryPaths(d.Dependent as ProcessableModule, module, Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".dlls"), Project.Properties["dll-suffix"]);
					modulePublicData.DynamicLibraries = (moduleSharedLibraries ?? packagePublicData.DynamicLibraries)
														.SafeClone()
														.AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".dlls" + ".external"));
				}

				if (d.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
				{
					FileSet dllRemappedAssemblies = MapDependentBuildOutputs.RemapModuleBinaryPaths(d.Dependent as ProcessableModule, module, Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".assemblies"), ".dll");
					FileSet exeRemappedAssemblies = MapDependentBuildOutputs.RemapModuleBinaryPaths(d.Dependent as ProcessableModule, module, dllRemappedAssemblies, ".exe");
					FileSet moduleAssemblies = exeRemappedAssemblies;
					modulePublicData.Assemblies = (moduleAssemblies ?? packagePublicData.Assemblies)
														.SafeClone()
														.AppendIfNotNull(Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".assemblies" + ".external"));

					FileSet moduleContentFiles = Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".contentfiles");
					modulePublicData.ContentFiles = (moduleContentFiles ?? packagePublicData.ContentFiles)
														.SafeClone();
				}

				// Only support copylocal of programs if dependent module is a utility module.  For buildable program modules, the module itself
				// should be setting the desired build output location and shouldn't need the programs to be copied to somewhere else.
				// Also, only support including programs if the parent module explicitly mark the dependent as copylocal.  This is a special
				// use case and we may not want to indiscriminately copy all dependent utility module's programs fileset.
				if (d.IsKindOf(DependencyTypes.CopyLocal) && d.Dependent.IsKindOf(Module.Utility))
				{
					FileSet modulePrograms = Project.GetFileSet("package." + d.Dependent.Package.Name + "." + dependentModule.Name + ".programs");
					modulePublicData.Programs = (modulePrograms ?? packagePublicData.Programs).SafeClone();
				}

				if (modulePublicData.Defines == null)
				{
					modulePublicData.Defines = new String[0];
				}
				if (modulePublicData.IncludeDirectories == null)
				{
					modulePublicData.IncludeDirectories = new PathString[0];
				}
				if (modulePublicData.UsingDirectories == null)
				{
					modulePublicData.UsingDirectories = new PathString[0];
				}
				if (modulePublicData.LibraryDirectories == null)
				{
					modulePublicData.LibraryDirectories = new PathString[0];
				}
				if (modulePublicData.Libraries == null)
				{
					modulePublicData.Libraries = new FileSet();
				}
				if (modulePublicData.DynamicLibraries == null)
				{
					modulePublicData.DynamicLibraries = new FileSet();
				}
				if (modulePublicData.Assemblies == null)
				{
					modulePublicData.Assemblies = new FileSet();
				}
				if (modulePublicData.ContentFiles == null)
				{
					modulePublicData.ContentFiles = new FileSet();
				}
				if (modulePublicData.NatvisFiles == null)
				{
					modulePublicData.NatvisFiles = new FileSet();
				}
				if (modulePublicData.Programs == null)
				{
					modulePublicData.Programs = new FileSet();
				}
				if (modulePublicData.SnDbsRewriteRulesFiles == null)
				{
					modulePublicData.SnDbsRewriteRulesFiles = new FileSet();
				}

				modulePublicData.Libraries.FailOnMissingFile = false;
				modulePublicData.DynamicLibraries.FailOnMissingFile = false;
				modulePublicData.Assemblies.FailOnMissingFile = false;
				modulePublicData.Programs.FailOnMissingFile = false;

				// TODO visual studio copy local
				// -dvaliant 2016/04/04
				/* when using Visual Studio to do copy local this is required for settings copy local to true on
				references i.e when a dotnet module depends a on loose dlls module:

				var copyLocal = ConvertUtil.ToEnum<CopyLocalType>(PropGroupValue("copylocal", "false"));

				if (d.IsKindOf(DependencyTypes.CopyLocal) || copyLocal == CopyLocalType.True || copyLocal == CopyLocalType.Slim)
				{
					foreach (var inc in modulePublicData.DynamicLibraries.Includes)
					{
						if (inc.OptionSet == null)
						{
							inc.OptionSet = "copylocal";
						}
					}
					if (!module.IsKindOf(Module.Native) || module.IsKindOf(Module.Managed))
					{
						foreach (var inc in modulePublicData.Assemblies.Includes)
						{
							if (inc.OptionSet == null)
							{
								inc.OptionSet = "copylocal";
							}
						}
					}
				}*/

				if (Log.WarningLevel >= Log.WarnLevel.Advise)
				{
					ValidateStructuredXmlUseForPublicData(d, dependentModule, modulePublicData);
				}

				if (!d.Dependent.Package.TryAddPublicData(modulePublicData, module, dependentModule))
				{
					throw new BuildException
					(
						$"{module.LogPrefix}Module is depending on [{d.Dependent.Package.Name}] {dependentModule.BuildGroup + "." + dependentModule.Name} but cannot add public data for it, because this dependency was already processed. " +
						"This may happen when several targets are chained nant build example-build test-build. Currently this is unsupported."
					);
				}
			}
		}

		private void ValidateStructuredXmlUseForPublicData(Dependency<IModule> d, IModule dependentModule, PublicData modulePublicData)
		{
			// Check if the initialize.xml file is written using Structured XML
			if 
			(
				(d.Dependent.Package.Project == null && !Project.Log.IsWarningEnabled(Log.WarningId.StructuredXmlNotUsed, Log.WarnLevel.Normal)) || 
				(d.Dependent.Package.Project != null && !d.Dependent.Package.Project.Log.IsWarningEnabled(Log.WarningId.StructuredXmlNotUsed, Log.WarnLevel.Normal))
			)
			{
				// Not all modules expose public data, so we need to check if any is exposed and then check if it is done using structured XML
				if (
					modulePublicData.Libraries.Includes.Count() > 0 ||
					modulePublicData.DynamicLibraries.Includes.Count() > 0 ||
					modulePublicData.Assemblies.Includes.Count() > 0 ||
					modulePublicData.ContentFiles.Includes.Count() > 0 ||
					modulePublicData.IncludeDirectories.Count() > 0 ||
					modulePublicData.Defines.Count() > 0 ||
					modulePublicData.LibraryDirectories.Count() > 0 ||
					modulePublicData.UsingDirectories.Count() > 0
				)
				{
					string package_prop = String.Format("package.{0}.structured-initialize-xml-used", dependentModule.Package.Name);
					string package_module_prop = String.Format("package.{0}.{1}.structured-initialize-xml-used", dependentModule.Package.Name, dependentModule.Name);
					if (Project.Properties.GetBooleanPropertyOrDefault(package_prop, false) == false &&
						Project.Properties.GetBooleanPropertyOrDefault(package_module_prop, false) == false)
					{
						DoOnce.Execute(package_module_prop, () =>
						{
							string message = String.Format("[{0}] Structured XML has not been used for declaring <publicdata> in initialize.xml for module '{1}' in package '{2}'.",
								(int)Log.WarningId.StructuredXmlNotUsed, dependentModule.Name, dependentModule.Package.Name);
							Log.Warning.WriteLine(message + " Please refer to the Framework docs to learn how the structured XML <publicdata> element can benefit your build scripts.");
						});
					}
				}
			}
		}

		private void SetModuleBuildSteps(Module module)
		{
			string prefix = !(module is Module_DotNet) ? "vcproj" : GetFrameworkPrefix(module as Module_DotNet);
			// Pre/post build steps
			AddBuildStepToModule(module, GetBuildStep(module, "prebuild", "prebuildtarget", prefix + ".pre-build-step", BuildStep.PreBuild));
			AddBuildStepToModule(module, GetBuildStep(module, "prelink", "prelinktarget", prefix + ".pre-link-step", BuildStep.PreLink));
			AddBuildStepToModule(module, GetBuildStep(module, "postbuild", "postbuildtarget", prefix + ".post-build-step", BuildStep.PostBuild));
			// Custom build steps. VS runs them last. Add as a postbuild step.
			AddBuildStepToModule(module, GetCustomBuildStep(module, BuildStep.PostBuild));
		}

		public void SetModuleAssetFiles(Module module)
		{
			module.DeployAssets = module.GetModuleBooleanPropertyValue("deployassets", BuildOptionSet.GetBooleanOptionOrDefault("deployassets", module.IsKindOf(Module.Program)));

			if(module.DeployAssets)
			{
				new ProcessModuleAssetFiles(module, LogPrefix).SetModuleAssetFiles();
			}
		}

		private void AddBuildStepToModule(Module module, BuildStep step)
		{
			if (step != null)
			{
				module.BuildSteps.Add(step);
			}
		}

		private BuildStep GetBuildStep(Module module, string stepname, string targetname, string commandname, uint type)
		{
			BuildStep step = new BuildStep(stepname, type);

			// Collect all target names:
			var targets = new List<Tuple<string, string, string>>() 
			{ 
				Tuple.Create(Project.Properties["eaconfig." + targetname] ?? "eaconfig." + targetname, "eaconfig." + targetname, "property"),
				Tuple.Create(Project.Properties["eaconfig." + module.Configuration.Platform + "." + targetname] ?? "eaconfig." + module.Configuration.Platform + "." + targetname, "eaconfig." + module.Configuration.Platform + "." + targetname, "property"),
				Tuple.Create(BuildOptionSet.GetOptionOrDefault(targetname, String.Empty), targetname, "option"),
				Tuple.Create(PropGroupValue(targetname, PropGroupName(targetname)), PropGroupName(targetname), "property")
			};

			foreach (var targetdef in targets)
			{
				foreach (var target in targetdef.Item1.ToArray())
				{

					if (Project.TargetExists(target))
					{
						var targetInput = new TargetInput();

						var nantbuildonly = BuildOptionSet.GetOptionOrDefault(target+".nantbuildonly", Project.Properties[PropGroupName(targetname)+"."+target+".nantbuildonly"]);

						step.TargetCommands.Add(new TargetCommand(target, targetInput, ConvertUtil.ToBooleanOrDefault(nantbuildonly, true)));
					}
					else
					{
						if (targetdef.Item3 == "option" || target != targetdef.Item2)
						{
							Log.ThrowWarning
							(
								Log.WarningId.InvalidTargetReference, Log.WarnLevel.Normal, 
								"Target '{0}' defined in {1} '{2}' does not exist", target, targetdef.Item3,targetdef.Item2
							);
						}
					}
				}
			}

			// Commands:
			var command = targetname.Replace("target", "command");
			var commands = new string[] 
			{ 
				Project.Properties["eaconfig." + command],
				Project.Properties["eaconfig." + module.Configuration.Platform + "." + command],
				BuildOptionSet.GetOptionOrDefault(command, String.Empty),
				PropGroupValue(commandname)
			};

			foreach (var commandbody in commands)
			{

				var script = commandbody.TrimWhiteSpace();
				if (!String.IsNullOrEmpty(script))
					step.Commands.Add(new Command(MacroMap.Replace(script, module.Macros, allowUnresolved: true))); // any place we are dealing with commands we have to allow unresolved unless %envvar% is used
			}

			if (step.TargetCommands.Count == 0 && step.Commands.Count == 0)
			{
				return null;
			}

			return step;
		}

		//some packages add header files into set of source file. Clean this up:
		private FileSet RemoveHeadersFromSourceFiles(Module_Native module, FileSet input)
		{
			if(input != null)
			{
			bool remove = false;
			foreach (var pattern in input.Includes)
			{
				if (pattern.Pattern.Value != null && (pattern.Pattern.Value.EndsWith("*", StringComparison.Ordinal) || HEADER_EXTENSIONS.Any(ext => pattern.Pattern.Value.ToLowerInvariant().EndsWith(ext, StringComparison.Ordinal))))
				{
					remove = true;
					break;
				}
			}
			if (remove)
			{
				var cleaned = new FileSet();
				cleaned.BaseDirectory = input.BaseDirectory;

				foreach (var fi in input.FileItems)
				{
					if (HEADER_EXTENSIONS.Contains(Path.GetExtension(fi.Path.Path)))
					{
						fi.SetFileDataFlags(FileData.Header);
						module.ExcludedFromBuild_Files.Include(fi);
					}
					else
					{
						cleaned.Include(fi);
					}
				}
				return cleaned;
			}
			}
			return input;
		}

		private void SetDebuglevelObsolete(IModule module, CcCompiler compiler)
		{
			// Getting package level debug level defines into the build
			if (BuildOptionSet.Options["debugflags"] != "off")
			{
				string debugLevel = Project.Properties.GetPropertyOrDefault("package.debuglevel", "0");
				debugLevel = Project.Properties.GetPropertyOrDefault("package." + module.Package.Name + ".debuglevel", debugLevel).TrimWhiteSpace();

				if (debugLevel != "0")
				{
					compiler.Defines.Add(String.Format("{0}_DEBUG={1}",  module.Package.Name.ToUpperInvariant(), debugLevel));
				}
			}
		}

		private void SetDebuglevelObsolete(IModule module, DotNetCompiler compiler)
		{
			// Getting package level debug level defines into the build
			if (BuildOptionSet.Options["debugflags"] != "off")
			{
				string debugLevel = Project.Properties.GetPropertyOrDefault("package.debuglevel", "0");
				debugLevel = Project.Properties.GetPropertyOrDefault("package." + module.Package.Name + ".debuglevel", debugLevel).TrimWhiteSpace();

				if (debugLevel != "0")
				{
					compiler.Defines.Add(String.Format("{0}_DEBUG", module.Package.Name.ToUpperInvariant()));
				}
			}
		}

		private BuildStep GetCustomBuildStep(Module module, uint type)
		{
			// Add Custom build steps. No Target exists for these steps at the moment:

			var source_custom_build = GetModulePropertyValue(".vcproj.custom-build-source").TrimWhiteSpace();

			string customcmdline = PropGroupValue("vcproj.custom-build-tool");

			if (!String.IsNullOrWhiteSpace(customcmdline))
			{
				// Deprecated way to define custom build step per file. If sources are defined bailout from here!
				// Data will be processed in IntitFileCustomBuildToolMapObsolete();
				
				if (!String.IsNullOrEmpty(source_custom_build))
				{
					return null;
				}
				BuildStep step = new BuildStep("custombuild", type);

				step.Before = Project.Properties.GetPropertyOrDefault(PropGroupName("custom-build-before-targets"), String.Empty);
				step.After = Project.Properties.GetPropertyOrDefault(PropGroupName("custom-build-after-targets"), String.Empty);

				// default type is postbuild, but if before or after target are set we clear the postbuild flag
				if (step.Before != String.Empty || step.After != String.Empty)
				{
					step.ClearType(BuildStep.PostBuild);
				}

				// set build type for native nant builds based on before target
				string beforetarget = step.Before.ToLower();
				if (beforetarget == "link")
				{
					step.SetType(BuildStep.PreLink);
				}
				else if (beforetarget == "build")
				{
					step.SetType(BuildStep.PreBuild);
				}
				else if (beforetarget == "run") 
				{ 
					// handled by eaconfig-run
				}
				else if (Log.Level >= Log.LogLevel.Detailed)
				{
					Log.Warning.WriteLine(String.Format(
						"Before value '{0}' not supported for native NAnt builds.", step.Before));
				}

				// set build type for native nant builds based on after target
				string aftertarget = step.After.ToLower();
				if (aftertarget == "build")
				{
					step.SetType(BuildStep.PostBuild);
				}
				else if (aftertarget == "run")
				{
					// handled by eaconfig-run
				}
				else if (Log.Level >= Log.LogLevel.Detailed)
				{
					Log.Warning.WriteLine("After value '{0}' not supported for native NAnt builds.", step.After);
				}

				step.Commands.Add(new Command(MacroMap.Replace(customcmdline, module.Macros, allowUnresolved: true)));

				step.OutputDependencies = MacroMap.Replace(PropGroupValue("vcproj.custom-build-outputs"), module.Macros).LinesToArray().Select(file => PathString.MakeNormalized(file)).OrderedDistinct().ToList();
				step.InputDependencies = MacroMap.Replace(PropGroupValue("vcproj.custom-build-dependencies"), module.Macros).LinesToArray().Select(file => PathString.MakeNormalized(file)).OrderedDistinct().ToList();
				var inputs = PropGroupFileSet("vcproj.custom-build-dependencies");
				if (inputs != null)
				{
					step.InputDependencies.AddRange(inputs.FileItems.Select(fi => fi.Path));
				}

				return step;
			}
			return null;
		}

		// note: if the convert function returns any value that does not take the function parameter into account 
		// then using this function is the same as tool.Options.Add(prefix+value) except you are doing a lot more unnecessary work
		private bool AddOption(Tool tool, string name, string prefix = "", bool useOptionProperty = false, Func<string, string> convert = null)
		{
			string value = GetOptionString(name, BuildOptionSet.Options, name, useOptionProperty).TrimWhiteSpace();

			if (convert == null)
			{
				if (value.OptionToBoolean())
				{
					tool.Options.Add(prefix);
					return true;
				}
			}
			else
			{
				value = convert(value);
				if (!String.IsNullOrWhiteSpace(value))
				{
					tool.Options.Add(prefix + convert(value));
					return true;
				}
			}
			return false;
		}

		// note: if the convert function returns any value that does not take the function parameter into account 
		// then using this function is the same as tool.Options.Add(prefix+value) except you are doing a lot more unnecessary work
		private bool AddOptionOrPropertyValue(Tool tool, string name, string prefix = "", Func<string, string> convert = null)
		{
			string value = GetModulePropertyValue(name);

			if (String.IsNullOrEmpty(value))
			{
				value = GetOptionString(name, BuildOptionSet.Options, useOptionProperty: false).TrimWhiteSpace();
			}

			if (convert == null)
			{
				if (value.OptionToBoolean())
				{
					tool.Options.Add(prefix);
					return true;
				}
			}
			else
			{
				value = convert(value);
				if (!String.IsNullOrWhiteSpace(value))
				{
					tool.Options.Add(prefix + value);
					return true;
				}
			}
			return false;
		}

		private void ExecuteProcessSteps(ProcessableModule module, Tool tool, OptionSet buildTypeOptionSet, IEnumerable<string> steps)
		{
			foreach (string step in steps)
			{
				var task = Project.TaskFactory.CreateTask(step, Project);
				if (task != null)
				{
					IBuildModuleTask bmt = task as IBuildModuleTask;
					if (bmt != null)
					{
						bmt.Module = module;
						bmt.Tool = tool;
						bmt.BuildTypeOptionSet = buildTypeOptionSet;
					}
					task.Execute();
				}
				else
				{
					Project.ExecuteTargetIfExists(step, force: true);
				}
			}
		}

		private BuildTool CreateCustomBuildTools(ProcessableModule module, FileItem fileitem, PathString basedir, FileSet sources = null, FileSet objects = null, List<PathString> includedirs = null)
		{
			OptionSet fileOptions = null;
			if (!String.IsNullOrEmpty(fileitem.OptionSetName))
			{
				if (Project.NamedOptionSets.TryGetValue(fileitem.OptionSetName, out fileOptions))
				{
					return CreateCustomBuildTools(module, fileitem, fileOptions, basedir, sources, objects, includedirs);
				}
				else
				{
					Log.Warning.WriteLine("ModuleProcessor_SetModuleData.CreateCustomBuildTools failed to add fileitem '{0}' because optionset '{1}' was not found in NamedOptionSets", fileitem.FileName, fileitem.OptionSetName);
				}
			}
			return null;
		}

		private BuildTool CreateCustomBuildTools(ProcessableModule module, FileItem fileitem, OptionSet fileOptions, PathString basedir, FileSet sources = null, FileSet objects = null, List<PathString> includedirs=null)
		{
			if (fileOptions == null)
			{
				return null;
			}

			var filepath = fileitem.Path.Path;
			var filename = Path.GetFileNameWithoutExtension(filepath);
			var fileext = Path.GetExtension(filepath);
			var filedir = Path.GetDirectoryName(filepath);
			var filebasedir = fileitem.BaseDirectory??basedir.Path;
			if(String.IsNullOrEmpty(filebasedir.TrimWhiteSpace()))
			{
				filebasedir = module.Package.Dir.Path;
			}
			var filereldir = PathUtil.RelativePath(filedir, filebasedir, failOnError: true);

			var execs = new List<Tuple<string, string>>();

			var build_tasks = OptionSetUtil.GetOptionOrDefault(fileOptions, "build.tasks", null).ToArray();
			if (build_tasks.Count == 0)
			{
				build_tasks.Add(String.Empty);
			}

			MacroMap fileItemMacros = new MacroMap(inherit: module.Macros)
			{
				{ "filepath", filepath },
				{ "filename", filename },
				{ "fileext", fileext },
				{ "filedir", filedir },
				{ "filebasedir", filebasedir },
				{ "filereldir", filereldir }
			};

			// build tool is usually created in "compile" stage, so the module.OutputDir is usually setup as
			// intermediate dir.  But a lot of times we run these custom tools because we want to create output
			// to final "bin" or "lib" dir location.  So we should add "outputbindir" and "outputlibdir" 
			// if these two are not already in the inherit list.
			if (!module.Macros.ContainsKey("outputbindir"))
			{
				string bindir = PathString.MakeNormalized(ModulePath.Private.GetModuleBinPath(Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), BuildType).OutputDir).Path;
				fileItemMacros.Add("outputbindir", bindir);
			}
			if (!module.Macros.ContainsKey("outputlibdir"))
			{
				string libdir = PathString.MakeNormalized(ModulePath.Private.GetModuleLibPath(Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), BuildType).OutputDir).Path;
				fileItemMacros.Add("outputlibdir", libdir);
			}

			foreach (string buildtask in build_tasks)
			{
				var buildtask_prefix = String.IsNullOrEmpty(buildtask) ? String.Empty : buildtask + ".";
				var command = MacroMap.Replace
				(
					OptionSetUtil.GetOptionOrDefault(fileOptions, buildtask_prefix + "command", String.Empty),
					fileItemMacros
				);

				// IM: Dice is using empty commands in VisualStudio just to have files act as input dependency for a project.
				// Do not add command.TrimWhiteSpace()  
				if (!String.IsNullOrEmpty(command))
				{
					execs.Add(Tuple.Create<string, string>(null, command));
				}

				var exec = OptionSetUtil.GetOptionOrDefault(fileOptions, buildtask_prefix + "executable", String.Empty).TrimWhiteSpace().Quote();
				var opt = OptionSetUtil.GetOptionOrDefault(fileOptions, buildtask_prefix + "options", String.Empty).TrimWhiteSpace();

				if (!String.IsNullOrEmpty(exec))
				{
					exec = MacroMap.Replace(exec, fileItemMacros, allowUnresolved: true); // any place we are dealing with commands we have to allow unresolved unless %envvar% is used

					if (!String.IsNullOrEmpty(opt))
					{
						opt = MacroMap.Replace(opt, fileItemMacros, allowUnresolved: true); // any place we are dealing with commands we have to allow unresolved unless %envvar% is used
					}
					else
					{
						opt = string.Empty;
					}

					execs.Add(Tuple.Create<string, string>(exec, opt));
				}
			}

			List<string> outputs =  MacroMap.Replace(OptionSetUtil.GetOptionOrDefault(fileOptions, "outputs", ""), fileItemMacros)
						.LinesToArray().OrderedDistinct().ToList();

			List<PathString> createoutputdirs = null;
			if (outputs.Count > 0 && fileOptions.GetBooleanOptionOrDefault("createoutputdirs", false))
			{
				createoutputdirs = new List<PathString>();
				foreach (var output in outputs)
				{
					createoutputdirs.Add(new PathString(Path.GetDirectoryName(output), PathString.PathState.Directory));
				}
			}

			string description = MacroMap.Replace(OptionSetUtil.GetOptionOrDefault(fileOptions, "description", ""), fileItemMacros);

			var treatOutputAsContent = fileOptions.GetBooleanOption("treatoutputascontent");

			var toolname = OptionSetUtil.GetOptionOrFail(fileOptions, "name");

			var tooltype = Tool.TypePreCompile;

			BuildTool tool = null;

			if (execs.Count > 1)
			{
				// Convert to script
				var script = new StringBuilder();
				foreach (var entry in execs)
				{
					if (entry.Item1 == null)
					{
						script.AppendLine(entry.Item2);
					}
					else
					{
						script.Append(entry.Item1);
						script.Append(" ");
						script.AppendLine(entry.Item2.LinesToArray().ToString(" "));
					}

					if (PlatformUtil.IsWindows)
					{
						script.AppendLine(BuildTool.SCRIPT_ERROR_CHECK);
					}
					else if (PlatformUtil.IsUnix || PlatformUtil.IsOSX)
					{
						script.AppendLine(@"[ $? -eq 0 ] || exit ");
					}
				}

				tool = new BuildTool(Project, toolname, script.ToString(), tooltype, description, outputdir: module.OutputDir, intermediatedir: module.IntermediateDir, createdirectories: createoutputdirs, treatoutputascontent: treatOutputAsContent);
			}
			else if (execs.Count == 1)
			{
				if (execs[0].Item1 == null)
				{
					// Script
					tool = new BuildTool(Project, toolname, execs[0].Item2, tooltype, description, outputdir: module.OutputDir, intermediatedir: module.IntermediateDir, createdirectories: createoutputdirs, treatoutputascontent: treatOutputAsContent);
				}
				else
				{
					// Exec + options
					tool = new BuildTool(Project, toolname, PathString.MakeNormalized(execs[0].Item1, PathString.PathParam.NormalizeOnly), tooltype, description, outputdir: module.OutputDir, intermediatedir: module.IntermediateDir, createdirectories: createoutputdirs, treatoutputascontent: treatOutputAsContent);
					tool.Options.AddRange(execs[0].Item2.LinesToArray());
				}
			}
			else
			{
				Log.Warning.WriteLine(LogPrefix + "Custom build file '{0}': custom tool '{0}' has no definition of either command or 'executable + options'", fileitem.Path.Path, toolname);
				return null;
			}

			fileitem.SetFileData(tool);

			ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["preprocess"].ToArray());

			tool.Files.Include(fileitem, baseDir: basedir.Path, failonmissing: false);


			foreach (var output in outputs)
			{
				tool.OutputDependencies.IncludePatternWithMacro(output, failonmissing: false);
			}

			string inputs = MacroMap.Replace(OptionSetUtil.GetOptionOrDefault(fileOptions, "inputs", ""), fileItemMacros);

			foreach (var input in inputs.LinesToArray().OrderedDistinct())
			{
				tool.InputDependencies.IncludePatternWithMacro(input, failonmissing: false);
			}

			foreach (var filesetName in OptionSetUtil.GetOptionOrDefault(fileOptions, "input-filesets", "").ToArray().OrderedDistinct())
			{
				FileSet fs;
				if(Project.NamedFileSets.TryGetValue(filesetName, out fs))
				{
					tool.InputDependencies.AppendIfNotNull(fs);
				}
				else
				{
					Log.Warning.WriteLine("Input dependency fileset '{0}' specified in custom build rule '{1}' does not exist", filesetName, tool.ToolName);
				}
			}


			Module_Native native_module = module as Module_Native;
			if (native_module != null)
			{
				if (sources != null)
				{
					string sourcefiles = MacroMap.Replace(OptionSetUtil.GetOptionOrDefault(fileOptions, "sourcefiles", ""), fileItemMacros);
					foreach (var src in sourcefiles.LinesToArray().OrderedDistinct())
					{
						sources.Include(src, failonmissing: false, data: new CustomBuildToolFlags(fileOptions.GetBooleanOption(FrameworkProperties.BulkBuildPropertyName) ? CustomBuildToolFlags.AllowBulkbuild : 0));
					}
				}

				string headerfiles = MacroMap.Replace(OptionSetUtil.GetOptionOrDefault(fileOptions, "headerfiles", ""), fileItemMacros);

				foreach (var hf in headerfiles.LinesToArray().OrderedDistinct())
				{
					native_module.ExcludedFromBuild_Files.Include(hf, failonmissing: false, data: new FileData(null, flags: FileData.Header));
				}

				if (objects != null)
				{
					string objectfiles = MacroMap.Replace(OptionSetUtil.GetOptionOrDefault(fileOptions, "objectfiles", ""), fileItemMacros);

					foreach (string of in objectfiles.LinesToArray().OrderedDistinct())
					{
						objects.Include(of, baseDir: filebasedir, failonmissing: false);
					}
				}

				if (includedirs != null)
				{
					string includedirsStr = MacroMap.Replace(OptionSetUtil.GetOptionOrDefault(fileOptions, "includedirs", ""), fileItemMacros);
					includedirs.AddRange(includedirsStr.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct());
				}

			}

			Module_DotNet dotnet_Module = module as Module_DotNet;             //TODO: this function is written with native very much in mind as it accumlates outputs for sources / headers / objs / etc
																			   // for dotnet the only one of these that makes sense is sources - however we might want additional dotnet cutputs like resources /
																			   // content / etc - best of all would remove specificity entirely and have each output file set declare its output type
			if (dotnet_Module != null && sources != null)
			{
				string sourcefiles = MacroMap.Replace(OptionSetUtil.GetOptionOrDefault(fileOptions, "sourcefiles", ""), fileItemMacros);
				foreach (var src in sourcefiles.LinesToArray().OrderedDistinct())
				{
					sources.Include(src, failonmissing: false, data: new CustomBuildToolFlags(0));
				}
			}

			ExecuteProcessSteps(module, tool, fileOptions, fileOptions.Options["postprocess"].ToArray());

			return tool;
		}

		#endregion

		#region DotNetModule
		/// <summary>BulkBuild features aren't available to C# or F# modules so if any bulkbuild properties are set throw an error</summary>
		private void ThrowIfBulkBuildPropertiesSet(Module_DotNet module)
		{
			if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.manual.sourcefiles")))
			{
				throw new BuildException($"[SetupBulkBuild] BulkBuild features aren't available to C# or F# modules, but '{module.GroupName}.bulkbuild.manual.sourcefiles' was specified.");
			}
			if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.manual.sourcefiles." + module.Configuration.System)))
			{
				throw new BuildException($"[SetupBulkBuild] BulkBuild features aren't available to C# or F# modules, but '{module.GroupName}.bulkbuild.manual.sourcefiles.{module.Configuration.System}' was specified.");
			}
			if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
			{
				throw new BuildException($"[SetupBulkBuild] BulkBuild features aren't available to C# or F# modules, but '{module.GroupName}.bulkbuild.filesets' was specified.");
			}
		}

		private string GetTargetFrameworkVersion(IModule module) => module.GetTargetFrameworkVersion();
		private string GetFrameworkPrefix(IModule module) => module.GetFrameworkPrefix();

		/// <summary>Returns the appropriate dotnet platform name given whether the module property value has been set or not</summary>
		private string ProcessModuleDotNetPlatform(string propertyValue, Module_DotNet module, DotNetCompiler compiler)
		{
			if (!String.IsNullOrWhiteSpace(propertyValue))
			{
				compiler.TargetPlatform = propertyValue;
				return propertyValue;
			}

			bool useExactPlatform = Project.Properties.GetBooleanPropertyOrDefault("eaconfig.use-exact-dotnet-platform", 
										module.GetModuleBooleanPropertyValue("use-exact-dotnet-platform", false))
										|| (compiler.Target == DotNetCompiler.Targets.Exe 
										|| compiler.Target == DotNetCompiler.Targets.WinExe);

			string platform = "anycpu";

			if (useExactPlatform)
			{
				var processor = (Project.Properties["config-processor"] ?? String.Empty).TrimWhiteSpace().ToLowerInvariant();
				switch (processor)
				{
					case "x86":
						platform = "x86";
						break;
					case "x64":
						platform = "x64";
						break;
					case "arm":
						platform = "ARM";
						break;
					default:
						//In case config processor is not defined, check pc/pc64
						switch (module.Configuration.System)
						{
							case "pc":
								platform = "x86";
								break;
							case "pc64":
								platform = "x64";
								break;
							default:
								platform = "anycpu";
								break;
						}
						break;
				}
			}

			compiler.TargetPlatform = platform;
			return platform;
		}

		private void AddAdditionalOptions(DotNetCompiler compiler)
		{
			var additional_args = GetModulePropertyValue(compiler.ToolName + "-args").TrimWhiteSpace();
			compiler.AdditionalArguments = additional_args;

			if (compiler.AdditionalArguments.Contains("-nowin32manifest"))
			{
				compiler.Options.Add("-nowin32manifest");
				compiler.AdditionalArguments = compiler.AdditionalArguments.Replace("-nowin32manifest", String.Empty);
			}
			if (compiler.AdditionalArguments.Contains("/nowin32manifest"))
			{
				compiler.Options.Add("/nowin32manifest");
				compiler.AdditionalArguments = compiler.AdditionalArguments.Replace("/nowin32manifest", String.Empty);
			}

			if (!String.IsNullOrEmpty(additional_args))
			{
				int index = 0;
				while (index < additional_args.Length)
				{
					int ind1 = Math.Min(additional_args.IndexOf(" /", index), additional_args.IndexOf(" -"));
					string option = null;
					if (ind1 < 0)
					{
						option = additional_args.Substring(index);
						index = additional_args.Length;
					}
					else
					{
						option = additional_args.Substring(index, ind1 - index);
						index = ind1 + 1;
					}
					option = option.TrimWhiteSpace();
					if (!String.IsNullOrEmpty(option))
					{
						compiler.Options.Add(option);
					}
				}
			}

			compiler.Options.AddRange(BuildOptionSet.Options[compiler.ToolName + ".options"].LinesToArray());
		}

		/// <summary>Add custom build files tools and build up list of generated source files</summary>
		private void SetupCustomBuildTools(Module_DotNet module, DotNetCompiler compiler)
		{
			FileSet generatedSourceFiles = new FileSet();
			FileSet customBuildFiles = AddModuleFiles(module, ".custombuildfiles");
			if (customBuildFiles != null)
			{
				PathString baseDir = PathString.MakeNormalized(customBuildFiles.BaseDirectory);
				foreach (FileItem fileitem in customBuildFiles.FileItems)
				{
					module.AddTool(CreateCustomBuildTools(module, fileitem, baseDir, generatedSourceFiles));
				}
			}

			// Append generate source files to explicit source files
			compiler.SourceFiles.Append(generatedSourceFiles);
		}

		private void SetGenerateDocs(Module_DotNet module, DotNetCompiler compiler)
		{
			string prefix = null;
			if (compiler is CscCompiler) prefix = "csc";
			else if (compiler is FscCompiler) prefix = "fsc";
			else throw new NotImplementedException("SetGenerateDocs is being called on a dot net compiler type that it isn't designed to support");

			string generateDocs = PropGroupValue(prefix + "-doc", "false");
			if (generateDocs != "true" && generateDocs != "false")
			{
				Log.Warning.WriteLine("{0} Property '{1}.{2}-doc'={3}, it should only be either 'true' or 'false', Other string values are considered invalid!.", LogPrefix, module.GroupName, prefix, generateDocs);
			}
			compiler.GenerateDocs = generateDocs.ToBoolean();

			if (compiler.GenerateDocs)
			{
				compiler.DocFile = PathString.MakeNormalized(module.GetPlatformModuleProperty(".doc-file", Path.Combine(module.IntermediateDir.Path, compiler.OutputName + ".xml")));
			}
		}

#endregion

		#region NativeModule

		private AsmCompiler CreateAsmTool(OptionSet buildOptionSet, Module_Native module)
		{
			var workingdir = buildOptionSet.Options["as.workingdir"];

			AsmCompiler compiler = new AsmCompiler(PathString.MakeNormalized(GetOptionString("as", buildOptionSet.Options), PathString.PathParam.NormalizeOnly), workingdir == null ? null : new PathString(workingdir, PathString.PathState.Undefined));
			compiler.Defines.AddRange(GetOptionString("as.defines", buildOptionSet.Options, ".defines").LinesToArray());
			compiler.Options.AddRange(GetOptionString("as.options", buildOptionSet.Options).LinesToArray());

			var additionaloptions = GetModulePropertyValue("as.options");
			if (!String.IsNullOrEmpty(additionaloptions))
			{
				compiler.Options.AddRange(additionaloptions.LinesToArray());
			}

			// Add module include directories, system include dirs are added later
			compiler.IncludeDirs.AddRange(GetModulePropertyValue(".includedirs",
				Path.Combine(Project.Properties["package.dir"], "include"),
				Path.Combine(Project.Properties["package.dir"], Project.Properties["eaconfig." + module.BuildGroup + ".sourcedir"]))
				.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct());

			string systemIncludes = GetOptionString(compiler.ToolName + ".system-includedirs", buildOptionSet.Options);
			if (!String.IsNullOrEmpty(systemIncludes))
			{
				compiler.SystemIncludeDirs.AddRange(systemIncludes.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).Distinct());
			}

			var removedefines = ((buildOptionSet.Options["remove.defines"]??String.Empty) + Environment.NewLine + GetModulePropertyValue("remove.defines")).LinesToArray();
			if (removedefines.Count > 0)
			{
				compiler.Defines.RemoveAll(x => removedefines.Any(y => x == y || x.StartsWith(y + "=")));
			}
			var removeasmoptions = ((buildOptionSet.Options["remove.as.options"]??String.Empty) + Environment.NewLine + GetModulePropertyValue("remove.as.options")).LinesToArray();
			if (removeasmoptions.Count > 0)
			{
				compiler.Options.RemoveAll(x => removeasmoptions.Contains(x));
			}

			compiler.ObjFileExtension = GetOptionString("as.objfile.extension", buildOptionSet.Options) ?? ".obj";

			return compiler;
		}


		private CcCompiler.PrecompiledHeadersAction GetModulePchAction(OptionSet buildOptionSet, Module_Native module)
		{
			CcCompiler.PrecompiledHeadersAction pchAction = CcCompiler.PrecompiledHeadersAction.NoPch;

			if (module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
			{
				if (buildOptionSet.GetBooleanOption("create-pch"))
				{
					pchAction = CcCompiler.PrecompiledHeadersAction.CreatePch;

					if (buildOptionSet.GetBooleanOption("use-pch"))
					{
						DoOnce.Execute("key-"+ module.Package.Name + module.Name + buildOptionSet.Options["buildset.name"],
							() => { Log.Warning.WriteLine(LogPrefix + "[{0}] {1} - {2} - both 'use-pch' and 'create-pch' options are specified. 'create-pch' option will be used.", module.Package.Name, module.Name, buildOptionSet.Options["buildset.name"]); }
						);
					}
				}
				else if (buildOptionSet.GetBooleanOption("use-pch"))
				{
					pchAction = CcCompiler.PrecompiledHeadersAction.UsePch;
				}
			}

			return pchAction;
		}

		private CcCompiler CreateCcTool(OptionSet currentOptionSet, Module_Native module, OptionSet moduleLevelOptionSet)
		{
			CcCompiler.PrecompiledHeadersAction pch = GetModulePchAction(currentOptionSet, module);

			// If a file specific optionset specified to not use pch, don't add force include to that header.
			bool fileSpecificOptSetAllowsPch = currentOptionSet.GetBooleanOptionOrDefault("use-pch", true);

			// Cannot forced include a c++ header to a c file.  We need to test for that.  Most of the
			// build type are c++ build types with 'clanguage' flag set to 'off' as default.  
			// C files build option will have 'clanguage' set to 'on'.
			bool forcedIncHdrCompatible = true;
			if (moduleLevelOptionSet.GetBooleanOptionOrDefault("clanguage", false) == false &&    // A c++ buildtype
				currentOptionSet.GetBooleanOptionOrDefault("clanguage", false))                     // A c file buildoption
			{
				forcedIncHdrCompatible = false;
			}
			var workingdir = currentOptionSet.Options["cc.workingdir"];

			CcCompiler compiler = new CcCompiler(PathString.MakeNormalized(GetOptionString("cc", currentOptionSet.Options), PathString.PathParam.NormalizeOnly), pch, workingdir == null ? null : new PathString(workingdir, PathString.PathState.Undefined));
			if (module.UseForcedIncludePchHeader && !String.IsNullOrEmpty(module.PrecompiledHeaderFile) &&
			   ((module.UsingSharedPchModule && fileSpecificOptSetAllowsPch && forcedIncHdrCompatible) || pch != CcCompiler.PrecompiledHeadersAction.NoPch))
			{
				string forcedIncludeHdrSwitch = compiler.GetForcedIncludeCcSwitch(module);
				if (!String.IsNullOrEmpty(forcedIncludeHdrSwitch))
				{
					compiler.Options.Insert(0, forcedIncludeHdrSwitch);
				}
			}
			compiler.Defines.AddRange(GetOptionString(compiler.ToolName + ".defines", currentOptionSet.Options, ".defines").LinesToArray());
			compiler.Defines.AddRange(Project.Properties["global.defines"].LinesToArray());
			compiler.Defines.AddRange(Project.Properties["global.defines." + module.Package.Name].LinesToArray());

			compiler.CompilerInternalDefines.AddRange(GetOptionString(compiler.ToolName + ".compilerinternaldefines", currentOptionSet.Options, ".compilerinternaldefines").LinesToArray());

			compiler.Options.AddRange(ReconcileWarningSettings(module, GetOptionString(compiler.ToolName + ".options", currentOptionSet.Options).LinesToArray()));

			var additionaloptions = GetModulePropertyValue(compiler.ToolName + ".options");
			if (!String.IsNullOrEmpty(additionaloptions))
			{
				compiler.Options.AddRange(additionaloptions.LinesToArray());
			}

			compiler.UsingDirs.AddRange(GetOptionString(compiler.ToolName + ".usingdirs", currentOptionSet.Options, ".usingdirs").LinesToArray().Select(dir => PathString.MakeNormalized(dir)).Distinct());

			// Add module include directories, system include dirs are added later
			compiler.IncludeDirs.AddRange(GetModulePropertyValue(".includedirs",
				Path.Combine(Project.Properties["package.dir"], "include"),
				Path.Combine(Project.Properties["package.dir"], Project.Properties["eaconfig." + module.BuildGroup + ".sourcedir"]))
				.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct());

			if (module.PrecompiledHeaderDir != null && !String.IsNullOrEmpty(module.PrecompiledHeaderDir.Path))
			{
				PathString pchHdrDir = PathString.MakeNormalized(module.PrecompiledHeaderDir.Path);
				if (compiler.IncludeDirs.FirstOrDefault() != pchHdrDir)
				{
					if (compiler.IncludeDirs.Contains(pchHdrDir))
					{
						// Need to move this and insert at the beginning to make sure correct header is used.
						compiler.IncludeDirs.Remove(pchHdrDir);
					}
					compiler.IncludeDirs.Insert(0, pchHdrDir);
				}
			}

			string systemIncludes = GetOptionString(compiler.ToolName + ".system-includedirs", currentOptionSet.Options);
			if (!String.IsNullOrEmpty(systemIncludes))
			{
				compiler.SystemIncludeDirs.AddRange(systemIncludes.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).Distinct());
			}

			AddModuleFiles(compiler.Assemblies, module, ".assemblies");
			foreach (PathString forceUsingAssembly in GetOptionString(compiler.ToolName + ".forceusing-assemblies", currentOptionSet.Options, ".forceusing-assemblies")
							.LinesToArray()
							.Select(lib => PathString.MakeNormalized(lib, PathString.PathParam.NormalizeOnly)).OrderedDistinct())
			{
				compiler.Assemblies.Include(forceUsingAssembly.Path, asIs: true);
			}
			foreach (PathString forceUsingAssembly in GetOptionString(compiler.ToolName + ".sdk-forceusing-assemblies", currentOptionSet.Options, ".sdk-forceusing-assemblies")
				.LinesToArray()
				.Select(lib => PathString.MakeNormalized(lib, PathString.PathParam.NormalizeOnly)).OrderedDistinct())
			{
				compiler.Assemblies.Include(forceUsingAssembly.Path, asIs: true, optionSetName:"sdk-reference");
			}

			SetupComAssembliesAssemblies(compiler, module);

			SetupWebReferences(compiler, module);

			var removedefines = ((currentOptionSet.Options["remove.defines"]??String.Empty) + Environment.NewLine + GetModulePropertyValue("remove.defines")).LinesToArray();
			if (removedefines.Count > 0)
			{
				compiler.Defines.RemoveAll(x => removedefines.Any(y => x == y || x.StartsWith(y + "=")));
			}
			var removeccoptions = ((currentOptionSet.Options["remove.cc.options"]??String.Empty) + Environment.NewLine + GetModulePropertyValue("remove.cc.options")).LinesToArray();
			if (removeccoptions.Count > 0)
			{
				compiler.Options.RemoveAll(x => removeccoptions.Contains(x));
			}

			compiler.ObjFileExtension = GetOptionString(compiler.ToolName + ".objfile.extension", currentOptionSet.Options) ?? ".obj";

			return compiler;
		}

		private List<string> ReconcileWarningSettings(Module_Native module, IEnumerable<string> options)
		{
			List<string> reconciledOptions = new List<string>();
			IEnumerable<string> warningSuppression = GetModulePropertyValue(".warningsuppression").LinesToArray();
			if (module.Configuration.Compiler == "vc")
			{
				// for vc compiler we can't make any assumptions about order because Visual Studio can reorder compiler command line
				// so we have to reconcile warning settings ourselves, anything from module warning suppression should trump
				// inherited build option settings
				Regex vcWarningSettiing = new Regex(@"[-/]w[ed0-4](\d+)");
				Dictionary<string, string> warningSettings = new Dictionary<string, string>();
				foreach (string option in options.Concat(warningSuppression))
				{
					Match match = vcWarningSettiing.Match(option);
					if (match.Success)
					{
						warningSettings[match.Groups[1].Value] = match.Groups[0].Value; // for each warning take the setting we last encountered that pertained to that warning number
					}
					else
					{
						reconciledOptions.Add(option);
					}
				}

				reconciledOptions.AddRange(warningSettings.Values);
				return reconciledOptions;
			}
			else
			{
				reconciledOptions.AddRange(options);
				reconciledOptions.AddUnique(warningSuppression);
				return reconciledOptions;
			}			
		}

		private Linker CreateLinkTool(Module_Native module)
		{
			var workingdir = BuildOptionSet.Options["link.workingdir"];

			Linker linker = new Linker(PathString.MakeNormalized(GetOptionString("link", BuildOptionSet.Options), PathString.PathParam.NormalizeOnly), workingdir == null ? null : new PathString(workingdir, PathString.PathState.Undefined));

			SetLinkOutput(module, out string outputname, out string outputextension, out PathString linkoutputdir, out string replacename); // Redefine output dir and output name based on linkoutputname property.

			linker.OutputName = outputname;
			linker.OutputExtension = outputextension;
			linker.LinkOutputDir = linkoutputdir;

			linker.Options.AddRange(GetOptionString(linker.ToolName + ".options", BuildOptionSet.Options)
									.LinesToArray());


			var removelinkoptions = ((BuildOptionSet.Options["remove.link.options"] ?? String.Empty) + Environment.NewLine + GetModulePropertyValue("remove.link.options")).LinesToArray();
			if (removelinkoptions.Count > 0)
			{
				linker.Options.RemoveAll(x => removelinkoptions.Contains(x));
			}

			string[] OPTION_NAME_LIST = new string[] { 
				"linkoutputname", 
				"linkoutputpdbname", 
				"linkoutputmapname", 
				"imgbldoutputname", 
				"securelinkoutputname" 
			};


			foreach (string option in OPTION_NAME_LIST)
			{
				if (BuildOptionSet.Options.TryGetValue(option, out string value))
				{
					if (option == "linkoutputname")
					{
						// To avoid recursive expansion of linkoutputname template.
						value = Path.Combine(linker.LinkOutputDir.Path, linker.OutputName + linker.OutputExtension);
					}
					else
					{
						value = MacroMap.Replace(value, module.Macros);
					}

					if (option == "linkoutputpdbname" || option == "linkoutputmapname")
					{
						module.RuntimeDependencyFiles.Add(value);
					}

					value = PathNormalizer.Normalize(value, false);

					module.Macros.Update(option, value);
				}
			}

			// setup import lib information
			ModulePath importLibPath = ModulePath.Private.GetModuleLibPath(module);
			linker.ImportLibOutputDir = PathString.MakeNormalized(importLibPath.OutputDir); // always set dir, even if not applicable
			module.Macros.Update("outputlibdir", linker.ImportLibOutputDir.Path);

			if (BuildOptionSet.Options["impliboutputname"] != null) 
			{
				// the existance of impliboutputname option controls whether this build type is expected to produce an import lib or not - later logic checks if 
				// ImportLibFullPath is set when gathering library dependencies for module that depend on this that have a link step
				linker.ImportLibFullPath = PathString.MakeNormalized(Path.Combine(importLibPath.OutputDir, importLibPath.OutputName + importLibPath.Extension));
				module.Macros.Update("impliboutputname", linker.ImportLibFullPath.Path);
			}

			for (int i = 0; i < linker.Options.Count; i++)
			{
				linker.Options[i] = MacroMap.Replace(linker.Options[i], module.Macros, additionalErrorContext: $"while replacing linker options for module '{module.Name}' with build type '{module.BuildType.Name}' ");
			}

			linker.LibraryDirs.AddRange(GetOptionString(linker.ToolName + ".librarydirs", BuildOptionSet.Options, ".librarydirs")
										.LinesToArray()
										.Select(dir => PathString.MakeNormalized(dir, PathString.PathParam.NormalizeOnly)).OrderedDistinct());

			//Options can contain lib names without a path (system libs, for example), don't normalize with making full path, and add asIs to the fileset:
			foreach (var library in GetOptionString(linker.ToolName + ".libraries", BuildOptionSet.Options, ".libs")
										.LinesToArray()
										.Select(lib => PathString.MakeNormalized(lib, PathString.PathParam.NormalizeOnly)).OrderedDistinct())
			{
				linker.Libraries.Include(library.Path, asIs:true);
			}

			// Some build scripts explicitly include libraries from other modules in the same package
			if (ModuleOutputNameMapping != null)
			{
				FileSet libsToMap = new FileSet();
				AddModuleFiles(libsToMap, module, ".libs");
				linker.Libraries.IncludeWithBaseDir(MapDependentBuildOutputs.RemapModuleBinaryPaths(module, null, libsToMap, Project.Properties["lib-suffix"]));
			}
			else
			{
				AddModuleFiles(linker.Libraries, module, ".libs");
			}

			AddModuleFiles(linker.DynamicLibraries, module, ".dlls");

			AddModuleFiles(linker.ObjectFiles, module, ".objects");

			linker.UseLibraryDependencyInputs = BuildOptionSet.GetBooleanOptionOrDefault("uselibrarydependencyinputs", linker.UseLibraryDependencyInputs);

			return linker;
		}

		private PostLink CreatePostLinkTool(OptionSet buildOptionSet, IModule module, Linker linker)
		{
			string program = GetOptionString("link.postlink.program", BuildOptionSet.Options).TrimWhiteSpace();

			if (!String.IsNullOrEmpty(program))
			{
				program = MacroMap.Replace(program, module.Macros);

				PostLink postlink = new PostLink(PathString.MakeNormalized(program, PathString.PathParam.NormalizeOnly));
				var primaryoutput = Path.Combine(linker.LinkOutputDir.Path, linker.OutputName + linker.OutputExtension);
				
				postlink.Options.AddRange
				(
					MacroMap.Replace(GetOptionString("link.postlink.commandline", BuildOptionSet.Options) ?? String.Empty, module.Macros)
					.LinesToArray()
				);
				return postlink;
			}
			return null;
		}

		private Librarian CreateLibTool(ProcessableModule module)
		{
			// create lib tool	
			string workingdir = BuildOptionSet.Options["lib.workingdir"];
			Librarian librarian = new Librarian(PathString.MakeNormalized(GetOptionString("lib", BuildOptionSet.Options), PathString.PathParam.NormalizeOnly), workingdir == null ? null : new PathString(workingdir, PathString.PathState.Undefined));
			librarian.Options.AddRange(GetOptionString(librarian.ToolName + ".options", BuildOptionSet.Options).LinesToArray());
			librarian.ObjectFiles.AppendWithBaseDir(PropGroupFileSet(".objects"));
			librarian.ObjectFiles.AppendWithBaseDir(PropGroupFileSet(".objects." + module.Configuration.System));

			// set paths
			ModulePath libPath = ModulePath.Private.GetModuleLibPath(module);
			librarian.OutputName = libPath.OutputName;
			librarian.OutputExtension = libPath.Extension;
			module.OutputDir = PathString.MakeNormalized(libPath.OutputDir);

			module.Macros.Update("outputname", libPath.OutputNameToken);
			module.Macros.Update("outputdir", module.OutputDir.Path);
			module.Macros.Update("liboutputname", libPath.FullPath);
			module.Macros.Update("output", libPath.FullPath);

			// replace option tokens with resolved paths
			for (int i = 0; i < librarian.Options.Count; i++)
			{
				librarian.Options[i] = MacroMap.Replace(librarian.Options[i], module.Macros);
			}

			// delete remove options
			IList<string> removeliboptions = ((BuildOptionSet.Options["remove.lib.options"] ?? String.Empty) + Environment.NewLine + GetModulePropertyValue("remove.lib.options")).LinesToArray();
			if (removeliboptions.Count > 0)
			{
				librarian.Options.RemoveAll(x => removeliboptions.Contains(x));
			}

			return librarian;
		}

		private void SetupNatvisFiles(Module_Native module)
		{
			module.NatvisFiles = new FileSet();
			AddModuleFiles(module.NatvisFiles, module, ".natvisfiles");
			if (module.Link != null && module.NatvisFiles.FileItems.Count > 0 && module.Configuration.Compiler == "vc")
			{
				foreach (FileItem item in module.NatvisFiles.FileItems)
				{
					module.Link.Options.Add("/NATVIS:" + item.FullPath.Quote());
				}
			}
		}

		private FileSet SetupBulkbuild(Module_Native module, CcCompiler compiler, FileSet cc_sources, FileSet custombuildsources, out FileSet excludedSources)
		{
			excludedSources = null;
			bool bulkbuild = module.IsBulkBuild();

			if (bulkbuild)
			{

				if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.sourcefiles")) &&
					PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
				{
					throw new BuildException($"[VerifyBulkBuildSetup] You cannot have both '{PropGroupName("bulkbuild.sourcefiles")}' and '{ PropGroupName("bulkbuild.filesets")}' defined.");
				}
				if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
				{
					foreach (string filesetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
					{
						if (!FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild." + filesetName + ".sourcefiles")))
						{
							throw new BuildException($"[VerifyBulkBuildSetup] FileSet '{PropGroupName("bulkbuild." + filesetName + ".sourcefiles")}' was specified in '{PropGroupName("bulkbuild.filesets")}' but it doesn't exist!");
						}
					}
				}
			}

			bool isReallyBulkBuild = false;

			string bulkbuildOutputdir = Path.Combine(module.Package.PackageConfigBuildDir.Path,  module.Package.Project.Properties["eaconfig."+module.BuildGroup+".outputfolder"].TrimLeftSlash()??String.Empty,"source");

			FileSet source = new FileSet();
			source.FailOnMissingFile = false;

			if (bulkbuild)
			{
				excludedSources = new FileSet();

				// As of eaconfig-1.21.01, unless explicitly specified, we're now NOT enabling partial
				// BulkBuild.  Metrics gathered on Def Jam and C&C show that it's just TOO slow. 
				bool enablePartialBulkBuild = Project.Properties.GetBooleanPropertyOrDefault("package.enablepartialbulkbuild",
												Project.Properties.GetBooleanProperty(PropGroupName("enablepartialbulkbuild")));

				List<FileItem> custombuildLooseSources = null;
				List<FileItem> custombuildBulkBuildSources = null;
				if (custombuildsources != null)
				{
					custombuildLooseSources = new List<FileItem>();
					custombuildBulkBuildSources = new List<FileItem>();

					foreach (var cbFi in custombuildsources.FileItems)
					{
						var flags = cbFi.Data as CustomBuildToolFlags;
						if (flags != null && flags.IsKindOf(CustomBuildToolFlags.AllowBulkbuild))
						{
							cbFi.Data = null;
							custombuildBulkBuildSources.Add(cbFi);
							enablePartialBulkBuild = true;
						}
						else
						{
							cbFi.Data = null;
							custombuildLooseSources.Add(cbFi);
						}
					}
				}

				FileSet manualSource = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.manual.sourcefiles"));
				FileSet manualSourceCfg = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.manual.sourcefiles." + module.Configuration.System));


				if (manualSource != null || manualSourceCfg != null)
				{
					isReallyBulkBuild = true;

					manualSource.SetFileDataFlags(FileData.BulkBuild);
					manualSourceCfg.SetFileDataFlags(FileData.BulkBuild);

					FileSetUtil.FileSetAppendWithBaseDir(source, manualSource);
					FileSetUtil.FileSetAppendWithBaseDir(source, manualSourceCfg);

					FileSetUtil.FileSetInclude(excludedSources, cc_sources);
				}

				//This is the simple case where the user wants to bulkbuild the runtime.sourcefiles.
				if (FileSetUtil.FileSetExists(Project, PropGroupName("bulkbuild.sourcefiles")))
				{
					isReallyBulkBuild = true;

					if (custombuildBulkBuildSources != null && custombuildBulkBuildSources.Count > 0)
					{
						FileSet bbSourceFiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles"));

						foreach (var cbFi in custombuildBulkBuildSources)
						{
							bbSourceFiles.Include(cbFi);
						}
					}

					// See if any files have optionset with option "bulkbuild"=off/false. Exclude these files from bulkbuildsources and set partial bulkbuild to true.

					FileSet bulkbuildfiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles"));

					foreach (var pattern in bulkbuildfiles.Includes)
					{
						if (!String.IsNullOrEmpty(pattern.OptionSet))
						{
							OptionSet fileoptions = null;
							if (Project.NamedOptionSets.TryGetValue(pattern.OptionSet, out fileoptions))
							{
								string bulkbuildoption = null;
								if (fileoptions.Options.TryGetValue("bulkbuild", out bulkbuildoption))
								{
									if (bulkbuildoption != null && (bulkbuildoption.Equals("off", StringComparison.OrdinalIgnoreCase) || bulkbuildoption.Equals("false", StringComparison.OrdinalIgnoreCase)))
									{
										enablePartialBulkBuild = true;
										bulkbuildfiles.Excludes.Add(pattern);
									}
								}
							}
						}
					}

					// On Mac exclude *.mm files from the bulkbuild
					// Mono returns Platform Unix instead of MacOSX. Test for Unix as well. 
					// TODO: find a better way to detect MacOSX
					// Some versions of Runtime do not have MacOSX definition. Use integer value 6
					//if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX)
					if (module.Configuration.System == "iphone" || module.Configuration.System == "iphone-sim" || module.Configuration.System == "osx")
					{
						int bbSourceCount = 0;

						FileSet bbSourceFiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles"));
						if (!enablePartialBulkBuild)
						{
							bbSourceCount = bbSourceFiles.FileItems.Count;
						}
						FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.mm");
						FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.m");
						if (!enablePartialBulkBuild && (bbSourceCount != bbSourceFiles.FileItems.Count))
						{
							enablePartialBulkBuild = true;
							// Since the PropGroupName("bulkbuild.sourcefiles") is now modified and no longer contains the 
							// original *.mm and *.m, if this function get runs again (which can happen if people chain 
							// "projectize" and "projectize-test" targets on command line), we need the 
							// PropGroupName("enablepartialbulkbuild") property set to true as well so that the
							// enablePartialBulkBuild variable will be evaluated to true next time it is run and we will 
							// proceed to add the extra loose files.
							Project.Properties[PropGroupName("enablepartialbulkbuild")] = "true";
						}
					}

					//Loose = common + platform-specific - bulkbuild
					FileSet looseSourcefiles = null;
					if (enablePartialBulkBuild)
					{
						looseSourcefiles = new FileSet();
						if (custombuildLooseSources != null)
						{
							foreach (var cbFi in custombuildLooseSources)
							{
								looseSourcefiles.Include(cbFi);
							}
						}
						looseSourcefiles.IncludeWithBaseDir(cc_sources);
						looseSourcefiles.Exclude(FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles")));
					}

					string bbfiletemplate =   PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("bulkbuild.filename.template"),
											  PropertyUtil.GetPropertyOrDefault(Project, "bulkbuild.filename.template", "BB_%groupname%.cpp"));


					string outputFile = MacroMap.Replace(bbfiletemplate, module.Macros);

					// Check it template defines any directories:
					var subdir = Path.GetDirectoryName(outputFile);
					if (!String.IsNullOrEmpty(subdir))
					{
						bulkbuildOutputdir = Path.Combine(bulkbuildOutputdir, subdir);
						outputFile = Path.GetFileName(outputFile);
					}

					string templatePattern = Project.Properties["bulkbuild.outputdir.template"];
					if (!String.IsNullOrEmpty(templatePattern))
					{
						bulkbuildOutputdir = MacroMap.Replace(templatePattern, module.Macros);
					}

					string maxBulkbuildSize = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("max-bulkbuild-size"),
											  PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.max-bulkbuild-size", String.Empty));

					// Generate bulkbuild CPP file
					FileSet bbloosefiles;

					string header = (compiler.PrecompiledHeaders == CcCompiler.PrecompiledHeadersAction.UsePch && !module.UseForcedIncludePchHeader) ? module.PrecompiledHeaderFile : null;

					bool splitbydirs = Project.Properties.GetBooleanPropertyOrDefault(module.GroupName + ".bulkbuild.split-by-directories", 
						Project.Properties.GetBooleanPropertyOrDefault("eaconfig.bulkbuild.split-by-directories",false));

					var minsplitfiles = Project.Properties.GetPropertyOrDefault(module.GroupName + ".bulkbuild.min-split-files",
						Project.Properties.GetPropertyOrDefault("eaconfig.bulkbuild.min-split-files", "0"));

					var deviateMaxSizeAllowance = Project.Properties.GetPropertyOrDefault(module.GroupName + ".bulkbuild.deviate-maxsize-allowance",
						Project.Properties.GetPropertyOrDefault("eaconfig.bulkbuild.deviate-maxsize-allowance", "5"));

					FileSet bbFiles = EA.GenerateBulkBuildFiles.generatebulkbuildfiles.Execute(Project, module.GroupName + ".bulkbuild.sourcefiles", outputFile, bulkbuildOutputdir, maxBulkbuildSize, header, out bbloosefiles, splitByDirectories: splitbydirs, minSplitFiles: minsplitfiles, deviateMaxSizeAllowance: deviateMaxSizeAllowance);

					if (bbloosefiles != null && bbloosefiles.FileItems.Count > 0)
					{
						if (enablePartialBulkBuild)
						{
							if (looseSourcefiles.FileItems.Count == 0)
							{
								looseSourcefiles.BaseDirectory = bbloosefiles.BaseDirectory;
							}
							looseSourcefiles.Include(bbloosefiles, force: true);
						}
						else
						{
							source.Include(bbloosefiles, force: true);
						}
					}

					//Add bulkbuild CPP to source fileset
					FileSetUtil.FileSetAppend(source, bbFiles);
					if (String.IsNullOrEmpty(source.BaseDirectory))
					{
						source.BaseDirectory = bbFiles.BaseDirectory;
					}


					//Add loose files to source fileset 
					if (enablePartialBulkBuild)
					{
						if (looseSourcefiles.FileItems.Count > 0)
						{
							FileSetUtil.FileSetAppendWithBaseDir(source, looseSourcefiles);
						}

						if (custombuildBulkBuildSources != null)
						{
							foreach (var cbFi in custombuildBulkBuildSources)
							{
								excludedSources.Include(cbFi);
							}
						}
						FileSetUtil.FileSetInclude(excludedSources, cc_sources);
						FileSetUtil.FileSetExclude(excludedSources, looseSourcefiles);
					}
					else
					{
						//IMTODO: to be more accurate use sourcefiles or not?
						excludedSources.IncludeWithBaseDir(FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild.sourcefiles")));
					}
				}

				//This is the case where the user supplies bulkbuild filesets. This means that the runtime.bulkbuild.fileset Property exists.
				if (PropertyUtil.PropertyExists(Project, PropGroupName("bulkbuild.filesets")))
				{
					isReallyBulkBuild = true;

					if (custombuildBulkBuildSources != null && custombuildBulkBuildSources.Count > 0)
					{
						var bbfilesets = PropGroupValue("bulkbuild.filesets");

						FileSet bbCusomBuildSourceFiles = new FileSet();

						foreach (var cbFi in custombuildBulkBuildSources)
						{
							bbCusomBuildSourceFiles.Include(cbFi);
						}
						string customBbName = PropGroupName("_auto_custombuild_bb");
						Project.NamedFileSets[customBbName] = bbCusomBuildSourceFiles;
						Project.Properties[PropGroupName("bulkbuild.filesets")] = bbfilesets + Environment.NewLine + customBbName;
					}


					// On Mac exclude *.mm files from the bulkbuild
					// Mono returns Platform Unix instead of MacOSX. Test for Unix as well. 
					// TODO: find a better way to detect MacOSX
					// Some versions of Runtime do not have MacOSX definition. Use integer value 6
					if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX)
					{
						if (module.Configuration.System == "iphone" || module.Configuration.System == "iphone-sim" || module.Configuration.System == "osx")
						{
							int bbSourceCount = 0;

							foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
							{
								FileSet bbSourceFiles = FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles"));
								if (bbSourceFiles != null)
								{
									if (!enablePartialBulkBuild)
									{
										bbSourceCount = bbSourceFiles.FileItems.Count;
									}
									FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.mm");
									FileSetUtil.FileSetExclude(bbSourceFiles, bbSourceFiles.BaseDirectory + "/**.m");

									if (!enablePartialBulkBuild && (bbSourceCount != bbSourceFiles.FileItems.Count))
									{
										enablePartialBulkBuild = true;
									}
								}
								else
								{
									throw new BuildException($"{LogPrefix}Fileset '{PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles")}' specified in property '{PropGroupName("bulkbuild.filesets")}'='{Project.Properties[PropGroupName("bulkbuild.filesets")]}' does not exist.");
								}
							}
						}
					}
					//Loose = common + platform-specific - bulkbuild(multiple ones)
					FileSet looseSourcefiles = null;
					if (enablePartialBulkBuild)
					{
						looseSourcefiles = new FileSet();
						if (custombuildLooseSources != null)
						{
							foreach (var cbFi in custombuildLooseSources)
							{
								looseSourcefiles.Include(cbFi);
							}
						}

						FileSetUtil.FileSetInclude(looseSourcefiles, cc_sources);

						foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName("bulkbuild.filesets")]))
						{
							FileSetUtil.FileSetExclude(looseSourcefiles, FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles")));
						}
					}

					// Generate bulkbuild CPP file
					foreach (string bbFileSetName in StringUtil.ToArray(Project.Properties[PropGroupName(".bulkbuild.filesets")]))
					{
						string outputFile = "BB_" + module.GroupName + "_" + bbFileSetName + ".cpp";

						string maxBulkbuildSize = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName(bbFileSetName + ".max-bulkbuild-size"),
															 PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("max-bulkbuild-size"),
															 PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.max-bulkbuild-size", String.Empty)));

						FileSet bbloosefiles;

						var header = (compiler.PrecompiledHeaders == CcCompiler.PrecompiledHeadersAction.UsePch && !module.UseForcedIncludePchHeader) ? module.PrecompiledHeaderFile : null;

						bool splitbydirs = Project.Properties.GetBooleanPropertyOrDefault(module.GroupName + ".bulkbuild.split-by-directories",
							Project.Properties.GetBooleanPropertyOrDefault("eaconfig.bulkbuild.split-by-directories", false));

						var minsplitfiles = Project.Properties.GetPropertyOrDefault(module.GroupName + ".bulkbuild.min-split-files",
							Project.Properties.GetPropertyOrDefault("eaconfig.bulkbuild.min-split-files", "0"));

						var deviateMaxSizeAllowance = Project.Properties.GetPropertyOrDefault(module.GroupName + ".bulkbuild.deviate-maxsize-allowance",
							Project.Properties.GetPropertyOrDefault("eaconfig.bulkbuild.deviate-maxsize-allowance", "5"));

						FileSet bbFiles = EA.GenerateBulkBuildFiles.generatebulkbuildfiles.Execute(Project, module.GroupName + ".bulkbuild." + bbFileSetName + ".sourcefiles", outputFile, bulkbuildOutputdir, maxBulkbuildSize, header, out bbloosefiles, splitByDirectories: splitbydirs, minSplitFiles: minsplitfiles, deviateMaxSizeAllowance: deviateMaxSizeAllowance);

						//Add bulkbuild CPP to source fileset
						FileSetUtil.FileSetAppend(source, bbFiles);
						if (String.IsNullOrEmpty(source.BaseDirectory))
						{
							source.BaseDirectory = bbFiles.BaseDirectory;
						}


						if (bbloosefiles != null && bbloosefiles.FileItems.Count > 0)
						{
							if (enablePartialBulkBuild)
							{
								looseSourcefiles.Include(bbloosefiles, force: true);
							}
							else
							{
								source.Include(bbloosefiles, force: true);
							}
						}

						FileSetUtil.FileSetInclude(excludedSources, FileSetUtil.GetFileSet(Project, PropGroupName("bulkbuild." + bbFileSetName + ".sourcefiles")));
					}

					//Add loose files to source fileset 
					if (enablePartialBulkBuild)
					{
						FileSetUtil.FileSetAppendWithBaseDir(source, looseSourcefiles);

						if (custombuildBulkBuildSources != null)
						{
							foreach (var cbFi in custombuildBulkBuildSources)
							{
								excludedSources.Include(cbFi);
							}
						}

						FileSetUtil.FileSetInclude(excludedSources, cc_sources);
						FileSetUtil.FileSetExclude(excludedSources, looseSourcefiles);
					}
					else
					{
						excludedSources.Append(custombuildsources);
						FileSetUtil.FileSetInclude(excludedSources, cc_sources);
					}
				}
			}

			if (!isReallyBulkBuild)
			{
				//This is the non-bulkbuild case. Set up build sourcefiles.
				source.AppendWithBaseDir(custombuildsources);
				source.AppendWithBaseDir(cc_sources);
			}

			return source;
		}

		private void ReplaceAllTemplates(Module_Native module)
		{
			IEnumerable<Tool> moduleTools = module.Tools;
			IEnumerable<Tool> compilerSourceFilesCustomOptionTools =
				module.Cc?.SourceFiles.FileItems
						.Select(item => item.GetTool())
						.Where(tool => tool != null)
				?? Enumerable.Empty<Tool>();

			IEnumerable<Tool> assemblerSourceFilesCustomOptionTools =
				module.Asm?.SourceFiles.FileItems
						.Select(item => item.GetTool())
						.Where(tool => tool != null)
				?? Enumerable.Empty<Tool>();

			Tool[] allTools = moduleTools
				.Concat(compilerSourceFilesCustomOptionTools)
				.Concat(assemblerSourceFilesCustomOptionTools)
				.ToArray();

			if (!allTools.Any())
			{
				return;
			}

			// extend module macros with pass-throughs for source and object files,
			// these are resolved later on a per file basis
			MacroMap map = new MacroMap(inherit: module.Macros)
			{
				{ "objectfile", "%objectfile%" },
				{ "sourcefile", "%sourcefile%" }
			};

			foreach (Tool tool in allTools)
			{
				for (int i = 0; i < tool.Options.Count; i++)
				{
					tool.Options[i] = MacroMap.Replace
					(
						tool.Options[i].Replace("\"%pchheaderfile%\"", "%pchheaderfile%"), // pch header file macro expansion is already quoted, so removed quoted macro to prevernt double quotes
						map, 
						allowUnresolved: true // allow unresolved tokens at this point, we are likely parsing custom build commands that contains envvar evals (e.g. %errorlevel%)
					); 
				}
			}
		}

		private void AddSystemIncludes(CcCompiler tool)
		{
			if (tool != null)
			{
				bool prepend = Project.Properties.GetBooleanPropertyOrDefault(tool.ToolName + ".usepreincludedirs", false);

				var systemincludes = GetOptionString(tool.ToolName + ".includedirs", BuildOptionSet.Options).LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
				if (prepend)
					tool.IncludeDirs.InsertRange(0, systemincludes);
				else
					tool.IncludeDirs.AddUnique(systemincludes);

				foreach (var filetool in tool.SourceFiles.FileItems.Select(fi => fi.GetTool() as CcCompiler).Where(t => t != null))
				{
					if (prepend)
						filetool.IncludeDirs.InsertRange(0, systemincludes);
					else
						filetool.IncludeDirs.AddUnique(systemincludes);
				}
			}
		}

		private void AddSystemIncludes(AsmCompiler tool)
		{
			if (tool != null)
			{
				bool prepend = Project.Properties.GetBooleanPropertyOrDefault("as.usepreincludedirs", false);

				var systemincludes = GetOptionString("as.includedirs", BuildOptionSet.Options).LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
				if (prepend)
					tool.IncludeDirs.InsertRange(0, systemincludes);
				else
					tool.IncludeDirs.AddUnique(systemincludes);

				foreach (var filetool in tool.SourceFiles.FileItems.Select(fi => fi.GetTool() as AsmCompiler).Where(t => t != null))
				{
					if (prepend)
						filetool.IncludeDirs.InsertRange(0, systemincludes);
					else
						filetool.IncludeDirs.AddUnique(systemincludes);
				}
			}
		}

		private void SetupGeneratedAssemblyInfo(ProcessableModule module)
		{
			if (module.GetModuleBooleanPropertyValue("generateassemblyinfo"))
			{
				AssemblyInfoTask GeneratedAssemblyInfo = new AssemblyInfoTask();
				GeneratedAssemblyInfo.Project = this.Project;
				GeneratedAssemblyInfo.Company = module.GetPlatformModuleProperty("assemblyinfo.company");
				GeneratedAssemblyInfo.Copyright = module.GetPlatformModuleProperty("assemblyinfo.copyright");
				GeneratedAssemblyInfo.Product = module.GetPlatformModuleProperty("assemblyinfo.product");
				GeneratedAssemblyInfo.Title = module.GetPlatformModuleProperty("assemblyinfo.title");
				GeneratedAssemblyInfo.Description = module.GetPlatformModuleProperty("assemblyinfo.description") ?? module.Package.Release.Manifest.Summary;
				GeneratedAssemblyInfo.VersionNumber = module.GetPlatformModuleProperty("assemblyinfo.version");

				string cacheMode = module.Package.Project.Properties["package.SnowCache.mode"].TrimWhiteSpace();
				GeneratedAssemblyInfo.Deterministic = (cacheMode == "upload" || cacheMode == "uploadanddownload" || cacheMode == "download" || cacheMode == "forceupload");

				FileSet sourceFiles = null;
				if (module is Module_DotNet)
				{
					Module_DotNet dotnetModule = (module as Module_DotNet);
					GeneratedAssemblyInfo.AssemblyExtension = dotnetModule.Compiler.OutputExtension;
					sourceFiles = dotnetModule.Compiler.SourceFiles;
				}
				else if (module is Module_Native)
				{
					Module_Native nativeModule = (module as Module_Native);
					GeneratedAssemblyInfo.AssemblyExtension = nativeModule.Link.OutputExtension;
					sourceFiles = nativeModule.Cc.SourceFiles;
				}
				else
				{
					throw new Exception("Generated Assembly Info should only be generated for DotNet and managed Native Modules");
				}

				string generatedAssemblyInfoPath = GeneratedAssemblyInfo.WriteFile(module.BuildGroup.ToString(), module.Name, module.Configuration.Name, module.BuildType);

				string configBuildDir = Project.Properties["package.configbuilddir"];

				if (module.BuildType.IsFSharp)
				{
					// must prepend in order not to break F# modules which require the file with the main method to be added last
					Pattern pattern = PatternFactory.Instance.CreatePattern(generatedAssemblyInfoPath, Path.Combine(configBuildDir, module.GroupName));
					pattern.PreserveBaseDirValue = true;
					sourceFiles.Includes.Prepend(new FileSetItem(pattern));
				}
				else
				{
					sourceFiles.Include(generatedAssemblyInfoPath, baseDir: Path.Combine(configBuildDir, module.GroupName));
				}
			}
		}

		private void SetupComAssembliesAssemblies(DotNetCompiler compiler, Module_DotNet module)
		{
			if (FileSetUtil.FileSetExists(Project, PropGroupName("comassemblies")))
			{
				// This function can get run in parallel threads but we are having the task create
				// a temporary fileset and then destroy the fileset.  So we need to make sure the entire
				// task including deleting the fileset be all in one lock scope.
				NamedLock.Execute(Project, "task-generatemoduleinteropassemblies", () =>
				{
					new GenerateModuleInteropAssembliesTask(Project, module: module.Name, group: module.BuildGroup.ToString(),
						generateAssembliesFileset: "__private_com_net_wrapper_references").Execute();

					var comassemblies = FileSetUtil.GetFileSet(Project, "__private_com_net_wrapper_references");
					foreach (var inc in comassemblies.Includes)
					{
						inc.OptionSet = "copylocal";
					}

					compiler.Assemblies.AppendIfNotNull(comassemblies);
					Project.NamedFileSets.Remove("__private_com_net_wrapper_references");
				});
			}
		}

		private void SetupComAssembliesAssemblies(CcCompiler compiler, Module_Native module)
		{
			if (FileSetUtil.FileSetExists(Project, PropGroupName("comassemblies")))
			{
				if (module.IsKindOf(Module.Managed))
				{
					// This function can get run in parallel threads but we are having the task create
					// a temporary fileset and then destroy the fileset.  So we need to make sure the entire
					// task including deleting the fileset be all in one lock scope.
					NamedLock.Execute(Project, "task-generatemoduleinteropassemblies", () =>
					{
						new GenerateModuleInteropAssembliesTask(Project, module: module.Name, group: module.BuildGroup.ToString(),
							generateAssembliesFileset: "__private_com_net_wrapper_references").Execute();

						var comassemblies = FileSetUtil.GetFileSet(Project, "__private_com_net_wrapper_references");
						foreach (var inc in comassemblies.Includes)
						{
							inc.OptionSet = "copylocal";
						}

						compiler.Assemblies.AppendIfNotNull(comassemblies);
						Project.NamedFileSets.Remove("__private_com_net_wrapper_references");
					});
				}
				else
				{
					compiler.ComAssemblies.AppendWithBaseDir(FileSetUtil.GetFileSet(Project, PropGroupName("comassemblies")));
				}
			}
		}

		private void SetupWebReferences(DotNetCompiler compiler, Module_DotNet module)
		{
			if (OptionSetUtil.OptionSetExists(Project, PropGroupName("webreferences")))
			{
				// This function can get run in parallel threads but we are having the task create
				// a temporary fileset and then destroy the fileset.  So we need to make sure the entire
				// task including deleting the fileset be all in one lock scope.
				NamedLock.Execute(Project, "task-generatemodulewebreferences", () =>
				{
					new GenerateModuleWebReferencesTask(Project, module: module.Name, group: module.BuildGroup.ToString(),
						output: "__private_generated_sources_fileset").Execute();

					compiler.SourceFiles.AppendIfNotNull(FileSetUtil.GetFileSet(Project, "__private_generated_sources_fileset"));
					Project.NamedFileSets.Remove("__private_generated_sources_fileset");
				});

				// The system.web.dll and System.web.services.dll must be included so the code can compile
				//  Dot Net frameworks version 3.0 and higher do not contain macorlib.dll, System.dll in reference directory
				compiler.Assemblies.Include("System.Web.dll", true);
				compiler.Assemblies.Include("System.Web.Services.dll", true);
			}
		}

		private void SetupWebReferences(CcCompiler compiler, Module_Native module)
		{
			if (module.IsKindOf(Module.Managed) && OptionSetUtil.OptionSetExists(Project, PropGroupName("webreferences")))
			{
				GenerateModuleWebReferencesTask task = new GenerateModuleWebReferencesTask(Project, 
					module: module.Name, group: module.BuildGroup.ToString());
				task.Execute();
				compiler.IncludeDirs.Add(new PathString(task.WebCodeDir, PathString.PathState.Directory));
			}
		}

		PathString GetObjectFile(FileItem fileitem, Tool tool, Module module, CommonRoots<FileItem> commonroots, ISet<string> duplicates, out uint nameConflictFlag)
		{
			nameConflictFlag = 0;
			OptionDictionary options = BuildOptionSet.Options;
			OptionSet fileoptions = null;
			if (!String.IsNullOrEmpty(fileitem.OptionSetName))
			{
				if (Project.NamedOptionSets.TryGetValue(fileitem.OptionSetName, out fileoptions))
				{
					options = fileoptions.Options;
				}
			}
			// Get commonRoot:
			string prefix = commonroots.GetPrefix(fileitem.Path.Path);
			string root = commonroots.Roots[prefix];

			string intermediatedir = module.IntermediateDir.Path + Path.DirectorySeparatorChar;

			// To prevent very long obj file paths make sure that part after root is not too long
			string rel = fileitem.Path.Path.Substring(root.Length);
			const int vsoverhead = 30;
			if (intermediatedir.Length + rel.Length >= (260 - vsoverhead) || intermediatedir.Length + Path.GetDirectoryName(rel).Length >= (248-vsoverhead))
			{
				rel = Path.Combine("src",Path.GetFileName(rel));
			}

			string basepath = Path.Combine(intermediatedir, rel);


			if (!duplicates.Add(basepath))
			{
				basepath = EA.CPlusPlusTasks.CompilerBase.GetOutputBasePath(fileitem, intermediatedir, String.Empty);
				nameConflictFlag = FileData.ConflictObjName;
			}

			//IMDODO !!!!
			// Disabled normalization because some paths can get too long (>260 characters)...  I'm looking at you NCAA xenon-vs-opt-gamereflect config...
			//             return PathString.MakeNormalized(basepath + (GetOptionString(tool.ToolName + ".objfile.extension", options) ?? ".obj"));
			return PathString.MakeNormalized(basepath + (GetOptionString(tool.ToolName + ".objfile.extension", options) ?? ".obj"), PathString.PathParam.NormalizeOnly);
		}
#endregion

		#region Helper functions

		Dictionary<string, List<Tuple<string, string>>> _toolTemplatesLookup = new Dictionary<string, List<Tuple<string, string>>>();

		protected void SetToolTemplates(Tool tool, OptionSet options = null)
		{
			if (tool != null)
			{
				string filter = (tool.ToolName == "asm" ? "as" : tool.ToolName) + ".template.";

				List<Tuple<string, string>> toolTemplates;
				if (!_toolTemplatesLookup.TryGetValue(filter, out toolTemplates))
				{
					toolTemplates = new List<Tuple<string, string>>();
					_toolTemplatesLookup.Add(filter, toolTemplates);

					foreach (var prop in Project.Properties.GetGlobalProperties().Where(p => p.Name.StartsWith(filter, StringComparison.Ordinal)))
						toolTemplates.Add(new Tuple<string, string>(prop.Name, prop.Value));
				}

				foreach (var kv in toolTemplates)
					tool.Templates[kv.Item1] = kv.Item2;

				var set = options ?? BuildOptionSet;

				foreach (var entry in set.Options.Where(e => e.Key.StartsWith(filter, StringComparison.Ordinal)))
				{
					tool.Templates[entry.Key] = entry.Value;
				}
			}
		}

		private string GetOutputName(IModule module)
		{
			return PropGroupValue("outputname", module.Name);
		}

		private void GetOutputNameAndExtension(string path, string name, out string outputname, out string extension)
		{
			//Here account for the case when module name has dots in it.
			var file = Path.GetFileName(path);
			if (file.StartsWith(name, StringComparison.OrdinalIgnoreCase))
			{
				int dotind = file.IndexOf('.',name.Length);
				if (dotind < 0)
				{
					outputname = file;
					extension = String.Empty;
				}
				else
				{
					outputname = file.Substring(0, dotind);
					extension = file.Substring(dotind);
				}
			}
			else
			{
				outputname = Path.GetFileNameWithoutExtension(file);
				extension = Path.GetExtension(file);
			}
		}

		protected PathString GetIntermediateDir(IModule module)
		{
			return PathString.MakeNormalized(ModulePath.Private.GetModuleIntermedidateDir(Project, module.Name, module.BuildGroup.ToString()));
		}

		protected PathString GetGenerationDir(IModule module)
		{
			return PathString.MakeNormalized(ModulePath.Private.GetModuleGenerationDir(Project, module.Name, module.BuildGroup.ToString()));
		}

		protected void SetLinkOutput(IModule module, out string outputname, out string outputextension, out PathString linkoutputdir, out string replacename)
		{
			// set paths
			ModulePath binPath = ModulePath.Private.GetModuleBinPath(Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), BuildType);
			linkoutputdir = PathString.MakeNormalized(binPath.OutputDir);
			outputname = binPath.OutputName;
			outputextension = binPath.Extension;
			replacename = binPath.OutputNameToken;

			if (module is Module_DotNet)
			{
				Module_DotNet dnModule = module as Module_DotNet;
				if (dnModule.TargetFrameworkFamily == DotNetFrameworkFamilies.Core && (outputextension == ".exe" || String.IsNullOrEmpty(outputextension)))
				{
					// A .Net Core build even with exe target actually creates a .dll assembly output.  During MSBuild
					// a separate CreateAppHost msbuild target will combine this .dll with apphost.exe from SDK to
					// create the final .exe.  Also, you cannot run this .exe without the .dll, but you can run the
					// .dll build output using "dotnet [x].dll" without the .exe, so for now, if it is a .Net Core
					// build, we set the extension back to .dll.
					outputextension = ".dll";
				}
			}

			module.OutputDir = linkoutputdir;

			module.Macros.Update("outputname", binPath.OutputNameToken);
			module.Macros.Update("outputdir", module.OutputDir.Path);
			module.Macros.Update("outputbindir", linkoutputdir.Path);
			module.Macros.Update("output", binPath.FullPath);
		}

#endregion

		#region Dispose interface implementation

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		void Dispose(bool disposing)
		{
			if (!this._disposed)
			{
				if (disposing)
				{
					// Dispose managed resources here
				}
			}
			_disposed = true;
		}
		private bool _disposed = false;

#endregion

		internal class CustomBuildToolFlags : BitMask
		{
			internal CustomBuildToolFlags(uint type = 0) : base(type) { }

			internal const uint AllowBulkbuild = 1;
		}

		private static readonly HashSet<string> HEADER_EXTENSIONS = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { ".h", ".hpp", ".hxx" };
	}
}
