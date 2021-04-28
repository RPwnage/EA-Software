// Copyright (c) 2007 Electronic Arts
using System;
using System.IO;
using System.Reflection;
using System.Diagnostics;
using System.Text;

namespace NAnt.Core.Util
{
    public class BuildFile
    {
        public const string BUILDFILE_EXT = ".build";
        public const string OVERRIDE_EXT = ".override";

        public static bool UseOverrides { get; set; }

        /// <summary>
        /// Gets the file name for the build file in the specified directory.
        /// </summary>
        /// <param name="directory">The directory to look for a build file.  When in doubt use Environment.CurrentDirectory for directory.</param>
        /// <param name="customSearchPattern">Look for a build file with this pattern or name.  If null look for a file that matches the default build pattern (*.build).</param>
        /// <param name="appendDefaultExtensions">Even if custom pattern provided, make sure to add default extentions for pattern search.</param>
        /// <param name="findInParent">Whether or not to search the parent directories for a build file.</param>
        /// <returns>The path to the build file or <c>null</c> if no build file could be found.</returns>
        public static string GetBuildFileName(string directory, string customSearchPattern, bool appendDefaultExtensions, bool findInParent)
        {
            string buildFileName = null;
            string searchPattern = String.Empty;
            string overrideSearchPattern = String.Empty;

            if (customSearchPattern != null)
                searchPattern = customSearchPattern;

            if (Path.IsPathRooted(searchPattern))
            {
                buildFileName = searchPattern;
            }
            else
            {
                if (String.IsNullOrEmpty(searchPattern))
                {
                    searchPattern += "*";                    
                }
                if (appendDefaultExtensions)
                {
                    searchPattern += BUILDFILE_EXT;   
                }
                overrideSearchPattern = searchPattern + OVERRIDE_EXT + "*";

                // find first file ending in .build
                DirectoryInfo directoryInfo = new DirectoryInfo(directory);

                FileInfo[] buildFiles = directoryInfo.GetFiles(searchPattern);
                FileInfo[] overrideBuildFiles = new FileInfo[0];

                if (UseOverrides && !String.IsNullOrEmpty(overrideSearchPattern))
                    overrideBuildFiles = directoryInfo.GetFiles(overrideSearchPattern);

                if (buildFiles.Length == 1)
                {
                    buildFileName = Path.Combine(directory, buildFiles[0].Name);
                    int len = overrideBuildFiles.Length;
                    if (len > 0 && UseOverrides)
                    {
                        Array.Sort(overrideBuildFiles,
                           delegate(FileInfo f1, FileInfo f2)
                           {
                               return f1.Name.CompareTo(f2.Name);
                           }
                        );
                        buildFileName = Path.Combine(directory, overrideBuildFiles[len - 1].Name);
                    }
                }
                else if (buildFiles.Length == 0)
                {
                    DirectoryInfo parentDirectoryInfo = directoryInfo.Parent;
                    if (findInParent && parentDirectoryInfo != null)
                    {
                        buildFileName = GetBuildFileName(parentDirectoryInfo.FullName, customSearchPattern, appendDefaultExtensions, findInParent);
                    }
                    else
                    {
                        throw new ApplicationException((String.Format("Could not find a '{0}' file in '{1}'", searchPattern, directory)));
                    }

                }
                else
                { // files.Length > 1
                    throw new ApplicationException(String.Format("More than one '{0}' file found in '{1}'.  Use -buildfile:<file> to specify.", searchPattern, directory));
                }
            }
            return buildFileName;
        }

        public static string GetPackageBuildFileName(string directory, string package, bool findInParent)
        {
            return GetBuildFileName(directory, package, true, findInParent);
        }
    }
}
