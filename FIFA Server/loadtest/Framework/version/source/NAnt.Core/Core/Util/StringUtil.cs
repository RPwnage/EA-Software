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
using System.Text;
using System.Text.RegularExpressions;
using System.Linq;

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

		public static bool GetBooleanValueOrDefault(string val, bool defaultVal)
		{
			if (String.IsNullOrEmpty(val))
			{
				return defaultVal;
			}
			val = val.Trim();
			if (val.Equals("true", StringComparison.OrdinalIgnoreCase))
			{
				return true;
			}
			else if (val.Equals("false", StringComparison.OrdinalIgnoreCase))
			{
				return false;
			}
			return defaultVal;
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
				throw new BuildException("StringUtil.ToArray: empty delimiter.");
			}

			string delimString;
			try
			{
				delimString = Regex.Unescape(delim);
			}
			catch (System.Exception e)
			{
				string msg = String.Format("StringUtil.ToArray: : cannot unescape delimiter '{0}'.", delim);
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

		public static string EnsureSeparator(string value, char sep)
		{
			if (String.IsNullOrEmpty(value))
			{
				return value;
			}

			StringBuilder sb = new StringBuilder();
			foreach (string item in ToArray(value))
			{
				if (sb.Length > 0)
				{
					sb.Append(sep);
				}
				sb.Append(item);

			}
			return sb.ToString();
		}

		public static string EnsureSeparator(string value, string sep)
		{
			if (String.IsNullOrEmpty(value))
			{
				return value;
			}

			StringBuilder sb = new StringBuilder();
			foreach (string item in ToArray(value))
			{
				if (sb.Length > 0)
				{
					sb.Append(sep);
				}
				sb.Append(item);

			}
			return sb.ToString();
		}

		public static string EnsureSeparatorAndQuote(string value, char sep)
		{
			if (String.IsNullOrEmpty(value))
			{
				return String.Empty;
			}

			StringBuilder sb = new StringBuilder();
			foreach (string item in ToArray(value))
			{
				if (sb.Length > 0)
				{
					sb.Append(sep);
				}
				sb.Append('"' + item.Trim('"') + '"');
			}
			return sb.ToString();
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

		public static IEnumerable<string> FindClosestMatchesFromList(IEnumerable<string> list, string match)
		{
			var keyDistancePairs = list.Select(key => new { Key = key, Dist = NAnt.Core.Util.StringUtil.MinimumEditDistance(key, match) });
			var closePairs = keyDistancePairs.Where(p => p.Dist < 5); // 5 is max dist we consider worth reporting
			var closePairsByDitance = closePairs.OrderBy(p => p.Dist);
			var top5OrFewer = closePairsByDitance.Take(5);
			return top5OrFewer.Select(p => p.Key).ToArray();
		}

		/// <summary>
		/// Calculates the Minimum Edit Distance between two strings in order to quantify the similarity of two strings.
		/// Minimum Edit Distance is the number of insertions, deletions and substitutions required to convert one string into another.
		/// Levenshtein Distance is Minimum Edit Distance when the substitution cost is set to 2.
		/// </summary>
		/// <param name="source">The source string</param>
		/// <param name="target">The target string</param>
		/// <param name="substitutionCost">
		/// The cost applied when a character is substituted.
		/// When the substitution cost is set to 2 it is called Levenshtein Distance.
		/// </param>
		public static int MinimumEditDistance(string source, string target, int substitutionCost=1)
		{
			// If either string is empty the distance is the length of the other string
			if (source.IsNullOrEmpty())
			{
				return target.IsNullOrEmpty() ? 0 : target.Length;
			}
			else if (target.IsNullOrEmpty())
			{
				return source.IsNullOrEmpty() ? 0 : source.Length;
			}

			int width = source.Length + 1;
			int height = target.Length + 1;

			int[,] distance = new int[width, height];

			// Initialize the first row and column to be values increasing by one
			for (int i = 1; i < width; ++i)
			{
				distance[i, 0] = i;
			}
			for (int i = 1; i < height; ++i)
			{
				distance[0, i] = i;
			}

			for (int i = 1; i < width; i++)
			{
				for (int j = 1; j < height; j++)
				{
					if (source[i - 1] == target[j - 1])
					{
						distance[i, j] = distance[i - 1, j - 1];
					}
					else
					{
						var delete = distance[i - 1, j] + 1;
						var insert = distance[i, j - 1] + 1;
						var substitution = distance[i - 1, j - 1] + substitutionCost;
						distance[i, j] = Math.Min(delete, Math.Min(insert, substitution));
					}
				}
			}

			return distance[width - 1, height - 1];
		}
	}
}
