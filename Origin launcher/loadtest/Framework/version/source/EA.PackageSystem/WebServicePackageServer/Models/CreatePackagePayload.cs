using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EA.PackageSystem.PackageCore
{
    public class CreatePackagePayload
    {
        public string package_id { get; set; }
        public string name { get; set; }
        public string description { get; set; }
        public PackageUser contact { get; set; }
        public Dictionary<string, string> metadata { get; set; } = new Dictionary<string, string>();
        public IEnumerable<string> admins { get; set; } = new List<string>();
    }	
}
