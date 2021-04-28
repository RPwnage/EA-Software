using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EA.PackageSystem.PackageCore
{
	/// <summary>
	/// Model for Package Server v2
	/// </summary>
	public class PackageMetaData
	{
		public string HomePageUrl { get; set; }
		public string LegacyPackageId { get; set; }
		public string Tags { get; set; }
	}
}
