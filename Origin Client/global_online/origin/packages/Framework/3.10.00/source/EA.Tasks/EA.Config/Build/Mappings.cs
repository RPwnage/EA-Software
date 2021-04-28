using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
 

namespace EA.Eaconfig.Build
{
    public static class Mappings
    {
        public static readonly KeyValuePair<string, uint>[] DependencyPropertyMapping = new KeyValuePair<string, uint> []
            { 
                // Use
                new KeyValuePair<string, uint>(".interfacedependencies", DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".usedependencies", DependencyTypes.Interface | DependencyTypes.Link),                
                new KeyValuePair<string, uint>(".uselinkdependencies", DependencyTypes.Link),
                // Build
                new KeyValuePair<string, uint>(".builddependencies", DependencyTypes.Build| DependencyTypes.Interface | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".linkdependencies", DependencyTypes.Build | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".buildinterfacedependencies", DependencyTypes.Build | DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".buildonlydependencies", DependencyTypes.Build),
                // Auto Build/Use
                new KeyValuePair<string, uint>(".autodependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Interface | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".autointerfacedependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".autolinkdependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Link)
            };

        public static readonly KeyValuePair<string, uint>[] ModuleDependencyPropertyMapping = new KeyValuePair<string, uint>[]
            { 
                // Use
                new KeyValuePair<string, uint>(".interfacemoduledependencies", DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".usemoduledependencies", DependencyTypes.Interface | DependencyTypes.Link),                
                new KeyValuePair<string, uint>(".uselinkmoduledependencies", DependencyTypes.Link),
                // Build
                new KeyValuePair<string, uint>(".moduledependencies", DependencyTypes.Build| DependencyTypes.Interface | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".linkmoduledependencies", DependencyTypes.Build | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".buildinterfacemoduledependencies", DependencyTypes.Build | DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".buildonlymoduledependencies", DependencyTypes.Build),
                // Auto Build/Use
                new KeyValuePair<string, uint>(".automoduledependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Interface | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".autointerfacemoduledependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".autolinkmoduledependencies", DependencyTypes.AutoBuildUse | DependencyTypes.Link)
            };

        public static readonly KeyValuePair<string, uint>[] ModuleDependencyConstraintsPropertyMapping = new KeyValuePair<string, uint>[]
            { 
                // Use
                new KeyValuePair<string, uint>(".interfacebuildmodules", DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".usebuildmodules", DependencyTypes.Interface | DependencyTypes.Link),                
                new KeyValuePair<string, uint>(".uselinkbuildmodules", DependencyTypes.Link),
                // Build
                new KeyValuePair<string, uint>(".buildmodules", DependencyTypes.Build| DependencyTypes.Interface | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".linkbuildmodules", DependencyTypes.Build | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".buildinterfacebuildmodules", DependencyTypes.Build | DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".buildonlybuildmodules", DependencyTypes.Build),
                // Auto Build/Use
                new KeyValuePair<string, uint>(".autobuildmodules", DependencyTypes.AutoBuildUse | DependencyTypes.Interface | DependencyTypes.Link),
                new KeyValuePair<string, uint>(".autointerfacebuildmodules", DependencyTypes.AutoBuildUse | DependencyTypes.Interface),
                new KeyValuePair<string, uint>(".autolinkbuildmodules", DependencyTypes.AutoBuildUse | DependencyTypes.Link)
            };


    }
}
