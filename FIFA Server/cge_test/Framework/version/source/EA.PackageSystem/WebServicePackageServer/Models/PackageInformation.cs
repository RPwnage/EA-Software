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
	public class PackageInformation
	{
		public string ID { get; set; }
		public string Name { get; set; }
		public string Description { get; set; }
		public PackageUser Contact { get; set; }
		public string Created_At { get; set; }
		public IEnumerable<PackageUser> Admins { get; set; }
		public PackageRelease Latest_Release { get; set; }
		public bool Editable { get; set; }
		public PackageMetaData Metadata { get; set; }

		public bool Yanked { get; set; }
	}
}
