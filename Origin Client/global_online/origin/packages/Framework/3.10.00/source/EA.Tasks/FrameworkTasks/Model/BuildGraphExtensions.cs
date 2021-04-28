using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NAnt.Core;
using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
    public static class BuildGraphExtensions
    {
        public static bool TryGetPackage(this Model.BuildGraph graph, string name, string version, string config, out Model.IPackage package)
        {
            return graph.Packages.TryGetValue(Model.Package.MakeKey(name, version, config), out package);
        }

        public static bool TryAddPackage(this Model.BuildGraph graph, Model.IPackage package)
        {
            return graph.Packages.TryAdd(package.Key, package);
        }


        public static Model.IPackage GetOrAddPackage(this Model.BuildGraph graph, string name, string version, PathString dir, string config, FrameworkVersions frameworkVersion)
        {
            return graph.Packages.GetOrAdd(Model.Package.MakeKey(name, version, config), (key) => new Model.Package(name, version, dir, config, frameworkVersion));
        }

        public static bool TryGetPackage(this Model.BuildGraph graph, string name, string version, PathString dir, string config, out Model.IPackage package)
        {
            return graph.Packages.TryGetValue(Model.Package.MakeKey(name, version, config), out package);
        }

        public static bool TryGetModule(this Model.BuildGraph graph, string name, BuildGroups buildgroup, IPackage package, out Model.IModule module)
        {
            return graph.Modules.TryGetValue(Model.Module.MakeKey(name, buildgroup, package), out module);
        }

        public static bool TryAddModule(this Model.BuildGraph graph, Model.IModule module)
        {
            return graph.Modules.TryAdd(module.Key, module);
        }

    }
}
