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
	public class PackageReleaseMetadata
	{
		public string Legacy_Version { get; set; }
		public string License_Name { get; set; }
		public string Documentation_Url { get; set; }
		public string Legacy_Package_Id { get; set; }
		public string Legacy_Release_Id { get; set; }
		public string Licence_Type { get; set; }
		public bool Contains_Open_Source { get; set; }
		public string Install_URL { get; set; }
		public string File_Name_Extension { get; set; }
		public string Status_Comment { get; set; }
		public string Framework_Version { get; set; }
		public string Internal_Only { get; set; }
	}
}
