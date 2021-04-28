// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

namespace NAnt.Core
{
	public static class StringExtensions
	{
        public static char[] WhiteSpaceDelimiters = new char[] {
            '\x0009', // tab
		    '\x000a', // newline
		    '\x000d', // carriage return
		    '\x0020'  // space
		};

        public static bool IsOptionBoolean(this string val)
		{
			if (!String.IsNullOrEmpty(val))
			{
				var tval = val.TrimWhiteSpace();
				if (tval.Equals("on", StringComparison.OrdinalIgnoreCase) || tval.Equals("true", StringComparison.OrdinalIgnoreCase) ||
					tval.Equals("off", StringComparison.OrdinalIgnoreCase) || tval.Equals("false", StringComparison.OrdinalIgnoreCase))
				{
					return true;
				}
			}
			return false;
		}

		public static bool OptionToBoolean(this string val)
		{
			if (!String.IsNullOrEmpty(val))
			{
				var tval = val.TrimWhiteSpace();
				if (tval.Equals("on", StringComparison.OrdinalIgnoreCase) || tval.Equals("true", StringComparison.OrdinalIgnoreCase))
				{
					return true;
				}
			}
			return false;
		}

		public static bool ToBoolean(this string val)
		{
			if (!String.IsNullOrEmpty(val))
			{
				if (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
				{
					return true;
				}
			}
			return false;
		}

		public static bool ToBooleanOrDefault(this string val, bool def)
		{
			if (String.IsNullOrEmpty(val))
			{
				return def;
			}
			return val.TrimWhiteSpace().Equals("true", StringComparison.OrdinalIgnoreCase);
		}

        // TODO - unify quote implementations
        // -dvaliant 2016/01/11
        /* be nice to not have two quote functions here (Quote and QuoteIncludingEmpty) but there are parts of Framework code
        currrently that rely on quoting empty string to return empty string to function correctly */
        public static string Quote(this string val, char quoteType = '\"')
		{
			if (!String.IsNullOrEmpty(val) && val.First() != quoteType)
			{
				val = quoteType + val + quoteType;
			}
			return val;
		}

        public static string QuoteIncludingEmpty(this string val, char quoteType = '\"')
        {
            if (val != null && val.FirstOrDefault() != quoteType)
            {
                val = quoteType + val + quoteType;
            }
            return val;
        }

		public static string TrimExtension(this string val)
		{
			if (!String.IsNullOrEmpty(val))
			{
				string extension = Path.GetExtension(val);
				if (!String.IsNullOrEmpty(extension))
				{
					return val.Substring(0, val.Length - extension.Length);
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
				throw new BuildException("StringExtensions.ToArray: empty delimiter.");
			}

			string delimString;
			try
			{
				delimString = Regex.Unescape(delim);
			}
			catch (Exception e)
			{
				string msg = String.Format("StringExtensions.ToArray: : cannot unescape delimiter '{0}'.", delim);
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

        // TODO clean up string extensions
        // -dvaliant 2018/04/18
        /* currently there's a lot of duplicate / almost duplicate string extensions code in NAnt.Perforce,
        should clean this all up */
        public static string UnescapeForCommandLine(this string source)
        {
            // https://msdn.microsoft.com/en-us/library/system.environment.getcommandlineargs.aspx
            bool needsQuote = false;
            string stringOutput = System.String.Empty;
            int slashCount = 0;
            for (int i = 0; i < source.Length; ++i)
            {
                if (source[i] == '\\')
                {
                    slashCount += 1;
                }
                else if (source[i] == '"')
                {
                    stringOutput += System.String.Concat(Enumerable.Repeat('\\', slashCount * 2));
                    slashCount = 0;
                    stringOutput += "\\\"";
                }
                else
                {
                    if (WhiteSpaceDelimiters.Contains(source[i]))
                    {
                        needsQuote = true;
                    }
                    stringOutput += System.String.Concat(Enumerable.Repeat('\\', slashCount));
                    slashCount = 0;
                    stringOutput += source[i];
                }
            }
            if (needsQuote)
            {
                if (slashCount % 2 == 1)
                {
                    stringOutput += System.String.Concat(Enumerable.Repeat('\\', slashCount * 2));
                }
                return '"' + stringOutput + '"';
            }
            else
            {
                stringOutput += System.String.Concat(Enumerable.Repeat('\\', slashCount));
                return stringOutput;
            }
        }

        public static bool ContainsArrayItem(this string array, string value)
		{
			foreach (string item in array.ToArray())
			{
				if (item == value)
				{
					return true;
				}
			}
			return false;
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
			if (input != null)
			{
				return input.Trim('"');
			}
			return String.Empty;
		}

		public static string TrimWhiteSpace(this string input)
		{
			if (input != null)
			{
				return input.Trim(_defaultDelimArray);
			}
			return String.Empty;
		}

		public static string RemoveWhiteSpace(this string input)
		{
			if (input != null)
			{
				return new String(input.Where(c => !_defaultDelimArray.Contains(c)).ToArray());
			}
			return String.Empty;
		}

		public static string TrimLeftSlash(this string input)
		{
			if (input != null)
			{
				return input.TrimStart('\\', '/');
			}
			return String.Empty;
		}

		public static string TrimRightSlash(this string input)
		{
			if (input != null)
			{
				return input.TrimEnd('\\', '/');
			}
			return String.Empty;
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
				throw new BuildException("StringExtensions.StrCompareVersions: null comparison for first argument, strA.");
			}

			if (Object.ReferenceEquals(strB, null))
			{
				throw new BuildException("StringExtensions.StrCompareVersions: null comparison for second argument, strB.");
			}

			if (strA != String.Empty)
			{
				if (strA.StartsWith("work", StringComparison.Ordinal)) strA = "999999" + strA.Substring(4);
				if (strA.StartsWith("dev", StringComparison.Ordinal)) strA = "999998" + strA.Substring(3);
				if (strA.StartsWith("stable", StringComparison.Ordinal)) strA = "999997" + strA.Substring(6);
			}
			if (strB != String.Empty)
			{
				if (strB.StartsWith("work", StringComparison.Ordinal)) strB = "999999" + strB.Substring(4);
				if (strB.StartsWith("dev", StringComparison.Ordinal)) strB = "999998" + strB.Substring(3);
				if (strB.StartsWith("stable", StringComparison.Ordinal)) strB = "999997" + strB.Substring(6);
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

						n0 = (int)Int64.Parse(digits.ToString());
					}

					int n1 = -1;
					if (Char.IsDigit(ValueAt(strB, iB)))
					{
						System.Text.StringBuilder digits = new System.Text.StringBuilder();

						while (iB < strB.Length && Char.IsDigit(ValueAt(strB, iB)))
						{
							digits.Append(strB[iB++]);
						}

						n1 = (int)Int64.Parse(digits.ToString());
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
					throw new BuildException(String.Format("'{0}' property contains invalid template parameter(s) : {1}", parametername, result));
				}
			}
			return result;
		}

		public static bool ContainsMSBuildMacro(this string str)
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

	public static class PathExtensions
	{
		public static string PathToUnix(this string path)
		{
			if (path != null)
			{
				return path.Replace("\\", "/");
			}
			return string.Empty;
		}

		public static string PathToWindows(this string path)
		{
			if (path != null)
			{
				return path.Replace("/", "\\");
			}
			return string.Empty;
		}
	}
}
