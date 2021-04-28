// NAnt - A .NET build tool
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

// Disable obsolete warning for Uri(string, bool) constructor. We need to use it to prevent escaping
#pragma warning disable 618


namespace NAnt.Core.Util
{
	/// <summary>Helper class for creating valid URIs.</summary>
	public class UriFactory
	{
		public static string GetLocalPath(string path)
		{
			if (!String.IsNullOrWhiteSpace(path))
			{
				path = CreateUri(path).LocalPath;
			}
			else
			{
				path = String.Empty;
			}
			return path;
		}

		public static Uri CreateUri(string path)
		{
			Uri u = null;

			// HACK: try to create a URI, if doesn't work, then try to convert path into
			// an absolute path. Then crate URI from absolute path.
			// If that doesn't work, we give up, and throw exception.

			if (!Uri.TryCreate(path, UriKind.RelativeOrAbsolute, out u))
			{
				u = new Uri(PathNormalizer.Normalize(path, true));
			}
			return u;
		}

		/// <remarks>
		/// The basic idea of this method is to remove the scheme from the file name before
		/// passing it to the Uri constructor. Thus, the constructor will treat the constructor
		/// as a file and not a URI, given us the result we desire.
		/// 
		/// The build file needs to contain a '#' in the filename for the test below. 
		/// I cant add this test to perforce because it wont accept filenames with '#'.
		/// </remarks>
		/// <example>
		/// <project>
		/// <fail 
		///     message='Uri class returned invalid result for buildfile path.'
		///     if='@{StrIndexOf("${nant.project.buildfile}", "Bug.#653.build")} == -1' />
		/// </project>
		/// </example>
		public static Uri CreateUri(Uri uri)
		{
			bool isUnc = uri.IsUnc;
			const string UncScheme = @"//";

			if (uri.IsFile)
			{
				string path = uri.ToString();
				string left = uri.GetLeftPart(UriPartial.Scheme);
				// .NET 2 API change: UriPartial.Scheme returns file://
				// instead of file:///, so we need to do the follow check
				// to get rid of the extra /.
				bool dotNet2 = path.StartsWith(left + "/", StringComparison.Ordinal);
				if (dotNet2)
				{
					path = path.Substring(left.Length + 1);
				}
				else
				{
					path = path.Substring(left.Length);
				}
				if (isUnc)
					path = UncScheme + path;
				uri = new Uri(path, true);
			}

			return uri;
		}


		//IM TODO:
		/*

						// Try to use the cache
						string fileName = doc.BaseURI;
						if (PlatformUtil.IgnoreCase) {
							fileName = fileName.ToLower();	
						}
						if (fileName.StartsWith(URI_FILE))
						{
							fileName = fileName.Substring(URI_FILE.Length);
						}
						else if(fileName.StartsWith(URI_NETWORK_FILE))
						{
							fileName = fileName.Substring(URI_NETWORK_FILE.Length);
						}
		 */


		const string URI_FILE = "file:///";
		const string URI_NETWORK_FILE = "file://";

	}
}
