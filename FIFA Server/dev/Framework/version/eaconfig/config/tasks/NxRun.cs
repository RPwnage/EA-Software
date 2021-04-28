// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using KitCommunicator.Switch;
using KitCommunicator.Common;
using EA.EAConfig;

namespace EA.nxConfig
{
	[TaskName("nxRun")]
	public class Run : Task
	{
		[TaskAttribute("console")]
		public string ConsoleName { get; set;  }

		[TaskAttribute("runtimeout")]
		public string RunTimeout { get; set; }

		[TaskAttribute("workingdir")]
		public string WorkingDir { get; set; }

		[TaskAttribute("programfolder")]
		public string ProgramFolder { get; set; }

		[TaskAttribute("program")]
		public string Program { get; set; }

		[TaskAttribute("args")]
		public string Args { get; set; }

		[TaskAttribute("isrunningtests")]
		public bool IsRunningTests { get; set; }

		bool HasFailedToConnect()
		{
			if (outputText.ToString().Contains("Failed to connect"))
			{
				return true;
			}
			return false;
		}

		private StringBuilder outputText = new StringBuilder();
		private List<string> errorMessages = new List<string>();
		private int returnCode = 0;

		protected override void ExecuteTask()
		{
			ManualResetEvent mre = new ManualResetEvent(false);
			ManualResetEvent textStreamEnded = new ManualResetEvent(false);

			SwitchCommunicator communicator = new SwitchCommunicator(ConsoleName);

			Logging.DebugLoggingAction = (message) => { Log.Debug.WriteLine(message); };
			Logging.InfoLoggingAction = (message) => { Log.Minimal.WriteLine(message); };
			Logging.WarnLoggingAction = (message) => { Log.Warning.WriteLine(message); };
			Logging.ErrorLoggingAction = (message) => { Log.Error.WriteLine(message); };
			Logging.FatalLoggingAction = (message) => { Log.Error.WriteLine(message); };

			communicator.Connect(force: true);

			int? switchRunExitCode = -1;

			communicator.OnProcessExited += (sender, e) => {
				switchRunExitCode = e.ExitCode;
				mre.Set();
			};

			StringBuilder logTracker = new StringBuilder();
			bool recordingTestRun = false;
			bool printMessages = true;
			communicator.OnOutput += (sender, e) =>
			{
				if (!printMessages) return;
				
				Log.Minimal.WriteLine(e.Message);

				outputText.AppendLine(e.Message);

				if (e.Message.Contains("Failed to connect"))
				{
					textStreamEnded.Set();
				}

				if (IsRunningTests)
				{// when running tests we want to capture some output so that we can display a list of errors at the bottom
					if (e.Message.StartsWith("[ RUN"))
					{
						recordingTestRun = true;
					}
					else if (e.Message.StartsWith("[       OK ]"))
					{
						recordingTestRun = false;
						logTracker.Clear();
					}
					else if (e.Message.StartsWith("[  FAILED  ]"))
					{
						if (recordingTestRun)
						{
							logTracker.AppendLine(e.Message);
							string error = logTracker.ToString();
							errorMessages.Add(error);
							logTracker.Clear();
						}
						recordingTestRun = false;
					}
					else if (e.Message.Contains("RETURNCODE="))
					{
						printMessages = false;
						returnCode = int.Parse(e.Message.Substring(e.Message.IndexOf("=") + 1));
						textStreamEnded.Set();
					}

					if (recordingTestRun)
					{
						logTracker.AppendLine(e.Message);
					}
				}
			};

			communicator.PowerOn();

			try
			{
				communicator.Launch(Program, Args, WorkingDir);

				int timeoutMS = -1;

				if (!string.IsNullOrWhiteSpace(RunTimeout))
					timeoutMS = int.Parse(RunTimeout);

				bool receivedEvent = false;
				//if timeout is negative we wont wait for this to return and just detach.
				if (timeoutMS > 0)
				{
					receivedEvent = mre.WaitOne(timeoutMS);

					textStreamEnded.WaitOne(TimeSpan.FromSeconds(5));

					if (!receivedEvent)
					{
						Log.Status.WriteLine($"Timed out waiting for {timeoutMS} ms");
					}
					else
					{
						if (switchRunExitCode.HasValue == false)
						{
							throw new BuildException($"Test failed with NULL return code.");
						}
						else if (switchRunExitCode != 0)
						{
							throw new BuildException($"Test failed with return code {switchRunExitCode}.");
						}
						else if (returnCode != 0)
						{
							throw new BuildException($"Test failed with return code {returnCode}.");
						}
					}
				}

				if (HasFailedToConnect())
				{
					Log.Warning.WriteLine("Detected that the process has failed to connect to the devkit. Forcing a reboot.");
					communicator.Reboot();
					communicator.Launch(Program, Args, WorkingDir);
				}
			}
			finally
			{
				if (IsRunningTests)
				{
					CommonRun.PrintErrorMessages(Log, errorMessages);
				}
			}
		}
	}
}
