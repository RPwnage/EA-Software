// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.IO;
using System.Reflection;
using System.Diagnostics;
using System.Collections.Generic;

namespace NAnt.Core.Util
{
	public static class PlatformUtil
	{
		public const string Windows = "Windows";
		public const string OSX = "OSX";
		public const string Unix = "Unix";
		public const string Xbox = "Xbox";
		public const string Unknown = "Unknown";

		private static readonly int s_platformId;
		private static Lazy<string> s_currentPlatform = new Lazy<string>(GetCurrentPlatform);
		private static Lazy<string> s_runtimeDirectory = new Lazy<string>(GetRuntimeDirectory);
		private static Lazy<uint> s_commandLineLength = new Lazy<uint>(GetMaxCommandLineLength);

		public static string Platform { get { return s_currentPlatform.Value; } }

		public static bool IsMonoRuntime { get; private set; }
		public static bool IsUnix { get { return Unix == Platform; } }
		public static bool IsWindows { get { return Windows == Platform; } }
		public static bool IsOSX { get { return OSX == Platform; } }

		// Test that you are running under Windows Subsystem for Linux (WSL).  In some situations, it can have different behaviour from real Linux machine.
		public static bool IsOnWindowsSubsystemForLinux { get { return s_isOnWSL; } }
		private static readonly bool s_isOnWSL;

		public static bool IgnoreCase { get { return IsWindows || IsOSX; } }
		public static string RuntimeDirectory { get { return s_runtimeDirectory.Value; } }
		public static uint MaxCommandLineLength { get { return s_commandLineLength.Value; } }

		// This is mainly used for our tools such as copy.exe, mkdir.exe, etc where we expect
		// file path as input and we expect the tool to be used by multiple platform hosts.  
		// On PC, we can have '-' and '/' and starting characters to indicate a command line switch.
		// but on osx and Linux host, full path starts with '/' and cannot be used as a 
		// command line switch character!
		public static char[] HostSupportedCommandLineSwitchChars
		{
			get 
			{
				if (IsOSX || IsUnix)
				{
					return new char[] { '-' };
				}
				else
				{
					return new char[] { '-', '/' };
				}
			}
		}

		static PlatformUtil()
		{
			s_platformId = (int)Environment.OSVersion.Platform;

			// On OSX host, it is always hard coded to Unix.  So we need to do another test for it.
			if (Environment.OSVersion.Platform == System.PlatformID.Unix)
			{
				// The following RuntimeInformation.IsOSPlatform() is only available on and after .Net 4.7.1 (or .Net Core).  But since
				// Framework now build against at least .Net 4.7.2, we should be OK to just use this function.
				if (System.Runtime.InteropServices.RuntimeInformation.IsOSPlatform(System.Runtime.InteropServices.OSPlatform.OSX))
				{
					s_platformId = 6; //MacOSX
				}
			}
			IsMonoRuntime = (Type.GetType("Mono.Runtime") != null);

			bool onWsl = false;
			if (s_platformId == 4 && File.Exists("/proc/sys/kernel/osrelease"))
			{
				// Currently the only way to detect WSL is examine the version file.
				// https://github.com/Microsoft/WSL/issues/423
				string versionContent = File.ReadAllText("/proc/sys/kernel/osrelease");
				if (versionContent != null && versionContent.Contains("Microsoft"))
				{
					onWsl = true;
				}
			}
			s_isOnWSL = onWsl;
		}

		private static string GetCurrentPlatform()
		{
			if (Environment.OSVersion.Platform == PlatformID.Win32NT)
			{
				return Windows;
			}
			else if (Environment.OSVersion.Platform == PlatformID.WinCE)
			{
				return Windows;
			}
			else if (s_platformId == 5) // PlatformID.Xbox
			{
				return Xbox;
			}
			else if (s_platformId == 4) // PlatformID.Unix
			{
				return Unix;
			}
			else if (s_platformId == 6) // PlatformID.MacOSX
			{
				return OSX;
			}		
			return Unknown;
		}

		private static string GetRuntimeDirectory()
		{
			Assembly assembly = Assembly.GetAssembly(typeof(String));
			Uri codeBaseUri = UriFactory.CreateUri(assembly.CodeBase);
			return Path.GetDirectoryName(codeBaseUri.LocalPath);
		}

		private static uint GetMaxCommandLineLength()
		{
			switch (Platform)
			{
				case Unix:
				case OSX:
					return DetermineUnixCommandLineMax();
				case Windows:
					if (Environment.OSVersion.Version.Major >= 6)
					{
						// in windows 7 practical experiments show that command limit
						// is 32767
						return 32767;
					}
					else if (Environment.OSVersion.Version.Major == 5)
					{
						// relying entirely on: http://support.microsoft.com/kb/830473
						return 8191;
					}
					else
					{
						return 2047;
					}
				default:
					throw new InvalidOperationException(String.Format("Could not determine max command line length for platform '{0}'.", Platform.ToString()));
			}
		}

		private static uint DetermineUnixCommandLineMax()
		{
			uint commandLineMaxLength = 262144; // lowest encountered unix command limit so far
			using (Process getConfProcess = new Process())
			{
				getConfProcess.StartInfo.FileName = "getconf";
				getConfProcess.StartInfo.Arguments = "ARG_MAX";
				getConfProcess.StartInfo.UseShellExecute = false;
				getConfProcess.StartInfo.RedirectStandardOutput = true;
				getConfProcess.StartInfo.CreateNoWindow = true;

				getConfProcess.Start();
				if (getConfProcess.WaitForExit(1000))
				{
					string result = getConfProcess.StandardOutput.ReadToEnd();
					if (0 == getConfProcess.ExitCode)
					{
						uint uintOutput;
						if (UInt32.TryParse(result, out uintOutput))
						{
							commandLineMaxLength = uintOutput;
						}
					}
				}
				return commandLineMaxLength;
			}
		}
	}
}
