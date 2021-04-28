// (c) Electronic Arts. All rights reserved.

using System;
using System.Diagnostics;
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using EA.Eaconfig;

namespace EA.AndroidConfig
{
	/// <summary>
	/// A task for executing the Android Debug Bridge which is used for communicating with
	/// an android device or emulator.
	/// </summary>
	[TaskName("adb")]
	public class AdbTask : Task
	{
		bool mReadStdOut = false;
		int mRetries = 0;
		int mTimeout = Int32.MaxValue;
		string mArguments = string.Empty;
		bool mCheckEmulatorAlive = false;

		/// <summary>The command line arguments passed to adb</summary>
		[TaskAttribute("args")]
		public string Arguments
		{
			get { return mArguments; }
			set { mArguments = value; }
		}

		/// <summary>
		/// The amount of time before the adb command times out.
		/// If using retries the maximum time will be the timeout times the number of retries.
		/// </summary>
		[TaskAttribute("timeout")]
		public int Timeout
		{
			get { return mTimeout; }
			set { mTimeout = value; }
		}

		/// <summary>The number of times to retry a command if it times out.</summary>
		[TaskAttribute("retries")]
		public int Retries
		{
			get { return mRetries; }
			set { mRetries = value; }
		}

		/// <summary>
		/// whether to read from standard output and store in StdOut attribute of analysis after
		/// the process finishes.
		/// </summary>
		[TaskAttribute("readstdout")]
		public bool ReadStdOut
		{
			get { return mReadStdOut; }
			set { mReadStdOut = value; }
		}

		public string StdOut { get; private set; }
		public int ExitCode { get; private set; }

		public bool CheckEmulatorAlive
		{
			get { return mCheckEmulatorAlive; }
			set { mCheckEmulatorAlive = value; }
		}

		static bool gConnected = false;

		static void ConnectViaTcp(Project project, string useTarget)
		{
			if (gConnected)
				return;

			gConnected = true;
			string connectArgs = "connect " + useTarget;
			const int connectTimeout = 15 * 1000;

			Execute(project, connectArgs, connectTimeout);
		}

		protected static string GetForceDeviceString(Project project)
		{	
			string deviceSerial = project.Properties["internal-android-device-serial"];
			if(!String.IsNullOrEmpty(deviceSerial))
			{
				return String.Format("-s {0}", deviceSerial);
			}
			
			string useTarget = project.Properties["internal-android-use-target"];
			if (useTarget == "device")
			{
				return "-d";
			}
			else if (useTarget == "emulator")
			{
				return "-e";
			}
			else if (useTarget != "auto")
			{
				ConnectViaTcp(project, useTarget);
			}

			return String.Empty;
		}

		public void SetArguments(string[] args)
		{
			mArguments = string.Join(" ", args);
		}

		public static AdbTask Execute(Project project, string arguments, int timeout = -1, int retries = 0, bool failonerror = true, bool readstdout = false, bool checkemulator = false)
		{
			if(timeout == -1)
				timeout = Int32.Parse(project.ExpandProperties("${android-timeout}"));
				
			AdbTask adb = new AdbTask();
			adb.Timeout = timeout;
			adb.Retries = retries;
			adb.Project = project;
			adb.Arguments = arguments;
			adb.ReadStdOut = readstdout;
			adb.FailOnError = failonerror;
			adb.CheckEmulatorAlive = checkemulator;
			adb.Execute();

			// android_config liberally tries to kill the debug server to prevent lingering child processes which cause hangs on build farm.
			// However, if connecting with the TCP transport, this connection will need to be re-established before any more ADB commands
			// can be run. Therefore, when a kill-server command is spotted we indicate that this connection must be re-established.
			if (arguments.Contains("kill-server"))
			{
				gConnected = false;
			}

			return adb;
		}

		public static ProcessStartInfo BuildStartInfo(Project project, string arguments, bool readOutput)
		{					
			string adbWorkingDir = AndroidBuildUtil.NormalizePath(project.Properties["package.AndroidSDK.platformtooldir"]);
			string adbpath = Path.Combine(adbWorkingDir, "adb.exe");
			string sdkInstalledDir = AndroidBuildUtil.NormalizePath(project.Properties["package.AndroidSDK.appdir"]);
			
			bool adbVerbose = project.Properties.GetBooleanPropertyOrDefault("package.AndroidSDK.adb-verbose", false);

			string args = string.Concat(AdbTask.GetForceDeviceString(project), " ", arguments);
			ProcessStartInfo startinfo = new ProcessStartInfo(adbpath, args);
			startinfo.WorkingDirectory = adbWorkingDir;
			
			// if we aren't reading the output then we use the shell because then we don't have to worry about the ADB not allowing us to exit like expected
			if(readOutput)
			{
				if (adbVerbose)
				{
					startinfo.EnvironmentVariables.Add("ADB_TRACE", "all");
				}
				startinfo.UseShellExecute = false;
				startinfo.RedirectStandardOutput = true;
				startinfo.RedirectStandardError = true;
				startinfo.WorkingDirectory = adbWorkingDir;
				startinfo.EnvironmentVariables["ANDROID_SDK_ROOT"] = sdkInstalledDir;
			}
			else
			{
				startinfo.UseShellExecute = true;
				startinfo.CreateNoWindow = true;
				startinfo.WindowStyle = ProcessWindowStyle.Hidden;
			}

			return startinfo;
		}

		public static Process ExecuteBackgroundProcess(Project project, string arguments, bool readOutput)
		{
			// if we are trying to read the output from adb then first make sure it has started
			if(readOutput)
				StartServer(project);
			
			ProcessStartInfo startinfo = AdbTask.BuildStartInfo(project, arguments, readOutput);

			project.Log.Status.WriteLine("[exec] {0} {1}", startinfo.FileName, startinfo.Arguments);

			return Process.Start(startinfo);
		}

		public static bool IsEmulatorAlive()
		{
			string[] emulatorProcessNames = {
				"emulator",
				"emulator-arm",
				"emulator-x86"
			};

			Process[] processList = Process.GetProcesses();
			foreach (Process process in processList)
			{
				foreach (string emulatorProcessName in emulatorProcessNames)
				{
					if (process.ProcessName == emulatorProcessName)
					{
						return true;
					}
				}
			}
			return false;
		}
		
		private static void StartServer(Project project)
		{
			foreach (Process process in Process.GetProcesses())
			{
				if (process.ProcessName.Contains("adb"))
					return;
			}
			
			// ADB is restarting so reset the device-serial
			project.Properties["internal-android-use-target"] = project.Properties["android-use-target"];
			project.Properties["internal-android-device-serial"] = "";
			
			AdbTask.Execute(project, "start-server");
		}

		protected override void ExecuteTask()
		{	
			// if we are trying to read the output from adb then first make sure it has started
			if(ReadStdOut)
				StartServer(Project);
			
			ProcessStartInfo startinfo = BuildStartInfo(Project, mArguments, ReadStdOut);

			Log.Info.WriteLine("[exec] {0} {1}", startinfo.FileName, startinfo.Arguments);

			for (int retry_count = 0; retry_count <= mRetries; ++retry_count)
			{
				if (CheckEmulatorAlive && Project.Properties["internal-android-use-target"] == "emulator" && !IsEmulatorAlive())
				{
					Log.Warning.WriteLine("adb.exe is trying to send a command to the emulator but no emulator process exists, the emulator may have crashed.");
				}

				Process adb_process = Process.Start(startinfo);
				if (ReadStdOut)
				{
					StdOut = adb_process.StandardOutput.ReadToEnd();
					StdOut += adb_process.StandardError.ReadToEnd();
				}

				if (!adb_process.WaitForExit(mTimeout))
				{
					adb_process.Close();
					Log.Warning.WriteLine("adb.exe did not exit cleanly after '{0}' ms", mTimeout);
					continue;
				}
				else
				{
					ExitCode = adb_process.ExitCode;
					if (FailOnError && ExitCode != 0)
					{
						// only dump the output on a failure for diagnosis
						if (Log.InfoEnabled)
						{
							Log.Info.WriteLine(StdOut);
						}

						string error_message = string.Format("Adb exited with exit code {0}!", ExitCode);
						Log.Error.WriteLine(error_message);
						AdbKillTask.Execute(Project, true);
						throw new BuildException(error_message);
					}
					return;
				}
			}

			if (FailOnError)
			{
				string error_message2 = string.Format("Adb failed to exit cleanly after {0} retries!", mRetries);
				Log.Error.WriteLine(error_message2);
				AdbKillTask.Execute(Project, true);
				throw new BuildException(error_message2);
			}
		}
	}
}
