// Copyright (c) 2007 Electronic Arts
using System;
using System.IO;
using System.Reflection;
using System.Diagnostics;

namespace NAnt.Core.Util
{
    public class PlatformUtil
    {
        public const string Windows = "Windows";
        public const string OSX = "OSX";
        public const string Unix = "Unix";
        public const string Xbox = "Xbox";
        public const string Unknown = "Unknown";

        PlatformUtil()
        {
        }
        static PlatformUtil()
        {
            _platformId = (int)Environment.OSVersion.Platform;

            if (Environment.OSVersion.Platform == System.PlatformID.Unix)
            {
                // need to execute uname -s to find out if it is a MAC or not   
                Process p = new Process();
                p.StartInfo.FileName = "uname";
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
                        _platformId = 6; //MacOSX;
                    }
                }
            }
            _isMonoRuntime = (Type.GetType ("Mono.Runtime") != null); 
        }

        static object SyncObject = typeof(PlatformUtil);
        static string _RuntimeDirectory;
        static string _currentPlatform;
        static bool   _isMonoRuntime;

        private static readonly int _platformId;

        public static string Platform
        {
            get
            {
                if (_currentPlatform == null)
                {
                    int p = _platformId;

                    if (Environment.OSVersion.Platform == PlatformID.Win32NT)
                    {
                        _currentPlatform = Windows;
                    }
                    else if (Environment.OSVersion.Platform == PlatformID.WinCE)
                    {
                        _currentPlatform = Windows;
                    }
                    else if (p == 5) // PlatformID.Xbox
                    {
                        _currentPlatform = Xbox;
                    }
                    else if (p == 4) // PlatformID.Unix
                    {
                        _currentPlatform = Unix;
                    }
                    else if (p == 6) // PlatformID.MacOSX
                    {
                        _currentPlatform = OSX;
                    }

                    if (_currentPlatform == null)
                        _currentPlatform = Unknown;
                }
                return _currentPlatform;
            }
        }

        public static bool IsUnix
        {
            get { return Unix == Platform; }
        }

        public static bool IsWindows
        {
            get { return Windows == Platform; }
        }

        public static bool IsOSX
        {
            get { return OSX == Platform; }
        }

        public static bool IgnoreCase
        {
            get { return IsWindows || IsOSX; }
        }

        public static string RuntimeDirectory
        {
            get
            {
                if (_RuntimeDirectory == null)
                {
                    lock (SyncObject)
                    {
                        if (_RuntimeDirectory == null)
                        {
                            Assembly assembly = Assembly.GetAssembly(typeof(String));
                            Uri codeBaseUri = UriFactory.CreateUri(assembly.CodeBase);
                            _RuntimeDirectory = Path.GetDirectoryName(codeBaseUri.LocalPath);
                        }
                    }
                }
                return _RuntimeDirectory;
            }
        }

        public static bool IsMonoRuntime
        {
            get { return _isMonoRuntime; }
        }
    }
}
