using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EA.PackageSystem
{
	public class PackageServerRestV2Api
	{
		private const string ALL_PACKAGES = "packages";
		private const string RELEASES = "releases";
		private const string DOWNLOAD_URL = "download_url";
		private const string UPLOAD_URL = "upload_url";
		private readonly string _url;


		public enum StorageDestination 
		{
			ARTIFACTORY
		}


		/// <summary>
		/// Class for generating urls for package server rest api
		/// </summary>
		/// <param name="url"> default is https://packages.ea.com/api/v2/ </param>
		public PackageServerRestV2Api(string url = "https://packages.ea.com/api/v2/")
		{
			if (!url.EndsWith("/"))
			{
				url = url + '/';
			}
			_url = url;
		}

		public string GetPackages()
		{
			return $"{_url}{ALL_PACKAGES}/";
		}

		public string GetPackage(string packageName)
		{
			return $"{_url}{ALL_PACKAGES}/{packageName.ToLower()}/";
		}

		public string GetPackageReleases(string packageName, bool getAll)
		{
			return $"{_url}{ALL_PACKAGES}/{packageName.ToLower()}/{RELEASES}/{(getAll ? "?limit=0" : "")}";
		}

		public string GetPackageRelease(string packageName, string releaseVersion)
		{
			return $"{_url}{ALL_PACKAGES}/{packageName.ToLower()}/{RELEASES}/{releaseVersion}/";
		}

		public string GetDownloadUrl(string packageName, string releaseVersion)
		{
			return $"{_url}{ALL_PACKAGES}/{packageName.ToLower()}/{DOWNLOAD_URL}/{releaseVersion}/";
		}
		
		public string GetUploadUrl(string packageName, string releaseVersion)
		{
			return $"{_url}{ALL_PACKAGES}/{packageName.ToLower()}/{UPLOAD_URL}/{releaseVersion}/";
		}
	}
}
