// (c) Electronic Arts. All Rights Reserved.

using System.Collections.Generic;
using System.Diagnostics;

using NAnt.Core;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{
	/// <summary>The common behavior shared between the kettle and capilano runner tasks</summary>
	public class ConsoleExecRunner
	{
		private Project Project;
		private Log Log;
		private object mLogLock = new object();

		public string Program { get; set; } = "";
		public string Arguments { get; set; } = "";
		public int TimeOut { get; set; } = int.MaxValue;
		public List<string> LogLines { get; private set; }

		public int ExitCode { get; set; } = -1;

		public ConsoleExecRunner(Project project)
		{
			Project = project;
			Log = Project.Log;
		}

		private void DataReceivedHandler(object sendingProcess, DataReceivedEventArgs outputData)
		{
			if (outputData.Data != null)
			{
				lock (mLogLock)
				{
					LogLines.Add(outputData.Data);
				}
				Log.Minimal.WriteLine(outputData.Data);
			}
		}
		
		public void Run()
		{
			ProcessStartInfo startInfo = new ProcessStartInfo(Program, Arguments);
			startInfo.CreateNoWindow = true;
			startInfo.UseShellExecute = false;
			startInfo.RedirectStandardOutput = true;
			startInfo.RedirectStandardError = true;

			LogLines = new List<string>();

			Log.Status.WriteLine("Executing \"{0}\" {1}", Program, Arguments);

			using (Process p = new Process())
			{
				p.StartInfo = startInfo;

				p.ErrorDataReceived += new DataReceivedEventHandler(DataReceivedHandler);
				p.OutputDataReceived += new DataReceivedEventHandler(DataReceivedHandler);

				p.Start();
				p.BeginOutputReadLine();
				p.BeginErrorReadLine();

				if (!p.WaitForExit(TimeOut))
				{
					Log.Status.WriteLine("Timeout value of {0}ms exceeded, killing process.", TimeOut);
					p.Kill();
				}

				ExitCode = p.ExitCode;
			}
		}
	}
}
