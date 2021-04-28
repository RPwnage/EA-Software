// NAnt - A .NET build tool
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.IO;
using System.Text;
using System.Threading;
using System.Collections;
using System.Diagnostics;

namespace NAnt.Core.Util
{
/*
    /// <summary>
    /// Maintains a list of all process launched using the Process Runner.
    /// Dispose will kill all processes and their children at the very end of
    /// the NAnt process.
    /// </summary>
    class ProcessWatcher : IDisposable
    {
        // This effectively makes ProcessWatcher a singleton object.
        public static readonly ProcessWatcher Instance = new ProcessWatcher();
        ArrayList _proccessIdsUnSafe = null;
        ArrayList _proccessIds = null;

        private ProcessWatcher()
        {
            _proccessIdsUnSafe = new ArrayList();
            _proccessIds = ArrayList.Synchronized(_proccessIdsUnSafe);
        }

        public void Dispose()
        {
            // Kill all child processes that are known to be running.  Note
            // that NAnt only knows about its direct children, it does not
            // have any info about grand-children.  SafeKill takes care of
            // that though.
            try
            {
                lock (_proccessIds.SyncRoot)
                {
                    foreach (int procId in _proccessIds)
                        ProcessRunner.SafeKill(procId);
                    _proccessIds.Clear();
                }
            }
            catch (System.Exception ) { }
        }

        /// <summary>
        /// Callback for all process launched using the proc runner.
        /// Cache a set of process ids and kill any that remain when we dispose.
        /// </summary>
        public void OnProcessStarted(ProcessRunnerEventArgs e)
        {
            _proccessIds.Add(e.Process.Id);
        }

        public void OnProcessStoped(ProcessRunnerEventArgs e)
        {
            _proccessIds.Remove(e.Process.Id);
        }

    }
*/

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
    ///		// when process runner leaves scope prcoess and child procs will be destroyed
    /// </example>
    public class ProcessRunner : IDisposable
    {
        public event ProcessEventHandler ProcessStartedEvent;
        public event ProcessEventHandler ProcessTimedOutEvent;
        public event ProcessEventHandler ProcessShutdownEvent;
        public event OuputEventHandler StdOutputEvent;
        public event OuputEventHandler StdErrorEvent;

        Process _process;
        StringBuilder _standardOutput = new StringBuilder();
        StringBuilder _standardError = new StringBuilder();

        public ProcessRunner(Process process)
        {
            _process = process;
            //required to capture output
            _process.StartInfo.RedirectStandardOutput = true;
            _process.StartInfo.RedirectStandardError = true;
            _process.StartInfo.RedirectStandardInput = true;
            //required to allow redirects
            _process.StartInfo.UseShellExecute = false;
            //required to hide the console window if launched from GUI
            _process.StartInfo.CreateNoWindow = true;
/*
            ProcessStartedEvent += new ProcessEventHandler(ProcessWatcher.Instance.OnProcessStarted);
            ProcessTimedOutEvent += new ProcessEventHandler(ProcessWatcher.Instance.OnProcessStoped);
            ProcessShutdownEvent += new ProcessEventHandler(ProcessWatcher.Instance.OnProcessStoped);
 */
        }

        public ProcessRunner(Process process, bool redirectOutput, bool redirectError, bool redirectIn)
        {
            _process = process;
            //required to capture output
            _process.StartInfo.RedirectStandardOutput = redirectOutput;
            _process.StartInfo.RedirectStandardError = redirectError;
            _process.StartInfo.RedirectStandardInput = redirectIn;
            //required to allow redirects
            _process.StartInfo.UseShellExecute = false;
            _process.StartInfo.CreateNoWindow = true;

/*
            ProcessStartedEvent += new ProcessEventHandler(ProcessWatcher.Instance.OnProcessStarted);
            ProcessTimedOutEvent += new ProcessEventHandler(ProcessWatcher.Instance.OnProcessStoped);
            ProcessShutdownEvent += new ProcessEventHandler(ProcessWatcher.Instance.OnProcessStoped);
 */
        }

        public ProcessRunner(Process process, bool redirectOutput, bool redirectError, bool redirectIn, bool createNoWindow)
            : this(process, redirectOutput, redirectError, redirectIn)
        {
            _process.StartInfo.CreateNoWindow = createNoWindow;
        }

        public bool Start()
        {
            return Start(Int32.MaxValue);
        }

        public bool Start(int timeOut)
        {
            // start the process (output gets captured by threads - created below)
            _process.Start();

            // send start event
            OnProcessStartedEvent(new ProcessRunnerEventArgs(_process));

            // create two threads to capture stdout and stderr (we need to use
            // threads because either stdout or stderr could block forcing a deadlock condition)
            Thread stdOutThread = null;
            Thread stdErrThread = null;

            if (_process.StartInfo.RedirectStandardOutput)
            {
                stdOutThread = new Thread(new ThreadStart(CaptureStandardOutput));
                stdOutThread.Priority = ThreadPriority.BelowNormal;
                ThreadRunner.Instance.ThreadStart(stdOutThread);
            }

            if (_process.StartInfo.RedirectStandardError)
            {
                stdErrThread = new Thread(new ThreadStart(CaptureStandardError));
                stdErrThread.Priority = ThreadPriority.BelowNormal;
                ThreadRunner.Instance.ThreadStart(stdErrThread);
            }

            // wait for the process to timeout
            bool retCode = _process.WaitForExit(timeOut);

            if (retCode == false)
            {
                // send time out event
                OnProcessTimedOutEvent(new ProcessRunnerEventArgs(_process));

                // cleanup the process tree
                SafeKill(_process.Id);
            }
            else
            {
                // wait for the threads to finish and process to terminate
                if (stdOutThread != null) stdOutThread.Join();
                if (stdErrThread != null) stdErrThread.Join();

                // send shutdown event if process was not abrubtly terminated
                OnProcessShutdownEvent(new ProcessRunnerEventArgs(_process));
            }

            // dispose of the threads
            if (stdOutThread != null) ThreadRunner.Instance.ThreadDispose(stdOutThread);
            if (stdErrThread != null) ThreadRunner.Instance.ThreadDispose(stdErrThread);

            return retCode;
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
            get { return _process.ExitCode; }
        }

        public Process Process
        {
            get { return _process; }
        }



        public static void SafeKill(Process process)
        {
            try
            {
                if (process != null && process.HasExited == false)
                {
                    ProcessUtil.KillProcessTree(process.Id);
                }
            }
            catch (System.Exception /*e*/) { }
        }

        public static void SafeKill(int processId)
        {
            // If process exited by the time GetProcessById() is called 
            // GetProcessById() function will throw an exception
            try
            {
                SafeKill(Process.GetProcessById(processId));
            }
            catch (System.Exception /*e*/) { }
        }

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
            CaptureOutput(_process.StandardOutput, _standardOutput, StdOutputEvent);
        }

        private void CaptureStandardError()
        {
            CaptureOutput(_process.StandardError, _standardError, StdErrorEvent);
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
                SafeKill(_process);

                if (disposing)
                {
                    _process.Dispose();
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;

    }
}
