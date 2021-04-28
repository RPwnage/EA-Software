using System;
using System.Text;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
    public interface IPackage
    {
        string Name { get; }
        string Version { get; }
        string ConfigurationName { get; }
        string Key { get; }

        PathString Dir { get; }

        void AddModule(IModule module);

        IEnumerable<IModule> Modules { get; }

        IEnumerable<Dependency<IPackage>> DependentPackages { get; }

        bool IsKindOf(uint t);

        void SetType(uint t);

        bool TryGetPublicData(out IPublicData data, IModule parentModule, IModule dependentModule);

        bool TryAddPublicData(IPublicData data, IModule parentModule, IModule dependentModule);

        PathString PackageBuildDir { get; set;  }

        PathString PackageConfigBuildDir { get; set;  }

        SourceControl SourceControl { get; set; }

        Project Project { get; set; }

        FrameworkVersions FrameworkVersion { get; }
    }

    public struct PackageDependencyTypes
    {
        public const uint Use = 1;
        public const uint Build = 2;

        public static string ToString(uint type)
        {
            StringBuilder sb = new StringBuilder();

            test(type, Use, "use", sb);
            test(type, Build, "build", sb);

            return sb.ToString().TrimEnd('|');
        }

        private static void test(uint t, uint mask, string name, StringBuilder sb)
        {
            if ((mask & t) != 0) sb.AppendFormat("{0}|", name);
        }

    }


    public class IPackageEqualityComparer : IEqualityComparer<IPackage>
    {
        public bool Equals(IPackage x, IPackage y)
        {
            return x.Key == y.Key;
        }

        public int GetHashCode(IPackage obj)
        {
            return obj.Key.GetHashCode();
        }
    }

}
