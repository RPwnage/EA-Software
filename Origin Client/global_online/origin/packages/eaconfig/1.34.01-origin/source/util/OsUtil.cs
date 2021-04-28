using NAnt.Core;

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

namespace EA.Eaconfig
{
    public class OsUtil
    {
        // Some versions of .Net or Mono do not have full enumeration. 
        // Define enum here and use int type in public interfaces.
        public enum OsUtilPlatformID
        {
            Win32S = 0,
            Win32Windows = 1,
            Win32NT = 2,
            WinCE = 3,
            Unix = 4,
            Xbox = 5,
            MacOSX = 6,
        }

        private static readonly int platformId;

        static OsUtil()        
        {
            platformId = (int)Environment.OSVersion.Platform;

            if (Environment.OSVersion.Platform == System.PlatformID.Unix)
            {
                // need to execute uname -s to find out if it is a MAC or not   
			    Process p = new Process();
			    p.StartInfo.FileName  = "uname";
			    p.StartInfo.Arguments = "-s";
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.CreateNoWindow = true;

                p.Start();
                if (p.WaitForExit(1000))
                {
                    string result = p.StandardOutput.ReadToEnd();
                    if (0 == p.ExitCode && (result.StartsWith("Darwin")))
                    {
                        platformId = (int)OsUtilPlatformID.MacOSX;
                    }
                }
            }
        }

        public static int PlatformID
        {
            get 
            {
                return platformId;
            }
        }
    }
}


