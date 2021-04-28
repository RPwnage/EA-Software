// (c) Electronic Arts. All Rights Reserved.
//-----------------------------------------------------------------------------
// GetConfigPlatform.cs
//
// Custom task which checks whether an existing solution file 
// contains the given configuration and platform, and on missing
// platform in the solution, assign an existing platform with that config
// to that platform property in the calling project instance
//
//
// (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved
//-----------------------------------------------------------------------------
using System;
using System.Text;
using System.Collections;
using System.Text.RegularExpressions;
using System.IO;
using NAnt.Core;
using NAnt.Core.Attributes;

namespace EA.GetConfigPlatform
{
    [TaskName("GetConfigPlatform")]
    public class GetConfigPlatform : Task
    {
        string _configname = "";
        string _platformname = "";
        string _filename = "";

        [TaskAttribute("configname", Required = false)]
        public string ConfigName
        {
            get { return _configname; }
            set { _configname = value; }
        }
        [TaskAttribute("platformname", Required = false)]
        public string PlatformName
        {
            get { return _platformname; }
            set { _platformname = value; }
        }
        [TaskAttribute("file", Required = true)]
        public string File
        {
            get { return _filename; }
            set { _filename = value; }
        }

        protected override void ExecuteTask()
        {
            FileInfo fo = new FileInfo(_filename);
            StringBuilder fileText = new StringBuilder((int)fo.Length);
            using (StreamReader reader = new StreamReader(_filename))
            {
                String line;
                while ((line = reader.ReadLine()) != null)
                {                    
                    fileText.Append(line);
					fileText.Append(Environment.NewLine);
                }
                reader.Close();
            }
            string fileString = fileText.ToString();
            // Make sure the file is in a supported format
            string version = "2005";
            Match match = Regex.Match(fileString, "Microsoft Visual Studio Solution File, Format Version (\\d+.\\d\\d)");
            if (match.Groups[1].ToString().Equals(@"12.00"))
            {
                version = "2011";
            }
            else
			if (match.Groups[1].ToString().Equals(@"11.00"))
            {
                version = "2010";
            }
            else
            if (match.Groups[1].ToString().Equals(@"10.00"))
            {
                version = "2008";
            }
            else
            if (match.Groups[1].ToString().Equals(@"9.00"))
            {
                version = "2005";
            }
            else 
            if (match.Groups[1].ToString().Equals(@"8.00"))
            {
                version = "2003";
            }
            else
            {
                    string errorString = String.Format("Error: solution {0} is neither VS2008, VS2005 or VS2003 format!", _filename);
                    throw new BuildException(errorString);
            }

            string globalSectionRegxString = "";
            string configPlatformRegxString = "";
            if (version == "2005" || version == "2008" || version == "2010" || version == "2011")
            {
                globalSectionRegxString = "GlobalSection\\(SolutionConfigurationPlatforms\\)\\s*=\\s*preSolution[\\s|\n|\t|\r\n]*(([^\n=]*\\s*=\\s*[^\n=]*[\\s|\n|\t|\r\n]*)*)[\\s|\n|\t|\r\n]*EndGlobalSection";
                configPlatformRegxString = "([^\n=\\|]+)\\|([^\n=\\|]+)\\s*=\\s*[^\n=]+";
            }
            else
            {
                globalSectionRegxString = "GlobalSection\\(SolutionConfiguration\\)\\s*=\\s*preSolution[\\s|\n|\t|\r\n]*(([^\n=]*\\s*=\\s*[^\n=]*[\\s|\n|\t|\r\n]*)*)[\\s|\n|\t|\r\n]*EndGlobalSection";
                configPlatformRegxString = "([^\n=\\s]+) ([^\n=\\s]+)\\s*=\\s*[^\n=]+";
            }

            // Find all solution configurations - preSolution
            match = Regex.Match(fileString, globalSectionRegxString);
            if (match.Success)
            {
                Hashtable slnConfigPlatform = new Hashtable();
                // Example of matching string: ps3-gcc-debug Win32 = ps3-gcc-debug Win32
                MatchCollection slnConfigMatchColl = Regex.Matches(match.Groups[1].ToString().Trim(), configPlatformRegxString);
                for (int i = 0; i < slnConfigMatchColl.Count; i++)
                {
                    string config = slnConfigMatchColl[i].Groups[1].ToString().Trim();
                    string platform = slnConfigMatchColl[i].Groups[2].ToString().Trim();

                    if (!slnConfigPlatform.Contains(config))
                    {
                        ArrayList platforms = new ArrayList();
                        platforms.Add(platform);
                        slnConfigPlatform.Add(config, platforms);
                    }
                    else if (!((ArrayList)slnConfigPlatform[config]).Contains(platform))
                    {
                        ((ArrayList)slnConfigPlatform[config]).Add(platform);
                    }
                }

                string configValue = Project.Properties[_configname];
                string platformValue = Project.Properties[_platformname];
                if (configValue != null && platformValue != null)
                {
                    if (slnConfigPlatform.Contains(configValue))
                    {
                        if (!((ArrayList)slnConfigPlatform[configValue]).Contains(platformValue))
                        {
                            // Set platform to an existing platform in the solution file.
                            Project.Properties[_platformname] = (string) ((ArrayList)slnConfigPlatform[configValue])[0];
                        }
                    }
                }
                else
                    throw new BuildException("ERROR: unrecognized config platform name '" + _configname + "'|'" + _platformname + "'!");

            }
            else
            {
                string errorString = String.Format("ERROR: {0} is missing SolutionConfiguration Section", _filename);
                throw new BuildException(errorString);
            }            
        }
    }
}
