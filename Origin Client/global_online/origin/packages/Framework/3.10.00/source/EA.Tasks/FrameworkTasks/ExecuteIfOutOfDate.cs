//-----------------------------------------------------------------------------
// ExecuteIfOutOfDate.cs
//
// NAnt custom task which does dependency checking based on the given input and
// output filesets.  If the dependency check fails, the given code will be
// executed and the dependency file updated.
//
// Required:
//	inputs - Fileset containing all input files for the code
//  outputs - Fileset containing all output files for the code
//  DependencyFile - following a successful run, a list of input files is written
//    to the dependency file.  This file is later used to determine if the list
//    of inputs has changed and a rebuild should be forced
//  code - block of nant code to execute if dependency check fails
//
// Optional
//  DummyOutputFile - if the code doesn't generate any output files, specify a
//    "dummy" output file, which will be generated automatically.  This provides
//    incremental build capability based on input file changes only.
//
//-----------------------------------------------------------------------------

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;


namespace EA.CallTargetIfOutOfDate
{
    /// <summary>
    /// Compares inputs against outputs to determine whether to execute specifed target.
    /// Target is executed if any of input files is newer than any of output files, any of output files does not exist, 
    /// or list of input dependencies does not match input dependencies from previous run.
    /// </summary>
    /// <remarks>
    /// This task is similar to task "CallTargetIfOutOfDate" except it executes arbitrary script from "code" instead of target.
    /// </remarks>
    [TaskName("ExecuteIfOutOfDate")]
	public class ExecuteIfOutOfDate : Task
    {
        #region Private Instance Fields

        FileSet _inputs = new FileSet();
        FileSet _outputs = new FileSet();
        string _dummyOutputFileName = null;
        string _dependencyFileName = null;
        PropertyElement _code = new PropertyElement();
        XmlElement _codeElement = null;

        #endregion Private Instance Fields

        #region Public Instance Properties

        /// <summary>Set of input files to check against.</summary>
        [FileSet("inputs")]
        public FileSet Inputs
        {
            get { return _inputs; }
        }

        /// <summary>Set of output files to check against.</summary>
        [FileSet("outputs")]
        public FileSet Outputs
        {
            get { return _outputs; }
        }

        /// <summary>If specified, this file is added to the list of output dependencies. Timestamp of DummyOutputFile file is updated when this task code is executed.</summary>
        [TaskAttribute("DummyOutputFile", Required = false)]
        public string DummyOutputFileName
		{
            get { return _dummyOutputFileName; }
            set { _dummyOutputFileName = value; }
		}

        /// <summary>If DependencyFile is specified, the list of "inputs" is stored in the file with this name. It is used to check for missing/added input files during consequtive runs.</summary>
        [TaskAttribute("DependencyFile", Required = false)]
        public string DependencyFileName
        {
            get { return _dependencyFileName; }
            set { _dependencyFileName = value; }
        }

        /// <summary>
        /// The Nant script to execute.
        /// </summary>
        [Property("code")]
        public PropertyElement Code
        {
            get { return _code; }
            set { _code = value; }
        }

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
                if (0 != string.Compare(a.Current, b.Current, true))
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

        public ExecuteIfOutOfDate()
        {
            Inputs.FailOnMissingFile = true;
            Outputs.FailOnMissingFile = false;
        }

        protected override void InitializeTask(XmlNode taskNode)
        {
            // find the <Code> element and get all the xml without expanding any text
            _codeElement = (XmlElement)taskNode.GetChildElementByName("code");
            if (_codeElement == null)
            {
                string msg = String.Format("Missing required <code> element.");
                throw new BuildException(msg);
            }
            Code.Value = _codeElement.OuterXml.Trim();
        }

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
            outputFiles.AddRange(GetFileListFromFileSet(Outputs));

            if (DummyOutputFileName != null && DummyOutputFileName != string.Empty)
            {
                outputFiles.Add(DummyOutputFileName);
            }

            return outputFiles;
        }

        private List<string> GetInputFiles()
        {
            return GetFileListFromFileSet(Inputs);
        }

        private void ExecuteCode()
        {
            try
            {
                // run code (taken from Target.cs in nant)
                foreach (XmlNode node in _codeElement)
                {
                    if (node.NodeType == XmlNodeType.Element)
                    {
                        Task task = Project.CreateTask(node, null);
                        if (task != null)
                        {
                            task.Execute();
                        }
                    }
                }
            }
            catch (BuildException e)
            {
                throw new BuildException("", Location.UnknownLocation, e);
            }
        }

        /// <summary>Execute the task.</summary>
		protected override void ExecuteTask()
		{
            List<string> outputFiles = GetOutputFiles();
            List<string> inputFiles = GetInputFiles();
            List<string> previousInputFiles = new List<string>();

            if (!File.Exists(DependencyFileName))
            {
                Log.Info.WriteLine(LogPrefix + "'{0}' does not exist.", DependencyFileName);
            }
            else
            {
                TextReader dependencyFile = new StreamReader(DependencyFileName);
                previousInputFiles = ReadDependencyFile(dependencyFile);
                dependencyFile.Close();

                if (!TaskNeedsRunning(inputFiles, outputFiles, previousInputFiles))
                {
                    Log.Info.WriteLine(LogPrefix + "up to date.");
                    return;
                }
            }

            // touch dummy output file
            bool dummyOutputFileSpecified = (DummyOutputFileName != null && DummyOutputFileName != string.Empty);
            if (dummyOutputFileSpecified)
            {
                // borrowed from the <touch> task
                Directory.CreateDirectory(Path.GetDirectoryName(DummyOutputFileName));
                using (FileStream fs = File.Create(DummyOutputFileName))
                {
                }
                File.SetLastWriteTime(DummyOutputFileName, DateTime.Now);
            }

            try
            {
                // run code
                ExecuteCode();

                // write dependency file
                Directory.CreateDirectory(Path.GetDirectoryName(DependencyFileName));
                TextWriter dependencyFile = new StreamWriter(DependencyFileName);
                WriteDependencyFile(dependencyFile, inputFiles);
                dependencyFile.Close();
            }
            catch
            {
                Log.Error.WriteLine(LogPrefix + "failed.");

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
