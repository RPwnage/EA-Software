// (c) Electronic Arts. All Rights Reserved.
//-----------------------------------------------------------------------------
// GenerateBulkBuildFiles.cs
//
// NAnt custom task which creates a bulkbuild '.cpp' file for every given
// input fileset.
//
// Required:
//	FileSetName    - Name of fileset containing files to be added to the bulk 'unit'.
//  OutputFilename - Filename for generated CPP.
//
// Optional
//	OutputDir	- This is where the BuildBuild files get generated to.
//
// (c) 2006 Electronic Arts, Inc.
//-----------------------------------------------------------------------------

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

using System;
using System.Collections;
using System.IO;
using System.Text;
using System.Reflection;
using EA.Eaconfig;


namespace EA.GenerateBulkBuildFiles
{
    [TaskName("GenerateBulkBuildFiles")]
    public class generatebulkbuildfiles : Task
    {
        string _fileSetName = null;
        string _outputFilename = null;
        string _outputdir = null;
        string _maxSize = null;
        FileSet _outputBulkBuildFiles = new FileSet();

        int _maxSizeInt = -1;

        [TaskAttribute("FileSetName", Required = true)]
        public string FileSetName
        {
            get { return _fileSetName; }
            set { _fileSetName = value; }
        }

        [TaskAttribute("OutputFilename", Required = true)]
        public string OutputFilename
        {
            get { return _outputFilename; }
            set { _outputFilename = value; }
        }

        [TaskAttribute("OutputDir", Required = false)]
        public string OutputDir
        {
            get { return _outputdir; }
            set { _outputdir = value; }
        }

        [TaskAttribute("MaxSize", Required = false)]
        public string MaxSize
        {
            get { return _maxSize; }
            set             
            {
                _maxSize = StringUtil.Trim(value); 

                if(!Int32.TryParse(_maxSize, out _maxSizeInt))
                {
                    _maxSizeInt = -1;
                }
            }
        }

        [FileSet("output-builkbuild-files", Required = false)]
        public FileSet OutputBulkBuildFiles
        {
            get { return _outputBulkBuildFiles; }
        }

        public static FileSet Execute(Project project, string fileSetName, string outputFilename, string dir, string maxBulkBuildSize)
        {
            // find a task with the given name
            Task task = TaskFactory.Instance.CreateTask("GenerateBulkBuildFiles", project);

            task.Project = project;
            task.GetType().InvokeMember("FileSetName", BindingFlags.SetProperty | BindingFlags.Public | BindingFlags.Instance, null, task, new object[] { fileSetName });
            task.GetType().InvokeMember("OutputFilename", BindingFlags.SetProperty | BindingFlags.Public | BindingFlags.Instance, null, task, new object[] { outputFilename });            
            if (!string.IsNullOrEmpty(dir))
            {
                task.GetType().InvokeMember("OutputDir", BindingFlags.SetProperty | BindingFlags.Public | BindingFlags.Instance, null, task, new object[] { dir });
            }
            if (null != task.GetType().GetProperty("MaxSize"))
            {
                task.GetType().InvokeMember("MaxSize", BindingFlags.SetProperty | BindingFlags.Public | BindingFlags.Instance, null, task, new object[] { maxBulkBuildSize });
            }
            task.Execute();

            FileSet result = null;

            if (null != task.GetType().GetProperty("OutputBulkBuildFiles"))
            {
                result = (FileSet)task.GetType().InvokeMember("OutputBulkBuildFiles", BindingFlags.GetProperty | BindingFlags.Public | BindingFlags.Instance, null, task, null);
            }
            else
            {
                result = new FileSet();
                FileSetUtil.FileSetInclude(result, dir + Path.DirectorySeparatorChar + outputFilename);
            }
            return result;
        }


        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            // This is where the output BulkBuild files get generated to.
            string buildDir = Project.Properties["package.builddir"];

            if (String.IsNullOrEmpty(_outputdir) && buildDir != null)
            {
                _outputdir = buildDir;
            }

            if (String.IsNullOrEmpty(_outputdir))
            {
                _outputdir = System.Environment.CurrentDirectory;
            }
            else
            {
                _outputdir = PathNormalizer.Normalize(_outputdir);
            }

            // Create the output directory if it doesn't exist.
            if (!System.IO.Directory.Exists(_outputdir))
            {
                System.IO.Directory.CreateDirectory(_outputdir);
            }

            int splitindex = 0;
            int count = 0;

            FileSet currentfileset = Project.NamedFileSets[FileSetName];

            // Write out the file contents to a string. 
            StringBuilder fileoutput = new StringBuilder("//-- AutoGenerated BulkBuild File.\n");

            string lastOptionSetName = null;
            foreach (FileItem fi in currentfileset.FileItems)
            {
                if (count > 0 && lastOptionSetName != fi.OptionSetName)
                {
                    fileoutput.AppendLine("//-- AutoGenerated BulkBuild File.");
                    UpdateBulkBuildFile(fileoutput, lastOptionSetName, ++splitindex);

                    fileoutput = new StringBuilder("//-- AutoGenerated BulkBuild File.\n");
                    count = 0;
                }

                fileoutput.AppendLine("#include \"" + fi.FileName + "\"");

                count++;
                if (_maxSizeInt > 0 && count >= _maxSizeInt)
                {
                    fileoutput.AppendLine("//-- AutoGenerated BulkBuild File.");
                    UpdateBulkBuildFile(fileoutput, fi.OptionSetName, ++splitindex);

                    fileoutput = new StringBuilder("//-- AutoGenerated BulkBuild File.\n");
                    count = 0;
                }

                lastOptionSetName = fi.OptionSetName;
            }

            if (count > 0)
            {
                fileoutput.AppendLine("//-- AutoGenerated BulkBuild File.");
                if (splitindex > 0)
                    ++splitindex;
                UpdateBulkBuildFile(fileoutput, lastOptionSetName, splitindex);
            }
        }

        private void UpdateBulkBuildFile(StringBuilder fileoutput, string optionSetName, int autosplitInd)
        {
            string outputfilenameFullPath;
            string outputFilename = GetBBFileName(autosplitInd);

            if ((_outputdir[_outputdir.Length - 1] == Path.DirectorySeparatorChar) || (_outputdir[_outputdir.Length - 1] == Path.AltDirectorySeparatorChar))
            {
                outputfilenameFullPath = _outputdir + outputFilename;
            }
            else
            {
                outputfilenameFullPath = _outputdir + Path.DirectorySeparatorChar + outputFilename;
            }

            // Do not want to touch the Generated BulkBuild file unless the fileset changes.
            string oldfilecontents = "";
            if (System.IO.File.Exists(outputfilenameFullPath))
            {
                System.IO.StreamReader sr = new System.IO.StreamReader(outputfilenameFullPath);
                oldfilecontents = sr.ReadToEnd();
                sr.Close();
            }
            string newOutput = fileoutput.ToString();
            if (oldfilecontents != newOutput)
            {
                System.IO.StreamWriter sw = new System.IO.StreamWriter(outputfilenameFullPath);
                sw.Write(newOutput);
                sw.Close();

                System.Console.WriteLine("{0}Updating '{1}.cpp'", LogPrefix, System.IO.Path.GetFileNameWithoutExtension(outputFilename));
            }

            if (optionSetName == null)
                FileSetUtil.FileSetInclude(_outputBulkBuildFiles, outputfilenameFullPath);
            else
                FileSetUtil.FileSetInclude(_outputBulkBuildFiles, outputfilenameFullPath, _outputBulkBuildFiles.BaseDirectory, optionSetName, false);
        }

        private string GetBBFileName(int index)
        {
            if (index < 1)
                return OutputFilename;

            return String.Format("{0}.Auto{1}{2}", Path.GetFileNameWithoutExtension(OutputFilename), index, Path.GetExtension(OutputFilename));
        }
    }
}
