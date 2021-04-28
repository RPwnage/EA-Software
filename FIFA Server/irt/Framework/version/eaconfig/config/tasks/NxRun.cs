// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using EA.Eaconfig;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.IO;
using System.Threading;
using Microsoft.Win32;
using System.Linq;

namespace EA.nxConfig
{
	[TaskName("nxRun")]
	public class Run : Task
	{
		string mConsoleName;
		string mRunTimeout;
		string mActivityTimeout;
		string mWorkingDir;
		string mProgramFolder;
		string mProgram;
		string mArgs;
		List<string> mLog;
		object mLogLock = new object();

		[TaskAttribute("console")]
		public string ConsoleName {
			get { return mConsoleName; }
			set { mConsoleName = value; }
		}

		[TaskAttribute("runtimeout")]
		public string RunTimeout {
			get { return mRunTimeout; }
			set { mRunTimeout = value; }
		}

		[TaskAttribute("activitytimeout")]
		public string ActivityTiemout {
			get { return mActivityTimeout; }
			set { mActivityTimeout = value; }
		}
		
		[TaskAttribute("workingdir")]
		public string WorkingDir {
			get { return mWorkingDir; }
			set { mWorkingDir = value; }
		}
		
		[TaskAttribute("programfolder")]
		public string ProgramFolder {
			get { return mProgramFolder; }
			set { mProgramFolder = value; }
		}

		[TaskAttribute("program")]
		public string Program {
			get { return mProgram; }
			set { mProgram = value; }
		}

		[TaskAttribute("args")]
		public string Args {
			get { return mArgs; }
			set { mArgs = value; }
		}

		void OutputReceivedHandler(object sendingProcess, DataReceivedEventArgs outputData)
		{
			if (outputData.Data != null)
			{
				lock (mLogLock)
				{
					mLog.Add(outputData.Data);
				}

				Log.Status.WriteLine(outputData.Data);
			}
		}

		void ErrorReceivedHandler(object sendingProcess, DataReceivedEventArgs outputData)
		{
			if (outputData.Data != null)
			{
				lock (mLogLock)
				{
					mLog.Add(outputData.Data);
				}

				Log.Error.WriteLine(outputData.Data);
			}
		}

		int NxCtrl(string command, string commandArgs=null)
		{
			string program = PathNormalizer.Normalize(Project.ExpandProperties(@"${package.nxsdk.appdir}/Tools/CommandLineTools/ControlTarget.exe"));
			string consoleString = string.IsNullOrWhiteSpace(mConsoleName) ? "" : ("-t " + mConsoleName);
			string args = string.Format("{0} {1} {2}", command, consoleString, string.IsNullOrWhiteSpace(commandArgs) ? "" : commandArgs);

			int exitCode = RunProgram(program, args, 60000);
			if (exitCode != 0)
			{
				Log.Error.WriteLine("Unable to run ControlTarget {0}!", args);
			}

			return exitCode;
		}

		void DisconnectToTarget()
		{
			NxCtrl("disconnect");
		}

		int RunProgram(string program, string args, int timeout)
		{
			if(timeout <= 0)
			{
				timeout = Timeout.Infinite;
			}
		
			ProcessStartInfo startInfo = new ProcessStartInfo(program, args);

			startInfo.CreateNoWindow = true;
			startInfo.UseShellExecute = false;
			startInfo.RedirectStandardOutput = true;
			startInfo.RedirectStandardError = true;

			mLog = new List<string>();

			Log.Status.WriteLine("Executing \"{0}\" {1}", program, args);

			using (Process p = new Process())
			{
				p.StartInfo = startInfo;

				p.ErrorDataReceived += new DataReceivedEventHandler(ErrorReceivedHandler);
				p.OutputDataReceived += new DataReceivedEventHandler(OutputReceivedHandler);

				p.Start();
				p.BeginOutputReadLine();
				p.BeginErrorReadLine();

				if (!p.WaitForExit(timeout))
				{
					Log.Error.WriteLine("Timeout value of {0}ms exceeded, killing process.", timeout);
					p.Kill();
				}
				
				HasBuildFailed();

				return p.ExitCode;
			}
		}

		enum HTC_PROTOCOL
		{
			HTC_GEN1,
			HTC_GEN2
		};

		// Start the background app requires to communicate between Host PC and the target device and return the host PC
		// comunication protocol setup.
		HTC_PROTOCOL StartHostTargetCommunicationApp()
		{
			// We pre-run the communication app first before executing ControlTargets.exe or RunOnTarget.exe because
			// those app hard coded to launch the communication app directly in installed folder at C:\Program Files\Nintendo
			// But build machines don't have those app installed, so we pre-launch them before executing ControlTargets.exe
			// and RunOnTarget.exe

			// Starting with NintendoSDK 10.4.0, there are two target managers.  The two different target managers use different
			// "host target communication (htc)" protocol: htc-gen1 or htc-gen2.
			// Both the PC AND the target console need to be setup to use the same protocol.

			// At the moment, NintendoSDK 10.4.0 doesn't have command line tools to help us detect which protocol is setup on 
			// target console.  So for now, we just start both app.  In fact, when Nintendo provided command line to let us detect which
			// protocol is setup on the device, we do need both app to be executed first.  Also, when that happen, we can 
			// adjust host PC's setting to match target machine (unless it is on developer machine?)

			// Detect local host htc protocol setup.
			HTC_PROTOCOL currentHostPcHtcProtocol = HTC_PROTOCOL.HTC_GEN1;
			try
			{
				object htcGenValObj = Registry.GetValue("HKEY_CURRENT_USER\\Software\\Nintendo\\NintendoSdk", "UserHtcGeneration", null);
				if (htcGenValObj != null)
				{
					Int32 htcGenVal = (Int32)htcGenValObj;
					if (htcGenVal == 2)
					{
						currentHostPcHtcProtocol = HTC_PROTOCOL.HTC_GEN2;
					}
				}
			}
			catch
			{
			}

			// Both of these htp app is available on the SDK, so we'll start those from the SDK
			string sdkDir = PathNormalizer.Normalize(Project.ExpandProperties("${package.nxsdk.appdir}"));
			Tuple<HTC_PROTOCOL, string>[] htcAppList = new Tuple<HTC_PROTOCOL, string>[]
			{
				new Tuple<HTC_PROTOCOL,string>(HTC_PROTOCOL.HTC_GEN2, Path.Combine(sdkDir, "Resources", "Firmwares", "NX", "NintendoSdkDaemon", "NintendoSdkDaemon.exe")),
				new Tuple<HTC_PROTOCOL,string>(HTC_PROTOCOL.HTC_GEN1, Path.Combine(sdkDir, "Resources", "Firmwares", "NX", "OasisTM", "NintendoTargetManager.exe"))
			};

			// Before starting these apps, make sure process isn't already running.  Need to make sure we run the latest version.
			// They are by design backward compatible (according to Nintendo.)
			List<Tuple<HTC_PROTOCOL, string>> launchHtcApps = new List<Tuple<HTC_PROTOCOL, string>>();
			foreach (Tuple<HTC_PROTOCOL, string> app in htcAppList)
			{
				string processName = Path.GetFileNameWithoutExtension(app.Item2);
				Process[] runningProcesses = Process.GetProcessesByName(processName);
				if (File.Exists(app.Item2))
				{
					FileVersionInfo sdkFileVerInfo = FileVersionInfo.GetVersionInfo(app.Item2);
					bool runningNewerVersion = false;
					if (runningProcesses != null && runningProcesses.Length > 0)
					{
						Process proc = runningProcesses.Where(p => p.MainModule.ModuleName.Contains(processName)).FirstOrDefault();
						if (proc != null)
						{
							runningNewerVersion = proc.MainModule.FileVersionInfo.FileVersion.StrCompareVersions(sdkFileVerInfo.FileVersion) >= 0;
						}
					}
					if (!runningNewerVersion)
						launchHtcApps.Add(new Tuple<HTC_PROTOCOL, string>(app.Item1, app.Item2));
				}
				else if (app.Item1 == currentHostPcHtcProtocol && currentHostPcHtcProtocol == HTC_PROTOCOL.HTC_GEN2 && (runningProcesses == null || runningProcesses.Length == 0))
				{
					Log.Warning.WriteLine("Current host PC is setup to communicate with console using {0}.  But selected SDK doesn't ship with {1} in this path {2}. You may be using incompatible SDK version.",
						currentHostPcHtcProtocol == HTC_PROTOCOL.HTC_GEN1 ? "htc-gen1" : "htc-gen2", processName, app.Item2);
				}
			}

			foreach (Tuple<HTC_PROTOCOL,string> app in launchHtcApps)
			{
				Log.Status.WriteLine("Starting detached {0} daemon \"{1}\"", app.Item1 == HTC_PROTOCOL.HTC_GEN1 ? "htc-gen1" : "htc-gen2", app.Item2);
				ProcessStartInfo startInfo = new ProcessStartInfo(app.Item2);
				startInfo.Arguments = "";
				startInfo.WorkingDirectory = Environment.CurrentDirectory;
				startInfo.UseShellExecute = false;
				startInfo.CreateNoWindow = true;
				ProcessUtil.CreateDetachedProcess(startInfo.FileName, startInfo.Arguments, startInfo.WorkingDirectory, null, false);
			}

			return currentHostPcHtcProtocol;
		}

		int ParseTimeout()
		{
			if (string.IsNullOrWhiteSpace(mRunTimeout))
			{
				return int.MaxValue;
			}

			return int.Parse(mRunTimeout) * 1000;
		}

		bool HasFailedToConnect()
		{
			lock (mLogLock)
			{
				foreach (string line in mLog)
				{
					if (line.Contains("Failed to connect"))
					   return true;
				}
			}

			return false;
		}

		bool HasBuildFailed()
		{
			lock (mLogLock)
			{
				foreach (string line in mLog)
				{
					if (line.Contains("RETURNCODE="))
					{
						var keyValue = line.Split('=');
						if(keyValue[1] != "0")
							throw new BuildException("Build failed!");
					}
				}
			}

			return false;
		}

		string PrepNxRunArguments()
		{
			StringBuilder stringBuilder = new StringBuilder();

			if (!string.IsNullOrWhiteSpace(mConsoleName))
				stringBuilder.Append("--target ").Append(mConsoleName).Append(" ");
			
			if (int.Parse(mRunTimeout) > 0)
				stringBuilder.Append("--failure-timeout ").Append(mRunTimeout).Append(" ");

			if (!string.IsNullOrWhiteSpace(mWorkingDir))
			{
				if (!Directory.Exists(mWorkingDir))
				{
					throw new BuildException(string.Format("Working directory '{0}' does not exist!", mWorkingDir));
				}
				
				stringBuilder.Append("--working-directory ").Append(mWorkingDir).Append(" ");
			}
			
			stringBuilder.Append(mProgram);
			stringBuilder.Append(" ");
			
			if (!string.IsNullOrWhiteSpace(mArgs))
				stringBuilder.Append("-- " + mArgs);

			return stringBuilder.ToString();
		}

		protected override void ExecuteTask()
		{
			HTC_PROTOCOL hostPcHtcProtocol = StartHostTargetCommunicationApp();

			if (NxCtrl("connect", "--force") != 0)
			{
				throw new BuildException("Unable to connect to dev kit!");
			}

			string NxRun = PathNormalizer.Normalize(Project.ExpandProperties("${package.nxsdk.appdir}/Tools/CommandLineTools/RunOnTarget.exe"));
			string arguments = PrepNxRunArguments();
			int timeout = ParseTimeout();

			if (hostPcHtcProtocol == HTC_PROTOCOL.HTC_GEN1)
			{
				// power-on doesn't appear to be supported starting with NintendoSdkDaemon 11.0.0.0
				// You will get "[ERROR] Specified method is not supported." on GEN2.
				bool kitIsOn = false;
				for (int i = 0; i < 5; ++i)
				{
					if (NxCtrl("power-on") == 0)
					{
						kitIsOn = true;
						break;
					}
				}

				if (!kitIsOn)
				{
					throw new BuildException("Unable to turn on dev kit!");
				}
			}

			// First attempt to run the program normally. If it succeeds, 
			// we're done and can return.
			if (RunProgram(NxRun, arguments, timeout) == 0)
			{
				DisconnectToTarget();
				return;
			}

			// If the execution has failed, scan the output for connection related
			// errors. If any of these have been emitted, reboot the console.
			// If not, then the test program has failed.
			if (HasFailedToConnect())
			{
				Log.Warning.WriteLine("Detected that the process has failed to connect to the devkit. Forcing a reboot.");
				NxCtrl("reset");
			}
			else
			{
				DisconnectToTarget();
				throw new BuildException("Build failed!");
			}

			// At this point if the reboot was successful we should try 
			// re-running the test application.
			if (RunProgram(NxRun, arguments, timeout) == 0)
			{
				DisconnectToTarget();
				return;
			}

			// Again, if we have failed to connect, attempt remediation but in
			// this case we power-cycle the kit. This can take a long time so
			// it isn't the default action.
			if (HasFailedToConnect())
			{
				Log.Warning.WriteLine("Detected that the process has failed to connect to the devkit. Forcing a power-cycle.");
				NxCtrl("power-off");
		  
				// Sometimes the kit doesn't succeed in turning on right away
				// via Nx-ctrl so give this a few iterations until it has.
				bool hasTurnedOn = false;
				for (int ii = 0; ii < 3 && !hasTurnedOn; ++ii)
				{
					hasTurnedOn = NxCtrl("power-on") == 0;
				}

				if (!hasTurnedOn)
				{
					// If it doesn't succeed in powering up three times, kill
					// the build process.
					throw new BuildException("Unable to power cycle kit.");
				}
			}
			else
			{
				DisconnectToTarget();
				throw new BuildException("Build failed!");
			}

			// Attempt running one last time.
			if (RunProgram(NxRun, arguments, timeout) == 0)
			{
				DisconnectToTarget();
				return;
			}

			DisconnectToTarget();
			// At this point either we just can't get the kit to work, or
			// it has worked and the build failed, in either case we need
			// to raise an exception.
			throw new BuildException("Build failed!");
		}
	}
}



