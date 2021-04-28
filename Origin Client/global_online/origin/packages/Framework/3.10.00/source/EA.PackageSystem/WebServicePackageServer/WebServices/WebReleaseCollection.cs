// Copyright (C) Electronic Arts Canada Inc. 2003.  All rights reserved.

using System;
using System.Collections;
using System.Net;

using PackageServer = EA.PackageSystem.PackageCore.Services;
using NAnt.Core.PackageCore;

namespace EA.PackageSystem.PackageCore.Services
{
    /*
    public class WebReleaseCollection : ArrayList
    {

        public void Add(string name, string version)
        {
            Add(name, version, false);
        }

        /// <summary>Add a release to the collection.</summary>
        /// <param name="name">Name of package.</param>
        /// <param name="version">Version of package.</param>
        /// <param name="addDependents">If <c>true</c> add release dependents as well, otherwise only add the release.</param>
        public void Add(string name, string version, bool addDependents)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }
            if (version == null)
            {
                throw new ArgumentNullException("version");
            }

            try
            {
                PackageServer.WebServices server = PackageServer.WebServicesFactory.Generate();

                Release release = server.GetRelease(name, version);
                if (release == null)
                {
                    string msg = String.Format("Package '{0}-{1}' not found on package server.", name, version);
                    msg += String.Format("\n\nIf package '{0}-{1}' already exists in your package roots, make sure it is either under its version folder, or it has a <version> tag inside its manifest.", name, version);
                    msg += String.Format("\n\n<packageroots> is not defined in your master config file. Packages outside {0} would not be recognized.", PackageMap.Instance.PackageRoots.FrameworkRoot.FullName);
                    throw new ApplicationException(msg);
                }

                // install dependents first so that if they fail the main package won't be installed
                if (addDependents)
                {
                    Release[] dependents = server.GetReleaseDependents(release.Name, release.Version);
                    if (dependents != null)
                    {
                        foreach (Release dependent in dependents)
                        {
                            Add(dependent);
                        }
                    }
                }
                Add(release);

            }   
            catch (WebException e)
            {
                String msg = String.Format("The package could not be downloaded because of network error: {0}\n{1}\n", Enum.GetName(typeof(WebExceptionStatus), e.Status), e.Message);
                throw new WebException(msg, e);
            }
        }
    }
     */
}
