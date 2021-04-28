// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
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
// Bob Summerwill (bobsummerwill@ea.com)


using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Linq;


namespace RegressNant
{
	// A utility class, containing process-related utility methods.
	public class ProcessUtil
	{
		private static readonly HashSet<string> EXCLUDED_PROCESSES = new HashSet<string>() { "FrameworkTelemetryProcessor.exe", "ecmdrAgent", "explorer" };
		//inner enum used only internally
		[Flags]
		private enum SnapshotFlags : uint
		{
			HeapList = 0x00000001,
			Process = 0x00000002,
			Thread = 0x00000004,
			Module = 0x00000008,
			Module32 = 0x00000010,
			Inherit = 0x80000000,
			All = 0x0000001F
		}
		//inner struct used only internally
		[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
		private struct ProcessEntry32
		{
			const int MAX_PATH = 260;
			internal UInt32 dwSize;
			internal UInt32 cntUsage;
			internal UInt32 th32ProcessID;
			internal IntPtr th32DefaultHeapID;
			internal UInt32 th32ModuleID;
			internal UInt32 cntThreads;
			internal UInt32 th32ParentProcessID;
			internal Int32 pcPriClassBase;
			internal UInt32 dwFlags;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = MAX_PATH)]
			internal string szExeFile;
		}

		[DllImport("kernel32", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		private static extern IntPtr CreateToolhelp32Snapshot([In]UInt32 dwFlags, [In]UInt32 th32ProcessID);

		[DllImport("kernel32", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		private static extern bool Process32First([In]IntPtr hSnapshot, ref ProcessEntry32 lppe);

		[DllImport("kernel32", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		private static extern bool Process32Next([In]IntPtr hSnapshot, ref ProcessEntry32 lppe);

		[DllImport("kernel32", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		private static extern bool CloseHandle([In] IntPtr hObject);


		// A private class, used for the elements in the hashtable.
		private class ProcessInfo
		{
			public string Name;
			public int ParentID;
			public int ProcessID;

			public ProcessInfo(string Name, int ProcessID, int ParentID)
			{
				this.Name = Name;
				this.ParentID = ParentID;
				this.ProcessID = ProcessID;
			}
		};

		private class ProcessInfoEqualityComparer : IEqualityComparer<ProcessInfo>
		{
			public bool Equals(ProcessInfo x, ProcessInfo y)
			{
				return x.ProcessID == y.ProcessID;
			}

			public int GetHashCode(ProcessInfo x)
			{
				return x.ProcessID.GetHashCode();
			}
		}


		// This is the meat of the thing.  It build up a Hashtable of name/parent id
		// pairs for each of the running processes in a system snapshot.  There are
		// a few signed/unsigned cast in this method, because the Win32 API is mainly
		// using unsigned integers while the .NET API is using signed integers.
		private static IEnumerable<ProcessInfo> GatherProcessInfo()
		{
			var Result = new HashSet<ProcessInfo>(new ProcessInfoEqualityComparer());

			// Take a snapshot of the current processes.  This returns a handle
			// which we can use to iterate through each of the processes.
			IntPtr Snapshot = CreateToolhelp32Snapshot((uint)SnapshotFlags.Process, 0);

			// Create a temporary object which we can use to dump the information
			// on each process into.
			ProcessEntry32 Info = new ProcessEntry32();
			Info.dwSize = (uint)Marshal.SizeOf(Info);

			// Get information on the first process.
			bool gotInfo = Process32First(Snapshot, ref Info);

			// Then iterate through each of the processes, creating a hashtable entry
			// for each process, with the name and parent ID for that process.
			while (gotInfo)
			{
				var pi = new ProcessInfo(Info.szExeFile, (int)Info.th32ProcessID, (int)Info.th32ParentProcessID);
				Result.Add(pi);
				gotInfo = Process32Next(Snapshot, ref Info);
			}

			// Close the handle for the snapshot, and return the hashtable we've created.
			CloseHandle(Snapshot);
			return Result;
		}

		private static IEnumerable<ProcessInfo> GetChildProcesses(int ParentProcessID, IEnumerable<ProcessInfo> Processes)
		{
			var Result = new List<ProcessInfo>();

			foreach (var p in Processes)
			{
				if (p.ParentID == ParentProcessID)
				{
					Result.Add(p);
				}
			}
			return Result;
		}

		private static bool IsDescendentOf(int ParentProcessID, Hashtable Processes, ProcessInfo Process)
		{
			while (Process != null && Process.ParentID != 0)
			{
				if (Process.ParentID == ParentProcessID)
				{
					return true;
				}
				Process = Processes[Process.ParentID] as ProcessInfo;
			}
			return false;
		}

		// Look up the name of a given process in the hashtable.  There are
		// methods in the .NET Process class to get at this information, but
		// they don't seem to produce reliable results, so we use the
		// information from the ToolHelp API.
		private static string GetProcessName(int ProcessID, Hashtable Processes)
		{
			ProcessInfo Info = (ProcessInfo)Processes[ProcessID];
			return Info.Name;
		}

		// Recursive function which traverses a process-tree, killing all the
		// child processes before killing the parent process in each case.
		private static void KillProcessTree(int ProcessID, string processName, bool KillParent, bool KillInstantly)
		{
			var pname = processName;
			int pid = ProcessID;
			if (IsWindows && IsDebug)
			{

				try
				{
					using (var procp = Process.GetProcessById(ProcessID))
					{
						if (procp != null)
						{
							if (pname != "unknown" && pname != procp.ProcessName)
							{
								return;
							}


						}
					}
				}
				catch (Exception)
				{
					//Exception is thrown if process already exited
				}
			}

			// Need to keep process object alive to have handle on a child process:

			Process proc = null;
			try
			{
				// Kill the parent so no more children can be spawned.
				if (KillParent)
				{
					if (KillInstantly)
					{
						proc = Process.GetProcessById(ProcessID);
						try
						{


							if (IsDebug)
							{
								Console.WriteLine("Killing process '{0}[{1}]'", proc.ProcessName, proc.Id);
							}
							proc.Kill();
							try
							{
								proc.WaitForExit();
							}
							catch (SystemException)
							{
								// The process has already exited.
							}

						}
						catch (Exception) { };
					}
					else
					{
						KillProcess(ProcessID);
					}
				}


				if (IsWindows)
				{
					var childprocesses = GetChildProcesses(ProcessID, GatherProcessInfo());
					if (IsDebug && childprocesses != null && childprocesses.Count() > 0)
					{
						Console.WriteLine("Killing child processes of '{0}[{1}]' (expected id={2})", pname, pid, ProcessID);
					}
					// Kill your children.
					foreach (ProcessInfo pi in childprocesses)
					{
						if (!EXCLUDED_PROCESSES.Contains(pi.Name ?? String.Empty))
						{
							if (IsDebug)
							{
								Console.WriteLine("Child process '{0}[{1}]' pid={2} (expected pid={3}).", pi.Name, pi.ProcessID, pi.ParentID, ProcessID);
							}
							KillProcessTree(pi.ProcessID, pi.Name, true, KillInstantly);
						}
						else
						{
							if (IsDebug)
							{
								Console.WriteLine("Skipped deleting child process '{0}[{1}]' pid={2} (expected pid={3}).", pi.Name, pi.ProcessID, pi.ParentID, ProcessID);
							}

						}
					}
				}
			}
			finally
			{
				if (proc != null)
				{
					try
					{
						proc.Close();
					}
					catch (Exception) { }
				}
			}
		}

		// Kill all the processes in a given process-tree.
		public static void KillProcessTree(int ProcessID, string processName)
		{
			KillProcessTree(ProcessID, processName, true, false);
		}

		// Kill all the processes in a given process-tree.
		public static void KillProcessTreeInstantly(int ProcessID, string processName)
		{
			KillProcessTree(ProcessID, processName, true, true);
		}


		// Kill all the child processes in a given process-tree.
		public static void KillChildProcessTree(int ProcessID, string processName, bool killInstantly = false)
		{
			KillProcessTree(ProcessID, processName, false, killInstantly);
		}

		// Utility function to kill a process.
		public static void KillProcess(int ProcessID)
		{
			Process processInfo;

			try
			{
				using (processInfo = Process.GetProcessById(ProcessID))
				{

					// Send a terminate request to GUI applications.

					if (processInfo.CloseMainWindow())
					{
						try
						{
							processInfo.WaitForExit(250);
						}
						catch (SystemException)
						{
						}
					}

					try
					{
						if (!processInfo.HasExited)
						{
							// Forcefully terminate the application.
							// We get here if the GUI application did not terminate
							// or if the application is a console application.
							Console.WriteLine("Killing process {0}[{1}] ", processInfo.ProcessName, processInfo.Id);
							processInfo.Kill();

							try
							{
								processInfo.WaitForExit();
							}
							catch (SystemException)
							{
								// The process has already exited.
							}
						}
					}
					catch (InvalidOperationException)
					{
						// The process has already exited.
					}
				}
			}
			catch (ArgumentException)
			{
				// The process is not running.
				return;
			}

		}

		// To get rid of compiler warnings
		private static void Void(Object obj) { }

		public static bool IsDebug = true;

		public static bool IsWindows
		{
			get
			{
				return Environment.OSVersion.Platform == PlatformID.Win32NT
					|| Environment.OSVersion.Platform == PlatformID.WinCE;
			}
		}


		[DllImport("kernel32.dll", SetLastError = true)]
		private static extern bool GenerateConsoleCtrlEvent(ConsoleCtrl.ConsoleEvent sigevent, int dwProcessGroupId);


	}
}
