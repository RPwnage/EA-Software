namespace EA.PackageSystem.PackageCore
{
	/// <summary>
	/// Model for Package Server v2 api
	/// </summary>
	public class PackageUploadPayloadResponse : PackageDownloadPayloadResponse
	{
		public string storage_key { get; set; }
		public string user { get; set; }
	}

	/// <summary>
	/// Model for Package Server v2 api
	/// </summary>
	public class PackageDownloadPayloadResponse
	{
		public string url { get; set; }

		public string storage_name { get; set; }
	}
}
