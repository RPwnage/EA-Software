// (c) Electronic Arts. All Rights Reserved.

using System;
using System.Diagnostics;
using System.Text.RegularExpressions;
using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

namespace EA.Eaconfig
{
	[TaskName("xbapp-deploy")]
	public class XbAppDeploy : Task
	{
		[TaskAttribute("timeout")]
		public string Timeout { get; set; }

		[TaskAttribute("console")]
		public string Console { get; set; }

		[TaskAttribute("layoutdir")]
		public string ModuleLayoutDir { get; set; }

		/// <summary>Do extra validation of the manifest before deploying. Does not block deployment but displays validation errors.</summary>
		[TaskAttribute("validatemanifest")]
		public bool ValidateManifest
		{
			get { return _validateManifest; }
			set { _validateManifest = value; }
		}
		private bool _validateManifest = true;

		[TaskAttribute("usenetworkshare")]
		public bool UseNetworkShare { get; set; }

		protected override void ExecuteTask()
		{
			ConsoleExecRunner xbappRunner = new ConsoleExecRunner(Project);
            xbappRunner.TimeOut = int.Parse(Timeout);

			string consoleString = string.IsNullOrEmpty(Console) ? "" : ("/X:" + Console);
			string validateString = ValidateManifest ? "/VM" : "";

            xbappRunner.Program = PathNormalizer.Normalize(Project.ExpandProperties(@"${eaconfig-run-bindir}\xbapp.exe"));
			if(UseNetworkShare)
            {
                if(Process.GetProcessesByName("xrfssvc").Length == 0)
                {
                    using (Process p = new Process())
                    {
                        p.StartInfo = new ProcessStartInfo(PathNormalizer.Normalize(Project.ExpandProperties(@"${eaconfig-run-bindir}\xrfssvc.exe")));
                        p.StartInfo.CreateNoWindow = true;
                        p.StartInfo.UseShellExecute = false;
                        p.Start();
                    }
                }   

                xbappRunner.Arguments += "registernetworkshare";
            }	
			else
            {
                xbappRunner.Arguments += "deploy";
            }
            xbappRunner.Arguments += " /s " + validateString + " /v " + consoleString + " " + ModuleLayoutDir;

            xbappRunner.Run();
			if (xbappRunner.ExitCode != 0)
			{
				if (xbappRunner.ExitCode == 1)
				{
					throw new BuildException("Unable to deploy application. xbapp returned error code 1 - invalid command line. Full xbapp log:\n" + String.Join("\n", xbappRunner.LogLines));
				}
				else
				{
					throw new BuildException("Unable to deploy application. Full xbapp log:\n" + String.Join("\n", xbappRunner.LogLines));
				}
			}

			string xapp_output = string.Join("\n", xbappRunner.LogLines);

			Regex rx = new Regex(@"Aumids returned:\s*(?:\d+>)?\s*(.*)\b", RegexOptions.Compiled | RegexOptions.IgnoreCase);
			Regex rxpack = new Regex(@"Package Full Name:\s*(?:\d+>)?\s*(.*)\b", RegexOptions.Compiled | RegexOptions.IgnoreCase);
			var matches = rx.Matches(xapp_output);
			var matchespack = rxpack.Matches(xapp_output);

			foreach (Match match in matches)
			{
				string matched_text = match.Groups[1].Value;
				if (!String.IsNullOrEmpty(matched_text))
				{
					Project.Properties["xbapp-assigned-name"] = matched_text;
					break;
				}
			}

			foreach (Match match in matchespack)
			{
				string matched_text = match.Groups[1].Value;
				if (!String.IsNullOrEmpty(matched_text))
				{
					Project.Properties["xbapp-package-name"] = matched_text;
					break;
				}
			}
		}
	}

	[TaskName("xbapp-terminate")]
	public class XbAppTerminate : Task
	{
		[TaskAttribute("timeout")]
		public string Timeout { get; set; }

		[TaskAttribute("console")]
		public string Console { get; set; }

		[TaskAttribute("package")]
		public string Package { get; set; }

		protected override void ExecuteTask()
		{
			ConsoleExecRunner runner = new ConsoleExecRunner(Project);
			runner.TimeOut = int.Parse(Timeout);

			string consoleString = string.IsNullOrEmpty(Console) ? "" : ("/X:" + Console);

			runner.Program = PathNormalizer.Normalize(Project.ExpandProperties(@"${eaconfig-run-bindir}\xbapp.exe"));
			runner.Arguments = "terminate " + consoleString + " " + Package;

			bool succeeded = false;
			bool couldNotTransition = false;

			for (int i = 0; i < 2; ++i)
			{
				runner.Run();
				if (runner.ExitCode == 0)
				{
					succeeded = true;
					break;
				}

				foreach (string logLine in runner.LogLines)
				{
					// If the PLM process could not make the transition to the terminated state, retry. 
					// If the retry also fails, this should be acceptable.
					if (logLine.Contains("The package is not in a valid state to make the requested transition."))
					{
						couldNotTransition = true;
						break;
					}
				}
			}

			if (!succeeded)
			{
				// If the terminate did not succeed for reasons other than the 
				// inability to transition PLM states, throw an exception.
				if (!couldNotTransition)
				{
					throw new BuildException("Unable to terminate application.");
				}
			}
		}
	}

	[TaskName("xbapp-uninstall")]
	public class XbAppUninstall : Task
	{
		[TaskAttribute("timeout")]
		public string Timeout { get; set; }

		[TaskAttribute("console")]
		public string Console { get; set; }

		[TaskAttribute("package")]
		public string Package { get; set; }

		protected override void ExecuteTask()
		{
			ConsoleExecRunner runner = new ConsoleExecRunner(Project);
			runner.TimeOut = int.Parse(Timeout);

			string consoleString = string.IsNullOrEmpty(Console) ? "" : ("/X:" + Console);

			runner.Program = PathNormalizer.Normalize(Project.ExpandProperties(@"${eaconfig-run-bindir}\xbapp.exe"));
			runner.Arguments = "uninstall " + consoleString + " " + Package;

			bool succeeded = false;
			bool done = false;

			for (int i = 0; i < 10 && !done; ++i)
			{
				runner.Run();
				if (runner.ExitCode == 0)
				{
					succeeded = true;
					break;
				}

				foreach (string logLine in runner.LogLines)
				{
					if (logLine.Contains("E0109 - The package could not be accessed because it is in use by the system.  Please retry the operation or reboot the console."))
					{
						goto retry;
					}
					else if (logLine.Contains("E0100 - The package is not registered on the console"))
					{
						succeeded = true;
						done = true;
						break;
					}
				}
				retry:
				;
			}

			if (!succeeded)
			{
				throw new BuildException("Unable to uninstall application.");
			}
		}
	}
}
