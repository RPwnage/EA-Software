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
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.IO;

namespace NAnt.Perforce
{
	public static class P4Utils
	{
		// old perforce versions use keyword for type / flag combos which need to be translated
		private class KeywordFileType
		{
			internal P4File.FileBaseType Type;
			internal P4File.FileTypeFlags Flags;
		}

		// map of keyword names to file types and flags
		private static readonly Dictionary<string, KeywordFileType> KeywordFileTypeMap = new Dictionary<string, KeywordFileType>()
		{
			{ "xtext", new KeywordFileType() { Type = P4File.FileBaseType.text, Flags = P4File.FileTypeFlags.Executable } },
			{ "ktext",  new KeywordFileType() { Type = P4File.FileBaseType.text, Flags = P4File.FileTypeFlags.RCSKeywordExpansion } },
			{ "kxtext", new KeywordFileType() { Type = P4File.FileBaseType.text, Flags = P4File.FileTypeFlags.Executable | P4File.FileTypeFlags.RCSKeywordExpansion } },
			{ "xbinary", new KeywordFileType() { Type = P4File.FileBaseType.binary, Flags = P4File.FileTypeFlags.Executable } },
			{ "ctext", new KeywordFileType() { Type = P4File.FileBaseType.text, Flags = P4File.FileTypeFlags.Compressed } },
			{ "cxtext", new KeywordFileType() { Type = P4File.FileBaseType.text, Flags = P4File.FileTypeFlags.Executable | P4File.FileTypeFlags.Compressed } },
			{ "uresource", new KeywordFileType() { Type = P4File.FileBaseType.resource, Flags = P4File.FileTypeFlags.StoreFullFilePerRevision } },
			{ "ltext", new KeywordFileType() { Type = P4File.FileBaseType.text, Flags = P4File.FileTypeFlags.StoreFullFilePerRevision } },
			{ "xltext", new KeywordFileType() { Type = P4File.FileBaseType.text, Flags = P4File.FileTypeFlags.Executable | P4File.FileTypeFlags.StoreFullFilePerRevision } },
			{ "ubinary", new KeywordFileType() { Type = P4File.FileBaseType.binary, Flags = P4File.FileTypeFlags.StoreFullFilePerRevision } },
			{ "uxbinary", new KeywordFileType() { Type = P4File.FileBaseType.binary, Flags = P4File.FileTypeFlags.Executable | P4File.FileTypeFlags.StoreFullFilePerRevision } },
			{ "tempobj", new KeywordFileType() { Type = P4File.FileBaseType.binary, Flags = P4File.FileTypeFlags.StoreFullFilePerRevision | P4File.FileTypeFlags.StoreOnlyHeadRevisions | P4File.FileTypeFlags.AlwaysClientWriteable } },
			{ "ctempobj", new KeywordFileType() { Type = P4File.FileBaseType.binary, Flags = P4File.FileTypeFlags.StoreOnlyHeadRevisions | P4File.FileTypeFlags.AlwaysClientWriteable } },
			{ "xtempobj", new KeywordFileType() { Type = P4File.FileBaseType.binary, Flags = P4File.FileTypeFlags.Executable | P4File.FileTypeFlags.StoreFullFilePerRevision | P4File.FileTypeFlags.StoreOnlyHeadRevisions | P4File.FileTypeFlags.AlwaysClientWriteable } },
			{ "xunicode", new KeywordFileType() { Type = P4File.FileBaseType.unicode, Flags = P4File.FileTypeFlags.Executable } }
		};

		// map of type flags to pattern
		private static readonly Dictionary<P4File.FileTypeFlags, string> FlagPatternMap = new Dictionary<P4File.FileTypeFlags, string>()
		{
			{ P4File.FileTypeFlags.AlwaysClientWriteable, "w" },
			{ P4File.FileTypeFlags.Executable, "x" },
			{ P4File.FileTypeFlags.RCSKeywordExpansion, "k" },
			{ P4File.FileTypeFlags.OldKeywordExpansion, "ko" },
			{ P4File.FileTypeFlags.ExclusiveOpen, "l" },
			{ P4File.FileTypeFlags.Compressed, "C" },
			{ P4File.FileTypeFlags.RCSDeltas, "D" },
			{ P4File.FileTypeFlags.StoreFullFilePerRevision, "F" },
			{ P4File.FileTypeFlags.StoreOnlyHeadRevisions, "S" },
			{ P4File.FileTypeFlags.PreserverOriginalModtime, "m" },
			{ P4File.FileTypeFlags.ArchiveTriggerReguired, "X" }
		};

		// see if a perforce location is a sub folder or file of another location
		public static bool SubLocationInLocation(string subLocation, string location, bool caseSensivite = false) // TODO - p4 allows wild catch matching, maybe we should too?
		{
			subLocation = caseSensivite ? subLocation.Trim() : subLocation.Trim().ToLowerInvariant();
			location = caseSensivite ? location.Trim() : location.Trim().ToLowerInvariant();
			if (location.EndsWith("..."))
			{
				location = location.Substring(0, location.Length - 3); // trim "..."
				if (subLocation.StartsWith(location))
				{
					return true;
				}
			}
			return false;
		}

		// converted unrooted location path to rooted one
		public static string RootedPath(string clientName, string path)
		{
			while (!path.StartsWith("//"))
			{
				path = "/" + path;
			}
			return path.Insert(2, clientName + "/");
		}

		public static string LocalPath(P4ClientStub client, string rootedPath)
		{
			if (rootedPath.StartsWith("//"))
			{
				rootedPath = rootedPath.Substring(2);
			}
			if (rootedPath.StartsWith(client.Name + '/'))
			{
				rootedPath = rootedPath.Substring((client.Name + '/').Length);
			}

			return Path.Combine(client.Root, rootedPath);
		}

		public static string Combine(params string[] paths)
		{
			// check blank case
			if (paths == null || paths.Length == 0)
			{
				return "//...";
			}

			// start with "//"
			StringBuilder finalPathBuilder = new StringBuilder("//");

			// strip leading/trailing slashes and "..." and then add final trailing slash to all but last component
			for( int i = 0; i < paths.Length - 1; ++i)
			{     
				finalPathBuilder.Append(StripDirectorySpecifier(paths[i]).Trim('/'));
				finalPathBuilder.Append('/');
			}

			// add last component, add "..." if necessary
			finalPathBuilder.Append(paths[paths.Length - 1].TrimStart('/'));
			string finalPath = finalPathBuilder.ToString();
			if (finalPath.EndsWith("/"))
			{
				return finalPath + "...";
			}
			else
			{
				return finalPath;
			}
		}

		public static string GetFileName(string path)
		{
			if (String.IsNullOrWhiteSpace(path))
			{
				return "";
			}
			if (path.EndsWith("..."))
			{
				return "";
			}
			if (path.EndsWith("/"))
			{
				return "";
			}
			return path.Split('/').Last();
		}

		public static bool LocationExists(P4Context context, bool ignoreDeletedFiles = true, params P4FileSpec[] fileSpecs)
		{
			// query only one file, for speed - hopefully this is common success case
			P4DepotFile file = context.Files(1, ignoreDeletedFiles: ignoreDeletedFiles, locations: fileSpecs).FirstOrDefault();
			if (file != null)
			{
				return true; // we found one file that was not delete at this file spec, therefore location exists
			}
			return false;
		}

		internal static DateTime P4TimestampToDateTime(string timestamp)
		{
			return DateTime.SpecifyKind(new DateTime(1970, 1, 1, 0, 0, 0), DateTimeKind.Utc).AddSeconds(Convert.ToDouble(timestamp)).ToLocalTime();
		}

		internal static P4File.FileType ParseFileTypeString(string typeString)
		{
			P4File.FileBaseType fileType;
			P4File.FileTypeFlags flags = P4File.FileTypeFlags.None;
			uint numStoreRevisions = 0;
			// check for keyword types first, keyword types have implicit flags
			if (KeywordFileTypeMap.ContainsKey(typeString))
			{
				KeywordFileType type = KeywordFileTypeMap[typeString];
				fileType = type.Type;
				flags = type.Flags;
				numStoreRevisions = (uint)(flags.HasFlag(P4File.FileTypeFlags.StoreOnlyHeadRevisions) ? 1 : 0);
			}
			else
			{
				// separate base type and flags
				string[] split = typeString.Split('+');

				// parse type to enum
				fileType = (P4File.FileBaseType)Enum.Parse(typeof(P4File.FileBaseType), split[0]);

				// determine flags
				if (split.Length > 1)
				{
					
					foreach (KeyValuePair<P4File.FileTypeFlags, string> kvp in FlagPatternMap)
					{
						if (split[1].Contains(kvp.Value))
						{
							flags |= kvp.Key;
						}
					}

					// determine store revisons
					if (flags.HasFlag(P4File.FileTypeFlags.StoreOnlyHeadRevisions))
					{
						// if we have the flags number is one if unspecified
						numStoreRevisions = 1;

						// match number of revision by regex
						Match match = Regex.Match(split[1], @"S(\d+)");
						if (match.Success)
						{
							numStoreRevisions = UInt32.Parse(match.Groups[1].Value);
						}
					}
				}
			}

			return new P4File.FileType(fileType, flags, numStoreRevisions);
		}

		internal static string MakeFileTypeString(P4File.FileType type)
		{
			StringBuilder typeString = new StringBuilder(type.ContentType.ToString("g"));
			
			// build flag suffix
			if(type.TypeFlags != P4File.FileTypeFlags.None)
			{
				typeString.Append('+');
				List<string> suffixes = new List<string>();
				foreach (P4File.FileTypeFlags flag in Enum.GetValues(typeof(P4File.FileTypeFlags)))
				{
					// ignore none flag
					if (flag == P4File.FileTypeFlags.None)
					{
						continue;
					}

					// skip special case flags
					if (flag == P4File.FileTypeFlags.RCSKeywordExpansion || flag == P4File.FileTypeFlags.OldKeywordExpansion || flag == P4File.FileTypeFlags.StoreOnlyHeadRevisions)
					{
						continue;
					}

					if (type.TypeFlags.HasFlag(flag))
					{
						suffixes.Add(FlagPatternMap[flag]);
					}
				}

				// special case flag, old keyword expansion
				if (type.TypeFlags.HasFlag(P4File.FileTypeFlags.RCSKeywordExpansion))
				{
					if (type.TypeFlags.HasFlag(P4File.FileTypeFlags.OldKeywordExpansion))
					{
						suffixes.Add(FlagPatternMap[P4File.FileTypeFlags.OldKeywordExpansion]);
					}
					else
					{
						suffixes.Add(FlagPatternMap[P4File.FileTypeFlags.RCSKeywordExpansion]);
					}
				}

				// special case flag, store number of revisions
				if (type.TypeFlags.HasFlag(P4File.FileTypeFlags.StoreOnlyHeadRevisions))
				{
					if (type.NumStoreRevisions > 1)
					{
						suffixes.Add(String.Format("{0}{1}", FlagPatternMap[P4File.FileTypeFlags.StoreOnlyHeadRevisions], type.NumStoreRevisions));
					}
					else
					{
						suffixes.Add(FlagPatternMap[P4File.FileTypeFlags.StoreOnlyHeadRevisions]);
					}
				}

				// add suffix string to stringbuilder
				typeString.Append(String.Join("", suffixes.OrderBy(s => s)));
			}

			return typeString.ToString();
		}

		private static string StripDirectorySpecifier(string path)
		{
			if (path.EndsWith("..."))
			{
				path = path.Substring(0, path.Length - 3);
			}
			return path;
		}
	}
}