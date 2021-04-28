using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

using NAnt.Core.Util;

namespace NAnt.Core
{

    public static class StringExtensions
    {

        public static bool IsOptionBoolean(this string val)
        {
            bool ret = false;
            if (!String.IsNullOrEmpty(val))
            {
                var tval = val.TrimWhiteSpace();
                if (tval.Equals("on", StringComparison.OrdinalIgnoreCase) || tval.Equals("true", StringComparison.OrdinalIgnoreCase) ||
                    tval.Equals("off", StringComparison.OrdinalIgnoreCase) || tval.Equals("false", StringComparison.OrdinalIgnoreCase))
                {
                    ret = true;
                }
            }
            return ret;
        }

        public static bool OptionToBoolean(this string val)
        {
            bool ret = false;
            if (!String.IsNullOrEmpty(val))
            {
                var tval = val.TrimWhiteSpace();
                if (tval.Equals("on", StringComparison.OrdinalIgnoreCase) || tval.Equals("true", StringComparison.OrdinalIgnoreCase))
                {
                    ret = true;
                }
            }
            return ret;
        }

        public static bool ToBoolean(this string val)
        {
            bool ret = false;
            if (!String.IsNullOrEmpty(val))
            {
                if (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
                {
                    ret = true;
                }
            }
            return ret;
        }

        public static bool ToBooleanOrDefault(this string val, bool def)
        {
            if (String.IsNullOrEmpty(val))
            {
                return def;
            }
            return val.TrimWhiteSpace().Equals("true", StringComparison.OrdinalIgnoreCase);
        }

        public static string Quote(this string val)
        {
            if (!String.IsNullOrEmpty(val) && !val.StartsWith("\"", StringComparison.Ordinal))
            {
                val = "\"" + val + "\"";
            }
            return val;
        }

        public static string TrimExtension(this string val)
        {
            if (!String.IsNullOrEmpty(val))
            {
                var ind = val.LastIndexOf('.');
                if (ind >= 0 && ind < val.Length)
                {
                    val = val.Substring(0, ind);
                }
            }
            return val;
        } 

        public static IList<string> ToArray(this string value, char[] delimArray = null)
        {
            return split(value, delimArray ?? _defaultDelimArray);
        }

        public static IList<string> LinesToArray(this string value)
        {
            return split(value, _linesDelimArray);
        }

        public static IList<string> ToArray(this string value, string delim)
        {
            if (String.IsNullOrEmpty(delim))
            {
                string msg = String.Format("StringExtensions.ToArray: empty delimiter.");
                throw new BuildException(msg);
            }

            string delimString;
            try
            {
                delimString = Regex.Unescape(delim);
            }
            catch (System.Exception e)
            {
                string msg = String.Format("StringExtensions.ToArray: : cannot unescape delimeter '{0}'.", delim);
                throw new BuildException(msg, e);
            }

            return split(value, delimString.ToCharArray());
        }

        public static bool IsEqual(this IList<string> x, IList<string> y, bool ignoreCase = false)
        {
            int countX = x == null ? 0 : x.Count;
            int countY = y == null ? 0 : y.Count;

            if (countX != countY)
            {
                return false;
            }
            else if (countX == 0)
            {
                return true;
            }

            for (int i = 0; i < countX; i++)
            {
                if (0 != String.Compare(x[i], y[i], ignoreCase))
                {
                    return false;
                }
            }
            return true;
        }

        public static IList<string> QuotedToArray(this string value)
        {
            List<string> list = new List<string>();
            if (!String.IsNullOrWhiteSpace(value))
            {
                foreach (Match match in quotedSplit.Matches(value))
                {
                    if (!String.IsNullOrEmpty(match.Groups["quoted"].Value))
                        list.Add(match.Groups["quoted"].Value);
                    else
                    {
                        // get the quoted string, but without the quotes.
                        list.Add(match.Groups["plain"].Value);
                    }
                }
            }
            return list;
        }

        public static bool ContainsArrayItem(this string array, string value)
        {
            bool ret = false;
            foreach (string item in array.ToArray())
            {
                if (item == value)
                {
                    ret = true;
                    break;
                }
            }
            return ret;
        }

        public static string ToNewLineString(this IEnumerable<string> array)
        {
            return array.ToString(Environment.NewLine);
        }

        public static string ToNewLineString(this IEnumerable array)
        {
            return array.ToString(Environment.NewLine);
        }

        public static string ToNewLineString<T>(this IEnumerable<T> array, Func<T, string> func)
        {
            return array.ToString(Environment.NewLine, func);
        }

        public static string NormalizeNewLineChars(this string value)
        {
            return value.LinesToArray().ToNewLineString();
        }

        public static string ToString(this IEnumerable<string> array, string sep)
        {
            if (array == null)
            {
                return String.Empty;
            }
            if (sep == null) sep = " ";

            StringBuilder sb = new StringBuilder();
            foreach (string s in array)
            {
                if (sb.Length > 0)
                {
                    sb.Append(sep);
                }
                sb.Append(s);

            }
            return sb.ToString();
        }

        public static string ToString(this IEnumerable array, string sep)
        {
            if (array == null)
            {
                return String.Empty;
            }
            if (sep == null) sep = " ";

            IEnumerator enumerator = array.GetEnumerator();
            StringBuilder sb = new StringBuilder();

            while (enumerator.MoveNext())
            {
                if (sb.Length > 0)
                {
                    sb.Append(sep);
                }
                sb.Append(enumerator.Current);
            }
            return sb.ToString();
        }

        public static string ToString<T>(this IEnumerable<T> array, string sep, Func<T, string> func)
        {
            if (array == null)
            {
                return String.Empty;
            }
            if (sep == null) sep = " ";

            IEnumerator<T> enumerator = array.GetEnumerator();
            StringBuilder sb = new StringBuilder();

            while (enumerator.MoveNext())
            {
                if (sb.Length > 0)
                {
                    sb.Append(sep);
                }
                sb.Append(func(enumerator.Current));
            }
            return sb.ToString();
        }

        public static string TrimQuotes(this string input)
        {
            string res = String.Empty;
            if (input != null)
            {
                res = input.Trim('"');
            }
            return res;
        }

        public static string TrimWhiteSpace(this string input)
        {
            string res = String.Empty;
            if (input != null)
            {
                res = input.Trim(_defaultDelimArray);
            }
            return res;
        }

        public static string TrimLeftSlash(this string input)
        {
            string res = String.Empty;
            if (input != null)
            {
                res = input.TrimStart('\\', '/');
            }
            return res;
        }

        public static string TrimRightSlash(this string input)
        {
            string res = String.Empty;
            if (input != null)
            {
                res = input.TrimEnd('\\', '/');
            }
            return res;
        }

        public static string EnsureTrailingSlash(this string path, string defaultPath=null)
        {
            if (!String.IsNullOrEmpty(path))
            {
                path = path.TrimEnd('\\', '/') + Path.DirectorySeparatorChar;
            } 
            else if (defaultPath != null)
            {
                path = defaultPath.TrimEnd('\\', '/') + Path.DirectorySeparatorChar;
            }
            return path;
        }

        public static string NormalizeText(this string text)
        {
            if (!String.IsNullOrEmpty(text))
            {
                // replace extra duplicate white spaces
                StringBuilder sb = new StringBuilder(text.Trim(_defaultDelimArray));
                sb.Replace("\t", " ");
                int len;
                do
                {
                    len = sb.Length;
                    sb.Replace("  ", " ");

                } while (len != sb.Length);

                text = sb.ToString();
            }
            return text;
        }

        public static int StrCompareVersions(this string strA, string strB)
        {
            int iA = 0;
            int iB = 0;

            if (Object.ReferenceEquals(strA, null))
            {
                string msg = String.Format("StringExtensions.StrCompareVersions: null comparison for first argument, strA.");
                throw new BuildException(msg);
            }

            if (Object.ReferenceEquals(strB, null))
            {
                string msg = String.Format("StringExtensions.StrCompareVersions: null comparison for second argument, strB.");
                throw new BuildException(msg);
            }

            if (strA != String.Empty)
            {
                if (strA.StartsWith("dev", StringComparison.Ordinal)) strA = "999" + strA.Substring(3);
                if (strA.StartsWith("work", StringComparison.Ordinal)) strA = "999" + strA.Substring(4);
            }
            if (strB != String.Empty)
            {
                if (strB.StartsWith("dev", StringComparison.Ordinal)) strB = "999" + strB.Substring(3);
                if (strB.StartsWith("work", StringComparison.Ordinal)) strB = "999" + strB.Substring(4);
            }

            if (ValueAt(strA, iA) == '-' || ValueAt(strB, iB) == '-') return 0; // treat options as same

            int diff = ValueAt(strA, iA) - ValueAt(strB, iB);

            while (iA <= strA.Length)
            {
                if (Char.IsDigit(ValueAt(strA, iA)) || Char.IsDigit(ValueAt(strB, iB)))
                {
                    int n0 = -1;
                    if (Char.IsDigit(ValueAt(strA, iA)))
                    {
                        System.Text.StringBuilder digits = new System.Text.StringBuilder();

                        while (iA < strA.Length && Char.IsDigit(ValueAt(strA, iA)))
                        {
                            digits.Append(strA[iA++]);
                        }

                        n0 = Int32.Parse(digits.ToString());
                    }

                    int n1 = -1;
                    if (Char.IsDigit(ValueAt(strB, iB)))
                    {
                        System.Text.StringBuilder digits = new System.Text.StringBuilder();

                        while (iB < strB.Length && Char.IsDigit(ValueAt(strB, iB)))
                        {
                            digits.Append(strB[iB++]);
                        }

                        n1 = Int32.Parse(digits.ToString());
                    }
                    diff = n0 - n1;
                }
                else
                {
                    diff = ValueAt(strA, iA) - ValueAt(strB, iB);
                    iA++;
                    iB++;
                }

                if (diff != 0) break;
            }
            return diff;
        }

        public static string ReplaceTemplateParameters(this string str, string parametername, string[,] templatevalue, Location location = null)
        {
            var result = str;
            if (!String.IsNullOrEmpty(result))
            {
                int len = templatevalue.GetLength(0);
                for (int i = 0; i < len; i++)
                {
                    result = result.Replace(templatevalue[i, 0], templatevalue[i, 1]);
                }
                if (result.Contains('%'))
                {
                    throw new BuildException(String.Format("'{0}' property contains invalid template pameter(s) : {1}", parametername, result), location ?? Location.UnknownLocation);
                }
            }
            return result;
        }

        public static bool ContainsMacro(this string str)
        {
            if (str!= null)
            {
                return str.Contains("$(");
            }
            return false;
        }


        private static IList<string> split(string value, char[] delim)
        {
            List<string> array = new List<string>();
            if (!String.IsNullOrEmpty(value))
            {
                foreach (string str in value.Split(delim, StringSplitOptions.RemoveEmptyEntries))
                {
                    string trimmedStr = str.Trim();
                    if (!String.IsNullOrEmpty(trimmedStr))
                    {
                        array.Add(trimmedStr);
                    }
                }
            }
            return array;
        }


        /// Helper function for StrCompareVersions. Returns character value at position pos, 
        /// or '\0' if index is outside the string
        /// <param name="str"> string</param>
        /// <param name="pos">position in the string</param>        
        private static char ValueAt(string str, int pos)
        {
            char val = '\0';
            if (pos < str.Length)
                val = str[pos];
            return val;
        }

        private static char[] _linesDelimArray = { 
            '\x000a', // newline
            '\x000d'  // carriage return
        };

        private static char[] _defaultDelimArray = { 
            '\x0009', // tab
            '\x000a', // newline
            '\x000d', // carriage return
            '\x0020'  // space
        };

        private static Regex quotedSplit = new Regex("((?<quoted>[^\\s]+=\"([^\"]*)\")|(?<plain>[^\\s]+))", RegexOptions.Compiled | RegexOptions.CultureInvariant);
    }
}
