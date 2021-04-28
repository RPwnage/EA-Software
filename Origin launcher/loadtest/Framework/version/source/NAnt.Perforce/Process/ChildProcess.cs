// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

using NAnt.Core.Util;

namespace NAnt.Perforce.Processes
{
	public abstract class ChildProcess : IDisposable
	{
		private class SafeProcessProperty<T>
		{
			private Func<Process, T> _PropertyQuery;
			private T _Value;

			internal SafeProcessProperty(T initialValue, Func<Process, T> propertyQuery)
			{
				_Value = initialValue;
				_PropertyQuery = propertyQuery;
			}

			internal T Get(Process process)
			{
				try
				{
					_Value = _PropertyQuery(process);
				}
				catch (InvalidOperationException) { }
				catch (SystemException) { }
				return _Value;
			}

			internal void Set(T newValue)
			{
				_Value = newValue;
			}
		}

		public class ProcessOutputEventArgs : EventArgs
		{
			public string Data { get; private set; }

			internal ProcessOutputEventArgs(string data)
			{
				Data = data;
			}
		}

		public delegate void ProcessOutputRecievedHandler(object sender, ProcessOutputEventArgs e);

		public const string UNKNOWN_PROCESS_NAME = "Unknown process (secured or exited)";
		public const int INVALID_PID = 0;
		public const int DEFAULT_EXIT_CODE = 1;

		protected Process _Process;

		private SafeProcessProperty<int> _CachedPID = new SafeProcessProperty<int>(INVALID_PID, p => p.Id);
		private SafeProcessProperty<int> _CachedExit = new SafeProcessProperty<int>(DEFAULT_EXIT_CODE, p => p.ExitCode);
		private SafeProcessProperty<string> _CachedName = new SafeProcessProperty<string>(UNKNOWN_PROCESS_NAME, p => p.ProcessName);
		private Task _StdOutTask = null;
		private Task _StdErrTask = null;
		private bool _Disposed;

		public int PID
		{
			get { return _CachedPID.Get(_Process); }
			private set { _CachedPID.Set(value); }
		}

		public int ExitCode
		{
			get { return _CachedExit.Get(_Process); }
			private set { _CachedExit.Set(value); }
		}

		public string Name
		{
			get { return _CachedName.Get(_Process); }
			private set { _CachedName.Set(value); }
		}

		public StringDictionary Environment { get { return _Process.StartInfo.EnvironmentVariables; } }
		public string Arguments { get { return _Process.StartInfo.Arguments; } }
		public string Command { get { return _Process.StartInfo.FileName; } }
		public string WorkingDir { get { return _Process.StartInfo.WorkingDirectory; } }

		public bool AllowChildren = false;

		static ChildProcess()
		{
			ProcessWatcher.EnsureInitialized();
		}

		protected ChildProcess(Process process)
		{
			_Process = process;

			// try and cache these as soon as we are created
			PID = PID;
			Name = Name;
		}

		protected abstract ChildProcess PlatformGetParent();

		public ChildProcess Parent()
		{
			ChildProcess parent = ProcessWatcher.GetParent(this);
			if (parent == null)
			{
				return PlatformGetParent();
			}
			return parent;
		}

		public void KillDescendants()
		{
			// clean up all descendants we can accurately detect, loop until we're can't detect any
			ChildProcess[] descendants = Descendants();
			while (descendants.Length > 0)
			{
				foreach (ChildProcess descendant in descendants)
				{
					descendant.Kill();
				}
				descendants = Descendants();
			}
		}

		public void Kill()
		{
			if (IsStarted())
			{
				try
				{
					if (_Process.CloseMainWindow()) // try and kill as gui application
					{
						try
						{
							_Process.WaitForExit(250);
						}
						catch (SystemException)
						{
							// The process associated with this object is gone
						}
					}
					if (!HasExited())
					{
						_Process.Kill(); // kill console application
						try
						{
							_Process.WaitForExit(); // this also guarantees async output handlers are done
							ExitCode = _Process.ExitCode; // cannot be retrieve after closing process so cache it
							_Process.Close();
						}
						catch (SystemException)
						{
							// The process associated with this object is gone
						}
					}
				}
				catch (InvalidOperationException)
				{
					// The process has already exited.
				}
			}
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (_Disposed)
			{
				return;
			}

			Kill(); // kill parent first to stop it spawning any more children
			if (!AllowChildren)
			{
				KillDescendants(); // clean up descendants
			}

			if (disposing)
			{
				// We are actually not supposed to just call task Dispose().  If we call the task .Dispose()
				// directly while the task is not in expected state, we will get following exception message:
				// "A task may only be disposed if it is in a completion state (RanToCompletion, Faulted or Canceled)"
				// https://docs.microsoft.com/en-us/dotnet/standard/parallel-programming/task-cancellation
				// https://docs.microsoft.com/en-us/dotnet/standard/parallel-programming/how-to-cancel-a-task-and-its-children
				// We need to properly cancel the task first.  However, our current setup doesn't really allow task 
				// cancellation.  To allow task cancellation, the task needs to be created and started at the same time
				// with Task.Factory.StartNew() with cancellation token input and the TaskCancellationException needs to be thrown
				// from inside the task itself.  It will be too much of a code structure change.  For now, we'll just
				// catch and ignore any exception from the Dispose.

				try
				{
					if (_StdErrTask != null)
					{
						_StdErrTask.Dispose();
					}
				}
				catch
				{
				}

				try
				{
					if (_StdOutTask != null)
					{
						_StdOutTask.Dispose();
					}
				}
				catch
				{
				}
			}

			_Disposed = true;
		}

		public void Start()
		{
			if (!IsStarted())
			{
				try
				{
					_Process.Start();
				}
				catch (Exception e)
				{
					// Under unix environment, if file doesn't have exe bit set, we usually get permission denied message.
					// But unfortunately, we get "cannot find specified file" which is confusing. So we explicitly test for
					// this scenario and give better message.
					bool exeBitSet = true;
					try
					{
						if ((PlatformUtil.IsUnix || PlatformUtil.IsOSX) && System.IO.File.Exists(_Process.StartInfo.FileName))
						{
							exeBitSet = ProcessUtil.IsUnixExeBitSet(_Process.StartInfo.FileName, _Process.StartInfo.WorkingDirectory);
						}
					}
					catch (Exception e2)
					{
						throw new Exception(e.Message + System.Environment.NewLine + e2.Message);
					}
					if (!exeBitSet)
					{
						throw new Exception(String.Format("ERROR: The executable '{0}' appears to be lacking execute attribute.  Please make sure that the execute bit is set!", _Process.StartInfo.FileName));
					}
					else
					{
						// Seems to be some other exception.  Just re-throw the error.
						throw;
					}
				}
				PID = PID; //retrieves pid from process

				if (_StdOutTask != null)
				{
					_StdOutTask.Start();
				}
				if (_StdErrTask != null)
				{
					_StdErrTask.Start();
				}
			}
			else
			{
				throw new ApplicationException("Process already started!");
			}
		}

		public void Start(string stdIn)
		{
			System.Text.Encoding originalInputEncoding = Console.InputEncoding;
			// P4.exe on PC cannot handle UTF8 encoding with byte order mark (BOM) in the input stream.
			// If current system is setup using code page 65001 (UTF8), _Process.StandardInput.Write() will
			// start with a BOM.  Which is kinda stupid.  In order to get around that, we need to change
			// the entire Console's InputEncoding format to make sure we are using UTF8 without BOM.
			// Then reset back to original when we're done.  We'll only do this on PC for now as
			// we only see this on PC.
			if (PlatformUtil.IsWindows && 
				originalInputEncoding.EncodingName.Contains("UTF-8") &&
				originalInputEncoding.GetPreamble().Length > 0)	// This is where BOM is stored!
			{
				System.Text.Encoding utf8NoBOM = new System.Text.UTF8Encoding(false);
				Console.InputEncoding = utf8NoBOM;
			}
			Start();
			_Process.StandardInput.Write(stdIn);
			_Process.StandardInput.Close();
			Console.InputEncoding = originalInputEncoding;
		}

		public bool WaitForExit(int milliseconds)
		{
			bool exited = _Process.WaitForExit(milliseconds);
			if (!exited)
			{
				// If process is killed, we can't get the ExitCode any more as "_Process" object no longer
				// hold a valid process.  When the _Process.ExitCode() gets executed, we receive a "No process
				// is associated with this object" exception (which is cryptic to end user). So we need to 
				// exit out of this function now and return false and let higher level function call to 
				// decide if they really want to throw exception.
				Kill();
				return false;
			}

			// if we created output handling tasks make sure these threads are terminated
			// before we return execution to calling function
			if (_StdOutTask != null)
			{
				_StdOutTask.Wait();
			}
			if (_StdErrTask != null)
			{
				_StdErrTask.Wait();
			}

			ExitCode = _Process.ExitCode;
			_Process.Close();
			return exited;
		}

		public void WaitForExit()
		{
			_Process.WaitForExit();
			ExitCode = _Process.ExitCode;
			_Process.Close();
		}

		public void Execute()
		{
			Start();
			WaitForExit();
		}

		public void Execute(string stdIn)
		{
			Start(stdIn);
			WaitForExit();
		}

		public bool Execute(int timeoutMilliseconds)
		{
			Start();
			return WaitForExit(timeoutMilliseconds);
		}

		public bool Execute(int timeoutMilliseconds, string stdIn)
		{
			Start(stdIn);
			return WaitForExit(timeoutMilliseconds);
		}


		public bool IsStarted()
		{
			try
			{
				Process.GetProcessById(_Process.Id);
				return true;
			}
			catch (ArgumentException)
			{
				return false;
			}
			catch (InvalidOperationException)
			{
				return false;
			}
		}

		public bool IsRunning()
		{
			return IsStarted() && !HasExited();
		}

		public bool HasExited()
		{
			try
			{
				Process process = Process.GetProcessById(_Process.Id);
				return process.HasExited;
			}
			catch (Win32Exception)
			{
				return true;
			}
			catch (ArgumentException)
			{
				return true;
			}
			catch (InvalidOperationException)
			{
				return true;
			}
		}

		public ChildProcess[] Children()
		{
			return
			(
				from process in GetAllProceses()
				where process.ChildOf(this)
				select process
			).ToArray();
		}

		public ChildProcess[] Descendants()
		{
			return
			(
				from process in GetAllProceses()
				where process.DescendedFrom(this)
				select process
			).ToArray();
		}

		private bool ChildOf(ChildProcess parent)
		{
			// if process watcher returns true then we are a child however these is a window in which process watcher
			// has not noticed the parenthood so on false check the hard way
			if (ProcessWatcher.IsChildOf(PID, parent.PID))
			{
				return true;
			}
			return parent == Parent();
		}

		public bool DescendedFrom(ChildProcess ancestor)
		{
			// we trust process watcher with a positive result, but we double check with a negative result as there is some delay
			// however only process watcher can detect ancestry between a grandparent and a grandchild if the ancestor in between
			// has died. process watcher is also faster
			if (ProcessWatcher.IsDesendedFrom(PID, ancestor.PID))
			{
				return true;
			}
			ChildProcess parent = Parent();
			while (parent != null)
			{
				if (parent == ancestor)
				{
					return true;
				}
				parent = parent.Parent();
			}
			return false;
		}

		public override bool Equals(Object obj)
		{
			if (obj == null)
			{
				return false;
			}

			ChildProcess other = obj as ChildProcess;
			return Equals(other);
		}

		public bool Equals(ChildProcess other)
		{
			// cast to object to prevent calling operator overload
			if ((object)other == null)
			{
				return false;
			}

			if (PID == INVALID_PID || other.PID == INVALID_PID)
			{
				return false;
			}

			return PID == other.PID;
		}

		public override int GetHashCode()
		{
			return PID;
		}

		public override string ToString()
		{
			return String.Format("{0} ({1})", Name, PID);
		}

		public string GetStdOutput()
		{
			return _Process.StandardOutput.ReadToEnd();
		}

		public string GetStdError()
		{
			return _Process.StandardError.ReadToEnd();
		}

		private void ConfigureOutputHandler(ProcessOutputRecievedHandler outputEventHandler)
		{
			if (outputEventHandler != null)
			{
				_StdOutTask = new Task(() => { ConsumeProcessStream(_Process.StandardOutput, outputEventHandler); }, TaskCreationOptions.LongRunning);
			}
		}

		private void ConfigureErrorHandler(ProcessOutputRecievedHandler errorEventHandler)
		{
			if (errorEventHandler != null)
			{
				_StdErrTask = new Task(() => { ConsumeProcessStream(_Process.StandardError, errorEventHandler); }, TaskCreationOptions.LongRunning);
			}
		}

		private void ConsumeProcessStream(StreamReader processStream, ProcessOutputRecievedHandler outputEventHandler)
		{
			while (true)
			{
				string line = processStream.ReadLine();
				if (line == null)
				{
					return;
				}
				outputEventHandler(this, new ProcessOutputEventArgs(line));
			}
		}

		public static bool operator ==(ChildProcess a, ChildProcess b)
		{
			if (ReferenceEquals(a, b))
			{
				return true;
			}

			// reference equals will have returned true if they are both null
			// cast to object to prevent recursive call to this function
			if ((object)a == null)
			{
				return false;
			}

			// returns false if b is null
			return a.Equals(b);
		}

		public static bool operator !=(ChildProcess a, ChildProcess b)
		{
			return !(a == b);
		}

		public static ChildProcess Create(string command, string arguments = null, string workingDir = null, ProcessOutputRecievedHandler outputEventHandler = null, ProcessOutputRecievedHandler errorEventHandler = null)
		{
			ChildProcess process = CreatePlatformChildProcess(CreateInternalProcess(command, arguments, workingDir));
			process.ConfigureOutputHandler(outputEventHandler);
			process.ConfigureErrorHandler(errorEventHandler);
			return process;
		}

		public static ChildProcess GetCurrentProcess()
		{
			return CreatePlatformChildProcess(Process.GetCurrentProcess());
		}

		public static ChildProcess[] GetAllProceses()
		{
			if (PlatformUtil.IsUnix || PlatformUtil.IsOSX)
			{
				return (from proc in Process.GetProcesses() select CreatePlatformChildProcess(proc)).ToArray();
			}
			else
			{
				return (from proc in Process.GetProcesses() select CreatePlatformChildProcess(proc)).Where(cp => cp.IsRunning()).ToArray();
			}
		}

		public static ChildProcess GetProcessById(int PID)
		{
			try
			{
				return CreatePlatformChildProcess(Process.GetProcessById(PID));
			}
			catch (ArgumentException)
			{
				return null;
			}
			catch (InvalidOperationException)
			{
				return null;
			}
		}

		// call this function to install event handlers kill all child processes when this process is aborted (ctrl+c, window close, etc) or when it exits
		public static void SetCurrentProcessKillAllDescendantsOnAbortOrExit()
		{
			throw new NotImplementedException();
		}

		public static ChildProcess FromSystemProcess(Process process)
		{
			return CreatePlatformChildProcess(process);
		}

		protected static ChildProcess CreatePlatformChildProcess(Process process)
		{
			switch (PlatformUtil.Platform)
			{
				case PlatformUtil.Windows:
					return new WinProcess(process);
				case PlatformUtil.OSX:
					return new OSXProcess(process);
				case PlatformUtil.Unix:
					return new LinuxProcess(process);
				default:
					throw new NotImplementedException("Child process not implemented for this platform!");
			}
		}

		private static Process CreateInternalProcess(string command, string arguments = null, string workingDir = null)
		{
			Process process = new Process();
			process.StartInfo.FileName = command;
			process.StartInfo.UseShellExecute = false;
			process.StartInfo.CreateNoWindow = true;
			process.StartInfo.RedirectStandardOutput = true;
			process.StartInfo.RedirectStandardError = true;
			process.StartInfo.RedirectStandardInput = true;
			process.StartInfo.Arguments = arguments;
			process.StartInfo.WorkingDirectory = workingDir;
			process.StartInfo.StandardOutputEncoding = System.Text.Encoding.UTF8;
			process.StartInfo.StandardErrorEncoding = System.Text.Encoding.UTF8;
			process.EnableRaisingEvents = true;
			return process;
		}
	}
}
