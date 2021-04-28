// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace EA.AndroidConfig
{
	[TaskName("AdbKillEmulators")]
	public class AdbKillEmulatorsTask : Task
	{
		private int mTimeout = Int32.MaxValue;
		
		[TaskAttribute("timeout")]
		public int TimeOut
		{
			get { return mTimeout; }
			set { mTimeout = value; }
		}
		
		/// <summary>A factory method to construct an KillEmulators task and execute it</summary>
		public static void Execute(Project project)
		{
			AdbKillEmulatorsTask killtask = new AdbKillEmulatorsTask();
			killtask.Project = project;
			killtask.Execute();
		}
		
		protected override void ExecuteTask()
		{
			try
			{
				var currentEmulators = FindEmulators();
				// send kill command to every emulator
				foreach (string emulator in currentEmulators)
				{
					string killCommand = String.Format("-s {0} emu kill", emulator);
					AdbTask.Execute(Project, killCommand, mTimeout, retries : 1, failonerror : false);
				}
				
				foreach (string emulator in currentEmulators)
				{
					// if the emulator still exists then sleep and give it some time to shutdown
					if(FindEmulators().Contains(emulator))
						Thread.Sleep(5000);
					if (FindEmulators().Contains(emulator))
						throw new Exception("Emulator did not shutdown properly after given the kill command.");
				}
			}
			catch (Exception)
			{
			}
		}
		
		private List<string> FindEmulators()
		{
			// find every emulator
			string deviceOutput = AdbTask.Execute(Project, "devices", mTimeout, retries : 1, failonerror : false, readstdout: true).StdOut;
			string[] lines = deviceOutput.Split(new string[] { System.Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
			List<string> emulators = new List<string>();
			foreach (string line in lines)
			{
				if(line.Contains("emulator-"))
				{
					string[] lineInfo = line.Split(new string[] { "\t", " " }, StringSplitOptions.RemoveEmptyEntries);
					emulators.Add(lineInfo[0]);
				}
			}
			return emulators;
		}
	}
	
	/// <summary>
	/// Kills the android debug bridge, which is a process that connects to the android emulator.
	/// The main reason to kill it is incase it is locking log files.
	/// </summary>
	[TaskName("AdbKill")]
	public class AdbKillTask : Task
	{
		private int mTimeout = Int32.MaxValue;
		private bool mKillEmulator = false;

		/// <summary>Kill all processes associated with the emulator, this will kill the emulator too.</summary>
		[TaskAttribute("killprocess")]
		public bool KillEmulator
		{
			get { return mKillEmulator; }
			set { mKillEmulator = value; }
		}

		[TaskAttribute("timeout")]
		public int TimeOut
		{
			get { return mTimeout; }
			set { mTimeout = value; }
		}

		/// <summary>A factory method to construct an Adbkill task and execute it</summary>
		public static void Execute(Project project, bool killemulator = false)
		{
			AdbKillTask killtask = new AdbKillTask();
			killtask.Project = project;
			killtask.KillEmulator = killemulator;
			killtask.Execute();
		}
		
		protected override void ExecuteTask()
		{	
			// try to gracefully kill the emulators by using the kill command before killing the server
			if (mKillEmulator)
			{					
				AdbKillEmulatorsTask.Execute(Project); // graceful kill
			}
			
			AdbTask.Execute(Project, "kill-server", mTimeout, 0, false);

			if (mKillEmulator)
			{
				string[] emulatorProcessNames = {
					"emulator",
					"emulator-arm",
					"emulator-x86",
					"qemu-system-i386",
					"qemu-system-x86_64",
					"adb_proxy",
					"adb"
				};
				ShutdownEmulatorProcesses(emulatorProcessNames, true);
			}
			
			// ADB is restarting so reset the device-serial
			Project.Properties["internal-android-use-target"] = Project.Properties["android-use-target"];
			Project.Properties["internal-android-device-serial"] = "";
		}

		private static void ShutdownEmulatorProcesses(string[] emulatorProcessNames, bool forceKill = true)
		{
			Process[] processList = Process.GetProcesses();
			List<Process> emulatorProcesses = new List<Process>();

			// Iterate over the number of live emulator processes and see how
			// many we need to kill.
			foreach (Process process in processList)
			{
				foreach (string emulatorProcessName in emulatorProcessNames)
				{
					if (process.ProcessName == emulatorProcessName)
					{
						emulatorProcesses.Add(process);

						// Issue a kill for each emulator process we find
						// in the current process list.
						try
						{
							if (forceKill || !process.CloseMainWindow()) // only kill for force close or for non-gui processes
							{
								process.Kill();
							}
						}
						catch (Exception)
						{
							// Win32Exceptions can be thrown if the process
							// can't be terminated or if it is terminating. If
							// it's the former, we try again down below and
							// abort if we can't completely terminate the
							// emulator processes. If it's the latter then, 
							// great news!, we're in the process of exiting.
						}
					}
				}
			}

			Stopwatch stopwatch = new Stopwatch();
			stopwatch.Start();
			int numEmulatorProcesses = emulatorProcesses.Count;

			// if the process zombifies we don't want to wait all day, so 
			// we pick a length of time that should be enough for the processes
			// to close out. 
			int timeoutMs = forceKill ? 5000 : 20000; // use large timeout if we're waiting for process to close properly
			while (stopwatch.ElapsedMilliseconds < timeoutMs)
			{
				emulatorProcesses.RemoveAll((Process process) =>
				{
					return process.HasExited;
				});

				foreach (Process process in emulatorProcesses)
				{
					if (forceKill)
					{
						// For processes that haven't yet died, re-issue the
						// kill order.
						try
						{
							process.Kill();
						}
						catch (Exception)
						{
							// This catches both the Win32Exception detailed
							// above, plus a potential InvalidOperationException
							// that can be thrown if the process has already
							// exited or there is no process associated with 
							// the object.
						}
					}
				}

				// All gone? We're done!
				if (emulatorProcesses.Count == 0)
				{
					return;
				}

				// Yield so that we don't consume all the resources while waiting
				// for death to happen.
				Thread.Sleep(1);
			}

			// This exception will be thrown if the loop above terminates, and
			// this only happens if it times out.
			throw new BuildException("Unable to terminate running emulator processes; aborting.");
		}
	}
}
