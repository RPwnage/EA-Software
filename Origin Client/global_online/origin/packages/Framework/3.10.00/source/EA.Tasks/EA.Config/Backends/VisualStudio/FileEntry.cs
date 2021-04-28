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
    internal class FileEntry
    {
        internal const uint Buildable = 1;
        internal const uint ExcludedFromBuild = 2;
        internal const uint BulkBuild = 4;
        internal const uint Headers = 8;
        internal const uint Assets = 16;
        internal const uint Resources = 32;
        internal const uint CopyLocal = 64;
        internal const uint CustomBuildTool = 128;
        internal const uint NonBuildable = 256;
        //VSDotNetProject defines masks starting with 1024. 

        internal class ConfigFileEntry : BitMask
        {
            internal readonly Configuration Configuration;
            internal readonly FileItem FileItem;
            internal readonly Tool Tool;

            internal ConfigFileEntry(FileItem fileitem, Configuration configuration, Tool tool, uint flags = 0) : base(flags)
            {
                Configuration = configuration;
                FileItem = fileitem;
                Tool = tool;
            }
        }

        internal FileEntry(PathString path, string filesetbasedir) 
        {
            Path = path;
            FileSetBaseDir = PathNormalizer.Normalize(filesetbasedir, false);
            ConfigEntries = new  List<ConfigFileEntry>();
        }

        internal bool TryAddConfigEntry(FileItem fileitem, Configuration configuration, Tool tool, uint flags = 0)
        {
            if (-1 != ConfigEntries.FindIndex(obj => obj.Configuration.Name == configuration.Name && obj.Configuration.SubSystem == configuration.SubSystem))
            {
                return false;
            }

            ConfigEntries.Add(new ConfigFileEntry(fileitem, configuration, tool, flags));

            return true;
        }

        internal ConfigFileEntry FindConfigEntry(Configuration configuration)
        {
            return ConfigEntries.Find(obj => obj.Configuration.Name == configuration.Name && obj.Configuration.SubSystem == configuration.SubSystem);
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

        internal readonly string FileSetBaseDir;
        internal readonly PathString Path;
        internal readonly List<ConfigFileEntry> ConfigEntries;
    }
}
