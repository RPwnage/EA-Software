using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using SetStartupProjects;

namespace VSSetStartupProjects
{
    class Program
    {
        static VisualStudioVersions generateForVSVersion(string vsversion, string sln)
        {
            string vsstring = null;
            VisualStudioVersions version = VisualStudioVersions.All;

            if (vsversion.Equals("2015"))
            {
                vsstring = "v14";
                version = VisualStudioVersions.Vs2015;
            }
            else if (vsversion.Equals("2017"))
            {
                vsstring = "v15";
                version = VisualStudioVersions.Vs2017;
            }
            else if (vsversion.Equals("2019"))
            {
                vsstring = "v16";
                version = VisualStudioVersions.Vs2019;
            }
            else
            {
                throw new ArgumentException($"Unsupported Visual Studio version: {vsversion}", "vsversion");
            }

            var solutionName = Path.GetFileNameWithoutExtension(sln);
            var suoFilePath = Path.Combine(Path.GetDirectoryName(sln), ".vs", solutionName, vsstring, ".suo");
            if (File.Exists(suoFilePath))
            {
                Console.WriteLine($".suo file {suoFilePath} already exists, and will not be regenerated.");
                return VisualStudioVersions.All;
            }

            return version;
        }

        static void Main(string[] args)
        {
            if (args.Length < 4)
            {
                throw new ArgumentException($"Minimum 4 arguments required (<sln_filepath> <vsversion> <startup_project_1> <startup_project_2> ...)");
            }

            string sln = args[0];
            VisualStudioVersions vsversion = generateForVSVersion(args[1], sln);
            if (vsversion == VisualStudioVersions.All)
                return;

            var startupProjects = new List<string> { };
            for (int i = 2; i < args.Length; ++i)
                startupProjects.Add(args[i]);

            var startupProjectGuids = new List<string> { };
            var allprojects = SolutionProjectExtractor.GetAllProjectFiles(sln).ToList();
            foreach (var project in allprojects)
            {
                var index = startupProjects.FindIndex(x => x.Equals(Path.GetFileNameWithoutExtension(project.RelativePath)));
                if (index >= 0)
                {
                    startupProjectGuids.Add(project.Guid);
                    if (startupProjects.Count == 1)
                        break;
                    startupProjects.RemoveAt(index);
                }
            }

            var startProjectSuoCreator = new StartProjectSuoCreator();
            startProjectSuoCreator.CreateForSolutionFile(sln, startupProjectGuids, vsversion);
        }
    }
}
