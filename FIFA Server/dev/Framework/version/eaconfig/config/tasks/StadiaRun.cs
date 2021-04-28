// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Management;
using System.Text;
using System.IO;
using System.Threading;

namespace EA.Eaconfig
{
    [TaskName("StadiaRun")]
	public class StadiaRun : Task
	{
		List<string> mLog;
		object mLogLock = new object();
		bool gotOutput = false;

		[TaskAttribute("console")]
		public string ConsoleName { get; set; }

		[TaskAttribute("runtimeout")]
		public string RunTimeout { get; set; }

		[TaskAttribute("activitytimeout")]
		public string ActivityTiemout { get; set; }

		[TaskAttribute("workingdir")]
		public string WorkingDir { get; set; }

		[TaskAttribute("programfolder")]
		public string ProgramFolder { get; set; }

		[TaskAttribute("program")]
		public string Program { get; set; }

		[TaskAttribute("programname")]
		public string ProgramName { get; set; }

		[TaskAttribute("args")]
		public string Args { get; set; }

		[TaskAttribute("serviceaccountkeylocation")]
		public string ServiceAccountKeyLocation { get; set; }

		[TaskAttribute("testaccountname")]
		public string TestAccountName { get; set; }

		[TaskAttribute("projectname")]
		public string ProjectName { get; set; }

		private string GGP { get { return Path.Combine(Project.GetPropertyOrFail("package.StadiaSDK.sdkdir"), "dev", "bin", "ggp.exe"); } }

		private string StadiaCopy { get { return Path.Combine(Project.GetPropertyOrFail("nant.location"), "stadia_copy.exe"); } }

		void OutputReceivedHandler(object sendingProcess, DataReceivedEventArgs outputData)
		{
			if (outputData.Data != null)
			{
				lock (mLogLock)
				{
					mLog.Add(outputData.Data);
					gotOutput = true;
				}

				Log.Minimal.WriteLine(outputData.Data);
			}
		}

		void ErrorReceivedHandler(object sendingProcess, DataReceivedEventArgs outputData)
		{
			if (outputData.Data != null)
			{
				lock (mLogLock)
				{
					mLog.Add(outputData.Data);
					gotOutput = true;
				}

				Log.Error.WriteLine(outputData.Data);
			}
		}

		private static void KillProcessAndChildren(int pid)
		{
			// Cannot close 'system idle process'.
			if (pid == 0)
			{
				return;
			}

			var searcher = new ManagementObjectSearcher("Select * From Win32_Process Where ParentProcessID=" + pid);
			foreach (ManagementObject mo in searcher.Get())
				KillProcessAndChildren(Convert.ToInt32(mo["ProcessID"]));

			try
			{
				Process proc = Process.GetProcessById(pid);
				proc.Kill();
			}
			catch (ArgumentException)
			{
				// Process already exited.
			}
		}

		static void KillProcesses(string processName)
		{
			foreach (var proc in Process.GetProcessesByName(processName))
				KillProcessAndChildren(proc.Id);
		}

		static void KillChromeClients()
		{
			KillProcesses("chrome_client");
		}

		int RunGGPProgram(string args, int timeout)
		{
			return RunProgram(GGP, args, timeout);
		}

		int RunProgram(string prog, string args, int timeout)
		{

			if(timeout <= 0)
			{
				timeout = Timeout.Infinite;
			}

			ProcessStartInfo startInfo = new ProcessStartInfo(prog, args)
			{
				CreateNoWindow = true,
				UseShellExecute = false,
				RedirectStandardOutput = true,
				RedirectStandardError = true
			};

			mLog = new List<string>();

			Log.Minimal.WriteLine($"Executing ggp {prog} {args}");

			using (Process p = new Process())
			{
				p.StartInfo = startInfo;

				p.ErrorDataReceived += (sender, dataArgs) => Console.WriteLine(dataArgs.Data);
				p.OutputDataReceived += (sender, dataArgs) => Console.WriteLine(dataArgs.Data);

				p.Start();
				p.BeginOutputReadLine();
				p.BeginErrorReadLine();

				TimeSpan timeoutDuration = TimeSpan.FromMilliseconds(timeout);
				Stopwatch stopwatch = Stopwatch.StartNew();
				while (stopwatch.Elapsed < timeoutDuration && !p.HasExited)
				{
					lock (mLogLock)
					{
						if (gotOutput)
						{
							stopwatch = Stopwatch.StartNew();
							gotOutput = false;
						}
					}
					Thread.Sleep(100);
				}

				if (stopwatch.Elapsed >= timeoutDuration && !p.HasExited)
				{
					Log.Error.WriteLine("Timeout value of {0}ms exceeded, killing process.", timeout);
					KillProcessAndChildren(p.Id);
				}

				HasBuildFailed(p);

				return p.ExitCode;
			}
		}

		int ParseTimeout()
		{
			if (string.IsNullOrWhiteSpace(RunTimeout))
			{
				return int.MaxValue;
			}

			return int.Parse(RunTimeout) * 1000;
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

		bool HasBuildFailed(Process proc)
		{
			// It is possible for the application process to generate an error after main has returned a non-zero
			// exitcode signaling all the unit tests passed.  These are typically issues during static deinitialization
			// of the application.  We therefore check the process exitcode first to ensure that the process completed
			// successfully before checking for further unit test errors.
			if (proc.ExitCode != 0)
			{
				throw new BuildException("Build failed!");
			}

			lock (mLogLock)
			{
				foreach (string line in mLog)
				{
					if (line.Contains("RETURNCODE="))
					{
						var keyValue = line.Split('=');
						if (keyValue[1] != "0")
							throw new BuildException("Build failed!");
					}
					else if (line.Contains("Segmentation Fault Signal"))
					{
						throw new BuildException("Build failed!");
					}
				}
			}

			return false;
		}

		string CheckReservationStatusArguments()
		{
			return "instance list -sv";
		}

		string ReserveInstance()
		{
			StringBuilder stringBuilder = new StringBuilder();
			stringBuilder.Append(string.Format("instance reserve {0}", ConsoleName));
			return stringBuilder.ToString();
		}

		string ReleaseInstance()
		{
			StringBuilder stringBuilder = new StringBuilder();
			stringBuilder.Append(string.Format("instance release {0}", ConsoleName));
			return stringBuilder.ToString();
		}

		string LoginToServiceAccount(FileInfo file)
		{
			return $"auth login --key-file {file.FullName} --default";
		}

		string InitUsingServiceAccount(FileInfo file)
		{
			if (!String.IsNullOrEmpty(ProjectName))
			{
				return $"init --key-file {file.FullName} --project {ProjectName}";
			}

			return $"init --key-file {file.FullName}";
		}

		string PrepStadiaRunArguments()
		{
			StringBuilder stringBuilder = new StringBuilder();

			stringBuilder.Append("run ");
			stringBuilder.Append("--instance ");
			stringBuilder.Append(ConsoleName);
			stringBuilder.Append(" --headless ");
			if (!String.IsNullOrEmpty(TestAccountName))
			{
				stringBuilder.Append($"--test-account {TestAccountName} ");
			}
			if (!String.IsNullOrEmpty(ProjectName))
			{
				stringBuilder.Append($"--project {ProjectName} ");
			}
			stringBuilder.Append("--cmd ");
			stringBuilder.Append(ProgramName);
			return stringBuilder.ToString();
		}

		protected override void ExecuteTask()
		{
			KillChromeClients();

			int timeout = ParseTimeout();

			// Make sure the environment variable is set to the directory of the SDK package we are using before running any executables
			Environment.SetEnvironmentVariable("GGP_SDK_PATH", Project.Properties["package.StadiaSDK.sdkdir"]);

			if (!String.IsNullOrEmpty(ServiceAccountKeyLocation))
			{
				if (File.Exists(ServiceAccountKeyLocation) == false)
				{
					throw new BuildException($"Service account key at {ServiceAccountKeyLocation} does not exist, make sure you have a Stadia Service Account Keyfile at this location");
				}
				FileInfo serviceAccountKey = new FileInfo(ServiceAccountKeyLocation);
				if (RunGGPProgram(LoginToServiceAccount(serviceAccountKey), timeout) != 0)
				{
					throw new BuildException($"Failed to login to service account at {ServiceAccountKeyLocation}");
				}
				if (RunGGPProgram(InitUsingServiceAccount(serviceAccountKey), timeout) != 0)
				{
					throw new BuildException($"Failed to init using service account at {ServiceAccountKeyLocation}");
				}
			}
			if (!EnsureReservation())
			{
				throw new BuildException($"Failed to reserve instance {ConsoleName}");
			}



			string programPath = Path.Combine(WorkingDir, ProgramName);
			if (RunProgram(StadiaCopy, $"{WorkingDir} -g {Project.GetPropertyOrFail("package.StadiaSDK.sdkdir")} -x {programPath} -i \"{ConsoleName}\"", timeout) != 0)
				throw new BuildException("Failed to mount.");

			string assetFilesDir = Path.Combine(WorkingDir, "assets");
			if (Directory.Exists(assetFilesDir))
			{
				if (RunGGPProgram("ssh put -r " + Path.Combine(assetFilesDir, "*"), timeout) != 0)
					throw new BuildException("Failed to Deploy Files.");
			}

			bool success;
			try
			{
				success = RunGGPProgram(PrepStadiaRunArguments(), timeout) == 0;
			}
			finally
			{
				KillChromeClients();
			}

			if (success)
				return;

			throw new BuildException("Build failed!");
		}

		static string EnsureReservationStdOut = string.Empty;
		static string EnsureReservationStdErr = string.Empty;
		/// <summary>
		/// Ensures that we have the instance reserved before interacting with it
		/// Will attempt to reserve the instance if it is not already reserved
		/// </summary>
		/// <returns></returns>
		private bool EnsureReservation()
		{
			bool haveReservation = false;
			Log.Minimal.WriteLine("Executing ggp {0}", CheckReservationStatusArguments());
			using (Process p = new Process())
			{
				ProcessStartInfo startInfo = new ProcessStartInfo(GGP, CheckReservationStatusArguments());

				startInfo.CreateNoWindow = true;
				startInfo.UseShellExecute = false;
				startInfo.RedirectStandardOutput = true;
				startInfo.RedirectStandardError = true;

				EnsureReservationStdOut = "";
				p.OutputDataReceived += new DataReceivedEventHandler((sender, e) =>
				{
					// Prepend line numbers to each line of the output.
					if (!String.IsNullOrEmpty(e.Data))
					{
						Console.WriteLine(e.Data);
						EnsureReservationStdOut += e.Data;
					}
				});

				EnsureReservationStdErr = "";
				p.ErrorDataReceived += new DataReceivedEventHandler((sender, e) =>
				{
					// Prepend line numbers to each line of the output.
					if (!String.IsNullOrEmpty(e.Data))
					{
						Console.WriteLine(e.Data);
						EnsureReservationStdErr += e.Data;
					}
				});

				p.StartInfo = startInfo;
				p.Start();
				p.BeginOutputReadLine();
				p.BeginErrorReadLine();
				p.WaitForExit();
				if (p.ExitCode != 0)
				{
					Log.Minimal.WriteLine(EnsureReservationStdErr);
				}
				else
				{
					if (EnsureReservationStdOut.Contains(ConsoleName))
					{
						haveReservation = true;
					}
				}
			}

			if (haveReservation == false)
			{
				using (Process p = new Process())
				{
					ProcessStartInfo startInfo = new ProcessStartInfo(GGP, ReserveInstance())
					{
						CreateNoWindow = true,
						UseShellExecute = false,
						RedirectStandardOutput = true,
						RedirectStandardError = true
					};

					p.StartInfo = startInfo;
					p.Start();
					p.WaitForExit();
					if (p.ExitCode != 0)
					{
						var stderr = p.StandardError.ReadToEndAsync().Result;
						Log.Warning.WriteLine(stderr);
					}
					haveReservation = (p.ExitCode == 0);
				}
			}
			return haveReservation;
		}
	}
}



