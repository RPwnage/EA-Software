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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
// Scott Hernandez (ScottHernandez@hotmail.com)
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Xml;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks
{

	/// <summary>Provides the abstract base class for tasks that execute external applications.</summary>
	public abstract class ExternalProgramBase : Task
	{
		static readonly int DefaultExitCode = -1;

		protected ArgumentSet _argSet = new ArgumentSet();
		protected StreamWriter _fileWriter = null;
		protected string _baseDirectory = "";

		/// <summary>Gets the application to start.</summary>
		public abstract string ProgramFileName { get; }

		/// <summary>Gets the command line arguments for the application.</summary>
		public abstract string ProgramArguments { get; }

		/// <summary>The file to which the standard output will be redirected.</summary>
		public virtual string OutputFile { get; }

		/// <summary>True if the output file is to be appended to.</summary>
		public virtual bool OutputAppend { get; }

		/// <summary>Write to standard output. Default is true.</summary>
		public virtual bool Stdout { get; set; } = true;

		/// <summary>Redirect standard output. Default is true.</summary>
		public virtual bool RedirectOut { get; set; } = true;

		/// <summary>Redirect standard input. Default is true.</summary>
		public virtual bool RedirectIn { get; set; } = true;

		/// <summary>Whether to create process in its own window. Default is true.</summary>
		public virtual bool CreateInWindow { get; set; } = true;

		/// <summary>If true standard output from the external program will not be written to log.  Default is false.</summary>
		public bool Quiet { get; set; }

        /// <summary>If true standard error from the external program will not be written to log.  Default is false.</summary>
        public bool QuietError { get; set; }

		/// <summary>Returns the exit code of the process once it has run. Default is -1.</summary>
		public int ExitCode { get; set; } = DefaultExitCode;

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
			set { _baseDirectory = value; } //so that it can be overridden.
		}

		/// <summary>The maximum amount of time the application is allowed to execute, expressed in milliseconds.  Defaults to no time-out.</summary>
		public virtual int TimeOut { get; set; } = Int32.MaxValue;

		/// <summary>Get the command line arguments for the application.</summary>
		protected List<string> Args { get { return _argSet.Arguments; } }

		public ExternalProgramBase(string name) : base(name) { }

		protected override void InitializeTask(XmlNode taskNode)
		{
			// maintain backwards compatibility with args not in argset.
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
					Log.Minimal.WriteLine(outputEventArgs.Line);
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
            if (!QuietError)
                Log.Minimal.WriteLine(outputEventArgs.Line);
			Log.IndentLevel = originalIndentLevel;
		}

		protected virtual void OnProcessStarted(ProcessRunnerEventArgs e)
		{
		}

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
						throw new BuildException(ProcessUtil.DecodeProcessExitCode(ProgramFileName, ExitCode), Location, stackTrace: BuildException.StackTraceType.XmlOnly);
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
