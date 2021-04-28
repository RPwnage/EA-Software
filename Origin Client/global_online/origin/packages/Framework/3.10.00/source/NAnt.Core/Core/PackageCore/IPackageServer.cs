using System;
using System.Xml;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Logging;

namespace NAnt.Core.PackageCore
{
    public class InstallProgressEventArgs : EventArgs
    {
        int _localProgress;
        int _globalProgress;
        string _message;
        Release _release;

        public InstallProgressEventArgs(int globalProgress, int localProgress, string message, Release release)
        {
            _localProgress = localProgress;
            _globalProgress = globalProgress;
            _message = message;
            _release = release;
        }

        public int LocalProgress
        {
            get { return _localProgress; }
        }

        public int GlobalProgress
        {
            get { return _globalProgress; }
        }


        public string Message
        {
            get { return _message; }
        }

        public Release Release
        {
            get { return _release; }
        }
    }

    public delegate void InstallProgressEventHandler(object source, InstallProgressEventArgs e);

    public class BreakException : ApplicationException
    {
    }

    public interface IPackageServer : IDisposable
    {
        event InstallProgressEventHandler UpdateProgress;

        Log Log { get; set; }

        bool Break { get; set; }

        bool TryGetRelease(string name, string version, out Release release);

        bool TryGetReleaseDependents(string name, string version, out IList<Release> releases);

        IList<Release> GetAllReleases();

        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        /// <param name="package"></param>
        /// <returns></returns>
        bool TryGetPackage(string name, out Package package);

        bool TryGetPackageReleases(string name, out IList<Release> releases);

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        IList<Package> GetAllPackages();

        //string PostPackage(string fileName, string uncName, string changes, int statusId, string statusComment, string requiredReleases);

        /// <summary>
        /// Installs required package from a Package Server or throws an exception.
        /// </summary>
        /// <param name="release">Release instance with information about package to install</param>        
        /// <returns>Root directory where package was installed</returns>
        void Install(Release release, string rootDirectory);

    }
}
