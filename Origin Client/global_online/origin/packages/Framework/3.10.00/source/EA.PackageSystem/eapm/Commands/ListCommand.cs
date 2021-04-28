using System;
using System.Linq;
using System.Text.RegularExpressions;

using NAnt.Core.PackageCore;
using PackageServer = EA.PackageSystem.PackageCore.Services;
using NAnt.Core;

namespace EA.PackageSystem.ConsolePackageManager
{

    [Command("list")]
    class ListCommand : Command
    {

        public override string Summary
        {
            get { return "list installed packages"; }
        }
        public override string Usage
        {
            get { return "[ options ] [ name1 ] [ name2 ] ..."; }
        }

        public override string Remarks
        {
            get
            {
                return
                    @"Options
    --newer         list packages with newer versions on the Package Server
    --only-names    do not show version numbers

Description
    If no package name is given, list all packages, otherwise list all 
    releases for the given package name.

Examples
    Show all install packages:
    > eapm list

    Show all the installed packages without version numbers:
    > eapm list --onlynames

    Show the latest installed version of the VisualStudio package:
    > eapm list visualstudio | head -n 1

    Show all the packages with newer versions available:
    > eapm list --newer
    
    Update all the packages with the term 'xbox' in them.
    > eapm list --newer | grep -i xbox | xargs eapm install";
            }
        }

        enum Action
        {
            ShowNewerReleases,
            ShowPackages,
            ShowReleases
        }

        Action ParseCommandLine(out string[] packageNames)
        {
            Action action = Action.ShowReleases;

            int argIndex = 0;
            for (argIndex = 0; argIndex < Arguments.Length; argIndex++)
            {
                if (!Arguments[argIndex].StartsWith("--"))
                {
                    break;
                }

                switch (Arguments[argIndex])
                {
                    case "--newer":
                        action = Action.ShowNewerReleases;
                        break;

                    case "--onlynames":
                        action = Action.ShowPackages;
                        break;

                    default:
                        string msg = String.Format("Unknown option '{0}'.", Arguments[argIndex]);
                        throw new InvalidCommandArgumentsException(this, msg);
                }
            }

            packageNames = new string[Arguments.Length - argIndex];
            Array.Copy(Arguments, argIndex, packageNames, 0, packageNames.Length);

            return action;
        }

        public override void Execute()
        {
            string[] packageNames;
            switch (ParseCommandLine(out packageNames))
            {
                case Action.ShowNewerReleases:
                    ShowNewerPackages(packageNames);
                    break;

                case Action.ShowPackages:
                    ShowPackages();
                    break;

                case Action.ShowReleases:
                    ShowReleases(packageNames);
                    break;
            }
        }

        void ShowNewerPackages(string[] packageNames)
        {
            PackageServer.WebServices services = PackageServer.WebServicesFactory.Generate();
            PackageServer.Release[] serverReleases = services.GetAllReleases();

            if (packageNames.Length == 0)
            {
                foreach (Package package in PackageMap.Instance.Packages.InOrder)
                {
                    DisplayNewerReleaseName(package, serverReleases);
                }
            }
            else
            {
                foreach (string packageName in packageNames)
                {
                    Package package = PackageMap.Instance.Packages.FindByName(packageName, true);
                    if (package == null)
                    {
                        string msg = String.Format("Package '{0}' not found.", packageName);
                        throw new ApplicationException(msg);
                    }
                    DisplayNewerReleaseName(package, serverReleases);
                }
            }
        }

        void DisplayNewerReleaseName(Package package, PackageServer.Release[] serverReleases)
        {
            // get the latest local release
            Release localRelease = package.Releases.First();

            // find the latest server release
            PackageServer.Release serverRelease = null;
            foreach (PackageServer.Release r in serverReleases)
            {
                if ((String.Compare(localRelease.Name, r.Name, true) == 0))
                {
                    serverRelease = r;
                    break;
                }
            }

            if ((serverRelease != null) && String.Compare(serverRelease.Version, localRelease.Version, true) > 0)
            {
                Console.WriteLine("{0}-{1}", serverRelease.Name, serverRelease.Version);
            }
        }

        void ShowPackages()
        {
            foreach (Package package in PackageMap.Instance.Packages.InOrder)
            {
                Console.WriteLine(package.Name);
            }
        }

        void ShowReleases(string[] packageNames)
        {
            if (packageNames.Length == 0)
            {
                DisplayReleaseNames(PackageMap.Instance.Releases);
            }
            else
            {
                foreach (string packageName in packageNames)
                {
                    Package package = PackageMap.Instance.Packages.FindByName(packageName, true);
                    if (package == null)
                    {
                        string msg = String.Format("Package '{0}' not found.", packageName);
                        throw new ApplicationException(msg);
                    }
                    DisplayReleaseNames(package.Releases);
                }
            }
        }

        void DisplayReleaseNames(ReleaseCollection releases)
        {
            foreach (Release release in releases)
            {
                Console.WriteLine(release.FullName);
            }
        }
    }
}
