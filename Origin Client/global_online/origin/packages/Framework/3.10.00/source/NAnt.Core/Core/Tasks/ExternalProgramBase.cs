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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Scott Hernandez (ScottHernandez@hotmail.com)
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Xml;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    /// <summary>Provides the abstract base class for tasks that execute external applications.</summary>
    public abstract class ExternalProgramBase : Task
    {

        public ExternalProgramBase(string name) : base(name) { }

        static readonly int DefaultExitCode = -1;

        protected bool _quiet = false;
        protected ArgumentSet _argSet = new ArgumentSet();
        protected StreamWriter _fileWriter = null;
        protected int _exitCode = DefaultExitCode;
        protected string _baseDirectory = "";

        /// <summary>Gets the application to start.</summary>
        public abstract string ProgramFileName
        {
            get;
        }

        /// <summary>Gets the command line arguments for the application.</summary>
        public abstract string ProgramArguments
        {
            get;
        }

        /// <summary>The file to which the standard output will be redirected.</summary>
        public virtual string OutputFile
        {
            get { return null; }
        }

        /// <summary>True if the output file is to be appended to.</summary>
        public virtual bool OutputAppend
        {
            get { return false; }
        }

        /// <summary>Write to standard output. Default is true.</summary>
        public virtual bool Stdout
        {
            get { return true; }
            set { } //so that it can be overriden.
        }

        /// <summary>Redirect standard output. Default is true.</summary>
        public virtual bool RedirectOut
        {
            get { return true; }
            set { } //so that it can be overriden.
        }

        /// <summary>Redirect standard input. Default is true.</summary>
        public virtual bool RedirectIn
        {
            get { return true; }
            set { } //so that it can be overriden.
        }

        /// <summary>Whether to create process in its own window. Default is true.</summary>
        public virtual bool CreateInWindow
        {
            get { return true; }
            set { } //so that it can be overriden.
        }

        /// <summary>If true standard output from the external program will not be written to log.  Default is false.</summary>
        public bool Quiet
        {
            get { return _quiet; }
            set { _quiet = value; }
        }

        /// <summary>Returns the exit code of the process once it has run. Default is -1.</summary>
        public int ExitCode
        {
            get { return _exitCode; }
            set { _exitCode = value; }
        }

        /// <summary>Gets the working directory for the application.</summary>
        public virtual string BaseDirectory
        {
            get
            {
                if (_baseDirectory == "")
                {
                    if (Project != null)
                    {
                        return Project.BaseDirectory;
                    }
                    else
                    {
                        return null;
                    }
                }
                else
                {
                    return _baseDirectory;
                }
            }
            set { _baseDirectory = value; } //so that it can be overriden.
        }

        /// <summary>The maximum amount of time the application is allowed to execute, expressed in milliseconds.  Defaults to no time-out.</summary>
        public virtual int TimeOut
        {
            get { return Int32.MaxValue; }
            set { }
        }

        /// <summary>Get the command line arguments for the application.</summary>
        protected List<string> Args
        {
            get { return _argSet.Arguments; }
        }

        protected override void InitializeTask(XmlNode taskNode)
        {
            // maintian backwards compatability with args not in argset.
            _argSet.Project = Project;
            _argSet.Initialize(taskNode);

            base.InitializeTask(taskNode);
        }

        /// <summary>Get the command line arguments, separated by spaces.</summary>
        public virtual string GetCommandLine()
        {
            // append any nested <arg> arguments to command line
            StringBuilder arguments = new StringBuilder(ProgramArguments);
            foreach (string arg in Args)
            {
                arguments.Append(" ");
                arguments.Append(arg);
            }
            return arguments.ToString();
        }

        /// <summary>
        /// Sets the StartInfo Options and returns a new Process that can be run.
        /// </summary>
        /// <returns>new Process with information about programs to run, etc.</returns>
        protected virtual void PrepareProcess(ref Process process)
        {
            process.StartInfo.FileName = ProgramFileName;
            process.StartInfo.Arguments = GetCommandLine();
            process.StartInfo.WorkingDirectory = BaseDirectory;
        }

        /// <summary>Callback for procrunner stdout</summary>
        public virtual void LogStdOut(OutputEventArgs outputEventArgs)
        {
            if (Stdout == true)
            {
                // log to output
                int originalIndentLevel = Log.IndentLevel;
                Log.IndentLevel = 0;
                if (!Quiet)
                    Log.Status.WriteLine(outputEventArgs.Line);
                Log.IndentLevel = originalIndentLevel;
            }

            if (_fileWriter != null)
            {
                // log to file
                _fileWriter.WriteLine(outputEventArgs.Line);
            }
        }


        /// <summary>Callback for procrunner stderr</summary>
        public virtual void LogStdErr(OutputEventArgs outputEventArgs)
        {
            int originalIndentLevel = Log.IndentLevel;
            Log.IndentLevel = 0;
            Log.Status.WriteLine(outputEventArgs.Line);
            Log.IndentLevel = originalIndentLevel;
        }

        protected virtual void OnProcessStarted(ProcessRunnerEventArgs e)
        {
        }

        struct StatusCode
        {
            public int mStatusCode;
            public string mStatus;

            public StatusCode(int statusCode, string status)
            {
                mStatusCode = statusCode;
                mStatus = status;
            }
        };

        static StatusCode[] mStatusCodes = new StatusCode[] {
            new StatusCode(unchecked((int)0x80000001), "STATUS_GUARD_PAGE_VIOLATION"),
            new StatusCode(unchecked((int)0x80000002), "STATUS_DATATYPE_MISALIGNMENT"),
            new StatusCode(unchecked((int)0x80000003), "STATUS_BREAKPOINT"),
            new StatusCode(unchecked((int)0x80000004), "STATUS_SINGLE_STEP"),
            new StatusCode(unchecked((int)0xC0000005), "STATUS_ACCESS_VIOLATION"),
            new StatusCode(unchecked((int)0xC0000006), "STATUS_IN_PAGE_ERROR"),
            new StatusCode(unchecked((int)0xC0000008), "STATUS_INVALID_HANDLE"),
            new StatusCode(unchecked((int)0xC000000D), "STATUS_INVALID_PARAMETER"),
            new StatusCode(unchecked((int)0xC0000017), "STATUS_NO_MEMORY"),
            new StatusCode(unchecked((int)0xC000001D), "STATUS_ILLEGAL_INSTRUCTION"),
            new StatusCode(unchecked((int)0xC0000025), "STATUS_NONCONTINUABLE_EXCEPTION"),
            new StatusCode(unchecked((int)0xC0000026), "STATUS_INVALID_DISPOSITION"),
            new StatusCode(unchecked((int)0xC000008C), "STATUS_ARRAY_BOUNDS_EXCEEDED"),
            new StatusCode(unchecked((int)0xC000008D), "STATUS_FLOAT_DENORMAL_OPERAND"),
            new StatusCode(unchecked((int)0xC000008E), "STATUS_FLOAT_DIVIDE_BY_ZERO"),
            new StatusCode(unchecked((int)0xC000008F), "STATUS_FLOAT_INEXACT_RESULT"),
            new StatusCode(unchecked((int)0xC0000090), "STATUS_FLOAT_INVALID_OPERATION"),
            new StatusCode(unchecked((int)0xC0000091), "STATUS_FLOAT_OVERFLOW"),
            new StatusCode(unchecked((int)0xC0000092), "STATUS_FLOAT_STACK_CHECK"),
            new StatusCode(unchecked((int)0xC0000093), "STATUS_FLOAT_UNDERFLOW"),
            new StatusCode(unchecked((int)0xC0000094), "STATUS_INTEGER_DIVIDE_BY_ZERO"),
            new StatusCode(unchecked((int)0xC0000095), "STATUS_INTEGER_OVERFLOW"),
            new StatusCode(unchecked((int)0xC0000096), "STATUS_PRIVILEGED_INSTRUCTION"),
            new StatusCode(unchecked((int)0xC00000FD), "STATUS_STACK_OVERFLOW"),
            new StatusCode(unchecked((int)0xC0000135), "STATUS_DLL_NOT_FOUND"),
            new StatusCode(unchecked((int)0xC0000138), "STATUS_ORDINAL_NOT_FOUND"),
            new StatusCode(unchecked((int)0xC0000139), "STATUS_ENTRYPOINT_NOT_FOUND"),
            new StatusCode(unchecked((int)0xC000013A), "STATUS_CONTROL_C_EXIT"),
            new StatusCode(unchecked((int)0xC0000142), "STATUS_DLL_INIT_FAILED"),
            new StatusCode(unchecked((int)0xC0000194), "STATUS_POSSIBLE_DEADLOCK"),
            new StatusCode(unchecked((int)0xC00002B4), "STATUS_FLOAT_MULTIPLE_FAULTS"),
            new StatusCode(unchecked((int)0xC00002B5), "STATUS_FLOAT_MULTIPLE_TRAPS"),
            new StatusCode(unchecked((int)0xC00002C9), "STATUS_REG_NAT_CONSUMPTION"),
            new StatusCode(unchecked((int)0xC0000409), "STATUS_STACK_BUFFER_OVERRUN"),
            new StatusCode(unchecked((int)0xC0000417), "STATUS_INVALID_CRUNTIME_PARAMETER"),
            new StatusCode(unchecked((int)0xC0000420), "STATUS_ASSERTION_FAILURE")
        };
#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return ProgramFileName; }
		}
#endif

        protected override void ExecuteTask()
        {
            // create the file writer for logging if needed
            if (OutputFile != null && RedirectOut)
            {
                try
                {
                    _fileWriter = new StreamWriter(OutputFile, OutputAppend);
                }
                catch (Exception e)
                {
                    Log.Status.WriteLine(LogPrefix + "Output not redirected because output file '{0}' cannot be opened.\n{1}", OutputFile, e.Message);
                    _fileWriter = null;
                }
            }

            // launch the process to run the exec
            Process process = null;
            ProcessRunner procRunner = null;

            try
            {
                process = new Process();
                
                PrepareProcess(ref process);

                using (procRunner = new ProcessRunner(process, RedirectOut, RedirectOut, RedirectIn, !CreateInWindow))
                {

                    // handle output events
                    if (RedirectOut)
                    {
                        procRunner.StdOutputEvent += new OuputEventHandler(LogStdOut);
                        procRunner.StdErrorEvent += new OuputEventHandler(LogStdErr);
                    }
                    procRunner.ProcessStartedEvent += new ProcessEventHandler(OnProcessStarted);

                    bool retCode = procRunner.Start(TimeOut);

                    if (retCode == false)
                    {
                        // the process timed out
                        string msg = String.Format("Process timed out after {0} milliseconds.", TimeOut);
                        throw new Exception(msg);
                    }

                    // save the exit code
                    try
                    {
                        ExitCode = process.ExitCode;
                    }
                    catch
                    {
                        ExitCode = DefaultExitCode;
                    }

                    // Keep the FailOnError check to prevent programs that return non-zero even if they are not returning errors.
                    if (FailOnError && process != null && process.ExitCode != 0)
                    {
                        string exitMessage = null;

                        for (int i = 0; i < mStatusCodes.Length; ++i)
                        {
                            if (mStatusCodes[i].mStatusCode == ExitCode)
                            {
                                exitMessage = String.Format("External program ({0}) returned {1} (exit code={2} / 0x{2:x}).", ProgramFileName, mStatusCodes[i].mStatus, ExitCode);
                            }
                        }

                        if (exitMessage == null)
                        {
                            exitMessage = String.Format("External program ({0}) returned errors (exit code={1}).", ProgramFileName, ExitCode);
                        }

                        throw new BuildException(exitMessage, Location, BuildException.StackTraceType.XmlOnly);
                    }

                }
            }
            catch (Exception e)
            {
                string msg = String.Format("Error running external program ({0})", ProgramFileName);
                if (process != null)
                {
                    msg = String.Format("Error running external program: ({0}){1}          [{0} {2}]{1}          Working Directory: [{3}]{1}", process.StartInfo.FileName, Environment.NewLine, process.StartInfo.Arguments, process.StartInfo.WorkingDirectory);
                }
                throw new BuildException(msg, Location, e);
            }
            finally
            {
                if (_fileWriter != null)
                {
                    _fileWriter.Close();
                }
            }
        }
    }
}
