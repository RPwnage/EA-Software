using NuGet.Frameworks;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace NAnt.Core.Nuget
{
    public class PackageContents
    {
        public readonly string Id;

        // All paths relative to this root
        public readonly DirectoryInfo RootDirectory;

        // These should be compiled against but not copied to output
        public List<string> References { get; } = new List<string>();

        // These should be copied to output (and could be compiled against but apparently ~libraries~ can have ~secrets~)
        public List<string> Libraries { get; } = new List<string>();

        // These should be copied to the output
        public List<string> Runtimes { get; } = new List<string>();

        // Things which are not part of linking but should be copied to the output
        public List<string> Content { get; } = new List<string>();

        // Items required to provide special build info (e.g., .targets files)
        public List<string> BuildItems { get; } = new List<string>();

        public PackageContents(string id, DirectoryInfo directory)
        {
            Id = id;
            RootDirectory = directory;
        }

        // Helper
        public IEnumerable<(string relativePath, string baseDir)> CopyItems
        {
            get
            {
                return Libraries.Select(library => GetLibCopyPath(library))
                    .Concat(Runtimes.Select(runtime => AsNonRelativeCopyPath(runtime)))
                    .Concat(Content.Select(content => AsRelativeCopyPath(content)));
            }
        }

        public IEnumerable<FileInfo> ReferenceItems
        {
            get
            {
                return References.Select(x => new FileInfo(Path.Combine(RootDirectory.FullName, x)));
            }
        }

        public IEnumerable<FileInfo> TargetItems
        {
            get
            {
                return BuildItems.Where(item => item.EndsWith(".targets")).Select(targetItem => new FileInfo(Path.Combine(RootDirectory.FullName, targetItem)));
            }
        }

        public IEnumerable<FileInfo> PropItems
        {
            get
            {
                return BuildItems.Where(item => item.EndsWith(".props")).Select(targetItem => new FileInfo(Path.Combine(RootDirectory.FullName, targetItem)));
            }
        }

        private (string relativePath, string baseDir) GetLibCopyPath(string path)
		{
            string basePath = RootDirectory.FullName;
            IEnumerable<string> pathComponents = path.Split(new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar });

            // reference paths always live in lib folder, and we don't want to include it in relative paths, so skip (safely)
            if (pathComponents.Any() && pathComponents.First() == "lib")
			{
                basePath = Path.Combine(basePath, "lib");
                pathComponents = pathComponents.Skip(1);
			}

            // if it's a framework folder skip it, if not, then just a subfolder for any framework
            if (pathComponents.Any() && NuGetFramework.ParseFolder(pathComponents.First()) != NuGetFramework.UnsupportedFramework)
            {
                basePath = Path.Combine(basePath, pathComponents.First());
                pathComponents = pathComponents.Skip(1);
            }

            return (String.Join(Path.DirectorySeparatorChar.ToString(), pathComponents), basePath);
        }

        private (string relativePath, string baseDir) AsNonRelativeCopyPath(string path)
		{
            string fullPath = Path.Combine(RootDirectory.FullName, path);
            return (Path.GetFileName(fullPath), Path.GetDirectoryName(fullPath));
		}

        private (string relativePath, string baseDir) AsRelativeCopyPath(string path)
        {
            return (path, RootDirectory.FullName);
        }
    }
}
