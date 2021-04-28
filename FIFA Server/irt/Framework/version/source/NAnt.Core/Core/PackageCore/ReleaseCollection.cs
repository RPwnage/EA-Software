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
using System.Threading;

using NAnt.Core.Util;

namespace NAnt.Core.PackageCore
{
    public class ReleaseCollection : ICollection<Release>, IEnumerable<Release>, IComparer<Release>
    {
        private readonly ReaderWriterLockSlim _lock = new ReaderWriterLockSlim();
        private readonly SortedSet<Release> _releases;

        public bool IsReadOnly { get { return false; } }

        public int Count
        {
            get
            {
                _lock.EnterReadLock();
                try
                {
                    return _releases.Count;
                }
                finally
                {
                    _lock.ExitReadLock();
                }
            }
        }

        public ReleaseCollection()
        {
            _releases = new SortedSet<Release>(this as IComparer<Release>);
        }

        public Release FindByDirectory(string path)
        {
            path = PathNormalizer.Normalize(path);
            _lock.EnterReadLock();
            try
            {
                return _releases.FirstOrDefault(release => String.Compare(release.Path, path, ignoreCase: true) == 0);
            }
            finally
            {
                _lock.ExitReadLock();
            }
        }

        public Release FindFlattenedRelease()
        {
            _lock.EnterReadLock();
            try
            {
                return _releases.FirstOrDefault(release => release.IsFlattened);
            }
            finally
            {
                _lock.ExitReadLock();
            }
        }

        // TODO ReleaseCollection.FindByVersion backwards compatibility
        // dvaliant 2017/05/19
        /* ReleaseCollection.FindByVersion is used in a few places outside of Framework,
        ideally this member should be removed but in order to preserve some
        level of backwards compability this class which mimicus the old
        interface is used - should be removed in a future version*/
        public Release FindByVersion(string version, bool ignoreCase = false)
        {
            if (version == Release.Flattened)
            {
                // It is possible for version name to be "flattened" but package is actually not a flattened package.
                // This happends when we try to do a p4 sync and the uri points to a flattened package.  After it is
                // synced, the package shows up under ondemand root as [package_name]\flattened (ie having a version folder
                // named "flattened" but it is no longer a flattened package).  So we test for null, if the return is null,
                // we try the non-flattened flow and actually compare the version name.
                Release foundRelease = FindFlattenedRelease();
                if (foundRelease != null)
                {
                    return foundRelease;
                }
            }

            _lock.EnterReadLock();
            try
            {
                return _releases.FirstOrDefault(release => String.Compare(release.Version, version, ignoreCase) == 0);
            }
            finally
            {
                _lock.ExitReadLock();
            }
        }

        public void Add(Release release)
        {
            _lock.EnterWriteLock();
            try
            {
                _releases.Add(release);
            }
            finally
            {
                _lock.ExitWriteLock();
            }
        }

        public void Clear()
        {
            _lock.EnterWriteLock();
            try
            {
                _releases.Clear();
            }
            finally
            {
                _lock.ExitWriteLock();
            }
        }

        public int Compare(Release releaseA, Release releaseB)
        {
            return releaseA.CompareTo(releaseB);
        }

        public bool Contains(Release item)
        {
            _lock.EnterReadLock();
            try
            {
                return _releases.Contains(item);
            }
            finally
            {
                _lock.ExitReadLock();
            }
        }

        public void CopyTo(Release[] array, int arrayIndex)
        {
            _lock.EnterReadLock();
            try
            {
                _releases.CopyTo(array, arrayIndex);
            }
            finally
            {
                _lock.ExitReadLock();
            }
        }
        public bool Remove(Release item)
        {
            _lock.EnterWriteLock();
            try
            {
                return _releases.Remove(item);
            }
            finally
            {
                _lock.ExitWriteLock();
            }
        }

        public IEnumerator<Release> GetEnumerator()
        {
            _lock.EnterReadLock();
            try
            {
                foreach (Release r in _releases)
                {
                    yield return r;
                }
            }
            finally
            {
                _lock.ExitReadLock();
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
