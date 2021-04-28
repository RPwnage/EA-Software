// NAnt - A .NET build tool
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
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;

namespace NAnt.Core.Util
{
	public abstract class Pattern
	{
		public static readonly char[] Wildcards = { '?', '*' };
		public bool PreserveBaseDirValue = false;

		// the below properties are used for directory caching optimization. If s_userDirCache is true then the dictionaries are used to
		// store the results of enumerating the directory (s_fileSystemInfoCache) and checking if the dir exists (s_existanceCache). This
		// is significantly faster than enumerating the files in a directory over and over (which is likely with Framework's architecture
		// in complex solutions) but means that Framework will not detect changes to file system after the first enumeration.
		// This is defaulted to false to support cases where a user might generate many files then enumerate them with a wild card pattern
		// within the same execution - directory caching would make these changes invisible to Framework. 
		protected static bool s_useDirCache = false;
		protected static ConcurrentDictionary<string, FileSystemInfo[]> s_fileSystemInfoCache = new ConcurrentDictionary<string, FileSystemInfo[]>();
		protected static ConcurrentDictionary<string, bool> s_existanceCache = new ConcurrentDictionary<string, bool>();

		public override string ToString()
		{
			return Value;
		}


		public abstract List<string> GetMatchingFiles(bool failOnMissingFile, List<RegexPattern> excludePatterns, StringCollection excludeFileNames);

		public string Value { get; set; } = null;

		public string BaseDirectory { get; set; } = null;
		public bool FailOnMissingFile { get; set; } = true;

		/// <summary>Check if the specified pattern does not contain any wild cards.</summary>
		/// <returns>True if the pattern does not contain any wild cards, otherwise false.</returns>
		public static bool IsExplicitPattern(string pattern)
		{
			return (pattern.IndexOfAny(Pattern.Wildcards) == -1);
		}

		/// <summary>Check if the specified pattern contains any wild cards.</summary>
		/// <returns>True if the pattern contains any wild cards, otherwise false.</returns>
		public static bool IsImplicitPattern(string pattern)
		{
			return (pattern.IndexOfAny(Pattern.Wildcards) != -1);
		}

		public static bool IsAbsolutePattern(string pattern, bool asis)
		{
			return (asis);
		}

		/// <summary>Adds regular expressions for any non-explicit (ie, uses wild cards) NAnt patterns.</summary>
		/// <param name="nantPattern">The NAnt pattern to convert.  Absolute or relative paths.</param>
		/// <param name="absoluteFileName">Absolute pattern will return file name here.</param>
		/// <returns>The resulting regular expression. Absolute canonical path.</returns>
		/// <remarks>
		///		<para>Only NAnt patterns that contain a wild card character as converted to regular expressions.
		///		Explicit patterns are expected to be handled using absoluteFileName.</para>
		/// </remarks>
		virtual public RegexPattern ConvertPattern(string nantPattern, out string absoluteFileName)
		{
			absoluteFileName = null;
			return null;
		}

		/// <summary>Verifies that a path matches the regex pattern.
		/// More efficient implementation, to be used when caller knows
		/// if the volume is case sensitive.</summary>
		protected static bool IsPathInPatternSet(string filePath, RegexPattern regexPattern, bool ignoreCase, List<RegexPattern> excludedPatterns, StringCollection excludeFileNames)
		{
			bool included = false;

			if (regexPattern != null)
			{
				included = IsMatch(filePath, regexPattern, ignoreCase);
			}
			else
			{
				included = true;
			}
			// check path against exclude regexes
			if (included)
			{
				foreach (RegexPattern rPattern in excludedPatterns)
				{
					bool test = IsMatch(filePath, rPattern, ignoreCase); 
					if (test)
						included = false;
				}
			}
			if (included)
			{
				StringComparison option = ignoreCase ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal;
				foreach (string name in excludeFileNames)
				{
					if (name.Equals(filePath, option))
					{
						included = false;
						break;
					}
				}
			}

			return included;
		}

		public static bool IsMatch(string filePath, RegexPattern regexPattern, bool ignoreCase)
		{
			bool isMatch = false;

			if (regexPattern.Type == RegexPattern.PatternType.Any)
			{
				isMatch = true;
			}
			if (regexPattern.Type == RegexPattern.PatternType.Regex)
			{
				Regex r = ToRegex(regexPattern.Pattern, !ignoreCase);

				// Check to see if the empty string matches the pattern
				if (filePath.Length == regexPattern.BaseDir.Length)
				{
					isMatch = r.IsMatch(String.Empty);
				}
				else
				{
					bool endsWithSlash = EndsWith(regexPattern.BaseDir, Path.DirectorySeparatorChar);

					if (endsWithSlash)
					{
						// IM The variant of IsMatch with start index does not give correct result. Use substring here :(
						isMatch = r.IsMatch(filePath.Substring(regexPattern.BaseDir.Length));
					}
					else
					{
						isMatch = r.IsMatch(filePath.Substring(regexPattern.BaseDir.Length + 1));
					}
				}
			}
			else
			{
				int incr = EndsWith(regexPattern.BaseDir, Path.DirectorySeparatorChar) ? 0 : 1;
				int filematchind = regexPattern.BaseDir.Length + incr;

				if ((regexPattern.Type & RegexPattern.PatternType.Dir) == RegexPattern.PatternType.Dir)
				{
					if (filematchind + regexPattern.Dir.Length <= filePath.Length)
					{
						int dirind = -1;
						if (regexPattern.DirPos > 1)
						{
							if (((regexPattern.Type & RegexPattern.PatternType.AnyFile) == RegexPattern.PatternType.AnyFile))
							{

								int count = filePath.Length - filematchind;
								if (regexPattern.Dir[0] == Path.DirectorySeparatorChar && filematchind > 0)
								{
									count++;
								}

								dirind = filePath.LastIndexOf(regexPattern.Dir, filePath.Length - 1, count, ignoreCase ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);
							}
							else
							{
								dirind = filePath.IndexOf(regexPattern.Dir, filematchind, ignoreCase ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);
								// If dir starts with \ it can be the first directory 
								if (dirind == -1 && regexPattern.Dir.Length > 1 && regexPattern.Dir[0] == Path.DirectorySeparatorChar)
								{
									if (0 == String.Compare(filePath, filematchind, regexPattern.Dir, 1, regexPattern.Dir.Length - 1, ignoreCase ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal))
									{
										dirind = filematchind-1;
									}
								}
							}
						}
						else if(regexPattern.DirPos > 0)
						{
							dirind = filePath.IndexOf(regexPattern.Dir, filematchind, ignoreCase ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);
							// Handle cases like this: // '*\*.h' ==> '^[^\\]*\\[^\\]*\.h$'
						}
						else
						{
							// Handle cases like this: // 'foo*\*.h'
							dirind = filePath.IndexOf(regexPattern.Dir, filematchind, regexPattern.Dir.Length, ignoreCase ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);
						}
						if (dirind > -1)
						{
							// Make sure there are no directory separators between found dir location and the start.
							if (!(regexPattern.DirPos > 1) && -1 != filePath.IndexOf(Path.DirectorySeparatorChar, filematchind, dirind - filematchind))
							{
								return false;
							}
							filematchind = dirind + regexPattern.Dir.Length;

						}
						else
						{
							return false;
						}

					}
					else
					{
						return false;
					}
				}

				if ((regexPattern.Type & RegexPattern.PatternType.Any) == RegexPattern.PatternType.Any)
				{
					isMatch = true;
				}
				else if ((regexPattern.Type & RegexPattern.PatternType.AnyFile) == RegexPattern.PatternType.AnyFile)
				{
					if (-1 == filePath.IndexOf(Path.DirectorySeparatorChar, filematchind))
					{
						isMatch = true;
					}
					else
					{
						return false;
					}
				}

				if ((regexPattern.Type & RegexPattern.PatternType.EndsWith) == RegexPattern.PatternType.EndsWith)
				{
					int pathSepShift = ((regexPattern.EndsWithPos > 1) && regexPattern.EndsWith[0] == Path.DirectorySeparatorChar) ? 1 : 0;

					if (filematchind + regexPattern.EndsWith.Length <= filePath.Length + pathSepShift)
					{
						isMatch = 0 == String.Compare(filePath, filePath.Length - regexPattern.EndsWith.Length, regexPattern.EndsWith, 0, regexPattern.EndsWith.Length, ignoreCase ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);

						if (isMatch && regexPattern.EndsWithPos == 1 && filePath.Length - regexPattern.EndsWith.Length - filematchind > 0)
						{
							isMatch = -1 == filePath.IndexOf(Path.DirectorySeparatorChar, filematchind, filePath.Length - regexPattern.EndsWith.Length - filematchind);
						}
					}
					else
					{
						isMatch = false;
					}
				}

			}

			return isMatch;
		}

		public class RegexPattern
		{
			public RegexPattern(PatternType type, bool isRecursive, string baseDir, String pattern, String dir, int dirPos, String endsWith, int endsWithPos, int maxRecursionLevel)
			{
				Type = type;
				IsRecursive = isRecursive;
				BaseDir = baseDir;
				Pattern = pattern;
				Dir = dir;
				DirPos = dirPos;
				EndsWith = endsWith;
				EndsWithPos = endsWithPos;
				MaxRecursionLevel = maxRecursionLevel;
			}
			public enum PatternType
			{
				Regex = 0,
				Any = 2,            // '**'
				AnyFile = 4,        // '*'
				Dir = 8,            // '**\xxx\**'
				EndsWith = 16,      // Any+EndsWith =='**.cpp';  AnyFile+EndsWith =='*.cpp'
			};
			public readonly PatternType Type;
			public readonly bool IsRecursive;
			public readonly string BaseDir;
			public readonly string Pattern;
			public readonly string EndsWith;
			public readonly int EndsWithPos;
			public readonly string Dir;
			public readonly int DirPos;
			public readonly int MaxRecursionLevel;
		}

		private static readonly ConcurrentDictionary<string, Regex> cachedCaseSensitiveRegexes = new ConcurrentDictionary<string, Regex>();
		private static readonly ConcurrentDictionary<string, Regex> cachedCaseInsensitiveRegexes = new ConcurrentDictionary<string, Regex>();


		/// <summary>Converts NAnt search pattern to a regular expression. Use cached version or compile and cache</summary>
		/// <param name="pattern">input pattern string</param>
		/// <param name="caseSensitive">Is volume case sensitive</param>
		/// <returns>Compiler Regular expression</returns>

		public static Regex ToRegex(string pattern, bool caseSensitive)
		{
			ConcurrentDictionary<string, Regex> regexCache = caseSensitive ? cachedCaseSensitiveRegexes : cachedCaseInsensitiveRegexes;

			Regex r = regexCache.GetOrAdd(pattern, ptrn =>
				{
					RegexOptions regexOptions = RegexOptions.Compiled;

					if (!caseSensitive)
						regexOptions |= RegexOptions.IgnoreCase;

					return new Regex(ptrn, regexOptions);

				});

			return r;
		}


		/// <summary>Converts NAnt search pattern to a regular expression pattern</summary>
		/// <param name="nantPattern">Search pattern relative to the search directory</param>
		/// <returns>Regular expression (absolute path) for searching matching file/directory names</returns>
		public static string ToRegexPatternOpt(string nantPattern)
		{
			StringBuilder pattern = new StringBuilder(nantPattern);

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

			// Special case directory separator string under Windows.
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

		public static bool EndsWith(string value, char c)
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}

			int stringLength = value.Length;
			if ((stringLength != 0) && (value[stringLength - 1] == c))
			{
				return true;
			}
			return false;
		}

		protected void InitBaseDir()
		{
			// To avoid increasing size of the pattern object by adding extra sync object instance.
			lock (this)
			{
				if (BaseDirectory == null)
				{
					BaseDirectory = Environment.CurrentDirectory;
				}
			}
		}
	}

	// <includes name="foo.txt" asis="true" /> --> foo.txt
	public class AbsolutePattern : Pattern
	{
		public override List<string> GetMatchingFiles(bool failOnMissingFile, List<RegexPattern> excludePatterns, StringCollection excludeFileNames)
		{
			InitBaseDir();

			List<string> collection = new List<string>();
			bool include = true;
			foreach (string pattern in excludeFileNames)
			{
				// A simple string comparison is used if the excluded item has 'asis' set to true.
				// we just use the pattern as it is without any conversions. Any regular expressions
				// in this pattern will be treated as a string and will not be resolved.
				if (pattern.Equals(Value))
				{
					include = false;
					break;
				}
			}

			if (include)
				collection.Add(Value);

			return collection;
		}
	}



	// <includes name="work/foo.txt"/> -> c:/work/foo.txt
	public class ExplictPattern : Pattern
	{
		public override List<string> GetMatchingFiles(bool failOnMissingFile, List<RegexPattern> excludePatterns, StringCollection excludeFileNames)
		{
			InitBaseDir();

			string path;
			if (Path.IsPathRooted(Value))
			{
				path = Value;
			}
			else
			{
				path = Path.Combine(BaseDirectory, Value);
			}

			// normalize the path
			path = PathNormalizer.Normalize(path);

			List<string> collection = new List<string>();

			bool caseInsenitive = !PathUtil.IsCaseSensitive;

			StringComparison compareOptions = caseInsenitive ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal;

			failOnMissingFile = this.FailOnMissingFile == false ? this.FailOnMissingFile : failOnMissingFile;

			List<RegexPattern> excludedPatterns = new List<RegexPattern>();
			string searchDirectory = String.Empty;
			if (excludePatterns != null && excludePatterns.Count > 0)
			{
				searchDirectory = Path.GetDirectoryName(path);
			}

			foreach (RegexPattern pattern in excludePatterns)
			{
				string baseDirectory = pattern.BaseDir;

				if (pattern.BaseDir.Length == 0 || String.Compare(searchDirectory, baseDirectory, compareOptions) == 0)
				{
					excludedPatterns.Add(pattern);
				}
				else
				{
					// check if the directory being searched is subdirectory of 
					// base directory of RegexEntry

					if (!pattern.IsRecursive)
					{
						continue;
					}

					// make sure base directory ends with directory separator
					if (!EndsWith(baseDirectory, Path.DirectorySeparatorChar))
					{
						baseDirectory += Path.DirectorySeparatorChar;
					}

					// check if path is subdirectory of base directory
					if (searchDirectory.StartsWith(baseDirectory, compareOptions))
					{
						excludedPatterns.Add(pattern);
					}
				}
			}

			if (IsPathInPatternSet(path, null, caseInsenitive, excludedPatterns, excludeFileNames))
			{
				// explicitly included files throw an error if they are missing
				if (failOnMissingFile && !File.Exists(path))
				{
					string msg = String.Format("Cannot find explicitly included file '{0}'.", path);
					throw new BuildException(msg);
				}

				collection.Add(path);
			}
			return collection;
		}

		/// <summary>Adds regular expressions for any non-explicit (ie, uses wild cards) NAnt patterns.</summary>
		/// <param name="nantPattern">The NAnt pattern to convert.  Absolute or relative paths.</param>
		/// <param name="absoluteFileName">Absolute pattern will return file name here.</param>
		/// <returns>The resulting regular expression. Absolute canonical path.</returns>
		/// <remarks>
		///		<para>Only NAnt patterns that contain a wild card character as converted to regular expressions.
		///		Explicit patterns are expected to be handled using absoluteFileName.</para>
		/// </remarks>
		override public RegexPattern ConvertPattern(string nantPattern, out string absoluteFileName)
		{
			InitBaseDir();

			string path;
			if (Path.IsPathRooted(Value))
			{
				path = Value;
			}
			else
			{
				path = Path.Combine(BaseDirectory, Value);
			}

			// normalize the path
			absoluteFileName = PathNormalizer.Normalize(path);

			return null;
		}

	}

	internal class FileInfoComparer : IComparer<FileInfo>
	{
		public int Compare(FileInfo a, FileInfo b)
		{
			return String.Compare(a.Name, b.Name, StringComparison.OrdinalIgnoreCase);
		}
	}

	internal class FileSystemInfoComparer : IComparer<FileSystemInfo>
	{
		public int Compare(FileSystemInfo a, FileSystemInfo b)
		{
			return String.Compare(a.Name, b.Name, StringComparison.OrdinalIgnoreCase);
		}
	}



	// <includes name="**/*.txt" />
	public class ImplicitPattern : Pattern
	{
		public override List<string> GetMatchingFiles(bool failOnMissingFile, List<RegexPattern> excludePatterns, StringCollection excludeFileNames)
		{
			InitBaseDir();

			List<string> matchingFiles = new List<string>();

			// convert the NAnt pattern to a regular expression
			string explicitFileName;
			RegexPattern regexPattern = ConvertPattern(Value, out explicitFileName);


			// recurse directory looking for matches
			ScanDirectoryOpt(regexPattern.BaseDir, regexPattern, excludePatterns, excludeFileNames, matchingFiles, 0, regexPattern.MaxRecursionLevel);
			return matchingFiles;
		}

		private static FileSystemInfo[] GetLiveFileSystemInfo(string directory)
		{
			DirectoryInfo directoryInfo = new DirectoryInfo(directory);
			FileSystemInfo[] info = directoryInfo.GetFileSystemInfos();
			Array.Sort(info, new FileSystemInfoComparer());

			return info;
		}

		static FileSystemInfo[] GetCachedFileSystemInfo(string directory)
		{
			return s_fileSystemInfoCache.GetOrAdd(directory, (key) =>
			{
				DirectoryInfo directoryInfo = new DirectoryInfo(key);
				FileSystemInfo[] info = directoryInfo.GetFileSystemInfos();

				// scan files and directories.  We set a default sort order (alphabetic, ignoring case) in order for the file list to be returned in a deterministic order
				Array.Sort(info, new FileSystemInfoComparer());
				return info;
			});
		}

		public static void SetDirectoryContentsCachingPolicy(bool useCache)
		{
			s_useDirCache = useCache;
		}



		/// <summary>Searches a directory recursively for files and directories matching the search criteria</summary>
		/// <param name="directoryPath">Directory in which to search (absolute canonical path)</param>
		/// <param name="regexPattern">Regular expression to match</param>
		/// <param name="excludePatterns">List of regular expression patterns to exclude</param>
		/// <param name="excludeFileNames">List of explicit file names to exclude</param>
		/// <param name="matchingFiles"></param>
		/// <param name="recursion_level"></param>
		/// <param name="MaxRecursionLevel"></param>
		private void ScanDirectoryOpt(string directoryPath, RegexPattern regexPattern, List<RegexPattern> excludePatterns, StringCollection excludeFileNames, List<string> matchingFiles, int recursion_level, int MaxRecursionLevel)
		{
			// if the directoryPath doesn't exist, return.
			if (s_useDirCache)
			{
				if (!s_existanceCache.GetOrAdd(directoryPath, key => Directory.Exists(key)))
				{
					return;
				}
			}
			else
			{
				if (!Directory.Exists(directoryPath))
				{
					return;
				}
			}

			// pre-normalize path to not do it in the loop
			directoryPath = PathNormalizer.Normalize(directoryPath);
			bool caseInsenitive = !PathUtil.IsCaseSensitive;

			StringComparison compareOptions = caseInsenitive ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal;

			List<RegexPattern> excludedPatterns = new List<RegexPattern>();

			foreach (RegexPattern pattern in excludePatterns)
			{
				string baseDirectory = pattern.BaseDir;

				if (pattern.BaseDir.Length == 0 || String.Compare(directoryPath, baseDirectory, compareOptions) == 0)
				{
					excludedPatterns.Add(pattern);
				}
				else
				{
					// check if the directory being searched is subdirectory of 
					// base directory of RegexEntry

					if (!pattern.IsRecursive)
					{
						continue;
					}

					// make sure base directory ends with directory separator
					if (!EndsWith(baseDirectory, Path.DirectorySeparatorChar))
					{
						baseDirectory += Path.DirectorySeparatorChar;
					}

					// check if path is subdirectory of base directory
					if (directoryPath.StartsWith(baseDirectory, StringComparison.OrdinalIgnoreCase))
					{
						excludedPatterns.Add(pattern);
					}
				}
			}

			string dirSeparator = Path.DirectorySeparatorChar.ToString();

			if (directoryPath.EndsWith(PathNormalizer.AltDirectorySeparatorChar.ToString(), StringComparison.Ordinal)
				|| directoryPath.EndsWith(Path.DirectorySeparatorChar.ToString(), StringComparison.Ordinal))
				// path already has a separator at the end. do not need it.
				dirSeparator = "";

			if (regexPattern.IsRecursive && recursion_level < MaxRecursionLevel)
			{
				FileSystemInfo[] infos = s_useDirCache ? GetCachedFileSystemInfo(directoryPath) : GetLiveFileSystemInfo(directoryPath);

				string dirPathPlusSeparator = directoryPath + dirSeparator;
				StringBuilder builder = new StringBuilder(1024);
				// From each filesystem info we need:
				// full name
				// partial name
				// attributes (is directory or not)
				foreach (FileSystemInfo findData in infos)
				{
					if ((findData.Attributes & FileAttributes.Directory) != 0)
					{
						builder.Clear();
						builder.Append(dirPathPlusSeparator).Append(findData.Name);
						string fullName = builder.ToString();

						// scan subfolders if we are running recursively
						ScanDirectoryOpt(fullName, regexPattern, excludePatterns, excludeFileNames, matchingFiles, recursion_level + 1, MaxRecursionLevel);
					}
				}

				// scan files.  We set a default sort order (alphabetic, ignoring case) in order for the file list to be returned in a deterministic order                
				foreach (FileSystemInfo findData in infos)
				{
					if ((findData.Attributes & FileAttributes.Directory) == 0)
					{
						// string targetPath = Path.Combine(directoryPath, fileInfo.Name) ;
						//
						// PERFORMANCE
						// we have normalized directory at this point. it is all
						// warm and fuzzy here. we can use this fact to utilize
						// more efficient path concatenation algorithm:
						//

						builder.Clear();
						builder.Append(dirPathPlusSeparator).Append(findData.Name);
						string targetPath = builder.ToString();

						if (IsPathInPatternSet(targetPath, regexPattern, caseInsenitive, excludedPatterns, excludeFileNames))
						{
							matchingFiles.Add(targetPath);
						}
					}
				}
			}
			else
			{
				// get info for the current directory
				DirectoryInfo currentDirectoryInfo = new DirectoryInfo(directoryPath);

				FileInfo[] fileInfos = currentDirectoryInfo.GetFiles();
				Array.Sort(fileInfos, new FileInfoComparer());

				foreach (FileInfo fileInfo in fileInfos)
				{
					// string targetPath = Path.Combine(directoryPath, fileInfo.Name) ;
					//
					// PERFORMANCE
					// we have normalized directory at this point. it is all
					// warm and fuzzy here. we can use this fact to utilize
					// more efficient path concatenation algorithm:
					//

					string targetPath = directoryPath + dirSeparator + fileInfo.Name;

					if (IsPathInPatternSet(targetPath, regexPattern, caseInsenitive, excludedPatterns, excludeFileNames))
					{
						matchingFiles.Add(targetPath);
					}
				}
			}
		}

		/// <summary>Adds regular expressions for any non-explicit (ie, uses wild cards) NAnt patterns.</summary>
		/// <param name="nantPattern">The NAnt pattern to convert.  Absolute or relative paths.</param>
		/// <param name="absoluteFileName">Absolute pattern will return file name here.</param>
		/// <returns>The resulting regular expression. Absolute canonical path.</returns>
		/// <remarks>
		///		<para>Only NAnt patterns that contain a wild card character as converted to regular expressions.
		///		Explicit patterns are expected to be handled using absoluteFileName.</para>
		/// </remarks>
		override public RegexPattern ConvertPattern(string nantPattern, out string absoluteFileName)
		{
			absoluteFileName = null;

			// normalize directory separators
			string normalizedPattern = nantPattern.Replace(PathNormalizer.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);

			var regpattern = ParseSearchDirectoryAndPatternOpt(normalizedPattern);

			return regpattern;
		}



		/// <summary>Given a NAnt search pattern returns a search directory and an regex search pattern.</summary>
		/// <param name="originalNAntPattern">NAnt search pattern (relative to the base directory OR absolute, relative paths referring to parent directories ( ../ ) also supported)</param>   
		private RegexPattern ParseSearchDirectoryAndPatternOpt(string originalNAntPattern)
		{
			string tempPattern = originalNAntPattern;
			// Already have normalized pattern.
			//s = s.Replace(PathNormalizer.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);

			// Get indices of pieces used for recursive check only
			int indexOfFirstDirectoryWildcard = tempPattern.IndexOf("**");
			int indexOfLastOriginalDirectorySeparator = tempPattern.LastIndexOf(Path.DirectorySeparatorChar);

			// search for the first wild card character (if any) and exclude the rest of the string beginning from the character
			int indexOfFirstWildcard = tempPattern.IndexOfAny(Pattern.Wildcards);
			if (indexOfFirstWildcard != -1)
			{
				// if found any wild card characters
				tempPattern = tempPattern.Substring(0, indexOfFirstWildcard);
			}

			// find the last DirectorySeparatorChar (if any) and exclude the rest of the string
			int indexOfLastDirectorySeparator = tempPattern.LastIndexOf(Path.DirectorySeparatorChar);

			// substring preceding the separator represents our search directory and the part 
			// following it represents NAnt search pattern relative to it
			if (indexOfLastDirectorySeparator != -1)
			{
				tempPattern = originalNAntPattern.Substring(0, indexOfLastDirectorySeparator);
			}
			else
			{
				tempPattern = String.Empty;
			}

			string searchDirectory = null;
			if (BaseDirectory.IsNullOrEmpty())
			{
				searchDirectory = new DirectoryInfo(tempPattern).FullName;
			}
			else
			{
				if (!Path.IsPathRooted(BaseDirectory))
				{
					BaseDirectory = Path.GetFullPath(BaseDirectory);
				}

				searchDirectory = new DirectoryInfo(Path.Combine(BaseDirectory, tempPattern)).FullName;
			}

			var recursive = (indexOfFirstWildcard != -1 && (indexOfFirstWildcard < indexOfLastOriginalDirectorySeparator)) || indexOfFirstDirectoryWildcard != -1;

			string modifiedNAntPattern = originalNAntPattern.Substring(indexOfLastDirectorySeparator + 1);

			//Optimize pattern so that we have more effective Regex expression better regular expression
			if (modifiedNAntPattern.StartsWith("**" + Path.DirectorySeparatorChar + "**", StringComparison.Ordinal))
			{
				modifiedNAntPattern = modifiedNAntPattern.Substring(3);
			}
			else if (modifiedNAntPattern.StartsWith("**" + Path.DirectorySeparatorChar + "*", StringComparison.Ordinal))
			{
				modifiedNAntPattern = "*" + modifiedNAntPattern.Substring(3);
			}

			var type = RegexPattern.PatternType.Regex;
			string endsWith = null;
			string regexPattern = null;
			string dir = null;
			int dirPos = 0;
			int endsWithPos = 0;


			if (-1 == modifiedNAntPattern.IndexOf('?'))
			{

				int nonstar = 0;
				while (nonstar < modifiedNAntPattern.Length && modifiedNAntPattern[nonstar] == '*')
				{
					nonstar++;
				};

				if (nonstar == modifiedNAntPattern.Length)
				{
					type |= (nonstar == 1) ? RegexPattern.PatternType.AnyFile : RegexPattern.PatternType.Any;
				}
				else
				{
					int enddir = modifiedNAntPattern.IndexOf('*', nonstar);
					if (enddir == -1)
					{
						type |= RegexPattern.PatternType.EndsWith;
						type |= (nonstar < 2 && - 1 == modifiedNAntPattern.IndexOf(Path.DirectorySeparatorChar, nonstar)) ? RegexPattern.PatternType.AnyFile : RegexPattern.PatternType.Any;
						endsWithPos = nonstar;
						endsWith = modifiedNAntPattern.Substring(nonstar);
					}
					else
					{
						if (enddir == modifiedNAntPattern.Length - 1)
						{
							type |= RegexPattern.PatternType.AnyFile;
						}
						else
						{
							int nonstar1 = enddir + 1;
							while (nonstar1 < modifiedNAntPattern.Length && modifiedNAntPattern[nonstar1] == '*')
							{
								nonstar1++;
							};

							if (nonstar1 == modifiedNAntPattern.Length)
							{
								type |= (nonstar1 - enddir == 1) ? RegexPattern.PatternType.AnyFile : RegexPattern.PatternType.Any;
							}
							else if (-1 == modifiedNAntPattern.IndexOf('*', nonstar1))
							{
								type |= RegexPattern.PatternType.EndsWith;
								type |= (nonstar1 - enddir == 1 && -1 == modifiedNAntPattern.IndexOf(Path.DirectorySeparatorChar, nonstar1)) ? RegexPattern.PatternType.AnyFile : RegexPattern.PatternType.Any;

								endsWithPos = nonstar1-enddir;
								endsWith = modifiedNAntPattern.Substring(nonstar1);
								/*
								if (nonstar1 - enddir > 1 && modifiedNAntPattern[nonstar1] == Path.DirectorySeparatorChar)
								{
									endsWith = modifiedNAntPattern.Substring(nonstar1 + 1);
								}
								else
								{
									endsWith = modifiedNAntPattern.Substring(nonstar1);
								}
								 * */
							}
							else
							{
								type = RegexPattern.PatternType.Regex;
							}
						}
						if (type != RegexPattern.PatternType.Regex)
						{
							type |= RegexPattern.PatternType.Dir;
							dir = modifiedNAntPattern.Substring(nonstar, enddir - nonstar);
							dirPos = nonstar;
						}
					}
				}
			}

			if (type == RegexPattern.PatternType.Regex)
			{
				regexPattern = Pattern.ToRegexPatternOpt(modifiedNAntPattern);
			}
			int maxRecursionLevel = (indexOfFirstDirectoryWildcard == -1) ? modifiedNAntPattern.Count(ch => ch == Path.DirectorySeparatorChar) : Int32.MaxValue;

			return new RegexPattern(type, recursive, searchDirectory, regexPattern, dir, dirPos, endsWith, endsWithPos, maxRecursionLevel);
		}
	}


	/// <summary>singleton class for creating patterns</summary>
	public class PatternFactory
	{
		static PatternFactory _instance = null;

		// private constructor
		PatternFactory() { }

		static public PatternFactory Instance
		{
			get
			{
				if (_instance == null)
					_instance = new PatternFactory();
				return _instance;
			}
		}

		/// <summary>Creates a pattern with asis bit set to true and default basedir.</summary>
		public Pattern CreatePattern(string pattern)
		{
			return CreatePattern(pattern, null, true);
		}

		/// <summary>Creates a pattern with default basedir.</summary>
		public Pattern CreatePattern(string pattern, bool asis)
		{
			return CreatePattern(pattern, null, asis);
		}

		/// <summary>Creates a pattern with asis bit set to false.</summary>
		public Pattern CreatePattern(string pattern, string basedir)
		{
			return CreatePattern(pattern, basedir, false);
		}

		/// <summary>Creates a pattern.</summary>
		public Pattern CreatePattern(string pattern, string basedir, bool asis)
		{
			// "//" messes up regex matching
			pattern = pattern.Replace("//", "/");
			if (pattern.StartsWith(@"\\", StringComparison.Ordinal))
			{
				pattern = @"\" + pattern.Replace(@"\\", @"\");
			}
			else
			{
				pattern = pattern.Replace(@"\\", @"\");
			}
			pattern = pattern.Replace(@"/\", @"/");
			pattern = pattern.Replace(@"\/", @"\");

			Pattern finalPattern = null;

			if (Pattern.IsAbsolutePattern(pattern, asis))
			{
				finalPattern = new AbsolutePattern();
				finalPattern.Value = pattern;
				finalPattern.BaseDirectory = basedir;
			}
			else if (Pattern.IsExplicitPattern(pattern))
			{
				finalPattern = new ExplictPattern();
				finalPattern.Value = pattern;
				finalPattern.BaseDirectory = basedir;
			}
			else if (Pattern.IsImplicitPattern(pattern))
			{
				finalPattern = new ImplicitPattern();
				finalPattern.Value = pattern;
				finalPattern.BaseDirectory = basedir;
			}
			else
			{
				string msg = String.Format("Could not create pattern: {0}", pattern);
				throw new BuildException(msg);
			}
			return finalPattern;
		}

	}
}
