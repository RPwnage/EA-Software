using System;
using System.Linq;
using System.Collections.Generic;
using System.Collections.Concurrent;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Core
{
    public abstract class ProcessableModule : Module, IProcessableModule
    {
        protected ProcessableModule(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package, bool isreal=true)
            : base (name, groupname, groupsourcedir, configuration, buildgroup, package, isreal)
        {
            BuildType = buildType;
            AdditionalData = new ModuleAdditionalData();

            if (package != null && package.Project != null)
            {
                ProcessModulleShouldWaitForDependents = package.Project.Properties.GetBooleanPropertyOrDefault(GroupName + ".processing.wait-for-dependents", 
                    package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.processing.wait-for-dependents", true));
            }
        }

        public int GraphOrder
        {
            get { return _graphOrder;  }
            internal set { _graphOrder = value;  }
        }
        int _graphOrder = -1;

        public BuildType BuildType;

        public readonly ModuleAdditionalData AdditionalData;

        public readonly bool ProcessModulleShouldWaitForDependents;

        public abstract void Apply(IModuleProcessor processor);
    }
}
