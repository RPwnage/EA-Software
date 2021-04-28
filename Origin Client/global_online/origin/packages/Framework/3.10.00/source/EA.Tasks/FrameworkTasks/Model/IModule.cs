using System;
using System.Collections.Generic;

using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
    public interface IModule
    {
        string Name { get; }
        string GroupName { get; }
        string GroupSourceDir { get; }

        BuildGroups BuildGroup { get; }

        string Key { get; }

        IPackage Package { get; }

        Configuration Configuration { get; }

        DependentCollection Dependents { get; }

        bool IsKindOf(uint t);

        bool CollectAutoDependencies { get; }

        string OutputName { get; }

        PathString OutputDir { get; set; }

        PathString IntermediateDir { get; set; }

        PathString ScriptFile { get;  }

        IPublicData Public(IModule parentModule);

        ISet<IModule> ModuleSubtree(uint dependencytype, ISet<IModule> modules);
    }

    

}
