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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections.Generic;
using System.IO;
using EA.Eaconfig;


namespace EA.CallTargetIfOutOfDate
{
	/// <summary>
	/// Compares inputs against outputs to determine whether to execute specified target.
	/// Target is executed if any of input files is newer than any of output files, any of output files does not exist, 
	/// or list of input dependencies does not match input dependencies from previous run.
	/// </summary>
	/// <remarks>
	/// <para>
	/// When target does not produce any output files, DummyOutputFile can be specified.
	/// </para>
	/// <para>
	/// This task is similar to task "ExecuteIfOutOfDate" except it executes specified target instead of script.
	/// </para>
	/// </remarks>
	[TaskName("CallTargetIfOutOfDate", NestedElementsCheck = true)]
	public class calltargetifoutofdate : Task
	{

		#region Private Instance Fields

		#endregion Private Instance Fields

		#region Public Instance Properties

		/// <summary>
		/// Input dependency files. 
		/// </summary>
		[TaskAttribute("InputFileset", Required = true)]
		public string InputFileSetName { get; set; } = null;

		/// <summary>
		/// Output dependency files. If target does not produce any output files "DummyOutputFile" parameter can be specified.
		/// </summary>
		[TaskAttribute("OutputFileset", Required = false)]
		public string OutputFileSetName { get; set; } = null;

		/// <summary>
		/// Contains file path. File is created automatically when task is executed and added to "output dependency files" list.
		/// </summary>
		[TaskAttribute("DummyOutputFile", Required = false)]
		public string DummyOutputFileName { get; set; } = null;

		/// <summary>
		/// File to store list of input dependency files from "InputFileset" parameter. 
		/// This list is used to check whether set of input dependencies changed from previous run. Target is executed when list changes.
		/// </summary>
		[TaskAttribute("DependencyFile", Required = true)]
		public string DependencyFileName { get; set; } = null;

		/// <summary>
		/// Name of a target to execute
		/// </summary>
		[TaskAttribute("TargetName", Required = true)]
		public string TargetName { get; set; } = null;

		#endregion Public Instance Properties

		#region static utility functions

		public static bool SortedListsEqual(List<string> listA, List<string> listB)
		{
			if (listA.Count != listB.Count)
			{
				return false;
			}

			IEnumerator<string> a = listA.GetEnumerator();
			IEnumerator<string> b = listB.GetEnumerator();
			while (a.MoveNext() && b.MoveNext())
			{
				if (!string.Equals(a.Current, b.Current, StringComparison.OrdinalIgnoreCase))
				{
					return false;
				}
			}
			return true;
		}

		/// <summary>Reads list of files from dependency file</summary>
		/// <returns>A sorted List containing all files listed in the dependency file</returns>
		public static List<string> ReadDependencyFile(TextReader dependencyFile)
		{
			List<string> files = new List<string>();
			string line;
			while ((line = dependencyFile.ReadLine()) != null)
			{
				line = line.TrimEnd(new char[] { '\r', '\n' });
				if (line.Length > 0)
				{
					files.Add(line);
				}
			}
			files.Sort();
			return files;
		}

		/// <summary>Writes list of files to a dependency file</summary>
		public static void WriteDependencyFile(TextWriter dependencyFile, List<string> files)
		{
			foreach (string file in files)
			{
				if (file.Length > 0)
				{
					dependencyFile.WriteLine(file);
				}
			}
		}

		public static List<string> GetFileListFromFileSet(FileSet fileSet)
		{
			List<string> files = new List<string>();
			foreach (FileItem file in fileSet.FileItems)
			{
				files.Add(file.FileName);
			}
			return files;
		}

		#endregion static utility functions

		/// <summary>Determine if the current and previous list of input files differ</summary>
		/// <returns><c>true</c> if the lists are not identical</returns>
		public bool InputDependencyListChanged(List<string> inputFiles, List<string> previousInputFiles)
		{
			if (previousInputFiles.Count != inputFiles.Count)
			{
				return true;
			}

			previousInputFiles.Sort();
			inputFiles.Sort();

			if (!SortedListsEqual(previousInputFiles, inputFiles))
			{
				return true;
			}

			return false;
		}

		/// <summary>Determine if the task needs to run.</summary>
		/// <returns><c>true</c> if we should run the program (dependents missing or not up to date), otherwise <c>false</c>.</returns>
		public bool TaskNeedsRunning(List<string> inputFiles, List<string> outputFiles, List<string> previousInputFiles)
		{
			// force rebuild if either input or output list is empty
			if (inputFiles.Count < 1)
			{
				return true;
			}
			if (outputFiles.Count < 1)
			{
				return true;
			}

			// force rebuild if dependency list has changed
			if (InputDependencyListChanged(inputFiles, previousInputFiles))
			{
				return true;
			}

			// force rebuild if any output file is missing
			foreach (string outputFile in outputFiles)
			{
				if (!File.Exists(outputFile))
				{
					Log.Info.WriteLine(LogPrefix + "'{0}' does not exist.", outputFile);
					return true;
				}
			}

			// force rebuild if any output file is older than any input file
			IEnumerator<string> currentInputFile = inputFiles.GetEnumerator();
			bool first = true;
			DateTime newestInputFileDateStamp = new DateTime();
			string newestFile;

			foreach (string outputFile in outputFiles)
			{
				if (!File.Exists(outputFile))
				{
					Log.Info.WriteLine("'{0}' does not exist.", outputFile);
					return true;
				}

				// rebuild if outputDateStamp is older than any input file
				DateTime outputDateStamp = File.GetLastWriteTime(outputFile);

				while (currentInputFile.MoveNext())
				{
					DateTime inputFileDateStamp = File.GetLastWriteTime(currentInputFile.Current);
					if (!first && inputFileDateStamp <= newestInputFileDateStamp)
					{
						continue;
					}
					first = false;
					newestInputFileDateStamp = inputFileDateStamp;
					newestFile = currentInputFile.Current;

					if (outputDateStamp <= newestInputFileDateStamp)
					{
						Log.Info.WriteLine(LogPrefix + "'{0}' is newer than '{1}'.", newestFile, outputFile);
						return true;
					}
				}
			}

			return false;
		}

		private List<string> GetOutputFiles()
		{
			List<string> outputFiles = new List<string>();
			if (OutputFileSetName != null && OutputFileSetName != string.Empty)
			{
				outputFiles.AddRange(GetFileListFromFileSet(Project.NamedFileSets[OutputFileSetName]));
			}

			if (DummyOutputFileName != null && DummyOutputFileName != string.Empty)
			{
				outputFiles.Add(DummyOutputFileName);
			}

			return outputFiles;
		}

		private List<string> GetInputFiles()
		{
			return GetFileListFromFileSet(Project.NamedFileSets[InputFileSetName]);
		}

		/// <summary>Execute the task.</summary>
		protected override void ExecuteTask()
		{
			// check input parameters
			// TODO: should this be moved to InitializeTask()?
			bool outputFileSetSpecified = (OutputFileSetName != null && OutputFileSetName != string.Empty);
			bool dummyOutputFileSpecified = (DummyOutputFileName != null && DummyOutputFileName != string.Empty);
			if (!outputFileSetSpecified && !dummyOutputFileSpecified)
			{
				throw new BuildException("You must specify one of OutputFileset or DummyOutputFile.", Location);
			}

			List<string> outputFiles = GetOutputFiles();
			List<string> inputFiles = GetInputFiles();
			List<string> previousInputFiles = new List<string>();

			if (!File.Exists(DependencyFileName))
			{
				Log.Info.WriteLine(LogPrefix + "'{0}' does not exist.", DependencyFileName);
			}
			else
			{
				using (TextReader dependencyFile = new StreamReader(DependencyFileName))
				{
					previousInputFiles = ReadDependencyFile(dependencyFile);
				}

				if (!TaskNeedsRunning(inputFiles, outputFiles, previousInputFiles))
				{
					Log.Info.WriteLine(LogPrefix + "up to date.");
					return;
				}
			}

			// touch dummy output file
			if (dummyOutputFileSpecified)
			{
				// borrowed from the <touch> task
				using (FileStream fs = File.Create(DummyOutputFileName))
				{
				}
				File.SetLastWriteTime(DummyOutputFileName, DateTime.Now);
			}

			try
			{
				// call target
				Target target = Project.Targets.Find(TargetName);
				if (target == null)
				{
					throw new BuildException(String.Format("Unknown target '{0}'.", TargetName));
				}
				target.Copy().Execute(Project);

				// write dependency file
				using (TextWriter dependencyFile = new StreamWriter(DependencyFileName))
				{
					WriteDependencyFile(dependencyFile, inputFiles);
				}
			}
			catch
			{
				Log.Info.WriteLine(LogPrefix + "failed.");

				// delete dummy output file if target build failed
				if (dummyOutputFileSpecified)
				{
					File.Delete(DummyOutputFileName);
				}

				// re-throw the exception
				throw;
			}

			Log.Info.WriteLine(LogPrefix + "succeeded.");
		}
	}
}
