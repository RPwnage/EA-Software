using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;

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
                        _isEmpty = FolderFiles.Count == 0 && FolderProjects.Count == 0 && children.Where(child => !child.Value.IsEmpty).Count() == 0;
                    }
                    return (bool)_isEmpty;
                }
            }
            private bool? _isEmpty = null;

#if FRAMEWORK_PARALLEL_TRANSITION
            public string FolderGuidToString()
            {
    // $$$ TEMP
    // The C# Guid struct has a ToString() method we should be able to use
                // Create a GUID-string representation of the hash code bytes
                // A GUID has the format: {00010203-0405-0607-0809-101112131415}
                const int GUID_BYTE_NUM = 16;
                const char DASH_CHAR = '-';
                System.Text.StringBuilder guid = new System.Text.StringBuilder(GUID_BYTE_NUM + 4);
                guid.Append('{');

                // Append each byte as a 2 character upper case hex string.
                guid.Append(Hash.BytesToHex(Guid.ToByteArray(), 0, 4));
                guid.Append(DASH_CHAR);
                for (int i = 0; i < 3; i++)
                {
                    guid.Append(Hash.BytesToHex(Guid.ToByteArray(), 4 + i * 2, 2));
                    guid.Append(DASH_CHAR);
                }
                guid.Append(Hash.BytesToHex(Guid.ToByteArray(), 10, 6));
                guid.Append('}');
    // $$$ ENDTEMP
                return (guid.ToString());
            }
#endif
        }


        public SolutionFolders(string logPrefix)
        {
            LogPrefix = logPrefix;
        }

        public void ProcessProjects(Project project, IEnumerable<ModuleGenerator> moduleGenerators)
        {
            var package_folder = new Dictionary<string, SolutionFolders.SolutionFolder>();
            var module_folder = new Dictionary<string, SolutionFolders.SolutionFolder>();
            // Fill folder files content:
            foreach (var folder in Folders.Values.Where(f => f.PackageName == project.Properties["package.name"]))
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
                }
                string packages = project.Properties[propertyName];
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
                            IEnumerable<ModuleGenerator> generators;
                            // See if second parameter is a group name.
                            BuildGroups group;
                            if (packagemodules.Count > 2 && Enum.TryParse<BuildGroups>(packagemodules[1].TrimWhiteSpace(), out group))
                            {
                                var modules = packagemodules.Skip(2);
                                generators = moduleGenerators.Where(gen => gen.Package.Name == packagemodules[0] && modules.Contains(gen.ModuleName) && gen.Modules.FirstOrDefault() != null && gen.Modules.FirstOrDefault().BuildGroup == group).Cast<VSProjectBase>();
                            }
                            else
                            {
                                var modules = packagemodules.Skip(1);
                                generators = moduleGenerators.Where(gen => gen.Package.Name == packagemodules[0] && modules.Contains(gen.ModuleName)).Cast<VSProjectBase>();
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
                var generator = moduleGenerators.FirstOrDefault(gen => gen.Key == entry.Key) as VSProjectBase;
                if (generator != null && !entry.Value.FolderProjects.ContainsKey(generator.ProjectGuid))
                {
                    if (!entry.Value.FolderProjects.ContainsKey(generator.ProjectGuid))
                    {
                        entry.Value.FolderProjects.Add(generator.ProjectGuid, generator);
                    }
                }
            }
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

        public readonly Dictionary<string, SolutionFolder> Folders = new Dictionary<string, SolutionFolder>(StringComparer.CurrentCultureIgnoreCase);

        private static readonly char[] SPLIT_CHARS = new char[] { '\n', '\r', ' ', ',', ';' };
        private static readonly char[] SPLIT_FOLDER_CHARS = new char[] { '\\', '/' };
        private static readonly char[] TRIM_CHARS = new char[] { '\"', '\n', '\r', '\t', ' ', '\\', '/' };

        private readonly string LogPrefix;

    }
}
