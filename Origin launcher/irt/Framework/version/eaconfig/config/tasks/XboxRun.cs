// (c) Electronic Arts Inc.

using System.Text;
using System.Threading;
using KitCommunicator;
using KitCommunicator.Xbox;
using KitCommunicator.XDK;
using KitCommunicator.GDK;
using NAnt.Core;
using NAnt.Core.Attributes;
using System;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.IO;

namespace EA.EAConfig
{
    
    [TaskName("XboxRun")]
    public class XboxRun : Task
    {
        [TaskAttribute("console")]
        public string ConsoleName { get; set; }

        [TaskAttribute("absolutetimeout")]
        public string AbsoluteTimeout { get; set; }

        [TaskAttribute("args")]
        public string Args { get; set; }

        [TaskAttribute("usegdk")]
        public bool UseGDK { get; set; }

        [TaskAttribute("buildlocation")]
        public string BuildLocation { get; set; }

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
            KitCommunicator.Common.DebugLoggingAction = (message) => { Log.Debug.WriteLine(message); };
            KitCommunicator.Common.InfoLoggingAction = (message) => { Log.Minimal.WriteLine(message); };
            KitCommunicator.Common.WarnLoggingAction = (message) => { Log.Warning.WriteLine(message); };
            KitCommunicator.Common.ErrorLoggingAction = (message) => { Log.Error.WriteLine(message); };
            KitCommunicator.Common.FatalLoggingAction = (message) => { Log.Error.WriteLine(message); };

            ManualResetEvent processExited = new ManualResetEvent(false);
            ManualResetEvent processReturnCodeObtained = new ManualResetEvent(false);

            XboxCommunicator communicator;
            if (UseGDK)
            {
                communicator = new GDKCommunicator(ConsoleName);
            }
            else
            {
                communicator = new XDKCommunicator(ConsoleName);
            }

            try
            {
                communicator.Connect();
            }
            catch(KitCommunicatorException)
            {
                communicator.RebootAndWaitForAvailability();
                communicator.Connect();
            }

            XboxPackage deployedPackage = communicator.Deploy(BuildLocation);
            string exitKey = "EXIT(";

            StringBuilder outputText = new StringBuilder();
            List<string> errorMessages = new List<string>();
            StringBuilder logTracker = new StringBuilder();
            int returnCode = -1;
            bool recordingTestRun = false;
            bool printMessages = true;
			communicator.OnOutput += (sender, e) =>
            {
                if (printMessages)
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
                        else if (e.Message.StartsWith(exitKey))
                        { // because of how xbox tools work, it's possible to miss very fast runs (eg. Engine.Base unit test). If we come across this output, it means the test is over.
                            printMessages = false;
                            int endIndex = e.Message.IndexOf(")");
                            string returnCodeString = e.Message.Substring(exitKey.Length, endIndex - exitKey.Length);
                            returnCode = int.Parse(returnCodeString);
                            processReturnCodeObtained.Set();
                            processExited.Set();
                        }
                        if (recordingTestRun)
                        {
                            logTracker.AppendLine(e.Message);
                        }
                    }
                }
            };

            communicator.OnProcessExited += (sender, e) =>
            {
                processExited.Set();
            };

            try
            {                
                int timeoutMS = -1;
                if (!string.IsNullOrWhiteSpace(AbsoluteTimeout))
                {
                    timeoutMS = int.Parse(AbsoluteTimeout);
                }
                communicator.Launch(deployedPackage.Aumid, Args);

                bool receivedExit = processExited.WaitOne(timeoutMS);

                if (receivedExit == false)
                {
                    Log.Error.WriteLine($"Timed out after waiting for {timeoutMS} ms. Terminating process.");
                    communicator.KillActiveTitleProcess();
                    throw new BuildException($"Timed out after waiting for {timeoutMS} ms. Process terminated.");
                }
                bool returnCodeObtained = processReturnCodeObtained.WaitOne(TimeSpan.FromSeconds(3));

                if (!returnCodeObtained)
				{
                    throw new BuildException("Unable to determine return code from process.");
				}
                else
				{
                    if (returnCode != 0)
					{
                        throw new BuildException($"Test failed with return code {returnCode}.");
					}
				}
            }
            finally
            {
                if (IsRunningTests)
                {
                    CommonRun.PrintErrorMessages(Log, errorMessages);
                }
                if (!OutputFile.IsNullOrEmpty())
                {
                    File.WriteAllText(OutputFile, outputText.ToString());
                }
                communicator.Uninstall(deployedPackage.PackageName);
            }
        }
	}
}
