// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
using System.Collections.Generic;
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;

namespace EA.Eaconfig.Core
{
	public class CommonRoots<T>
	{
		public readonly IDictionary<string, string> Roots;
		internal readonly IEnumerable<string> CustomPrefixes;

		private Func<T, string> GetPath;

		public CommonRoots(IOrderedEnumerable<T> files, Func<T,string> getPath, IEnumerable<string> prefixes = null, bool usePrefixesAsRoot = false)
		{
			GetPath = getPath;
			CustomPrefixes = prefixes;
			StringComparison stringComp = PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase;
			Roots = new Dictionary<string, string>();

			// Loop through all files to find the common root of the files that is under the matched prefix.  The common root
			// for each prefix is expected to be the deepest common directory under the prefix for the listed files.
			foreach (var file in files)
			{
				string currentPath = GetPath(file);
				if (String.IsNullOrEmpty(currentPath))
				{
					continue;
				}

				// either the drive of current path or the first matched prefixes
				string currentDrive = GetPrefix(currentPath);

				// Move current path to parent directory
				currentPath = Path.GetDirectoryName(currentPath);
				if (usePrefixesAsRoot)
				{
					// We basically only want to find the list of prefixes that the fileset really uses but don't want
					// to drill down to the deepest common directory for the prefix.
					currentPath = currentDrive;
				}

				// Make sure path ends with a path separator
				if (currentPath != Path.GetPathRoot(currentPath))
				{
					currentPath += Path.DirectorySeparatorChar;
				}

				if (!Roots.ContainsKey(currentDrive))
				{
					// there is no common path set for this drive yet
					Roots[currentDrive] = currentPath;
				}

				while (Roots[currentDrive] != Path.GetPathRoot(Roots[currentDrive])
					   && !currentPath.StartsWith(Roots[currentDrive], stringComp))
				{
					// Move the path back until we get a common root and reset Roots[currentDrive]
					string newPath = Path.GetDirectoryName(Roots[currentDrive].TrimEnd(new char[] { Path.DirectorySeparatorChar }));

					if (newPath != Path.GetPathRoot(newPath))
					{
						newPath += Path.DirectorySeparatorChar;
					}
					Roots[currentDrive] = newPath;
				}
			}
		}

		internal IDictionary<string, bool> FilterMap(IOrderedEnumerable<T> files, IDictionary<string, string> predefinedFilterMap = null)
		{
			var duplicates = new Dictionary<string, bool>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);
			var temp = new Dictionary<string, string>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

			foreach (var file in files)
			{
				string basedir = Path.GetDirectoryName(GetPath(file));

				string prefix = GetPrefix(basedir);

				string root;

				if (!Roots.TryGetValue(prefix, out root))
				{
					throw new BuildException(String.Format("Internal error: common root for '{0}' is not defined", prefix));
				}

				string dirName = String.Empty;

				if (predefinedFilterMap == null || !predefinedFilterMap.TryGetValue(prefix, out dirName))
				{
					if (String.Compare(basedir, 0, root, 0, basedir.Length, !PathUtil.IsCaseSensitive) != 0)
					{
						// work out what the name of the next directory down should be
						int endIdx = basedir.IndexOf(Path.DirectorySeparatorChar, root.Length);

						if (endIdx >= 0)
						{
							dirName = basedir.Substring(root.Length, endIdx - root.Length);
						}
						else
						{
							dirName = basedir.Substring(root.Length);
						}
					}
				}

				if (!String.IsNullOrEmpty(dirName))
				{
					string previousRoot;
					if (temp.TryGetValue(dirName, out previousRoot))
					{
						if (0 != String.Compare(root, previousRoot, !PathUtil.IsCaseSensitive))
						{
							duplicates[dirName] = true;
						}
					}
					else
					{
						temp.Add(dirName, root);
					}
				}
			}

			return duplicates;
		}

		public string GetPrefix(string path)
		{
			var prefix = CustomPrefixes == null ? null : CustomPrefixes.FirstOrDefault(p => path.StartsWith(p, (PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase)));
			
			if(prefix == null)
			{
				return PathUtil.GetDriveLetter(path);
			}

			return prefix;
		}
	}
}
