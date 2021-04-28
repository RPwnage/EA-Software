using System;
using System.Collections.Specialized;

using EA.PackageSystem.PackageCore;
using PackageServer = EA.PackageSystem.PackageCore.Services;

using Release = NAnt.Core.PackageCore.Release;
using WebRelease = EA.PackageSystem.PackageCore.Services.Release;

namespace EA.PackageSystem.ConsolePackageManager
{
    [Command("history")]
    class HistoryCommand : Command
    {
        public override string Summary
        {
            get { return "Retrieve the versions, dates, and statuses of all the releases of a given package."; }
        }

        public override string Usage
        {
            get { return "[package1] [package2] ..."; }
        }

        public override string Remarks
        {
            get { return "eapm history [package1] [package2] ..."; }
        }

        public override void Execute()
        {
            PackageServer.WebServices server = PackageServer.WebServicesFactory.Generate();
            
            foreach (string argument in Arguments)
            {
                WebRelease[] releases = server.GetPackageReleases(argument);

                foreach (WebRelease release in releases)
                {
                    Console.WriteLine("{0} {1} {2} {3}", argument, release.Version, release.DateCreated, release.StatusName);
                }
            }
        }
    }
}
