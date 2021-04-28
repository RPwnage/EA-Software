// (c) Electronic Arts. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using EA.EAConfig;
using KitCommunicator.Common;
using KitCommunicator.Playstation.PS4;
using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{
	[TaskName("KettleRun")]
	public class KettleRun : Task
	{
		[TaskAttribute("console")]
		public string ConsoleName { get; set; }

		[TaskAttribute("runtimeout")]
		public string RunTimeout { get; set; }

		[TaskAttribute("activitytimeout")]
		public string ActivityTimeout { get; set; }

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

		/// <summary>
		/// A file where all output will be saved to.
		/// Needed so that it can be parsed and uploaded to frosting reporter.
		/// </summary>
		[TaskAttribute("outputfile")]
		public string OutputFile { get; set; }

		protected override void ExecuteTask()
		{
			ManualResetEvent mre = new ManualResetEvent(false);
			ManualResetEvent textStreamEnded = new ManualResetEvent(false);

			PS4Communicator Communicator = new PS4Communicator(ConsoleName);

			Logging.DebugLoggingAction = (message) => { Log.Debug.WriteLine(message); };
			Logging.InfoLoggingAction = (message) => { Log.Minimal.WriteLine(message); };
			Logging.WarnLoggingAction = (message) => { Log.Warning.WriteLine(message); };
			Logging.ErrorLoggingAction = (message) => { Log.Error.WriteLine(message); };
			Logging.FatalLoggingAction = (message) => { Log.Error.WriteLine(message); };

			uint orbisRunPid = 0;
			int? orbisRunExitCode = -1;
			Communicator.OnProcessExited += (sender, e) => {
				orbisRunPid = e.PID;
				orbisRunExitCode = e.ExitCode;
				mre.Set();
			};

			StringBuilder outputText = new StringBuilder();
			List<string> errorMessages = new List<string>();
			StringBuilder logTracker = new StringBuilder();
			bool recordingTestRun = false;
			Communicator.OnOutput += (sender, e) =>
			{
				Log.Minimal.WriteLine(e.Message);

				outputText.AppendLine(e.Message);

				if (IsRunningTests)
				{
					// when running tests we want to capture some output so that we can display a list of errors at the bottom
					// note: this output format is specific to gtest output
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
					else if (e.Message.StartsWith("RETURNCODE="))
					{
						textStreamEnded.Set();
					}

					if (recordingTestRun)
					{
						logTracker.AppendLine(e.Message);
					}
				}
			};

			List<(string, string)> settingsToReverse = new List<(string, string)>();

			try
			{
				Communicator.Disconnect(force: true);
				Communicator.Connect();

				Communicator.SetFileServingDirectory(Project.GetPropertyOrDefault("kettleconfig-run-workingdir", WorkingDir));

				Dictionary<string, int> settingsToSet = new Dictionary<string, int>();
				settingsToSet.Add("Core Dump Mode", 1);
				settingsToSet.Add("Dump Level", 0);
				settingsToSet.Add("Report System Software Errors Automatically", 1);
				settingsToSet.Add("Skip error screen when triggering a core dump by the application", 1);
				settingsToSet.Add("Skip Warning Screen after Improper Shutdown", 1);
				settingsToSet.Add("Enable System Crash Reporting", 0);
				settingsToReverse = SetSettings(Communicator, settingsToSet);

				uint processID = Communicator.Launch(Program, Args, WorkingDir, debugMode: false);

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
						if (orbisRunExitCode.HasValue == false)
						{
							throw new BuildException($"Test failed with NULL return code.");
						}
						else if (orbisRunExitCode != 0)
						{
							throw new BuildException($"Test failed with return code {orbisRunExitCode}.");
						}
					}
				}
			}
			finally
			{
				if (IsRunningTests)
				{
					CommonRun.PrintErrorMessages(Log, errorMessages);
				}
				// reverse settings that differed from what we needed
				if (settingsToReverse.Any())
				{
					Communicator.SetSettings(settingsToReverse);
				}
				if (!OutputFile.IsNullOrEmpty())
				{
					File.WriteAllText(OutputFile, outputText.ToString());
				}

				Communicator.Disconnect();
			}
		}


		/// <summary>
		/// Sets the given settings (if different on the kit). Returns a list of settings that were changed with their original values so that they can be reset at the end.
		/// </summary>
		/// <param name="communicator"></param>
		/// <param name="settingsToSet"></param>
		/// <returns></returns>
		private List<(string, string)> SetSettings(PS4Communicator communicator, Dictionary<string, int> settingsToSet)
		{
			List<(string, string)> settingsToReverse = new List<(string, string)>();
			List<(string, string)> newSettings = new List<(string, string)>();

			List<Tuple<string, string>> originalSettings = communicator.GetSettings(settingsToSet.Keys.ToList());

			foreach (var originalSetting in originalSettings)
			{
				string settingName = originalSetting.Item1;
				int originalValue = Convert.ToInt32(originalSetting.Item2);

				int newValue = settingsToSet[settingName];

				if (originalValue != newValue)
				{
					settingsToReverse.Add((settingName, originalValue.ToString()));
					newSettings.Add((settingName, newValue.ToString()));
				}
			}
			if (newSettings.Any())
			{
				communicator.SetSettings(newSettings);
			}
			return settingsToReverse;
		}
	}
}
