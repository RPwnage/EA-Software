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
     
    public class Module_ExternalVisualStudio : ProcessableModule
    {
        internal Module_ExternalVisualStudio(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildtype, IPackage package)
            : base(name, groupname, groupsourcedir, configuration, buildgroup, buildtype, package)
        {
            SetType(Module.ExternalVisualStudio);
        }

        public Module_ExternalVisualStudio(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, IPackage package)
            : this(name, groupname, groupsourcedir, configuration, buildgroup, new BuildType("MakeStyle", "MakeStyle", String.Empty, false, true), package)
        {
        }

        public override void Apply(IModuleProcessor processor)
        {
            processor.Process(this);
        }

        public bool IsProjectType(DotNetProjectTypes test)
        {
            // using bitwise & operator to test a single bit of the bitmask
            return (DotNetProjectTypes & test) == test;
        }

        public PathString VisualStudioProject;
        public string VisualStudioProjectConfig;
        public string VisualStudioProjectPlatform;
        public Guid VisualStudioProjectGuild;
        public DotNetProjectTypes DotNetProjectTypes = 0;
    }
}

