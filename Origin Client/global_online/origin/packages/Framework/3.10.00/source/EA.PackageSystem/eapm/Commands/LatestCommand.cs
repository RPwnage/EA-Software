/* (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved */

using System;

using EA.PackageSystem.PackageCore;
using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.ConsolePackageManager {

    [Command("latest")]
    class LatestCommand : Command {

        public override string Summary {
            get { return "query the latest version of a package on the package server"; } 
        }
        public override string Usage {
            get { return "<packagename>"; } 
        }

        public override string Remarks {
            get { return 
@"Description
    Query the latest version of the package on the package server without installing it.

Examples
    Query the latest version of eaconfig:
    > eapm latest eaconfig";
            } 
        }

        public override void Execute() {
            if (Arguments.Length != 1) {
                throw new InvalidCommandArgumentsException(this);
            }

            string name = Arguments[0];
            string version;
            Console.WriteLine("Finding latest version ...");
            version = GetLatestVersion(name);
            Console.Write( name );
            if ( version != null )
            {
                Console.Write( "-" + version + "\n" );
            }

        }

        string GetLatestVersion(string name) {
            try {
                PackageServer.WebServices server = PackageServer.WebServicesFactory.Generate();
                PackageServer.Release[] releases = server.GetPackageReleases(name);
                if (releases == null) {
                    string msg = String.Format("Package '{0}' not found on package server.", name);
                    throw new ApplicationException(msg);
                }

                if (releases.Length <= 0) {
                    string msg = String.Format("Package '{0}' has no releases.", name);
                    throw new ApplicationException(msg);
                }

                return releases[0].Version;
            } catch (Exception e) {
                string msg = String.Format("Could not determine latest version because:\n{0}", e.Message);
                throw new ApplicationException(msg);
            }
        }
    }
}
