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
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.Collections.Specialized;

using NAnt.Core;
using NAnt.Core.Attributes;

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
        /// <include file='Examples\String\StrLen.example' path='example'/>        
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
        /// <include file='Examples\String\StrEndsWith.example' path='example'/>        
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
        /// <include file='Examples\String\StrStartsWith.example' path='example'/>        
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
        /// <include file='Examples\String\StrCompare.example' path='example'/>        
        [Function()]
		public static string StrCompare(Project project, string strA, string strB)
		{
			return String.Compare(strA, strB).ToString();
		}

        /// <summary>
        /// Compares two specified version strings.  Version strings should be numbers 
        /// separated by any non digit separator. Separators are used in comparison as well.
        /// Dot separator will take a presedence over dash separator. 
        /// Numbers are always considered newer than letters, version strings that are starting with "dev" or "work" are considered a special case, dev and work takes presedence over numbers
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
        [Function()]
        public static string StrCompareVersions(Project project, string strA, string strB)
        {
            int iA = 0;
            int iB = 0;

            if(!String.IsNullOrEmpty(strA))
            {
                if (strA.StartsWith("dev")) strA = "999" + strA.Substring(3);
                if (strA.StartsWith("work")) strA = "999" + strA.Substring(4);
            }
            if (!String.IsNullOrEmpty(strB))
            {
                if (strB.StartsWith("dev")) strB = "999" + strB.Substring(3);
                if (strB.StartsWith("work")) strB = "999" + strB.Substring(4);
            }

            if (ValueAt(strA, iA) == '-' || ValueAt(strB, iB) == '-') return "0"; // treat options as same

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

            return diff.ToString();
        }

        /// <summary>
        /// Helper function for StrCompareVersions. Returns character value at position pos, 
        /// or '\0' if index is outside the string
        /// </summary>
        /// <param name="str"> string</param>
        /// <param name="pos">position in the string</param>        
        private static char ValueAt(string str, int pos)
        {
            char val = '\0';
            if (pos < str.Length)
                val = str[pos];
            return val;
        }

		/// <summary>
		/// Reports the index position of the last occurrence of a string within a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string to search.</param>
		/// <param name="strB">The string to seek.</param>
		/// <returns>The last index position of string strB in string strA if found; otherwise -1 if not found.</returns>
        /// <include file='Examples\String\StrLastIndexOf.example' path='example'/>        
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
        /// <include file='Examples\String\StrIndexOf.example' path='example'/>        
        [Function()]
		public static string StrIndexOf(Project project, string strA, string strB)
		{
			return strA.IndexOf(strB).ToString();
		}

        /// <summary>
        /// Reports if strA contains strB.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="strA">The string to search.</param>
        /// <param name="strB">The string to seek.</param>
        /// <returns>true if strB is found in  string strA; otherwise false if not found.</returns>
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
        /// <include file='Examples\String\StrRemove.example' path='example'/>        
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
        /// <include file='Examples\String\StrReplace.example' path='example'/>        
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
        /// <include file='Examples\String\StrInsert.example' path='example'/>        
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
        /// <include file='Examples\String\StrSubstring.example' path='example'/>        
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
        /// <include file='Examples\String\StrSubstringLength.example' path='example'/>        
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
        /// <include file='Examples\String\StrConcat.example' path='example'/>        
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
        /// <include file='Examples\String\StrPadLeft.example' path='example'/>        
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
        /// <include file='Examples\String\StrPadRight.example' path='example'/>        
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
        /// <include file='Examples\String\StrTrim.example' path='example'/>        
        [Function()]
		public static string StrTrim(Project project, string strA)
		{
			return strA.Trim();
		}

		/// <summary>
		/// Returns a copy of strA in uppercase.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <returns>A string in uppercase.</returns>
        /// <include file='Examples\String\StrToUpper.example' path='example'/>        
        [Function()]
		public static string StrToUpper(Project project, string strA)
		{
			return strA.ToUpper();
		}

		/// <summary>
		/// Returns a copy of strA in lowercase.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <returns>A string in lowercase.</returns>
        /// <include file='Examples\String\StrToLower.example' path='example'/>        
        [Function()]
		public static string StrToLower(Project project, string strA)
		{
			return strA.ToLower();
		}

		/// <summary>
		/// Echo a string.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="strA">The string.</param>
		/// <returns>The string.</returns>
        /// <include file='Examples\String\StrEcho.example' path='example'/>        
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
        /// <include file='Examples\String\StrIsEmpty.example' path='example'/>        
        [Function()]
        public static string StrIsEmpty(Project project, string strA)
        {
            return (strA.Length == 0).ToString();
        }

	}
}
