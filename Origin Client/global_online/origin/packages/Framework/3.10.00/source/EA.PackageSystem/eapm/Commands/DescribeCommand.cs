using System;
using System.Collections.Specialized;

using NAnt.Core.PackageCore;
using EA.PackageSystem.PackageCore.Services;

using Release = NAnt.Core.PackageCore.Release;
using WebRelease = EA.PackageSystem.PackageCore.Services.Release;

namespace EA.PackageSystem.ConsolePackageManager
{

    [Command("describe")]
    class DescribeCommand : Command
    {

        public override string Summary
        {
            get { return "display information about a package"; }
        }
        public override string Usage
        {
            get { return "[ options ] [ <name>[-<version>] ] ..."; }
        }

        public override string Remarks
        {
            get
            {
                return
                    @"Options
    --about             show description
    --changes           show changes
    --contact           show contact information about the package
    --lastused          show the last day this package was used
    --license           show license and license comments
    --homepageurl       show home page URL
    --documentationurl  show documentation URL
    --name              show name and version
    --path              show the package directory path
    --requires          show what packages this package requires (see note)
    --requiredby        show what packages require this package
    --serverurl         show the package server URL (slow)
    --size              show the size of the package in bytes (slow)
    --status            show status and status comments
    --summary           show one line name and summary
    --usecount          show how many times this package has been used

Description
    If no option is given then all information except the serverurl and
    package contents will be displayed.

    If no version is given use the latest version of the package.

    Required packages are determined by past builds, that is, only packages
    that have been built will have any information.  To get a complete list
    of packages used by a package do a buildall on that package.

    Information about the package is stored in the package's Manifest.xml
    file.  If the package is missing this file most of this information will
    be <unknown>.  See the 'Manifest' topic in the Framework docs for details 
    on this file.

    A package is considered used if it is built or required by a nother 
    package.  This information is stored in the log file in the packages
    directory.

Examples
    Show information about a package:
    > eapm describe framework-0.8.0

    Show summary information about all the installed package:
    > eapm list --only-names | xargs eapm describe --summary";
            }
        }

        void ParseCommandLine(string[] args, StringCollection packageNames, StringCollection options)
        {
            int argIndex = 0;
            for (; argIndex < args.Length; argIndex++)
            {
                string arg = args[argIndex];
                if (!arg.StartsWith("--"))
                {
                    // rest of arguments are package names now
                    break;
                }
                options.Add(arg);
            }

            for (; argIndex < args.Length; argIndex++)
            {
                packageNames.Add(args[argIndex]);
            }
        }

        public override void Execute()
        {
            StringCollection packageNames = new StringCollection();
            StringCollection options = new StringCollection();
            ParseCommandLine(Arguments, packageNames, options);

            foreach (string fullPackageName in packageNames)
            {
                var release = GetRelease(fullPackageName);

                if (options.Count == 0)
                {
                    DisplayRelease(release);
                }
                else
                {
                    foreach (string option in options)
                    {
                        switch (option)
                        {
                            case "--about": DisplayAbout(release); break;
                            case "--changes": DisplayChanges(release); break;
                            case "--contact": DisplayContact(release); break;
                            case "--homepageurl": DisplayHomePageUrl(release); break;
                            case "--documentationurl": DisplayDocumentationUrl(release); break;
                            case "--license": DisplayLicense(release); break;
                            case "--name": DisplayName(release); break;
                            case "--path": DisplayPath(release); break;
                            case "--size": DisplaySize(release); break;
                            case "--summary": DisplaySummary(release); break;

                            default:
                                string msg = String.Format("Unknown option '{0}'.", option);
                                throw new InvalidCommandArgumentsException(this, msg);
                        }
                    }
                }
            }
        }

        Release GetRelease(string fullPackageName)
        {
            Release release;
            try
            {
                string packageName;
                string version;
                Release.ParseFullPackageName(fullPackageName, out packageName, out version);
                release = PackageMap.Instance.Releases.FindByNameAndVersion(packageName, version, true);
            }
            catch
            {
                release = PackageMap.Instance.Releases.FindByName(fullPackageName, true);
            }

            if (release == null)
            {
                string msg = String.Format("Package '{0}' not found.", fullPackageName);
                throw new ApplicationException(msg);
            }
            return release;
        }

        string GetPossiblyNullStringAsEmpty(string value)
        {
            /*
            if (value == null) {
                value = "";
            }
            */
            return value;
        }

        void DisplayRelease(Release release)
        {
            string msg = String.Format(@"{0}
by {1} <{2}>

About: {3}

Changes: {4}

Path: {5}",
                release.FullName,
                GetPossiblyNullStringAsEmpty(release.ContactName),
                GetPossiblyNullStringAsEmpty(release.ContactEmail),
                GetPossiblyNullStringAsEmpty(release.About),
                GetPossiblyNullStringAsEmpty(release.Changes),
                release.Path);

            Console.WriteLine(msg);
        }

        void DisplayAbout(Release release)
        {
            Console.WriteLine(GetPossiblyNullStringAsEmpty(release.About));
        }

        void DisplayChanges(Release release)
        {
            Console.WriteLine(GetPossiblyNullStringAsEmpty(release.Changes));
        }

        void DisplayContact(Release release)
        {
            Console.WriteLine("{0} <{1}>",
                GetPossiblyNullStringAsEmpty(release.ContactName),
                GetPossiblyNullStringAsEmpty(release.ContactEmail));
        }

        void DisplayHomePageUrl(Release release)
        {
            Console.WriteLine(release.HomePageUrl);
        }

        void DisplayDocumentationUrl(Release release)
        {
            Console.WriteLine(release.DocumentationUrl);
        }

        void DisplayLicense(Release release)
        {
            Console.WriteLine(GetPossiblyNullStringAsEmpty(release.License));
            if (release.LicenseComment != null && release.LicenseComment.Length > 0)
            {
                Console.WriteLine(release.LicenseComment);
            }
        }

        void DisplayName(Release release)
        {
            Console.WriteLine(release.FullName);
        }

        void DisplayPath(Release release)
        {
            Console.WriteLine(release.Path);
        }

        void DisplaySummary(Release release)
        {
            Console.WriteLine("{0}\t{1}", release.FullName, GetPossiblyNullStringAsEmpty(release.Summary));
        }

        void DisplaySize(Release release)
        {
            Console.WriteLine(release.GetSize());
        }
    }
}
