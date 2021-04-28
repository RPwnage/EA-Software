using NAnt.Core;
using NAnt.Core.Attributes;

using System;
using System.Collections.Generic;

namespace EA.Eaconfig.Structured.ObjectModel
{
    public class Package
    {
        private static readonly Guid NamedPackageObjectId = new Guid("E65A0E6B-9095-4596-A113-046AA419963C");

        public static Package GetPackage(Project project)
        {
            Package package = (Package)project.NamedObjects[NamedPackageObjectId];
            if (package == null)
            {
                string packageName = project.Properties["package.name"];

                package = new Package(packageName);

                project.NamedObjects[NamedPackageObjectId] = package;
            }
            return package;
        }


        public Package(string name)
        {
            Name = name;
        }

        public readonly string Name;

        public readonly IDictionary<string, Module> Modules = new Dictionary<string, Module>();
    }
}
