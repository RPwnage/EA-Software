// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.AndroidConfig
{
	/// <summary>Tries to set the target</summary>
	[TaskName("SetTarget")]
	public class SetTargetTask : Task
	{
		public SetTargetTask()
			: base()
		{
		}
				
		protected override void ExecuteTask()
		{
			try
			{
				Log.Minimal.WriteLine("Attempting to query ADB for all Android devices...");
				
				int timeoutMs = Int32.Parse(Project.ExpandProperties("${android-timeout}"));
				string deviceOutput = AdbTask.Execute(Project, "devices", timeoutMs, retries: 0, failonerror: false, readstdout: true).StdOut;
				if (string.IsNullOrEmpty(deviceOutput))
				{
					throw new BuildException("No devices found.");
				}

				string[] lines = deviceOutput.Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
				var deviceSerials = new List<string>();
				foreach (string line in lines)
				{
					if (!line.Contains("List of devices"))
					{
						string[] lineInfo = line.Split(new string[] { "\t", " " }, StringSplitOptions.RemoveEmptyEntries);
						deviceSerials.Add(lineInfo[0]);
					}
				}

				if (!deviceSerials.Any())
				{
					Log.Minimal.WriteLine("Found no devices.");
				}
				else
				{
					Log.Status.WriteLine("Found devices: " + String.Join(", ", deviceSerials) + ".");

					string selectedAbi = Project.GetPropertyOrFail("package.AndroidSDK.android_abi");
					int androidSdkVersion = Int32.Parse(Project.GetPropertyOrFail("package.AndroidSDK.minimumSDKVersion"));

					foreach (string deviceSerial in deviceSerials)
					{
						try
						{
							Log.Minimal.WriteLine("Getting SDK version from device '{0}'...", deviceSerial);
							int hwSdkVersion = Int32.Parse(GetDeviceProperty(deviceSerial, timeoutMs, "ro.build.version.sdk"));

							Log.Status.WriteLine("Device '{0}' has SDK version '{1}'.", deviceSerial, androidSdkVersion);
							if (hwSdkVersion >= androidSdkVersion)
							{
								Log.Minimal.WriteLine(String.Format("Getting ABI list from device {0}...", deviceSerial));
								string deviceAbis = GetDeviceProperty(deviceSerial, timeoutMs, "ro.product.cpu.abilist").Trim();
								if (String.IsNullOrEmpty(deviceAbis))
								{
									// some devices only have one abi so abilist doesn't return any values
									deviceAbis = GetDeviceProperty(deviceSerial, timeoutMs, "ro.product.cpu.abi").Trim();
								}

								if (deviceAbis.Contains(selectedAbi))
								{
									Log.Status.WriteLine("Device '{0}' has ABIs '{1}'.", deviceSerial, deviceAbis);
									Log.Minimal.WriteLine(String.Format("Setting internal-android-use-target to '{0}' and continuing...", deviceSerial));
									Project.Properties["internal-android-use-target"] = deviceSerial.Contains("emulator") ? "emulator" : "device";
									Project.Properties["internal-android-device-serial"] = deviceSerial;
									Project.Properties["internal-android-detected-abis"] = deviceAbis;
									return;
								}
								else
								{
									Log.Minimal.WriteLine("The selected abi '{0}' wasn't supported by the device '{1}''s abilist: {2}.", selectedAbi, deviceSerial, deviceAbis);
								}
							}
							else
							{
								Log.Minimal.WriteLine("Device '{0}' nas SDK version '{1}' tjat is older than minmum SDK version {2}!", deviceSerial, hwSdkVersion, androidSdkVersion);
							}
						}
						catch
						{
						}
					}
				}
			}
			catch(Exception e)
			{
				Log.Status.WriteLine("Unable to connect to Device: " + e.Message);
			}

			Log.Status.WriteLine("Setting internal-android-use-target to 'emulator' and continuing.");
			Project.Properties["internal-android-use-target"] = "emulator";
			Project.Properties["internal-android-device-serial"] = "";
		}

		private string GetDeviceProperty(string deviceSerial, int timeoutMs, string property)
		{
			string deviceOutput = AdbTask.Execute(Project, String.Format("-s {0} shell getprop {1}", deviceSerial, property), timeoutMs, retries: 0, failonerror: false, readstdout: true).StdOut;
			if (String.IsNullOrEmpty(deviceOutput))
			{
				throw new BuildException("Failed to get property '" + property + "'");
			}
			return deviceOutput;
		}
	}
}
