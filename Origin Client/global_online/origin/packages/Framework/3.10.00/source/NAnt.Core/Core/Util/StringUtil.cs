using NAnt.Core;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

namespace NAnt.Core.Util
{
    public class StringUtil
    {
        public static bool GetBooleanValue(string val)
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

        public static string GetValueOrDefault(string val, string def)
        {            
            if (String.IsNullOrEmpty(val))
            {
                return def;
            }
            return val;
        }


        public static IList<string> ToArray(string value)
        {
            return split(value, _defaultDelimArray);
        }

        public static IList<string> ToArray(string value, char[] DelimArray)
        {
            return split(value, DelimArray);
        }

        public static IList<string> ToArray(string value, string delim)
        {
            if (String.IsNullOrEmpty(delim))
            {
                string msg = String.Format("StringUtil.ToArray: empty delimiter.");
                throw new BuildException(msg);
            }

            string delimString;
            try
            {
                delimString = Regex.Unescape(delim);
            }
            catch (System.Exception e)
            {
                string msg = String.Format("StringUtil.ToArray: : cannot unescape delimeter '{0}'.", delim);
                throw new BuildException(msg, e);
            }

            return split(value, delimString.ToCharArray());
        }

        public static bool ArraysEqual(IList<string> x, IList<string> y, bool ignoreCase = false)
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

            for(int i = 0; i < countX; i++)
            {
                if (0 != String.Compare(x[i], y[i], ignoreCase))
                {
                    return false;
                }
            }
            return true;
        }

        public static IList<string> QuotedToArray(string value)
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

        public static bool ArrayContains(string array, string value)
        {
            bool ret = false;
            foreach (string item in StringUtil.ToArray(array))
            {
                if (item == value)
                {
                    ret = true;
                    break;
                }
            }
            return ret;
        }

        public static string ArrayToString(IEnumerable<string> array, string sep)
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

        public static string ArrayToString(IEnumerable array, string sep)
        {
            if (array == null)
            {
                return String.Empty;
            }
            if (sep == null) sep = " ";

            IEnumerator enumerator = array.GetEnumerator();
            StringBuilder sb = new StringBuilder();

            while(enumerator.MoveNext())
            {
                    if (sb.Length > 0)
                    {
                        sb.Append(sep);
                    }
                    sb.Append(enumerator.Current);
            }
            return sb.ToString();
        }


        public static string TrimQuotes(string input)
        {
            string res = String.Empty;
            if (input != null)
            {
                res = input.Trim('"');
            }
            return res;
        }

        public static string Trim(string input)
        {
            string res = String.Empty;
            if (input != null)
            {
                res = input.Trim(_defaultDelimArray);
            }
            return res;
        }

        public static string NormalizeText(string text)
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


        public static int StrCompareVersions(string strA, string strB)
        {
            int iA = 0;
            int iB = 0;

            if (Object.ReferenceEquals(strA, null))
            {
                string msg = String.Format("StringUtil.StrCompareVersions: null comparison for first argument, strA.");
                throw new BuildException(msg);
            }

            if (Object.ReferenceEquals(strB, null))
            {
                string msg = String.Format("StringUtil.StrCompareVersions: null comparison for second argument, strB.");
                throw new BuildException(msg);
            }

            if (strA != String.Empty)
            {
                if (strA.StartsWith("dev")) strA = "999" + strA.Substring(3);
                if (strA.StartsWith("work")) strA = "999" + strA.Substring(4);
            }
            if (strB != String.Empty)
            {
                if (strB.StartsWith("dev")) strB = "999" + strB.Substring(3);
                if (strB.StartsWith("work")) strB = "999" + strB.Substring(4);
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

        private static IList<string> split(string value, char[] delim)
        {
            List<string> array = new List<string>();
            if (!String.IsNullOrEmpty(value))
            {
                foreach (string str in value.Split(delim))
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


        private static char[] _defaultDelimArray = { 
            '\x0009', // tab
            '\x000a', // newline
            '\x000d', // carriage return
            '\x0020'  // space
        };
        
        private static Regex quotedSplit = new Regex("((?<quoted>[^\\s]+=\"([^\"]*)\")|(?<plain>[^\\s]+))", RegexOptions.Compiled | RegexOptions.CultureInvariant);

        private static char[] TRIM_CHARS = new char[] { '\n', '\r', '\t', ' '};

        // Utility function for trimming leading and trailing whitespace, and then splitting on any form of whitespace.
        // String.Split() just doesn't cut it.
        public static string[] TrimAndSplit(string inputString)
        {
            string[] splitResult = Regex.Split(inputString.Trim(TRIM_CHARS), @"\s+");
            return splitResult;
        }
    }
}
