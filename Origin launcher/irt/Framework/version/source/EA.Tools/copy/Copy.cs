// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Concurrent;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.Threading;
using NAnt.Shared;

namespace EA.Tools
{
	internal class Copy
	{
		internal static readonly char[] Wildcards = { '?', '*' };

		internal static bool OverwriteReadOnlyFiles = true;
		internal static bool SkipUnchangedFiles = false;
		internal static bool UseHardLinksIfPossible = false;
		internal static bool UseSymlinksIfPossible = false;
		internal static bool Recursive = false;
		internal static bool Silent = false;
		internal static bool CopyEmptyDir = false;
		internal static bool Info = false;
		internal static bool Warnings = false;
		internal static bool Summary = false;
		internal static bool IgnoreMissingInput = false;
		internal static bool DeleteOriginalFile = false;
		internal static bool CopyOnlyIfNewerOrMissing = false;
		internal static bool ClearReadOnlyAttributeAfterCopy = false;  // (only if hardlinks and symlinks aren't used)
		internal static int Retries = 0;

		static private readonly bool isMonoRuntime = (Type.GetType("Mono.Runtime") != null);

		static private readonly bool caseSensitive = true;

		static private readonly ConcurrentDictionary<string, Regex> regexCollection = new ConcurrentDictionary<string, Regex>();

		static Copy()
		{
			switch (Environment.OSVersion.Platform)
			{
				case PlatformID.Win32NT:
				case PlatformID.Win32Windows:
					caseSensitive = false;
					break;
				case PlatformID.Unix:
				case (PlatformID)6:   // Some versions of Runtime do not have MacOSX definition. Use integer value 6;
					caseSensitive = true;
					break;
				default:
					caseSensitive = true;
					break;
			}
		}


		internal static uint Execute(string src, string dst, ref uint upToDate, ref uint copied, ref uint linked)
		{
			if (!Silent && Warnings && DeleteOriginalFile && (UseSymlinksIfPossible || UseHardLinksIfPossible))
			{
				Console.WriteLine
				(
					"Using symbolic link (-x) option or hardlink (-l) with delete orignal option (-d) is not recommended. " +
					"If a symbolic link or hard link is created the original file wiil not be deleted. " +
					"This is to preserve legacy behaviour."
				);
			}

			uint errorCount = 0;

			// Evaluate full path if input is relative to project path.
			src = PathCombine(Environment.CurrentDirectory, src);
			dst = PathCombine(Environment.CurrentDirectory, dst);

			if (IsExplicitPattern(src))
			{
				if (Directory.Exists(src))
				{
					errorCount += CopyDirectory(src, dst, ref upToDate, ref copied, ref linked);
				}
				else
				{
					if (dst.EndsWith("\\", StringComparison.Ordinal) || dst.EndsWith("/", StringComparison.Ordinal) || Directory.Exists(dst))
					{
						dst = PathCombine(dst, Path.GetFileName(src));
					}

					errorCount += CopyFile(src, dst, ref upToDate, ref copied, ref linked);
				}
			}
			else
			{
				string basedir = Path.GetDirectoryName(src);
				string pattern = Path.GetFileName(src);

				if (IsImplicitPattern(basedir))
				{
					throw new ArgumentException(String.Format("Patterns only allowed in file names, invalid argument '{0}'", src));
				}

				if (Directory.Exists(basedir))
				{
					errorCount += CopyDirectory(basedir, dst, ref upToDate, ref copied, ref linked, pattern);
				}
				else
				{
					if (!Silent && Warnings)
					{
						Console.WriteLine("Copy: source directory does not exist: '{0}'", basedir);
					}
				}
			}
			return errorCount;
		}

		private static bool IsExplicitPattern(string pattern)
		{
			return (pattern.IndexOfAny(Copy.Wildcards) == -1);
		}

		private static bool IsImplicitPattern(string pattern)
		{
			return (pattern.IndexOfAny(Copy.Wildcards) != -1);
		}

		private static uint CopyDirectory(string sourceFolder, string destFolder, ref uint upToDate, ref uint copied, ref uint linked, string pattern = null)
		{
			uint errorCount = 0;
			if (CopyEmptyDir)
			{
				Directory.CreateDirectory(destFolder);
			}

			string[] files = Directory.GetFiles(sourceFolder);
			foreach (string file in files)
			{
				string name = Path.GetFileName(file);
				if (IsMatch(pattern, name))
				{
					string dest = PathCombine(destFolder, name);
					errorCount += CopyFile(file, dest, ref upToDate, ref copied, ref linked);
				}
			}
			if (Recursive)
			{
				foreach (var folder in Directory.GetDirectories(sourceFolder))
				{
					string name = Path.GetFileName(folder);
					string dest = PathCombine(destFolder, name);
					errorCount += CopyDirectory(folder, dest, ref upToDate, ref copied, ref linked, pattern);
				}
			}
			return errorCount;
		}

		private static bool IsMatch(string pattern, string path)
		{
			if (String.IsNullOrEmpty(pattern))
			{
				return true;
			}

			return ToRegex(pattern).IsMatch(Path.GetFileName(path));
		}


		public static Regex ToRegex(string pattern)
		{
			return regexCollection.GetOrAdd(pattern, (key) =>
			{
				RegexOptions regexOptions = RegexOptions.Compiled;
				if (!caseSensitive)
					regexOptions |= RegexOptions.IgnoreCase;

				var r = new Regex(ToRegexPattern(key), regexOptions);
				return r;

			});
		}


		public static string ToRegexPattern(string input_pattern)
		{
			StringBuilder pattern = new StringBuilder(input_pattern);

			// The '\' character is a special character in regular expressions
			// and must be escaped before doing anything else.
			pattern.Replace(@"\", @"\\");

			// Escape the rest of the regular expression special characters.
			// NOTE: Characters other than . $ ^ { [ ( | ) * + ? \ match themselves.
			// TODO: Decide if ] and } are missing from this list, the above
			// list of characters was taking from the .NET SDK docs.
			pattern.Replace(".", @"\.");
			pattern.Replace("$", @"\$");
			pattern.Replace("^", @"\^");
			pattern.Replace("{", @"\{");
			pattern.Replace("[", @"\[");
			pattern.Replace("(", @"\(");
			pattern.Replace(")", @"\)");
			pattern.Replace("+", @"\+");

			// Special case directory seperator string under Windows.
			string seperator = Path.DirectorySeparatorChar.ToString();
			if (seperator == @"\")
			{
				seperator = @"\\";
			}

			// Convert NAnt pattern characters to regular expression patterns.

			// SPECIAL CASE: to match subdirectory OR current directory.  If
			// we don't do this then we can write something like 'src/**/*.cs'
			// to match all the files ending in .cs in the src directory OR
			// subdirectories of src.
			pattern.Replace("**" + seperator, "(.|" + seperator + ")|");

			// | is a place holder for * to prevent it from being replaced in next line
			pattern.Replace("**", ".|");
			pattern.Replace("*", "[^" + seperator + "]*");
			pattern.Replace("?", "[^" + seperator + "]?");
			pattern.Replace('|', '*'); // replace place holder string

			// Help speed up the search
			pattern.Insert(0, '^'); // start of line
			pattern.Append('$'); // end of line

			return pattern.ToString();
		}

		private static uint CopyFile(string src, string dst, ref uint upToDate, ref uint copied, ref uint linked)
		{
			if (IgnoreMissingInput && !File.Exists(src))
			{
				return 0;
			}
			else
			{
				bool skipCopyOldFile = false;
				LinkOrCopyStatus status = LinkOrCopyStatus.Failed;

				if (CopyOnlyIfNewerOrMissing)
				{
					FileInfo srcInfo = new FileInfo(src);
					FileInfo dstInfo = new FileInfo(dst);
					if (dstInfo.Exists)
					{
						if (srcInfo.LastWriteTime <= dstInfo.LastWriteTime)
						{
							if (!Silent)
							{
								Console.WriteLine("Skip copying of {0} to {1} as it has older timestamp", Path.GetFileName(srcInfo.FullName), Path.GetFileName(dstInfo.FullName));
							}
							skipCopyOldFile = true;
						}
					}
				}

				if (skipCopyOldFile)
				{
					status = LinkOrCopyStatus.AlreadyCopied;
				}
				else
				{
					List<LinkOrCopyMethod> methods = new List<LinkOrCopyMethod>();
					if (UseSymlinksIfPossible)
					{
						methods.Add(LinkOrCopyMethod.Symlink);
					}
					else if (UseHardLinksIfPossible)
					{
						methods.Add(LinkOrCopyMethod.Hardlink);
					}
					methods.Add(LinkOrCopyMethod.Copy);

					using (Mutex fileAccessMutex = new Mutex(true, FileWriteAccessMutexName.GetMutexName(dst), out bool createdNewMutex))
					{
						if (!createdNewMutex)
						{
							fileAccessMutex.WaitOne();  // Wait forever!
						}
						status = LinkOrCopyCommon.TryLinkOrCopy
						(
							src,
							dst,
							overwriteMode: OverwriteReadOnlyFiles ? OverwriteMode.OverwriteReadOnly : OverwriteMode.OverwriteReadOnly,
							copyFlags: (UseSymlinksIfPossible || UseHardLinksIfPossible || !ClearReadOnlyAttributeAfterCopy ? CopyFlags.MaintainAttributes : CopyFlags.None )| CopyFlags.PreserveTimestamp | (SkipUnchangedFiles ? CopyFlags.SkipUnchanged : CopyFlags.None),
							retries: (uint)Retries,
							retryDelayMilliseconds: 500,
							methods: methods.ToArray()
						);
						fileAccessMutex.ReleaseMutex();
						fileAccessMutex.Close();
					}
				}

				switch (status)
				{
					case LinkOrCopyStatus.Failed:
						return 1;
					case LinkOrCopyStatus.AlreadyHardlinked:
					case LinkOrCopyStatus.AlreadyJunctioned:
					case LinkOrCopyStatus.AlreadySymlinked:
						upToDate++;
						return 0;
					case LinkOrCopyStatus.Hardlinked:
					case LinkOrCopyStatus.Symlinked:
						linked++;
						return 0;
					case LinkOrCopyStatus.Copied:
						DeleteSourceIfRequested(src);
						upToDate++;
						return 0;
					case LinkOrCopyStatus.AlreadyCopied:
						DeleteSourceIfRequested(src);
						copied++;
						return 0;
					default:
						return 0;
				}
			}
		}

		private static void DeleteSourceIfRequested(string src)
		{
			if (DeleteOriginalFile)
			{
				File.SetAttributes(src, FileAttributes.Normal);
				File.Delete(src);
			}
		}

		private static string PathCombine(string src, string dst)
		{
			try
			{
				return Path.Combine(src, dst);
			}
			catch (Exception ex)
			{
				Console.WriteLine("Error in Path.Combine({0}, {1}): '{2}'", src, dst, ex.Message);
				throw;
			}
		}
	}
}
