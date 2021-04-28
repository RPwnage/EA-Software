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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;

namespace NAnt.Core.Util
{
	public class IncludedFilesCollection
	{
		private class IncludedNode
		{
			internal readonly ConcurrentDictionary<string, IncludedNode> IncludedFiles = new ConcurrentDictionary<string, IncludedNode>();
		}

		private readonly ConcurrentDictionary<string, IncludedNode> m_files = new ConcurrentDictionary<string, IncludedNode>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

		internal IncludedFilesCollection()
		{
		}

		internal IncludedFilesCollection(IncludedFilesCollection other)
		{
			foreach (KeyValuePair<string, IncludedNode> rootKvp in other.m_files)
			{
				IncludedNode copyNode = m_files.GetOrAdd(rootKvp.Key, i => new IncludedNode());
				foreach (KeyValuePair<string, IncludedNode> nestedKvp in rootKvp.Value.IncludedFiles)
				{
					copyNode.IncludedFiles.TryAdd(nestedKvp.Key, m_files.GetOrAdd(nestedKvp.Key, i => new IncludedNode()));
				}
			}
		}

		public IEnumerable<string> GetIncludedFiles(string file, bool recursive = false)
		{
			string key = NormalizeKey(file);

			if (!m_files.TryGetValue(key, out IncludedNode node))
			{
				return Enumerable.Empty<string>();
			}

			if (recursive)
			{
				HashSet<IncludedNode> queued = new HashSet<IncludedNode>(node.IncludedFiles.Values);
				Queue<IncludedNode> unprocessed = new Queue<IncludedNode>(node.IncludedFiles.Values);
				List<string> includes = new List<string>(node.IncludedFiles.Keys);

				while (unprocessed.Any())
				{
					IncludedNode process = unprocessed.Dequeue();
					foreach (KeyValuePair<string, IncludedNode> includeFile in process.IncludedFiles)
					{
						if (!queued.Contains(includeFile.Value))
						{
							queued.Add(includeFile.Value);
							includes.Add(includeFile.Key);
							unprocessed.Enqueue(includeFile.Value);
						}
					}
				}

				return includes;
			}
			else
			{
				return node.IncludedFiles.Keys;
			}
		}

		public bool Contains(string key)
		{
			return m_files.ContainsKey(NormalizeKey(key));
		}

		public bool AnyStartsWith(string key)
		{
			string normalizedKey = NormalizeKey(key);
			foreach (string path in m_files.Keys)
			{
				if (path.StartsWith(normalizedKey)) return true;
			}
			return false;
		}

		internal void AddInclude(string include)
        {
            string key = NormalizeKey(include);
            m_files.GetOrAdd(key, i => new IncludedNode());
        }

        internal void AddInclude(string parent, string include)
        {
            string parentKey = NormalizeKey(parent);
            IncludedNode parentNode = m_files.GetOrAdd(parentKey, i => new IncludedNode());

            string key = NormalizeKey(include);
            IncludedNode includedNode = m_files.GetOrAdd(key, i => new IncludedNode());

            parentNode.IncludedFiles.GetOrAdd(key, includedNode);
        }

        private static string NormalizeKey(string key)
		{
			return PathUtil.IsCaseSensitive ? key.ToLowerInvariant() : key;
		}
	}
}
