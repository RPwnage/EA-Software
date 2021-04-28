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
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class ProjectRefEntry 
    {
        internal const uint CopyLocal = 1;
        internal const uint CopyLocalSatelliteAssemblies = 2;
        internal const uint LinkLibraryDependencies = 4;
        internal const uint UseLibraryDependencyInputs = 8;
        internal const uint ReferenceOutputAssembly = 16;

        internal class ConfigProjectRefEntry : BitMask
        {
            internal readonly Configuration Configuration;

            internal ConfigProjectRefEntry(Configuration configuration, uint flags = 0) : base(flags)
            {
                Configuration = configuration;
            }
        }

        internal ProjectRefEntry(VSProjectBase projectRef)
        {
            ProjectRef = projectRef;
            ConfigEntries = new List<ConfigProjectRefEntry>();
        }

        internal bool TryAddConfigEntry(Configuration configuration, uint flags = 0)
        {
            if (-1 != ConfigEntries.FindIndex(obj => obj.Configuration.Name == configuration.Name && obj.Configuration.SubSystem == configuration.SubSystem))
            {
                return false;
            }

            ConfigEntries.Add(new ConfigProjectRefEntry(configuration, flags));

            return true;
        }

        internal string ConfigSignature
        {
            get
            {
                return ConfigEntries.OrderBy(ce => ce.Configuration.Name).Aggregate(new StringBuilder(), (result, ce)=> result.AppendLine(ce.Configuration.Name + ":" +ce.Type.ToString())).ToString();
            }
        }

        internal bool IsKindOf(Configuration config, uint flags)
        {
            var kind = ConfigEntries.FirstOrDefault(ce=> ce.Configuration == config);

            return (kind != null && kind.IsKindOf(flags));
        }

        internal readonly VSProjectBase ProjectRef;
        internal readonly List<ConfigProjectRefEntry> ConfigEntries;
    }
}
