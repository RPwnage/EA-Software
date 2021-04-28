// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;
using System.IO;
using System.Text;
using System.Reflection;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Text.RegularExpressions;
using System.Xml;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using System.Collections.Specialized;

namespace NAnt.Core.PackageCore
{

    public class ReleaseCollection : ICollection<Release>, IEnumerable<Release>, IComparer<Release>
    {

        internal ReleaseCollection(PackageCollection packages = null)
        {
            _lock = new ReaderWriterLockSlim(LockRecursionPolicy.SupportsRecursion);
            _packages = packages;
            if (_packages == null)
            {
                _releases = new SortedSet<Release>(this as IComparer<Release>);
            }
        }

        /// <summary>Releases are stored in ascending order without duplicates.</summary>
        public void Add(Release release)
        {
            _lock.EnterUpgradeableReadLock();
            try
            {
                if (_releases != null)
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
            }
            finally
            {
                _lock.ExitUpgradeableReadLock();
            }
        }


        public Release FindByNameAndVersion(string packageName, string version, bool ignoreCase = false)
        {
            bool flattened = (version == Release.Flattened);

            _lock.EnterReadLock();
            try
            {
                if (_releases != null)
                {
                    bool testpackage = true;
                    foreach (Release release in _releases)
                    {
                        if (testpackage && String.Compare(release.Name, packageName, ignoreCase) != 0)
                        {
                            return null;
                        }
                        else
                        {
                            testpackage = false;
                        }
                        if (flattened && release.IsFlattened)
                        {
                            return release;
                        }
                        else if (String.Compare(release.Version, version, ignoreCase) == 0)
                        {
                            return release;
                        }
                    }
                }
                else
                {
                    Package p;
                    if (_packages.TryGetValue(packageName, out p))
                    {
                        return p.Releases.FindByNameAndVersion(packageName, version, ignoreCase);
                    }
                }
            }
            finally
            {
                _lock.ExitReadLock();
            }
            return null;
        }


        public Release FindByName(string packageName, bool ignoreCase = false)
        {
            _lock.EnterReadLock();
            try
            {
                if (_releases != null)
                {
                    foreach (Release release in this)
                    {
                        if (String.Compare(release.Name, packageName, ignoreCase) == 0)
                        {
                            return release;
                        }
                    }
                }
                else
                {
                    Package p;
                    if (_packages.TryGetValue(packageName, out p))
                    {
                        return p.Releases.FindByName(packageName, ignoreCase);
                    }

                }
            }
            finally
            {
                _lock.ExitReadLock();
            }
            return null;
        }

        public Release FindByDirectory(string path)
        {
            path = PathNormalizer.Normalize(path);

            _lock.EnterReadLock();
            try
            {
                foreach (Release release in this)
                {
                    if (String.Compare(release.Path, path, true) == 0)
                    {
                        return release;
                    }
                }
            }
            finally
            {
                _lock.ExitReadLock();

            }
            return null;
        }

        public Release FindByNameAndDirectory(string packageName, string path)
        {
            _lock.EnterReadLock();
            try
            {
                foreach (Release release in this)
                {
                    if (String.Compare(release.Name, packageName, true) == 0)
                    {
                        if (String.Compare(release.Path, path, true) == 0)
                        {
                            return release;
                        }
                    }
                }
            }
            finally
            {
                _lock.ExitReadLock();
            }
            return null;
        }

        public int Compare(Release releaseA, Release releaseB)
        {
            return releaseA.CompareTo(releaseB);
        }

        public IEnumerator<Release> GetEnumerator()
        {
            _lock.EnterReadLock();
            try
            {
                if (_releases != null)
                {
                    foreach (Release r in _releases)
                    {
                        yield return r;
                    }
                }
                else
                {
                    foreach (KeyValuePair<string, Package> item in _packages)
                    {
                        foreach (Release r in item.Value.Releases)
                        {
                            yield return r;
                        }
                    }
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

        public int Count
        {
            get
            {
                _lock.EnterReadLock();
                try
                {
                    if (_releases != null)
                    {
                        return _releases.Count;
                    }
                    else
                    {
                        int count = 0;
                        foreach (Package p in _packages.Values)
                        {
                            count += p.Releases.Count;
                        }
                        return count;
                    }
                }
                finally
                {
                    _lock.ExitReadLock();
                }
            }
        }

        public bool IsReadOnly
        {
            get
            {
                _lock.EnterReadLock();
                try
                {
                    return _releases == null;
                }
                finally
                {
                    _lock.ExitReadLock();
                }
            }
        }

        public void Clear()
        {
            _lock.EnterReadLock();
            try
            {
                if (_releases != null)
                {
                    _releases.Clear();
                }
            }
            finally
            {
                _lock.ExitReadLock();
            }
        }

        public bool Contains(Release item)
        {
            bool ret = false;

            _lock.EnterReadLock();
            try
            {
                if (_releases != null)
                {
                    return _releases.Contains(item);
                }
                else
                {
                    if (item != null)
                    {
                        Package p;
                        if (_packages.TryGetValue(item.Package.Name, out p))
                        {
                            ret = p.Releases.Contains(item);
                        }
                    }
                }
            }
            finally
            {
                _lock.ExitReadLock();
            }
            return ret;
        }

        public void CopyTo(Release[] array, int arrayIndex)
        {
            throw new InvalidOperationException("CopyTo currently unsupported for ReleaseCollection");
        }

        public bool Remove(Release item)
        {
            bool ret = false;

            if (item != null)
            {
                Package p;
                _lock.EnterUpgradeableReadLock();
                try
                {
                    if (_packages != null)
                    {
                        if (_packages.TryGetValue(item.Package.Name, out p))
                        {
                            _lock.EnterWriteLock();
                            try
                            {
                                ret = p.Releases.Remove(item);
                                if (p.Releases.Count == 0)
                                {
                                    _packages.TryRemove(item.Package.Name, out p);
                                }
                            }
                            finally
                            {
                                _lock.ExitWriteLock();
                            }
                        }
                    }
                    else if (_releases != null)
                    {
                        _lock.EnterWriteLock();
                        try
                        {
                            _releases.Remove(item);
                        }
                        finally
                        {
                            _lock.ExitWriteLock();
                        }
                    }
                }
                finally
                {
                    _lock.ExitUpgradeableReadLock();
                }
            }
            return ret;
        }

        private readonly ReaderWriterLockSlim _lock;
        private readonly PackageCollection _packages;
        private readonly SortedSet<Release> _releases;
    }


    public class Release : IComparable<Release>
    {
        public const string Flattened = "flattened";

        public static bool IsReleaseDirectory(DirectoryInfo directory)
        {
            bool isRelease = _processedDirectories.GetOrAddBlocking(directory.FullName, (dir) =>
                {
                    return -1 != Array.FindIndex<FileInfo>(directory.GetFiles(), (file) => { return file.Name.EndsWith(".build", StringComparison.Ordinal) || file.Name.Equals("Manifest.xml", StringComparison.OrdinalIgnoreCase); });
                });
            return isRelease;
        }

        internal static bool ClearReleaseDirectoryCache(DirectoryInfo directory)
        {
            return _processedDirectories.TryRemoveBlocking(directory.FullName);
        }

        internal static void ClearPackageDirectoryCache(DirectoryInfo releaseDirectory, int maxdepth, int depth = 0)
        {
            // We need to check 2) because a package folder can also be a package root,
            // and in that case, the release folder (unflattened) might look like a flattened package.
            if ((releaseDirectory.Attributes & (FileAttributes.Hidden | FileAttributes.System)) != 0)
                return;

            try
            {
                if (depth >= maxdepth)
                {
                    return;
                }
                ClearReleaseDirectoryCache(releaseDirectory);

                foreach (DirectoryInfo d in releaseDirectory.GetDirectories())
                {
                    ClearPackageDirectoryCache(d, maxdepth, depth + 1);
                }
            }
            catch (UnauthorizedAccessException)
            {
                // ignore the unauthorized access exception.
            }
        }


        public static bool IsValidReleaseDirectoryName(string name)
        {
            for (int i = 0; i < name.Length; i++)
            {
                var ch = name[i];
                if (!(Char.IsLetterOrDigit(ch) || ch == '_' || ch == '.' || ch == '-'))
                    return false;
            }
            return true;
        }

        public static void ParseFullPackageName(string fullPackageName, out string name, out string version)
        {
            string pattern = @"^(?<name>[\.\w]+)-(?<version>[\.\-\w]+)$";
            Match m = Regex.Match(fullPackageName, pattern);
            if (!m.Success)
            {
                string msg = String.Format("Cannot parse name and version from '{0}'.", fullPackageName);
                throw new Exception(msg);
            }

            name = m.Groups["name"].Value;
            version = m.Groups["version"].Value;
        }

        public Release(string version, string path, Package package, bool flattened)
        {
            _package = package;
            _path = path;
            IsFlattened = flattened;
            if (flattened)
            {
                _version = Manifest.Version;
                if (String.IsNullOrEmpty(_version))
                {
                    _version = Release.Flattened;
                }
            }
            else
            {
                _version = version;
            }
        }

        public Release(Release other, Package package)
        {
            _package = package;
            _path = other._path;
            IsFlattened = other.IsFlattened;
            _version = other._version;
            _manifest = other._manifest;
            _tag = other._tag;
            _packageSize = other._packageSize;
        }


        public Release(DirectoryInfo releaseDirectory, Package package)
        {
            _package = package;

            // To calculate the version we need to take the partial path from the package
            // to this directory.  Normally this is just the directory name but in the
            // version/0.2 case it might be "version/0.2".
            _version = GetVersion(releaseDirectory, package.Name);

            _path = PathNormalizer.Normalize(releaseDirectory.FullName);

            IsFlattened = false;
        }

        // Create release info out of PackageSErver
        public Release(Manifest manifest, string packageName, string version, string path)
        {
            _manifest = manifest;
            _package = new Package(packageName);
            _version = version;
            _path = path;
            IsFlattened = false; //IMTODO: are releases downloaded from the package sever always not flattened?
        }

        /// <summary>
        /// Retrieve the version of a packages based on the directory structure and name of package.
        /// </summary>
        public static string GetVersion(DirectoryInfo releaseDirectory, string packageName)
        {
            if (releaseDirectory == null || String.Compare(releaseDirectory.Name, packageName, true) == 0)
                return "";

            string ver = GetVersion(releaseDirectory.Parent, packageName);

            if (ver == String.Empty)
                return releaseDirectory.Name;
            return ver + "/" + releaseDirectory.Name;
        }


        public readonly bool IsFlattened;

        public string FullName
        {
            get { return String.Format("{0}-{1}", Package.Name, Version); }
        }

        public string Name
        {
            get { return Package.Name; }
        }

        public string Path
        {
            get { return _path; }
        }

        public string Version
        {
            get { return _version; }
        }

        public bool IsMasterRelease
        {
            get
            {
                return Package.MasterPackage != null;
            }
        }

        // Following 4 properties are for auto-building/-cleaning of dependent packages
        public bool Buildable
        {
            get
            {
                return Manifest.Buildable;
            }
        }

        public void UpdateAutoBuildCleanInfo(string autoBuildClean, string autoBuildTarget = null, string autoCleanTarget = null)
        {
            bool? newAutobuildClean = ConvertUtil.ToNullableBoolean(autoBuildClean);

            // Determine whether to auto-build or not (and whether the masterconfig settings are valid)
            if (newAutobuildClean == true)
            {
                if (FrameworkVersion == FrameworkVersions.Framework1)
                {
                    throw new BuildException("Cannot auto-build " + Name + "-" + Version + ".  It is a Framework 1.x package");
                }
                else if( FrameworkVersion == FrameworkVersions.Framework2)
                {
                    if (!Buildable)
                    {
                        throw new BuildException("Cannot auto-build " + Name + "-" + Version + ".  It is a Framework 2.x package, but it isn't buildable.");
                    }
                }
            }

            Package.MasterPackage.UpdateAutobuildClean(newAutobuildClean);

            if (autoBuildTarget != null)
            {
                Package.MasterPackage.AutoBuildTarget = autoBuildTarget;
            }

            if (autoCleanTarget != null)
            {
                Package.MasterPackage.AutoCleanTarget = autoCleanTarget;
            }
        }


        public bool AutoBuildClean(Project project)
        {
            bool ret = false;

            if (Package.MasterPackage != null)
            {
                bool? autobuildclean = Package.MasterPackage.EvaluateAutobuildClean(project);

                if (autobuildclean == null)
                {
                    if (Manifest.FrameworkVersion == FrameworkVersions.Framework2 && Buildable)
                    {
                        ret = true;
                    }
                }
                else if (autobuildclean == true)
                {
                    ret = true;

                    if (Manifest.FrameworkVersion == FrameworkVersions.Framework1)
                    {
                        project.Log.Debug.WriteLine("Cannot auto-build {0}-{1}.  It is a Framework 1.x package. AutoBuildClean flag reset to false.", Name, Version);

                        ret = false;
                    }
                    else
                    {
                        if (!Buildable)
                        {
                            project.Log.Debug.WriteLine("Cannot auto-build {0}-{1}.  It is a Framework 2.x package, but it isn't buildable. AutoBuildClean flag reset to false.", Name, Version);
                            ret = false;
                        }
                    }
                }
                else
                {
                    ret = false;
                }
            }
            // not in masterconfig
            return ret;
        }

        public string BuildGroupName(Project project)
        {
            string buildgroup = String.Empty;

            if (Package.MasterPackage != null)
            {
                MasterConfig.GroupType gt = Package.MasterPackage.EvaluateGrouptype(project);
                if (gt.Name != "masterversions")
                {
                    buildgroup = gt.Name;
                }
            }
            return buildgroup;
        }

        public string OutputMapOptionSet(Project project)
        {
            string outputMapOptionSet = String.Empty;

            if (Package.MasterPackage != null)
            {
                MasterConfig.GroupType gt = Package.MasterPackage.EvaluateGrouptype(project);

                outputMapOptionSet = gt.OutputMapOptionSet;
            }
            return outputMapOptionSet;
        }


        public string AutoBuildTarget
        {
            get
            {
                if (Package.MasterPackage != null)
                {
                    return Package.MasterPackage.AutoBuildTarget;
                }
                //FW1 package
                return String.Empty;
            }
        }

        public string AutoCleanTarget
        {
            get
            {
                if (Package.MasterPackage != null && !String.IsNullOrEmpty(Package.MasterPackage.AutoBuildTarget))
                {
                    return Package.MasterPackage.AutoBuildTarget;
                }
                //FW1 package
                return "clean";
            }
        }

        /// <summary>Gets or sets an object that contains data to associate with the release.</summary>
        public object Tag
        {
            get { return _tag; }
            set { _tag = value; }
        }

        /// <summary>Gets list of package global properties defind in masterconfig.</summary>
        public IList<string> MasterProperties
        {
            get
            {
                if (Package.MasterPackage != null)
                {
                    return Package.MasterPackage.Properties;
                }
                return new List<string>();
            }
        }

        /// <summary>True if the release has a valid manifest, otherwise false.</summary>
        public bool HasManifest
        {
            get
            {
                return Manifest != Manifest.DefaultManifest;
            }
        }

        public long GetSize()
        {
            if (_packageSize < 0)
            {
                _packageSize = Utility.GetDirectorySize(Path);
            }
            return _packageSize;
        }

        public Package Package
        {
            get { return _package; }
        }

        #region Manifest properties

        public Manifest Manifest
        {
            get
            {
                if (_manifest == null)
                {
                    _manifest = Manifest.Load(Path);
                    if (_manifest.Compatibility != null)
                    {
                        _manifest.Compatibility.SetPackageName(Package.Name);
                    }
                }
                return _manifest;
            }
        }

        // TODO:  Temporary reroute until we update Dippybird to the new Manifest interface.
		public string ManifestVersion
        {
            get
            {
                return Manifest.Version;
            }
        }

        public string OwningTeam
        {
            get
            {
                return Manifest.OwningTeam;
            }
        }

        public string DriftVersion
        {
            get 
            {
                return Manifest.DriftVersion;
            }
        }

        public string SourceLocation
        {
            get
            {
                return Manifest.SourceLocation;
            }
        }

        public string Summary
        {
            get
            {
                return Manifest.Summary;
            }
        }

        public string About
        {
            get
            {
                return Manifest.About;
            }
        }

        public string Changes
        {
            get
            {
                return Manifest.Changes;
            }
        }

        public string HomePageUrl
        {
            get
            {
                return Manifest.HomePageUrl;
            }
        }

        public string ContactName
        {
            get
            {
                return Manifest.ContactName;
            }
        }

        public string ContactEmail
        {
            get
            {
                return Manifest.ContactEmail;
            }
        }

        public Manifest.ReleaseStatus Status
        {
            get
            {
                return Manifest.Status;
            }
        }

        public string StatusComment
        {
            get
            {
                return Manifest.StatusComment;
            }
        }

        public string License
        {
            get
            {
                return Manifest.License;
            }
        }

        public string LicenseComment
        {
            get
            {
                return Manifest.LicenseComment;
            }
        }

        public string DocumentationUrl
        {
            get
            {
                return Manifest.DocumentationUrl;
            }
        }

        // For Framework 2 Compatibility Mode (Fw2CM). Might be replaced by interface suggested by
        // Gerry Shaw
        public FrameworkVersions FrameworkVersion
        {
            get
            {
                return Manifest.FrameworkVersion;
            }
        }

        public Compatibility Compatibility
        {
            get
            {
                return Manifest.Compatibility;
            }
        }


        #endregion

        public int CompareTo(Release value)
        {
            // sort package names in descending order
            int difference = String.Compare(this.Package.Name, value.Package.Name);
            if (difference == 0)
            {
                // special case version sorting
                difference = NumericalStringComparator.Compare(value.Version, this.Version);
            }
            return difference;
        }

        /// <summary>Deletes the package from disk and the package map.</summary>
        public void Delete()
        {
            DirectoryInfo dirInfo = new DirectoryInfo(Path);

            // delete this release from disk and remove it from the package map
            Utility.ObliterateDirectory(dirInfo.FullName);
            Package.Releases.Remove(this);

            if ((dirInfo.Parent != null) &&
                (Directory.GetFiles(dirInfo.Parent.FullName).Length == 0) &&
                (Directory.GetDirectories(dirInfo.Parent.FullName).Length == 0))
            {

                // if package directory is empty delete entire package as well
                Utility.ObliterateDirectory(dirInfo.Parent.FullName);

                // if package has no releases remove it from package map
                // (releases may still exist in another package root)
                if (Package.Releases.Count == 0)
                {
                    Package p;
                    PackageMap.Instance.Packages.TryRemove(Package.Name, out p);
                }
            }
        }

        public void Refresh()
        {
            _packageSize = -1;
        }

        void AddUniqueRelease(ReleaseCollection releases, Release r)
        {
            while (true)
            {
                Release existing = releases.FindByName(r.Name);
                if (existing == null)
                {
                    break;
                }
                releases.Remove(existing);
            }
            releases.Add(r);
        }


        private class Utility
        {

            /// <summary>Get the size of a directory and it's contents in bytes.</summary>
            /// <param name="path">The complete path to the directory.</param>
            /// <returns>The size in bytes of the directory and it's contents.</returns>
            public static long GetDirectorySize(string path)
            {
                try
                {
                    return GetDirectorySize(new DirectoryInfo(path));
                }
                catch
                {
                    return 0;
                }
            }

            static long GetDirectorySize(DirectoryInfo info)
            {
                long size = 0;

                foreach (FileInfo f in info.GetFiles())
                {
                    size += f.Length;
                }

                foreach (DirectoryInfo d in info.GetDirectories())
                {
                    size += GetDirectorySize(d.FullName);
                }

                return size;
            }

            /// <summary>Remove all files and directories in the given path regardless of readonly attributes.</summary>
            /// <param name="path">The path </param>
            public static void ObliterateDirectory(string path)
            {
                PathUtil.DeleteDirectory(path, verify:false);
            }
        }

        private Package _package;
        private string _path;
        private string _version;
        private Manifest _manifest = null;
        private object _tag;
        private long _packageSize = -1;


        private static ConcurrentDictionary<string, Lazy<bool>> _processedDirectories = new ConcurrentDictionary<string, Lazy<bool>>();

    }

    /// <summary>
    /// Class to store package compatibility.
    /// </summary>
    /// <remarks>
    ///  <para>One declares package compatibility in the Manifest.xml file of a package. An example may look like:
    ///   &lt;compatibility package="eacore"&gt;
    ///   &lt;api-supported&gt;
    ///   1.00.00
    ///   1.00.01
    ///   &lt;/api-supported&gt;
    ///   &lt;binary-compatible&gt;
    ///   1.00.01
    ///   &lt;/binary-compatible&gt;
    ///   &lt;dependent package="EABase"&gt;
    ///   &lt;compatible&gt;
    ///   2.0.4
    ///   2.0.5
    ///   &lt;/compatible&gt;
    ///   &lt;incompatible version="2.0.3" message="Known bug in blah blah blah" /&gt;
    ///   &lt;/dependent&gt;
    ///   &lt;/compatibility&gt;
    ///  </para>
    /// </remarks>
    public class Compatibility
    {
        #region Helper Classes
        /// <summary>
        /// Class to store &lt;incompatible&gt;
        /// </summary>
        public class Incompatible
        {
            public string Version, Message;

            public Incompatible(string version)
            {
                Version = version;
            }
        }

        /// <summary>
        /// Class to store &lt;depenent&gt;
        /// </summary>
        public class Dependent
        {
            string _packageName = null;
            string[] _compatibles = null;
            Incompatible[] _incompatibles = null;

            internal bool fw3Verified = false;

            //FW3:

            public readonly string MinRevision;
            public readonly  string MaxRevision;
            public readonly string MinVersion;
            public readonly string MaxVersion;

            public readonly bool Fail;
            public readonly string Message;

            internal Dependent(XmlNode node)
            {
                // FW3 
               _packageName = XmlUtil.GetRequiredAttribute(node, "package");
               MinRevision = XmlUtil.GetOptionalAttribute(node, "minrevision");
               MaxRevision = XmlUtil.GetOptionalAttribute(node, "maxrevision");
               MinVersion = XmlUtil.GetOptionalAttribute(node, "minversion");
               MaxVersion = XmlUtil.GetOptionalAttribute(node, "maxversion");

               Fail = ConvertUtil.ToBooleanOrDefault(XmlUtil.GetOptionalAttribute(node, "fail"), false, node);
               Message = XmlUtil.GetOptionalAttribute(node, "message");
                // End FW3

                XmlNode child = node.GetChildElementByName("compatible");
                if (child != null)
                {
                    string val = child.InnerText.Replace("\t", "").Replace("\r", "").Trim('\n');
                    _compatibles = val.Split('\n');
                }

                XmlNodeList nodes = node.SelectNodes("incompatible");
                if (nodes != null)
                {
                    _incompatibles = new Incompatible[nodes.Count];
                    for (int i = 0; i < nodes.Count; i++)
                    {
                        _incompatibles[i] = new Incompatible(nodes[i].Attributes["version"].Value);
                        if (nodes[i].Attributes["message"] != null)
                            _incompatibles[i].Message = nodes[i].Attributes["message"].Value;
                    }
                }
            }

            #region Properties
            public string PackageName
            {
                get { return _packageName; }
            }

            public string[] Compatibles
            {
                get
                {
                    return _compatibles;
                }
            }

            public Incompatible[] Incompatibles
            {
                get
                {
                    return _incompatibles;
                }
            }
            #endregion
        }
        #endregion

        #region Attributes
        //FW3
        public readonly string Revision = null;
        //END FW3
        string _packageName = null;
        string[] _apiSupported = null;
        string[] _binaryCompatible = null;
        Dependent[] _dependents = null;

        internal void SetPackageName(string name)
        {
            if (_packageName == null)
            {
                _packageName = name;
            }
        }
        #endregion

        #region Constructor and Method
        internal Compatibility(XmlNode node)
        {
            _packageName = XmlUtil.GetOptionalAttribute(node, "package");
            Revision = XmlUtil.GetOptionalAttribute(node, "revision");

            string val;

            XmlNode child = node.GetChildElementByName("api-supported");
            if (child != null)
            {
                val = child.InnerText.Replace("\t", "").Replace("\r", "").Trim('\n');
                _apiSupported = val.Split('\n');
            }

            child = node.GetChildElementByName("binary-compatible");
            if (child != null)
            {
                val = child.InnerText.Replace("\t", "").Replace("\r", "").Trim('\n');
                _binaryCompatible = val.Split('\n');
            }

            XmlNodeList nodes = node.SelectNodes("dependent");
            if (nodes != null)
            {
                _dependents = new Dependent[nodes.Count];
                for (int i = 0; i < nodes.Count; i++)
                {
                    _dependents[i] = new Dependent(nodes[i]);
                }
            }
        }


        public StringCollection CheckCompatibility(string myVersion)
        {
            StringCollection warnings = new StringCollection();
            char[] trim = " or ".ToCharArray();

            // Example:
            // eacore-1.02.00:
            //<compatibility package="eacore">
            //<dependent package="EABase">
            //<compatible>
            //  2.0.5
            //  2.0.6
            //  //2.0.7
            //</compatible>
            //<incompatible version="2.0.3" message="Known bug in blah blah blah" />
            //</dependent>
            //</compatibility>
            //
            // EABase-2.0.7
            //<compatibility package="EABase">
            //<api-supported>
            //  2.0.6
            //</api-supported>
            //</compatibility>
            // For each <dependent package>
            foreach (Compatibility.Dependent dep in Dependents)
            {
                string name = dep.PackageName;
                //IMTODO
                string version = null;
                //string version = PackageMap.Instance.GetMasterVersion(name);

                if (version == null)
                {
                    //dep wasn't in masterversions, so we can't check it
                    continue;
                }

                // 1. Check if the one listed in master config is compatible with my
                //    <compatible> versions. That is, check if dep.Compatibles contains
                //    version
                StringBuilder sb = new StringBuilder();
                if (dep.Compatibles != null)
                {
                    bool found = false;
                    foreach (string compatible in dep.Compatibles)
                    {
                        if (!Expression.Evaluate(compatible + "==" + version))
                            sb.Append(compatible + " or ");
                        else
                        {
                            // In the above example, if eacore-1.02.00 lists EABase-2.0.7, then 2.0.7 passes
                            // 1.02.00's test; otherwise, show warning of 2.0.7 isn't listed as a compatible version
                            found = true;
                            break;
                        }
                    }
                    if (found)
                    {
                        // dep.Compatibles contains version, so clear sb
                        sb.Remove(0, sb.Length);
                    }
                }
                if (sb.Length > 0)
                    warnings.Add(_packageName + "-" + myVersion + ": " + name + " " + version +
                        " isn't listed as a compatible version with " + sb.ToString().Trim(trim));

                // 2. Then check if the master one's <api-supported> has any of my <compatible> versions
                // In above example, EABase-2.0.7 has 2.0.6 in <api-supported>,
                // eacore-1.02.00 has 2.0.5 and 2.0.6 in <compatible>
                // 2.0.6 is in <api-supported> but 2.0.5 isn't, so give warning of not listing 2.0.5 as
                // compatible API
                Release info1 = PackageMap.Instance.Releases.FindByNameAndVersion(name, version);
                if (info1 != null)
                {
                    Compatibility comp1 = info1.Compatibility;
                    if (comp1 != null)
                    {
                        StringCollection coll = new StringCollection();
                        if (dep.Compatibles != null)
                        {
                            ArrayList apiSupported = new ArrayList(comp1.APISupported);
                            foreach (string compatible in dep.Compatibles)
                            {
                                if (!Expression.Evaluate(compatible + "==" + version) &&
                                    !apiSupported.Contains(compatible))
                                    coll.Add(compatible);
                            }
                        }
                        if (coll.Count > 0)
                        {
                            sb.Remove(0, sb.Length);
                            foreach (string s in coll)
                                sb.Append(s + " or ");
                            warnings.Add(_packageName + "-" + myVersion + ": " + name + " " + version + " doesn't list "
                                + sb.ToString().Trim(trim) + " as compatible API");
                        }
                    }
                }
            }
            return warnings;
        }
        #endregion

        #region Properties
        public string PackageName
        {
            get { return _packageName; }
        }

        public string[] APISupported
        {
            get
            {
                return _apiSupported;
            }
        }

        public string[] BinaryCompatible
        {
            get
            {
                return _binaryCompatible;
            }
        }

        public Dependent[] Dependents
        {
            get
            {
                return _dependents;
            }
        }
        #endregion
    }
}
