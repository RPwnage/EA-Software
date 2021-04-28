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
using System.IO;
using System.Linq;
using System.Threading.Tasks;

using Microsoft.Win32;

using NAnt.Core.Util;
using NAnt.Perforce.Extensions.String;
using NAnt.Perforce.Processes;

namespace NAnt.Perforce
{
	namespace Internal
	{
		internal static class P4Caller
		{
			public interface ITaskSpecificOutputMonitor
			{
				void TaskOutputProc(string x);
			}

			private class ResponseFile : IDisposable
			{
				internal readonly string Path;

				internal ResponseFile(string[] args)
				{
					Path = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "p4_rspfile_" + System.IO.Path.GetRandomFileName());
					if (File.Exists(Path))
					{
						File.Delete(Path);
					}
					File.WriteAllLines(Path, args);
				}

				public void Dispose()
				{
					if (File.Exists(Path))
					{
						File.Delete(Path);
					}
				}
			}

			internal static string P4ExecutableLocation
			{
				get
				{
					return GetP4Executable();
				}
			}
			
			private static string _P4ClientVersionStringCache = null;

			// track the last 10 commands issued to p4, thread access is locked on _CommandHistory
			private static int _CommandBufferIndex = 0;
			private static readonly string[] _CommandHistoryBuffer = Enumerable.Repeat<string>(null, 5).ToArray();

			private static string P4ExecutableStr = null;
			private static readonly string[] _PreGroupOutputCommands = { "print" }; // print is also the only command that can give us output before our tagged regex, handled specially to avoid tripping up on descriptions
			private static readonly string[] _MultiRecordCommands = { "add", "changes", "clients", "dirs", "edit", "move", "files", "flush", "fstat", "integrate", "print", "opened", "reconcile", "reopen", "resolve", "revert", "sync", "submit", "unshelve", "users", "where" }; // impossible to 100% detect from output whether its a multi record, so list expectations here. if this doesn't cut it we'll have to fall back on a map of multirecord commands to expected fields

			private static readonly string p4Executable = "p4.exe";
			private static readonly string p4LocationX64 = @"c:\Program Files\Perforce\p4.exe";
			private static readonly string p4LocationX86 = @"c:\Program Files (x86)\Perforce\p4.exe";
			internal static P4Output Run(P4Context context, string command, string[] args = null, string input = null, ResponseFileBehaviour responseFileBehaviour = P4Defaults.DefaultResponseFileBehaviour, uint retries = P4Defaults.DefaultRetries, bool useSyncOuput = false, int timeoutMillisec = P4GlobalOptions.DefaultLongTimeoutMs, ITaskSpecificOutputMonitor taskOutputMonitor = null, bool suppressTicketsFile = true)
			{
				bool useResponseFile = ShouldUseResponseFile(responseFileBehaviour, context, command, args, retries);
				suppressTicketsFile = suppressTicketsFile && (context.GetServer().UseExplicitPassword || context.GetServer().UseExplicitTicket);

				if (args != null && args.Length > 0)
				{
					if (useResponseFile)
					{
						using (ResponseFile respFile = new ResponseFile(args))
						{
							string p4Args = String.Format("{0} {1}", String.Join(" ", context.GlobalArgsString(retries, respFile.Path)), command);
							return Run(command, input, p4Args, context, args, useSyncOuput, timeoutMillisec, taskOutputMonitor, suppressTicketsFile);
						}
					}
					else
					{
						string p4Args = String.Format("{0} {1} {2}", String.Join(" ", context.GlobalArgsString(retries)), command, String.Join(" ", (from arg in args select QuoteIfNecessary(arg))));
						return Run(command, input, p4Args, context, useSyncOuput: useSyncOuput, timeoutMillisec: timeoutMillisec, taskOutputMonitor: taskOutputMonitor, suppressTicketsFile: suppressTicketsFile);
					}
				}
				else
				{
					string p4Args = String.Format("{0} {1}", String.Join(" ", context.GlobalArgsString(retries)), command);
					return Run(command, input, p4Args, context, useSyncOuput: useSyncOuput, timeoutMillisec: timeoutMillisec, taskOutputMonitor: taskOutputMonitor, suppressTicketsFile: suppressTicketsFile);
				}
			}

			internal static string GetClientVersionString(P4Context sanitizeContext = null)
			{
				// This function appears to get called multiple times.  So we'll just cache the value.
				if (String.IsNullOrEmpty(_P4ClientVersionStringCache))
				{
					_P4ClientVersionStringCache = Run(null, null, "-V", sanitizeContext, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Messages.Last();
				}
				return _P4ClientVersionStringCache;
			}

			private static P4Output Run(string command, string input, string p4Args, P4Context sanitizeContext = null, string[] responseFileArgs = null, bool useSyncOuput = false, int timeoutMillisec = P4GlobalOptions.DefaultLongTimeoutMs, ITaskSpecificOutputMonitor taskOutputMonitor = null, bool suppressTicketsFile = true)
			{
				// handle output special case for print
				bool preGroups = _PreGroupOutputCommands.Contains(command);
				
				// set up p4 commands
				string reportableInput = sanitizeContext != null ? sanitizeContext.GetServer().SanitizeInput(input) : input;
				string reportableCommand = sanitizeContext != null ? sanitizeContext.GetServer().SanitizeCommand(String.Format("{0} {1}", P4ExecutableLocation, p4Args)) : null;

				string[] savedCommandHistory = null;
				lock (_CommandHistoryBuffer)
				{
					// store a copy if current command history in output
					savedCommandHistory = _CommandHistoryBuffer.Skip(_CommandBufferIndex)
						.Concat(_CommandHistoryBuffer.Take(_CommandBufferIndex))
						.Where(cmd => cmd != null)
						.ToArray();

					// update command history with new command
					_CommandHistoryBuffer[_CommandBufferIndex] = reportableCommand;
					_CommandBufferIndex = (_CommandBufferIndex + 1) % _CommandHistoryBuffer.Length;
				}

				P4Output p4output = new P4Output(reportableCommand, savedCommandHistory, reportableInput, _MultiRecordCommands.Contains(command), preGroups, responseFileArgs);
				bool asyncOutput = !useSyncOuput;

				// set up p4 processs, we use a temp directory and a blank temp p4config file to prevent P4 from picking up any user setings			
				// temp working dir
				string tempWorkingDir = Path.Combine(Path.GetTempPath(), "p4_workingdir_" + Path.GetFileNameWithoutExtension(Path.GetRandomFileName()));
				if (Directory.Exists(tempWorkingDir))
				{
					Directory.Delete(tempWorkingDir, true);
				}
				Directory.CreateDirectory(tempWorkingDir);

				// temp junk file - give p4 an empty tickets and config file
				string tempConfigFile = Path.GetRandomFileName();
				File.WriteAllText(Path.Combine(tempWorkingDir, tempConfigFile), "");
				bool processTimedOut = false;

				// run p4
				try
				{
					using (ChildProcess p4Proc = CreateP4Process(p4Args, p4output, tempWorkingDir, tempConfigFile, asyncOutput, taskOutputMonitor, suppressTicketsFile))
					{
						// p4 doesn't spawn children so disable slow child clean up
						p4Proc.AllowChildren = true;

						if (input != null)
						{
							p4Proc.Start(input);
						}
						else
						{
							p4Proc.Start();
						}

						// reading output must be done before exit for commands like "print" as they can cause p4 to hang waiting for us to clear some 
						// space in stdout buffer
						if (!asyncOutput)
						{
							// do these in parallel as if they are run serial we can theoretically block the child when it tries to write to standard error if we
							// are trying to read to the end of standard output
							string fullOutput = null;
							string fullError = null;

							Parallel.Invoke
							(
								() =>
								{
									fullOutput = p4Proc.GetStdOutput();
								},
								() =>
								{
									fullError = p4Proc.GetStdError();
								}
							);

							Array.ForEach(fullOutput.Split('\n'), output => { OutputProc(p4output, output); });
							Array.ForEach(fullError.Split('\n'), error => { ErrorProc(p4output, error); });
						}

						bool success = p4Proc.WaitForExit(timeoutMillisec);
						if (!success)
						{
							processTimedOut = true;
						}
					}

					// Should throw the time out exception outside above scope to allow the above "using" block
					// to properly close down any stderr / stdout tasks first.  We have been having concurrency
					// issues with p4output's _UnprocessOutput usage.
					if (processTimedOut)
					{
						throw new TimeOutException(reportableCommand, timeoutMillisec);
					}

					// throw all exception generated from errors in the command
					//
					// NOTE: We need to make sure we call p4output.ThrowAnyExceptions() outside the scope of
					// the above "using p4Proc" to make sure all "child" stdout/stderr tasks are shut down as well.
					// Otherwise, the p4output.ThrowAnyExceptions() would clear and set _UnprocessOutput to null while
					// the stdout/stderr were still trying to send output.
					p4output.ThrowAnyExceptions();
				}
				finally
				{
					if (Directory.Exists(tempWorkingDir))
					{
						try
						{
							Directory.Delete(tempWorkingDir, true);
						}
						catch
						{
							// If the p4 process is killed due to a timeout, the "process" actually didn't always terminate quick enough and
							// we could run into situation where the above Delete function throwing exception saying another process is using
							// it.  If this happens, we'll just ignore the error.
						}
					}
				}

				return p4output;
			}

			private static ChildProcess CreateP4Process(string p4Args, P4Output p4output, string workingDir, string junkFile, bool asyncOutput, ITaskSpecificOutputMonitor taskOutputMonitor = null, bool suppressTicketsFile = true)
			{
				ChildProcess p4Process = null;
				if (asyncOutput)
				{
					p4Process = ChildProcess.Create(
						P4ExecutableLocation,
						p4Args,
						workingDir,
						(obj, output) =>
						{
							OutputProc(p4output, output.Data);
							if (taskOutputMonitor != null)
							{
								taskOutputMonitor.TaskOutputProc(output.Data);
							}
						},
						(obj, errorOut) =>
						{
							ErrorProc(p4output, errorOut.Data);
						}
					);
				}
				else
				{
					p4Process = ChildProcess.Create(P4ExecutableLocation, p4Args, null);
				}

				// clear environment variables to stop these being picked up by p4
				// set P4ENVIRO to the explict path of our empty config file
				// set P4CONFIG to the name of our empty config file (which will be in our temp working directory)
				// set P4TICKETS to junk file as well - we don't want it to find a ticket without our explicit passing of one
				p4Process.Environment.Remove("P4CLIENT");
				p4Process.Environment.Remove("P4USER");
				p4Process.Environment.Remove("P4PORT");
				p4Process.Environment.Remove("P4PASSWD");
				p4Process.Environment["P4CONFIG"] = junkFile;
				p4Process.Environment["P4ENVIRO"] = Path.Combine(workingDir, junkFile);
				if (suppressTicketsFile)
				{
					p4Process.Environment["P4TICKETS"] = Path.Combine(workingDir, junkFile);
				}

				return p4Process;
			}

			private static void OutputProc(P4Output p4output, string output)
			{
				p4output.AddOutput(output);
			}

			private static void ErrorProc(P4Output p4output, string errorOut)
			{
				p4output.AddError(errorOut);
			}

			private static string GetP4Executable()
			{
				if (P4ExecutableStr == null)
				{
					if (PlatformUtil.IsUnix || PlatformUtil.IsOSX)
					{
						P4ExecutableStr = GetUnixP4Executable();
					}
					else
					{
						P4ExecutableStr = GetWindowsP4Executable();
					}
					//important: ensure the version is at least the minimum, otherwise throw an exception because we assume a minimum version in future code.
					P4Version version = new P4Version(GetClientVersionString(null));
					if (!version.AtLeast(2015, 2))
					{
						throw new Exception(String.Format("ERROR: The p4 client version is currently '{0}'. Please make sure you are running at least version '2015.2' .", _P4ClientVersionStringCache));
					}
				}
				return P4ExecutableStr;
			}

			private static string SearchPathForP4Exe()
			{
				// Search if p4.exe is in system path
				string pathEnv = Environment.GetEnvironmentVariable("PATH");
				string[] paths = pathEnv.Split(';');
				string p4LocationUserPath = paths.Select(x => Path.Combine(x, p4Executable)).Where(x => File.Exists(x)).FirstOrDefault();
				if (!String.IsNullOrEmpty(p4LocationUserPath))
				{
					return p4LocationUserPath;
				}

				return null;
			}

			private static string GetWindowsP4Executable()
			{
				// try get install path from registry
				const string p4InstRootKey = @"SOFTWARE\Perforce\Environment";
				const string p4InstRootValue = "P4INSTROOT";
				string p4InstRoot = null;

				//if we are set to search the path first, do so. Otherwise, wre will check as a last resort later in this function
				if (P4GlobalOptions.P4ExeSearchPathFirst)
				{
					string path = SearchPathForP4Exe();
					if (path != null)
					{
						return path;
					}
				}

				using (RegistryKey key = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64).OpenSubKey(p4InstRootKey, false))
				{
					if (key != null)
					{
						object value = key.GetValue(p4InstRootValue);
						if (value != null)
						{
							p4InstRoot = value.ToString();
						}
					}
				}
				if (p4InstRoot == null)
				{
					using (RegistryKey key = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32).OpenSubKey(p4InstRootKey, false))
					{
						if (key != null)
						{
							object value = key.GetValue(p4InstRootValue);
							if (value != null)
							{
								p4InstRoot = value.ToString();
							}
						}
					}
				}

				if (!String.IsNullOrWhiteSpace(p4InstRoot))
				{
					string p4ExeInstPath = Path.Combine(p4InstRoot, p4Executable);
					if (File.Exists(p4ExeInstPath))
					{
						return p4ExeInstPath;
					}
				}

				// fall back to likely install path
				if (File.Exists(p4LocationX64))
				{
					return p4LocationX64;
				}
				if (File.Exists(p4LocationX86))
				{
					return p4LocationX86;
				}

				//If path first property is not set, we need to check it here. Otherwise, it would have been checked before now.
				if (!P4GlobalOptions.P4ExeSearchPathFirst)
				{
					string path = SearchPathForP4Exe();
					if (path != null)
					{
						return path;
					}
				}
				
				throw new FileNotFoundException(String.Format("Cannot locate p4.exe. Framework expects p4.exe to be locatable by the local machine {0} registry key or located at {1} or {2} or in your systemm PATH.", p4InstRootKey, p4LocationX64, p4LocationX86));
			}

			private static string GetUnixP4Executable()
			{
				// Need to do a quick test to make sure p4 is installed and in system path.
				string p4Exe = @"p4";

				// On osx when nant is being re-spawn from Xcode's schell command, Xcode seems to 
				// completely changed the environment path (mainly strip out /usr/local/bin, but /usr/bin still exists)
				// causing p4 application not found error.  Initially attempted to spawn a process and try to execute "p4 -V"
				// to test for p4 in the path.  Unfortunately ProcessStartInfo's EnvironmentVariables
				// is read only and you can't update the path variable (and try to add /usr/local/bin).  
				// So we have to tests all the paths ourselves and specify the full path.
				string pathEnv = Environment.GetEnvironmentVariable("PATH") + ":/usr/local/bin:/usr/bin:/usr/sbin:/sbin";
				string[] paths = pathEnv.Split(':');
				string p4LocationUnix = paths.Select(x => Path.Combine(x, p4Exe)).Where(x => File.Exists(x)).FirstOrDefault();
				if (String.IsNullOrEmpty(p4LocationUnix))
				{
					throw new FileNotFoundException("Cannot locate p4. Please make sure command line p4 is installed in your path (like /usr/local/bin) and have executable attribute set.");
				}

				bool exeBitSet = true;
				try
				{
					if ((PlatformUtil.IsUnix || PlatformUtil.IsOSX) && File.Exists(p4LocationUnix))
					{
						exeBitSet = ProcessUtil.IsUnixExeBitSet(p4LocationUnix);
					}
				}
				catch
				{
					// Looks like there is an error executing MonoHelper to determine the file's exe bit.  Just suppress this exception for now.
					// If the process failed to execute, we actually have another check when the process is being executed.
				}
				if (!exeBitSet)
				{
					throw new Exception(String.Format("ERROR: The p4 executable '{0}' appears to be lacking execute attribute. Please make sure that the execute bit is set!", p4LocationUnix));
				}

				return p4LocationUnix;
			}

			private static bool ShouldUseResponseFile(ResponseFileBehaviour responseFileBehaviour, P4Context context, string command, string[] args, uint retries)
			{
				if (responseFileBehaviour == ResponseFileBehaviour.UseGlobalDefault)
				{
					responseFileBehaviour = P4GlobalOptions.ResponseFileBehaviour;
				}

				switch (responseFileBehaviour)
				{
					case ResponseFileBehaviour.UseGlobalDefault: // in case someone set the global default to be use global default
					case ResponseFileBehaviour.UseWhenLimitExceeded:
						return CommandLineExceedsLimit(context, command, args, retries);
					case ResponseFileBehaviour.AlwaysUse:
						return true;
					case ResponseFileBehaviour.NeverUse:
						return false;
					default:
						throw new InvalidOperationException("Could not determine response file behaviour.");
				}
			}

			private static string QuoteIfNecessary(string input)
			{
				if (input.Any(c => c == '\\' || Extensions.String.Extensions.WhiteSpaceDelimiters.Contains(c)))
				{
					return input.Quoted();
				}
				return input;
			}

			private static bool CommandLineExceedsLimit(P4Context context, string command, string[] args, uint retries)
			{
				uint commandLineMax = PlatformUtil.MaxCommandLineLength;
				commandLineMax = commandLineMax - Math.Min(commandLineMax, 128); // add a small safety buffer

				string commandLine = null;
				if (args != null && args.Length > 0)
				{
					commandLine = String.Format("{0} {1} {2} {3}", GetP4Executable(), context.GlobalArgsString(retries), command, String.Join(" ", (from arg in args select QuoteIfNecessary(arg))));
				}
				else 
				{
					commandLine = String.Format("{0} {1} {2}", GetP4Executable(), context.GlobalArgsString(retries), command);
				}

				return commandLine.Length >= commandLineMax;
			}
		}
	}
}
