using System;
using System.Collections.Generic;
using System.Text;
using NAnt.Core.Metrics;
using System.Diagnostics;


namespace EA.MetricsProcessor
{
    internal class MetricsData
    {
        internal const string UNDEFINED = "????";

        internal Dictionary<String, int> TargetIDCache = new Dictionary<String, int>();
        internal Dictionary<String, PackageVersionCache> PackageVersions = new Dictionary<String, PackageVersionCache>();
        internal Dictionary<String, ConfigCache> ConfigKeys = new Dictionary<String, ConfigCache>();


        internal static string TargetKey(string inTargetName, int? parentTargetID, int inConfigID, string inBuildGroup)
        {
            string parentTargetIDStr = parentTargetID == null ? "NULL" : parentTargetID.ToString();
            return inTargetName + "-" + parentTargetIDStr + "-" + inConfigID.ToString() + "-" + inBuildGroup;
        }

        internal class PackageVersionCache
        {
            internal PackageVersionCache(string name, string version, int dbVersionKey)
            {
                Name = name;
                Version = version;
                DbVersionKey = dbVersionKey;
            }

            internal readonly string Name;
            internal readonly string Version;
            internal readonly int DbVersionKey;
            internal bool IsUseStored = false;
            internal bool IsBuildStored = false;

            internal static string Key(string packageName, string packageVersion)
            {
                return packageName + "-" + packageVersion;
            }
        }

        internal class ConfigCache
        {
            internal ConfigCache(string configName, string configSystem, string configCompiler, int dbConfigKey)
            {
                ConfigName = configName;
                ConfigSystem = configSystem;
                ConfigCompiler = configCompiler;
                DbConfigKey = dbConfigKey;
            }
            internal readonly string ConfigName;
            internal readonly string ConfigSystem;
            internal readonly string ConfigCompiler;
            internal readonly int DbConfigKey;

            internal static string Key(string configName, string configSystem, string configCompiler)
            {
                return configName + "*" + configSystem + "*" + configCompiler;
            }
        }
    }
}
