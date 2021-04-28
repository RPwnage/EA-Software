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

using System.Collections.Generic;
using System.Linq;

// TODO string extensions refactor
// -dvaliant 2017/01/25
/* there's lots of misc useful string extension code scattered throughout Frameowork right now
be nice to pull this into a single .cs that is built into every assembly*/ 

namespace NAnt.Perforce.Extensions
{
	namespace String
	{
		public static class Extensions
		{
			public static char[] WhiteSpaceDelimiters = new char[] { 
				'\x0009', // tab
				'\x000a', // newline
				'\x000d', // carriage return
				'\x0020'  // space
			};

			public static string Quoted(this string source, char quote = '\"')
			{
				if (!source.IsQuoted())
				{
					return quote + source + quote;
				}
				return source;
			}

			public static bool ContainsWhiteSpace(this string source, char quote = '\"')
			{
				return source.Any(c => WhiteSpaceDelimiters.Contains(c));
			}

			public static bool IsQuoted(this string source, char quote = '\"')
			{
				return source.Length >= 2 && source[0] == quote && source[source.Length - 1] == quote;
			}

            // TODO reconcile QuoteIfNecessary
            // -dvaliant 2017/01/25
            /* there's a separate, different version of this function in P4Caller that doesn't handle escaped quotes but does check
            for other escapes (mainly for linux) - should reconcile these */           
            public static string QuoteIfNecessary(this string source, char quote = '\"')
            {
                bool quoted = false;
                char lastChar = default(char);
                foreach (char c in source)
                {
                    if (c == quote && lastChar != '\\')
                    {
                        quoted = !quoted;

                    }
                    else if (WhiteSpaceDelimiters.Contains(c) && !quoted)
                    {
                        return source.Quoted();
                    }

                    lastChar = c;
                }
                return source;
            }

            public static string Unquoted(this string source)
			{
				if (source.IsQuoted())
				{
					return source.Substring(1, source.Length - 2);
				}
				return source;
			}

			public static IEnumerable<string> SplitRespectingQuotes(this string source, char quote = '\"', bool escapedSlashesAndQuotes = true)
			{
				bool inQuoted = false;
				bool lastWasEscapingSlash = false;
				int lastSplit = 0;
				foreach (int i in Enumerable.Range(0, source.Length))
				{
					if (source[i] == quote && (!escapedSlashesAndQuotes || !lastWasEscapingSlash))
					{
						lastWasEscapingSlash = false;
						inQuoted = !inQuoted;
					}
					else if (source[i] == '\\')
					{
						lastWasEscapingSlash = !lastWasEscapingSlash;
					}
					else if (!inQuoted && WhiteSpaceDelimiters.Contains(source[i]))
					{
						lastWasEscapingSlash = false;
						string output = source.Substring(lastSplit, i - lastSplit);
						if (i - lastSplit > 0)
						{
							yield return escapedSlashesAndQuotes ? output.Unquoted().Replace(@"\\", @"\").Replace(@"\"  + quote, quote.ToString()) : output.Unquoted();
						}
						lastSplit = i + 1;
					}
					else
					{
						lastWasEscapingSlash = false;
					}
				}

				if (lastSplit < source.Length)
				{
					string output = source.Substring(lastSplit);
					if (!System.String.IsNullOrWhiteSpace(output))
					{
						yield return escapedSlashesAndQuotes ? output.Unquoted().Replace(@"\\", @"\").Replace(@"\"  + quote, quote.ToString()) : output.Unquoted();
					}
				}

				yield break;
			}
		}
	}
}
