using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;


namespace EA.Eaconfig.Modules.Tools
{
    public class AppPackageTool : Tool
    {
        public AppPackageTool(string appname, PathString toolexe, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
            : base("apppackage", toolexe, Tool.AppPackage, "apppackage", env: env,  createdirectories:createdirectories)
        {
            AppName = appname;
            AppManifests = new FileSet();
            AppImages = new FileSet();
            AppConfigs = new FileSet();
        }
        
        // Generation properties - Used in VS solution generation
        public readonly FileSet AppManifests;
        public readonly FileSet AppImages;
        public readonly FileSet AppConfigs;  
        public readonly string  AppName;
        public PathString  OutputDir;

        private static IDictionary<string, string> Environment(Project project)
        {
            var optionset = project.NamedOptionSets["build.env"];
            return optionset == null ? null : optionset.Options;
        }
    }
}
