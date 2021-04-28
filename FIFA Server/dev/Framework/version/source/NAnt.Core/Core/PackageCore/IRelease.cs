using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NAnt.Core.PackageCore
{
	public class ReleaseBase
	{
		/// <remarks/>
		public string Name { get; set; }

		public string Version { get; set;  }

		public string StatusName { get; set; }

		/// <remarks/>
		public int PackageId { get; set; }

		public string FileNameExtension { get; set; }
	}
}
