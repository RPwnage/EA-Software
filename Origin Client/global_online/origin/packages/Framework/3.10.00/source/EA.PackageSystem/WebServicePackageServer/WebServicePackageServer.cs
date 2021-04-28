using System;
using System.IO;
using System.Net;
using System.Collections.Generic;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using EA.SharpZipLib;
using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem
{
    public class WebServicePackageServer : IPackageServer
    {
        private const int RETRY_COUNT = 3;

        public WebServicePackageServer()
        {
            _webServices = PackageServer.WebServicesFactory.Generate();
        }

        public event InstallProgressEventHandler UpdateProgress;

        public Log Log
        {
            get { return _log; }
            set { _log = value; }
        }


        public bool Break
        {
            get { return _break; }
            set { _break = value; }
        }

        public bool TryGetRelease(string name, string version, out Release release)
        {
            release = CreateRelease(_webServices.GetRelease(name, version));
            return release != null;
        }

        public bool TryGetReleaseDependents(string name, string version, out IList<Release> releases)
        {
            releases = new List<Release>();

            PackageServer.Release[] webReleases = _webServices.GetReleaseDependents(name, version);
            if (webReleases != null)
            {
                foreach (PackageServer.Release webRelease in webReleases)
                {
                    releases.Add(CreateRelease(webRelease));
                }
                return true;
            }
            return false;
        }
        public IList<Release> GetAllReleases()
        {
            List<Release> releases = new List<Release>();

            PackageServer.Release[] webReleases = _webServices.GetAllReleases();

            if (webReleases != null)
            {
                foreach (PackageServer.Release webRelease in webReleases)
                {
                    releases.Add(CreateRelease(webRelease));
                }
            }
            return releases;
        }

        public bool TryGetPackage(string name, out Package package)
        {
            package = CreatePackage(_webServices.GetPackage(name));

            return package != null;
        }

        public bool TryGetPackageReleases(string name, out IList<Release> releases)
        {
            releases = new List<Release>();

            PackageServer.Release[] webReleases = _webServices.GetPackageReleases(name);

            if (webReleases != null)
            {
                foreach (PackageServer.Release webRelease in webReleases)
                {
                    releases.Add(CreateRelease(webRelease));
                }
                return true;
            }
            return false;
        }

        public IList<Package> GetAllPackages()
        {
            List<Package> packages = new List<Package>();

            PackageServer.Package[] webPackages = _webServices.GetAllPackages();

            if (webPackages != null)
            {
                foreach (PackageServer.Package webPackage in webPackages)
                {
                    packages.Add(CreatePackage(webPackage));
                }
            }
            return packages;
        }

        public void Install(Release release, string rootDirectory)
        {
            UpdateProgressMessage(-1, String.Format("Downloading {0}-{1}{2}", release.Name, release.Version, release.Path));

            string downloadUrl = GetDownloadUrl(release);

            int retries = 0;
            do
            {
                retries++;
                try
                {
                    InstallImpl(release, rootDirectory, downloadUrl);
                    return;
                }
                catch (Exception ex)
                {
                    if (Log != null)
                    {
                        if (retries <= 1)
                        {
                            Log.Status.WriteLine("Package download failed: {0}", ex.Message);
                        }
                        else
                        {
                            Log.Status.WriteLine("retry {0}: Package download failed: {1}", retries, ex.Message);
                        }
                    }

                    if (retries >= RETRY_COUNT)
                    {
                        throw;
                    }
                }
                Thread.Sleep(1200);
            } while (retries < RETRY_COUNT);
        }

        private void InstallImpl(Release release, string rootDirectory, string downloadUrl)
        {
            string downloadPath = null;
            string packagePath = Path.Combine(rootDirectory, release.Name, release.Version);
            // I use path to store FileNameExtension:
            if (release.Path == ".msi" || release.Path == ".exe")
            {
                // Download the file
                downloadPath = packagePath;
                Directory.CreateDirectory(downloadPath);
                string downloadFileName = Path.GetFileName(downloadUrl);
                downloadPath = downloadPath + "/" + downloadFileName;
                if (File.Exists(downloadPath))
                {
                    string msg = String.Format("\nDownload error: {0} already exists in the release folder.\n", downloadFileName);
                    throw new ApplicationException(msg);
                }
                DownloadFile(downloadUrl, downloadPath);
                return;
            }
            else if (release.Path == ".zip")
            {
                // get a temp file
                downloadPath = Path.GetTempFileName();
            }
            else
            {
                string msg = String.Format("Cannot install non-zip file package.");
                throw new ApplicationException(msg);
            }
            
            // Download the file
            DownloadFile(downloadUrl, downloadPath);

            try
            {
                UpdateProgressMessage(-1, String.Format("Unzipping {0}-{1} to '{2}'", release.Name, release.Version, packagePath));

                if (Directory.Exists(packagePath))
                {
                    UpdateProgressMessage(-1, String.Format("Clean package {0}-{1} directory '{2}'", release.Name, release.Version, packagePath));
                    PathUtil.DeleteDirectory(packagePath, failOnError:false);
                }

                ZipLib zipLib = new ZipLib();
                zipLib.ZipEvent += new ZipEventHandler(ZipEventCallback);
                zipLib.UnzipFile(downloadPath, rootDirectory);
                zipLib.ZipEvent -= new ZipEventHandler(ZipEventCallback);
            }
            catch (Exception e)
            {
                // delete partial package directory
                try
                {
                    PathUtil.DeleteDirectory(packagePath);
                }
                catch(Exception delEx)
                {
                    if (Log != null)
                    {
                        Log.Warning.WriteLine("Failed to delete package directory '{0}' after package download failed, directory  may be in inconsistent state. Reason: {1}", packagePath, delEx.Message);
                    }
                }
                if (e is BreakException)
                {
                    throw;
                }
                string msg = String.Format("The package could not be unzipped because: {0}", e.Message);
                throw new ApplicationException(msg);
            }
            finally
            {
                File.Delete(downloadPath);
            }
        }

        private void DownloadFile(string downloadUrl, string downloadPath)
        {
            FileStream fileStream = null;
            Stream downloadStream = null;

            try
            {
                const int bufferSize = 8192;
                byte[] buffer = new Byte[bufferSize];

                WebClient web = new WebClient();
                web.Credentials = System.Net.CredentialCache.DefaultCredentials;
                downloadStream = web.OpenRead(downloadUrl);

                fileStream = File.Create(downloadPath, bufferSize);
                // get content-length HTTP header
                ulong totalSize;
                try
                {
                    totalSize = Convert.ToUInt64(web.ResponseHeaders["content-length"]);
                }
                catch
                {
                    totalSize = 0;
                }

                ulong totalDownloaded = 0;

                while (true)
                {
                    int bytesRead = downloadStream.Read(buffer, 0, bufferSize);
                    if (bytesRead < 0)
                    {
                        throw new Exception("A network error occured during file download");
                    }
                    if (bytesRead == 0)
                    {
                        break;
                    }
                    fileStream.Write(buffer, 0, bytesRead);

                    totalDownloaded += (ulong)bytesRead;

                    // the * 50 is because downloading is only 50% of the work of installing a package
                    if (totalSize > 0)
                    {
                        UpdateProgressMessage((int)(totalDownloaded * 100 / totalSize));
                    }
                }

                downloadStream.Close();
                fileStream.Close();

            }
            catch (Exception e)
            {
                if (downloadStream != null)
                {
                    downloadStream.Close();
                }

                if (fileStream != null)
                {
                    fileStream.Close();
                    try
                    {
                        File.Delete(downloadPath);
                    }
                    catch
                    {
                    }
                }

                if (e is BreakException)
                {
                    throw;
                }

                string msg = String.Format("The package could not be downloaded because: {0}", e.Message);

                if (e is WebException)
                {
                    WebException wex = e as WebException;
                    msg = String.Format("The package could not be downloaded because of network error: {0}\n{1}\n", Enum.GetName(typeof(WebExceptionStatus), wex.Status), wex.Message);
                    throw new WebException(msg, wex);
                }

                throw new ApplicationException(msg);
            }
        }

        private void UpdateProgressMessage(int progress, string message = null)
        {
            if (Break)
            {
                throw new BreakException();
            }


            //int progress = (_currentReleaseIndex * 100 + _localProgress + 1) / _releases.Count;
            if (UpdateProgress != null)
            {
                if (message == null)
                {
                    message = _progressMessage;
                }
                else
                {
                    _progressMessage = message;
                }

                Release release = null;
                /*
                if (_currentReleaseIndex >= 0 && _currentReleaseIndex < _releases.Count)
                {
                    release = (PackageServer.Release)Releases[_currentReleaseIndex];
                }
                 */
                UpdateProgress(this, new InstallProgressEventArgs(progress, progress, message, release));
            }

        }

        private string GetDownloadUrl(Release release)
        {
            string webUrl = "http://packages.worldwide.ea.com";
            string webServices2URL = PackageServer.WebServices2URL.GetWebServices2URL();
            if (webServices2URL != null)
            {
                Uri uri = new Uri(webServices2URL);
                webUrl = "http://" + uri.Host;
            }
            return String.Format("{0}/packages/{1}/{1}-{2}{3}",
                webUrl, release.Name, release.Version, release.Path);
        }

        private void ZipEventCallback(object source, ZipEventArgs e)
        {
            if (e.EntryCount != 0)
            {
                UpdateProgressMessage(e.EntryIndex * 100 / e.EntryCount, String.Format("Unzipping {0}", e.ZipEntry.Name));
            }
        }



        private Release CreateRelease(PackageServer.Release webRelease)
        {
            if (webRelease == null)
            {
                return null;
            }
            FrameworkVersions frameworkVersion = (FrameworkVersions)webRelease.FrameworkVersion;
            Manifest.ReleaseStatus status;
            if (!Enum.TryParse<Manifest.ReleaseStatus>(StringUtil.Trim(webRelease.StatusName), true, out status))
            {
                status = Manifest.ReleaseStatus.Unknown;
            }

            bool buildable = false; //unknown

            Manifest manifest = new Manifest(frameworkVersion, buildable, webRelease.ContactName, webRelease.ContactEmail,
                         webRelease.Summary, webRelease.Description, ""/*about*/, webRelease.Changes, webRelease.ReleaseUrl, status, webRelease.StatusComment,
                         webRelease.LicenseName, webRelease.LicenseComment, webRelease.DocumentationUrl, webRelease.Version, null,
                //Version2 IMTODO:
                //string owningTeam, string sourceLocation, string driftVersion, string tags, string community, bool isSupported, string platformsInCode,
                //string containsOpenSource, string licenseType, bool internalOnly, IEnumerable<Platform> platforms, IEnumerable<Build> builds
                         String.Empty, String.Empty, String.Empty, String.Empty, String.Empty, true, String.Empty,
                         String.Empty, String.Empty, false, null, null);

            Release release = new Release(manifest, webRelease.Name, webRelease.Version, webRelease.FileNameExtension);

            return release;
        }

        private Package CreatePackage(PackageServer.Package webPackage)
        {
            Package package = null;
            if (webPackage != null)
            {
                package = new Package(webPackage.Name);
            }
            return package;
        }


        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing && _webServices != null)
                {
                    // Do I need to do any cleanuu?
                    _webServices.Dispose();
                    _webServices = null;
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;
        private bool _break = false;
        private Log _log;
        private string _progressMessage = String.Empty;
        private PackageServer.WebServices _webServices;
    }
}
