// Copyright (C) 2001-2003 Electronic Arts
//
// Kosta Arvanitis (karvanitis@ea.com) 
using System;

namespace NAnt.Shared.Properties
{
    /// <summary>
    /// PackageTask properties.
    /// </summary>
    public static class PackageProperties
    {
        // This property, ${build.dir} is legacy from FW1 and should not be used -
        // use PackageBuildDirectoryPropertyName ${package.builddir} instead
        public const string BuildDirectoryPropertyName = "build.dir";
        public const string PackageBuildDirectoryPropertyName = "package.builddir";
        public const string PackageConfigPropertyName = "package.config";
        public const string PackageConfigsPropertyName = "package.configs";
        public const string PackageAllConfigsPropertyName = "package.available.configs";
        public const string PackageConfigDirPropertyName = "package.configdir";
        public const string PackageDirectoryPropertyName = "package.dir";
        public const string PackageNamePropertyName = "package.name";
        public const string PackageTargetVersionPropertyName = "package.targetversion";
        public const string PackageVersionPropertyName = "package.version";

        public const string ConfigNameProperty = "config";
        public const string ConfigFileExtension = ".xml";

        public const string PlatformConfigPackagePostfix = "_config";

    }

    public static class FrameworkProperties
    {
        public const string BulkBuildPropertyName = "bulkbuild";
        public const string BulkBuildExcludedPropertyName = "bulkbuild.excluded";
        public const string AutoBuildUseDependencyPropertyName = "autobuilduse";

    }

}
