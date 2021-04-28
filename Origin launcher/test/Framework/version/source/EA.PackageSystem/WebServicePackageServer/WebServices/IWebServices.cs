using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NAnt.Core.PackageCore;

namespace EA.PackageSystem.PackageCore.Services
{
	public interface IWebServices : IDisposable 
	{
		Task<ReleaseBase> GetReleaseV2(string name, string version);

		Task<ReleaseBase[]> GetPackageReleasesV2(string name);

		/// <summary>
		/// Attempts to post the package at the filename location
		/// </summary>
		/// <param name="fileName"></param>
		/// <param name="uncName"></param>
		/// <param name="changes"></param>
		/// <param name="statusId"></param>
		/// <param name="statusComment"></param>
		/// <param name="requiredReleases"></param>
		/// <param name="account"></param>
		/// <returns>Any Errors received while posting</returns>
		string PostPackageV2OrGetError(string fileName, string uncName, string changes, int statusId, string statusComment, string requiredReleases, string account);

		/// <summary>
		/// Get the Download URL from the server
		/// </summary>
		/// <param name="name"></param>
		/// <param name="version"></param>
		/// <returns></returns>
		Task<string> GetDownloadUrl(string name, string version);

		bool UsingNewPackageServer();

		/// <summary>
		/// Creates a new package
		/// </summary>
		/// <param name="packageName"></param>
		/// <param name="manifestContents"></param>
		/// <returns></returns>
		bool CreateNewPackage(string packageName, string manifestContents);

		/// <summary>
		/// Edits the status of a package release
		/// </summary>
		/// <param name="package">the package to edit release status of</param>
		/// <param name="release">the release to edit status of</param>
		/// <param name="status">the new status</param>
		/// <returns></returns>
		bool EditPackageStatus(string package, string release, string status);
	}
}
