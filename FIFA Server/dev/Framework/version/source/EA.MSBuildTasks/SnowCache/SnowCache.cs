// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Text;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System.Runtime.InteropServices;
using NAnt.Core.Metrics;
using Newtonsoft.Json;
using System.Diagnostics;

namespace EA.Framework.MsBuild
{
	static class ProcessHelper
	{
		const uint CREATE_NEW_PROCESS_GROUP = 0x00000200;
		const uint DETACHED_PROCESS = 0x00000008;

		[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
		struct STARTUPINFOEX
		{
			public STARTUPINFO StartupInfo;
			public IntPtr lpAttributeList;
		}

		[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
		struct STARTUPINFO
		{
			public Int32 cb;
			public string lpReserved;
			public string lpDesktop;
			public string lpTitle;
			public Int32 dwX;
			public Int32 dwY;
			public Int32 dwXSize;
			public Int32 dwYSize;
			public Int32 dwXCountChars;
			public Int32 dwYCountChars;
			public Int32 dwFillAttribute;
			public Int32 dwFlags;
			public Int16 wShowWindow;
			public Int16 cbReserved2;
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
			public int bInheritHandle;
		}

		[DllImport("kernel32.dll", SetLastError = true)]
		static extern bool CloseHandle(IntPtr hObject);

		[DllImport("kernel32.dll")]
		[return: MarshalAs(UnmanagedType.Bool)]
		static extern bool CreateProcess(	string lpApplicationName, string lpCommandLine, ref SECURITY_ATTRIBUTES lpProcessAttributes,
											ref SECURITY_ATTRIBUTES lpThreadAttributes, bool bInheritHandles, uint dwCreationFlags,
											IntPtr lpEnvironment, string lpCurrentDirectory, [In] ref STARTUPINFO lpStartupInfo,
											out PROCESS_INFORMATION lpProcessInformation);

		public static bool StartProcess(string aExeName, string aCommandLine)
		{
			var pInfo = new PROCESS_INFORMATION();
			var sInfo = new STARTUPINFO();
			sInfo.cb = Marshal.SizeOf(sInfo);
			var pSec = new SECURITY_ATTRIBUTES();
			var tSec = new SECURITY_ATTRIBUTES();
			pSec.nLength = Marshal.SizeOf(pSec);
			tSec.nLength = Marshal.SizeOf(tSec);

			bool ok = CreateProcess(aExeName, aCommandLine, ref pSec, ref tSec, false, CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS, IntPtr.Zero, System.IO.Path.GetDirectoryName(aExeName), ref sInfo, out pInfo);

			if (pInfo.hProcess != IntPtr.Zero)
			{
				CloseHandle(pInfo.hProcess);
			}

			if (pInfo.hThread != IntPtr.Zero)
			{
				CloseHandle(pInfo.hThread);
			}

			return ok;
		}
	}

	static class SnowCacheServerHelper
	{
		static readonly string ServerExeName = "SnowCacheServer.exe";

		static private NamedPipeClientStream ConnectToPipe(string aPipeName, int aRetries = 10, int aTimeout = 500)
		{
			NamedPipeClientStream client = null;

			for (int retry = 0; retry <= aRetries; retry++)
			{
				try
				{
					client = new NamedPipeClientStream(".", aPipeName, PipeDirection.InOut);
					client.Connect(aTimeout);
					if (client.IsConnected)
					{
						return client;
					}

					client?.Dispose();
				}
				catch
				{
					client?.Dispose();
				}

			}

			return null;
		}

		static private string GetPipeName(string aStreamName)
		{
			return "snowcache_" + aStreamName;
		}

		static private String GetClientIdFromExeName(String aExeName)
		{
			int lastSep = Math.Max(aExeName.LastIndexOf('\\'), aExeName.LastIndexOf('/'));
			if (lastSep != -1)
			{
				aExeName = aExeName.Substring(0, lastSep);
			}
			return aExeName.Replace("\\", "").Replace("/", "").Replace(":", "").Replace(".", "").ToLower();
		}

		// simple check to see if running
		// more robust process based api fail as crossing the 32/64 bit boundary
		static private bool isProcessRunning(string apExeName)
		{
			bool exeRunning = false;
			try
			{
				var stream = File.OpenWrite(apExeName);
				stream.Dispose();
			}
			catch
			{
				exeRunning = true;
			}
			return exeRunning;
		}

		static private bool LaunchServerInTempDir(string aBinDirectory, string aServerFilename, string aClientId, string aTempPath, string aTempExeName, TaskLoggingHelper log)
		{
			// Disable small block allocators as they cause handle and memory leaks when using a lot of threads and windows concurrency. (-noSmallBlockArenas GlobalSBA,Global)
			string commandLine = " -exe \"" + aServerFilename + "\"" + " -noSmallBlockArenas GlobalSBA,Global";

			Directory.CreateDirectory(aTempPath);

			foreach (var fileInBin in System.IO.Directory.GetFiles(aBinDirectory, "*.*", SearchOption.TopDirectoryOnly))
			{
				if (File.Exists(fileInBin))
				{
					string sourceFile = fileInBin;
					var sourceFileTime = File.GetLastWriteTime(sourceFile);


					string targetFile = aTempPath + Path.GetFileName(fileInBin);
					var targetFileFileTime = File.GetLastWriteTime(targetFile);

					if (sourceFileTime != targetFileFileTime)
					{
						File.Copy(sourceFile, targetFile, true);
						var attributes = File.GetAttributes(targetFile);
						if ((attributes & FileAttributes.ReadOnly) != 0)
						{
							File.SetAttributes(targetFile, attributes & ~FileAttributes.ReadOnly);
						}
					}
				}
			}

			log.LogMessage(MessageImportance.High, "SnowCache: id   = " + aClientId);
			log.LogMessage(MessageImportance.High, "SnowCache: path = " + aTempExeName);

			// Start the process with the info we specified.
			// create with p/invoke or .NET will make the new process inherit handles, which will prevent this task from closing at the end of the build
			if (!ProcessHelper.StartProcess(aTempExeName, commandLine.ToString()))
			{
				log.LogMessage(MessageImportance.High, "Failed to spawn server.");
				return false;
			}
			return true;
		}

		static public NamedPipeClientStream InitializeConnectionToServer(string aExeName, TaskLoggingHelper log)
		{
			NamedPipeClientStream pipeClient = null;

			string clientId = GetClientIdFromExeName(aExeName);

			System.Threading.Mutex globalMutex = new System.Threading.Mutex(false, clientId + "Mtx");
			try
			{
				string binDirectory = Path.GetDirectoryName(aExeName);
				string serverFilename = Path.Combine(binDirectory, ServerExeName);
				string tempPath = Path.GetTempPath() + clientId + "\\";
				string tempExeName = tempPath + ServerExeName;

				globalMutex.WaitOne();

				string pipeName = GetPipeName(clientId);
				pipeClient = ConnectToPipe(pipeName,5,200);

				if (pipeClient != null)
				{
					// check for out-of-date temp exe
					if (File.Exists(tempExeName) && (File.GetLastWriteTime(tempExeName) < File.GetLastWriteTime(aExeName)))
					{
						bool tempExeRunning = isProcessRunning(tempExeName);
						if (tempExeRunning)
						{
							log.LogMessage(MessageImportance.High, "Shutting down out-of-date server.");
							using (StreamWriter sw = new StreamWriter(pipeClient, Encoding.UTF8, 4096, true))
							{
								sw.Write("shutdown");
							}
							log.LogMessage(MessageImportance.High, "waiting for out-of-date server to close.");
							while (isProcessRunning(tempExeName))
							{
								System.Threading.Thread.Sleep(100);
							}
							pipeClient.Dispose();
							pipeClient = null;
						}
					}
				}

				if (pipeClient == null)
				{
					log.LogMessage(MessageImportance.High, "spawning server.");
					bool serverSpawned = LaunchServerInTempDir(binDirectory, serverFilename, clientId, tempPath, tempExeName, log);

					if (serverSpawned == false)
					{
						log.LogMessage(MessageImportance.High, "Failed to spawn server.");
						return null;
					}

					pipeClient = ConnectToPipe(pipeName, 50, 100);

					if (pipeClient == null)
					{
						log.LogMessage(MessageImportance.High, "Failed to connect to server.");
						return null;
					}
				}
			}
			catch
			{
				log.LogMessage(MessageImportance.High, "SnowCache: Startup Failed" + clientId);
				pipeClient?.Dispose();
				pipeClient = null;
			}
			finally
			{
				globalMutex?.ReleaseMutex();
				globalMutex?.Dispose();
			}

			return pipeClient;
		}
	}


	public class SnowCache : Task
	{
		[Required]
		public string ExecutableFile { get; set; }

		[Required]
		public string Mode { get; set; }

		[Required]
		public string Platform { get; set; }

		[Required]
		public string Configuration { get; set; }

		[Required]
		public string SolutionFile { get; set; }

		[Required]
		public string ProjectFile { get; set; }

		[Required]
		public string IntermediateDir { get; set; }

		[Required]
		public string LocalDir { get; set; }

		[Required]
		public string AvalancheHost { get; set; }

		public bool LoggingEnabled { get; set; } = false;

		public bool DiagnosticsEnabled { get; set; } = false;

		public string SyncedChangelist { get; set; }

		[Output]
		public bool Success { get; set; } = false;

		public override bool Execute()
		{
			Success = false;
			NamedPipeClientStream pipeClient = null;
			try
			{
				pipeClient = SnowCacheServerHelper.InitializeConnectionToServer(ExecutableFile, Log);

				if (pipeClient != null)
				{
					StringBuilder cmdLineSb = new StringBuilder();

					cmdLineSb.Append(" -mode ").Append(Mode);
					cmdLineSb.Append(" -platform ").Append(Platform);
					cmdLineSb.Append(" -config ").Append(Configuration);
					cmdLineSb.Append(" -proj ").Append(ProjectFile);
					cmdLineSb.Append(" -sln ").Append(SolutionFile);
					cmdLineSb.Append(" -intdir ").Append(IntermediateDir);
					cmdLineSb.Append(" -localdir ").Append(LocalDir);
					cmdLineSb.Append(" -avalanchehost ").Append(AvalancheHost);


					if (DiagnosticsEnabled)
					{
						Log.LogMessage(MessageImportance.High, "SnowCache: Connecting to Avalanche host " + AvalancheHost);
						cmdLineSb.Append(" -diagnostics true ").Append(LocalDir);
					}

					if (!string.IsNullOrEmpty(SyncedChangelist))
					{
						cmdLineSb.Append(" -synced_changelist ").Append(SyncedChangelist);
					}

					if (LoggingEnabled)
					{
						cmdLineSb.Append(" -log true");
					}

					using (StreamWriter sw = new StreamWriter(pipeClient, Encoding.UTF8, 4096, true))
					{
						sw.Write(cmdLineSb.ToString());
					}

					string reply;
					List<string> diagnostics = new List<string>();
					using (StreamReader sr = new StreamReader(pipeClient, Encoding.UTF8, false, 4096, true))
					{
						reply = sr.ReadLine();
						while (!sr.EndOfStream)
						{
							diagnostics.Add(sr.ReadLine());
						}
					}

					if (String.IsNullOrEmpty(reply))
					{
						Log.LogMessage(MessageImportance.High, "Error - No reply received from snowcacheserver. This likely means that your local snowcacheserver.exe has crashed.");
					}
					else
					{
						var dict = reply.Split(new[] { '|' }, StringSplitOptions.RemoveEmptyEntries).Select(x => x.Split('=')).ToDictionary(x => x[0], y => y[1]);

						string successResult;
						if (dict.TryGetValue("success", out successResult))
						{
							Success = (string.Compare(successResult, "true", true) == 0);
						}

						// TODO, not the same as displayed project name in tool_shared
						string projectFilename = Path.GetFileNameWithoutExtension(ProjectFile);

						string formattedReply = reply.Replace('|', ' ');

						Log.LogMessage(MessageImportance.High, "SnowCache: " + projectFilename + " " + formattedReply);
						if (diagnostics.Any())
						{
							foreach (var diagnostic in diagnostics)
							{
								Log.LogMessage(MessageImportance.High, "SnowCache: " + diagnostic);
							}
						}
					}
				}
			}
			catch (Exception e)
			{
				Log.LogMessage(MessageImportance.High, "EA.Framework.MsBuild: " + e.Message);
			}
			finally
			{
				pipeClient?.Close();
				pipeClient?.Dispose();
			}

			// result passed back to build in the 'Result' parameter
			return true;
		}
	}

	public class SnowCacheSlnSummary : Task
	{
		[Required]
		public string ExecutableFile { get; set; }

		[Required]
		public string Action { get; set; }

		[Required]
		public string Mode { get; set; }

		[Required]
		public string Platform { get; set; }

		[Required]
		public string Configuration { get; set; }

		[Required]
		public string SolutionFile { get; set; }

		[Output]
		public bool Success { get; set; } = false;

		public string SyncedChangelist { get; set; }

		public override bool Execute()
		{
			NamedPipeClientStream pipeClient = null;
			try
			{
				pipeClient = SnowCacheServerHelper.InitializeConnectionToServer(ExecutableFile, Log);

				if (pipeClient != null)
				{
					StringBuilder cmdLineSb = new StringBuilder();

					cmdLineSb.Append(" -slnsummaryaction ").Append(Action);
					cmdLineSb.Append(" -mode ").Append(Mode);
					cmdLineSb.Append(" -platform ").Append(Platform);
					cmdLineSb.Append(" -config ").Append(Configuration);
					cmdLineSb.Append(" -sln ").Append(SolutionFile);

					using (StreamWriter sw = new StreamWriter(pipeClient, Encoding.UTF8, 4096, true))
					{
						sw.Write(cmdLineSb.ToString());
					}

					string reply;
					List<string> diagnostics = new List<string>();
					using (StreamReader sr = new StreamReader(pipeClient, Encoding.UTF8, false, 4096, true))
					{
						reply = sr.ReadLine();
						while (!sr.EndOfStream)
						{
							diagnostics.Add(sr.ReadLine());
						}
					}

					var dict = reply.Split(new[] { '|' }, StringSplitOptions.RemoveEmptyEntries).Select(x => x.Split('=')).ToDictionary(x => x[0], y => y[1]);

					string successResult;
					if (dict.TryGetValue("success", out successResult))
					{
						Success = (string.Compare(successResult, "true", true) == 0);
					}

					string formattedReply = reply.Replace('|', ' ');

					Log.LogMessage(MessageImportance.High, "\n*********************************************************************");
					Log.LogMessage(MessageImportance.High, "SnowCacheSlnSummary: " + SolutionFile + " " + formattedReply);
					Log.LogMessage(MessageImportance.High, "*********************************************************************\n");

					LogTelemetry(formattedReply);
				}
			}
			catch (Exception e)
			{
				Log.LogMessage(MessageImportance.High, "EA.Framework.MsBuild: " + e.Message);
			}
			finally
			{
				pipeClient?.Close();
				pipeClient?.Dispose();
			}

			// result passed back to build in the 'Result' parameter
			return true;
		}

		private void LogTelemetry(string snowcacheServerReply)
		{			
			string hitRatioToken = "hit ratio=";
			int startIndex = snowcacheServerReply.IndexOf(hitRatioToken);
			if (startIndex > 0)
			{// This task is executed at least twice, if hit ratio not found then its not the final one (the one that we want)
				startIndex += hitRatioToken.Length;
				int endIndex = snowcacheServerReply.IndexOf("%", startIndex);
				float hitRatio = Convert.ToSingle(snowcacheServerReply.Substring(startIndex, endIndex - startIndex));

				FileInfo solutionFileInfo = new FileInfo(SolutionFile);

				string formattedSolutionName = solutionFileInfo.Name.Replace(".sln", string.Empty);

				List<Event> events = new List<Event>();
				Event hitRatioEvent = new Event();
				hitRatioEvent.Name = "snowcache";
				hitRatioEvent.TimeStamp = System.DateTime.Now;
				hitRatioEvent.Properties = new Dictionary<string, string>();
				hitRatioEvent.Properties.Add("hitratio", hitRatio.ToString());
				hitRatioEvent.Properties.Add("config", Configuration);
				hitRatioEvent.Properties.Add("platform", Platform);
				hitRatioEvent.Properties.Add("solutionfile", formattedSolutionName);
				if (!string.IsNullOrEmpty(SyncedChangelist))
				{
					hitRatioEvent.Properties.Add("changelist", SyncedChangelist);
				}
				else
				{
					hitRatioEvent.Properties.Add("changelist", "-1");
				}
				hitRatioEvent.Properties.Add("mode", Mode);
				hitRatioEvent.Properties.Add("telemetryversion", "3");
				events.Add(hitRatioEvent);
				string telemetryClientPath = Telemetry.CopyTelemetryClientToTempDirectory();
				// write telemetry to json file
				string serializedEvents = JsonConvert.SerializeObject(events.ToArray());
				string serializedEventsFile = Path.Combine(telemetryClientPath, Process.GetCurrentProcess().Id.ToString());
				File.WriteAllText(serializedEventsFile, serializedEvents);
				
				Telemetry.UploadTelemetry(telemetryClientPath, serializedEventsFile);
			}
		}
	}

}
