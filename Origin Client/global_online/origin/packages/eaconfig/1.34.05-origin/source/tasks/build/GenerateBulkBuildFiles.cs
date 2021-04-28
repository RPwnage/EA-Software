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

                bool isPortable = false;

                {
                    string prop = Project.Properties["eaconfig.generate-portable-solution"];
                    if (!string.IsNullOrEmpty(prop))
                    {
                        bool result;
                        if (Boolean.TryParse(prop, out result))
                        {
                            isPortable = result;
                        }
                    }
                }

                if (isPortable)
                    fileoutput.AppendLine("#include \"" + GetRelativeFileName(fi.FileName, _outputdir) + "\"");
                else
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

        // GetRelativeFileName() is taken from nantToVSTools and should be kept in
        // sync with that package until nantToVSTools is retired.
        private const int MAX_PATH_LENGTH = 256;
        private static readonly char AltDirectorySeparatorChar = Path.DirectorySeparatorChar == '\\' ? '/' : '\\';

        private static readonly char[] PATH_SEPARATOR_CHARS = new char[] { Path.DirectorySeparatorChar };
        private static readonly char[] TRIM_CHARS = new char[] { '\"', '\n', '\r', '\t', ' ' };
        private static readonly string TO_PARENT_S = ".." + Path.DirectorySeparatorChar;

        private static string SEP_STRING = Path.DirectorySeparatorChar.ToString();
        private static string SEP_DOUBLE_STRING = Path.DirectorySeparatorChar.ToString() + Path.DirectorySeparatorChar.ToString();

        private static string GetRelativeFileName(string fileName, string basePath)
        {
            if (fileName == null || basePath == null)
            {
                throw new BuildException("File Name and/or Base Path can't be null");
            }

            fileName = TrimBlanks(fileName);
            basePath = TrimBlanks(basePath);
            if (basePath == String.Empty)
            {
                throw new BuildException("Base Path not specified");
            }
            if (fileName == String.Empty)
            {
                throw new BuildException("File Name not specified");
            }
            basePath = Normalize(basePath);
            if (basePath[0] == Path.DirectorySeparatorChar)
            {
                // rooted, but missing the drive - place it on current drive
                basePath = Path.GetFullPath(basePath);
            }
            else if (basePath != Path.GetFullPath(basePath))
            {
                string msg = String.Format("Base Path '{0}' must be an absolute path, did you mean '{1}'?",
                    basePath, Path.GetFullPath(basePath));
                throw new BuildException(msg);
            }

            // getFullPath=false:
            // if the filename is in relative format, assume that it's relative to the base,
            // not to the current directory
            fileName = Normalize(fileName);
            if (fileName[0] == Path.DirectorySeparatorChar && !Path.IsPathRooted(fileName))
            {
                // rooted, but missing the drive - place it on basePath drive
                fileName = basePath.Substring(0, 2) + fileName;
                if (fileName.IndexOf("/U/") > -1) throw new BuildException(fileName);
            }
            else if (!Path.IsPathRooted(fileName))
            {
                fileName = (basePath[basePath.Length - 1] == Path.DirectorySeparatorChar) ?
                            basePath + fileName :
                            basePath + Path.DirectorySeparatorChar + fileName;
            }

            string[] file_list = fileName.Split(Path.DirectorySeparatorChar);
            string[] base_list = basePath.Split(Path.DirectorySeparatorChar);
            int fLen = file_list.Length;
            // if the file ended in '\', we get a final empty string
            //if (file_list[fLen-1] == String.Empty)
            //	fLen--;
            int bLen = base_list.Length;
            if (base_list[bLen - 1] == String.Empty)
                bLen--;

            // Handle DOS names that are on different drives:
            if (String.Compare(file_list[0], base_list[0], true) != 0)
            {
                // not on the same drive, so only absolute fileName will do
                // Log.WriteLine("WARNING: the file {0} is located on a different drive compared to {1}", fileName, basePath);
                return fileName;
            }

            // Work out how much of the filepath is shared by base and fileName.
            // i = 1 - skip the drive name
            int i;
            for (i = 1; i < fLen && i < bLen && String.Compare(file_list[i], base_list[i], true) == 0; i++)
                ;
            // i is pointing to the first differing path elements
            StringBuilder sb = new StringBuilder(MAX_PATH_LENGTH);
            for (int j = 0; j < bLen - i; j++)
            {
                sb.Append(TO_PARENT_S);
            }
            sb.Append(String.Join(Path.DirectorySeparatorChar.ToString(), file_list, i, fLen - i));
            return sb.ToString();
        }

        /// <summary>
        /// Trim white space, quotation marks and new lines from a given string
        /// </summary>
        public static string TrimBlanks(string inS)
        {
            return inS.Trim(TRIM_CHARS);
        }

        /// <summary>
        // Normalize the directory separators
        /// </summary>
        public static string Normalize(string path)
        {
            try
            {
                // normalize drive letter
                if (path.Length > 1 && path[1] == Path.VolumeSeparatorChar)
                {
                    string driveLetter = path[0].ToString().ToUpper();
                    path = driveLetter + path.Substring(1);
                }

                // normalize directory separators
                path = path.Replace(AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
                if (path.StartsWith(SEP_DOUBLE_STRING))
                {
                    path = SEP_STRING + path.Replace(SEP_DOUBLE_STRING, SEP_STRING);
                }
                else
                {
                    path = path.Replace(SEP_DOUBLE_STRING, SEP_STRING);
                }

                // convert root drives into directories, d: -> d:\
                if (path.Length == 2 && Path.IsPathRooted(path) && path[1] == Path.VolumeSeparatorChar)
                {
                    path = path + Path.DirectorySeparatorChar;
                }
            }
            catch (System.Exception e)
            {
                string msg = String.Format("Normalize failed: {0}\n{1}\n{2}\n", e.Message, path, e.StackTrace);
                throw new BuildException(msg);
            }

            return path;
        }
    }
}
