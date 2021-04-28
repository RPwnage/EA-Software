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
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace NAnt.Core.Util
{

    /// <summary>Helper class for normalizing paths so they can be compared.</summary>
    public class PathNormalizer
    {

        public static readonly char AltDirectorySeparatorChar = Path.DirectorySeparatorChar == '\\' ? '/' : '\\';

        /// <summary>Converts a path so that it can be compared against another path.</summary>
        /// <remarks>
        ///   <para>For some unknown reason using the DirectoryInfo and FileInfo classes to get the full name
        ///   of a file or directory does not return a full path that can be always be compared.  Often the
        ///   drive letter or directory seperators are wrong.  If you want to determine if a path starts
        ///   with another path first normalize the paths with this method and then use the 
        ///   "String.StartsWith" method.
        /// </para>
        /// </remarks>
        /// <example>
        /// string path1 = PathNormalizer.Normalize(path1);
        /// string path2 = PathNormalizer.Normalize(path2);
        /// if (path1.StartsWith(path2) {
        ///     string dir = path1.Substring(path2.Length);
        /// }
        /// </example>
        public static string Normalize(string _path, bool getFullPath)
        {
            if (String.IsNullOrWhiteSpace(_path))
            {
                return String.Empty;
            }
            var path = new StringBuilder(_path);
            try
            {
                path = path.Replace(AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
                bool quoted = false;

                if (path.Length >= 2 &&path[0] == '\"' && path[path.Length-1]=='\"')
                {
                    path.Remove(0, 1);
                    path.Remove(path.Length - 1, 1);
                    quoted = true;
                }

                if (path.Length > 1)
                {
                    path.Replace(SEP_DOUBLE_STRING, SEP_STRING, 1, path.Length - 1);
                }

                if (getFullPath)
                {
                    // get full path
                    path = new StringBuilder(Path.GetFullPath(path.ToString()));
                }

                // normalize drive letter
                if (path.Length > 1)
                {
                    if (path[1] == Path.VolumeSeparatorChar)
                    {
                        path[0] = Char.ToUpper(path[0]);
                    }

                    // remove trailing directory seperator
                    if (path[path.Length - 1] == Path.DirectorySeparatorChar)
                    {
                        path = path.Remove(path.Length - 1, 1);
                    }

                    // convert root drives into directories, d: -> d:\
                    if (path.Length == 2 && path[1] == Path.VolumeSeparatorChar &&  Path.IsPathRooted(path.ToString()) )
                    {
                        path = path.Append(Path.DirectorySeparatorChar);
                    }
                }

                if (quoted)
                {
                    path.Insert(0, '\"');
                    path.Append('\"');
                }
            }
            catch (System.Exception e)
            {
                string msg = String.Format("{0}\n{1}", e.Message, path.ToString());
                throw new BuildException(msg);
            }

            return path.ToString();
        }

        public static string Normalize(string path)
        {
            return Normalize(path, true);
        }

        public static string AddQuotes(string path)
        {
            if (!path.StartsWith("\"", StringComparison.Ordinal))
                path = path.Insert(0, "\"");

            if (!path.EndsWith("\"", StringComparison.Ordinal))
                path = path.Insert(path.Length, "\"");

            return path;
        }

        public static string StripQuotes(string path)
        {
            if (path.StartsWith("\"", StringComparison.Ordinal))
                path = path.Substring(1);

            if (path.EndsWith("\"", StringComparison.Ordinal))
                path = path.Substring(0, path.Length - 1);

            return path;
        }

        public static string MakeRelative(string srcPath, string dstPath)
        {
            return MakeRelative(srcPath, dstPath, false);
        }

        public static string MakeRelative(string srcPath, string dstPath, bool failOnError)
        {

            if (srcPath.Equals(dstPath)) return string.Empty;

            try
            {
                Uri u1 = UriFactory.CreateUri(srcPath);
                // On linux need to add dot to get correct relative path.On Windows we need to ensure there is no trailing
                Uri u2 = UriFactory.CreateUri(dstPath.EnsureTrailingSlash() + '.');
                return Uri.UnescapeDataString(u2.MakeRelativeUri(u1).ToString()).Replace('/', Path.DirectorySeparatorChar);
            }
            catch (System.Exception)
            {
                if (failOnError)
                {
                    throw;
                }
            }
            return srcPath;
        }

        private static string SEP_STRING = Path.DirectorySeparatorChar.ToString();
        private static string SEP_DOUBLE_STRING = Path.DirectorySeparatorChar.ToString() + Path.DirectorySeparatorChar.ToString();
    }
}
