using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules
{
     
    public class Module_MakeStyle : ProcessableModule
    {
        internal Module_MakeStyle(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildtype, IPackage package)
            : base(name, groupname, groupsourcedir, configuration, buildgroup, buildtype, package)
        {
            SetType(Module.MakeStyle);
        }

        public Module_MakeStyle(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, IPackage package)
            : this(name, groupname, groupsourcedir, configuration, buildgroup, new BuildType("MakeStyle", "MakeStyle", String.Empty, false, true), package)
        {
        }

        public override void Apply(IModuleProcessor processor)
        {
            processor.Process(this);
        }

        public PathString OutputFile;
    }
}
