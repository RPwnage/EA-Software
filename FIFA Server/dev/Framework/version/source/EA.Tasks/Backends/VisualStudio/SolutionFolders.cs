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
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
	class SolutionFolders
	{
		public class SolutionFolder
		{
			public readonly PathString PathName;
			public readonly string PackageName;
			public readonly string FolderName;
			public readonly Guid Guid;
			public SolutionFolder Parent;

			public readonly IDictionary<string, SolutionFolder> children = new Dictionary<string, SolutionFolder>(StringComparer.InvariantCultureIgnoreCase);
			public readonly List<PathString> FolderFiles = new List<PathString>();
			public readonly IDictionary<Guid, VSProjectBase> FolderProjects = new Dictionary<Guid, VSProjectBase>();

			public SolutionFolder(string package, PathString pathname)
			{
				PathName = pathname;
				PackageName = package;
				FolderName = PathName.Path.Trim(TRIM_CHARS).ToArray(SPLIT_FOLDER_CHARS).LastOrDefault() ?? PathName.Path;

				Guid = Hash.MakeGUIDfromString("Solution Folder" + PathName.Path);
			}

			public void AddChild(SolutionFolder child)
			{
				SolutionFolder existing_entry;

				if (children.TryGetValue(child.PathName.Path, out existing_entry))
				{
					if (existing_entry.Parent != this)
					{
					   // string parenName = children[child.PathName].Parent == null ? String.Empty : children[child.PathName].Parent.PathName;
					   // Console.WriteLine("ERROR: Solution Folder '{0}' has different parent Solution Folders: {1}, {2}", child.PathName, PathName, parenName);
					}

				}
				else
				{
					child.Parent = this;
					children.Add(child.PathName.Path, child);

				}
			}

			public void AddFile(PathString file)
			{
				if (!FolderFiles.Contains(file))
				{
					FolderFiles.Add(file);
				}
			}

			public bool IsEmpty
			{
				get
				{
					if (_isEmpty == null)
					{
						if (FolderFiles.Count > 0 || FolderProjects.Count > 0 || children.Any(child => !child.Value.IsEmpty))
						{
							_isEmpty = false;
						}
						else
						{
							_isEmpty = true;
						}
					}
					return (bool)_isEmpty;
				}
			}
			private bool? _isEmpty = null;
		}


		public SolutionFolders(string logPrefix)
		{
			LogPrefix = logPrefix;
		}

		public class ModuleGeneratorProcessor
		{
			public ModuleGeneratorProcessor(IDictionary<string, ModuleGenerator> generators)
			{
				Generators = generators;
				PackagePlusModuleLookup = new Dictionary<string, ModuleGenerator>();
				foreach (var g in generators.Values)
				{
					// We need to add build group info in the key.  We still have old style package where the package
					// build script contains no build module specification (in which case module name is just package name)
					// However, those packages can still have group (ie runtime/tool/test/example) specific module.
					// So we need to include build group in the key or we would run into error about key already added.
					// See following unit tests for example:
					//		//packages/test_nant/dev/tests/CMUnitTests/no_modules_CS_setup/
					//		//packages/test_nant/dev/tests/CMUnitTests/no_modules_MCpp_setup/
					// g.Modules are list of the same module from different configs. So they should have same build group.
					PackagePlusModuleLookup.Add(g.Package.Name + "|" + g.Modules.FirstOrDefault().BuildGroup + "|" + g.ModuleName, g);
				}
			}

			internal IDictionary<string, ModuleGenerator> Generators;
			internal Dictionary<string, ModuleGenerator> PackagePlusModuleLookup = new Dictionary<string, ModuleGenerator>();
		}

		public void ProcessProjects(Project project, IEnumerable<ModuleGenerator> moduleGenerators, ModuleGeneratorProcessor processor)
		{
			var package_folder = new Dictionary<string, SolutionFolders.SolutionFolder>();
			var module_folder = new Dictionary<string, SolutionFolders.SolutionFolder>();
			HashSet<string> propertyProcessed = new HashSet<string>();
			var packageName = project.Properties["package.name"];

			// This is an optimization to get rid of millions of ".Where"-calls further down in the nested for-loops

			// Fill folder files content:
			foreach (var folder in Folders.Values.Where(f => f.PackageName == packageName))
			{
				string fpath = folder.PathName.Path.Replace(Path.DirectorySeparatorChar, '.');
				string propertyName = String.Format("solution.folder.packages.{0}", fpath);
				if (!project.Properties.Contains(propertyName))
				{
					// Check the leaf folder name for backward compatibility
					string leafFolder = folder.FolderName;
					propertyName = String.Format("solution.folder.packages.{0}", leafFolder);
					if (!project.Properties.Contains(propertyName))
					{
						continue;
					}
					else if (propertyProcessed.Contains(propertyName))
					{
						// If we're trying another property for backward compatibility purposes, make sure we haven't already processed 
						// the packages listed this property!  Otherwise, we're just going to get erroneous warnings about attempting to 
						// re-add the same module (already in leafFolder) to fpath (the one we're currently processing).
						continue;
					}
				}
				string packages = project.Properties[propertyName];
				if (!propertyProcessed.Contains(propertyName))
				{
					propertyProcessed.Add(propertyName);
				}

				if (!String.IsNullOrEmpty(packages))
				{
					// Collect per package and per module definitions:
					foreach (string pname in packages.Trim(TRIM_CHARS).ToArray(SPLIT_CHARS))
					{
						var packagemodules = pname.ToArray(SPLIT_FOLDER_CHARS);

						if (packagemodules.Count == 1)
						{
							var generators = moduleGenerators.Where(gen => gen.Package.Name == packagemodules[0]).Cast<VSProjectBase>();

							SetFolders(project.Log, folder, generators, package_folder);
						}
						else if (packagemodules.Count > 1)
						{
							IEnumerable<ModuleGenerator> generators = Enumerable.Empty<ModuleGenerator>();
							// See if second parameter is a group name.
							BuildGroups group;
							if (packagemodules.Count > 2 && Enum.TryParse<BuildGroups>(packagemodules[1].TrimWhiteSpace(), out group))
							{
								var modules = packagemodules.Skip(2);
								generators = moduleGenerators.Where(gen => gen.Package.Name == packagemodules[0] && modules.Contains(gen.ModuleName) && gen.Modules.FirstOrDefault() != null && gen.Modules.FirstOrDefault().BuildGroup == group).Cast<VSProjectBase>();
							}
							else
							{
								List<ModuleGenerator> generatorList = null;
								foreach (var m in packagemodules.Skip(1))
								{
									// If there are no buildgroup info, it is runtime by default.
									ModuleGenerator gen;
									if (!processor.PackagePlusModuleLookup.TryGetValue(packagemodules[0] + "|runtime|" + m, out gen))
										continue;

									if (generatorList == null)
									{
										generatorList = new List<ModuleGenerator>();
										generators = generatorList;
									}
									generatorList.Add(gen);
								}
							}

							if (generators.Count() == 0 && _nonGeneratedProjectMap.ContainsKey(pname) == false)
							{
								_nonGeneratedProjectMap.Add(pname, folder);
							}

							SetFolders(project.Log, folder, generators, module_folder);
						}
					}
				}
			}

			// module generators mappings override package level mappings.

			foreach (var entry in module_folder)
			{
				package_folder[entry.Key] = entry.Value;
			}
			// Assign solution folders:
			foreach (var entry in package_folder)
			{
				VSProjectBase generator = null;
				ModuleGenerator generatorTemp;
				if (processor.Generators.TryGetValue(entry.Key, out generatorTemp))
					generator = generatorTemp as VSProjectBase;

				if (generator != null && !entry.Value.FolderProjects.ContainsKey(generator.ProjectGuid))
				{
					SolutionFolder otherfolder;
					if (!_vsProjectFolderMap.TryGetValue(generator.ProjectGuid, out otherfolder))
					{
						entry.Value.FolderProjects.Add(generator.ProjectGuid, generator);
						_vsProjectFolderMap.Add(generator.ProjectGuid, entry.Value);
					}
					else
					{
						if (otherfolder.PathName.Path != entry.Value.PathName.Path)
						{
							project.Log.Warning.WriteLine(LogPrefix + "Can't add project '{0}' to different solution folders: '{1}' and '{2}'. Using folder '{1}'.", generator.Name, otherfolder.PathName.Path, entry.Value.PathName.Path);
						}
					}
				}
			}
		}

		// A less than perfect way to attempt to find the correct solution folder for a pregenerated project
		public SolutionFolder GetFolderForPreGeneratedProject(IModule module)
		{
			SolutionFolder folder = null;
			foreach(var mappings in _nonGeneratedProjectMap)
			{
				if (mappings.Key.ToLower().Contains(module.Name.ToLower()))
				{
					folder = mappings.Value;
					break;
				}
			}
			return folder;
		}

		private void SetFolders(Log log, SolutionFolder folder, IEnumerable<ModuleGenerator> generators, IDictionary<string, SolutionFolders.SolutionFolder> map)
		{
			foreach (var gen in generators)
			{
				SolutionFolder otherfolder;
				if (!map.TryGetValue(gen.Key, out otherfolder))
				{
					map.Add(gen.Key, folder);
				}
				else
				{
					if (otherfolder.PathName.Path != folder.PathName.Path)
					{
						log.Warning.WriteLine(LogPrefix + "Can't add project {0} to different solution folders: '{1}' and '{2}'. Using folder '{1}'.", gen.Name, otherfolder.PathName.Path, folder.PathName.Path);
					}
				}
			}
		}

		public void ProcessFolders(Project project)
		{
			ParseFolderNames(project);
		}

		private bool ParseFolderNames(Project project)
		{
			bool hasFolders = false;
			if (project.Properties.Contains("solution.folders"))
			{
				string folders = project.Properties["solution.folders"];
				string packageName = project.Properties["package.name"];

				if (!String.IsNullOrEmpty(folders))
				{
					string[] fpaths = folders.Split(SPLIT_CHARS);
					foreach (string fpath in fpaths)
					{
						ProcessFolderPath(packageName, fpath);
					}
				}

				// Fill folder files content:
				foreach (var folder in new List<SolutionFolders.SolutionFolder>(Folders.Values))
				{
					string fpath = folder.PathName.Path.Replace(Path.DirectorySeparatorChar, '.').Replace(PathNormalizer.AltDirectorySeparatorChar, '.');
					string filesetName = String.Format("solution.folder.{0}", fpath);

					if (!AddFileSet(project, packageName, folder, filesetName))
					{
						// For backward compatibility, check leaf folder in the path
						// and match that to solution.folder.{0}
						// FolderPaths should have contained full path of parent folders
						// already, so checking leaf folder is good enough
						string leafFolder = folder.FolderName;
						filesetName = String.Format("solution.folder.{0}", leafFolder);
						AddFileSet(project, packageName, folder, filesetName);
					}
				}
			}

			return hasFolders;
		}

		private bool AddFileSet(Project project, string package, SolutionFolder folder, string filesetName)
		{
			FileSet fs;
			var filefolder = folder;

			if (project.NamedFileSets.TryGetValue(filesetName, out fs))
			{
				var normalizedFileSetBaseDir = PathNormalizer.Normalize(fs.BaseDirectory, false);
				foreach (FileItem fn in fs.FileItems)
				{
					var normalizedBaseDir = fn.BaseDirectory==null? normalizedFileSetBaseDir : PathNormalizer.Normalize(fn.BaseDirectory, false);

					string filedir = Path.GetDirectoryName(fn.Path.Path);
					if (!String.IsNullOrEmpty(normalizedBaseDir) && !String.IsNullOrEmpty(filedir) && filedir.StartsWith(normalizedBaseDir))
					{
						string folderpath = Path.Combine(folder.PathName.Path, filedir.Substring(normalizedBaseDir.Length).TrimLeftSlash());

						if (filefolder.PathName.Path != folderpath)
						{
							filefolder = ProcessFolderPath(package, folderpath);
						}
					}
					else
					{
						filefolder = folder;
					}

					filefolder.AddFile(fn.Path);
				}
				return true;
			}
			return false;
		}

		private SolutionFolder ProcessFolderPath(string package, string fpath)
		{
			SolutionFolder folderPath = null;
			if (!String.IsNullOrEmpty(fpath))
			{
				StringBuilder currentFullPathBuilder = new StringBuilder();
				string parentFullPath = null;
				string[] folderNames = fpath.Trim(TRIM_CHARS).Split(SPLIT_FOLDER_CHARS);
				for (int i = 0; i < folderNames.Length; i++)
				{
					if (!String.IsNullOrEmpty(folderNames[i]))
					{
						currentFullPathBuilder.Append(folderNames[i]);
						string currentFullPath = currentFullPathBuilder.ToString();

						if (!Folders.TryGetValue(currentFullPath, out folderPath))
						{
							folderPath = new SolutionFolder(package, PathString.MakeNormalized(currentFullPath, PathString.PathParam.NormalizeOnly));
							Folders.Add(currentFullPath, folderPath);
						}
						if (i > 0 && parentFullPath != null)
						{
							var parentFolder = Folders[parentFullPath];
							parentFolder.AddChild(folderPath);
						}
						parentFullPath = currentFullPath;
						currentFullPathBuilder.Append(Path.DirectorySeparatorChar);
					}
				}
			}
			return folderPath;
		}

		public SolutionFolder AddSolutionFolder(string name)
		{
			SolutionFolder folder = new SolutionFolder(string.Empty, PathString.MakeNormalized(name));
			Folders.Add(name, folder);
			return folder;
		}

		public readonly Dictionary<string, SolutionFolder> Folders = new Dictionary<string, SolutionFolder>(StringComparer.CurrentCultureIgnoreCase);

		private readonly Dictionary<string, SolutionFolder> _nonGeneratedProjectMap = new Dictionary<string, SolutionFolder>();
		private readonly  Dictionary<Guid, SolutionFolder> _vsProjectFolderMap = new Dictionary<Guid, SolutionFolder>();

		private static readonly char[] SPLIT_CHARS = new char[] { '\n', '\r', ' ', ',', ';' };
		private static readonly char[] SPLIT_FOLDER_CHARS = new char[] { '\\', '/' };
		private static readonly char[] TRIM_CHARS = new char[] { '\"', '\n', '\r', '\t', ' ', '\\', '/' };

		private readonly string LogPrefix;

	}
}
