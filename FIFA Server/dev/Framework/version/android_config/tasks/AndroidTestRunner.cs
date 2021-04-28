// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.AndroidConfig
{
	/// <summary>
	/// A task for executing the Android Debug Bridge which is used for communicating with
	/// an android device or emulator.
	/// </summary>
	[TaskName("AndroidTestRunner")]
	public class TestRunnerTask : Task
	{
		int mTimeout = Int32.MaxValue;
		string mArgs;
		string mApkFile;
		bool mDebug;
		string mSoSearchPath;

		Process mDebugProcess;

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

		/// <summary>
		/// The command line arguments to pass to the application being run.
		/// </summary>
		[TaskAttribute("args")]
		public string Args
		{
			get { return mArgs; }
			set { mArgs = value; }
		}

		/// <summary>
		/// The apk file of the android package you want to run.
		/// </summary>
		[TaskAttribute("apkfile")]
		public string ApkFile
		{
			get { return mApkFile; }
			set { mApkFile = value; }
		}

		/// <summary>
		/// Whether you want to debug the application using the command line GDB debugger.
		/// </summary>
		[TaskAttribute("debug")]
		public bool Debug
		{
			get { return mDebug; }
			set { mDebug = value; }
		}

		/// <summary>
		/// The path to search for shared object files used to load symbols for GDB debugging.
		/// May contain multiple directories separated by semi-colons.
		/// Does not recursively search sub-directories.
		/// </summary>
		[TaskAttribute("sopath")]
		public string SoSearchPath
		{
			get { return mSoSearchPath; }
			set { mSoSearchPath = value; }
		}

		/// <summary>
		/// Checks the list of processes running on the device/emulator using the ps command. Finds any
		/// previously launched app processes still running and kills them.
		/// </summary>
		private void KillPreviousProcesses() 
		{
			Log.Status.WriteLine("[AndroidTestRunner] Checking for Previously Launched Processes");
			AdbTask adb_shell_ps = AdbTask.Execute(Project, "shell ps", mTimeout, 0, true, true);
			Regex expression = new Regex(@"com\.ea\.\S*", RegexOptions.IgnoreCase);
			Match match = expression.Match(adb_shell_ps.StdOut);
			while (match.Success)
			{
				string eaPackageName = match.Groups[0].Value;
				AdbTask.Execute(Project, string.Format("shell am force-stop {0}", eaPackageName), mTimeout);
				AdbTask.Execute(Project, string.Format("shell am kill {0}", eaPackageName), mTimeout);
				match = match.NextMatch();
			}
		}

		/// <summary>
		/// Splits the command line arguments on the white space characters but keeps quoted
		/// substrings as single arguments.
		/// </summary>
		private List<string> SplitArguments(string str)
		{
			Regex expression = new Regex("[^\"\'\\s]+|[\"\'][^\"\']*[\"\']");
			Match match = expression.Match(str);
			List<string> arguments = new List<string>();
			while (match.Success)
			{
				string value = match.Value;
				if (value.StartsWith("\"") && value.EndsWith("\""))
				{
					value = value.Replace("\"", "");
				}
				else if (value.StartsWith("\'") && value.EndsWith("\'"))
				{
					value = value.Replace("\'", "");
				}
				arguments.Add(value);
				match = match.NextMatch();
			}
			return arguments;
		}

		private string BuildActivityManagerExtraArgs(string activity)
		{
			// set argument 0 to be the application name to match the standard convention
			List<string> cmd_args = new List<string>() { activity };
			
			if (Args != null)
			{
				foreach (string arg in SplitArguments(Args))
				{
					cmd_args.Add(arg);
				}
			}

			string extra_args = "";
			if (cmd_args.Count >= 1)
			{
				extra_args = string.Format(" --ei argc {0} --es arg0 \'{1}\'", cmd_args.Count, cmd_args[0]);
				for (int i = 1; i < cmd_args.Count; ++i)
				{
					extra_args = string.Format("{0} --es arg{1} \'{2}\'", extra_args, i, cmd_args[i]);
				}
			}

			return extra_args;
		}

		private DateTime GetStartTimeForPid(string pid)
		{
			AdbTask pidDirectory = AdbTask.Execute(Project, "shell ls -ld /proc/" + pid, mTimeout, 0, true, true);
			string output = pidDirectory.StdOut;
			string[] columns = output.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

			// Columns if file:
			// Permissions      Owner       Group   Size    Date    Time    Filename
			// Columns if dir:
			// Permissions      Owner       Group           Date    Time    Filename

			if (columns.Length == 7)
			{
				throw new Exception("Expected directory for /proc/" + pid + " but encountered file?");
			}

			string date = columns[3].Trim();
			string time = columns[4].Trim();
			string filename = columns[5].Trim();

			if (filename != pid)
			{
				throw new Exception("Expected directory to be /proc/" + pid + " but encountered " + filename);
			}

			List<string> rawTimeComponents = new List<string>();
			rawTimeComponents.AddRange(date.Split('-'));
			rawTimeComponents.AddRange(time.Split(':'));

			List<int> timeComponents = new List<int>();
			foreach (string component in rawTimeComponents)
			{
				timeComponents.Add(int.Parse(component));
			}

			return new DateTime(timeComponents[0], timeComponents[1], timeComponents[2], timeComponents[3], timeComponents[4], 0);
		}

		private string FindPid(string package)
		{
			AdbTask adb_shell_ps = AdbTask.Execute(Project, "shell ps", mTimeout, 0, true, true);
			string output = adb_shell_ps.StdOut;
			string[] lines = output.Split(new string[] { System.Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
			List<string> pids = new List<string>();

			foreach (string line in lines)
			{
				string[] columns = line.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

				// Columns
				// USER     PID     PPID        VSIZE       RSS     WCHAN       PC      <blank>     NAME

				if (columns.Length != 9)
					continue;

				if (columns[8].Trim() != package)
					continue;

				pids.Add(columns[1]);
			}

			if (pids.Count == 0)
			{
				return null;
			} 

			List<DateTime> pidStartTimes = new List<DateTime>();
			foreach (string pid in pids)
			{
				DateTime time = GetStartTimeForPid(pid);
				pidStartTimes.Add(time);
			}

			int minIndex = 0;

			for (int i = 1; i < pids.Count; ++i)
			{
				if (pidStartTimes[i] < pidStartTimes[minIndex])
				{
					minIndex = i;
				}
			}

			return pids[minIndex];
		}

		/// <summary>
		/// Starts the test and returns the tests pid
		/// </summary>
		private void StartTest(string package, string activity)
		{
			Log.Status.WriteLine("[AndroidTestRunner] Running: {0}", ApkFile);

			string amExtraArgs = BuildActivityManagerExtraArgs(activity);

			string cmd_str = string.Format("shell \"am start -a android.intent.action.MAIN -n {0}/{1}{2}\"", package, activity, amExtraArgs);
			AdbTask am_start = AdbTask.Execute(Project, cmd_str, mTimeout, 0, true, true);
			Log.Status.WriteLine("[AndroidTestRunner] am start: {0}", am_start.StdOut);
				
			// Checking StdOut for errors saying that the test failed to start, otherwise would could be waiting a long time for a test that is not running
			if (new Regex(@".*Error: Activity class {.*} does not exist", RegexOptions.IgnoreCase).Match(am_start.StdOut).Success)
			{
				throw new Exception("Test Failed to Start - Could not find activity class");
			}
		}

		// list of common warnings and messages we want to filter out of the logcat results
		private String[] logcat_filter = new String[] { 
			"PR_CAPBSET_DROP [-\\d]+ failed: Invalid argument.",
			"E/cutils-trace\\([-\\s\\d]+\\): Error opening trace file: No such file or directory",
			"W/WindowManager\\([-\\s\\d]+\\): Screenshot failure taking screenshot"
		};

		private void PrintFilteredLogcatOutput(string line, string pid = null)
		{
			if (line.Trim() != String.Empty)
			{
				// check to see if the pid is in the line
				if(!String.IsNullOrEmpty(pid) && !line.Contains(pid))
					return;
					
				foreach (string filter in logcat_filter)
				{
					Regex filter_expression = new Regex(filter, RegexOptions.IgnoreCase);
					if (filter_expression.Match(line).Success)
						return;
				}

				Log.Minimal.WriteLine("logcat> {0}", line);
			}
		}

		public void CreateGdbSetupScript()
		{
			StreamWriter sw = new StreamWriter(Path.Combine(Project.ExpandProperties("${package.builddir}/${config}/build/temp"), "setup_script.gdb"));
			sw.WriteLine(string.Format(@"# The following script was automatically generated by AndroidTestRunner

# The solib-search-path must be set otherwise gdb will not be able to locate the shared library for the application
set solib-search-path {0}

# Connect to the gdbserver that was forwarded on port 5039
target remote :5039
			", mSoSearchPath));
			sw.Close();
		}

		/// <summary>
		/// Try to determine the AndroidOS version, if it fails it will assume you are using the same version that the SDK package is set to
		/// </summary>
		private float GetAndroidOSVersion()
		{
			AdbTask adb_shell_ps = AdbTask.Execute(Project, "shell grep ro.build.version.sdk= /system/build.prop", mTimeout, 0, true, true);
			string output = adb_shell_ps.StdOut;

			string[] tokens = output.Trim().Split(new string[] { "=" }, StringSplitOptions.RemoveEmptyEntries);
			float version;
			if (tokens.Length == 2 && float.TryParse(tokens[1], out version))
			{
				return version;
			}
			if (float.TryParse(Project.Properties["android_api"], out version))
			{
				return version;
			}
			return 0;
		}

		/// <summary>
		/// This function attaches to the pid specified and remotely debugs it using gdb
		/// </summary>
		public void DebugProcess(string package, string activity, string gdb_path)
		{
			string pid = FindPid(package);

			if (pid == null)
			{
				Log.Status.WriteLine("[AndroidTestRunner] Unable to spawn process on phone. Ensure the package is installed.");
				AdbKillTask.Execute(Project);

				return;
			}

			// enable port forwarding so gdb can connect to gdbserver
			AdbTask.Execute(Project, "forward tcp:5039 tcp:5039", mTimeout);

			// start gdbserver and attach to process specified
			string gdb_server_path = string.Format("/data/data/{0}/lib/gdbserver", package);
			AdbTask.ExecuteBackgroundProcess(Project, string.Format("shell {0} :5039 --attach {1}", gdb_server_path, pid), false);

			// transfer app_process to the local machine so that we can pass it to the local gdb.
			string app_process_path = "/system/bin/app_process";

			float os_version = GetAndroidOSVersion();
			if (os_version >= 21) 
			{
				// in Android 5.0 (21) app_process was replaced by a symlink to either app_process32 or app_process64
				// and trying to pull a symlink will fail
				app_process_path = "/system/bin/app_process32";
			}
			string tempdir = Project.ExpandProperties("${package.builddir}\\${config}\\build\\temp");
			AdbTask.Execute(Project, string.Format("pull {0} {1}\\app_process", app_process_path, tempdir), mTimeout);

			// GDB is started in a new window so that users can type into STDIN for interacting with GDB
			string args = string.Format("/c start /WAIT \"Remote GDB debugging session for {0}\" \"{1}\" -x {2}\\setup_script.gdb -e {2}\\app_process", mApkFile, gdb_path, tempdir);
			ProcessStartInfo startInfo = new ProcessStartInfo("cmd.exe", args);
			startInfo.UseShellExecute = false;
			startInfo.RedirectStandardOutput = true;
			startInfo.RedirectStandardError = true;
			startInfo.WorkingDirectory = tempdir;

			Log.Status.WriteLine("[AndroidTestRunner] {0} {1}", gdb_path, args);

			mDebugProcess = new Process();
			mDebugProcess.StartInfo = startInfo;
			mDebugProcess.Start();

			// gdb starts in a new window while the logcat output is printed to the main window
			Process logcat_process = AdbTask.ExecuteBackgroundProcess(Project, "logcat", true);
			while (!mDebugProcess.HasExited)
			{
				string line = logcat_process.StandardOutput.ReadLine();
				PrintFilteredLogcatOutput(line);
			}
			logcat_process.Kill();
		}

		public string StartAndWaitForTestToComplete(string package, string activity, int timeoutms)
		{
			Process logcat_process = AdbTask.ExecuteBackgroundProcess(Project, "logcat", true);

			StartTest(package, activity);
				
			Regex complete_expression = new Regex(@"AndroidSDK RUN COMPLETED. RETURN CODE: ([-\d]+)", RegexOptions.IgnoreCase);
			Regex eamain_complete_expression = new Regex(@"RETURNCODE=([-\d]+)", RegexOptions.IgnoreCase);
			Regex error_expression = new Regex(@"terminated by signal|EA_ASSERT failed\:|FATAL EXCEPTION: main", RegexOptions.IgnoreCase);

			Regex[] process_start_expressions = new Regex[] { 
				new Regex(@"Start proc " + package + @".*pid=(\d+)"),
				new Regex(@"Start proc (\d+):" + package),
			};

			Regex process_exited_due_to_signal_expression = null;
			string pid = null;

			TimeSpan timeout_duration = TimeSpan.FromMilliseconds(timeoutms);
			Stopwatch stopwatch = Stopwatch.StartNew();
			bool Terminated = false;

			string return_code = "1";
			while (stopwatch.Elapsed < timeout_duration)
			{
				string line = logcat_process.StandardOutput.ReadLine();

				if (pid == null)
				{
					foreach (Regex process_start_expression in process_start_expressions)
					{
						Match start_match = process_start_expression.Match(line);
						if (start_match.Success)
						{
							pid = start_match.Groups[1].Value;
							process_exited_due_to_signal_expression = new Regex("Process " + pid + " exited due to signal");
							
							// we need to whitelist the app or it could be filtered by logcat for writing too much to the logcat
							AdbTask.Execute(Project, "logcat -P " + pid);
							
							break;
						}
					}
				}
				
				// if we get a fatal exception then stop filtering on the pid
				if(line.Contains("Fatal signal"))
					pid = null;

				PrintFilteredLogcatOutput(line, pid);

				Match match = complete_expression.Match(line);
				if (match.Success) 
				{
					Terminated = true;
					return_code = match.Groups[1].Value;
					break;
				}

				// check for error expressions (ie. seg faults, etc)
				match = error_expression.Match(line);
				if (match.Success)
				{
					Terminated = true;
					break;
				}

				if (process_exited_due_to_signal_expression != null)
				{
					match = process_exited_due_to_signal_expression.Match(line);
					if (match.Success)
					{
						Terminated = true;
						break;
					}
				}

				match = eamain_complete_expression.Match(line);
				if (match.Success)
				{
					Terminated = true;
					return_code = match.Groups[1].Value;
					break;
				}
			}
			if (Terminated == false)
			{
				Log.Status.WriteLine("[AndroidTestRunner] Timed out waiting for test to terminate, after {0} ms", timeoutms);
			}

			// kill logcat
			logcat_process.Kill();

			return return_code;
		}

		protected override void ExecuteTask()
		{
			KillPreviousProcesses();

			AdbTask.Execute(Project, "logcat -c", mTimeout);

			string package = Project.Properties["package.android_config.deploypackagename"];
			string activity = Project.Properties["package.android_config.deploypackageactivity"];

			int test_timeout = mTimeout;
			if (Debug)
			{
				// GRADLE-TODO: can probably nuke this block? it's very unlikely users would debug this way rather than using VS /w Tegra
				StartTest(package, activity);
			
				string gdb_path = Project.GetPropertyOrDefault("android-gdb", null);
				if (gdb_path == null)
				{
					string gdb_file = string.Format("{0}-gdb.exe", Project.Properties["package.AndroidNDK.tool-prefix"]);
					gdb_path = Path.Combine(Project.Properties["package.AndroidNDK.toolchain-dir"], "bin", gdb_file);
				}

				CreateGdbSetupScript();
				DebugProcess(package, activity, gdb_path);
			}
			else
			{
				
				string return_code = StartAndWaitForTestToComplete(package, activity, test_timeout);

				KillPreviousProcesses();

				if (return_code != "0")
				{
					Log.Status.WriteLine("[AndroidTestRunner] Error: Test Failed with error code: {0}!", return_code);
					throw new Exception("Test Failed");
				}
			}

			Log.Status.WriteLine("[AndroidTestRunner] Finished Successfully!");
		}
	}
}

