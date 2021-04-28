using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.IO;
using System.Diagnostics;
using System.Xml.Xsl;
using System.Web.Services;

using PackageServer = EA.PackageSystem.PackageCore.Services;

namespace MasterConfigUpdater
{
    class Framework2Extensions
    {
        PackageServer.WebServices mServer;
        bool mForce;
        bool mNewest;

        public Framework2Extensions(PackageServer.WebServices server, bool force, bool newest)
        {
            mServer = server;
            mForce = force;
            mNewest = newest;
        }

        public string update_package_version(string name, string oldVersion)
        {
            Console.Write("  Processing {0}... ", name);
            System.Diagnostics.Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();
            PackageServer.Release[] releases = mServer.GetPackageReleases(name);
            stopwatch.Stop();

            if (releases == null)
            {
                Console.WriteLine("not found on package server!");
                return oldVersion;
            }

            if (releases.Length == 0)
            {
                Console.WriteLine("no releases on package server!");
                return oldVersion;
            }

            bool update = mForce;

            if (!update)
            {
                // Scan all releases to find the version of the package in the current masterconfig
                // file. If it is found and it is not the newest version, perform the version number
                // update. This will handle cases of, say, updating eaconfig-1.33.12 to eaconfig-1.33.13
                // but will not update cases of, say, eaconfig-dev to eaconfig-1.33.13 because the
                // pre-existing version has not been published.
                for (int i = 0; i < releases.Length; ++i)
                {
                    if (oldVersion == releases[i].Version && (i > 0 || releases[i].StatusName != "Official"))
                    {
                        update = true;
                        break;
                    }
                }
            }

            int releaseIndex = 0;

            if (!mNewest)
            {
                for (; releaseIndex < releases.Length; ++releaseIndex)
                {
                    if (releases[releaseIndex].StatusName == "Official")
                        break;
                }
            }

            // This typically means there are no official versions on the package server. If this
            // is the case the program will then skip the update assuming that the provided 
            // masterconfig file is correct.
            if (releaseIndex >= releases.Length)
            {
                releaseIndex = 0;
                update = false;
            }

            string version;

            if (update)
            {
                Console.Write("updating from {0} to {1}.", oldVersion, releases[releaseIndex].Version);
                version = releases[releaseIndex].Version;
            }
            else
            {
                Console.Write("not updating {0} (package server latest: {1})", oldVersion, releases[releaseIndex].Version);
                version = oldVersion;
            }

            // Debug builds profile the API call, in case you wish to nag IT about how long service calls take ;)
#if DEBUG
            Console.WriteLine(" ({0} ms)", stopwatch.ElapsedMilliseconds);
#else
            Console.WriteLine();
#endif
            return version;
        }
    }

    class Program
    {
        static string sInputFile;
        static string sOutputFile;
        static bool sForce;
        static bool sNewest;

        static void PrintHelp()
        {
            Console.WriteLine("Usage: MasterConfigUpdater [-f] INPUT [optional output]");
            Console.WriteLine();
            Console.WriteLine("  -f  Force update to latest version. Normally MasterConfigUpdater will leave");
            Console.WriteLine("      package versions alone if the old version -- such as a development");
            Console.WriteLine("      version -- has not been published to the package server.");
            Console.WriteLine();
            Console.WriteLine("  -n  Update to the newest version regardless of status. MasterConfigUpdater");
            Console.WriteLine("      will only update to the latest version that has Official status by");
            Console.WriteLine("      default.");
            Console.WriteLine();
            Console.WriteLine("MasterConfigUpdater will update the specified input masterconfig file with the");
            Console.WriteLine("latest versions of packages published on the package server. The update will");
            Console.WriteLine("happen in-place unless an optional output masterconfig file is specified.");
        }

        static bool ValidateArgs(string[] args)
        {
            bool printHelp = false;
            if (args.Length == 0)
            {
                printHelp = true;
            }
            else
            {
                int helpIndex = args[0].IndexOf("help");
                printHelp = (helpIndex == 1 || helpIndex == 2 || args[0] == "-h");
            }

            if (printHelp)
            {
                PrintHelp();
                return false;
            }

            int fileSearchBaseIndex = 0;

            foreach (string arg in args)
            {
                if (arg == "-f")
                {
                    sForce = true;
                    ++fileSearchBaseIndex;
                }
                else if (arg == "-n")
                {
                    sNewest = true;
                    ++fileSearchBaseIndex;
                }
            }

            if (!File.Exists(args[fileSearchBaseIndex]))
                throw new ApplicationException(string.Format("Input masterconfig file '{0}' does not exist.", args[0]));

            sInputFile = args[fileSearchBaseIndex];

            if (args.Length == fileSearchBaseIndex + 1)
                sOutputFile = sInputFile;
            else
                sOutputFile = args[fileSearchBaseIndex + 1];

            if (args.Length > fileSearchBaseIndex + 2)
                throw new ApplicationException("Unknown arguments.");

            return true;
        }

        static int Main(string[] args)
        {
            FileVersionInfo info = FileVersionInfo.GetVersionInfo(Assembly.GetExecutingAssembly().Location);
            Console.WriteLine("MasterConfigUpdater-{0}.{1}.{2} (C) 2013 Electronic Arts", info.FileMajorPart, info.FileMinorPart, info.FileBuildPart);
            Console.WriteLine();

            try
            {
                if (!ValidateArgs(args))
                    return -1;

                UpdateMasterConfig();
            }
            catch (ApplicationException e)
            {
                Exception current = e;
                while (current != null && current.Message.Length > 0)
                {
                    Console.WriteLine(current.Message);
                    current = current.InnerException;
                }
                Console.WriteLine();
                Console.WriteLine("Try 'MasterConfigUpdater help' for more information.");

                return -1;
            }
            catch (Exception e)
            {
                Console.WriteLine("INTERNAL ERROR");
                Console.WriteLine(e.ToString());
                Console.WriteLine();
                Console.WriteLine("Please send bug reports to framework2users@ea.com.");

                return -1;
            }

            return 0;
        }

        static void UpdateMasterConfig()
        {
            if (File.Exists(sOutputFile) && (File.GetAttributes(sOutputFile) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
                throw new ApplicationException(string.Format("Unable to write output file '{0}'. Please ensure it is writable.", sOutputFile));

            PackageServer.WebServices server = PackageServer.WebServicesFactory.Generate();

            XslCompiledTransform xslt = new XslCompiledTransform();

            xslt.Load(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "MasterConfig.xsl"));

            Framework2Extensions extensions = new Framework2Extensions(server, sForce, sNewest);
            XsltArgumentList argList = new XsltArgumentList();
            argList.AddExtensionObject("urn:Framework2", extensions);

            string tempFile = Path.GetTempFileName();
            Stream tempStream = File.Create(tempFile);
            xslt.Transform(sInputFile, argList, tempStream);
            tempStream.Close();

            File.Delete(sOutputFile);
            File.Move(tempFile, sOutputFile);
        }
    }
}
