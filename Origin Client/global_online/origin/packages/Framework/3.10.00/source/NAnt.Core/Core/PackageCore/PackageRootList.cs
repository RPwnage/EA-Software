using System;
using System.IO;
using System.Text;
using System.Runtime;
using System.Collections.Generic;

using NAnt.Core.Util;

namespace NAnt.Core.PackageCore
{


    public class PackageRootList
    {
        public enum RootType { Undefined = 0, DefaultRoot = 1, FrameworkRoot = 2, OnDemandRoot = 4 }

        public const int DefaultMinLevel = 0;
        public const int DefaultMaxLevel = 2;

        public int Add(PathString path, int? minlevel, int? maxlevel, RootType rootType = RootType.Undefined)
        {
            path = path.Normalize();
            int ind = -1;
            // if this is the only package set it as the default
            if (-1 == (ind = FindIndex(path)))
            {
                if ((rootType & RootType.DefaultRoot) != 0)
                {
                    _packageRoots.Insert(0, new Root(new DirectoryInfo(path.Path), minlevel, maxlevel));
                    ind = 0;
                }
                else
                {
                    _packageRoots.Add(new Root(new DirectoryInfo(path.Path), minlevel, maxlevel));
                    ind = _packageRoots.Count - 1;
                }
            }
            else
            {
                // Move to the first position
                if (ind != 0 && ((rootType & RootType.DefaultRoot) != 0))
                {
                    var def = _packageRoots[ind];
                    _packageRoots.RemoveAt(ind);
                    _packageRoots.Insert(0, def);
                }
            }
            if ((rootType & RootType.OnDemandRoot) != 0)
            {
                _onDemandRoot = ind;
            }
            if ((rootType & RootType.FrameworkRoot) != 0)
            {
                _framework = ind;
            }
            return ind;
        }

        public Root this[int index]
        {
            get
            {
                return _packageRoots[index];
            }
        }

        public bool Contains(PathString path)
        {
            path = path.Normalize();
            return _packageRoots.Exists(
                        match => 
                        String.Equals(match.Dir.FullName, path.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase));
        }

        public int FindIndex(PathString path)
        {
            path = path.Normalize();
            return _packageRoots.FindIndex(
                        match =>
                        String.Equals(match.Dir.FullName, path.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase));
        }

        [TargetedPatchingOptOut("Performance critical to inline across NGen image boundaries")]
        public List<Root>.Enumerator GetEnumerator()
        {
            return _packageRoots.GetEnumerator();
        }

        public int Count 
        { 
            get { return _packageRoots.Count; } 
        }

        public void Clear()
        {
            _packageRoots.Clear();
            _framework = -1;
            _onDemandRoot = -1;
        }

        public DirectoryInfo OnDemandRoot
        {
            get
            {
                var root = _onDemandRoot < 0 ? FrameworkRoot : _packageRoots[_onDemandRoot].Dir;
                return root;
            }
        }

		public DirectoryInfo FrameworkRoot 
		{
			get 
			{ 
				if (_framework == -1) 
				{
					throw new Exception("Framework package root has not been set.");
				}

                return _packageRoots[_framework].Dir;
			}

		}

		public bool IsFrameworkRoot(DirectoryInfo item) 
		{
			if (_framework != -1)
                return String.Equals(_packageRoots[_framework].Dir.FullName, PathNormalizer.Normalize(item.FullName), PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase);
			return false;
		}

		public bool HasFrameworkRoot 
		{
			get { return (_framework != -1); }
		}

        public string HasNestedRootListedFirst()
        {
            string msg = null;
            string curr = null;
            string comp = null;
            for (int i = 0; msg == null && i < _packageRoots.Count; i++)
            {
                curr = _packageRoots[i].Dir.FullName;
                for (int j = i + 1; msg == null && j < _packageRoots.Count; j++)
                {
                    comp = _packageRoots[j].Dir.FullName;
                    if (IsSubDir(comp, curr))
                    {
                        msg = String.Format("[master config file] Nested package root \"{0}\" cannot be listed before its parent \"{1}\".",curr, comp);
                    }
                }
            }
            return msg;
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
            for (int i = 0; i < _packageRoots.Count; i++)
            {
                DirectoryInfo packageDirectory = _packageRoots[i].Dir;
                s.Append(prefix);
                s.Append(packageDirectory.FullName);
                if (i != _packageRoots.Count - 1)
                {
                    s.Append(sep);
                }
            }
            return s.ToString();
        }

        public class Root
        {
            public Root(DirectoryInfo dir, int? minlevel, int? maxlevel)
            {
                Dir = dir;
                MinLevel = minlevel ?? DefaultMinLevel;
                MaxLevel = maxlevel ?? DefaultMaxLevel;
            }

            public readonly DirectoryInfo Dir;
            public readonly int MinLevel;
            public readonly int MaxLevel;
        }

        private int _framework = -1;        
        private int _onDemandRoot = -1;
        private readonly List<Root> _packageRoots = new List<Root>();
    }
}
