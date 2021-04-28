// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.Collections.Specialized;
using System.Runtime.InteropServices;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

namespace NAnt.Core.Functions
{
    /// <summary>
    /// Collection of path manipulation routines.
    /// </summary>
    [FunctionClass("Path Functions")]
    public class PathFunctions : FunctionClassBase
    {

        /// <summary>
        /// Returns the complete path.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The relateive path to convert.</param>
        /// <returns>Complete path with drive letter (if applicable).</returns>
        /// <include file='Examples\Path\PathGetFullPath.example' path='example'/>        
        [Function()]
        public static string PathGetFullPath(Project project, string path)
        {
            return project.GetFullPath(path);
        }

        /// <summary>
        /// Return the true/real filename stored in the file system. If file doesn't exist, it will
        /// just return the path passed in.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">full path or path that specify a file</param>
        /// <returns>if file exist, return the actual filename stored in the file system, else return the path passed in.</returns>
        /// <include file='Examples\Path\PathGetFileSystemFileName.example' path='example'/>
        [Function()]
        public static string PathGetFileSystemFileName(Project project, string path)
        {
            string realPath = path;
            string workPath = project.GetFullPath(path);

            if (!NAnt.Core.Util.PlatformUtil.IsWindows)
            {
                return workPath;
            }
            // convert user entered path into a consistent long filename path.
            // This is very important if user entered DOS 8.3 filename, this step
            // allow normalize of the user entered path.
            // example:
            // MIXEDC~1.TXT --> Mixed Case.txt
            // mixed case.txt -- > mixed case.txt (doesn't convert to proper case for us, have to use next step to do so.)
            const int MAX_PATH_LENGTH = 4096;
            IntPtr lpShortFileName = Marshal.StringToHGlobalAnsi(workPath.Replace('/', '\\'));
            IntPtr lpBuffer = Marshal.AllocHGlobal(MAX_PATH_LENGTH);
            if (GetLongPathName(lpShortFileName, lpBuffer, MAX_PATH_LENGTH) != 0)
            {
                string fullPath = Marshal.PtrToStringAnsi(lpBuffer);

                // remove \. in the c:\directory\. created by GetLongPathName()
                if (fullPath.EndsWith("\\."))
                {
                    fullPath = fullPath.Substring(0, fullPath.Length - 2);
                }

                // if file exist on the drive, get the actual filename stored in the file system
                FindData fd = new FindData();
                IntPtr handle = FindFirstFile(fullPath, fd);
                IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);
                if (handle != INVALID_HANDLE_VALUE)
                {
                    string actualFileName = fd.fileName;
                    FindClose(handle);
                    realPath = fullPath.Substring(0, fullPath.Length - actualFileName.Length) + actualFileName;
                }
            }
            Marshal.FreeHGlobal(lpShortFileName);
            Marshal.FreeHGlobal(lpBuffer);
            return realPath;
        }

        /// <summary>
        /// Returns the path with backward slashes '\' converted to forward slashes '/'.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path string from which to convert.</param>
        /// <returns>Path with backward slashes converted to forward slashes.</returns>
        /// <include file='Examples\Path\PathToUnix.example' path='example'/>        
        [Function()]
        public static string PathToUnix(Project project, string path)
        {
            return path.Replace(@"\", @"/");
        }

        /// <summary>
        /// Returns the path with drive letter and semicolon are substituted to cygwin notation,
        /// backward slashes '\' converted to forward slashes '/'.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path string from which to convert.</param>
        /// <returns>cygwin path.</returns>
        /// <include file='Examples\Path\PathToUnix.example' path='example'/>        
        [Function()]
        public static string PathToCygwin(Project project, string path)
        {
            string p = path.Replace('\\', '/');
            StringBuilder cygwin = new StringBuilder();
            int prev = 0;
            int index = p.IndexOf(':');
            while (index > 0)
            {
                // Go one back, to get to the drive letter
                index--;
                // Copy everything up to the drive letter
                cygwin.Append(p.Substring(prev, index - prev));
                char drive = p[index];
                cygwin.Append("/cygdrive/");
                cygwin.Append(Char.ToLower(drive));
                prev = index + 2; // Skip the drive letter and colon
                index = p.IndexOf(':', prev);
            }
            // Append the rest of the path
            cygwin.Append(p.Substring(prev));
            return cygwin.ToString();
        }

        /// <summary>
        /// Returns the string with forward slashes '/' converted to backward slashes '\'.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path string from which to convert.</param>
        /// <returns>Path with forward slashes converted to backward slashes.</returns>
        /// <include file='Examples\Path\PathToWindows.example' path='example'/>        
        [Function()]
        public static string PathToWindows(Project project, string path)
        {
            return path.Replace(@"/", @"\");
        }

        /// <summary>
        /// Returns the directory name for the specified path string.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path of a file or directory.</param>
        /// <returns>Directory name of the specified path string.</returns>
        /// <include file='Examples\Path\PathGetDirectoryName.example' path='example'/>        
        [Function()]
        public static string PathGetDirectoryName(Project project, string path)
        {
            return System.IO.Path.GetDirectoryName(path);
        }

        /// <summary>
        /// Relative path.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path of a file or directory.</param>
        /// /// <param name="basepath">Base path used to compute relative path.</param>
        /// <returns>Relative path or ful;l path when computing relative path is not possible.</returns>
        [Function()]
        public static string PathGetRelativePath(Project project, string path, string basepath)
        {
            return PathUtil.RelativePath(path, basepath);
        }

        /// <summary>
        /// Returns the file name and extension of the specified path string.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path string from which to obtain the file name and extension.</param>
        /// <returns>File name and extension of the specified path string.</returns>
        /// <include file='Examples\Path\PathGetFileName.example' path='example'/>        
        [Function()]
        public static string PathGetFileName(Project project, string path)
        {
            return System.IO.Path.GetFileName(path);
        }

        /// <summary>
        /// Returns the extension of the specified path string.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path string from which to get the extension.</param>
        /// <returns>Extension of the specified path string.</returns>
        /// <include file='Examples\Path\PathGetExtension.example' path='example'/>        
        [Function()]
        public static string PathGetExtension(Project project, string path)
        {
            return System.IO.Path.GetExtension(path);
        }

        /// <summary>
        /// Returns the file name of the specified path string without the extension.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path of the file.</param>
        /// <returns>File name of the specified path string without the extension.</returns>
        /// <include file='Examples\Path\PathGetFileNameWithoutExtension.example' path='example'/>        
        [Function()]
        public static string PathGetFileNameWithoutExtension(Project project, string path)
        {
            return System.IO.Path.GetFileNameWithoutExtension(path);
        }

        /// <summary>
        /// Changes the extension of the specified path.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The specified path.</param>
        /// <param name="extension">The new extension.</param>
        /// <returns>The specified path with a different extension.</returns>
        /// <include file='Examples\Path\PathChangeExtension.example' path='example'/>        
        [Function()]
        public static string PathChangeExtension(Project project, string path, string extension)
        {
            return System.IO.Path.ChangeExtension(path, extension);
        }

        /// <summary>
        /// Combine two specified paths.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path1">The first specified path.</param>
        /// <param name="path2">The second specified path.</param>
        /// <returns>A new path containing the combination of the two specified paths.</returns>
        /// <include file='Examples\Path\PathCombine.example' path='example'/>        
        [Function()]
        public static string PathCombine(Project project, string path1, string path2)
        {
            return System.IO.Path.Combine(path1, path2);
        }

        /// <summary>
        /// Returns the root directory of the specified path.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The specified path.</param>
        /// <returns>The root directory of the specified path.</returns>
        /// <include file='Examples\Path\PathGetPathRoot.example' path='example'/>        
        [Function()]
        public static string PathGetPathRoot(Project project, string path)
        {
            return System.IO.Path.GetPathRoot(path);
        }

        /// <summary>
        /// Returns a unique temporary file name which is created as an empty file on disk.
        /// </summary>
        /// <param name="project"></param>
        /// <returns>The name of a temporary file which has been created on disk.</returns>
        /// <include file='Examples\Path\PathGetTempFileName.example' path='example'/>        
        [Function()]
        public static string PathGetTempFileName(Project project)
        {
            return System.IO.Path.GetTempFileName();
        }

        /// <summary>
        /// Returns the path to the systems temp folder.
        /// </summary>
        /// <param name="project"></param>
        /// <returns>The path to the systems temp folder.</returns>
        /// <include file='Examples\Path\PathGetTempPath.example' path='example'/>        
        [Function()]
        public static string PathGetTempPath(Project project)
        {
            return System.IO.Path.GetTempPath();
        }

        /// <summary>
        /// Check if the specified path has a filename extension.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The specified path.</param>
        /// <returns>True or False.</returns>
        /// <include file='Examples\Path\PathHasExtension.example' path='example'/>        
        [Function()]
        public static string PathHasExtension(Project project, string path)
        {
            return System.IO.Path.HasExtension(path).ToString();
        }

        /// <summary>
        /// Check if the specified path contains absolute or relative path information.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The specified path.</param>
        /// <returns>True or False.</returns>
        /// <include file='Examples\Path\PathIsPathRooted.example' path='example'/>        
        [Function()]
        public static string PathIsPathRooted(Project project, string path)
        {
            return System.IO.Path.IsPathRooted(path).ToString();
        }

        /// <summary>
        /// Verifies that path does not contain invalid characters.
        /// </summary>
        /// <param name="path">The specified path.</param>
        /// <param name="parameterName">Name of the path variable to pass to exception.</param>
        /// Throws InvalidArgument exception if path is invalid

        public static void PathVerifyValid(string path, string parameterName)
        {
            if (path != null)
            {
                if (-1 < path.IndexOfAny(System.IO.Path.GetInvalidPathChars()))
                {
                    throw new System.ArgumentException("Illegal characters in path (" + parameterName + ")='" + path + "'", parameterName);
                }
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private class FindData
        {
            public int fileAttributes = 0;
            // creationTime was an embedded FILETIME structure.
            public int creationTime_lowDateTime = 0;
            public int creationTime_highDateTime = 0;
            // lastAccessTime was an embedded FILETIME structure.
            public int lastAccessTime_lowDateTime = 0;
            public int lastAccessTime_highDateTime = 0;
            // lastWriteTime was an embedded FILETIME structure.
            public int lastWriteTime_lowDateTime = 0;
            public int lastWriteTime_highDateTime = 0;
            public int nFileSizeHigh = 0;
            public int nFileSizeLow = 0;
            public int dwReserved0 = 0;
            public int dwReserved1 = 0;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
            public String fileName = null;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 14)]
            public String alternateFileName = null;
        }

        // Declares a managed prototype for the unmanaged function.
        [DllImport("Kernel32.dll", EntryPoint = "FindFirstFileA")]
        private static extern IntPtr FindFirstFile(String fileName, [In, Out] FindData findFileData);

        [DllImport("Kernel32.dll", EntryPoint = "FindClose")]
        private static extern bool FindClose(IntPtr hFindFile);

        [DllImport("Kernel32.dll", EntryPoint = "GetLongPathNameA")]
        private static extern Int32 GetLongPathName(
            IntPtr lpszShortPath,
            IntPtr lpszLongPath,
            Int32 cchBuffer
            );
    }
}
