// (c) Electronic Arts. All rights reserved.

using System;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading;

using EA.Eaconfig;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace EA.AndroidConfig
{
	/// <summary>Deploys the android package file (APK) to the android device</summary>
	[TaskName("DeployAndroidPackage")]
	public class DeployAndroidPackageTask : Task
	{
		[TaskAttribute("packagefile")]
		public string PackageFile {
			get { return mPackageFile; }
			set { mPackageFile = value; }
		} string mPackageFile;

		[TaskAttribute("timeout")]
		public int Timeout
		{
			get { return mTimeout; }
			set { mTimeout = value; }
		} protected int mTimeout = Int32.MaxValue;

		public DeployAndroidPackageTask()
			: base()
		{
		}

		private string GetPackageName(string apkfile)
		{
			// GRADLE-TODO: this code is pretty fragile, most importantly it doesn't error if it fails to do it's job, but in general it's parsing isn't very robust

			string program = PathNormalizer.Normalize(Path.Combine(Project.Properties.GetPropertyOrFail("package.AndroidSDK.buildtooldir"), "aapt2.exe"));
			string args = String.Concat("dump xmltree \"", apkfile, "\" --file \"AndroidManifest.xml\"");
			ProcessStartInfo startInfo = new ProcessStartInfo(program, args);
			startInfo.CreateNoWindow = true;
			startInfo.UseShellExecute = false;
			startInfo.RedirectStandardOutput = true;
			startInfo.RedirectStandardError = true;

			Log.Status.WriteLine("[DeployAndroidPackage] {0} {1}", program, args);

			Process aapt_process = new Process();
			aapt_process.StartInfo = startInfo;

			aapt_process.Start();

			const string packageTag = "A: package=";
			string e_tag = "";
			string package = null;
			string activity = null;
			string allOutput = aapt_process.StandardOutput.ReadToEnd();
			foreach (string line in allOutput.Split('\n'))
			{
				string lineTrimmed = line.Trim();
				if (lineTrimmed.StartsWith("E: "))
				{
					e_tag = lineTrimmed;
				}
				else if (lineTrimmed.StartsWith(packageTag))
				{
					int start = packageTag.Length + 1;
					int end = lineTrimmed.IndexOf('"', start);
					package = lineTrimmed.Substring(start, end - start);
				}
				else if (e_tag.StartsWith("E: activity") && lineTrimmed.StartsWith("A: http://schemas.android.com/apk/res/android:name"))
				{
					int start = lineTrimmed.IndexOf('"', 0) + 1;
					int end = lineTrimmed.IndexOf('"', start);
					activity = lineTrimmed.Substring(start, end - start);
				}
				if (package != null && activity != null)
				{
					break;
				}
			}

			aapt_process.WaitForExit();

			Project.Properties["package.android_config.deploypackagename"] = package;
			Project.Properties["package.android_config.deploypackageactivity"] = activity;

			return package;
		}

		private void DeployFile()
		{
			Stopwatch stopwatch = Stopwatch.StartNew();

			bool fast_install = Project.Properties.GetBooleanPropertyOrDefault("package.android_config.enable-fast-install", true);
			bool using_emulator = (Project.Properties["internal-android-use-target"] == "emulator");
			bool is_x86 = (Project.Properties["config-processor"] == "x86" || Project.Properties["config-processor"] == "x64");
			bool gpu_emulation = Project.Properties.GetBooleanPropertyOrDefault("package.AndroidSDK.enable-gpu-emulation", false);

            if (fast_install && using_emulator && gpu_emulation == false && is_x86 == false)
			{
				// Running a program during deployment wakes up the emulator to make it deploy 10x faster.
				// we only use this method for ARM with snapshot, since it crashes/hangs if we use 
				// the emulator arguments -qemu -icount auto.
				string adbWorkingDir = AndroidBuildUtil.NormalizePath(Project.Properties["package.AndroidSDK.platformtooldir"]);
				string adbpath = Path.Combine(adbWorkingDir, "adb.exe");

				const string args = "-e shell sh -c \"while true; do echo wake-up android emulator!; done\" > /dev/null";
				ProcessStartInfo processStartInfo = new ProcessStartInfo(adbpath, args);
				Log.Status.WriteLine("[exec] {0} {1}", adbpath, args);
				var busy_process = Process.Start(processStartInfo);

				try
				{
					ADBInstall();
				}
				finally
				{
					busy_process.Close();
				}
			}
			else
			{
				ADBInstall();
			}

            Log.Status.WriteLine("[debug] It took {0} ms to install apk", stopwatch.ElapsedMilliseconds);
		}

		private AdbTask ADBInstallWithRetryRegex()
		{
			AdbTask installTask = null;
			try
			{
				string abi = Project.Properties.GetPropertyOrFail("package.AndroidSDK.android_abi");
				string installArgumentsWithAbi = String.Concat("install --abi " + abi + " \"", PackageFile, "\"");
				installTask = AdbTask.Execute(Project, installArgumentsWithAbi, mTimeout, 1, readstdout: true, failonerror: false);
			}
			catch (Exception e)
			{
				throw new BuildException("Unable to install package: " + e.Message);
			}
			
			if(installTask.StdOut.Contains("Unknown option: --abi"))
			{
				Log.Status.WriteLine("[exec] Install failed when using --abi option.  Trying again without it.");
				try
				{
					string installArguments = String.Concat("install \"", PackageFile, "\"");
					installTask = AdbTask.Execute(Project, installArguments, mTimeout, 1, readstdout: true, failonerror: false);
				}
				catch (Exception e)
				{
					throw new BuildException("Unable to install package: " + e.Message);
				}
			}
			
			return installTask;
		}
		
		private void ADBInstall()
		{
			AdbTask installTask = ADBInstallWithRetryRegex();
			
			// adb install can fail with but with 0 return value and an out of space error even when the device
			// is not out of space - rebooting seems to be the only remedy
			if (installTask.StdOut.Contains("[INSTALL_FAILED_INSUFFICIENT_STORAGE"))
			{
				Log.Minimal.WriteLine("Suspicious out of space error: [INSTALL_FAILED_INSUFFICIENT_STORAGE]. Rebooting device...");
				AdbTask.Execute(Project, "reboot", mTimeout);
				AdbTask.Execute(Project, "wait-for-device", 60000, 1);
				Thread.Sleep(5000);		// after a reboot and installing a task, it takes a few seconds before you can run the package without errors
				AdbTask.Execute(Project, "shell input keyevent 82", 5000); // swipe up to unlock(NOTE: this assumes there is no password)
				installTask = ADBInstallWithRetryRegex();
			}

			// we already queried device abis, so if we get an abi error print the results for debugging
			if (installTask.StdOut.Contains("[INSTALL_FAILED_NO_MATCHING_ABIS"))
			{
				Log.Minimal.WriteLine("Detected [INSTALL_FAILED_NO_MATCHING_ABIS] error. Previously detected abis were: " + (Project.Properties["internal-android-detected-abis"] ?? String.Empty) + " .");
			}

			// adb install can return 0 even when it fails, we can still catch these by checking for 
			// patterns in the output
			Match firstMatch = Regex.Match(installTask.StdOut, @"(Failure \[.*\])|((Error)\:.*)");
			if (firstMatch.Success)
			{
				throw new BuildException("Unable to install package: " + firstMatch.Value);
			}

			if(installTask.ExitCode != 0)
			{
				throw new BuildException("Unable to install package: " + installTask.StdOut);
			}
		}

		protected override void ExecuteTask()
		{
			if (!File.Exists(PackageFile))
			{
				throw new BuildException("ERROR: AndroidDeployTask can't find package file: " + PackageFile);
			}

			string packagename = GetPackageName(PackageFile);

			UninstallAndroidPackage.Execute(Project, packagename, mTimeout); // GRADLE-TODO: seems like we could add -r (replace) to our install command and then we could skip this?

			DeployFile();
		}
	}

	/// <summary>Deploys asset files to the device's SD Card</summary>
	[TaskName("DeployToSdCard")]
	public class DeployToSdCardTask : Task
	{
		[TaskAttribute("modulename")]
		public string ModuleName {
			get { return mModuleName; }
			set { mModuleName = value; }
		} protected string mModuleName;

		[TaskAttribute("groupname")]
		public string GroupName
		{
			get { return mGroupName; }
			set { mGroupName = value; }
		} protected string mGroupName;

		[TaskAttribute("timeout")]
		public int Timeout
		{
			get { return mTimeout; }
			set { mTimeout = value; }
		} protected int mTimeout = Int32.MaxValue;

		public DeployToSdCardTask()
			: base()
		{
		}

		static string CanonicalizePath(string path)
		{
			return path.Replace(@"\", "/").TrimStart(new char[] { '/' });
		}

		protected override void ExecuteTask()
		{
			FileSet fs = FileSetUtil.GetFileSet(Project, GroupName + ".android.sdcard.deploy-contents-fileset");
			if (fs == null)
			{
				return;
			}

			bool fast_deploy = Project.Properties.GetBooleanPropertyOrDefault("package.android_config.enable-fast-file-deployment", true);
			bool gpu_emulation = Project.Properties.GetBooleanPropertyOrDefault("package.AndroidSDK.enable-gpu-emulation", false);
			bool is_x86 = (Project.Properties["config-processor"] == "x86");
			bool use_emulator = (Project.Properties["internal-android-use-target"] == "emulator");

			if (fast_deploy && is_x86 == false && gpu_emulation == false && use_emulator)
			{
				// Launch a process to keep the emulator active during transfer.  This action may
				// seem counter intuitive, but it improves the transfer rate greatly.
				// only needed for ARM with snapshot, since icount commandline argument causes hang.
				AdbTask.Execute(Project, "shell am start -n com.example.android.apis/.graphics.Sweep", mTimeout, 1);
			}

			string sdcardPartialDir = "com.ea." + ModuleName;

			foreach (FileItem fi in fs.FileItems)
			{
				var baseDir = fi.BaseDirectory ?? fs.BaseDirectory;

				OptionSet os = null;
				if (!String.IsNullOrEmpty(fi.OptionSetName))
				{
					os = Project.NamedOptionSets[fi.OptionSetName];
				}

				if (os != null && os.Options.Contains("android.sdcard.partialdir"))
				{
					sdcardPartialDir = CanonicalizePath(os.Options["android.sdcard.partialdir"]);
				}

				if (!String.IsNullOrEmpty(sdcardPartialDir) && !sdcardPartialDir.EndsWith("/"))
				{
					sdcardPartialDir += "/";
				}

				string suffixPath = Path.GetFileName(fi.FullPath);
				if (fi.FullPath.IndexOf(baseDir, StringComparison.InvariantCultureIgnoreCase) == 0)
				{
					suffixPath = fi.FullPath.Substring(baseDir.Length);
				}

				string arguments = string.Concat("push ", fi.FullPath, " /sdcard/", sdcardPartialDir, CanonicalizePath(suffixPath));
				AdbTask.Execute(Project, arguments, Timeout, 1);
			}
		}
	}
}
