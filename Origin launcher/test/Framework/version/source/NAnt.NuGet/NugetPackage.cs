using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;

using NuGet.Frameworks;
using NuGet.Packaging;

namespace NAnt.NuGet
{
	public class NugetPackage
	{
        public readonly string Id;
        public readonly string RootDirectory; // All paths relative to this root
        public readonly ReadOnlyCollection<string> References; // These should be compiled against but not copied to output
        public readonly ReadOnlyCollection<string> Libraries; // These should be copied to output (and could be compiled against but apparently ~libraries~ can have ~secrets~)
        public readonly ReadOnlyCollection<string> Runtimes; // These should be copied to the output
        public readonly ReadOnlyCollection<string> Content; // Things which are not part of linking but should be copied to the output
        public readonly ReadOnlyCollection<string> BuildItems; // Items required to provide special build info (e.g., .targets files)

        private static readonly ReadOnlyCollection<string> s_assemblyReferencesExtensions = new ReadOnlyCollection<string>(new[] { ".dll", ".exe", ".winmd" });

        internal NugetPackage(string id, string directory, PackageFolderReader reader, NuGetFramework framework)
        {
            Id = id;
            RootDirectory = directory;

            FrameworkReducer frameworkReducer = new FrameworkReducer();
            List<string> GetFrameworkMatch(IEnumerable<FrameworkSpecificGroup> items)
            {
                NuGetFramework matchingGroup = frameworkReducer.GetNearest(framework, items.Select(item => item.TargetFramework));
                return items.Where(group => group.TargetFramework.Equals(matchingGroup)).SelectMany(item => item.Items).ToList();
            }

            References = new ReadOnlyCollection<string>(GetFrameworkMatch(reader.GetReferenceItems()));
            Libraries = new ReadOnlyCollection<string>(GetFrameworkMatch(reader.GetLibItems()));
            Content = new ReadOnlyCollection<string>(GetFrameworkMatch(reader.GetContentItems()));
            BuildItems = new ReadOnlyCollection<string>(GetFrameworkMatch(reader.GetBuildItems()));
            Runtimes = new ReadOnlyCollection<string>(reader.GetFiles(PackagingConstants.Folders.Runtimes).ToList()); // TODO, runtimes can be both architecture and framework specifc - need to filter these
        }

        public override string ToString() => Id;

        public IEnumerable<(string relativePath, string baseDir)> CopyItems => GetLibraryCopies()
                /*.Concat(GetRuntimes(runtimeIdentifier))*/ // NUGET-TODO
                .Concat(Content.Select(content => AsRelativeCopyPath(content)));
		public IEnumerable<FileInfo> ReferenceItems => References.Select(x => new FileInfo(Path.Combine(RootDirectory, x)));
        public IEnumerable<FileInfo> TargetItems => BuildItems.Where(item => item.EndsWith(".targets")).Select(targetItem => new FileInfo(Path.Combine(RootDirectory, targetItem)));
        public IEnumerable<FileInfo> PropItems => BuildItems.Where(item => item.EndsWith(".props")).Select(targetItem => new FileInfo(Path.Combine(RootDirectory, targetItem)));

        private (string relativePath, string baseDir) AsRelativeCopyPath(string path)
        {
            return (path, RootDirectory);
        }

        private IEnumerable<(string relativePath, string baseDir)> GetLibraryCopies()
        {
            List<(string, string)> validLibraries = new List<(string, string)>();
            foreach (string path in Libraries)
            {
                if (!s_assemblyReferencesExtensions.Contains(Path.GetExtension(path), StringComparer.OrdinalIgnoreCase))
                {
                    continue;
                }

                string basePath = RootDirectory;
                IEnumerable<string> pathComponents = path.Split(new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar });

                // reference paths always live in lib folder, and we don't want to include it in relative paths, so skip (safely)
                if (pathComponents.Any() && pathComponents.First() == PackagingConstants.Folders.Lib)
                {
                    basePath = Path.Combine(basePath, PackagingConstants.Folders.Lib);
                    pathComponents = pathComponents.Skip(1);
                }

                // if it's a framework folder skip it, if not, then just a subfolder for any framework
                if (pathComponents.Any() && NuGetFramework.ParseFolder(pathComponents.First()) != NuGetFramework.UnsupportedFramework)
                {
                    basePath = Path.Combine(basePath, pathComponents.First());
                    pathComponents = pathComponents.Skip(1);
                }
                validLibraries.Add((String.Join(Path.DirectorySeparatorChar.ToString(), pathComponents), basePath));
            }
            return validLibraries;
        }

        /*private IEnumerable<(string relativePath, string baseDir)> GetRuntimes(string runtimeIdentifier)
        {
            if (runtimeIdentifier == null)
			{
                return Enumerable.Empty<(string, string)>();
			}

            List<(string, string)> compatbileRuntimes = new List<(string, string)>();
            foreach (string path in Runtimes)
            {
                string basePath = RootDirectory;
                IEnumerable<string> pathComponents = path.Split(new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar });

                // runtimes always live in runtimes
                if (pathComponents.Any() && pathComponents.First() == PackagingConstants.Folders.Runtimes)
                {
                    basePath = Path.Combine(basePath, PackagingConstants.Folders.Runtimes);
                    pathComponents = pathComponents.Skip(1);
                }

				// check if runtime folder matched required runtime
				// NUGET-TODO, this doesn't cut it runtime can fall back through matching (e.g linux-x85 can match linux)
				// currently unclear if a package containing runtimes needs to include a runtime.json itself or if can use another
				// (see https://docs.microsoft.com/en-us/dotnet/core/rid-catalog#:~:text=RID%20is%20short%20for%20Runtime,specific%20assets%20in%20NuGet%20packages.)
				// (see Nuget NuGet.RuntimeModel.RuntimeGraph)
				if (pathComponents.Any() && pathComponents.First() == runtimeIdentifier) 
                {
                    basePath = Path.Combine(basePath, PackagingConstants.Folders.Runtimes);
                    pathComponents = pathComponents.Skip(1);
                }
            }
        }*/
	}
}
