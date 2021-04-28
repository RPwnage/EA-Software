using System;
using System.IO;

using EA.PackageSystem.PackageCore;
using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.ConsolePackageManager {

    [Command("post")]
    class PostCommand : Command {

        public override string Summary {
            get { return "post a package"; } 
        }
        public override string Usage {
            get { return "<packagename>-<version>.zip"; } 
        }

        public override string Remarks {
            get { return 
@"Description
    Post the given package zip file to the EA Package Server.  The 
    Manifest.xml file will be used to specify the change details.

    NOTE: You should post the first release of a package manually via the 
    web site.

Examples
    Post the Config-0.8.0 package to the server:
    > eapm post build/Config-0.8.0.zip"; 
            } 
        }

        public override void Execute() {
            if (Arguments.Length != 1) {
                throw new InvalidCommandArgumentsException(this);
            }

            string path = Arguments[0];

            try {
                PackagePoster poster = new PackagePoster();
                Console.WriteLine("Posting {0} ", path);
                poster.Post(path);

            } catch (Exception e) {
                throw new ApplicationException("Package could not be posted.", e);
            }
        }
    }
}
