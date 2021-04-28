// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;

using EA.Eaconfig;
using EA.AndroidSDK.Tasks;

namespace EA.AndroidConfig
{
	// GRADLE-TODO: there are like 9 different ways external processes are called in this file, lets try and clean it up

	// determine how the emulator does snapshotting
	public enum AvdSnapShotMode
	{
		NoSnapshot, // neither load a snapshot nor save one
		NoShapshotLoad, // doesn't load snapshot, has to perform full boot which is quite slow, emulator will save image state when it exits
		NoSnapshotSave // will load previously saved state, when the emulator exits it will not try to save the state
	}

	[TaskName("AndroidEmulatorInit")]
	public class EmulatorInitTask : Task
	{
		// costants
		const int SDCardSizeMiB = 512;
		const int SDCardSizeB = SDCardSizeMiB * 1024 * 1024;
		private const int AVDVersion = 1;
		private const string AVDMarkerVersionText = "AVD Version: ";

		// private
		string m_sdkInstalledDir;
		string m_toolsDir;
		string m_jdkDir;
		string m_systemImageType;
		string m_systemImageDir;
		bool m_isX86;
		string m_homeDir;
		string m_avdDir;
		string m_avdPath;
		string m_deviceType;
		string m_avdName;
		string m_imageSDKVersion;
		
		// avd options
		string m_imageRAMSizeMiB;
		string m_internalStorageSizeMiB;

		private static Regex s_avdVersionMatch = new Regex("^" + AVDMarkerVersionText + "(\\d+)\\s$", RegexOptions.Multiline);

		protected override void ExecuteTask()
		{
			try
			{
				// start the environment fresh
				AdbKillTask.Execute(Project, true);
				AdbTask.Execute(Project, "start-server");
				Project.Properties["internal-android-use-target"] = "emulator";
				Project.Properties["internal-android-device-serial"] = "";

				m_homeDir = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
				m_avdDir = AndroidBuildUtil.NormalizePath(Path.Combine(m_homeDir, ".android", "avd"));

				m_imageSDKVersion = Project.Properties["package.android_config.imageSDKVersion"] ?? Project.Properties["package.AndroidSDK.targetSDKVersion"];
				m_deviceType = Project.Properties["package.android_config.device-type"] ?? "Nexus 5";
				m_isX86 = Project.Properties["config-processor"] == "x86" || Project.Properties["config-processor"] == "x64";
				m_systemImageType = Project.Properties["package.android_config.system-image-type"] ?? "android";
				m_systemImageDir = (m_systemImageType == "android") ? "default" : m_systemImageType;
				m_avdName = String.Format("{0}_{1}_{2}_sdk{3}", m_deviceType.Replace(' ', '_'), m_systemImageType, Project.Properties.GetPropertyOrFail("package.AndroidSDK.android_abi"), m_imageSDKVersion);
				m_sdkInstalledDir = AndroidBuildUtil.NormalizePath(Project.Properties["package.AndroidSDK.appdir"]);
				m_toolsDir = AndroidBuildUtil.NormalizePath(Path.Combine(m_sdkInstalledDir, "tools"));

				TaskUtil.Dependent(Project, "jdk");
				m_jdkDir = AndroidBuildUtil.NormalizePath(Project.Properties.GetPropertyOrFail("package.jdk.appdir"));
				m_imageRAMSizeMiB = Project.Properties["package.android_config.ram-size"] ?? "1536";
				m_internalStorageSizeMiB = Project.Properties["package.android_config.hd-size"] ?? "800";
				m_avdPath = AndroidBuildUtil.NormalizePath(Path.Combine(m_avdDir, m_avdName + ".avd"));

				string sdCardPath = AndroidBuildUtil.NormalizePath(Path.Combine(m_avdDir, "sdcard.img")); // share sd card between all images to save space
				bool gpuEmulation = Project.Properties.GetBooleanPropertyOrDefault("package.android_config.enable-gpu-emulation", false);
				bool headlessAVD = Project.Properties.GetBooleanPropertyOrDefault("package.android_config.emulate-headless", false);
				bool useSnapshot = !(Project.Properties.GetBooleanPropertyOrDefault("package.android_config.no-snapshot", false)); 
				
				string avdCompleteMarkerFile = Path.Combine(m_avdPath, "AvdImageCreateCompleteMarker.txt");
				
				// download sdk components needed to run emulator
				DownloadEmulator();
				DownloadSystemImage();

				// x86 emulator requires HAXM or Hyper-V 
				if (m_isX86)
				{
					EnsureEmulationSupported();
				}

				// check sdcard file exists, if not create it
				if (!File.Exists(sdCardPath))
				{
					Log.Status.WriteLine("Emulator SD card image has not been created. Doing one time SD card creation.");
					if (!Directory.Exists(m_avdDir)) // ensure avd directory exists before writing to it
					{
						Directory.CreateDirectory(m_avdDir);
					}
					MakeSDCard(sdCardPath, SDCardSizeB);
				}

				// check avd exists, if not create it
				bool rebuildAvdPath = false;
				if (!Directory.Exists(m_avdPath))
				{
					rebuildAvdPath = true;
					Log.Minimal.WriteLine("Target emulator AVD for target SDK and ABI has not been created. Doing one time AVD creation...");
				}
				else
				{
					if (!File.Exists(avdCompleteMarkerFile))
					{
						rebuildAvdPath = true;
						Log.Status.WriteLine("Target emulator AVD path exists but AVD creation marker file does not. Folder might be corrupted. Need to recreate it...");
					}
					else
					{
						bool avdMarkerOutOfDate = false;
						bool avdMarkerCorrupt = false;
						int avdMarkerVersion = 0;
						string avdMarkerContext = null;
						try
						{
							avdMarkerContext = File.ReadAllText(avdCompleteMarkerFile);
						}
						catch
						{
							avdMarkerCorrupt = true;
						}
						if (!avdMarkerCorrupt)
						{
							Match match = s_avdVersionMatch.Match(avdMarkerContext);
							if (match.Success)
							{

								if (!Int32.TryParse(match.Groups[1].Value, out avdMarkerVersion))
								{
									avdMarkerCorrupt = true;
								}
								else
								{
									avdMarkerOutOfDate = avdMarkerVersion != AVDVersion;
								}
							}
							else
							{
								avdMarkerOutOfDate = true;
							}
						}

						if (avdMarkerCorrupt)
						{
							rebuildAvdPath = true;
							Log.Minimal.WriteLine("AVD marker file corrupt. Recreating AVD...");
						}
						else if (avdMarkerOutOfDate)
						{
							rebuildAvdPath = true;
							Log.Minimal.WriteLine("AVD marker file out of date (version {0} found, version {1} required). Recreating AVD...", avdMarkerVersion, AVDVersion);
						}
						else
						{
							// if we change some setttings, we need to make sure the avd we are about to use supports it
							string iniPath = AndroidBuildUtil.NormalizePath(Path.Combine(m_avdDir, m_avdName + ".avd", "config.ini"));
							SortedDictionary<string, string> iniSettings = ReadIniSettings(iniPath);

							bool settingsChanged = false;

							// gpu
							string gpuEnabledString = "";
							if (iniSettings.TryGetValue("hw.gpu.enabled", out gpuEnabledString))
							{
								settingsChanged = !((gpuEnabledString == "yes" && gpuEmulation) || (gpuEnabledString == "no" && !gpuEmulation));
							}

							if (settingsChanged)
							{
								rebuildAvdPath = true;
								Log.Minimal.WriteLine("Settings has changed since the last time the emulator AVD was created. Recreating AVD...");
							}
						}
					}
				}
				
				if (rebuildAvdPath)
				{
					// call avd manager to generate .avd
					CreateAvd
					(
						avdCompleteMarkerFile: avdCompleteMarkerFile,
						sdCardPath: sdCardPath,
						rebuildAvd: rebuildAvdPath,
						useSnapshot: useSnapshot,
						gpuEmulation: gpuEmulation
					);

					// if we aren't using snapshots then don't bother trying to create a snapshot
					if (useSnapshot)
					{
						// call emulator with snapshot saving, this will do the OS boot up and then save that state, from here on out we we can load the snapshot of a fresh boot after install
						StartEmulator
						(
							AvdSnapShotMode.NoShapshotLoad, 
							gpuEmulation,
							headlessAVD
						);

						// try and close emulator gently first as it perform snapshot save on close which will be missed if we kill it violently
						Log.Minimal.WriteLine("Waiting for emulator to save snapshot...");
						try
						{							
							AdbKillEmulatorsTask.Execute(Project); // graceful kill
						}
						catch(Exception ex)
						{
							Log.Status.WriteLine(ex.ToString());
							AdbKillTask.Execute(Project, true);
							Directory.Delete(m_avdPath, true); // delete avd so we can try again
							throw;
						}
					}
				}

				// start emulator and leave it running
				StartEmulator(useSnapshot ? AvdSnapShotMode.NoSnapshotSave : AvdSnapShotMode.NoSnapshot, gpuEmulation, headlessAVD);
			}
			catch
			{
				AdbKillTask.Execute(Project, true);
				throw;
			}
		}

		private void EnsureEmulationSupported(bool autoInstallHaxm = true)
		{
			string emulatorCheckPath = Path.Combine(Path.GetDirectoryName(PathNormalizer.Normalize(Project.Properties["package.AndroidSDK.emulator"])), "emulator-check.exe"); // GRADLE-TODO, this is a little unwieldy - maybe we straight up assume emulator path rathern than having a property

			Process emulatorCheckProcess = new Process();
			emulatorCheckProcess.StartInfo.FileName = emulatorCheckPath;
			emulatorCheckProcess.StartInfo.Arguments = "hyper-v accel";

			ProcessRunner checkRunner = new ProcessRunner(emulatorCheckProcess);
			checkRunner.Start();

			// example output from emulator check:
			/*
				hyper-v:
				0
				Hyper-V is not installed
				hyper-v
				accel:
				0
				HAXM version 7.3.2 (4) is installed and usable.
				accel
			*/

			IEnumerator<string> trimmedOutputLines = checkRunner.StandardOutput
				.Split('\n')
				.Select(line => line.Trim())
				.GetEnumerator();

			bool hyperCheckSucceeded = false;
			bool haxmCheckedSucceeded = false;

			ReadEmulatorCheckLine(trimmedOutputLines, expected: "hyper-v:");
			hyperCheckSucceeded = ReadEmulatorCheckLine(trimmedOutputLines) == "2"; // "2" apparently is the good return code?
			string hyperVDescription = ReadEmulatorCheckLine(trimmedOutputLines);
			hyperCheckSucceeded = hyperVDescription == "Hyper-V is enabled" && hyperCheckSucceeded;
			ReadEmulatorCheckLine(trimmedOutputLines, expected: "hyper-v");
			ReadEmulatorCheckLine(trimmedOutputLines, expected: "accel:");
			haxmCheckedSucceeded = ReadEmulatorCheckLine(trimmedOutputLines) == "0"; // so far I haven't found a configuration where this returns anything but 0, so if isn't 0 assume badness
			string haxmDescription = ReadEmulatorCheckLine(trimmedOutputLines);
			ReadEmulatorCheckLine(trimmedOutputLines, expected: "accel");
			// don't validate any output after this, we're shouldn't get any but it's probably not woth failing even if we do

			// if hyper-v is ready to go, then we can early out
			if (hyperCheckSucceeded)
			{
				Log.Status.WriteLine("Hyper-V is enabled. Emulation is possible.");
				return;
			}

			// to figure out if HAXM is really good to go we need to parse the output
			if (haxmCheckedSucceeded)
			{
				AndroidSDKManager.Component haxmComponent = AndroidSDKManager.GetLatestComponentVersion(Project, "Hardware_Accelerated_Execution_Manager");
				string targetHaxmVersion = haxmComponent.ComponentVersion;

				Match installMatch = Regex.Match(haxmDescription, "HAXM version ([\\d+\\.]+) (?:\\(\\d+\\) )?is installed and usable\\.");
				bool isInstalled = installMatch.Success;
				string installedVersion = installMatch.Groups[1].Value;
				if (isInstalled && installedVersion == targetHaxmVersion)
				{
					Log.Status.WriteLine("HAXM is installed and usable. Emulation is possible.");
					return;
				}
				else if (autoInstallHaxm && (Regex.IsMatch(haxmDescription, "HAXM is not installed") || (isInstalled && installedVersion != targetHaxmVersion) ))
				{
					// try to download and install HAXM
					AndroidSDKManager.InstallComponent(Project, haxmComponent, m_sdkInstalledDir);

					string haxmDir = AndroidBuildUtil.NormalizePath(Path.Combine(m_sdkInstalledDir, "extras", "intel", "Hardware_Accelerated_Execution_Manager")); // GRADLE-TODO: component path concept would be good here
					string haxmSilentInstaller = AndroidBuildUtil.NormalizePath(Path.Combine(haxmDir, "silent_install.bat"));
					ExecTask haxmInstaller = new ExecTask(Project)
					{
						Program = haxmSilentInstaller,
						WorkingDirectory = haxmDir
					};
					haxmInstaller.Execute();

					// check upport again, but don't try installing HAXM again
					EnsureEmulationSupported(autoInstallHaxm: false);
					return;
				}
			}

			// if we get here then nothing worked
			throw new BuildException("Emulation not supported. Emulation check reported the following - Hyper V: " + hyperVDescription + ", HAXM: " + haxmDescription);
		}

		private void DownloadEmulator()
		{
			AndroidSDKManager.Component emulatorComponent = AndroidSDKManager.GetLatestComponentVersion(Project, "emulator");
			AndroidSDKManager.InstallComponent(Project, emulatorComponent, m_sdkInstalledDir);
		}
		
		private void DownloadSystemImage()
		{
			string abi = Project.Properties.GetPropertyOrFail("package.AndroidSDK.android_abi");
			string androidEmulatorPackageName = String.Join(";", "system-images", m_systemImageType, abi);

			ReadOnlyCollection<AndroidSDKManager.Component> versions = AndroidSDKManager.GetAvailableComponentVersions(Project, androidEmulatorPackageName);
			AndroidSDKManager.Component targetComponent = versions.FirstOrDefault(v => v.ComponentVersion.StartsWith(m_imageSDKVersion));
			if (targetComponent == null)
			{
				throw new BuildException("Could not find an emulator image the matches major version '" + m_imageSDKVersion + "'! Found versions: " + String.Join(", ", versions.Select(v => v.ComponentVersion)) + ".");
			}

			AndroidSDKManager.InstallComponent(Project, targetComponent, m_sdkInstalledDir);
		}

		private void StartEmulator(AvdSnapShotMode snapshotMode, bool gpuEmulation = false, bool headlessAVD = false)
		{
			string emulatorPath = AndroidBuildUtil.NormalizePath(Project.Properties["package.AndroidSDK.emulator"]);
			
			Log.Debug.WriteLine("Using emulator images from '{0}'.", m_homeDir);

			bool fastInstall = Project.Properties.GetBooleanPropertyOrDefault("package.android_config.enable-fast-install", false);
			bool verbose = Project.Properties.GetBooleanPropertyOrDefault("package.AndroidSDK.emulator.verbose", false);

			string snapshotArgs;
			switch (snapshotMode)
			{
				case AvdSnapShotMode.NoSnapshot:
					snapshotArgs = "-no-snapshot";
					break;
				case AvdSnapShotMode.NoSnapshotSave:
					snapshotArgs = "-no-snapshot-save -no-snapshot-update-time";
					break;
				case AvdSnapShotMode.NoShapshotLoad:
					snapshotArgs = "-no-snapshot-load";
					break;
				default:
					throw new BuildException(String.Format("Unknown snapshot state '{0}'.", snapshotMode));
			}

			string[] arguments = {
				"-avd " + m_avdName,
				"-memory " + m_imageRAMSizeMiB,
				snapshotArgs,
				verbose ? "-debug-all" : String.Empty,
				"-gpu " + (gpuEmulation ? "auto" : "off"),
				(headlessAVD ? "-no-window" : ""),
				// this option seems to speed up the emulator, especially during deployment, by waking it out of its idle state and running an instruction 
				// counter in the background. however the ARM emulator freezes if a snapshot is load with this option
				((fastInstall && (snapshotMode != AvdSnapShotMode.NoSnapshotSave || m_isX86)) ? "-qemu -icount auto" : String.Empty) // qemu args must be last
			};

			// Start the emulator
			string joinedArguments = String.Join(" ", arguments.Where(s => !String.IsNullOrWhiteSpace(s)));
			Log.Status.WriteLine("[AndroidEmulatorInit] {0} {1}", emulatorPath, joinedArguments);

			ProcessStartInfo processStartInfo = new ProcessStartInfo(emulatorPath, joinedArguments);
			processStartInfo.WorkingDirectory = m_sdkInstalledDir;
			processStartInfo.EnvironmentVariables["ANDROID_HOME"] = m_homeDir;
			processStartInfo.EnvironmentVariables["ANDROID_SDK_HOME"] = m_homeDir;
			processStartInfo.EnvironmentVariables["ANDROID_SDK_ROOT"] = m_sdkInstalledDir;
			processStartInfo.EnvironmentVariables["JAVA_HOME"] = m_jdkDir;
			processStartInfo.UseShellExecute = false;
			processStartInfo.CreateNoWindow = true;
			processStartInfo.WindowStyle = ProcessWindowStyle.Hidden;
			Process emulator_process = Process.Start(processStartInfo);
			
			// wait for device
			Log.Minimal.WriteLine("Waiting for Android emulator to start...");

			try
			{
				AdbTask.Execute(Project, "wait-for-device", 60000, 1); // GRADLE TODO: we might have failed almost immediately trying to launch emulator should check before we sit here for 60 seconds
			}
			catch (Exception e)
			{	
				AdbKillTask.Execute(Project, true);
				
				// This attempts to run the emulator again to get the actual error.  We are assuming it is going to fail again...
				var exitCode = emulator_process.ExitCode;
				if (exitCode != 0)
				{
					Process emulatorFailProcess = new Process();
					emulatorFailProcess.StartInfo = processStartInfo;
					using (ProcessRunner emulatorRunner = new ProcessRunner(emulatorFailProcess))
					{
						emulatorRunner.UseJobObject = ProcessRunner.JobObjectUse.Yes;
						if (emulatorRunner.Start(1000) && emulatorRunner.ExitCode != 0)
						{
							throw new BuildException(string.Format("Emulator exited with an error: {0}", emulatorRunner.StandardOutput));
						}
					}
				}
				
				throw new BuildException("Unable to start emulator: " + e.Message);
			}

			Log.Minimal.WriteLine("Waiting for device boot...");
			WaitForProp("dev.bootcomplete");
			Log.Minimal.WriteLine("Waiting for system boot...");
			WaitForProp("sys.boot_completed");
			
			Log.Minimal.WriteLine("Detected that Android emulator has started and system has booted.");
			
			// set the serial to the new emulator that we just started
			SetTargetTask setTargetTask = new SetTargetTask();
			setTargetTask.Project = Project;
			setTargetTask.Execute();
		}

		private void MakeSDCard(string file, int sizeInBytes)
		{
			string m_sdkInstalledDir = AndroidBuildUtil.NormalizePath(Project.Properties["package.AndroidSDK.appdir"]);
			string mkSDCard = AndroidBuildUtil.NormalizePath(Path.Combine(m_sdkInstalledDir, "tools", "mksdcard.exe"));
			string[] arguments = 
			{
				sizeInBytes.ToString(),
				String.Format("\"{0}\"", file)
			};
			string joinedArgs = String.Join(" ", arguments);

			ExecTask mkSDCardExec = new ExecTask(Project);
			mkSDCardExec.Program = mkSDCard;
			mkSDCardExec.Arguments = joinedArgs;
			mkSDCardExec.Execute();
		}

		private void CreateAvd(string avdCompleteMarkerFile, string skin = "1080x1920", string sdCardPath = null, bool useSnapshot = true, bool rebuildAvd = false, bool gpuEmulation = false)
		{
			List<string> arguments = new List<string>()
			{
				"-v", // verbose
				"create avd", // verb
				String.Format("--name \"{0}\"", m_avdName), //name, snapshot will be created in %ANDROID_SDK_HOME%/.android/avd/<name>
				!String.IsNullOrWhiteSpace(m_deviceType) ? String.Format("--device \"{0}\"", m_deviceType) : String.Empty,
				!String.IsNullOrWhiteSpace(sdCardPath) ? String.Format("--sdcard \"{0}\"", sdCardPath) : String.Empty,
				rebuildAvd ? "--force" : String.Empty
			};

			arguments.Add(String.Format("--package \"system-images;android-{0};{1};{2}\"", m_imageSDKVersion, m_systemImageType, Project.Properties.GetPropertyOrFail("package.AndroidSDK.android_abi")));
			string avdBatchFile = AndroidBuildUtil.NormalizePath(Path.Combine(m_sdkInstalledDir, "tools", "bin", "avdmanager.bat"));	
			
			string joinedArgs = String.Join(" ", arguments.Where(s => !String.IsNullOrWhiteSpace(s)));

			Log.Status.WriteLine("[exec] {0} {1}", avdBatchFile, joinedArgs);

			Process androidBatProcess = new Process();
			androidBatProcess.StartInfo.FileName = avdBatchFile;
			androidBatProcess.StartInfo.Arguments = joinedArgs;
			androidBatProcess.StartInfo.WorkingDirectory = m_sdkInstalledDir;
			androidBatProcess.StartInfo.EnvironmentVariables["ANDROID_HOME"] = m_homeDir;
			androidBatProcess.StartInfo.EnvironmentVariables["ANDROID_SDK_HOME"] = m_homeDir;
			androidBatProcess.StartInfo.EnvironmentVariables["ANDROID_SDK_ROOT"] = m_sdkInstalledDir;
			androidBatProcess.StartInfo.EnvironmentVariables["JAVA_HOME"] = m_jdkDir;
			androidBatProcess.StartInfo.UseShellExecute = false;
			androidBatProcess.StartInfo.CreateNoWindow = false;
			androidBatProcess.StartInfo.RedirectStandardInput = true;
			androidBatProcess.StartInfo.RedirectStandardOutput = true;

			androidBatProcess.Start();
			androidBatProcess.WaitForExit();

			if (androidBatProcess.ExitCode != 0)
			{
				throw new BuildException("Unable to create emulator AVD: \n" + androidBatProcess.StandardOutput.ReadToEnd());
			}

			// update settings file, config.ini
			string iniPath = AndroidBuildUtil.NormalizePath(Path.Combine(m_avdDir, m_avdName + ".avd", "config.ini"));
			SortedDictionary<string, string> iniSettings = ReadIniSettings(iniPath);

			iniSettings["hw.accelerometer"] = "yes";
			iniSettings["hw.audioInput"] = "yes";
			iniSettings["hw.battery"] = "yes";
			iniSettings["hw.camera.back"] = "none";
			iniSettings["hw.cpu.ncore"] = "4";
			iniSettings["hw.dPad"] = "no";
			iniSettings["hw.device.manufacturer"] = "Google";
			iniSettings["hw.device.name"] = m_deviceType;
			iniSettings["hw.gps"] = "yes";
			iniSettings["hw.keyboard"] = "yes";
			iniSettings["hw.lcd.density"] = "240";
			iniSettings["hw.mainKeys"] = "yes";
			iniSettings["hw.ramSize"] = m_imageRAMSizeMiB;
			iniSettings["hw.sensors.orientation"] = "yes";
			iniSettings["hw.sensors.proximity"] = "yes";
			iniSettings["hw.trackBall"] = "no";
			iniSettings["hw.sdCard"] = "yes";
			iniSettings["sdcard.size"] = SDCardSizeMiB.ToString() + "M";
			iniSettings["skin.dynamic"] = "yes";
			iniSettings["disk.dataPartition.size"] = m_internalStorageSizeMiB + "M";
			iniSettings["fastboot.forceColdBoot"] = "no";
			iniSettings["fastboot.forceFastBoot"] = "yes";
			iniSettings["vm.heapSize"] = "128";
			
			if(gpuEmulation)
			{
				iniSettings["hw.gpu.enabled"] = "yes";
				iniSettings["hw.gpu.mode"] = "auto";
			}
			
			WriteAvdIniSettings(iniPath, iniSettings);

			File.WriteAllLines
			(
				avdCompleteMarkerFile,
				new string[] 
				{
					"Creation Time: " + DateTime.Now.ToString(),
					AVDMarkerVersionText + AVDVersion
				}
			);
		}

		private SortedDictionary<string, string> ReadIniSettings(string settingsFile)
		{
			SortedDictionary<string, string> settings = new SortedDictionary<string, string>();
			foreach (string line in File.ReadAllLines(settingsFile))
			{
				string[] tokens = line.Split('=');
				if (tokens.Length == 2)
				{
					settings[tokens[0].Trim()] = tokens[1].Trim();
				}
			}
			return settings;
		}

		private void WriteAvdIniSettings(string settingsFile, SortedDictionary<string, string> settings)
		{
			File.WriteAllLines(settingsFile, from kvp in settings where kvp.Value != null select String.Format("{0}={1}", kvp.Key, kvp.Value));
		}

		private void WaitForProp(string property)
		{
			int timeoutMs = Int32.Parse(Project.ExpandProperties("${android-emulator-timeout??${android-timeout}}"));

			TimeSpan timeout_duration = TimeSpan.FromMilliseconds(timeoutMs);
			Stopwatch stopwatch = Stopwatch.StartNew();

			AdbTask adb = new AdbTask();
			adb.Project = Project;
			adb.Arguments = "shell getprop " + property; // opens the android shell, all following arguments are run in the shell
			adb.Timeout = timeoutMs;
			adb.ReadStdOut = true;

			bool startedEmulatorDetected = false;
			while (stopwatch.Elapsed < timeout_duration && startedEmulatorDetected == false)
			{
				adb.Execute();
				startedEmulatorDetected = (adb.StdOut.Trim() == "1");

				if (startedEmulatorDetected == false)
				{
					Thread.Sleep(1000);
				}
			}

			if (startedEmulatorDetected == false)
			{
				throw new BuildException("Failed to detect started emulator after " + timeoutMs + "ms");
			}

			Log.Status.WriteLine("[debug] Emulator took {0} ms to boot property '{1}'", stopwatch.ElapsedMilliseconds, property);
		}

		private static string ReadEmulatorCheckLine(IEnumerator<string> trimmedOutputLines, string expected = null)
		{
			if (!trimmedOutputLines.MoveNext())
			{
				string expectedValueReporting = expected != null ? " (expected '" + expected + "')" : String.Empty;
				throw new BuildException("Unexpected end of output from emulator-check" + expectedValueReporting + ", could not determine if emulator could run.");
			}
			if (expected != null && expected != trimmedOutputLines.Current)
			{
				throw new BuildException("Unexpected output from emulator-check. Expected '" + expected + "', got '" + trimmedOutputLines.Current + "'.");
			}
			return trimmedOutputLines.Current;
		}
	}
}

