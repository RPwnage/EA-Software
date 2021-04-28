// (c) Electronic Arts Inc.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using KitCommunicator.PS5;
using NAnt.Core;
using NAnt.Core.Attributes;

namespace EA.EAConfig
{
    [TaskName("Ps5Run")]
    public class Ps5Run : Task
    {
        [TaskAttribute("console")]
        public string ConsoleName { get; set; }

        [TaskAttribute("absolutetimeout")]
        public string AbsoluteTimeout { get; set; }

        [TaskAttribute("workingdir")]
        public string WorkingDir { get; set; }

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

            PS5Communicator communicator = new PS5Communicator(ConsoleName);

            KitCommunicator.Common.DebugLoggingAction = (message) => { Log.Debug.WriteLine(message); };
            KitCommunicator.Common.InfoLoggingAction = (message) => { Log.Minimal.WriteLine(message); };
            KitCommunicator.Common.WarnLoggingAction = (message) => { Log.Warning.WriteLine(message); };
            KitCommunicator.Common.ErrorLoggingAction = (message) => { Log.Error.WriteLine(message); };
            KitCommunicator.Common.FatalLoggingAction = (message) => { Log.Error.WriteLine(message); };

            uint prosperoRunPid = 0;
            int? prosperoRunExitCode = -1;

            communicator.OnProcessExited += (sender, e) => {
                prosperoRunPid = e.PID;
                prosperoRunExitCode = e.ExitCode;
                mre.Set();
            };

            StringBuilder outputText = new StringBuilder();
            List<string> errorMessages = new List<string>();
            StringBuilder logTracker = new StringBuilder();
            bool recordingTestRun = false;
            communicator.OnOutput += (sender, e) =>
            {
                Log.Minimal.WriteLine(e.Message);

                outputText.AppendLine(e.Message);

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
                communicator.Connect();
                
                Dictionary<string, string> settingsToSet = new Dictionary<string, string>();
                settingsToSet.Add("Core Dump Mode", "1");
                settingsToSet.Add("Dump Level", "0");
                settingsToSet.Add("Report System Software Errors Automatically", "1");
                settingsToSet.Add("Skip error screen when triggering a core dump by the application", "1");
                settingsToSet.Add("Skip Warning Screen after Improper Shutdown", "1");
                settingsToSet.Add("Enable System Crash Reporting", "0");
                settingsToReverse = SetSettings(communicator, settingsToSet);

                uint processID = communicator.Launch(Program, Args, WorkingDir, debugMode: false);

                int timeoutMS = -1;
                
                if(!string.IsNullOrWhiteSpace(AbsoluteTimeout))
                    timeoutMS = int.Parse(AbsoluteTimeout);

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
                        if (prosperoRunExitCode.HasValue == false)
                        {
                            throw new BuildException($"Test failed with NULL return code.");
                        }
                        else if (prosperoRunExitCode != 0)
                        {
                            throw new BuildException($"Test failed with return code {prosperoRunExitCode}.");
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
                    communicator.SetSettings(settingsToReverse);
                }
                if (!OutputFile.IsNullOrEmpty())
                {
                    File.WriteAllText(OutputFile, outputText.ToString());
                }

                communicator.Disconnect();
            }
        }

        /// <summary>
        /// Sets the given settings (if different on the kit). Returns a list of settings that were changed with their original values so that they can be reset at the end.
        /// </summary>
        /// <param name="communicator"></param>
        /// <param name="settingsToSet"></param>
        /// <returns></returns>
        private List<(string, string)> SetSettings(PS5Communicator communicator, Dictionary<string, string> settingsToSet)
		{
            List<(string, string)> settingsToReverse = new List<(string, string)>();
            List<(string, string)> newSettings = new List<(string, string)>();

            List<Tuple<string, string>> originalSettings = communicator.GetSettings(settingsToSet.Keys.ToList());

            foreach(var originalSetting in originalSettings)
			{
                string settingName = originalSetting.Item1;
                string originalValue = originalSetting.Item2;

                string newValue = settingsToSet[settingName];

                if (originalValue != newValue)
                {
                    settingsToReverse.Add((settingName, originalValue));
                    newSettings.Add((settingName, newValue));
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
