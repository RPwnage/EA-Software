﻿// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.IO;
using System.Text;

namespace NAnt.Core.Util
{
	/// <summary>
	/// An enum indicating whether to preserve the directory separator or change to match the current system
	/// </summary>
	public enum DirectorySeparatorOptions
	{
		/// <summary>Change directory separators to match what is used by the current system.</summary>
		CurrentSystem,
		/// <summary>Preserve the directory separators, keep them as they are in the original path.</summary>
		Preserve
	}

	/// <summary>Helper class for normalizing paths so they can be compared.</summary>
	public class PathNormalizer
	{

		public static readonly char AltDirectorySeparatorChar = Path.DirectorySeparatorChar == '\\' ? '/' : '\\';


		private unsafe static void RollbackLastFolder(char* path, ref int pathLength)
		{
			for (int j = pathLength - 1; j >= 0; --j)
			{
				if (path[j] != '\\')
					continue;
				pathLength = j;
				break;
			}
		}

		// This function is only valid on Windows, so cannot be used on Linux/OSX
		private unsafe static string GetFullPathFast(StringBuilder origPath)
		{
			int origPathLength = origPath.Length;
			char* path = stackalloc char[origPathLength + 1];
			int pathLength = 0;
			int dotCount = 0;
			int slashCount = 0;

			for (int i=0; i!= origPathLength; ++i)
			{
				char c = origPath[i];
				if (c == '\\')
				{
					if (dotCount == 1)
					{
						dotCount = 0;
					}
					else if (dotCount == 2)
					{
						RollbackLastFolder(path, ref pathLength);
						dotCount = 0;
					}
					else if (slashCount == 0)
					{
						slashCount = 1;
					}
					else
					{
						// Double slash.. just skip them
					}
				}
				else if (c == '.')
				{
					if (slashCount == 0)
						path[pathLength++] = '.';
					else
						++dotCount;
				}
				else
				{
					if (slashCount == 1 && dotCount == 1)
					{
						path[pathLength++] = '\\';
						path[pathLength++] = '.';
						slashCount = 0;
						dotCount = 0;
					}
					else if (slashCount == 1)
					{
						path[pathLength++] = '\\';
						slashCount = 0;
					}
					if (dotCount == 0)
						path[pathLength++] = c;
				}
			}

			if (dotCount == 2)
				RollbackLastFolder(path, ref pathLength);

			if (dotCount == 0 && slashCount > 0)
				path[pathLength++] = '\\';

			path[pathLength] = '\0';

			string result = new string(path);

			if (origPath[0] == '\\' && origPath[1] == '\\')
				result = '\\' + result;
			else if (origPath[0] == '\\')
				result = Directory.GetCurrentDirectory().Substring(0, 2) + result;
			else if (origPath[1] != ':')
				result = Directory.GetCurrentDirectory() + '\\' + result;

			return result;
		}

		/// <summary>Converts a path so that it can be compared against another path.</summary>
		/// <remarks>
		///   <para>For some unknown reason using the DirectoryInfo and FileInfo classes to get the full name
		///   of a file or directory does not return a full path that can be always be compared.  Often the
		///   drive letter or directory separators are wrong.  If you want to determine if a path starts
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
		public static string Normalize(string _path, bool getFullPath, bool useFastPath, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			if (String.IsNullOrWhiteSpace(_path))
			{
				return String.Empty;
			}
			var path = new StringBuilder(_path);
			try
			{
				if (separatorOptions == DirectorySeparatorOptions.CurrentSystem)
				{
					path = path.Replace(AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
				}

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
					if (useFastPath)
						path = new StringBuilder(GetFullPathFast(path));
					else
						path = new StringBuilder(Path.GetFullPath(path.ToString()));
				}

				// normalize drive letter
				if (path.Length > 1)
				{
					if (path[1] == Path.VolumeSeparatorChar)
					{
						path[0] = Char.ToUpper(path[0]);
					}

					// remove trailing directory separator
					if (path[path.Length - 1] == Path.DirectorySeparatorChar || path[path.Length - 1] == AltDirectorySeparatorChar)
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

		public static string Normalize(string path, bool getFullPath, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			return Normalize(path, getFullPath, !PlatformUtil.IsMonoRuntime, separatorOptions: separatorOptions);
		}

		public static string Normalize(string path, DirectorySeparatorOptions separatorOptions = DirectorySeparatorOptions.CurrentSystem)
		{
			return Normalize(path, true, !PlatformUtil.IsMonoRuntime, separatorOptions: separatorOptions);
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
				// On Linux need to add dot to get correct relative path.On Windows we need to ensure there is no trailing
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
