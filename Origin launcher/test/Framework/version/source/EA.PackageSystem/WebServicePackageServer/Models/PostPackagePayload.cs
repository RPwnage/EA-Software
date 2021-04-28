using System;
using System.Collections.Generic;

namespace EA.PackageSystem.PackageCore
{
	public class PostPackagePayload
	{
		public string version { get; set; }
		public string changes { get; set; }
		public PackageUser author { get; set; }
		public Dictionary<string, string> metadata { get; set; } = new Dictionary<string, string>();
		public string status_name { get; set; }
		public string description { get; set; }

	}
}
