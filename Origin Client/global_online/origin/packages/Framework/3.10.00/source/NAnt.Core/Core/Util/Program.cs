using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Diagnostics;

using NAnt.Core;
using NAnt.Core.Logging;

namespace NAnt.Core.Util
{
    public class Program : IDisposable
    {
        public const uint RedirectOutput = 1;
        public const uint RedirectError = 2;
        public const uint RedirectIn = 4;
        public const uint CreateInWindow = 8;
        public const uint OutputAppend = 16;
        public const uint StdOut = 32;
        public const uint StdErr = 64;
        public const uint Quiet = 128;

        public const uint DefaultFlags = (RedirectOutput | RedirectError | StdOut | StdErr);

        public const int DefaultExitCode = -1;


        public static int Execute(Project project, PathString executable, IEnumerable<string> options = null, PathString workingdir = null, IDictionary<string, string> env = null, uint flags = DefaultFlags, PathString outputfile = null, int timeout = Int32.MaxValue)
        {
            int exitcode = DefaultExitCode;

            try
            {
                using (Program exec = new Program(project.Log, executable, options, workingdir??PathString.MakeNormalized(project.BaseDirectory), env??project.Env(EnvironmentTypes.exec), flags, outputfile, timeout))
                {
                    exitcode = exec.Run();
                }
            }
            catch (Exception e)
            {
                string msg = String.Format("Error running external program ({0}). Details:{1}{2}", executable, Environment.NewLine, e.StackTrace);
                throw new BuildException(msg, e);
            }
            return exitcode;
        }

        public static int Execute(Log log, PathString executable, IEnumerable<string> options = null, PathString workingdir = null, IDictionary<string, string> env = null, uint flags = DefaultFlags, PathString outputfile = null, int timeout = Int32.MaxValue)
        {
            int exitcode = DefaultExitCode;

            try
            {
                using (Program exec = new Program(log, executable, options, workingdir, env, flags, outputfile, timeout))
                {
                    exitcode = exec.Run();
                }
            }
            catch (Exception e)
            {
                string msg = String.Format("Error running external program ({0}). Details:{1}{2}", executable, Environment.NewLine, e.StackTrace);
                throw new BuildException(msg, e);
            }
            return exitcode;
        }


        private Program(Log log, PathString executable, IEnumerable<string> options, PathString workingdir, IDictionary<string, string> env, uint flags, PathString outputfile, int timeout)
        {
            _log = log;
            _executable = executable;
            _options = options;
            _outputfile = outputfile;
            _flags = new BitMask(flags);
            _timeout = timeout;
            _workingdir = workingdir;
            _env = env;

            // create the file writer for logging if needed
            if (_outputfile != null && _flags.IsKindOf(RedirectOutput))
            {
                try
                {
                    _outputWriter = new StreamWriter(_outputfile.Path, _flags.IsKindOf(OutputAppend));
                }
                catch (Exception e)
                {
                    if (_log != null)
                    {
                        _log.Warning.WriteLine("Output not redirected because output file '{0}' cannot be opened.\n{1}", _outputfile, e.Message);
                    }
                    _outputWriter = null;
                }
            }
        }

        private int Run()
        {
            int exitcode = DefaultExitCode;

            // launch the process to run the exec
            using (Process process = new Process())
            {
                PrepareProcess(process);

                using (ProcessRunner procRunner = new ProcessRunner(process, _flags.IsKindOf(RedirectOutput), _flags.IsKindOf(RedirectError), _flags.IsKindOf(RedirectIn), !_flags.IsKindOf(CreateInWindow)))
                {
                    // handle output events
                    if (_flags.IsKindOf(RedirectOutput))
                    {
                        procRunner.StdOutputEvent += new OuputEventHandler(LogStdOut);
                        procRunner.StdErrorEvent += new OuputEventHandler(LogStdErr);
                    }
                    procRunner.ProcessStartedEvent += new ProcessEventHandler(OnProcessStarted);

                    if (!(_log.Info is NullLogWriter))
                    {
                        _log.Info.WriteLine("Executing external program:{0}{1} {2}{0}\tworking directory: {3}{0}\tenvironment: {4}", Environment.NewLine, _executable, process.StartInfo.Arguments, process.StartInfo.WorkingDirectory, process.StartInfo.EnvironmentVariables.Cast<System.Collections.DictionaryEntry>().ToString("; ", de => de.Key + "=" + de.Value));
                    }

                    if (!procRunner.Start(_timeout))
                    {
                        // the process timed out
                        string msg = String.Format("Process timed out after {0} milliseconds.", _timeout);
                        throw new Exception(msg);
                    }

                    // save the exit code
                    try
                    {
                        exitcode = process.ExitCode;
                    }
                    catch
                    {
                        exitcode = DefaultExitCode;
                    }
                }
            }
            return exitcode;
        }


        protected virtual void PrepareProcess(Process process)
        {
            process.StartInfo.FileName = _executable.Path;
            process.StartInfo.Arguments = _options.ToString(" ");
            if (_workingdir != null)
            {
                process.StartInfo.WorkingDirectory = _workingdir.Path;
            }
            if (_env != null)
            {
                ProcessUtil.SetEnvironmentVariables(_env, process.StartInfo, true, _log);
            }
        }

        /// <summary>Callback for procrunner stdout</summary>
        protected virtual void LogStdOut(OutputEventArgs outputEventArgs)
        {
            if (_flags.IsKindOf(StdOut) && !_flags.IsKindOf(Quiet))
            {
                if (_log != null)
                {
                    _log.Status.WriteLine(outputEventArgs.Line);
                }
            }

            if (_outputWriter != null)
            {
                // log to file
                _outputWriter.WriteLine(outputEventArgs.Line);
            }
        }

        /// <summary>Callback for procrunner stderr</summary>
        protected virtual void LogStdErr(OutputEventArgs outputEventArgs)
        {
            if (_flags.IsKindOf(StdErr))
            {
            if (_log != null)
            {
                _log.Error.WriteLine(outputEventArgs.Line);
            }
            else
            {
                Console.WriteLine(outputEventArgs.Line);
            }
            }
        }

        protected virtual void OnProcessStarted(ProcessRunnerEventArgs e)
        {
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
                if (disposing)
                {
                    if (_outputWriter != null)
                    {
                        _outputWriter.Dispose();
                    }
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;

        private readonly Log _log;
        private readonly PathString _executable;
        private readonly IEnumerable<string> _options;
        private readonly PathString _workingdir;
        private readonly IDictionary<string, string> _env;
        private readonly int _timeout;
        private readonly PathString _outputfile;
        private readonly BitMask _flags;
        private StreamWriter _outputWriter;
    }
}
