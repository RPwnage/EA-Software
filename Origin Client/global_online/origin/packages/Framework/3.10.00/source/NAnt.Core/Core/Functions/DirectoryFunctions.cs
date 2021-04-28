// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
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
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of directory manipulation routines.
	/// </summary>
	[FunctionClass("Directory Functions")]
	public class DirectoryFunctions : FunctionClassBase {

		/// <summary>
		/// Determines whether the given path refers to an existing directory on disk.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to test.</param>
		/// <returns>True or False.</returns>
        /// <include file='Examples\Directory\DirectoryExists.example' path='example'/>        
        [Function()]
		public static string DirectoryExists(Project project, string path) {
            path = project.GetFullPath(path);
			return System.IO.Directory.Exists(path).ToString();
		}

        /// <summary>
        /// Returns true if the specified directory does not contain any files or directories; otherwise false.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path to the directory.</param>
        /// <returns>True or False.</returns>
        /// <include file='Examples\Directory\DirectoryIsEmpty.example' path='example'/>        
        [Function()]
        public static string DirectoryIsEmpty(Project project, string path) 
        {
            path = project.GetFullPath(path);
            if (System.IO.Directory.GetFiles(path).Length == 0 && System.IO.Directory.GetDirectories(path).Length == 0)
                return Boolean.TrueString;
            return Boolean.FalseString;
        }

        /// <summary>
        /// Returns the number of files in a specified directory which match the given search pattern.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path to the directory.</param>
        /// <param name="pattern">The search pattern to use.</param>
        /// <returns>Number of files in a specified directory that match the given search pattern.</returns>
        /// <include file='Examples\Directory\DirectoryGetFileCount.example' path='example'/>        
        [Function()]
        public static string DirectoryGetFileCount(Project project, string path, string pattern) 
        {
            path = project.GetFullPath(path);
            return System.IO.Directory.GetFiles(path, pattern).Length.ToString();
        }
        
        /// <summary>
        /// Returns a delimited string of file names in the given directory.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path to the directory.</param>
        /// <param name="pattern">The search pattern to use.</param>
        /// <param name="delim">The delimiter to use.</param>
        /// <returns>Delimited string of file names.</returns>
        /// <include file='Examples\Directory\DirectoryGetFiles.example' path='example'/>        
        [Function()]
        public static string DirectoryGetFiles(Project project, string path, string pattern, char delim) 
        {
            path = project.GetFullPath(path);
            string[] files = System.IO.Directory.GetFiles(path, pattern);

            StringBuilder fileList = new StringBuilder();
            for(int i=0; i < files.Length; i++) {
                fileList.Append(files[i]);
                if (i != files.Length - 1)
                    fileList.Append(delim);
            }
            return fileList.ToString();
        }
	
		/// <summary>
		/// Returns a delimited string of subdirectory names in the given directory.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="path">The path to the directory.</param>
		/// <param name="pattern">The search pattern to use.</param>
		/// <param name="delim">The delimiter to use.</param>
		/// <returns>Delimited string of directory names.</returns>
		/// <include file='Examples\Directory\DirectoryGetDirectories.example' path='example'/>        
		[Function()]
		public static string DirectoryGetDirectories(Project project, string path, string pattern, char delim) 
		{
			path = project.GetFullPath(path);
			string[] directories = System.IO.Directory.GetDirectories(path, pattern);

			StringBuilder directoryList = new StringBuilder();
			for(int i=0; i < directories.Length; i++) 
			{
				directoryList.Append(directories[i]);
				if (i != directories.Length - 1)
					directoryList.Append(delim);
			}
			return directoryList.ToString();
		}

		/// <summary>
		/// Returns a collection of directory names from an preorder traversal of the directory tree
		/// <returns>ArrayList of directory names: provided directory and children.</returns>
		/// </summary>
		[Function()] 
		private static System.Collections.ArrayList DirectoriesGetDirectoryListHelper(string strCurrentPath, string strPattern)
		{
			String[] strNewDirectories = System.IO.Directory.GetDirectories(strCurrentPath, strPattern);
			System.Collections.ArrayList strCurrentDirectoryList = new System.Collections.ArrayList();

			strCurrentDirectoryList.AddRange(strNewDirectories);

			// now add all the subdirectories' directories
			for(int i=0; i < strNewDirectories.Length; i++)
			{
				strCurrentDirectoryList.AddRange(DirectoriesGetDirectoryListHelper(strNewDirectories[i], strPattern));
			}

			return (strCurrentDirectoryList);
		}

		/// <summary>
		/// Returns a delimited string of recursively searched directory names from (and including) the provided path
		/// </summary>
		/// 
		/// <param name="project">The project being built</param>
		/// <param name="path">The path to the directory.</param>
		/// <param name="pattern">The search pattern to use.</param>
		/// <param name="delim">The delimiter to use.</param>
		/// <returns>Delimited string of directory names: provided directory and children.</returns>
		/// <include file='Examples\Directory\DirectoryGetDirectoriesRecursive.example' path='example'/>        
		/// 
		/// 
		[Function()]
		public static string DirectoryGetDirectoriesRecursive(Project project, string path, string pattern, char delim) 
		{
			path = project.GetFullPath(path);

			// first get the directories in the current path
			System.Collections.ArrayList directoriesArrayList = DirectoriesGetDirectoryListHelper(project.GetFullPath(path), pattern);

			String[] directories = (String[])directoriesArrayList.ToArray(typeof(String));
			StringBuilder directoryList = new StringBuilder();
			for(int i=0; i < directories.Length; i++) 
			{
				directoryList.Append(directories[i]);
				if (i != directories.Length - 1)
					directoryList.Append(delim);
			}

			// return the full list 
			return directoryList.ToString();
		}

        /// <summary>
        /// Returns last accessed time of specified directory as a string. The string is formated using pattern specified by user.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path to the directory.</param>
        /// <param name="pattern">format pattern.</param>
        /// <returns>Formated last access time of a directory specified by user.</returns>
        /// <include file='Examples\Directory\DirectoryGetLastAccessTime.example' path='example'/>        
        [Function()]
        public static string DirectoryGetLastAccessTime(Project project, string path, string pattern) 
        {
            path = project.GetFullPath(path);
            DateTime lastAccessTime = System.IO.Directory.GetLastAccessTime(path);

            return lastAccessTime.ToString(pattern);
        }

        /// <summary>
        /// Returns last write time of specified directory as a string. The string is formated using pattern specified by user.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="path">The path to the directory.</param>
        /// <param name="pattern">format pattern.</param>
        /// <returns>Formated last access time of a directory specified by user.</returns>
        /// <include file='Examples\Directory\DirectoryGetLastWriteTime.example' path='example'/>        
        [Function()]
        public static string DirectoryGetLastWriteTime(Project project, string path, string pattern) 
        {
            path = project.GetFullPath(path);
            DateTime lastWriteTime = System.IO.Directory.GetLastWriteTime(path);

            return lastWriteTime.ToString(pattern);
        }

        /// <summary>
        /// Moves the whole directory. When source and dest are on the same drive it just renames dir without copying files.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="source">The path to the source directory.</param>
        /// <param name="dest">The path to destination directory.</param>
        /// <returns>Full path to the destination directory.</returns>
        [Function()]
        public static string DirectoryMove(Project project, string source, string dest)
        {
            source = project.GetFullPath(source);
            dest = project.GetFullPath(dest);
            System.IO.Directory.Move(source, dest);
            return dest;
        }
    }
}
