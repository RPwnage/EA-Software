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
using System.Linq;
using System.Web;
using System.Collections.Specialized;

namespace NAnt.Perforce.ConnectionManagment
{
	public class P4QueryData
	{
		public const uint UseHeadCL = 0;

		public readonly uint Changelist;
		public readonly bool Rooted;
		public readonly P4Server.P4CharSet CharSet;

		public P4QueryData(uint changelist, bool rooted, P4Server.P4CharSet charSet)
		{
			Changelist = changelist;
			Rooted = rooted;
			CharSet = charSet;
		}

		public string BuildP4AbsolutePath(Uri uri, string pkgName, string pkgVersion)
		{
			string absolutePath = uri.AbsolutePath.StartsWith("//") ? uri.AbsolutePath : "/" + uri.AbsolutePath; // The uri spec only allows a single slash at the start of the absolute path, we need to add an extra one for perforce

			if (Rooted)
			{
				absolutePath = P4Utils.Combine(absolutePath, pkgName, pkgVersion);
			}

			return absolutePath;
		}

		// converts a uri query to a cl number, if query is "?head" returns 0
		public static P4QueryData Parse(Uri uri)
		{
			const string ChangelistKey = "cl";
			const string CharSetKey = "charset";

			string query = uri.Query;
			NameValueCollection collection = HttpUtility.ParseQueryString(query);
			bool rooted = false;
			bool foundHeadQuery = false;
			uint changelist = UseHeadCL;
			P4Server.P4CharSet charSet = P4Server.P4CharSet.none; // if no charset was specified in uri, set it to none.

			foreach (var key in collection)
			{
				if (key == null)
				{
					// We can have multiple entries separated by comma.  
					// We need to split them up first in case we accidentally do a partial match of a longer string.
					string[] tokens = collection[null].Split(',');
					foreach (string token in tokens)
					{
						if (token == "rooted")
						{
							rooted = true;
						}
						else if (token == "head")
						{
							// changelist is defaulted with UseHeadCL.  If it is changed, it means we had encounterred a query with changelist info.
							if (changelist != UseHeadCL)
							{
								throw new FormatException(String.Format("The uri provided '{0}' appears to have both changelist and head specification.  Those two queries cannot be used together.", uri.OriginalString));
							}
							foundHeadQuery = true;
							changelist = UseHeadCL;
						}
						else
						{
							throw new FormatException(String.Format("The uri provided '{0}' appears to have unrecognized query token '{1}'. Please check the Framework documentation on Perforce URIs for the recognized query strings.", uri.OriginalString, token));
						}
					}
				}
				else if (key.ToString() == ChangelistKey)
				{
					if (foundHeadQuery)
					{
						throw new FormatException(String.Format("The uri provided '{0}' appears to have both changelist and head specification. Those two queries cannot be used together.", uri.OriginalString));
					}
					changelist = uint.Parse(collection[ChangelistKey]);
				}
				else if (key.ToString() == CharSetKey)
				{
					string charSetValue = collection[CharSetKey];
					if (!Enum.TryParse(collection[CharSetKey], out charSet))
					{
						string validCharSets = String.Join(", ", Enum.GetValues(typeof(P4Server.P4CharSet)).Cast<P4Server.P4CharSet>().Select(s => "'" + s + "'"));
						throw new FormatException(String.Format("The uri provided '{0}' has invalid charset value '{1}'. Valid values are: {2}.", uri.OriginalString, charSet, validCharSets));
					}
				}
				else
				{
					throw new FormatException(String.Format("The uri provided '{0}' appears to have unrecognized query entry {1}.  Please check the Framework documentation on Perforce URIs for the recognized query strings.", uri.OriginalString, key.ToString()));
				}
			}
			return new P4QueryData(changelist, rooted, charSet);
		}
	}
}