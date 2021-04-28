// Originally based on NAnt - A .NET build tool
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
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Util
{
	/* helper class for token replacments e.g.
			%package.builddir&\%package.name%\things => C:\build\MyPackage\dev\MyPackage\things
	most of the code outside of the .Replace function just exists so we can use collection
	initialization that accepts string, functions and lambdas e.g.

	new MacroMap()
	{
		{ "string", "a string" }
		{ "function", MyFunction }, // where MyFunction is a string() function
		{ "lambda", () => "a lazy string" },
	};

	the functions and lambdas are not called unless the token is encountered in case they are
	expensive to compute, however they are called everytime to allow for changing values
	(not a good API choice, but we need to deal with bad legacy APIs) - use a lazy if they value 
	can be safely cached

	*/
	public sealed class MacroMap : IEnumerable<KeyValuePair<string, MacroMap.LazyReplacement>>
	{
		public delegate string LazyReplacement(); // allow implicit conversion from lambda in .Add and .Update

		private readonly Dictionary<string, LazyReplacement> m_macros = new Dictionary<string, LazyReplacement>();
		private readonly MacroMap m_inherit = null;

		public MacroMap(MacroMap inherit = null)
		{
			m_inherit = inherit;
		}

		public void Add(string key, string value)
		{
			ValidateKey(key);
			m_macros.Add(key, () => value);
		}

		public void Add(string key, LazyReplacement value)
		{
			ValidateKey(key);
			m_macros.Add(key, value);
		}

		public void Update(string key, string value)
		{
			ValidateKey(key);
			m_macros[key] = () => value;
		}

		public void Update(string key, LazyReplacement value)
		{
			ValidateKey(key);
			m_macros[key] = value;
		}

		public bool ContainsKey(string key)
		{
			return m_macros.ContainsKey(key) || (m_inherit != null && m_inherit.ContainsKey(key));
		}

		public IEnumerator<KeyValuePair<string, LazyReplacement>> GetEnumerator()
		{
			return m_macros.GetEnumerator();
		}

		public static string Replace(string replaceString, string token, string replacement, bool allowUnresolved = false, string additionalErrorContext = null)
		{
			return Replace(replaceString, new MacroMap() { { token, replacement } }, allowUnresolved, additionalErrorContext);
		}

		public static string Replace(string replaceString, MacroMap map, bool allowUnresolved = false, string additionalErrorContext = null)
		{
			if (String.IsNullOrEmpty(replaceString))
			{
				return replaceString;
			}

			StringBuilder replacedBuilder = null;

			int offset = 0; // current offset into string
			int tokenEnd = 0; // end of last token (including %) or 0 if no tokens encountered
			int tokenStart = -1; // -1 if not in a token currently, otherwise start of token (not including %)

			CharEnumerator replaceCharEnumerator = replaceString.GetEnumerator();
			while (replaceCharEnumerator.MoveNext())
			{
				if (replaceCharEnumerator.Current == '%')
				{
					// if tokenStart is -1 we ar not current in a token
					if (tokenStart == -1)
					{
						// peek next character
						offset += 1;
						if (!replaceCharEnumerator.MoveNext())
						{
							throw new FormatException($"Unexpected end in token replacement string '{replaceString}' at offset {offset}{additionalErrorContext ?? String.Empty}. Expected a token escape (%%) or a token string (%token%).");
						}

						// if next character was % then this is just an escaped % (%% is escape sequence)
						if (replaceCharEnumerator.Current == '%')
						{
							// we need to replace escape sequence, so we need  the builder
							replacedBuilder = replacedBuilder ?? new StringBuilder();

							// if we have moved any distance since the start of string / last token ended add that to the builder
							if (offset > tokenEnd)
							{
								replacedBuilder.Append(replaceString, tokenEnd, offset - 1 - tokenEnd);
							}

							tokenEnd = offset + 1; // move tracking of last token to past escape
							replacedBuilder.Append('%');
						}
						else
						{
							tokenStart = offset;
						}
					}
					else
					{
						string token = replaceString.Substring(tokenStart, offset - tokenStart);

						// find replacement for token
						string replacement = null;
						MacroMap searchMap = map;
						while (searchMap != null)
						{
							if (searchMap.m_macros.TryGetValue(token, out LazyReplacement macroReplace))
							{
								try
								{
									replacement = macroReplace();
								}
								catch (Exception exception)
								{
									// wrap exception from evaluating replace value in more context
									throw new ArgumentException($"Error evaluating %{token}% at offset {tokenStart - 1} in replacement string '{replaceString}'{additionalErrorContext ?? String.Empty}: {exception.Message}", exception);
								}
								if (replacement == null)
								{
									throw new ArgumentException($"Error evaluating %{token}% at offset {tokenStart - 1} in replacement string '{replaceString}'{additionalErrorContext ?? String.Empty}: replacment value was null.");
								}
								break;
							}
							searchMap = searchMap.m_inherit;
						}

						// if we replacement was null then we found none, so error, otherwise add to builder
						if (replacement == null)
						{
							if (!allowUnresolved)
							{
								throw new ArgumentException($"Macro %{token}% at offset {tokenStart - 1} in replacement string '{replaceString}'{additionalErrorContext ?? String.Empty} is not valid in this context.");
							}
						}
						else
						{
							// create the builder on if we have something to replace
							replacedBuilder = replacedBuilder ?? new StringBuilder();

							// add everything 
							replacedBuilder.Append(replaceString, tokenEnd, tokenStart - 1 - tokenEnd);

							tokenEnd = offset + 1;
							replacedBuilder.Append(replacement);
						}

						tokenStart = -1;
					}
				}
				offset += 1;
			}

			// add characters trailing after tokens
			if (replacedBuilder != null)
			{
				int firstAfterLastToken = tokenEnd;
				if (offset > firstAfterLastToken)
				{
					replacedBuilder.Append(replaceString, tokenEnd, offset - tokenEnd);
				}
			}

			// check for unfinished token
			if (tokenStart != -1)
			{
				throw new FormatException($"Unexpected end in token replacement string '{replaceString}' at offset {offset}. Expected a token escape (%%) or a token string (%token%).");
			}

			return replacedBuilder?.ToString() ?? replaceString; // if we never created replace builder then string contained no tokens and we can return original reference
		}

		private void ValidateKey(string key)
		{
			if (key.Any(c => c == '%'))
			{
				throw new FormatException($"Macro replacement key '{key}' cannot contain '%'!");
			}
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return m_macros.GetEnumerator();
		}
	}
}