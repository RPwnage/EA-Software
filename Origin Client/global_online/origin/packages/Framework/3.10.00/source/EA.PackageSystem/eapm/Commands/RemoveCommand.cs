using System;
using System.IO;

using NAnt.Core.PackageCore;
using EA.PackageSystem.PackageCore.Services;

using Release = NAnt.Core.PackageCore.Release;
using WebRelease = EA.PackageSystem.PackageCore.Services.Release;

namespace EA.PackageSystem.ConsolePackageManager
{

    [Command("remove")]
    class RemoveCommand : Command
    {

        public override string Summary
        {
            get { return "remove a package"; }
        }
        public override string Usage
        {
            get { return "<packagename>-<version>"; }
        }

        public override string Remarks
        {
            get
            {
                return
                    @"Description
    Deletes a specific version of a package.

    NOTE: This command removes all files in the package directory including
    any modified files.

    THERE IS NO WAY TO UNDO THIS COMMAND.

Examples
    Remove the Config-0.8.0 package from your machine:
    > eapm remove config-0.8.0

    Remove all versions of the packages from your machine: (USE CAUTION)
    > eapm list config | xargs -n 1 eapm remove";
            }
        }

        public override void Execute()
        {
            if (Arguments.Length != 1)
            {
                throw new InvalidCommandArgumentsException(this);
            }

            string name;
            string version;
            GetPackageNameAndVersion(out name, out version);

            Release release = PackageMap.Instance.Releases.FindByNameAndVersion(name, version, true);

            if (release == null)
            {
                Console.WriteLine("Package '{0}-{1}' does not exist in '{2}'.", name, version, PackageMap.Instance.PackageRoots.ToString());
            }
            else
            {
                try
                {
                    Release r = PackageMap.Instance.GetFrameworkRelease();
                    if (release.Name != r.Name || release.Version != r.Version)
                    {
                        Console.WriteLine("Deleting directory {0}", release.Path);
                        release.Delete();
                    }
                    else
                    {
                        Console.WriteLine("\neapm.exe is a part of " + r.FullName + " and hence can not uninstall " + r.FullName + ". \nIf you would like to uninstall " + r.FullName + ", please use the 'Uninstall' option in the Start Menu under " + r.FullName + "!");
                    }

                }
                catch (Exception e)
                {
                    string msg = String.Format("The directory '{0}' could not be deleted because:\n{1}", release.Path, e.Message);
                    throw new ApplicationException(msg);
                }
            }
        }

        void GetPackageNameAndVersion(out string name, out string version)
        {
            string fullPackageName = Arguments[0];

            int splitIndex = fullPackageName.IndexOf("-");
            if (splitIndex > 0)
            {
                name = fullPackageName.Substring(0, splitIndex);
                version = fullPackageName.Substring(splitIndex + 1, fullPackageName.Length - splitIndex - 1);
            }
            else
            {
                string msg = String.Format("Could not determine name and version from '{0}'.", fullPackageName);
                throw new ApplicationException(msg);
            }
        }
    }
}
