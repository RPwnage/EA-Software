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
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
	[TaskName("verify-src-includes")]
	sealed class VerifySourceIncludesTask : Task
	{
		#if (LOCAL_DEBUG_SWITCH)
		private int mCurrIndent = 0;
		#endif

		private int mTotalIrregularities = 0;
		IList<string> mRemoveFromGlobalIncludes = new List<string>();

		protected override void ExecuteTask()
		{
			string removeIncldues = Project.Properties.GetPropertyOrDefault("verify-src-includes.remove-from-includes","");
			if (!String.IsNullOrEmpty(removeIncldues))
			{
				mRemoveFromGlobalIncludes = removeIncldues.Split(new char[]{';'});
			}

			BuildGraph buildGraph = Project.BuildGraph();

			foreach (var module in buildGraph.SortedActiveModules.Reverse())
			{
				#if (LOCAL_DEBUG_SWITCH)
				Console.WriteLine("{0}Processing package ({1}) - module ({2})", "".PadRight(mCurrIndent), module.Package.Name, module.Name);
				mCurrIndent += 4;
				#endif

				CheckSourceFilesInModule(module);

				#if (LOCAL_DEBUG_SWITCH)
				mCurrIndent -= 4;
				#endif
			}

			if (mTotalIrregularities != 0)
			{
				throw new BuildException(String.Format("Found {0} source file with include irregularities", mTotalIrregularities));
			}
		}

		private IEnumerable<PathString> GetOptionSetIncludes(OptionSet buildOptionSet, string optionSetName)
		{
			IEnumerable<PathString> outList = null;
			if (buildOptionSet != null)
			{
				string includedirs = OptionSetUtil.GetOption(buildOptionSet, optionSetName);
				if (!String.IsNullOrWhiteSpace(includedirs))
				{
					outList = includedirs.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
				}
			}
			return outList;
		}

		private void CheckSourceFilesInModule(IModule module)
		{
			Module_Native nmod = module as Module_Native;
			if (nmod == null)
			{
				#if (LOCAL_DEBUG_SWITCH)
				Console.WriteLine("{0}Skipping non-native module.", "".PadRight(mCurrIndent));
				#endif
			}
			else
			{
				#if (LOCAL_DEBUG_SWITCH)
				Console.WriteLine("{0}Native Module", "".PadRight(mCurrIndent));
				#endif

				foreach (Tool tool in nmod.Tools)
				{
					FileSet fs = null;
					List<PathString> toolIncDirs = null;
					List<PathString> systemincludes = new List<PathString>();

					OptionSet buildOptionSet = OptionSetUtil.GetOptionSet(Project, nmod.BuildType.Name);
					if (buildOptionSet == null)
					{
						buildOptionSet = OptionSetUtil.GetOptionSet(Project, nmod.BuildType.BaseName);
					}

					// We'll just use the include dirs from the tool that has been constructed.  During build graph construction,
					// The tool's include dir should have been populated with all the dependency's include.  There should be no
					// need for us to do this again.

					if (tool is CcCompiler)
					{
						CcCompiler ccTool = tool as CcCompiler;
						fs = ccTool.SourceFiles;
						toolIncDirs = ccTool.IncludeDirs;
						IEnumerable<PathString> incs = GetOptionSetIncludes(buildOptionSet, "cc.includedirs");
						if (incs != null)
						{
							foreach (PathString ph in incs)
							{
								if (!systemincludes.Contains(ph))
								{
									systemincludes.Add(ph);
								}
							}
						}
						incs = GetOptionSetIncludes(buildOptionSet, "cc.system-includedirs");
						if (incs != null)
						{
							foreach (PathString ph in incs)
							{
								if (!systemincludes.Contains(ph))
								{
									systemincludes.Add(ph);
								}
							}
						}
					}
					else if (tool is AsmCompiler)
					{
						AsmCompiler asmTool = tool as AsmCompiler;
						fs = asmTool.SourceFiles;
						toolIncDirs = asmTool.IncludeDirs;
						IEnumerable<PathString> incs = GetOptionSetIncludes(buildOptionSet, "as.includedirs");
						if (incs != null)
						{
							foreach (PathString ph in incs)
							{
								if (!systemincludes.Contains(ph))
								{
									systemincludes.Add(ph);
								}
							}
						}
						incs = GetOptionSetIncludes(buildOptionSet, "as.system-includedirs");
						if (incs != null)
						{
							foreach (PathString ph in incs)
							{
								if (!systemincludes.Contains(ph))
								{
									systemincludes.Add(ph);
								}
							}
						}
					}

					if (fs != null && toolIncDirs != null)
					{
						List<string> depFiles = new List<string>();

						foreach (var fileitem in fs.FileItems)
						{
							#if (LOCAL_DEBUG_SWITCH)
							Console.WriteLine("{0}src: {1}", "".PadRight(mCurrIndent),Path.GetFileName(fileitem.FileName));
							#endif

							// Get the .dep file for this source file.

							EA.Eaconfig.Core.FileData filedata = fileitem.Data as EA.Eaconfig.Core.FileData;
							string objPath = null;
							if (filedata != null && filedata.ObjectFile != null && !String.IsNullOrEmpty(filedata.ObjectFile.Path))
							{
								objPath = filedata.ObjectFile.Path;
							}
							else
							{
								string buildDir = nmod.IntermediateDir.Normalize().Path;
								string outputBasePath = EA.CPlusPlusTasks.CompilerBase.GetOutputBasePath(fileitem, buildDir, fileitem.BaseDirectory ?? fs.BaseDirectory);
								objPath = outputBasePath + EA.CPlusPlusTasks.CompilerBase.DefaultObjectFileExtension;
							}

							string depFile = Path.ChangeExtension(objPath, ".dep");

							if (System.IO.File.Exists(depFile))
							{
								#if (LOCAL_DEBUG_SWITCH)
								Console.WriteLine("{0}dep: {1}", "".PadRight(mCurrIndent), depFile);
								#endif
								depFiles.Add(depFile);
							}
							else
							{
								module.Package.Project.Log.ThrowWarning
								(
									Log.WarningId.UsingImproperIncludes,
									Log.WarnLevel.Normal,
									"INTERNAL ERROR: Unable to find expected dependency file {0}", depFile
								);
							}
						}

						if (depFiles.Count() > 0)
						{
							VerifyModuleDepFilesWithIncludes(nmod, fs.BaseDirectory, depFiles, toolIncDirs, systemincludes);
						}
					}
				}
			}
		}

		private void VerifyModuleDepFilesWithIncludes(Module_Native module, string moduleBaseDir, List<string> depFiles, List<PathString> toolIncDirs, List<PathString> systemIncludes)
		{
			string nBaseDir = PathNormalizer.Normalize(moduleBaseDir,true);

			List<PathString> unusedIncludes = new List<PathString>();
			foreach (var ph in toolIncDirs)
			{
				// Ignore the system includes
				if (!systemIncludes.Contains(ph))
					unusedIncludes.Add(ph);
			}

			foreach (var depFile in depFiles)
			{
				using (StreamReader file = new StreamReader(depFile))
				{
					Log.Status.WriteLine(LogPrefix + module.Package.Name + " - " + module.Name + " - " + Path.GetFileName(depFile));
					string line;
					int lineCount=0;
					while ((line=file.ReadLine()) != null)
					{
						// We skip the first 2 lines (first line is compiler, second line is compiler switches, rest is dependency includes
						++lineCount;
						if (lineCount <= 2)
							continue;

						string npath = PathNormalizer.Normalize(line,true);

						bool foundInclude=false;
						foreach (var incPath in toolIncDirs)
						{
							string nIncPath = incPath.NormalizedPath;

							if (mRemoveFromGlobalIncludes.Contains(nIncPath))
							{
								unusedIncludes.Remove(incPath);
								continue; 
							}

							if (npath.StartsWith(nIncPath, StringComparison.OrdinalIgnoreCase))
							{
								foundInclude = true;

								if (unusedIncludes.Contains(incPath))
								{
									unusedIncludes.Remove(incPath);
								}

								// We could possibly just break out from here.   But in case we have following scenarios in the
								// include list:
								//    C:\packages\pkg1\includes
								//    C:\packages\pkg1\includes\ABC
								//    C:\packages\pkg1\includes\DEF
								// being checked against include file C:\packages\pkg1\includes\ABC\test.h
								// Where this include file can be matched with multiple include path, we
								// continue with the loop so that we don't have false positive in the unusedIncludes list.
							}
						}

						// Double check that the path isn't from the module itself.  It should be OK to have
						// relative path to the package itself.  Although, this could be easily setup in
						// package's build script itself.
						if (!foundInclude && !String.IsNullOrEmpty(nBaseDir))
						{
							if (npath.StartsWith(nBaseDir, StringComparison.OrdinalIgnoreCase))
							{
								foundInclude = true;
								break;
							}
						}

						if (!foundInclude)
						{
							string srcFileName = Path.GetFileName(depFile);
							srcFileName = srcFileName.Substring(0, srcFileName.Length - EA.CPlusPlusTasks.Dependency.DependencyFileExtension.Length);
							Log.ThrowWarning
							(
								Log.WarningId.UsingImproperIncludes, Log.WarnLevel.Normal,
								"Package ({0}), Module ({1}), File ({2}) has this include that does not have proper build script dependency set: {3}",
								module.Package.Name, module.Name, srcFileName, line
							);

							if (!Log.IsWarningEnabled(Log.WarningId.UsingImproperIncludes, Log.WarnLevel.Normal))
							{
								++mTotalIrregularities;
							}
						}
					}
				}
			}

			if (unusedIncludes.Count() > 0)
			{
				StringBuilder bdr = new StringBuilder();
				bdr.AppendLine(String.Format("Package ({0}) - Module ({1}) has these unused includedirs:", module.Package.Name, module.Name));
				foreach (var inc in unusedIncludes)
				{
					bdr.AppendLine(String.Format("    {0}", inc.Path));
				}

				FindExcessPackageDependencies(module, unusedIncludes, bdr);

				Log.ThrowWarning(Log.WarningId.FoundUnusedIncludes, Log.WarnLevel.Normal, bdr.ToString());
				if (!Log.IsWarningEnabled(Log.WarningId.FoundUnusedIncludes, Log.WarnLevel.Normal))
				{
					++mTotalIrregularities;
				}
			}

		}

		void FindExcessPackageDependencies(Module module, List<PathString> unusedIncludes, StringBuilder builder)
		{
			// This function examines all dependents of the provided module and sees if all 
			// of its provided include directories, ie:
			//     package.<package>.includedirs
			//     package.<package>.<module>.includedirs
			// are unreferenced. If they are all unreferenced then the package can possibly
			// be moved from use/build dependencies to linkdependencies.

			List<string> unreferencedPackages = new List<string>();

			foreach (Dependency<IModule> dependent in module.Dependents)
			{
				IModule dependentModule = dependent.Dependent;
				string packageIncludes = Project.Properties[string.Format("package.{0}.includedirs", dependentModule.Package.Name)] ?? "";
				string moduleIncludes = Project.Properties[string.Format("package.{0}.{1}.includedirs", dependentModule.Package.Name, dependentModule.Name)] ?? "";
				string includes = string.Join(
					System.Environment.NewLine, 
					packageIncludes, 
					moduleIncludes);

				List<PathString> dependentIncludeDirs = new List<PathString>();

				dependentIncludeDirs.AddRange(includes.LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct());

				bool allIncludesUnreferenced = true;

				foreach (PathString packageIncludePath in dependentIncludeDirs)
				{
					if (unusedIncludes.Find(path => path == packageIncludePath) == null)
					{
						allIncludesUnreferenced = false;
						break;
					}
				}

				if (allIncludesUnreferenced)
				{
					unreferencedPackages.Add(dependentModule.Package.Name);
				}
			}

			if (unreferencedPackages.Count > 0)
			{
				builder.AppendLine();
				builder.AppendLine(String.Format("Package ({0}) - Module ({1}) has the following packages listed as use/build dependencies, but does not reference any include files:", module.Package.Name, module.Name));

				foreach (string package in unreferencedPackages.Distinct())
				{
					builder.Append("    ").AppendLine(package);
				}

				builder.AppendLine().AppendLine(String.Format("It might be possible to move these to linkdependencies for better build system performance.", module.Package.Name, module.Name));
			}
		}
	}
}

