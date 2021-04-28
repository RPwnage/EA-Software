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
	/// Collection of character manipulation routines.
	/// </summary>
	[FunctionClass("Character Functions")]
	public class CharacterFunctions : FunctionClassBase
	{
		/// <summary>
		/// Indicates whether the character at the specified position in a string is a decimal digit.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="s">The string.</param>
		/// <param name="index">The character position in s.</param>
		/// <returns>Returns true if the character at position index in s is a decimal digit; otherwise, false.</returns>
        /// <include file='Examples\Char\CharIsDigit.example' path='example'/>        
        [Function()]
		public static string CharIsDigit(Project project, string s, int index)
		{
			return Char.IsDigit(s, index).ToString();
		}

		/// <summary>
		/// Indicates whether the character at the specified position in a string is categorized 
		/// as a decimal digit or hexadecimal number.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="s">The string.</param>
		/// <param name="index">The character position in s.</param>
		/// <returns>Returns true if the character at position index in s is a decimal digit or hexadecimal number; otherwise, false.</returns>
        /// <include file='Examples\Char\CharIsNumber.example' path='example'/>        
        [Function()]
		public static string CharIsNumber(Project project, string s, int index)
		{
			return Char.IsNumber(s, index).ToString();
		}

		/// <summary>
		/// Indicates whether the character at the specified position in a string is categorized 
		/// as white space.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="s">The string.</param>
		/// <param name="index">The character position in s.</param>
		/// <returns>Returns true if the character at position index in s is white space; otherwise, false.</returns>
        /// <include file='Examples\Char\CharIsWhiteSpace.example' path='example'/>        
        [Function()]
		public static string CharIsWhiteSpace(Project project, string s, int index)
		{
			return Char.IsWhiteSpace(s, index).ToString();
		}

		/// <summary>
		/// Indicates whether a character is categorized as an alphabetic letter.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="s">The string.</param>
		/// <param name="index">The character position in s.</param>
		/// <returns>Returns true if the character at position index in s is an alphabetic character; otherwise, false.</returns>
        /// <include file='Examples\Char\CharIsLetter.example' path='example'/>        
        [Function()]
		public static string CharIsLetter(Project project, string s, int index)
		{
			return Char.IsLetter(s, index).ToString();
		}

		/// <summary>
		/// Indicates whether the character at the specified position in a string is categorized as an 
		/// alphabetic character or a decimal digit.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="s">The string.</param>
		/// <param name="index">The character position in s.</param>
		/// <returns>Returns true if the character at position index in s is an alphabetic character or a decimal digit; otherwise, false.</returns>
        /// <include file='Examples\Char\CharIsLetterOrDigit.example' path='example'/>        
        [Function()]
		public static string CharIsLetterOrDigit(Project project, string s, int index)
		{
			return Char.IsLetterOrDigit(s, index).ToString();
		}

		/// <summary>
		/// Converts the value of a character to its lowercase equivalent.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="c">The character.</param>
		/// <returns>The lowercase equivalent of c.</returns>
        /// <include file='Examples\Char\CharToLower.example' path='example'/>        
        [Function()]
		public static string CharToLower(Project project, char c)
		{
			return Char.ToLower(c).ToString();
		}

		/// <summary>
		/// Converts the value of a character to its uppercase equivalent.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="c">The character.</param>
		/// <returns>The uppercase equivalent of c.</returns>
        /// <include file='Examples\Char\CharToUpper.example' path='example'/>        
        [Function()]
		public static string CharToUpper(Project project, char c)
		{
			return Char.ToUpper(c).ToString();
		}

		/// <summary>
		/// Indicates whether the character at the specified position in a string is categorized as a lowercase letter.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="s">The string.</param>
		/// <param name="index">The character position in s.</param>
		/// <returns>Returns true if the character at position index in s is a lowercase letter; otherwise, false.</returns>
        /// <include file='Examples\Char\CharIsLower.example' path='example'/>        
        [Function()]
		public static string CharIsLower(Project project, string s, int index)
		{
			return Char.IsLower(s, index).ToString();
		}

		/// <summary>
		/// Indicates whether the character at the specified position in a string is categorized as an uppercase letter.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="s">The string.</param>
		/// <param name="index">The character position in s.</param>
		/// <returns>Returns true if the character at position index in s is an uppercase letter; otherwise, false.</returns>
        /// <include file='Examples\Char\CharIsUpper.example' path='example'/>        
        [Function()]
		public static string CharIsUpper(Project project, string s, int index)
		{
			return Char.IsUpper(s, index).ToString();
		}
	}
}