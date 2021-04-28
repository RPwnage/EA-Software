// Copyright (C) Electronic Arts Canada Inc. 2003.  All rights reserved.

using System;
using System.IO;
using System.Net;
using System.Text;
using System.Xml;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core.PackageCore;
using NAnt.Core.Logging;


namespace EA.PackageSystem.PackageCore
{
    public class PackageInstaller
    {

        public  PackageInstallerReleaseCollection Releases
        {
            get { return _releases; }
        }
        PackageInstallerReleaseCollection _releases;


        public Log Log
        {
            get
            {
                if (_log == null)
                {
                    _log = new Log(Log.LogLevel.Normal, identLevel: 0, id:String.Empty);
                    _log.Listeners.AddRange(LoggerFactory.Instance.CreateLogger(Log.DefaultLoggerTypes));
                }
                return _log;
            }

            set
            {
                _log = value;
            }
        }
        private Log _log;

        public bool Break
        {
            get { return _break; }
            set 
            { 
                _break = true;
                if (_packageServer != null)
                {
                    _packageServer.Break = _break;
                }
            }
        }
        bool   _break = false;

        public bool WithDependents
        {
            get { return _withDependents; }
            set { _withDependents = true; }
        }
        bool   _withDependents = false;

        public event InstallProgressEventHandler UpdateProgress;

        public PackageInstaller()
        {
            _releases = new PackageInstallerReleaseCollection(this);
        }

        public void Install()
        {
            using (_packageServer = PackageServerFactory.CreatePackageServer())
            {
                _packageServer.Log = Log;
                _packageServer.Break = Break;
                _packageServer.UpdateProgress += new InstallProgressEventHandler(UpdateProgress);

                foreach (var release in Releases)
                {
                    _packageServer.Install(release, PackageMap.Instance.PackageRoots.OnDemandRoot.FullName);

                    Log.Status.WriteLine("  installed '{0}-{1}' to '{2}'", release.Name, release.Version, PackageMap.Instance.PackageRoots.OnDemandRoot.FullName);
                }

            }
        }

        private void UpdateInstallProgress(object sender, InstallProgressEventArgs e)
        {
            if (e != null)
            {
                IPackageServer packageServer = sender as IPackageServer;
                if (packageServer != null && packageServer.Log != null)
                {
                    if (e.LocalProgress < 0)
                    {
                        packageServer.Log.Status.WriteLine(e.Message);
                    }
                    else
                    {
                        packageServer.Log.Debug.WriteLine("{0}. Progress - {1}%", e.Message, e.LocalProgress);
                    }
                }
            }
        }

        private IPackageServer _packageServer;

    }

    public class PackageInstallerReleaseCollection : List<Release>, ICollection<Release>
    {
        private PackageInstaller _installer;

        internal PackageInstallerReleaseCollection(PackageInstaller installer)
        {
            _installer = installer;
        }

        /// <summary>Add a release to the collection.</summary>
        /// <param name="name">Name of package.</param>
        /// <param name="version">Version of package.</param>
        /// <param name="addDependents">If <c>true</c> add release dependents as well, otherwise only add the release.</param>
        public void Add(string name, string version, bool addDependents = false)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }
            if (version == null)
            {
                throw new ArgumentNullException("version");
            }


            using (IPackageServer packageServer = PackageServerFactory.CreatePackageServer())
            {
                Release packageRelease;
                if (!packageServer.TryGetRelease(name, version, out packageRelease))
                {
                    string msg = String.Format("Package '{0}-{1}' not found on package server.", name, version);
                    throw new ApplicationException(msg);
                }

                if (addDependents)
                {
                    IList<Release> dependentToinstall = null;
                    if (packageServer.TryGetReleaseDependents(name, version, out dependentToinstall) && dependentToinstall.Count > 0)
                    {
                        _installer.Log.Status.WriteLine("   Found {0} dependents for package '{1}-{2}' on the package server:", dependentToinstall.Count, name, version);
                        foreach (Release r in dependentToinstall)
                        {
                            _installer.Log.Status.WriteLine("          '{0}-{1}'", r.Name, r.Version);
                            if (!this.Any(rel => 0 == rel.CompareTo(r)))
                            {
                                this.Add(r);
                            }
                        }
                    }
                }

                this.Add(packageRelease);
            }
        }
    }
}
