// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{
	[TaskName("SetXcodeVersion")]
	public class SetXcodeVersion : Task
	{
		[TaskAttribute("VersionPropertyName", Required = true)]
		public string VersionPropertyName
		{
			get { return _versionpropertyname; }
			set { _versionpropertyname = value; }
		}
		private string _versionpropertyname;

		private static string _XcodeVersion = null;

		protected override void ExecuteTask()
		{
			if (PlatformUtil.IsWindows)
			{
				// Assuming we are using at least Xcode 10 when we do testing on pc host machine.
				Project.Properties[VersionPropertyName] = "10";
				return;
			}

			// The property being used is package.xcode.version which could already be set if people set it as global property.
			if (Project.Properties.Contains(VersionPropertyName))
			{
				string currValue = Project.Properties[VersionPropertyName];
				if (!String.IsNullOrEmpty(currValue))
				{
					_XcodeVersion = currValue;
					return;
				}
			}

			if (string.IsNullOrEmpty(_XcodeVersion))
			{
				System.Diagnostics.ProcessStartInfo startInfo = new System.Diagnostics.ProcessStartInfo();
				startInfo.CreateNoWindow = true;
				startInfo.RedirectStandardOutput = true;
				startInfo.UseShellExecute = false;

				string xcodeBuildDir = Project.GetPropertyOrDefault("package.xcode.toolbindir", null);
				if (String.IsNullOrEmpty(xcodeBuildDir))
				{
					// Old workflow properties.
					if (Project.Properties["config-system"] == "iphone")
					{
						xcodeBuildDir = Project.GetPropertyOrDefault("package.iphonesdk.toolbindir", Project.Properties["package.iphonesdk.xcrundir"]);
					}
					else if (Project.Properties["config-system"] == "osx")
					{
						xcodeBuildDir = Project.Properties["package.osx_config.toolbindir"];
					}
				}
				if (String.IsNullOrEmpty(xcodeBuildDir))
				{
					throw new Exception("ERROR: Unabled to build path to xcodebuild, SetXcodeVersion must be run with an osx or iphone config.");
				}
				string xcodeBuildPath = Path.Combine(xcodeBuildDir,"xcodebuild");
				if (!File.Exists(xcodeBuildPath))
				{
					throw new Exception(string.Format("ERROR: Path to xcodebuild ({0}) does not exists, SetXcodeVersion must be run with an osx or iphone config.", xcodeBuildPath));
				}

				startInfo.FileName = xcodeBuildPath;
				startInfo.Arguments = "-version";
				startInfo.WorkingDirectory = Project.Properties["package.dir"];

				int numTries = 2;
				bool gotVerInfo = false;
				while (numTries > 0 && !gotVerInfo)
				{
					--numTries;
					try
					{
						System.Diagnostics.Process runProc = new System.Diagnostics.Process();
						runProc.StartInfo = startInfo;
						runProc.Start();
						string outMessage = runProc.StandardOutput.ReadToEnd();
						int xcodeIdx = outMessage.LastIndexOf("Xcode ");
						if (xcodeIdx != -1)
						{
							string[] tmp = outMessage.Substring(xcodeIdx + 6).Split();
							if (tmp.Length >= 1)
							{
								_XcodeVersion = tmp[0];
								gotVerInfo = true;
							}
						}
					}
					catch
					{
						gotVerInfo = false;
					}
				}
			}

			if (!String.IsNullOrEmpty(_XcodeVersion))
			{
				Project.Properties.Add(VersionPropertyName, _XcodeVersion);
			}
			else
			{
				throw new Exception("ERROR: Unable to determine Xcode version!");
			}
			
		}
	}
}
