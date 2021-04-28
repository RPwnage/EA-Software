using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules
{

    public class Module_Utility : ProcessableModule
    {
        internal Module_Utility(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package)
            : base(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package)
        {
            SetType(Module.Utility);
        }

        public override void Apply(IModuleProcessor processor)
        {
            processor.Process(this);
        }

    }

}
