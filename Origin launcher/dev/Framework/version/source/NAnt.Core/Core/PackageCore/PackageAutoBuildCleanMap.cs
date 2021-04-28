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
using System.Collections.Concurrent;
using System.Linq;
using System.Threading;

namespace NAnt.Core.PackageCore
{
	public class PackageAutoBuildCleanMap
	{
		public sealed class PackageAutoBuildCleanState : IDisposable
		{
			private bool _disposed = false;
			private PackageAutoBuildCleanMap _map;
			private PackageConfigBuildStatus _currentStatus;
			private PackageConfigBuildStatus _targetStatus;
			private PackageConfigKey _key;

			internal PackageAutoBuildCleanState(PackageAutoBuildCleanMap map, PackageConfigBuildStatus targetStatus, string packageFullName, string packageConfig)
			{
				_map = map;
				_targetStatus = targetStatus;
				_key = GetPackageConfigKey(packageFullName, packageConfig);
			}

			public bool IsDone()
			{
				_currentStatus = _map.GetOrAddStatus(_key);
				return _targetStatus == _currentStatus;
			}

			public void Dispose()
			{
				if (!_disposed)
				{
					_disposed = true;
					if (_currentStatus != _targetStatus)
					{
						_map.UpdateStatus(_key, _targetStatus);
					}
				}
			}
		}

		internal enum PackageConfigBuildStatus { Unknown = 0, Built = 1, Cleaned = 2, Processing = 3 }

		private class BuildStatusLock
		{
			internal PackageConfigBuildStatus Status;
			internal readonly ManualResetEventSlim Done;

			internal BuildStatusLock()
			{
				Status = PackageConfigBuildStatus.Processing;
				Done = new ManualResetEventSlim();
			}
		}

		private struct PackageConfigKey
		{
			private string m_packageFullName;
			private string m_packageConfig;

			// use concurrent lookups to get the exact same string refernce every time, this allows us to use reference equality
			// to quickly comapre keys
			private static ConcurrentDictionary<string, string> s_packageFullNames = new ConcurrentDictionary<string, string>();
			private static ConcurrentDictionary<string, string> s_packageConfigs = new ConcurrentDictionary<string, string>();

			public PackageConfigKey(string packageFullName, string packageConfig)
			{
				m_packageFullName = s_packageFullNames.GetOrAdd(packageFullName, packageFullName);
				m_packageConfig = s_packageConfigs.GetOrAdd(packageConfig, packageConfig);
			}

			public override bool Equals(object obj)
			{
				PackageConfigKey target = (PackageConfigKey)obj; // assume this will only ever be compare to PackageConfigKeys, this is a private class so we can make this assumption
				return ReferenceEquals(target.m_packageFullName, m_packageFullName) && ReferenceEquals(target.m_packageConfig, m_packageConfig);
			}

			public override int GetHashCode()
			{
				return m_packageFullName.GetHashCode() ^ m_packageConfig.GetHashCode();
			}
		}

		public static readonly PackageAutoBuildCleanMap AssemblyAutoBuildCleanMap = new PackageAutoBuildCleanMap();

		private readonly ConcurrentDictionary<PackageConfigKey, BuildStatusLock> _autoBuildcleanPackages = new ConcurrentDictionary<PackageConfigKey, BuildStatusLock>();

		public PackageAutoBuildCleanState StartClean(string packageFullName, string packageConfig)
		{
			return new PackageAutoBuildCleanState(this, PackageConfigBuildStatus.Cleaned, packageFullName, packageConfig);
		}

		public PackageAutoBuildCleanState StartBuild(string packageFullName, string packageConfig)
		{
			return new PackageAutoBuildCleanState(this, PackageConfigBuildStatus.Built, packageFullName, packageConfig);
		}

		public bool IsCleanedNonBlocking(string packageFullName, string packageConfig)
		{
			PackageConfigKey key = GetPackageConfigKey(packageFullName, packageConfig);

			BuildStatusLock statusLock;
			if (_autoBuildcleanPackages.TryGetValue(key, out statusLock))
			{
				return (PackageConfigBuildStatus.Cleaned == statusLock.Status);
			}
			return false;
		}

		public void ClearAll()
		{
			// snapshot the current keys when ClearAll is called then try to remove one by one
			PackageConfigKey[] keyList = _autoBuildcleanPackages.Keys.ToArray();
			foreach (PackageConfigKey key in keyList)
			{
				BuildStatusLock statusLock;
				if (_autoBuildcleanPackages.TryRemove(key, out statusLock))
				{
					statusLock.Done.Set();
				}
			}
		}

		private void UpdateStatus(PackageConfigKey key, PackageConfigBuildStatus status)
		{
			BuildStatusLock statusLock;
			if (_autoBuildcleanPackages.TryGetValue(key, out statusLock))
			{
				statusLock.Status = status;
				if (status == PackageConfigBuildStatus.Built || status == PackageConfigBuildStatus.Cleaned)
				{
					statusLock.Done.Set();
				}
			}
		}

		private PackageConfigBuildStatus GetOrAddStatus(PackageConfigKey key)
		{
			// a thread that calls GetOrAddStatus will create an event entry
			// for the given key and be allowed to continue, all other threads
			// after will become trapped waiting for the event to be set by the
			// calling thread (by calling UpdateStatus)
			BuildStatusLock createdStatusLock = null;
			BuildStatusLock statusLock = _autoBuildcleanPackages.GetOrAdd(key, k => createdStatusLock = new BuildStatusLock());

			// if createdStatusLock is equal to statusLock, this call was the adding call and we let the call site process the package
			if (createdStatusLock == statusLock)
			{
				return statusLock.Status;
			}

			// wait on the thread that create to 
			statusLock.Done.Wait();

			return statusLock.Status;
		}

		private static PackageConfigKey GetPackageConfigKey(string packageFullName, string packageConfig)
		{
			return new PackageConfigKey(packageFullName, packageConfig);
		}
	}
}
