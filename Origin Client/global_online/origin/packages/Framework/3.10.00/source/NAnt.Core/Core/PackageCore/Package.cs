// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;
using System.Linq;
using System.Collections.Concurrent;
using System.IO;
using System.Threading.Tasks;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Xml;
using System.Collections.Generic;

using NAnt.Core;

namespace NAnt.Core.PackageCore
{

    public class PackageCollection : ConcurrentDictionary<string, Package>
    {
        public Package FindByName(string packageName)
        {
            Package p;
            if (!TryGetValue(packageName, out p))
            {
                p = null;
            }
            return p;
        }

        //IMTODO : do I need to add case insensitive search?
        public Package FindByName(string packageName, bool ignoreCase)
        {
            return FindByName(packageName);
        }

        public IEnumerable<Package> InOrder { get { return Values.OrderBy(p => p.Name); } }
    }


    public class Package : IComparable<Package>
    {
        public readonly string Name;

        public readonly ReleaseCollection Releases = new ReleaseCollection();

        internal readonly MasterConfig.Package MasterPackage;

        /// <summary>Gets or sets an object that contains data to associate with the package.</summary>
        public object Tag
        {
            get { return _tag; }
            set { _tag = value; }
        } private object _tag;

        internal Package(PackageMap packageMapInstance, DirectoryInfo packageDirectory, int maxdepth, MasterConfig.Package masterPackage)
        {
            Name = packageDirectory.Name;
            MasterPackage = masterPackage;

            if (maxdepth < 0)
            {
                throw new BuildException("Maxdepth can't negative: maxdepth=" + maxdepth);
            }

            ProcessReleaseDirectory(packageMapInstance, packageDirectory, 0, maxdepth);
        }

        /// <summary>
        /// creates a package object WITHOUT scanning and adding releases automatically.        /// </summary>
        public Package(string packageName)
        {
            Name = packageName;
            MasterPackage = null;
        }

        private void ProcessReleaseDirectory(PackageMap packageMapInstance, DirectoryInfo releaseDirectory, int depth, int maxdepth)
        {
            // The package directory is a flattened package release directory if
            // 1) it contains a manifest inside the package directory, and
            // 2) a release with the same path has not been added yet

            // We need to check 2) because a package folder can also be a package root,
            // and in that case, the release folder (unflattened) might look like a flattened package.
            if ((releaseDirectory.Attributes & (FileAttributes.Hidden | FileAttributes.System)) != 0)
                return;

            try
            {
                if (depth == 0 &&
                    Release.IsReleaseDirectory(releaseDirectory))
                {
                    // amcdonald@ea.com : If there is an existing release stored for this directory, make sure it isn't for another package, 
                    // as it can get confused with nested package roots, thinking that a package in a nested package root is 
                    //just a version of a package in the parent package root.
                    Release release = packageMapInstance.Releases.FindByDirectory(releaseDirectory.FullName);

                    if (release == null || release.Version == this.Name)
                    {
                        Release r = new Release(Release.Flattened, releaseDirectory.FullName, this, flattened:true);
                        if (r.Version != null && !String.IsNullOrEmpty(r.Version))
                        {
                            // found <version> tag in Manifest for this flat package.
                            Releases.Add(r);
                        }
                    }
                }

                // Limit depth to prevent needless searching.  See bug 449 for more details.
                // http://framework.eac.ad.ea.com/bugzilla/show_bug.cgi?id=449
                if (depth >= maxdepth)
                {
                    return;
                }

                foreach (DirectoryInfo d in releaseDirectory.GetDirectories())
                {
                    // make sure the directory is even a candidate for a release directory by checking the name
                    if (Release.IsValidReleaseDirectoryName(d.Name))
                    {
                        if (Release.IsReleaseDirectory(d))
                        {
                            // found a release directory, add it to the collection
                            Release r = new Release(d, this);
                            Releases.Add(r);
                        }
                        else
                        {
                            // maybe a release lives in a subdirectory (ie, the version/0.2 case)
                            ProcessReleaseDirectory(packageMapInstance, d, depth + 1, maxdepth);
                        }
                    }
                }
            }
            catch (UnauthorizedAccessException)
            {
                // ignore the unauthorized access exception.
            }
        }

        public int CompareTo(Package other)
        {
            return String.Compare(Name, other.Name, StringComparison.Ordinal);
        }
    }

}
