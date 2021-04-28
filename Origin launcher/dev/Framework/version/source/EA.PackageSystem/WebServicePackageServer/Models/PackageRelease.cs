using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NAnt.Core.PackageCore;

namespace EA.PackageSystem.PackageCore
{
	/// <summary>
	/// Model for Package Server v2
	/// </summary>
	public class PackageRelease : ReleaseBase
	{
		public string Changes { get; set; }
		public string CreatedAt { get; set; }
		public string Status { get; set; }
		public PackageReleaseMetadata Metadata { get; set; }
		public bool Yanked { get; set; }
		public bool Editable { get; set; }
		public PackageUser Author { get; set; }
	}
}
