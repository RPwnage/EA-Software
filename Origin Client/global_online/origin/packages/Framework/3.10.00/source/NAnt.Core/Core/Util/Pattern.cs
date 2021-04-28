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

/*
Examples:
"**\*.class" matches all .class files/dirs in a directory tree.

"test\a??.java" matches all files/dirs which start with an 'a', then two
more characters and then ".java", in a directory called test.

"**" matches everything in a directory tree.

"**\test\**\XYZ*" matches all files/dirs that start with "XYZ" and where
there is a parent directory called test (e.g. "abc\test\def\ghi\XYZ123").

Example of usage:

PatternCollection includes = new PatternCollection();
string dir = "test";
includes.Add(PatternFactory.Instance.CreatePattern("**\\*.class", dir, false));
includes.Add(PatternFactory.Instance.CreatePattern("**\\*.class", dir, false));
foreach (string filename in GetMatchingFiles()) {
    Console.WriteLine(filename);
}
*/
using System;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Collections.Concurrent;

namespace NAnt.Core.Util
{
    public abstract class Pattern
    {
        public static readonly char[] Wildcards = { '?', '*' };
        string _pattern = null;
        string _basedir = null;
        bool _failOnMissingFile = true;
        public bool PreserveBaseDirValue = false;

        public abstract List<string> GetMatchingFiles(bool failOnMissingFile, List<RegexPattern> excludePatterns, StringCollection excludeFileNames);

        public string Value
        {
            get { return _pattern; }
            set { _pattern = value; }
        }

        public string BaseDirectory
        {
            get { return _basedir; }
            set { _basedir = value; }
        }

        public bool FailOnMissingFile
        {
            get { return _failOnMissingFile; }
            set { _failOnMissingFile = value; }
        }

        /// <summary>Check if the specified pattern does not contain any wildcards.</summary>
        /// <returns>True if the pattern does not contain any wildcards, otherwise false.</returns>
        public static bool IsExplicitPattern(string pattern)
        {
            return (pattern.IndexOfAny(Pattern.Wildcards) == -1);
        }

        /// <summary>Check if the specified pattern contains any wildcards.</summary>
        /// <returns>True if the pattern contains any wildcards, otherwise false.</returns>
        public static bool IsImplicitPattern(string pattern)
        {
            return (pattern.IndexOfAny(Pattern.Wildcards) != -1);
        }

        public static bool IsAbsolutePattern(string pattern, bool asis)
        {
            return (asis);
        }

        /// <summary>Adds regular expressions for any non-explicit (ie, uses wildcards) nant patterns.</summary>
        /// <param name="nantPattern">The NAnt pattern to convert.  Absolute or relative paths.</param>
        /// <param name="absoluteFileName">Absolute pattern will return file name here.</param>
        /// <returns>The resulting regular expression. Absolute canonical path.</returns>
        /// <remarks>
        ///		<para>Only nant patterns that contain a wildcard character as converted to regular expressions.
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
                Regex r = ToRegex(regexPattern.Pattern, !ignoreCase);

                // Check to see if the empty string matches the pattern
                if (filePath.Length == regexPattern.BaseDir.Length)
                {
                    included = r.IsMatch(String.Empty);
                }
                else
                {
                    bool endsWithSlash = EndsWith(regexPattern.BaseDir, Path.DirectorySeparatorChar);

                    if (endsWithSlash)
                    {
                        included = r.IsMatch(filePath.Substring(regexPattern.BaseDir.Length));
                    }
                    else
                    {
                        included = r.IsMatch(filePath.Substring(regexPattern.BaseDir.Length + 1));
                    }
                }
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
                    Regex r = ToRegex(rPattern.Pattern, !ignoreCase);
                    bool test = false;

                    // Check to see if the empty string matches the pattern
                    if (filePath.Length == rPattern.BaseDir.Length)
                    {
                        test = r.IsMatch(String.Empty);
                    }
                    else
                    {
                        bool endsWithSlash = EndsWith(rPattern.BaseDir, Path.DirectorySeparatorChar);

                        if (endsWithSlash)
                        {
                            test = r.IsMatch(filePath.Substring(rPattern.BaseDir.Length));
                        }
                        else
                        {
                            test = r.IsMatch(filePath.Substring(rPattern.BaseDir.Length + 1));
                        }
                    }
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

        public class RegexPattern
        {
            public bool IsRecursive;
            public string BaseDir;
            public string Pattern;
        }

        private static readonly ConcurrentDictionary<string, Regex> cachedCaseSensitiveRegexes = new ConcurrentDictionary<string, Regex>();
        private static readonly ConcurrentDictionary<string, Regex> cachedCaseInsensitiveRegexes = new ConcurrentDictionary<string, Regex>();


        /// <summary>Converts NAnt search pattern to a regular expression. Use cached version or compile and cache</summary>
        /// <param name="pattern">input pattern string</param>
        /// <param name="caseSensitive">Is volume case sensitive</param>
        /// <returns>Compiler Regular expresssion</returns>

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
        /// <returns>Regular expresssion (absolute path) for searching matching file/directory names</returns>
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
                    // basedirectory of RegexEntry

                    if (!pattern.IsRecursive)
                    {
                        continue;
                    }

                    // make sure basedirectory ends with directory separator
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

        /// <summary>Adds regular expressions for any non-explicit (ie, uses wildcards) nant patterns.</summary>
        /// <param name="nantPattern">The NAnt pattern to convert.  Absolute or relative paths.</param>
        /// <param name="absoluteFileName">Absolute pattern will return file name here.</param>
        /// <returns>The resulting regular expression. Absolute canonical path.</returns>
        /// <remarks>
        ///		<para>Only nant patterns that contain a wildcard character as converted to regular expressions.
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

            // convert the nant pattern to a regular expression
            string explicitFileName;
            RegexPattern regexPattern = ConvertPattern(Value, out explicitFileName);

            // recurse directory looking for matches
            ScanDirectoryOpt(regexPattern.BaseDir, regexPattern, excludePatterns, excludeFileNames, matchingFiles);
            return matchingFiles;
        }

        /// <summary>Searches a directory recursively for files and directories matching the search criteria</summary>
        /// <param name="directoryPath">Directory in which to search (absolute canonical path)</param>
        /// <param name="regexPattern">Regular expression to match</param>
        /// <param name="excludePatterns">List of regular expression patterns to exclude</param>
        /// <param name="excludeFileNames">List of explicit file names to exclude</param>
        /// <param name="matchingFiles"></param>
        void ScanDirectoryOpt(string directoryPath, RegexPattern regexPattern, List<RegexPattern> excludePatterns, StringCollection excludeFileNames, List<string> matchingFiles)
        {
            // if the directoryPath doesn't exist, return.
            if (!Directory.Exists(directoryPath))
            {
                return;
            }

            // pre-normalize path to not do it in the loop
            directoryPath = PathNormalizer.Normalize(directoryPath);
            bool caseInsenitive = !PathUtil.IsCaseSensitive;

            StringComparison compareOptions = caseInsenitive ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal;

            // get info for the current directory
            DirectoryInfo currentDirectoryInfo = new DirectoryInfo(directoryPath);

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
                    // basedirectory of RegexEntry

                    if (!pattern.IsRecursive)
                    {
                        continue;
                    }

                    // make sure basedirectory ends with directory separator
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

            if (regexPattern.IsRecursive)
            {
                FileSystemInfo[] infos = currentDirectoryInfo.GetFileSystemInfos();
                // scan files and directories.  We set a default sort order (alphabetic, ignoring case) in order for the file list to be returned in a deterministic order
                Array.Sort(infos, new FileSystemInfoComparer());
                foreach (FileSystemInfo directoryInfo in infos)
                {
                    if ((directoryInfo.Attributes & FileAttributes.Directory) != 0)
                    {
                        // scan subfolders if we are running recursively                    
                        ScanDirectoryOpt(directoryInfo.FullName, regexPattern, excludePatterns, excludeFileNames, matchingFiles);
                    }
                }

                // scan files.  We set a default sort order (alphabetic, ignoring case) in order for the file list to be returned in a deterministic order                
                foreach (FileSystemInfo fileInfo in infos)
                {
                    if ((fileInfo.Attributes & FileAttributes.Directory) == 0)
                    {
                        // string targetPath = Path.Combine(directoryPath, fileInfo.Name) ;
                        //
                        // PERFORMANCE
                        // we have normalized directory at this point. it is all
                        // warm and fuzzy here. we can use this fact to utilize
                        // more efficient path concatenation algorighm:
                        //

                        string targetPath = directoryPath + dirSeparator + fileInfo.Name;

                        if (IsPathInPatternSet(targetPath, regexPattern, caseInsenitive, excludedPatterns, excludeFileNames))
                        {
                            matchingFiles.Add(targetPath);
                        }
                    }
                }
            }
            else
            {
                FileInfo[] fileInfos = currentDirectoryInfo.GetFiles();
                Array.Sort(fileInfos, new FileInfoComparer());

                foreach (FileInfo fileInfo in fileInfos)
                {
                    // string targetPath = Path.Combine(directoryPath, fileInfo.Name) ;
                    //
                    // PERFORMANCE
                    // we have normalized directory at this point. it is all
                    // warm and fuzzy here. we can use this fact to utilize
                    // more efficient path concatenation algorighm:
                    //

                    string targetPath = directoryPath + dirSeparator + fileInfo.Name;

                    if (IsPathInPatternSet(targetPath, regexPattern, caseInsenitive, excludedPatterns, excludeFileNames))
                    {
                        matchingFiles.Add(targetPath);
                    }
                }
            }
        }

        /// <summary>Adds regular expressions for any non-explicit (ie, uses wildcards) nant patterns.</summary>
        /// <param name="nantPattern">The NAnt pattern to convert.  Absolute or relative paths.</param>
        /// <param name="absoluteFileName">Absolute pattern will return file name here.</param>
        /// <returns>The resulting regular expression. Absolute canonical path.</returns>
        /// <remarks>
        ///		<para>Only nant patterns that contain a wildcard character as converted to regular expressions.
        ///		Explicit patterns are expected to be handled using absoluteFileName.</para>
        /// </remarks>
        override public RegexPattern ConvertPattern(string nantPattern, out string absoluteFileName)
        {
            string regexPattern = null;
            string searchDirectory = null;
            bool recursive = false;
            absoluteFileName = null;

            // normalize directory seperators
            string normalizedPattern = nantPattern.Replace(PathNormalizer.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);

            ParseSearchDirectoryAndPatternOpt(normalizedPattern, out searchDirectory, out recursive, out regexPattern);

            RegexPattern regpattern = new RegexPattern();
            regpattern.IsRecursive = recursive;
            regpattern.BaseDir = searchDirectory;
            regpattern.Pattern = regexPattern;

            return regpattern;
        }



        /// <summary>Given a NAnt search pattern returns a search directory and an regex search pattern.</summary>
        /// <param name="originalNAntPattern">NAnt searh pattern (relative to the Basedirectory OR absolute, relative paths refering to parent directories ( ../ ) also supported)</param>
        /// <param name="searchDirectory">Out. Absolute canonical path to the directory to be searched</param>
        /// <param name="recursive">Out. Indicates whether directory should be parsed recursively</param>
        /// <param name="regexPattern">Out. Regex search pattern (absolute canonical path)</param>        
        void ParseSearchDirectoryAndPatternOpt(string originalNAntPattern, out string searchDirectory, out bool recursive, out string regexPattern)
        {
            string s = originalNAntPattern;
            // Already have normalized pattern.
            //s = s.Replace(PathNormalizer.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);

            // Get indices of pieces used for recursive check only
            int indexOfFirstDirectoryWildcard = s.IndexOf("**");
            int indexOfLastOriginalDirectorySeparator = s.LastIndexOf(Path.DirectorySeparatorChar);

            // search for the first wildcard character (if any) and exclude the rest of the string beginnning from the character
            int indexOfFirstWildcard = s.IndexOfAny(Pattern.Wildcards);
            if (indexOfFirstWildcard != -1)
            {
                // if found any wildcard characters
                s = s.Substring(0, indexOfFirstWildcard);
            }

            // find the last DirectorySeparatorChar (if any) and exclude the rest of the string
            int indexOfLastDirectorySeparator = s.LastIndexOf(Path.DirectorySeparatorChar);

            recursive = (indexOfFirstWildcard != -1 && (indexOfFirstWildcard < indexOfLastOriginalDirectorySeparator)) || indexOfFirstDirectoryWildcard != -1;


            // substring preceding the separator represents our search directory and the part 
            // following it represents nant search pattern relative to it
            if (indexOfLastDirectorySeparator != -1)
            {
                s = originalNAntPattern.Substring(0, indexOfLastDirectorySeparator);
            }
            else
            {
                s = "";
            }

            // combine the relative path with the base directory and canonize the resulting path
            if (!Path.IsPathRooted(s))
            {

                searchDirectory = new DirectoryInfo(Path.Combine(BaseDirectory, s)).FullName;
            }
            else
            {
                searchDirectory = new DirectoryInfo(s).FullName;
            }
            string modifiedNAntPattern = originalNAntPattern.Substring(indexOfLastDirectorySeparator + 1);

            regexPattern = Pattern.ToRegexPatternOpt(modifiedNAntPattern);
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
