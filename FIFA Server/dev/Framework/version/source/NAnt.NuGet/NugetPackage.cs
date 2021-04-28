using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using NuGet.Client;
using NuGet.Commands;
using NuGet.ContentModel;
using NuGet.Frameworks;
using NuGet.Packaging;
using NuGet.Packaging.Core;
using NuGet.ProjectManagement;
using NuGet.RuntimeModel;

namespace NAnt.NuGet
{
	public class NugetPackage
	{
        public readonly string Id;
        public readonly string Version;
        public readonly string RootDirectory; // All paths relative to this root

        private readonly string[] m_libraries; // These should be copied to output (and could be compiled against but apparently ~libraries~ can have ~secrets~)
        private readonly string[] m_nativeRuntimes; // These should be copied to the output
        private readonly string[] m_content; // Things which are not part of linking but should be copied to the output
        private readonly string[] m_buildItems; // Items required to provide special build info (e.g., .targets files)

        public override string ToString() => Id;

        public IEnumerable<FileInfo> CompileItems => m_libraries.Select(x => new FileInfo(Path.Combine(RootDirectory, x)));
        public IEnumerable<FileInfo> TargetItems => m_buildItems.Where(item => item.EndsWith(BuildAssetsUtils.TargetsExtension)).Select(targetItem => new FileInfo(Path.Combine(RootDirectory, targetItem)));
        public IEnumerable<FileInfo> PropItems => m_buildItems.Where(item => item.EndsWith(BuildAssetsUtils.PropsExtension)).Select(targetItem => new FileInfo(Path.Combine(RootDirectory, targetItem)));
        public IEnumerable<FileInfo> RuntimeItems => m_nativeRuntimes.Select(item => new FileInfo(Path.Combine(RootDirectory, item)));
       
        public IEnumerable<(string relativePath, string baseDir)> CopyItems => GetLibraryCopies()
            .Concat(m_nativeRuntimes.Select(path => (Path.GetDirectoryName(path), Path.GetFileName(path))))
            .Concat(m_content.Select(content => AsRelativeCopyPath(content)));

        internal NugetPackage(string id, string version, string directory, PackageFolderReader reader, NuGetFramework framework, IEnumerable<string> runtimes, RuntimeGraph runtimeGraph)
        {
            Id = id;
            Version = version;
            RootDirectory = directory;

            // check for support of targeted .net
            FrameworkReducer frameworkReducer = new FrameworkReducer();
            IEnumerable<NuGetFramework> supportedFrameworks = reader.GetSupportedFrameworks()
                .Concat(reader.GetItems(PackagingConstants.Folders.Ref).Select(refItem => refItem.TargetFramework))
                .Distinct();
            if (supportedFrameworks.Any()) // if we have native only package dependencies they wull support no Frameworks - often they have no runtime eithers and have deps to individual runtime packages so we don't validate that
            {
                NuGetFramework nearest = frameworkReducer.GetNearest(framework, supportedFrameworks);
                if (nearest == null)
                {
                    throw new UnsupportFrameworkNugetPackageException(id, version, framework, supportedFrameworks);
                }
            }

            // pull out files needed for framework and runtime
            ManagedCodeConventions managedCodeConventions = new ManagedCodeConventions(runtimeGraph);
            ContentItemCollection contentCollection = new ContentItemCollection();
            contentCollection.Load(reader.GetFiles());

            string[] GetFrameworkMatchingItems(PatternSet patternSet)
			{
                if (runtimes?.Any() ?? false)
                {
                    return runtimes.SelectMany
                    (
                        runtime =>
                        {
							SelectionCriteria criteria = managedCodeConventions.Criteria.ForFrameworkAndRuntime(framework, runtime);
                            ContentItemGroup itemGroup = contentCollection.FindBestItemGroup(criteria, patternSet);
                            return itemGroup?.Items.
                                Select(item => item.Path)
                                .Where(path => Path.GetFileName(path) != PackagingCoreConstants.EmptyFolder) ??
                                Enumerable.Empty<string>();
                        }
                    )
                    .Distinct()
                    .ToArray();
                }
                else
				{
                    SelectionCriteria criteria = managedCodeConventions.Criteria.ForFramework(framework);
                    ContentItemGroup itemGroup = contentCollection.FindBestItemGroup(criteria, patternSet);
					IEnumerable<string> items = itemGroup?.Items.
                        Select(item => item.Path)
                        .Where(path => Path.GetFileName(path) != PackagingCoreConstants.EmptyFolder)
                        .Distinct();

                    return items?.ToArray() ?? new string[] { };
                }
            }

            m_content = GetFrameworkMatchingItems(managedCodeConventions.Patterns.ContentFiles);
            m_buildItems = GetFrameworkMatchingItems(managedCodeConventions.Patterns.MSBuildFiles);
            m_libraries = GetFrameworkMatchingItems(managedCodeConventions.Patterns.CompileLibAssemblies);
            m_nativeRuntimes = GetFrameworkMatchingItems(managedCodeConventions.Patterns.NativeLibraries);
        }
		
        private (string relativePath, string baseDir) AsRelativeCopyPath(string path)
        {
            return (path, RootDirectory);
        }

        private IEnumerable<(string relativePath, string baseDir)> GetLibraryCopies()
        {
            List<(string, string)> validLibraries = new List<(string, string)>();
            foreach (string path in m_libraries)
            {
                if (!Constants.AssemblyReferencesExtensions.Contains(Path.GetExtension(path), StringComparer.OrdinalIgnoreCase))
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
	}
}
