// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001 Gerry Shaw
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
// Jim Petrick (jpetrick@ea.com)

using System;
using System.IO;                            //	TODO: Delete this if not using a form for input.
using System.Diagnostics;                   //	TODO: Delete this if not using a form for input.

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.PerforceTasks    //	TODO: Replace 'PerforceTasks' with your namespace. 
{
	/// <summary>
	/// TODO: This provides helper functions to interface with Perforce.
	/// </summary>
	public class P4Form: IDisposable
	{
		private Process p = null;			//	Process created via the GetForm utility.
		private string stdout = null;			//	Holds the stdout string from a call to GetForm.
		private string stderr = null;			//	Holds the stderr string from a call to GetForm.
		protected string sentForm = null;           //	Holds last form sent via PutForm (for DEBUG only).
		private readonly Log Log;

		public P4Form(string prog, string dir, bool fail, Log log)
		{
			Log = log;
			ProgramName = prog;
			WorkDir = dir;
			FailOnError = fail;
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (disposing)
			{
				p.Dispose();
			}
		}

		/// <summary>Returns the name of the program to execute.</summary>
		public string ProgramName { get; set; } = "p4.exe";

		/// <summary>The working directory that the program executes in.</summary>
		public string WorkDir { get; set; } = String.Empty;

		/// <summary>Determines if task failure stops the build, or is just reported. Default is "true".</summary>
		public bool FailOnError { get; set; } = true;

		/// <summary>The maximum amount of time the application is allowed to execute, expressed in milliseconds.  Defaults to no time-out.</summary>
		public int TimeOut { get; set; } = Int32.MaxValue;

		/// <summary>The form we are operating on.</summary>
		public string Form { get; set; } = String.Empty;


		/// <summary>
		/// This routine executes P4 with the given command and saves the stdout in 'Form'.
		/// It is meant to be used to retrieve form data from Perforce.
		/// </summary>
		/// <param name="command">Command line to be sent to Perforce.</param>
		public void GetForm(string command)
		{
			DoP4Command(command, false);
		}	//	end GetForm

		/// <summary>
		/// This routine executes P4 with the given command to write form data to Perforce.
		/// It is meant to be used to write form data to Perforce.
		/// </summary>
		/// <param name="command">Command line to be sent to Perforce.</param>
		public void PutForm(string command)
		{
			DoP4Command(command, true);
		}	//	end PutForm


		/// <summary>
		/// This routine executes P4 with the given command and redirects the stdin
		/// to be the input form.
		/// </summary>
		/// <param name="commandLine">Command line to be sent to Perforce.</param>
		/// <param name="redirect">If true, redirects the standard input stream.</param>
		private void DoP4Command(string commandLine, bool redirect)
		{
			//	Clear the output stream captures for each command.
			stdout = null;
			stderr = null;

			//	Set up the process starting info.
			p = new Process();
			p.StartInfo.FileName = ProgramName;
			p.StartInfo.Arguments = commandLine;
			p.StartInfo.RedirectStandardInput = redirect;
			p.StartInfo.RedirectStandardOutput = true;
			p.StartInfo.RedirectStandardError = true;
			p.StartInfo.UseShellExecute = false;		//	Required to allow redirects.
			p.StartInfo.WorkingDirectory = WorkDir;		//	Required to allow redirects.
			p.StartInfo.CreateNoWindow = true;			//	Hide the console window if launched from GUI

			try
			{
				// TODO: try to get the code in ExternalProgramBase is a state so that we
				// can just use the code from the file instead of copying and pasting

				//	Start the process up.
				p.Start();

				//	Write the form to stdin.
				if (redirect)
				{
					sentForm = Form;
					p.StandardInput.Write(Form);
					p.StandardInput.Close();
				}

				using (var stdOutThread = new System.Threading.Tasks.Task(StdOut, System.Threading.Tasks.TaskCreationOptions.LongRunning))
				{
					using (var stdErrThread = new System.Threading.Tasks.Task(StdErr, System.Threading.Tasks.TaskCreationOptions.LongRunning))
					{
						// start the threads
						stdOutThread.Start();
						stdErrThread.Start();

						// wait for program to exit
						p.WaitForExit(TimeOut);


						// make sure that these threads have time to finish before we start using the results
						stdOutThread.Wait();
						stdErrThread.Wait();

					}
				}

			}
			catch (Exception e)
			{
				// Under unix environment, if file doesn't have exe bit set, we usually get permission denied message.
				// But unfortunately, we get "cannot find specified file" which is confusing. So we explicitly test for
				// this scenario and give better message.
				bool exeBitSet = true;
				try
				{
					if ((PlatformUtil.IsUnix || PlatformUtil.IsOSX) && System.IO.File.Exists(ProgramName))
					{
						exeBitSet = ProcessUtil.IsUnixExeBitSet(ProgramName, WorkDir);
					}
				}
				catch (Exception e2)
				{
					throw new BuildException(this.GetType().ToString() + ": Error running external program (" + ProgramName + ")." + System.Environment.NewLine + e2.Message, e);
				}
				if (!exeBitSet)
				{
					throw new BuildException(this.GetType().ToString() + ": Error running external program (" + ProgramName + ").  The executable appears to be lacking the execute attribute.", e);
				}
				else
				{
					// Seems to be some other exception.  Just re-throw the error.
					throw new BuildException(this.GetType().ToString() + ": Error running external program (" + ProgramName + ").", e);
				}
			}

			// Keep the FailOnError check to prevent programs that return non-zero even if they are not returning errors.
			if (FailOnError && p != null && p.ExitCode != 0)
			{
				if (stderr != null && stderr.Length > 0)
				{
					int indentLevel = Log.IndentLevel;
					Log.IndentLevel = 0;
					Log.Error.WriteLine(ProgramName + ": " + stderr);
					Log.IndentLevel = indentLevel;
				}
				Form = String.Empty;
				if (stdout != null)
					Form = stdout;
				if (stderr != null)
					Form += stderr;

				throw new BuildException(this.GetType().ToString() + "External program (" + ProgramName + ")returned errors:\n" + stderr);
			}

			//	Save the stdOut as the new form.
			if (redirect == false && stdout == null)
				throw new BuildException(this.GetType().ToString() + "External program (" + ProgramName +
										") did not return an expected form.\nnstderr =[" + stderr + "]\n");
			Form = stdout;
			FilterComments();
		}	//	end DoP4Command

		/// <summary>
		/// This function will capture the stdout from the process and save it in
		/// a string.
		/// </summary>		
		private void StdOut()
		{
			// Capture standard output
			StreamReader stream = p.StandardOutput;

			string output;
			while ((output = stream.ReadLine()) != null)
				stdout += output + "\n";

		}	//	end StdOut

		/// <summary>
		/// This function captures the stderr from the process and send it to the log.
		/// </summary>		
		private void StdErr()
		{
			StreamReader stream = p.StandardError;
			string errors;

			while ((errors = stream.ReadLine()) != null)
				if (errors.Length > 0)
					stderr += errors + "\n";
		}	//	end StdErr


		/// <summary>
		/// Finds the named field in the form and sets it to 'value'.
		/// </summary>
		/// <param name="field">The name of the field to replace.</param>
		/// <param name="newValue">The new value for the field.</param>
		public void ReplaceField(string field, string newValue)
		{
			if (Form == null)
				return;

			bool found = false;
			bool done = false;
			string[] lines = Form.Split('\n');
			string output = "";

			foreach (string line in lines)
			{
				if (found)
				{
					if (line.Length > 0 && !Char.IsWhiteSpace(line[0]))	//	Not the value part of any field.
						done = true;
					else if (!done)							//	Still in the field we are replacing.
						continue;
				}
				else if (line.IndexOf(field) == 0)
				{
					//	Found the field tag, replace the rest of the line, then 
					//	delete all lines until the next field id is found 
					//	(any line not beginning with whitespace).
					if (newValue != null)
					{
						output += field + "\n";
						output += "\t" + newValue + "\n";
						found = true;
						continue;
					}
				}
				//	If we get here, the line belongs in the output file.
				output += line + "\n";
			}

			//	Save the output as the new form.
			Form = output + "\n";
		}	//	end ReplaceField

		/// <summary>
		/// Locates the named field and returns it's value.
		/// </summary>
		/// <param name="field">The name of the field to locate.</param>
		/// <returns>The value of the field within the form, or null if field is not found.</returns>
		public string GetField(string field)
		{
			bool found = false;
			string[] lines = Form.Split('\n');
			string output = "";

			foreach (string line in lines)
			{
				if (found)
				{
					if (line.Length > 0 && !Char.IsWhiteSpace(line[0]))		//	Not the value part of any field.
						return output.Trim();
					output += line + "\n";
				}
				else if (line.IndexOf(field) == 0)
				{
					//	Found the field tag, replace the rest of the line, then 
					//	delete all lines until the next field id is found 
					//	(any line not beginning with whitespace).
					output = line.Substring(field.Length + 1) + "\n";
					found = true;
					continue;
				}
			}
			return (found) ? output.Trim() : null;
		}	//	end GetField

		/// <summary>Removes all comments from the form.</summary>
		private void FilterComments()
		{
			if (Form == null)
				return;

			string[] lines = Form.Split('\n');
			string output = "";

			foreach (string line in lines)
			{
				string oline = line;
				if (oline.Length > 0)
				{
					int index = oline.IndexOf('#');
					if (index >= 0)
					{
						oline = oline.Substring(0, index);
						if (oline.Length == 0)
							continue;
					}
				}
				output += oline + "\n";
			}

			//	Save the output as the new form.
			Form = output;
		}	//	end FilterComments
	}	// end class P4Form
}
