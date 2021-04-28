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
using System.IO;
using System.Linq;
using System.Runtime;
using System.Text;

using NAnt.Core.Util;

namespace NAnt.Core.PackageCore
{
	public class PackageRootList : IEnumerable<PackageRootList.Root>
	{
		public enum RootType
		{
			Undefined = 0,
			OnDemandRoot = 1,
			DevelopmentRoot = 2,
			LocalOnDemandRoot = 3
		}

		public class Root
		{
			public Root(DirectoryInfo dir, RootType type)
			{
				Dir = dir;
				Type = type;
			}

			public override string ToString()
			{
				return string.Format("[type={0}] {1}", Type.ToString(), Dir.FullName);
			}

			public readonly DirectoryInfo Dir;
			public readonly RootType Type;
		}

		public const int DefaultMinLevel = 0;
		public const int DefaultMaxLevel = 1;

		private readonly List<Root> m_packageRoots = new List<Root>();
		private Root m_developmentRoot;
		private Root m_ondemandRoot;
		private Root m_localOndemandRoot;
		private const bool m_localOndemandDefaultSetting = false;

		// The rootTypeOption input is dependent on the rootType input being process.
		public void Add(PathString path, RootType rootType = RootType.Undefined, string rootTypeOption = null)
		{
			path = path.Normalize();
			int insertIndex = 0;

			Root newRoot = new Root(new DirectoryInfo(path.Path), rootType);

			if (rootType == RootType.DevelopmentRoot)
			{
				if (m_developmentRoot != null)
				{
					throw new BuildException("Package root list already has a development root set!");
				}

				m_developmentRoot = newRoot;
				insertIndex = 0; // dev root always goes at start
			}
			else if (rootType == RootType.OnDemandRoot)
			{
				if (m_ondemandRoot != null)
				{
					throw new BuildException("Package root list already has an on-demand root set!");
				}

				m_ondemandRoot = newRoot;
				insertIndex = m_packageRoots.Count(); // on demand always goes to end
			}
			else if (rootType == RootType.LocalOnDemandRoot)
			{
				if (m_localOndemandRoot != null)
				{
					throw new BuildException("Package root list already has a local on-demand root set!");
				}

				m_localOndemandRoot = newRoot;
				insertIndex = m_packageRoots.Count(); // local on demand always goes to end - barring regular on demand root
				if (m_packageRoots.Count() > 0 && m_packageRoots[insertIndex - 1].Type == RootType.OnDemandRoot)
				{
					insertIndex -= 1;
				}

				// setup up default usage for local on demand root
				if (!String.IsNullOrEmpty( rootTypeOption ))
				{
					if (rootTypeOption.ToLower() == "use-as-default=true")
					{
						UseLocalOnDemandRootAsDefault = true;
					}
				}
			}
			else
			{
				insertIndex = m_packageRoots.Count(); // other roots go to the end, barring the two ondemand roots
				if (m_packageRoots.Count() > 0 && (m_packageRoots[insertIndex - 1].Type == RootType.LocalOnDemandRoot))
				{
					insertIndex -= 1;
					if (m_packageRoots.Count() > 1 && m_packageRoots[insertIndex - 1].Type == RootType.LocalOnDemandRoot)
					{
						insertIndex -= 1;
					}
				}
			}

			m_packageRoots.Insert(insertIndex, newRoot);
		}

		public void OverrideOnDemandRoot(PathString onDemandRootOverride)
		{
			if (m_packageRoots.Any() && m_packageRoots.Last().Type == RootType.OnDemandRoot && m_ondemandRoot != null)
			{
				m_packageRoots.RemoveAt(m_packageRoots.Count - 1);
				m_ondemandRoot = null;
			}

			try
			{
				if (!Directory.Exists(onDemandRootOverride.Path))
				{
					Directory.CreateDirectory(onDemandRootOverride.Path);
				}
				Add(onDemandRootOverride, PackageRootList.RootType.OnDemandRoot);
			}
			catch (Exception e)
			{
				throw new BuildException(String.Format("On-demand package download package root '{0}' specifed by override could not be created: {1}", onDemandRootOverride.Path, e.Message));
			}
		}

		public Root this[int index]
		{
			get
			{
				return m_packageRoots[index];
			}
		}

		public bool Contains(PathString path)
		{
			path = path.Normalize();
			return m_packageRoots.Exists(
						match => 
						String.Equals(match.Dir.FullName, path.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase));
		}

		public int FindIndex(PathString path)
		{
			path = path.Normalize();
			return m_packageRoots.FindIndex(
						match =>
						String.Equals(match.Dir.FullName, path.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase));
		}

		[TargetedPatchingOptOut("Performance critical to inline across NGen image boundaries")]
		public List<Root>.Enumerator GetEnumerator()
		{
			return m_packageRoots.GetEnumerator();
		}

		public int Count 
		{ 
			get { return m_packageRoots.Count; } 
		}

		public void Clear()
		{
			m_packageRoots.Clear();
			m_developmentRoot = null;
			m_ondemandRoot = null;
			m_localOndemandRoot = null;
			UseLocalOnDemandRootAsDefault = m_localOndemandDefaultSetting;
		}

		public DirectoryInfo OnDemandRoot
		{
			get
			{
				if (m_ondemandRoot != null)
				{
					return m_ondemandRoot.Dir;
				}

				throw new Exception("On demand root has not been set.");
			}
		}

		public DirectoryInfo DevelopmentRoot
		{
			get 
			{ 
				if (m_developmentRoot != null)
				{
					return m_developmentRoot.Dir;
				}

				throw new Exception("Development root has not been set.");
			}
		}

		public DirectoryInfo LocalOnDemandRoot
		{
			get
			{
				if (m_localOndemandRoot != null)
				{
					return m_localOndemandRoot.Dir;
				}

				throw new Exception("Local on-demand root has not been set in the masterconfig file.");
			}
		}

		public bool UseLocalOnDemandRootAsDefault {
			// m_useLocalOnDemandRootAsDefault only store what is listed in masterconfig!
			get; private set;
		} = m_localOndemandDefaultSetting;

		public bool HasLocalOnDemandRoot
		{
			get
			{
				return m_localOndemandRoot != null;
			}
		}

		public bool HasOnDemandRoot
		{
			get
			{
				return m_ondemandRoot != null;
			}
		}

		private bool IsSubDir(string dir, string subdir)
		{
			bool isSubdir = false;

			if (!String.IsNullOrEmpty(dir) && (dir[dir.Length - 1] == '\\' || dir[dir.Length - 1] == '/'))
			{
				dir = dir.Substring(0, dir.Length - 1);
			}

			if (!String.IsNullOrEmpty(subdir) && (subdir[subdir.Length - 1] == '\\' || subdir[subdir.Length - 1] == '/'))
			{
				subdir = subdir.Substring(0, subdir.Length - 1);
			}

			if (!String.IsNullOrEmpty(dir) && subdir.Length >= dir.Length && subdir.StartsWith(dir, StringComparison.OrdinalIgnoreCase))
			{
				if (subdir.Length == dir.Length)
				{
					//they are equal
					isSubdir = true;
				}
				else if ((subdir[dir.Length] == '\\' || subdir[dir.Length] == '/'))
				{
					isSubdir = true;
				}
			}            
			return isSubdir;
		}

		public override string ToString()
		{
			return ToString(sep: ";", prefix: "");
		}

		public string ToString(string sep="; ", string prefix="")
		{
			StringBuilder s = new StringBuilder();
			for (int i = 0; i < m_packageRoots.Count; i++)
			{
				DirectoryInfo packageDirectory = m_packageRoots[i].Dir;
				s.Append(prefix);
				s.Append(packageDirectory.FullName);
				if (i != m_packageRoots.Count - 1)
				{
					s.Append(sep);
				}
			}
			return s.ToString();
		}

		IEnumerator<Root> IEnumerable<Root>.GetEnumerator()
		{
			return m_packageRoots.GetEnumerator();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return m_packageRoots.GetEnumerator();
		}
	}
}
