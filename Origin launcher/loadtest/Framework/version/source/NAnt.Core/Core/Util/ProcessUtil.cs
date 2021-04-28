// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)
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
using System.Threading;
using NAnt.Core.Logging;

using Mono.Unix.Native;

namespace NAnt.Core.Util
{
	// A utility class, containing process-related utility methods.
	public class ProcessUtil
	{
		private static readonly HashSet<string> EXCLUDED_PROCESSES = new HashSet<string>() { "FrameworkTelemetryProcessor.exe" };

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
		private static void KillProcessTree(int ProcessID, bool KillParent, bool KillInstantly)
		{
			// Kill the parent so no more children can be spawned.
			if (KillParent)
			{
				if (KillInstantly)
				{
					try
					{
						using (var proc = Process.GetProcessById(ProcessID))
						{
							proc.Kill();
						}
					}
					catch (Exception) { };
				}
				else
				{
					KillProcess(ProcessID);
				}
			}

			if (PlatformUtil.IsWindows)
			{
				// Kill your children.
				foreach (ProcessInfo pi in GetChildProcesses(ProcessID, GatherProcessInfo()))
				{
					if (!EXCLUDED_PROCESSES.Contains(pi.Name ?? String.Empty))
					{
						KillProcessTree(pi.ProcessID, true, KillInstantly);
					}
				}
			}
		}

		// Kill all the processes in a given process-tree.
		public static void KillProcessTree(int ProcessID)
		{
			KillProcessTree(ProcessID, true, false);
		}

		// Kill all the processes in a given process-tree.
		public static void KillProcessTreeInstantly(int ProcessID)
		{
			KillProcessTree(ProcessID, true, true);
		}


		// Kill all the child processes in a given process-tree.
		public static void KillChildProcessTree(int ProcessID, bool killInstantly = false)
		{
			KillProcessTree(ProcessID, false, killInstantly);
		}

		// Utility function to kill a process.
		public static void KillProcess(int ProcessID)
		{
			try
			{
				using (Process processInfo = Process.GetProcessById(ProcessID))
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
							// The process has already exited.
						}
					}

					try
					{
						if (!processInfo.HasExited)
						{
							// Forcefully terminate the application.
							// We get here if the GUI application did not terminate
							// or if the application is a console application.
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


		private const int TOKEN_QUERY = 0X00000008;
		private const int ERROR_INSUFFICIENT_BUFFER = 0x7A;

		// const int ERROR_NO_MORE_ITEMS = 259;

		enum TOKEN_INFORMATION_CLASS
		{
			TokenUser = 1,
			TokenGroups,
			TokenPrivileges,
			TokenOwner,
			TokenPrimaryGroup,
			TokenDefaultDacl,
			TokenSource,
			TokenType,
			TokenImpersonationLevel,
			TokenStatistics,
			TokenRestrictedSids,
			TokenSessionId
		}

		struct TOKEN_USER
		{
			// Pragma below is to disable a warning that the field is never
			// initialized and will always have its default value. Uses of
			// this structure are via functions that fill out its fields 
			// with P/Invoke calls so it will eventually have a value, 
			// just not assigned by this code.
#pragma warning disable 0649
			public SID_AND_ATTRIBUTES User;
#pragma warning restore 0649
		}

		[StructLayout(LayoutKind.Sequential)]
		struct SID_AND_ATTRIBUTES
		{
			public IntPtr Sid;
			public int Attributes;
		} 

		[DllImport("advapi32")]
		static extern bool OpenProcessToken(IntPtr ProcessHandle, int DesiredAccess, ref IntPtr TokenHandle);

		[DllImport("advapi32", CharSet = CharSet.Auto, SetLastError = true)]
		static extern bool GetTokenInformation(IntPtr tokenHandle, TOKEN_INFORMATION_CLASS tokenInfoClass, IntPtr tokenInformation, int tokeInfoLength, ref int reqLength);

		[DllImport("advapi32", CharSet = CharSet.Auto, SetLastError=true)]
		static extern bool ConvertSidToStringSid(IntPtr pSID, [In, Out, MarshalAs(UnmanagedType.LPTStr)] ref string pStringSid);


		public static string GetProcessOwnerSID(Process process)
		{
			int Access = TOKEN_QUERY;
			IntPtr procToken = IntPtr.Zero;
			string SID = null;
			try
			{
				// We put this in a try catch block in case the process just terminated while we're trying to open it and get it's user info.
				IntPtr processHandle = process.Handle;
				if (OpenProcessToken(processHandle, Access, ref procToken))
				{
					SID = GetUserSidFromProcessToken(procToken);
					CloseHandle(procToken);
				}
			}
			catch (Exception)
			{
			}
			return SID;
		}


		private static string GetUserSidFromProcessToken(IntPtr procToken)
		{
			string stringSID = null;
			IntPtr tokenInfo = IntPtr.Zero;
			try
			{
				int ln = 0;
				if (!GetTokenInformation(procToken, TOKEN_INFORMATION_CLASS.TokenUser, IntPtr.Zero, 0, ref ln) &&
					 Marshal.GetLastWin32Error() != ERROR_INSUFFICIENT_BUFFER)
				{
					return stringSID;
				}
				tokenInfo = Marshal.AllocHGlobal(ln);

				if (GetTokenInformation(procToken, TOKEN_INFORMATION_CLASS.TokenUser, tokenInfo, ln, ref ln))
				{
					TOKEN_USER tokenUser = (TOKEN_USER)Marshal.PtrToStructure(tokenInfo, typeof(TOKEN_USER));
					IntPtr SID = tokenUser.User.Sid;

					ConvertSidToStringSid(SID, ref stringSID);
				}
			}
			catch (Exception)
			{
			}
			finally
			{
				if (tokenInfo != IntPtr.Zero)
				{
					Marshal.FreeHGlobal(tokenInfo);
				}
			}

			return stringSID;
		}


		//NOTICE (Rashin):
		//This is a hack to get around the fact that StartInfo.EnvironmentVariables
		//is a StringDictionary, which lowercases all keys. This is a bug
		//that should be fixed on Microsoft/Mono's end, as this basically
		//precludes the usage of environment variables on UNIX-like systems.
		public static void SetEnvironmentVariables(IDictionary<string, string> envMap, ProcessStartInfo startInfo, bool clearEnv, Log log)
		{
			try
			{
				// Access to startInfo.EnvironmentVariables populates EnvironmentVariables hash table.
				// Sometimes on Windows parent environment contains same name with different case which leads to exception.
				// We will populate the table below, try to access startInfo.EnvironmentVariables to create internal instance of 
				// HashTable and catch exception.
				Void(startInfo.EnvironmentVariables);
			}
			catch (Exception ex) { Void(ex); }

			var keyDict = new StringDictionary();

			FieldInfo[] fields = startInfo.EnvironmentVariables.GetType().GetFields(BindingFlags.NonPublic | BindingFlags.Instance);
			Hashtable envTable = null;
			if (fields.Length == 1
				&& fields[0].FieldType.Equals(typeof(Hashtable)))
			{
				envTable = fields[0].GetValue(startInfo.EnvironmentVariables) as Hashtable;

				envTable.Clear();
				//register existing environment keys in dictionary
				foreach (DictionaryEntry entry in System.Environment.GetEnvironmentVariables())
				{
					if (PlatformUtil.IsWindows && keyDict.ContainsKey((string)entry.Key))
					{
						envTable[keyDict[(string)entry.Key]] = (string)entry.Value;
					}
					else
					{
						keyDict[(string)entry.Key] = (string)entry.Key;
						envTable[entry.Key] = entry.Value;
					}
				}
			}
			else if (PlatformUtil.IsWindows == false)
			{
				if (log != null)
				{
					log.Warning.WriteLine("Environment variables are not case sensitive which will affect UNIX-like systems.");
				}
			}

			if (clearEnv)
				startInfo.EnvironmentVariables.Clear();

			foreach (KeyValuePair<string, string> entry in envMap)
			{
				if (envTable != null)
				{
					if (PlatformUtil.IsWindows && keyDict.ContainsKey(entry.Key))
					{
						envTable[keyDict[entry.Key]] = entry.Value;
					}
					else
					{
						keyDict[entry.Key] = entry.Key;
						envTable[entry.Key] = entry.Value;
					}
				}
				else
				{
					startInfo.EnvironmentVariables[entry.Key] = entry.Value;
				}
			}
		}

		public static void SafeInitEnvVars(ProcessStartInfo startInfo)
		{
			try
			{
				// Access to startInfo.EnvironmentVariables populates EnvironmentVariables hash table.
				// Sometimes on Windows parent environment contains same name with different case which leads to exception.
				// We will populate the table below, try to access startInfo.EnvironmentVariables to create internal instance of 
				// HashTable and catch exception.
				ProcessUtil.Void(startInfo.EnvironmentVariables);
			}
			catch (Exception ex) { ProcessUtil.Void(ex); }

		}

		public static bool IsUnixExeBitSet(string prog, string workingdir = null)
		{
			// On Windows Platform, there are no exe bit.  So should have just return true as it doesn't matter.
			if (!PlatformUtil.IsUnix && !PlatformUtil.IsOSX)
				return true;

			Stat statObj;
			if (Syscall.stat(prog, out statObj) != 0)
			{
				// If it returns false but didn't throw exception, just throw another exception.
				throw new BuildException(string.Format("Mono.Unix.Native.Syscall.stat is unable to determind file permission for '{0}'", prog));
			}

			FilePermissions permissions = statObj.st_mode;
			return ((permissions & FilePermissions.S_IXUSR) == FilePermissions.S_IXUSR);
		}

		// To get rid of compiler warnings
		private static void Void(Object obj) { }

		public static string DecodeProcessExitCode(string program, int exitcode)
		{
			return String.Format("External program ({0}) returned {1}.", program, DecodeProcessExitCode(exitcode));
		}

		public static string DecodeProcessExitCode(int exitcode)
		{
			string exitMessage = null;

			for (int i = 0; i < mStatusCodes.Length; ++i)
			{
				if (mStatusCodes[i].mStatusCode == exitcode)
				{
					exitMessage = String.Format("{0} (exit code={1} / 0x{1:x})", mStatusCodes[i].mStatus, exitcode);
					break;
				}
			}

			if (exitMessage == null)
			{
				exitMessage = String.Format("(exit code={0})", exitcode);
			}

			return exitMessage;
		}


		private struct StatusCode
		{
			public int mStatusCode;
			public string mStatus;

			public StatusCode(int statusCode, string status)
			{
				mStatusCode = statusCode;
				mStatus = status;
			}
		};

		private static readonly StatusCode[] mStatusCodes = new StatusCode[] {
			new StatusCode(unchecked((int)0x80000001), "STATUS_GUARD_PAGE_VIOLATION"),
			new StatusCode(unchecked((int)0x80000002), "STATUS_DATATYPE_MISALIGNMENT"),
			new StatusCode(unchecked((int)0x80000003), "STATUS_BREAKPOINT"),
			new StatusCode(unchecked((int)0x80000004), "STATUS_SINGLE_STEP"),
			new StatusCode(unchecked((int)0xC0000005), "STATUS_ACCESS_VIOLATION"),
			new StatusCode(unchecked((int)0xC0000006), "STATUS_IN_PAGE_ERROR"),
			new StatusCode(unchecked((int)0xC0000008), "STATUS_INVALID_HANDLE"),
			new StatusCode(unchecked((int)0xC000000D), "STATUS_INVALID_PARAMETER"),
			new StatusCode(unchecked((int)0xC0000017), "STATUS_NO_MEMORY"),
			new StatusCode(unchecked((int)0xC000001D), "STATUS_ILLEGAL_INSTRUCTION"),
			new StatusCode(unchecked((int)0xC0000025), "STATUS_NONCONTINUABLE_EXCEPTION"),
			new StatusCode(unchecked((int)0xC0000026), "STATUS_INVALID_DISPOSITION"),
			new StatusCode(unchecked((int)0xC000008C), "STATUS_ARRAY_BOUNDS_EXCEEDED"),
			new StatusCode(unchecked((int)0xC000008D), "STATUS_FLOAT_DENORMAL_OPERAND"),
			new StatusCode(unchecked((int)0xC000008E), "STATUS_FLOAT_DIVIDE_BY_ZERO"),
			new StatusCode(unchecked((int)0xC000008F), "STATUS_FLOAT_INEXACT_RESULT"),
			new StatusCode(unchecked((int)0xC0000090), "STATUS_FLOAT_INVALID_OPERATION"),
			new StatusCode(unchecked((int)0xC0000091), "STATUS_FLOAT_OVERFLOW"),
			new StatusCode(unchecked((int)0xC0000092), "STATUS_FLOAT_STACK_CHECK"),
			new StatusCode(unchecked((int)0xC0000093), "STATUS_FLOAT_UNDERFLOW"),
			new StatusCode(unchecked((int)0xC0000094), "STATUS_INTEGER_DIVIDE_BY_ZERO"),
			new StatusCode(unchecked((int)0xC0000095), "STATUS_INTEGER_OVERFLOW"),
			new StatusCode(unchecked((int)0xC0000096), "STATUS_PRIVILEGED_INSTRUCTION"),
			new StatusCode(unchecked((int)0xC00000FD), "STATUS_STACK_OVERFLOW"),
			new StatusCode(unchecked((int)0xC0000135), "STATUS_DLL_NOT_FOUND"),
			new StatusCode(unchecked((int)0xC0000138), "STATUS_ORDINAL_NOT_FOUND"),
			new StatusCode(unchecked((int)0xC0000139), "STATUS_ENTRYPOINT_NOT_FOUND"),
			new StatusCode(unchecked((int)0xC000013A), "STATUS_CONTROL_C_EXIT"),
			new StatusCode(unchecked((int)0xC0000142), "STATUS_DLL_INIT_FAILED"),
			new StatusCode(unchecked((int)0xC0000194), "STATUS_POSSIBLE_DEADLOCK"),
			new StatusCode(unchecked((int)0xC00002B4), "STATUS_FLOAT_MULTIPLE_FAULTS"),
			new StatusCode(unchecked((int)0xC00002B5), "STATUS_FLOAT_MULTIPLE_TRAPS"),
			new StatusCode(unchecked((int)0xC00002C9), "STATUS_REG_NAT_CONSUMPTION"),
			new StatusCode(unchecked((int)0xC0000409), "STATUS_STACK_BUFFER_OVERRUN"),
			new StatusCode(unchecked((int)0xC0000417), "STATUS_INVALID_CRUNTIME_PARAMETER"),
			new StatusCode(unchecked((int)0xC0000420), "STATUS_ASSERTION_FAILURE")
		};


		[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
		struct STARTUPINFOEX
		{
			public STARTUPINFO StartupInfo;
			public IntPtr lpAttributeList;
		}

		[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
		struct STARTUPINFO
		{
			public uint cb;
			public string lpReserved;
			public string lpDesktop;
			public string lpTitle;
			public uint dwX;
			public uint dwY;
			public uint dwXSize;
			public uint dwYSize;
			public uint dwXCountChars;
			public uint dwYCountChars;
			public uint dwFillAttribute;
			public uint dwFlags;
			public short wShowWindow;
			public short cbReserved2;
			public IntPtr lpReserved2;
			public IntPtr hStdInput;
			public IntPtr hStdOutput;
			public IntPtr hStdError;
		}

		[StructLayout(LayoutKind.Sequential)]
		internal struct PROCESS_INFORMATION
		{
			public IntPtr hProcess;
			public IntPtr hThread;
			public int dwProcessId;
			public int dwThreadId;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct SECURITY_ATTRIBUTES
		{
			public int nLength;
			public IntPtr lpSecurityDescriptor;
			public bool bInheritHandle;
		}

		[DllImport("kernel32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		static extern bool CreateProcess(
			string lpApplicationName, string lpCommandLine, ref SECURITY_ATTRIBUTES lpProcessAttributes,
			ref SECURITY_ATTRIBUTES lpThreadAttributes, bool bInheritHandles, uint dwCreationFlags,
			IntPtr lpEnvironment, string lpCurrentDirectory, [In] ref STARTUPINFO lpStartupInfo,
			out PROCESS_INFORMATION lpProcessInformation);

		public static void CreateDetachedProcess(string program, string args, string workingdir, IDictionary<string,string> environment, bool clearEnv)
		{
			bool res = false;


			if (Environment.OSVersion.Platform == PlatformID.Win32NT)
			{
				res = CreateDetachedProcessWin(program, args, workingdir, environment, clearEnv);
			}
			else
			{
				res = CreateDetachedProcessUnix(program, args, workingdir, environment, clearEnv);
			}
			if (!res)
			{
				var msg = String.Format("Failed to start detached process \"{0} {1}\"", program, args);
				throw new BuildException(msg);
			}
		}

		private static bool CreateDetachedProcessWin(string program, string args, string workingdir, IDictionary<string,string> environment, bool clearEnv)
		{
			var res = false;

			const uint DETACHED_PROCESS = 0x00000008;

			var pInfo = new PROCESS_INFORMATION();
			var sInfo = new STARTUPINFO();
			sInfo.cb = (uint)Marshal.SizeOf(sInfo);
			var pSec = new SECURITY_ATTRIBUTES();
			var tSec = new SECURITY_ATTRIBUTES();
			pSec.nLength = Marshal.SizeOf(pSec);
			tSec.nLength = Marshal.SizeOf(tSec);
			var lpApplicationName = program;
			var lpCommandLine = String.Format("{0} {1}", program.Quote(), args).TrimWhiteSpace();
			var lpCurrentDirectory = workingdir.TrimWhiteSpace();
			if(String.IsNullOrEmpty(lpCurrentDirectory))
			{
				lpCurrentDirectory = null;
			}

			var lpEnvironment = IntPtr.Zero;
			try
			{
				if (environment != null || clearEnv)
				{
					var envStr = environment.ToString("\0", pair => String.Format("{0}={1}", pair.Key, pair.Value)) + "\0";

					lpEnvironment = Marshal.StringToHGlobalAnsi(envStr);
				}

				res = CreateProcess(
					lpApplicationName,
					lpCommandLine,
					ref pSec,
					ref tSec,
					false,  // Set bInheritHandles to FALSE 
					DETACHED_PROCESS,
					lpEnvironment, // Use parent's environment block 
					lpCurrentDirectory,
					ref sInfo,
					out pInfo);

//                if (!res)
//                {
//                    int err = Marshal.GetLastWin32Error();
//                }
			}
			finally
			{
				if (lpEnvironment != IntPtr.Zero)
				{
					Marshal.FreeHGlobal(lpEnvironment);
				}
			}

			return res;
		}

		private static bool CreateDetachedProcessUnix(string program, string args, string workingdir, IDictionary<string,string> environment, bool clearEnv)
		{
			var res = false;
			var unix_program = Path.Combine(PathUtil.GetExecutingAssemblyLocation(), "startdetachedprocess.sh");

			// Make sure that the shell script has the execute bit set.  Other teams might have checked in that
			// script file to their own perforce without that exe bit being set.  If the exe bit
			// wasn't set, process.Start() will just failed without returning false.
			if (File.Exists(unix_program))
			{
				using (var chmodProcess = new Process())
				{
					chmodProcess.StartInfo.FileName = "chmod";
					chmodProcess.StartInfo.Arguments = String.Format("ugo+x \"{0}\"", unix_program);
					chmodProcess.StartInfo.WorkingDirectory = workingdir;
					chmodProcess.StartInfo.RedirectStandardOutput = false;
					chmodProcess.StartInfo.RedirectStandardError = false;
					chmodProcess.StartInfo.RedirectStandardInput = false;
					chmodProcess.StartInfo.UseShellExecute = false;
					chmodProcess.StartInfo.CreateNoWindow = true;
					chmodProcess.Start();
					// We need to wait for the system to finish updating the exe bit before
					// we can continue.
					chmodProcess.WaitForExit();
				}
			}

			// Start bash script that will fork a process
			using (var process = new Process())
			{
				process.StartInfo.FileName = unix_program;
				process.StartInfo.Arguments = args;
				process.StartInfo.WorkingDirectory = workingdir;
				process.StartInfo.RedirectStandardOutput = false;
				process.StartInfo.RedirectStandardError = false;
				process.StartInfo.RedirectStandardInput = false;
				process.StartInfo.UseShellExecute = false;
				process.StartInfo.CreateNoWindow = true;

				ProcessUtil.SetEnvironmentVariables(environment, process.StartInfo, clearEnv, null);
				
				res = process.Start();
			}

			return res;
		}

		public interface IProcessGroup
		{
			void Kill();
			void Start();
		}

		// JobProcessGroup uses the Win32 job objects to track all processes
		// spawned by a created process. The job object allows the children
		// to be terminated even if they are zombies.
		class JobProcessGroup : IProcessGroup
		{
			IntPtr m_jobHandle;
			Process m_process;

			[DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
			private static extern IntPtr CreateJobObject(IntPtr securityAttributes, string name);

			[DllImport("kernel32", SetLastError = true)]
			[return: MarshalAs(UnmanagedType.Bool)]
			private static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);

			[DllImport("kernel32", SetLastError = true)]
			[return: MarshalAs(UnmanagedType.Bool)]
			private static extern bool TerminateJobObject(IntPtr job, UInt32 exitCode);

			public JobProcessGroup(Process process)
			{
				m_process = process;
			}

			public void Kill()
			{
				if (m_process != null && m_jobHandle != new IntPtr(0))
				{
					TerminateJobObject(m_jobHandle, (uint)m_process.ExitCode);

					CloseHandle(m_jobHandle);
					m_jobHandle = new IntPtr(0);
				}

				m_process = null;
			}

			public void Start()
			{
				IntPtr jobHandle = CreateJobObject(new IntPtr(0), null);

				// This may be problematic if the child process terminates
				// before it can be assigned to the job object. A better
				// solution is to start the child process with the main
				// thread suspended, however doing so cannot be done from 
				// .NET and would require p/invoking to CreateProcess. Which
				// has it's own problems because marshalling IO needs to be
				// done by hand, etc.
				//
				// If this is a problem it may just be easier to catch the
				// exception thrown by AssignProcessToJobObject.
				m_process.Start();

				if (jobHandle != new IntPtr(0))
				{
					m_jobHandle = jobHandle;

					IntPtr processHandle = m_process.Handle;
					AssignProcessToJobObject(jobHandle, processHandle);
				}
			}
		}

		// This "vanilla" process group code is what Framework normally uses for
		// tracking processes and their spawn. The clean-up by killing a process
		// tree may not work reliably for cases when processes are detached from
		// their parents, or their parents no longer exist.
		class ProcessGroup : IProcessGroup
		{
			Process m_process;

			public ProcessGroup(Process process)
			{
				m_process = process;
			}

			public void Kill()
			{
				// if an exception is thrown during process start, then process object may never have associated process
				// however kill will still get called during .Dispose of ProcessUtil - handle these exceptions here
				// so they don't mask initial exception
				bool processHasExited = true;
				try
				{
					processHasExited = m_process.HasExited;
				}
				catch (InvalidOperationException) { }
				catch (SystemException) { }

				if (m_process != null && !processHasExited)
				{
					// Invoking Process.CloseMainWindow() causes child processes to remain active, with their process object "HasExited == true".
					// Use instant kill.
					ProcessUtil.KillProcessTreeInstantly(m_process.Id);
				}
			}

			public void Start()
			{
				m_process.Start();
			}
		}

		public static IProcessGroup CreateProcessGroup(Process process, bool useJobObject)
		{
			// Job objects are a Windows-only construct, so non-Windows 
			// environments use the historic NAnt process spawn and cleanup 
			// paths.
			if (PlatformUtil.IsWindows && useJobObject)
			{
				return new JobProcessGroup(process);
			}

			return new ProcessGroup(process);
		}

		public static IProcessGroup CreateProcessGroup(Process process)
		{
			// devenv is unsanitary when it comes to launching sub processes. 
			// Some of the processes that devenv spawns prevent the termination
			// of the top level instance which can cause hangs on the build farm. 
			//
			// Capturing devenv in a job object will allow Framework to forcefully 
			// terminate all children it launches, assuming that devenv's spawn
			// do not use the CREATE_BREAKAWAY_FROM_JOB process creation flag.
			//
			// Currently this only runs devenv/msbuild in a job. Other processes (ie: 
			// cl.exe instances that spawn mspdbsrv) do not run in a job 
			// with sufficient reliability.
			return CreateProcessGroup(process, process.StartInfo.FileName.ToLowerInvariant().Contains("devenv") || process.StartInfo.FileName.ToLowerInvariant().Contains("msbuild"));
		}

		public class WindowsNamedMutexWrapper : IDisposable
		{
			static System.Collections.Concurrent.ConcurrentDictionary<string, Mutex> GlobalMutexCache = new System.Collections.Concurrent.ConcurrentDictionary<string, Mutex>();
			public WindowsNamedMutexWrapper(string name)
			{
				try
				{
					mMutex = GlobalMutexCache.GetOrAdd(name, (mutexName, self) =>
					{
						return new Mutex(true, mutexName, out self.mCreatedNew);
					}, this);
				}
				catch (ArgumentException e)
				{
					throw new BuildException(String.Format("ERROR: Unable to create named Mutex for task '{0}' to perform cross-process synchronization.  The task name is too long.  Operation error message is: {1}", name, e.Message));
				}
				catch (Exception e)
				{
					throw new BuildException(String.Format("ERROR: Unable to create named Mutex for task '{0}' to perform cross-process synchronization.  Operation error message is: {1}", name, e.Message));
				}
			}

			public void Dispose()
			{
				Dispose(true);
				GC.SuppressFinalize(this);
			}

			void Dispose(bool disposing)
			{
				if (!this.mDisposed)
				{
					if (disposing)
					{
						if (mMutex != null)
						{
							mMutex.ReleaseMutex();
							mMutex = null;
						}
					}
				}
				mDisposed = true;
			}
			private bool   mDisposed = false;

			private Mutex  mMutex;
			private bool   mCreatedNew = false;

			public bool HasOwnership
			{
				get
				{
					return mCreatedNew;
				}
			}

			public bool WaitOne(int timeout = -1) // TODO: ideally this should be called in constructor if we didn't create the mutex
			{
				if (timeout >= 0)
				{
					return mMutex.WaitOne(timeout);
				}
				else
				{
					// Wait forever.
					return mMutex.WaitOne();
				}
			}
		}

	}
}
