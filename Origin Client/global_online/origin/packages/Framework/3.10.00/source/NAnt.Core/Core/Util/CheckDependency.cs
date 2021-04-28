using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core.Logging;

namespace NAnt.Core.Util
{
    public class CheckDependency
    {
        public static CheckDependency Execute(Project project, List<PathString> inputs, List<PathString> outputs, PathString dependencyFile = null, PathString dummyOutputFile = null)
        {
            CheckDependency worker = new CheckDependency(project.Log, inputs, outputs, dependencyFile, dummyOutputFile);

            worker.Check();

            return worker;

        }

        public bool IsUpToDate
        {
            get
            {
                return _isUpToDate;
            }
        }


        public void Update()
        {
            Touch.File(DummyOutputFile);

            if (DependencyFile != null && !String.IsNullOrEmpty(DependencyFile.Path))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(DependencyFile.Path));

                using (TextWriter dependencyFile = new StreamWriter(DependencyFile.Path))
                {
                    //WriteDependencyFile(dependencyFile, inputFiles);
                    dependencyFile.Close();
                }
            }
        }


        private CheckDependency(Log log, List<PathString> inputs, List<PathString> outputs, PathString dependencyFile, PathString dummyOutputFile)
        {
            _isUpToDate = false;
            Log = log;
            Inputs = inputs;
            Outputs = outputs;
            DependencyFile = dependencyFile;
            DummyOutputFile = dummyOutputFile;
        }


		private void Check()
		{
            // force rebuild if either input or output list is empty
            if (Inputs == null || Inputs.Count < 1)
            {
                _isUpToDate = false;
                return;
            }
            if (Outputs == null || Outputs.Count < 1)
            {
                _isUpToDate = false;
                return;
            }

            // force rebuild if dependency list has changed
            if (InputDependencyListChanged(Inputs, ReadDependencyFile()))
            {
                _isUpToDate = false;
                return;
            }

            // earliest output file:
            DateTime oldestOutputTime = DateTime.MaxValue;
            PathString oldestFile = null;

            foreach (var outputFile in Outputs)
            {
                FileInfo fi = new FileInfo(outputFile.Path);
                if (!fi.Exists)
                {
                    _isUpToDate = false;
                    _reason.AppendFormat("'{0}' does not exist.{1}", outputFile.Path, Environment.NewLine);
                    return;
                }

                // rebuild if outputDateStamp is older than any input file
                DateTime outputDateStamp = fi.LastWriteTime;

                if (outputDateStamp < oldestOutputTime)
                {
                    oldestOutputTime = outputDateStamp;
                    oldestFile = outputFile;
                }
            }

            foreach (var input in Inputs)
            {
                if (oldestOutputTime <= File.GetLastWriteTime(input.Path))
                {
                    _isUpToDate = false;
                    _reason.AppendFormat("'{0}' is newer than '{1}'.{2}",input.Path, oldestFile.Path, Environment.NewLine);                    
                    return;
                }
            }
            _isUpToDate = true;
        }


        /// <summary>Reads list of files from dependency file</summary>
        /// <returns>A sorted List containing all files listed in the dependency file</returns>
        private List<PathString> ReadDependencyFile()
        {
            if(DependencyFile == null || String.IsNullOrEmpty(DependencyFile.Path))
            {
                return null;
            }

            if (!File.Exists(DependencyFile.Path))
            {
                Log.Info.WriteLine("[checkdependency] '{0}' does not exist.", DependencyFile.Path);
                return null;
            }

            List<PathString> files = new List<PathString>();
            string line;
            using (TextReader dependencyFile = new StreamReader(DependencyFile.Path))
            {
                while ((line = dependencyFile.ReadLine()) != null)
                {
                    line = line.TrimEnd(new char[] { '\r', '\n' });
                    if (line.Length > 0)
                    {
                        files.Add(new PathString(line, PathString.PathState.Normalized|PathString.PathState.FullPath));
                    }
                }
            }
            files.Sort();
            return files;
        }


        /// <summary>Writes list of files to a dependency file</summary>
        private static void WriteDependencyFile(TextWriter dependencyFile, List<PathString> files)
        {
            foreach (var file in files)
            {
                if (!String.IsNullOrEmpty(file.Path))
                {
                    dependencyFile.WriteLine(file.Path);
                }
            }
        }

        /// <summary>Determine if the current and previous list of input files differ</summary>
        /// <returns><c>true</c> if the lists are not identical</returns>
        private bool InputDependencyListChanged(List<PathString> inputFiles, List<PathString> previousInputFiles)
        {
            if (previousInputFiles.Count != inputFiles.Count)
            {
                return true;
            }

            previousInputFiles.Sort();
            inputFiles.Sort();

            if (!SortedListsEqual(previousInputFiles, inputFiles))
            {
                _reason.AppendLine("Input dependency list changed.");
                return true;
            }

            return false;
        }

        private static bool SortedListsEqual(List<PathString> listA, List<PathString> listB)
        {
            if (listA.Count != listB.Count)
            {
                return false;
            }

            var a = listA.GetEnumerator();
            var b = listB.GetEnumerator();
            while (a.MoveNext() && b.MoveNext())
            {
                if (a.Current != a.Current)
                    return false;
            }
            return true;
        }


        private bool _isUpToDate = false;
        private StringBuilder _reason = new StringBuilder();

        private readonly Log Log;
        private readonly List<PathString> Inputs;
        private readonly List<PathString> Outputs;
        private readonly PathString DependencyFile;
        private readonly PathString DummyOutputFile;
    }
}
