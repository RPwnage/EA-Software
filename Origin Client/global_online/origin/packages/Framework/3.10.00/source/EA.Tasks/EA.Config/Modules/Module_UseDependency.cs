using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules
{

    public class Module_UseDependency : ProcessableModule
    {
        public static readonly string PACKAGE_DEPENDENCY_NAME = "package-use-dependency";

        internal Module_UseDependency(string name, Configuration configuration, BuildGroups buildgroup, string subsystem, IPackage package)
            : base(name, String.Empty, String.Empty, configuration, buildgroup, buildtype(subsystem), package, isreal: false)
        {
        }

        public override void Apply(IModuleProcessor processor)
        {
            processor.Process(this);
        }


        private static BuildType buildtype(string subsystem)
        {
            return new BuildType("UseDependency", "UseDependency", subsystem, false, false);
        }
    }

}

