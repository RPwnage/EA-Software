// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2002 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// File Mainatiners
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.IO;
using System.Text;
using System.Diagnostics;

namespace NAnt.Core.Util
{
	public class OutputEventArgs : EventArgs
	{
		public string Line;

		public OutputEventArgs(string line)
		{
			Line = line;
		}
	}

	public class ProcessRunnerEventArgs : EventArgs
	{
		public Process Process;

		public ProcessRunnerEventArgs(Process process)
		{
			Process = process;
		}
	}

	public delegate void OuputEventHandler(OutputEventArgs e);
	public delegate void ProcessEventHandler(ProcessRunnerEventArgs e);

	/// <remarks>
	/// Launches a process and maintains two threads for reading from the processes stdout and stderr.
	/// Uses the process watcher instance to maintain a list of process ids. Useful for cleaning up processes
	/// when terminating.
	/// When an instance of a process runner goes out of scope the process will be automatically destroyed along
	/// with any child processes it launched.
	/// </remarks>
	/// <example>
	///		Process p = new Process;  // prepare a process
	///		ProcessRunner run = new ProcessRunner(p); // attach process to process runner
	///
	///		run.ProcessEvent += new EventHandler; // handle process events
	///		run.StdOutputEvent += new EventHandler; // handle output events
	///
	///		try
	///		{
	///			timeout = run.Start()
	///
	///			if ( timeout )
	///				// report any errors
	///		}
	///		catch( e )
	///			// report any errors
	///
	///		// when process runner leaves scope process and child processes will be destroyed
	/// </example>
	public class ProcessRunner : IDisposable
	{
		public event ProcessEventHandler ProcessStartedEvent;
		public event ProcessEventHandler ProcessTimedOutEvent;
		public event ProcessEventHandler ProcessShutdownEvent;
		public event OuputEventHandler StdOutputEvent;
		public event OuputEventHandler StdErrorEvent;

		ProcessUtil.IProcessGroup _processGroup;
		StringBuilder _standardOutput = new StringBuilder();
		StringBuilder _standardError = new StringBuilder();

		public enum JobObjectUse
		{
			Yes,
			No,
			Default
		};

		public JobObjectUse UseJobObject = JobObjectUse.Default;

		public ProcessRunner(Process process)
		{
			Process = process;
			//required to capture output
			Process.StartInfo.RedirectStandardOutput = true;
			Process.StartInfo.RedirectStandardError = true;
			Process.StartInfo.RedirectStandardInput = true;
			//required to allow redirects
			Process.StartInfo.UseShellExecute = false;
			//required to hide the console window if launched from GUI
			Process.StartInfo.CreateNoWindow = true;
			m_createStdThreads = true;
		}

		public ProcessRunner(Process process, bool redirectOutput, bool redirectError, bool redirectIn)
		{
			Process = process;
			//required to capture output
			Process.StartInfo.RedirectStandardOutput = redirectOutput;
			Process.StartInfo.RedirectStandardError = redirectError;
			Process.StartInfo.RedirectStandardInput = redirectIn;
			//required to allow redirects
			Process.StartInfo.UseShellExecute = false;
			Process.StartInfo.CreateNoWindow = true;
			m_createStdThreads = true;
		}

		public ProcessRunner(Process process, bool redirectOutput, bool redirectError, bool redirectIn, bool createNoWindow, bool createStdThreads = true)
			: this(process, redirectOutput, redirectError, redirectIn)
		{
			Process.StartInfo.CreateNoWindow = createNoWindow;
			m_createStdThreads = createStdThreads;
		}

		public bool Start()
		{
			return Start(Int32.MaxValue);
		}

		public bool Start(int timeout)
		{
			StartAsync();
			return WaitForExit(timeout);
		}

		// create two threads to capture stdout and stderr (we need to use
		// threads because either stdout or stderr could block forcing a deadlock condition)
		System.Threading.Tasks.Task m_stdoutThread;
		System.Threading.Tasks.Task m_stderrThread;
		bool m_createStdThreads;
		bool m_started;

		public void StartAsync()
		{
			if (m_started)
			{
				throw new System.InvalidOperationException("Process has already been started.");
			}

			// start the process (output gets captured by threads - created below)
			try
			{
				if (UseJobObject == JobObjectUse.Default)
				{
					_processGroup = ProcessUtil.CreateProcessGroup(Process);
				}
				else
				{
					_processGroup = ProcessUtil.CreateProcessGroup(Process, UseJobObject == JobObjectUse.Yes);
				}
				_processGroup.Start();
			}
			catch (Exception e)
			{
				// Under unix environment, if file doesn't have exe bit set, we usually get permission denied message.
				// But unfortunately, we get "cannot find specified file" which is confusing. So we explicitly test for
				// this scenario and give better message.
				bool exeBitSet = true;
				try
				{
					if ((PlatformUtil.IsUnix || PlatformUtil.IsOSX) && System.IO.File.Exists(Process.StartInfo.FileName))
					{
						exeBitSet = ProcessUtil.IsUnixExeBitSet(Process.StartInfo.FileName, Process.StartInfo.WorkingDirectory);
					}
				}
				catch (Exception e2)
				{
					throw new Exception(e.Message + Environment.NewLine + e2.Message);
				}
				if (!exeBitSet)
				{
					throw new Exception(String.Format("ERROR: The executable '{0}' appears to be lacking execute attribute.  Please make sure that the execute bit is set!", Process.StartInfo.FileName));
				}
				else
				{
					// Seems to be some other exception.  Just re-throw the error.
					throw e;
				}
			}

			// send start event
			OnProcessStartedEvent(new ProcessRunnerEventArgs(Process));

			if (m_createStdThreads)
			{
				try
				{
					if (Process.StartInfo.RedirectStandardOutput)
					{
						m_stdoutThread = new System.Threading.Tasks.Task(CaptureStandardOutput, System.Threading.Tasks.TaskCreationOptions.LongRunning);
						m_stdoutThread.Start();
					}

					if (Process.StartInfo.RedirectStandardError)
					{
						m_stderrThread = new System.Threading.Tasks.Task(CaptureStandardError, System.Threading.Tasks.TaskCreationOptions.LongRunning);
						m_stderrThread.Start();
					}
				}
				catch (Exception)
				{
				}
			}

			m_started = true;
		}

		public bool WaitForExit()
		{
			return WaitForExit(Int32.MaxValue);
		}

		public bool WaitForExit(int timeoutMs)
		{
			bool hasExited;

			if (!m_started)
			{
				throw new System.InvalidOperationException("Cannot wait for a process to exit that has not yet been started.");
			}

			try
			{
				// wait for the process to timeout
				hasExited = Process.WaitForExit(timeoutMs);

				if (!hasExited)
				{
					// send time out event
					OnProcessTimedOutEvent(new ProcessRunnerEventArgs(Process));

					// cleanup the process tree
					_processGroup.Kill();
				}
				else
				{
					// cleanup the process tree (this only really affect job object process groups), process has cleanly exited here 
					// but we want to wipe out subprocesses
					_processGroup.Kill();

					// wait for the threads to finish and process to terminate
					if (m_stdoutThread != null) m_stdoutThread.Wait();
					if (m_stderrThread != null) m_stderrThread.Wait();

					// send shutdown event if process was not abruptly terminated
					OnProcessShutdownEvent(new ProcessRunnerEventArgs(Process));
				}
			}
			finally
			{
				try
				{
					if (m_stdoutThread != null)
					{
						m_stdoutThread.Dispose();
						m_stdoutThread = null;
					}

					if (m_stderrThread != null)
					{
						m_stderrThread.Dispose();
						m_stderrThread = null;
					}
				}
				catch(Exception)
				{
				}
			}

			m_started = false;

			return hasExited;
		}

		public string GetOutput()
		{
			if (_standardOutput.Length > 0)
				_standardOutput.AppendLine();
			_standardOutput.Append(_standardError.ToString());
			if (_standardError.Length > 0)
				_standardError.AppendLine();

			return _standardOutput.ToString();
		}

		public string StandardOutput
		{
			get { return _standardOutput.ToString(); }
		}

		public string StandardError
		{
			get { return _standardError.ToString(); }
		}

		public int ExitCode
		{
			get { return Process.ExitCode; }
		}

		public Process Process { get; }

		private void OnOutputEvent(OuputEventHandler outputLineEvent, string line)
		{
			if (outputLineEvent != null)
			{
				OutputEventArgs outputEventArgs = new OutputEventArgs(line);
				outputLineEvent(outputEventArgs);
			}
		}

		private void OnProcessStartedEvent(ProcessRunnerEventArgs e)
		{
			if (ProcessStartedEvent != null)
			{
				ProcessStartedEvent(e);
			}
		}

		private void OnProcessTimedOutEvent(ProcessRunnerEventArgs e)
		{
			if (ProcessTimedOutEvent != null)
			{
				ProcessTimedOutEvent(e);
			}
		}

		private void OnProcessShutdownEvent(ProcessRunnerEventArgs e)
		{
			if (ProcessShutdownEvent != null)
			{
				ProcessShutdownEvent(e);
			}
		}

		private void CaptureOutput(StreamReader reader, StringBuilder output, OuputEventHandler outputLineEvent)
		{
			while (true)
			{
				string line = reader.ReadLine();
				if (line == null)
				{
					return;
				}
				OnOutputEvent(outputLineEvent, line);
				output.AppendFormat("{0}\n", line);
			}
		}

		private void CaptureStandardOutput()
		{
			CaptureOutput(Process.StandardOutput, _standardOutput, StdOutputEvent);
		}

		private void CaptureStandardError()
		{
			CaptureOutput(Process.StandardError, _standardError, StdErrorEvent);
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		void Dispose(bool disposing)
		{
			if (!this._disposed)
			{
				// destroy the process
				if (_processGroup != null)
				{
					_processGroup.Kill();
				}

				if (disposing)
				{
					Process.Dispose();
				}
			}
			_disposed = true;
		}
		private bool _disposed = false;

	}
}
