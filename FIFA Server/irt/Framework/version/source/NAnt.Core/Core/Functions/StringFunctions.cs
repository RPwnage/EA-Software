// NAnt - A .NET build tool
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
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using System.Text;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of string manipulation routines. 
	/// </summary>
	[FunctionClass("String Functions")]
	public class StringFunctions : FunctionClassBase
	{
		/// <summary>
		/// Gets the number of characters in a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to count.</param>
		/// <returns>The number of characters in the specified string.</returns>
		/// <include file='Examples/String/StrLen.example' path='example'/>        
		[Function()]
		public static string StrLen(Project project, string strA)
		{
			return strA.Length.ToString();
		}

		/// <summary>
		/// Determines whether the end of a string matches a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to search.</param>
		/// <param name="strB">The string to match.</param>
		/// <returns>True if the specified string strA ends with the specified string strB.</returns>
		/// <include file='Examples/String/StrEndsWith.example' path='example'/>        
		[Function()]
		public static string StrEndsWith(Project project, string strA, string strB)
		{
			return strA.EndsWith(strB).ToString();
		}

		/// <summary>
		/// Determines whether the start of a string matches a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to search.</param>
		/// <param name="strB">The string to match.</param>
		/// <returns>True if the specified string, strA, starts with the specified string, strB.</returns>
		/// <include file='Examples/String/StrStartsWith.example' path='example'/>        
		[Function()]
		public static string StrStartsWith(Project project, string strA, string strB)
		{
			return strA.StartsWith(strB).ToString();
		}

		/// <summary>
		/// Compares two specified strings.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The first String.</param>
		/// <param name="strB">The second String.</param>
		/// <returns>
		///		Less than zero - strA is less than strB. &lt;br/&gt;
		///		Zero - strA is equal to strB. &lt;br/&gt;
		///		Greater than zero - strA is greater than strB. &lt;br/&gt;
		/// </returns>
		/// <include file='Examples/String/StrCompare.example' path='example'/>        
		[Function()]
		public static string StrCompare(Project project, string strA, string strB)
		{
			return String.Compare(strA, strB).ToString();
		}

		/// <summary>
		/// Compares two specified version strings.  Version strings should be numbers 
		/// separated by any non digit separator. Separators are used in comparison as well.
		/// Dot separator will take a precedence over dash separator. 
		/// Numbers are always considered newer than letters, version strings that are starting with "dev" or "work" are considered a special case, dev and work takes precedence over numbers
		/// 
		/// For example, versions will be sorted this way in descending order:
		/// test-09
		/// test-8-1
		/// test-8.1-alpha
		/// test-8.1
		/// test-8       
		/// Work and dev will be newer than numeric versions, but only if package name is not included:
		/// dev
		/// 1.01.00
		/// This function assumes that separators used in both version strings are the same.
		/// 
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The first version String.</param>
		/// <param name="strB">The second version String.</param>
		/// <returns>
		///		Less than zero - strA is less than strB. &lt;br/&gt;
		///		Zero - strA is equal to strB. &lt;br/&gt;
		///		Greater than zero - strA is greater than strB. &lt;br/&gt;
		///     Comparison algorithm takes into account version formatting, rather than doing simple lexicographical comparison.
		/// </returns>
		/// <include file='Examples/String/StrCompareVersions.example' path='example'/>
		[Function()]
		public static string StrCompareVersions(Project project, string strA, string strB)
		{
			int diff = strA.StrCompareVersions(strB);

			return diff.ToString();
		}

		/// <summary>
		/// Compares two specified version strings in the same way as StrCompareVersions.
		/// However returns a boolean value of true if strA is less than strB.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The first version String.</param>
		/// <param name="strB">The second version String.</param>
		/// <returns>true if strA is less than strB, false otherwise</returns>
		/// <include file='Examples/String/StrVersionLess.example' path='example'/>
		[Function()]
		public static string StrVersionLess(Project project, string strA, string strB)
		{
			int diff = strA.StrCompareVersions(strB);

			return (diff < 0).ToString();
		}

		/// <summary>
		/// Compares two specified version strings in the same way as StrCompareVersions.
		/// However returns a boolean value of true if strA is less than or equal to strB.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The first version String.</param>
		/// <param name="strB">The second version String.</param>
		/// <returns>true if strA is less than or equal to strB, false otherwise</returns>
		/// <include file='Examples/String/StrVersionLessOrEqual.example' path='example'/>
		[Function()]
		public static string StrVersionLessOrEqual(Project project, string strA, string strB)
		{
			int diff = strA.StrCompareVersions(strB);

			return (diff <= 0).ToString();
		}

		/// <summary>
		/// Compares two specified version strings in the same way as StrCompareVersions.
		/// However returns a boolean value of true if strA is greater than strB.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The first version String.</param>
		/// <param name="strB">The second version String.</param>
		/// <returns>true if strA is greater than strB, false otherwise</returns>
		/// <include file='Examples/String/StrVersionGreater.example' path='example'/>
		[Function()]
		public static string StrVersionGreater(Project project, string strA, string strB)
		{
			int diff = strA.StrCompareVersions(strB);

			return (diff > 0).ToString();
		}

		/// <summary>
		/// Compares two specified version strings in the same way as StrCompareVersions.
		/// However returns a boolean value of true if strA is greater than or equal to strB.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The first version String.</param>
		/// <param name="strB">The second version String.</param>
		/// <returns>true if strA is greater than or equal to strB, false otherwise</returns>
		/// <include file='Examples/String/StrVersionGreaterOrEqual.example' path='example'/>
		[Function()]
		public static string StrVersionGreaterOrEqual(Project project, string strA, string strB)
		{
			int diff = strA.StrCompareVersions(strB);

			return (diff >= 0).ToString();
		}

		/// <summary>
		/// Reports the index position of the last occurrence of a string within a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to search.</param>
		/// <param name="strB">The string to seek.</param>
		/// <returns>The last index position of string strB in string strA if found; otherwise -1 if not found.</returns>
		/// <include file='Examples/String/StrLastIndexOf.example' path='example'/>        
		[Function()]
		public static string StrLastIndexOf(Project project, string strA, string strB)
		{
			return strA.LastIndexOf(strB).ToString();
		}

		/// <summary>
		/// Reports the index of the first occurrence of a string in a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to search.</param>
		/// <param name="strB">The string to seek.</param>
		/// <returns>The index position of string strB in string strA if found; otherwise -1 if not found.</returns>
		/// <include file='Examples/String/StrIndexOf.example' path='example'/>        
		[Function()]
		public static string StrIndexOf(Project project, string strA, string strB)
		{
			return strA.IndexOf(strB).ToString();
		}

		/// <summary>
		/// Reports if the first string contains the second string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to search.</param>
		/// <param name="strB">The string to seek.</param>
		/// <returns>true if strB is found in  string strA; otherwise false if not found.</returns>
		/// <include file='Examples/String/StrContains.example' path='example'/>
		[Function()]
		public static string StrContains(Project project, string strA, string strB)
		{
			return strA.Contains(strB) ? "true" : "false";
		}

		/// <summary>
		/// Deletes a specified number of characters from the specified string beginning at a specified position.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <param name="startIndex">The position in the string to begin deleting characters.</param>
		/// <param name="count">The number of characters to delete.</param>
		/// <returns>A string having count characters removed from strA starting at startIndex.</returns>
		/// <include file='Examples/String/StrRemove.example' path='example'/>        
		[Function()]
		public static string StrRemove(Project project, string strA, int startIndex, int count)
		{
			return strA.Remove(startIndex, count);
		}

		/// <summary>
		/// Replaces all occurrences of a string in a string, with another string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <param name="oldValue">The string to be replaced.</param>
		/// <param name="newValue">The string to replace all occurrences of oldValue.</param>
		/// <returns>A string having every occurrence of oldValue in strA replaced with newValue.</returns>
		/// <include file='Examples/String/StrReplace.example' path='example'/>        
		[Function()]
		public static string StrReplace(Project project, string strA, string oldValue, string newValue)
		{
			return strA.Replace(oldValue, newValue);
		}

		/// <summary>
		/// Inserts a string at a specified index position in a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to insert into.</param>
		/// <param name="startIndex">The index position of the insertion.</param>
		/// <param name="value">The string to insert.</param>
		/// <returns>A string equivalent to strA but with value inserted at position startIndex.</returns>
		/// <include file='Examples/String/StrInsert.example' path='example'/>        
		[Function()]
		public static string StrInsert(Project project, string strA, int startIndex, string value)
		{
			return strA.Insert(startIndex, value);
		}

		/// <summary>
		/// Retrieves a substring from a string. The substring starts at a specified character position.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <param name="startIndex">The starting character position of a substring in this instance.</param>
		/// <returns>A substring of strA starting at startIndex.</returns>
		/// <include file='Examples/String/StrSubstring.example' path='example'/>        
		[Function()]
		public static string StrSubstring(Project project, string strA, int startIndex)
		{
			return strA.Substring(startIndex);
		}

		/// <summary>
		/// Retrieves a substring from a string.
		/// The substring starts at a specified character position and has a specified length.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <param name="startIndex">The index of the start of the substring.</param>
		/// <param name="length">The number of characters in the substring.</param>
		/// <returns>A substring of strA starting at startIndex and with a specified length.</returns>
		/// <include file='Examples/String/StrSubstringLength.example' path='example'/>        
		[Function()]
		public static string StrSubstring(Project project, string strA, int startIndex, int length)
		{
			return strA.Substring(startIndex, length);
		}

		/// <summary>
		/// Concatenates two strings.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The first string.</param>
		/// <param name="strB">The second string.</param>
		/// <returns>The concatenation of strA and strB.</returns>
		/// <include file='Examples/String/StrConcat.example' path='example'/>        
		[Function()]
		public static string StrConcat(Project project, string strA, string strB)
		{
			return String.Concat(strA, strB);
		}

		/// <summary>
		/// Right-aligns the characters a string, padding with spaces on the left for a total length.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to pad.</param>
		/// <param name="totalWidth">
		/// The number of characters in the resulting string, equal to the number of original characters plus 
		/// any additional padding characters.
		/// </param>
		/// <returns>
		/// A string that is equivalent to strA, but right-aligned and padded on the left with as many 
		/// spaces as needed to create a length of totalWidth. 
		/// If totalWidth is less than the length of strA, a string that is identical to strA is returned.
		/// </returns>
		/// <include file='Examples/String/StrPadLeft.example' path='example'/>        
		[Function()]
		public static string StrPadLeft(Project project, string strA, int totalWidth)
		{
			return strA.PadLeft(totalWidth);
		}

		/// <summary>
		/// Left-aligns the characters in a string, padding with spaces on the left for a total length.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to pad.</param>
		/// <param name="totalWidth">
		/// The number of characters in the resulting string, equal to the number of original characters plus 
		/// any additional padding characters.
		/// </param>
		/// <returns>
		/// A string that is equivalent to strA, but left-aligned and padded on the right with as many 
		/// spaces as needed to create a length of totalWidth. 
		/// If totalWidth is less than the length of strA, a string that is identical to strA is returned.
		/// </returns>
		/// <include file='Examples/String/StrPadRight.example' path='example'/>        
		[Function()]
		public static string StrPadRight(Project project, string strA, int totalWidth)
		{
			return strA.PadRight(totalWidth);
		}

		/// <summary>
		/// Removes all occurrences of white space characters from the beginning and end of a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to trim.</param>
		/// <returns>A string equivalent to strA after white space characters are removed.</returns>
		/// <include file='Examples/String/StrTrim.example' path='example'/>        
		[Function()]
		public static string StrTrim(Project project, string strA)
		{
			return strA.Trim();
		}

		/// <summary>
		/// Removes all occurrences of white space characters (including tabs and new lines)from the beginning and end of a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to trim.</param>
		/// <returns>A string equivalent to strA after white space characters are removed.</returns>
		/// <include file='Examples/String/StrTrim.example' path='example'/>        
		[Function()]
		public static string StrTrimWhiteSpace(Project project, string strA)
		{
			return strA.TrimWhiteSpace();
		}


		/// <summary>
		/// Returns a copy of the given string in uppercase.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <returns>A string in uppercase.</returns>
		/// <include file='Examples/String/StrToUpper.example' path='example'/>        
		[Function()]
		public static string StrToUpper(Project project, string strA)
		{
			return strA.ToUpper();
		}

		/// <summary>
		/// Returns a copy of the given string in lowercase.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <returns>A string in lowercase.</returns>
		/// <include file='Examples/String/StrToLower.example' path='example'/>        
		[Function()]
		public static string StrToLower(Project project, string strA)
		{
			return strA.ToLower();
		}

		/// <summary>
		/// Takes a space separated list of words and joins them with only the first letter of each capitalized
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">A space separate list of words</param>
		/// <returns>A one word string composed of all of the input words joined and with only the starting letter of each capitalized</returns>
		/// <include file='Examples/String/StrPascalCase.example' path='example'/>
		[Function()]
		public static string StrPascalCase(Project project, string strA)
		{
			bool startOfNewWord = true;
			StringBuilder new_string = new StringBuilder(strA.Length);
			for (int i = 0; i < strA.Length; ++i)
			{
				if (strA[i] == ' ')
				{
					startOfNewWord = true;
				}
				else if (startOfNewWord)
				{
					new_string.Append(char.ToUpperInvariant(strA[i]));
					startOfNewWord = false;
				}
				else
				{
					new_string.Append(char.ToLowerInvariant(strA[i]));
				}
			}
			return new_string.ToString();
		}

		/// <summary>
		/// Echo a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <returns>The string.</returns>
		/// <include file='Examples/String/StrEcho.example' path='example'/>        
		[Function()]
		public static string StrEcho(Project project, string strA)
		{
			return strA;
		}

		/// <summary>
		/// Returns true if the specified string is the Empty string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <returns>True of False.</returns>
		/// <include file='Examples/String/StrIsEmpty.example' path='example'/>        
		[Function()]
		public static string StrIsEmpty(Project project, string strA)
		{
			return (strA.Length == 0).ToString();
		}

		/// <summary>
		/// Returns "strTrue" or "strFalse" depending on value of conditional
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strCondition">The string to determine whether True or False.</param>
		/// <param name="strTrue">String to return when conditional evaluates to True</param>
		/// <param name="strFalse">String to return when conditional evaluates to False</param>
		/// <returns>Either strTrue or strFalse</returns>
		/// <include file='Examples/String/StrSelectIf.example' path='example'/>
		[Function()]
		public static string StrSelectIf(Project project, string strCondition, string strTrue, string strFalse)
		{
			if (Expression.Evaluate(project.ExpandProperties(strCondition)))
				return strTrue;
			else
				return strFalse;
		}

		/// <summary>
		/// Eliminates duplicates from the list of space separated items.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="str">The string to process.</param>
		/// <returns>A Space separated String containing distinct items.</returns>
		/// <include file='Examples/String/DistinctItems.example' path='example'/>
		[Function()]
		public static string DistinctItems(Project project, string str)
		{
			return str.ToArray().OrderedDistinct().ToString(" ");
		}

		/// <summary>
		/// Eliminates duplicates from the list of items.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="str">The string to process.</param>
		/// <param name="sep">A custom separator string</param>
		/// <returns>String containing distinct items.</returns>
		/// <include file='Examples/String/DistinctItemsCustomSeparator.example' path='example'/>
		[Function()]
		public static string DistinctItemsCustomSeparator(Project project, string str, string sep)
		{
			return str.ToArray().OrderedDistinct().ToString(sep);
		}
	}
}
