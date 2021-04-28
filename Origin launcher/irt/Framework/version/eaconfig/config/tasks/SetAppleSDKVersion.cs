// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{
	[TaskName("SetAppleSDKVersion")]
	public class SetAppleSDKVersion : Task
	{
		[TaskAttribute("VersionPropertyName", Required = true)]
		public string VersionPropertyName
		{
			get { return _versionpropertyname; }
			set { _versionpropertyname = value; }
		}
		private string _versionpropertyname;

		/// <summary>
		/// Either iphone or mac, to indicate which SDK we are trying to detect
		/// </summary>
		[TaskAttribute("SdkName", Required = true)]
		public string SdkName
		{
			get { return _sdkName; }
			set { _sdkName = value; }
		}
		private string _sdkName;

		private string GetVersionFromOutput(string output)
		{
			string[] lines = output.Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);

			Regex regex = null;
			if (SdkName == "iphone")
			{
				regex = new Regex(@"iphoneos([0-9]+\.[0-9]+)");
			}
			else if (SdkName == "mac")
			{
				regex = new Regex(@"macosx([0-9]+\.[0-9]+)");
			}
			else
			{
				throw new Exception("ERROR: SetAppleSDKVersion, invalid SdkName, must indicate either that you want to detect an install of mac or iphone sdk.");
			}

			string sdkVersion = null;
			foreach (string line in lines)
			{
				Match match = regex.Match(line);
				if (match != null && match.Groups.Count >= 2)
				{
					sdkVersion = match.Groups[1].Value;
					break;
				}
			}

			return sdkVersion;
		}

		protected override void ExecuteTask()
		{
			if (PlatformUtil.IsWindows)
			{
				Project.Properties[VersionPropertyName] = "";
				return;
			}

			System.Diagnostics.ProcessStartInfo startInfo = new System.Diagnostics.ProcessStartInfo();
			startInfo.CreateNoWindow = true;
			startInfo.RedirectStandardOutput = true;
			startInfo.UseShellExecute = false;

			string xcodeBuildPath = null;
			if (Project.Properties["config-system"] == "iphone")
			{
				xcodeBuildPath = Path.Combine(Project.GetPropertyOrDefault("package.iphonesdk.toolbindir", Project.Properties["package.iphonesdk.xcrundir"]), "xcodebuild");
			}
			else if (Project.Properties["config-system"] == "osx")
			{
				xcodeBuildPath = Path.Combine(Project.Properties["package.osx_config.toolbindir"], "xcodebuild");
			}
			else
			{
				throw new Exception("ERROR: Unabled to build path to xcodebuild, SetAppleSDKVersion must be run with an osx or iphone config.");
			}

			startInfo.FileName = xcodeBuildPath;
			startInfo.Arguments = "-showsdks";
			startInfo.WorkingDirectory = Project.Properties["package.dir"];

			System.Diagnostics.Process runProc = new System.Diagnostics.Process();
			runProc.StartInfo = startInfo;
			runProc.Start();

			string outputText = runProc.StandardOutput.ReadToEnd();
			string sdkVersion = GetVersionFromOutput(outputText);

			if (!String.IsNullOrEmpty(sdkVersion))
			{
				Project.Properties.Add(VersionPropertyName, sdkVersion);
			}
			else
			{
				throw new Exception(string.Format("ERROR: Unable to determine installed {0} SDK version!", SdkName));
			}
		}
	}
}
