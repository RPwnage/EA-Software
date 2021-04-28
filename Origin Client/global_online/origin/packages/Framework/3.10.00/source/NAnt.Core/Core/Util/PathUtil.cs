using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace NAnt.Core.Util
{
    static public class PathUtil
    {
        static PathUtil()
        {
            switch (Environment.OSVersion.Platform)
            {
                case PlatformID.Win32NT:
                case PlatformID.Win32Windows:

                    IsCaseSensitive = false;
                    break;
                case PlatformID.Unix:
                case (PlatformID)6:   // Some versions of Runtime do not have MacOSX definition. Use integer value 6;

                    IsCaseSensitive = true;
                    break;
                default:

                    IsCaseSensitive = true;
                    break;
            }

            PathStringComparison = PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase;
        }

        public static readonly bool IsCaseSensitive;

        public static StringComparison PathStringComparison;

        /// <summary>Takes two paths and returns a relative path from the source path to the destination path.</summary>
        /// <param name="srcPath">The source path.</param>
        /// <param name="dstPath">The destination path.</param>
        /// <param name="failOnError">Whether this function should throw an exception if an error occurs.</param>
        /// <param name="addDot">Whether to add a dot, represting the current directory, to the beginning of the relative path.</param>
        public static string RelativePath(PathString srcPath, PathString dstPath, bool failOnError = false, bool addDot = false)
        {
            if (srcPath == null)
            {
                return String.Empty;
            }
            if (dstPath == null)
            {
                return srcPath.Path;
            }
            return RelativePath(srcPath.Path, dstPath.Path, failOnError, addDot);
        }

        /// <summary>Takes two paths and returns a relative path from the source path to the destination path.</summary>
        /// <param name="srcPath">The source path.</param>
        /// <param name="dstPath">The destination path.</param>
        /// <param name="failOnError">Whether this function should throw an exception if an error occurs.</param>
        /// <param name="addDot">Whether to add a dot, represting the current directory, to the beginning of the relative path.</param>
        public static string RelativePath(string srcPath, string dstPath, bool failOnError = false, bool addDot = false)
        {
            try
            {
                if (srcPath == dstPath)
                {
                    return String.Empty;
                }

                srcPath = srcPath.TrimQuotes();
                dstPath = dstPath.TrimQuotes().EnsureTrailingSlash();

                if (srcPath != null && srcPath.StartsWith(dstPath, StringComparison.Ordinal))
                {
                    if (addDot)
                    {
                        return "." + Path.DirectorySeparatorChar + srcPath.Substring(dstPath.Length).TrimLeftSlash();
                    }
                    return srcPath.Substring(dstPath.Length).TrimLeftSlash();
                }
                Uri u1 = new Uri(srcPath, UriKind.RelativeOrAbsolute);
                if (u1.IsAbsoluteUri)
                {
                    // On linux need to add dot to get correct relative path.On Windows we need to ensure there is no trailing
                    Uri u2 = UriFactory.CreateUri(dstPath + '.');
                    PathString relative = PathString.MakeNormalized(Uri.UnescapeDataString(u2.MakeRelativeUri(u1).ToString()), PathString.PathParam.NormalizeOnly);
                    return relative.Path;
                }
            }
            catch (Exception)
            {
                if (failOnError)
                {
                    throw;
                }
            }
            return srcPath;
        }

        /// <summary>Return the drive letter as string</summary>
        /// <param name="path">The path whose drive letter we are interested in.</param>
        /// <returns>The drive letter as a string, or null if no drive letter can be determined.</returns>
        public static string GetDriveLetter(string path)
        {
            if (String.IsNullOrEmpty(path))
            {
                return null;
            }

            // Some versions of Runtime do not have MacOSX definition. Use integer value 6
            if (Environment.OSVersion.Platform == PlatformID.Unix ||
               (int)Environment.OSVersion.Platform == 6)
            {
                if (path[0] == Path.DirectorySeparatorChar)
                {
                    int i = path.IndexOf(Path.DirectorySeparatorChar, 1);
                    if (i != -1)
                    {
                        return path.Substring(0, i);
                    }
                }
                return String.Empty;
            }

            int ind = path.IndexOf(':');

            if (ind < 0) // It must be a network path. return host name as a drive letter
            {
                path = path.TrimStart(PATH_SEPARATOR_CHARS_ALT);
                ind = path.IndexOfAny(PATH_SEPARATOR_CHARS_ALT);
            }
            if (ind < 0)
            {
                return null;
            }
            return path.Substring(0, ind);
        }
        
        /// <summary>Checks whether a path is a subdirectory of a projects build root directory.</summary>
        public static bool IsPathInBuildRoot(Project project, string path)
        {
            string buildroot = project.Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT];
            if (buildroot != null)
            {
                path = project.GetFullPath(path);
                buildroot = project.GetFullPath(project.ExpandProperties(buildroot));
                buildroot = buildroot.TrimEnd(new char[] { '\\', '/' }) + Path.DirectorySeparatorChar;

                return IsPathInDirectory(path, buildroot);
            }
            return false;
        }

        /// <summary>Checks whether the given path is equal to or a sub directory of the given directory.</summary>
        /// <param name="path">The path that is tested to see if it is within the directory.</param>
        /// <param name="dir">The directory that is tested to see if it contains the path.</param>
        /// <returns>Return true if the path is within the directory and false otherwise.</returns>
        public static bool IsPathInDirectory(string path, string dir)
        {
            if (path == dir) return true;

            return path.StartsWith(dir.EnsureTrailingSlash(), PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>Checks if a string is a valid path string, ie. contains no invalid symbols.</summary>
        public static bool IsValidPathString(string path)
        {
            return (-1 == path.IndexOfAny(Path.GetInvalidPathChars())) && !path.ContainsMacro();
        }

        private static readonly char[] PATH_SEPARATOR_CHARS_ALT = new char[] { Path.DirectorySeparatorChar, PathNormalizer.AltDirectorySeparatorChar };

        /// <returns>returns true of success or false otherwise</returns>
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool CreateHardLink(string newFileName, string exitingFileName, IntPtr securityAttributes);

        /// <returns>returns 0 if successful or -1 if failure</returns>
        [DllImport("libc", CharSet = CharSet.Ansi)]
        private static extern int link(string oldname, string newname);

        /// <summary>
        /// Copies a file but creates a hard link at the destination instead of copying the entire contents of the file.
        /// </summary>
        /// <param name="dst">The path to the destination file.</param>
        /// <param name="src">The path to the source file being copied.</param>
        /// <returns>Whether or not the operation was successful.</returns>
        public static bool CreateHardLink(string dst, string src)
        {
            if (GetDriveLetter(src) != GetDriveLetter(dst))
            {
                return false;
            }
            var srcAttrib = File.GetAttributes(src);
            if (File.Exists(dst))
            {
                try
                {
                    File.SetAttributes(dst, FileAttributes.Normal);
                    File.Delete(dst);
                }
                catch (Exception)
                {
                }
            }
            bool res = true;
            // Sometimes VisualStudio locks files. Since we use hard links, the file must be already linked.
            // Can be done better with detecting hard link
            if (!File.Exists(dst))
            {
                try
                {
                    if (Environment.OSVersion.Platform == PlatformID.Win32NT
                        || Environment.OSVersion.Platform == PlatformID.Win32Windows)
                    {
                        res = CreateHardLink(dst, src, IntPtr.Zero);
                    }
                    else if (Environment.OSVersion.Platform == PlatformID.Unix
                        || (int)Environment.OSVersion.Platform == 6)
                    {
                        res = (link(src, dst) == 0);
                    }
                }
                catch (Exception)
                {
                    res = false;
                }
            }
            if (res)
            {
                try
                {
                    File.SetAttributes(dst, srcAttrib);
                }
                catch (Exception)
                {
                    res = false;
                }
            }
            try
            {
                File.SetAttributes(src, srcAttrib);
            }
            catch (Exception)
            {
                res = false;
            }

            return res;
        }

        /// <summary>
        /// Deletes a directory with all content. File attributes are reset to normal before delete.
        /// </summary>
        /// <param name="path">The path to the directory</param>
        /// <param name="verify">Verify that directory does not exist after delete and throw exceptioon otherwise.</param>
        /// <param name="failOnError">Fail in the case of error if true, otherwise ignore errors</param>
        public static void DeleteDirectory(string path, bool verify = true, bool failOnError = true)
        {
            if (Directory.Exists(path))
            {
                try
                {
                    int count = 0;
                    const int maxIters = 20;
                    bool attributesDone = false;
                    while (Directory.Exists(path))
                    {
                        if (count > 0 && !attributesDone)
                        {
                            // setting file attributes is s-l-o-w, try to delete the directory without setting them first
                            SetAllFileAttributesToNormal(path);
                            attributesDone = true;
                        }

                        ++count;
                        try
                        {
                            Directory.Delete(path, true);
                        }
                        catch (Exception)
                        {
                            if (count >= maxIters) throw;
                        }

                        if (count >= maxIters) break;
                        Thread.Sleep(100);
                    }
                }
                catch (Exception e)
                {
                    if (failOnError)
                    {
                        string msg = String.Format("Cannot delete directory '{0}'.", path);
                        throw new BuildException(msg, e);
                    }
                    else
                    {
                        VoidEx(e);
                    }
                }

                // verify that the directory has been deleted
                if (verify && Directory.Exists(path) && failOnError)
                {
                    string msg = String.Format("Cannot delete directory '{0}' because of an unknown reason.", path);
                    throw new BuildException(msg);
                }
            }
        }



        /// <summary>For each file and directory in the given path set the file attributes to Normal.</summary>
        private static void SetAllFileAttributesToNormal(string path)
        {
            // clear any read-only flags on directories
            DirectoryInfo info = new DirectoryInfo(path);
            info.Attributes = FileAttributes.Normal;

            // clear any read-only flags on files
            string[] fileNames = Directory.GetFiles(path);
            foreach (string fileName in fileNames)
            {
                File.SetAttributes(fileName, FileAttributes.Normal);
            }

            // recurse into sub directories to do the same
            string[] directoryNames = Directory.GetDirectories(path);
            foreach (string directoryName in directoryNames)
            {
                SetAllFileAttributesToNormal(directoryName);
            }
        }

        // To get rid of compiler warnings
        private static void VoidEx(Exception ex) { }

    }
}
