using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Attributes;

[TaskName("mkver")]
public class MkverTask : Task
{
    string m_outdir = "";
    bool m_pregen = false;

    public MkverTask()
        : base("mkver")
    {
    }

    [TaskAttribute("outdir", Required=true)]
    public string OutDir
    {
        get { return m_outdir; }
        set { m_outdir = value; }
    }

    [TaskAttribute("pregen", Required=true)]
    public bool PreGen
    {
        get { return m_pregen; }
        set { m_pregen = value; }
    }

    private static int _ExecCmd(string name, string arg, ref string output)
    {
        Process p = new Process();
        p.StartInfo.UseShellExecute = false;
        p.StartInfo.RedirectStandardOutput = true;
        p.StartInfo.RedirectStandardError = true;
        p.StartInfo.FileName = name;
        p.StartInfo.Arguments = arg;
        p.Start();
        output = p.StandardOutput.ReadToEnd();
        p.WaitForExit();
        return(p.ExitCode);
    }

    private string _GetChangeList(string filePath)
    {
        string changeList = "";
        _ExecCmd("p4", "changes -m1 " + filePath + "/...#have", ref changeList);
        if (changeList != "")
        {
            string[] changeListSplit = changeList.Split();
            changeList = changeListSplit[1];
        }
        else
        {
            Log.Warning.WriteLine(string.Format("{0} Could not acquire changelist for version string using '{1}' via perforce. Please check perforce connection. Defaulting to '?' changelist number.",
                LogPrefix, filePath));
            changeList = "?";
        }
        return(changeList);
    }

    private string _GetSdkVersion(string path)
    {
        try
        {
            string versionFile = File.ReadAllText(path);
            Match match = Regex.Match(versionFile, @"(\d+)\.(\d+)\.(\d+)\.(\d+)\.(\d+)");
            if (match.Success)
            {
                return(match.Value);
            } 
            else
            {
                Log.Warning.WriteLine(string.Format("{0} Could not match version number regex when parsing '{1}' for generating version string. Did the version number format change? Defaulting to '?' version number.",
                    LogPrefix, path));
                return("?");
            }
        }
        catch (Exception e)
        {
            Log.Warning.WriteLine(string.Format("{0} Reading SDK version for generating version string failed with: {1} Defaulting to '?' version number.",
                LogPrefix, e.Message));
            return("?");
        }
    }

    protected override void ExecuteTask()
    {
        // about the environment
        string config = Project.Properties["config"];
        string platform = Project.Properties["config-system"];

        // get the build directory
        string buildDir = Project.Properties["package.dir"].Replace("\\", "/");

        // version filename
        string fileName = OutDir + "/version.c";

        // generate version filename (if enabled)
        string pregenFilename = Project.Properties["package.builddir"] + "/pregen_version";
        string pregenContents = File.Exists(pregenFilename) ? File.ReadAllText(pregenFilename) : "";
        string generatedVersion = "";

        if (PreGen || (pregenContents == ""))
        {
        // get sdk versions
        string dirtySdkVer = _GetSdkVersion(Project.Properties["package.DirtySDK.dir"] + "/version.txt");
        string blazeSdkVer = _GetSdkVersion(Project.Properties["package.BlazeSDK.dir"] + "/readme.md");
        string dirtyCastVer = _GetSdkVersion(Project.Properties["package.dir"] + "/version.txt");

        // fallback to readme.txt for blazesdk for older versions
        if (blazeSdkVer == "?")
        {
            Log.Status.WriteLine(string.Format("{0} Attempting to fallback to old BlazeSDK readme", LogPrefix));
            blazeSdkVer = _GetSdkVersion(Project.Properties["package.BlazeSDK.dir"] + "/readme.txt");
        }

        // get changelist numbers
        string dirtySdkCl = Project.Properties["package.DirtySDK.version"] == "flattened" ? _GetChangeList(Project.Properties["package.DirtySDK.dir"]) : "ondemand";
        string blazeSdkCl = Project.Properties["package.BlazeSDK.version"] == "flattened" ? _GetChangeList(Project.Properties["package.BlazeSDK.dir"]) : "ondemand";
        string dirtyCastCl = _GetChangeList(Project.Properties["package.dir"]);

        // concat changelist numbers to version strings
        dirtySdkVer += "/" + dirtySdkCl;
        blazeSdkVer += "/" + blazeSdkCl;
        dirtyCastVer += "/" + dirtyCastCl;

            generatedVersion = string.Format("DirtySDK-{0} BlazeSDK-{1} DirtyCast-{2}", dirtySdkVer, blazeSdkVer, dirtyCastVer);
        }
        else
        {
            generatedVersion = pregenContents;
        }

        // create directory if required
        if (!Directory.Exists(OutDir))
        {
            Directory.CreateDirectory(OutDir);
        }

        string generatedData = string.Join(Environment.NewLine, new string[] {
            string.Format("const char _ServerBuildTime[] = __DATE__ \" \" __TIME__;"),
            string.Format("const char _ServerBuildLocation[] = \"{0}@{1}:{2}\";", Environment.UserName, Environment.MachineName, buildDir),
            string.Format("const char _SDKVersion[] = \"{0} {1}\";", config, generatedVersion)
        });
        string versionFile = File.Exists(fileName) ? File.ReadAllText(fileName) : "";

        if (PreGen && (pregenContents != generatedVersion))
        {
            Log.Status.WriteLine(LogPrefix + "Generating version file (pregen): " + generatedVersion);

            using (StreamWriter file = new StreamWriter(pregenFilename, false))
            {
                file.Write(generatedVersion);
            }
        }

        if (!PreGen && (generatedData != versionFile))
        {
            Log.Status.WriteLine(LogPrefix + "Generating version file: " + generatedVersion);

            // create version file
            using (StreamWriter file = new StreamWriter(fileName, false))
            {
                file.Write(generatedData);
            }
        }
    }
}
