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

using EA.SharpZipLib;
using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Shared.Properties;
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Threading;
using NAnt.Authentication;
using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem
{
	public sealed class WebServicePackageServer : IPackageServerV2
	{
		private class TimeoutOverrideWebClient : WebClient
		{
			private readonly int m_requestTimeoutParam;

			public TimeoutOverrideWebClient()
			{
				int requestTimeout = DEFAULT_WEB_REQUEST_TIMEOUT;
				try
				{
					// We're hiding this in a try catch block because the following call
					// is trying to get the value from app.config.  If this value wasn't
					// specified in this config file, we want to catch this exception and
					// just use the default timeout value.
					string tm = System.Configuration.ConfigurationManager.AppSettings["WebRequestTimeoutMs"];
					if (tm != null)
					{
						int newTimeout = Convert.ToInt32(tm);
						requestTimeout = newTimeout;
					}
				}
				catch
				{
				}
				m_requestTimeoutParam = requestTimeout;
			}

			protected override WebRequest GetWebRequest(Uri address)
			{
				WebRequest wr = base.GetWebRequest(address);
				// Default timeout is 100000 (ie 100 sec).  We're going to increase the time out
				// as we notice that if we have too much parallel process trying
				// to do download from package server, we could get a timeout exception with
				// web.OpenRead().
				wr.Timeout = m_requestTimeoutParam;
				return wr;
			}
		}

		public event InstallProgressEventHandler UpdateProgress;

		public Project Project { get; set; }

		private string m_progressMessage = String.Empty;
		private PackageServer.IWebServices m_webServices;
		private string m_credentialFile;

		private const int RETRY_COUNT = 3;

		// The original .Net default timeout value is 100000.  However, we've also set nant.app.config
		// to 200000.
		private const int DEFAULT_WEB_REQUEST_TIMEOUT = 150000;

		public void Init(Project project)
		{
			Project = project;

			m_credentialFile = project.Properties[FrameworkProperties.CredStoreFilePropertyName];
			m_webServices = PackageServer.WebServicesFactory.Generate(project.Log, m_credentialFile);
		}

		public void Init(Project project, bool useV1 = false, string token = null)
		{
			Project = project;

			m_credentialFile = project.Properties[FrameworkProperties.CredStoreFilePropertyName];
			m_webServices = PackageServer.WebServicesFactory.Generate(project.Log, m_credentialFile, useV1, token);
		}

		public bool TryGetPackageReleases(string name, out IList<MasterConfig.IPackage> releases)
		{
			releases = new List<MasterConfig.IPackage>();

			ReleaseBase[] webReleases = m_webServices.GetPackageReleasesV2(name).Result;

			if (webReleases != null)
			{
				foreach (var webRelease in webReleases)
				{
					if (!webRelease.FileNameExtension.EndsWith(".zip", StringComparison.OrdinalIgnoreCase))
					{
						releases.Add(new PackageMap.PackageSpec(webRelease.Name, webRelease.Version));
					}
				}
				return true;
			}
			return false;
		}

		public Release Install(MasterConfig.IPackage packageSpec, string rootDirectory)
		{
			var retryintervalSeconds = TimeSpan.FromSeconds(1);
			UpdateProgressMessage(-1, String.Format("Downloading {0}-{1}{2}", packageSpec.Name, packageSpec.Version, rootDirectory));

			/*
			
			DISABLING THIS WHOLE CHECK FOR NOW - results in an extra api call to the actual package server api (not the download url), which 
			seems to fail intermittent if hit too frequently. Can re-enable this when we have a better package srever.
			 
				// error handling is a bit funky here - we null check this because we will get a null back if package does not exist - 
				// but we don't handle that immediately, we fall back to the catch down below
				// TODO: clean up error handling
				PackageServer.Release webServerRelease = m_webServices.GetRelease(packageSpec.Name, packageSpec.Version);
				if (webServerRelease != null && !webServerRelease.FileNameExtension.EndsWith(".zip", StringComparison.OrdinalIgnoreCase))
				{
					throw new BuildException($"Attempted to download manifest only package {packageSpec.Name}-{packageSpec.Version}. Framework can only download .zip packages from web package server.");
				}
			*/
			int retries = 0;
			string downloadUrl = "";
			do
			{
				retries++;
				try
				{
					downloadUrl = m_webServices.GetDownloadUrl(packageSpec.Name, packageSpec.Version).Result;
				}
				catch (Exception ex)
				{
					Project.Log.Warning.Write(ex.Message);
				}
				if (!String.IsNullOrEmpty(downloadUrl))
				{
					break;
				}
				else
				{
					Project.Log.Warning.Write($"Failed to get download url for  {packageSpec.Name}-{packageSpec.Version}, retrying in {retryintervalSeconds.Seconds} seconds");
				}
				Thread.Sleep(retryintervalSeconds);
			}
			while (retries < RETRY_COUNT);
			if (String.IsNullOrEmpty(downloadUrl))
			{
				throw new Exception($"Failed to get download URL for {packageSpec.Name}-{packageSpec.Version}");
			}
			Project.Log.Minimal.WriteLine("Downloading EA package from {0}", downloadUrl);

			retries = 0;
			do
			{
				retries++;
				try
				{
					return InstallImpl(packageSpec, rootDirectory, downloadUrl);
				}
				catch (Exception ex)
				{
					// Errors like 404 (package not found) or 403 (no access)
					// are often unresponsive to retries, so bail out if one of
					// these is encountered.
					//
					// Unfortunately the exception is sometimes buried in a
					// WebException's inner WebException so it requires
					// descending a hierarchy of contained exceptions to dig
					// it out.

					if (ex is WebException webException && webException.Status == WebExceptionStatus.ProtocolError)
					{
						if (webException.Response is HttpWebResponse webResponse)
						{
							if (webResponse.StatusCode == HttpStatusCode.NotFound
								|| webResponse.StatusCode == HttpStatusCode.Forbidden)
							{
								throw;
							}
						}
					}

					if (retries <= 1)
					{
						Project.Log.Minimal.WriteLine("Package download failed: {0}", ex.Message);
					}
					else
					{
						Project.Log.Status.WriteLine("retry {0}: Package download failed: {1}", retries, ex.Message);
					}

					if (retries >= RETRY_COUNT)
					{
						throw;
					}
				}
				Thread.Sleep(retryintervalSeconds);
			}
			while (retries < RETRY_COUNT);

			// theoretically unreachable, but compiler cannot detect this so for paranoia's
			// sake throw a sensibl-ish exception
			throw new InvalidOperationException("Web package server exceeded retry limit.");
		}

		public void Update(Release release, Uri ondemandUri, string rootDirectory)
		{
			throw new NotImplementedException("Web package server does not support updating releases.");
		}

		public bool IsPackageServerRelease(Release release, string rootDirectory)
		{
			throw new NotImplementedException("Web package server does not support updating releases.");
		}

		private Release InstallImpl(MasterConfig.IPackage packageSpec, string rootDirectory, string downloadUrl)
		{
			string downloadPath = null;
			string packagePath = Path.Combine(rootDirectory, packageSpec.Name, packageSpec.Version);

			// get a temp file
			downloadPath = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());

			// Download the file
			DownloadFile(downloadUrl, downloadPath);

			try
			{
				int pid = System.Diagnostics.Process.GetCurrentProcess().Id;
				string tempUnzipRoot = Path.Combine(rootDirectory, ".web");
				string tempUnzipRootPid = Path.Combine(tempUnzipRoot, pid.ToString());
				string tempUnzipRootPkgName = Path.Combine(tempUnzipRootPid, packageSpec.Name);
				string tempUnzipPkgPath = Path.Combine(tempUnzipRootPkgName, packageSpec.Version);
				if (!Directory.Exists(tempUnzipRoot))
				{
					Directory.CreateDirectory(tempUnzipRoot);
				}

				UpdateProgressMessage(-1, String.Format("Unzipping {0}-{1} to '{2}'", packageSpec.Name, packageSpec.Version, tempUnzipPkgPath));

				if (Directory.Exists(tempUnzipPkgPath))
				{
					UpdateProgressMessage(-1, String.Format("Clean package {0}-{1} unzip directory '{2}'", packageSpec.Name, packageSpec.Version, tempUnzipPkgPath));
					PathUtil.DeleteDirectory(tempUnzipPkgPath, failOnError: false);
				}

				ZipLib zipLib = new ZipLib();
				zipLib.ZipEvent += new ZipEventHandler(ZipEventCallback);
				zipLib.UnzipFile(downloadPath, tempUnzipRootPid);
				zipLib.ZipEvent -= new ZipEventHandler(ZipEventCallback);

				UpdateProgressMessage(-1, String.Format("Moving unzipped {0}-{1} to '{2}'", packageSpec.Name, packageSpec.Version, packagePath));

				if (Directory.Exists(packagePath))
				{
					PathUtil.DeleteDirectory(packagePath);
				}

				PathUtil.MoveDirectory(tempUnzipPkgPath, packagePath);


				// Temp directory cleanup.
				try
				{
					// Exception will be thrown if directory is not empty or has permission issues.
					Directory.Delete(tempUnzipRootPkgName);
					Directory.Delete(tempUnzipRootPid);
					Directory.Delete(tempUnzipRoot);
				}
				catch
				{
				}

				return new Release(packageSpec, packagePath, isFlattened: false, Manifest.Load(packagePath));
			}
			catch (Exception e)
			{
				// delete partial package directory
				try
				{
					PathUtil.DeleteDirectory(packagePath);
				}
				catch (Exception delEx)
				{
					Project.Log.Warning.WriteLine("Failed to delete package directory '{0}' after package download failed, directory  may be in inconsistent state. Reason: {1}", packagePath, delEx.Message);
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

			const int bufferSize = 8192;
			byte[] buffer = new Byte[bufferSize];
			var genericCredential = PackageServer.WebServicesFactory.GetGenericCredential(Project.Log, m_credentialFile);

			using (TimeoutOverrideWebClient web = new TimeoutOverrideWebClient
			{
				Credentials = PackageServer.WebServicesFactory.GetCredentials(Project.Log, genericCredential)
			})
			{
				try
				{
					downloadStream = web.OpenRead(downloadUrl);

					Directory.CreateDirectory(Path.GetDirectoryName(downloadPath));
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

					if (e is BreakException || e is WebException)
					{
						throw;
					}

					string msg = String.Format("The package could not be downloaded because: {0}", e.Message);
					throw new ApplicationException(msg);
				}
			}
		}

		private void UpdateProgressMessage(int progress, string message = null)
		{
			if (UpdateProgress != null)
			{
				if (message == null)
				{
					message = m_progressMessage;
				}
				else
				{
					m_progressMessage = message;
				}

				Release release = null;
				UpdateProgress(this, new InstallProgressEventArgs(progress, progress, message, release));
			}

		}

		private void ZipEventCallback(object source, ZipEventArgs e)
		{
			if (e.EntryCount != 0)
			{
				UpdateProgressMessage(e.EntryIndex * 100 / e.EntryCount, String.Format("Unzipping {0}", e.ZipEntry.Name));
			}
		}

		public void Dispose()
		{
			if (m_webServices != null)
			{
				m_webServices.Dispose();
				m_webServices = null;
			}
		}


	}
}
