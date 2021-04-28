using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
    [TaskName("script-init-verify")]
    public class ScriptInitVerify : Task
    {
        private string _packageName;
        private string _outputProperty;

        [TaskAttribute("package-name", Required = true)]
        public string PackageName
        {
            get { return _packageName; }
            set { _packageName = value; }
        }
        [TaskAttribute("output-property-name", Required = false)]
        public string OutputPropertyName
        {
            get { return _outputProperty; }
            set { _outputProperty = value; }
        }

        protected override void ExecuteTask()
        {
            // Default is true, because we can't detect this on use dependencies and in some other cases;
            bool hasPackageModule = true;

            var config = Properties["config"];
            var version = Properties[String.Format("package.{0}.version", PackageName)];


            IPackage package;

            if (Project.BuildGraph().Packages.TryGetValue(Package.MakeKey(PackageName, version, config), out package))
            {
                // There is slight chance of concurrent access to modules collection. If error happens just ignore, build scripts should be cleaned so that we don't need this check at all.
                try
                {
                    // If package is buildable
                    if ((package.Modules.Count() > 0 && package.Modules.Where(m => (m is Module_UseDependency)).Count() == 0) && (package.IsKindOf(Package.FinishedLoading) && !package.Modules.Any(m => m.Name == PackageName)))
                    {
                        hasPackageModule = false;

                        if (Log.WarningLevel >= Log.WarnLevel.Advise)
                        {
                            Log.Warning.WriteLine("Processing '{0}':  Initialize.xml from package '{1}' invokes task ScriptInit without 'Libs' parameter. ScriptInit in this case exports library '{2}', but package does not have any modules with this name. Fix your Initialize.xml file.",
                                Properties["package.name"], PackageName, Project.ExpandProperties("${package." + PackageName + ".builddir}/${config}/lib/${lib-prefix}" + PackageName + "${lib-suffix}"));
                        }
                    }
                }
                catch (Exception)
                {
                }
            }

            Properties.AddLocal(OutputPropertyName, hasPackageModule.ToString().ToLowerInvariant());
        }
    }
}
