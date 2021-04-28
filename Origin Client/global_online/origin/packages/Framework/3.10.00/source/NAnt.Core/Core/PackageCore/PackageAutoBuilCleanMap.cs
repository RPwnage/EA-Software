using System;
using System.Collections.Generic;
using System.Threading;

namespace NAnt.Core.PackageCore
{


    public class PackageAutoBuilCleanMap
    {
        public static readonly PackageAutoBuilCleanMap AssemblyAutoBuilCleanMap = new PackageAutoBuilCleanMap();

        public class PackageAutoBuilCleanState : IDisposable
        {

            internal PackageAutoBuilCleanState(PackageAutoBuilCleanMap map, PackageAutoBuilCleanMap.PackageConfigBuildStatus targetStatus, string packageFullName, string packageConfig)
            {
                _map = map;
                _targetStatus = targetStatus;
                _key = map.GetPackageConfigKey(packageFullName, packageConfig);
            }

            public bool IsDone()
            {
                _currentStatus = _map.GetOrAddStatus(_key);
                return _targetStatus == _currentStatus;
            }

            public void Dispose()
            {
                Dispose(true);
                GC.SuppressFinalize(this);
            }

            void Dispose(bool disposing)
            {
                if (!this._disposed)
                {
                    if (disposing)
                    {
                        if (_currentStatus != _targetStatus)
                        {
                            _map.UpdateStatus(_key, _targetStatus);
                        }
                    }
                }
                _disposed = true;
            }
            private bool _disposed = false;


            
            private PackageAutoBuilCleanMap _map;
            private PackageConfigBuildStatus _targetStatus;
            private PackageConfigBuildStatus _currentStatus;
            private string _key;
        }

        public PackageAutoBuilCleanState StartClean(string packageFullName, string packageConfig)
        {
            return new PackageAutoBuilCleanState(this, PackageConfigBuildStatus.Cleaned, packageFullName, packageConfig);
        }

        public PackageAutoBuilCleanState StartBuild(string packageFullName, string packageConfig)
        {
            return new PackageAutoBuilCleanState(this, PackageConfigBuildStatus.Built, packageFullName, packageConfig);
        }

        public bool Stop;

        public readonly int Timeout;

        public bool IsBuilt(string packageFullName, string packageConfig)
        {
            PackageConfigBuildStatus status = GetOrAddStatus(GetPackageConfigKey(packageFullName, packageConfig), false);
            return PackageConfigBuildStatus.Built == status;
        }

        public bool IsCleaned(string packageFullName, string packageConfig)
        {
            PackageConfigBuildStatus status = GetOrAddStatus(GetPackageConfigKey(packageFullName, packageConfig), false);
            return PackageConfigBuildStatus.Cleaned == status;
        }

        public bool IsCleanedNonBlocking(string packageFullName, string packageConfig)
        {
            bool cleaned = false;
            string key = GetPackageConfigKey(packageFullName, packageConfig);

            lock (_autoBuildcleanPackages)
            {
                PackageConfigBuildStatus status;

                if (_autoBuildcleanPackages.TryGetValue(key, out status))
                {
                    cleaned = (PackageConfigBuildStatus.Cleaned == status);
                }
            }

            return cleaned;
        }

        public PackageAutoBuilCleanMap(int timeout = -1)
        {
            _autoBuildcleanPackages = new Dictionary<string,PackageConfigBuildStatus>();
            Stop = false;
            Timeout = (timeout > 0) ? timeout : DEFAULT_WAIT_TIMEOUT;
        }

        private void UpdateStatus(string key, PackageConfigBuildStatus status)
        {
            lock (_autoBuildcleanPackages)
            {
                _autoBuildcleanPackages[key] = status;
                Monitor.PulseAll(_autoBuildcleanPackages);
            }
        }

        private PackageConfigBuildStatus GetOrAddStatus(string key, bool add = true)
        {
            PackageConfigBuildStatus status = PackageConfigBuildStatus.Unknown;

            lock(_autoBuildcleanPackages)
            {
                if (!_autoBuildcleanPackages.TryGetValue(key, out status))
                {
                    if (add)
                    {
                        _autoBuildcleanPackages[key] = PackageConfigBuildStatus.Processing;
                    }
                }
                else
                {
                    status = WaitFor(key, status);
                }
            }

            return status;
        }

        private PackageConfigBuildStatus WaitFor(string key, PackageConfigBuildStatus status)
        {
            while (!Stop && status == PackageConfigBuildStatus.Processing)
            {
                if (_autoBuildcleanPackages.TryGetValue(key, out status))
                {
                    if (status == PackageConfigBuildStatus.Processing)
                    {
                        Monitor.Wait(_autoBuildcleanPackages, Timeout);
                    }
                }
                else
                {
                    status = PackageConfigBuildStatus.Unknown;
                }
            }
            return status;
        }

        private string GetPackageConfigKey(string packageFullName, string packageConfig)
        {
            return packageFullName + "-" + packageConfig;
        }

        internal enum PackageConfigBuildStatus { Unknown = 0, Built = 1, Cleaned = 2, Processing = 3 }
        
        private readonly Dictionary<string, PackageConfigBuildStatus> _autoBuildcleanPackages;

        const int DEFAULT_WAIT_TIMEOUT =  1000;


    }
}
