using System;

using NAnt.Core.PackageCore;
using EA.PackageSystem.PackageCore;
using EA.PackageSystem.PackageCore.Services;

using Release = NAnt.Core.PackageCore.Release;
using WebRelease = EA.PackageSystem.PackageCore.Services.Release;


namespace EA.PackageSystem.ConsolePackageManager
{

    [Command("install")]
    class InstallCommand : Command
    {

        public override string Summary
        {
            get { return "install a package"; }
        }
        public override string Usage
        {
            get { return "<packagename>[-version] [skipdependents]"; }
        }

        public override string Remarks
        {
            get
            {
                return
                    @"Description
    If no version is given, install the latest version of the package.

Examples
    Install the latest version of the Config package:
    > eapm install config

    Install a specific version of the Config package:
    > eapm install config-0.8.0

    Update all installed packages that have ubi in their name to the
    latest versions on the Package Server:
    > eapm list --newer | grep -i ubi | xargs -n 1 eapm install";
            }
        }

        public override void Execute()
        {
            if (Arguments.Length < 1 || Arguments.Length > 2)
            {
                throw new InvalidCommandArgumentsException(this);
            }

            string name;
            string version;
            GetPackageNameAndVersion(out name, out version);

            if (version == null)
            {
                Console.WriteLine("Finding latest version ...");
                version = GetLatestVersion(name);
            }

            Release release = PackageMap.Instance.Releases.FindByNameAndVersion(name, version, true);

            if (release != null)
            {
                Console.WriteLine("Package '{0}' already installed.", release.FullName);
            }
            else
            {
                bool getDependents= true;	//by default, get/download dependent packages

                if (Arguments.Length > 1)
                {
                    if (Arguments[1] == "skipdependents")
                    {
                        //don't get/download dependent packages
                        getDependents = false;
                    }
                }

                var installer = new PackageCore.PackageInstaller();
                installer.Releases.Add(name, version, getDependents);
                installer.UpdateProgress += new InstallProgressEventHandler(UpdateProgress);
                installer.Install();
                installer.UpdateProgress -= new InstallProgressEventHandler(UpdateProgress);

                //Console.WriteLine();
            }
        }

        Release _lastRelease = null;
        int _stepsPrinted = 0;

        void UpdateProgress(object sender, InstallProgressEventArgs e)
        {
            if (_lastRelease != e.Release)
            {
                if (_lastRelease != null)
                {
                    Console.WriteLine();
                }
                _lastRelease = e.Release;
                if (e.Release != null)
                {
                    Console.Write("Installing {0}-{1} .", e.Release.Name, e.Release.Version);
                    _stepsPrinted = 0;
                }
            }

            int steps = e.LocalProgress / 5;
            while (_stepsPrinted < steps)
            {
                Console.Write(".");
                _stepsPrinted++;
            }
        }

        string GetLatestVersion(string name)
        {
            try
            {
                WebServices server = WebServicesFactory.Generate();

                WebRelease[] releases = server.GetPackageReleases(name);
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

                return releases[0].Version;
            }
            catch (Exception e)
            {
                string msg = String.Format("Could not determine latest version because:\n{0}", e.Message);
                throw new ApplicationException(msg);
            }
        }

        void GetPackageNameAndVersion(out string name, out string version)
        {
            try
            {
                Release.ParseFullPackageName(Arguments[0], out name, out version);
            }
            catch
            {
                // could not parse, maybe it's just a package name without version
                name = Arguments[0];
                version = null;
            }
        }
    }
}
