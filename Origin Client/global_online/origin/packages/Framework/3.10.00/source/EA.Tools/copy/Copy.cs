using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

namespace EA.Tools
{
    internal class Copy
    {
        internal static readonly char[] Wildcards = { '?', '*' };

        internal static bool OverwriteReadOnlyFiles = false;
        internal static bool SkipUnchangedFiles = false;
        internal static bool UseHardlinksIfPossible = false;
        internal static bool Recurcive = false;
        internal static bool Silent = false;
        internal static bool CopyEmptyDir = false;
        internal static bool Info = false;
        internal static bool Warnings = false;
        internal static bool Summary = false;
        internal static bool IgnoreMissingInput = false;

        static private readonly bool isMonoRuntime = (Type.GetType("Mono.Runtime") != null);

        static private readonly bool caseSensitive = true;

        static private readonly ConcurrentDictionary<string, Regex> regexCollection = new ConcurrentDictionary<string, Regex>();

        static internal int copied = 0;
        static internal int uptodate = 0;
        static internal int linked = 0;

        static Copy()
        {
            switch (System.Environment.OSVersion.Platform)
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


        internal static int Execute(string src, string dst)
        {
            int err_count = 0;
            // Evaluate full path if input is relative to project path.
            src = Path.Combine(Environment.CurrentDirectory, src);
            dst = Path.Combine(Environment.CurrentDirectory, dst);

            if (IsExplicitPattern(src))
            {
                if(Directory.Exists(src))
                {
                    err_count += CopyDirectory(src, dst);
                }
                else
                {
                    if (dst.EndsWith("\\", StringComparison.Ordinal) || dst.EndsWith("/", StringComparison.Ordinal) || Directory.Exists(dst))
                    {
                        dst = Path.Combine(dst, Path.GetFileName(src));
                    }

                    err_count += CopyFile(src, dst);
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

                err_count += CopyDirectory(basedir, dst, pattern);
            }
            return err_count;
        }

        private static bool IsExplicitPattern(string pattern)
        {
            return (pattern.IndexOfAny(Copy.Wildcards) == -1);
        }

        private static bool IsImplicitPattern(string pattern)
        {
            return (pattern.IndexOfAny(Copy.Wildcards) != -1);
        }

        private static int CopyDirectory(string sourceFolder, string destFolder, string pattern = null)
        {
            int err_count = 0;
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
                    string dest = Path.Combine(destFolder, name);
                    err_count += CopyFile(file, dest);
                }
            }
            if (Recurcive)
            {
                foreach (var folder in Directory.GetDirectories(sourceFolder))
                {
                    string name = Path.GetFileName(folder);
                    string dest = Path.Combine(destFolder, name);
                    err_count += CopyDirectory(folder, dest, pattern);
                }
            }
            return err_count;
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


        private static int CopyFile(string src, string dst)
        {
            bool res = false;

            if (File.Exists(src))
            {
                if (NeedCopy(src, dst))
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(dst));

                    if (UseHardlinksIfPossible)
                    {
                        res = CreateHardLink(dst, src);

                        if (res)
                        {
                            linked++;
                        }
                        else
                        {
                            var win32Exception = new System.ComponentModel.Win32Exception(System.Runtime.InteropServices.Marshal.GetLastWin32Error());
                            if (!Silent && Warnings)
                            {
                                Console.WriteLine("Hardlinking had a problem {0}, will retry copying. {1}", win32Exception.Message, win32Exception);
                            }
                        }
                    }

                    if (!res)
                    {
                        var srcAttrib = File.GetAttributes(src);

                        if (OverwriteReadOnlyFiles && File.Exists(dst))
                        {
                            // must set file attribute to normal in order to clobber hidden/readonly attributes
                            File.SetAttributes(dst, FileAttributes.Normal);
                        }

                        try
                        {

                            File.Copy(src, dst, true);
                        }
                        catch (IOException ioe)
                        {
                            // if DST is a hardlink, it has to be deleted first.
                            if (ioe.Message.StartsWith("The process cannot access the file"))
                            {
                                File.Delete(dst);
                                File.Copy(src, dst, true);
                            }
                            else
                            {
                                throw;
                            }
                        }

                        File.SetAttributes(dst, srcAttrib);

                        if (!Silent && Info)
                        {
                            Console.WriteLine("copied {0} --> {1}", src, dst);
                        }
                        copied++;
                    }
                    res = true;
                }
                else
                {
                    res = true;
                    uptodate++;
                }
            }
            else
            {
                if (Warnings)
                {
                    Console.WriteLine("source {0} does not exist. ", src);
                }
                if (IgnoreMissingInput)
                {
                    res = true;
                }
            }

            return res ? 0 : 1;
        }

        private static bool NeedCopy(string src, string dst)
        {
            if (!File.Exists(dst))
            {
                return true;
            }
            else if (src == dst)
            {
                return false;
            }
            else if (!SkipUnchangedFiles  || File.GetLastWriteTime(src) > File.GetLastWriteTime(dst))
            {
                return true;
            }

            return false;
        }

        private static bool CreateHardLink(string dst, string src)
        {
            if (isMonoRuntime)
            {
                //IMTODO: Implement symlinks on Unix?
                return false;
            }

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
                catch (System.Exception)
                {
                }
            }
            bool res = true;
            // Sometimes VisualStudio locks files. Since we use hard links, the file must be already liked.
            // Can be done better with detecting hard link
            if (!File.Exists(dst))
            {
                res = CreateHardLink(dst, src, System.IntPtr.Zero);
            }
            if (res)
            {
                try
                {
                    File.SetAttributes(dst, srcAttrib);
                }
                catch (System.Exception)
                {
                    res = false;
                }
            }
            try
            {
                File.SetAttributes(src, srcAttrib);
            }
            catch (System.Exception)
            {
                res = false;
            }
            if (res && !Silent && Info)
            {
                Console.WriteLine("hardlink {0} --> {1}", src, dst);
            }

            return res;
        }

        [System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = true)]
        internal static extern bool CreateHardLink(string newFileName, string exitingFileName, System.IntPtr securityAttributes);


        private static string GetDriveLetter(string path)
        {
            if (String.IsNullOrEmpty(path))
            {
                return null;
            }

            // Some versions of Runtime do not have MacOSX definition. Use integer value 6
            if (Environment.OSVersion.Platform == System.PlatformID.Unix ||
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
                return System.String.Empty;
            }

            int ind = path.IndexOf(':');

            if (ind < 0) // It must be a network path. return host name as a drive letter
            {
                char[] PATH_SEPARATOR_CHARS_ALT = new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar };

                path = path.TrimStart(PATH_SEPARATOR_CHARS_ALT);
                ind = path.IndexOfAny(PATH_SEPARATOR_CHARS_ALT);
            }
            if (ind < 0)
            {
                return null;
            }
            return path.Substring(0, ind);
        }
    }
}
