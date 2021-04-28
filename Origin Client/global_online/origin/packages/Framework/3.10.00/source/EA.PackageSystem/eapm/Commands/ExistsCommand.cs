using System;
using System.Text.RegularExpressions;

using EA.PackageSystem.PackageCore;
using PackageServer = EA.PackageSystem.PackageCore.Services;
using Release = NAnt.Core.PackageCore.Release;
using EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.ConsolePackageManager
{

    [Command("exists")]
    class ExistsCommand : Command
    {

        public override string Summary
        {
            get { return "returns 1 if package(s) exists on the package server"; }
        }
        public override string Usage
        {
            get { return "name[-version]"; }
        }

        public override string Remarks
        {
            get
            {
                return
                    @"Description
    If packaghe exists returns 1, othwise 0 

Examples
    Tell if package exists on the package server:
    > eapm exists eaconfig

    Tell if package version exists on the package package server:
    > eapm exists eaconfig-dev";
            }
        }

        enum Action
        {
            ShowPackageServer
        }

        Action ParseCommandLine(out string[] packageNames)
        {
            Action action = Action.ShowPackageServer;

            //argIndex here for potential possibility down
            //the line when we provide more arguments...
            int argIndex = 0;
            foreach (string argument in Arguments)
            {
                if (!argument.StartsWith("--"))
                {
                    break;
                }

                switch (argument)
                {
                    default:
                        string msg = String.Format("Unknown option '{0}'.", argument);
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
                case Action.ShowPackageServer:
                    ShowPackageServer(packageNames);
                    break;
            }
        }

        void ShowPackageServer(string[] packageNames)
        {
            if (packageNames.Length == 0)
            {
                throw new ApplicationException("No package specified");
            }

            if (packageNames.Length > 1)
            {
                throw new ApplicationException("More than one package specified");
            }

            string name = packageNames[0];
            string version = null;


            try
            {
                string packageName;
                Release.ParseFullPackageName(name, out packageName, out version);
                name = packageName;

            }
            catch
            {
                name = packageNames[0];
                version = null;
            }

            PackageServer.WebServices services = PackageServer.WebServicesFactory.Generate();
            if (String.IsNullOrEmpty(version))
            {
                PackageServer.Release[] releases = services.GetPackageReleases(name);
                if (releases == null)
                {
                    string msg = String.Format("Package '{0}' not found on package server.", name);
                    throw new ApplicationException(msg);
                }

                if (releases.Length <= 0)
                {
                    string msg = String.Format("Package '{0}' has no releases.", name);
                    throw new ApplicationException(msg);
                }
                Console.WriteLine("package '{0}' found on the package server.", name);
            }
            else
            {
                PackageServer.Release release = services.GetRelease(name, version);

                if (release == null)
                {
                    string msg = String.Format("Package '{0}-{1}' not found on package server.", name, version);
                    throw new ApplicationException(msg);
                }
                Console.WriteLine("package release'{0}-{1}' found on the package server.", name, version);
            }
        }
    }
}
