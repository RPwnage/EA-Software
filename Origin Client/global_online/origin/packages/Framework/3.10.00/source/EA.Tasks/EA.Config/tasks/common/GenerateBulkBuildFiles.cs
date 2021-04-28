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
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.IO;
using System.Text;
using System.Reflection;
using System.Linq;
using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;


namespace EA.GenerateBulkBuildFiles
{
    [TaskName("GenerateBulkBuildFiles")]
    public class generatebulkbuildfiles : Task
    {
        string _fileSetName = null;
        string _outputFilename = null;
        string _outputdir = null;
        string _maxSize = null;
        string _pchHeader = null;
        string _bulkBuildFileHeader = null;
        FileSet _outputBulkBuildFiles = new FileSet();
        FileSet _outputLooseFiles = new FileSet();
        
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
                _maxSize = Eaconfig.StringUtil.Trim(value); 

                if(!Int32.TryParse(_maxSize, out _maxSizeInt))
                {
                    _maxSizeInt = -1;
                }
            }
        }

        [TaskAttribute("pch-header", Required = false)]
        public string PchHeader
        {
            get { return _pchHeader; }
            set { _pchHeader = value; }
        }



        [FileSet("output-bulkbuild-files", Required = false)]
        public FileSet OutputBulkBuildFiles
        {
            get { return _outputBulkBuildFiles; }
        }

        [FileSet("output-loose-files", Required = false)]
        public FileSet OutputLooseFiles
        {
            get { return _outputLooseFiles; }
        }


        public static FileSet Execute(Project project, string fileSetName, string outputFilename, string dir, string maxBulkBuildSize, string pchheader, out FileSet looseFiles)
        {
            // find a task with the given name
            Task task = project.TaskFactory.CreateTask("GenerateBulkBuildFiles", project);

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
            if (!String.IsNullOrWhiteSpace(pchheader) && null != task.GetType().GetProperty("PchHeader"))
            {
                task.GetType().InvokeMember("PchHeader", BindingFlags.SetProperty | BindingFlags.Public | BindingFlags.Instance, null, task, new object[] { pchheader });
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

            if (null != task.GetType().GetProperty("OutputLooseFiles"))
            {
                looseFiles = (FileSet)task.GetType().InvokeMember("OutputLooseFiles", BindingFlags.GetProperty | BindingFlags.Public | BindingFlags.Instance, null, task, null);
            }
            else
            {
                looseFiles = null;
            }
            
            result.SetFileDataFlags(FileData.BulkBuild);
            
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

            _bulkBuildautoSplitTemplate = PropertyUtil.GetPropertyOrDefault(Project, "bulkbuild.autosplit.template", "%filename%.Auto%index%%ext%");


            // Create the output directory if it doesn't exist.
            if (!System.IO.Directory.Exists(_outputdir))
            {
                System.IO.Directory.CreateDirectory(_outputdir);
            }

            string bulkbuildtemplate = (Project.Properties["bulkbuild.template"] ?? "#include \"%filename%\"").TrimWhiteSpace();

            string bulkbuildHeaderTemplate = Project.Properties["bulkbuild.header.template"].TrimWhiteSpace();
            if (!String.IsNullOrEmpty(bulkbuildHeaderTemplate) && !String.IsNullOrWhiteSpace(_pchHeader))
            {
                _bulkBuildFileHeader = bulkbuildHeaderTemplate.Replace("%pchheader%", _pchHeader.TrimWhiteSpace());
            }

            FileSet currentfileset = Project.NamedFileSets[FileSetName];

            _outputBulkBuildFiles.BaseDirectory = _outputdir; 
            _outputLooseFiles.BaseDirectory = currentfileset.BaseDirectory;

            bool isPortable = Project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false);
            bool isGeneration = Project.Properties.GetBooleanPropertyOrDefault("eaconfig.build.process-generation-data", false);

            var filegroups = currentfileset.FileItems.GroupBy(fi => fi.OptionSetName);

            int groupindex = 0;

            foreach (var group in filegroups)
            {
#if FRAMEWORK_PARALLEL_TRANSITION
                // The old eaconfig let you do this
                if (group.Count() == 1)
                {
                    Project.Log.Warning.WriteLine("Generating a bulk-build file [{0}] with only one include.  Consider changing the build script for package [{1}]", OutputFilename, Project.Properties["package.name"]);
                }
#else
                if (group.Count() == 1)
                {
                    if (Log.WarningLevel >= Log.WarnLevel.Advise)
                    {
                        Project.Log.Warning.WriteLine("Attempting to generate a bulk-build file [{0}] with only one include.  Reverting to a loose file. Consider changing the build script for package [{1}]", OutputFilename, Project.Properties["package.name"]);
                    }
                    var fi = group.FirstOrDefault();
                    if (fi != null)
                    {
                        _outputLooseFiles.Include(fi, failonmissing: false);
                    }
                }
                else if(group.Count() > 0)
#endif
                {
                    // Write out the file contents to a string. 
                    int count = 0;
                    string optionsetname = null;

                    StringBuilder fileoutput = new StringBuilder("//-- AutoGenerated BulkBuild File."+Environment.NewLine);

                    var first_fi = group.FirstOrDefault();
                    if (first_fi != null)
                    {
                        optionsetname = first_fi.OptionSetName;
                    }

                    if (String.IsNullOrEmpty(optionsetname) && !String.IsNullOrEmpty(_bulkBuildFileHeader))
                    {
                        fileoutput.AppendLine(_bulkBuildFileHeader);
                    }



                    foreach (FileItem fi in group)
                    {
                        var filepath = (isPortable && isGeneration) ? PathUtil.RelativePath(fi.FileName, _outputdir) : fi.FileName;

                        fileoutput.AppendLine(bulkbuildtemplate.Replace("%filename%", filepath));

                        count++;
                        if (_maxSizeInt > 0 && count >= _maxSizeInt)
                        {
                            if (groupindex == 0)
                            {
                                ++groupindex; // Start with 1.
                            }
                            fileoutput.AppendLine("//-- AutoGenerated BulkBuild File.");
                            UpdateBulkBuildFile(fileoutput, optionsetname, groupindex);
                            ++groupindex;
                            fileoutput = new StringBuilder("//-- AutoGenerated BulkBuild File."+Environment.NewLine);
                            if (String.IsNullOrEmpty(optionsetname) && !String.IsNullOrEmpty(_bulkBuildFileHeader))
                            {
                                fileoutput.AppendLine(_bulkBuildFileHeader);
                            }

                            count = 0;
                        }
                    }

                    if (count > 0)
                    {
                        if (filegroups.Count() > 1 && groupindex == 0)
                        {
                            groupindex++; //Start with 1 when we have multiple bb files
                        }
                        fileoutput.AppendLine("//-- AutoGenerated BulkBuild File.");
                        UpdateBulkBuildFile(fileoutput, optionsetname, groupindex);
                        ++groupindex;
                    }
                }
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
            if (!String.Equals(oldfilecontents, newOutput, StringComparison.Ordinal))
            {
                System.IO.StreamWriter sw = new System.IO.StreamWriter(outputfilenameFullPath);
                sw.Write(newOutput);
                sw.Close();

                Project.Log.Status.WriteLine("{0}\"{1}/{2}\": Updating '{3}.cpp'", LogPrefix, Project.Properties["package.name"], Project.Properties["build.module"], System.IO.Path.GetFileNameWithoutExtension(outputFilename));
            }

            if (optionSetName == null)
                FileSetUtil.FileSetInclude(_outputBulkBuildFiles, outputfilenameFullPath);
            else
                FileSetUtil.FileSetInclude(_outputBulkBuildFiles, outputfilenameFullPath, _outputBulkBuildFiles.BaseDirectory, optionSetName, true);
        }

        private string GetBBFileName(int index)
        {
            if (index < 1)
                return OutputFilename;

            _bulkBuildautoSplitTemplate = PropertyUtil.GetPropertyOrDefault(Project, "bulkbuild.autosplit.template", "%filename%.Auto%index%%ext%");

            return _bulkBuildautoSplitTemplate
                .Replace("%filename%", Path.GetFileNameWithoutExtension(OutputFilename))
                .Replace("%index%", String.Format("{0:00}", index))
                .Replace("%ext%", Path.GetExtension(OutputFilename));
        }

        private string _bulkBuildautoSplitTemplate;
    }
}
