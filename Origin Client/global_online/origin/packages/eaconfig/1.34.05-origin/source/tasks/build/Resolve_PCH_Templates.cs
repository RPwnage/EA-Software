using NAnt.Core;
using NAnt.Core.Attributes;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    [TaskName("Resolve_PCH_Templates")]
    public class Resolve_PCH_Templates : EAConfigTask
    {

        [TaskAttribute("BuildTypeName", Required = true)]
        public string BuildTypeName
        {
            get { return _buildTypeName; }
            set { _buildTypeName = value; }
        }

        [TaskAttribute("PchFile", Required = true)]
        public string PchFile
        {
            get { return _pchFile; }
            set { _pchFile = value; }
        }

        [TaskAttribute("PchHeaderFile", Required = true)]
        public string PchHeaderFile
        {
            get { return _pchHeaderFile; }
            set { _pchHeaderFile = value; }
        }

        [TaskAttribute("OutputDir", Required = true)]
        public string OutputDir
        {
            get { return _outputDir; }
            set { _outputDir = value; }
        }

        [TaskAttribute("FileSetName", Required = true)]
        public string FileSetName
        {
            get { return _fileSetName; }
            set { _fileSetName = value; }
        }

        public FileSet FileSet
        {
            get { return _fileSet; }
        }


        public static void Execute(Project project, OptionSet buildOptionSet, string pchFile, string pchHeaderFile, string outputDir, FileSet fileSet)
        {
            Resolve_PCH_Templates task = new Resolve_PCH_Templates(project, buildOptionSet, pchFile, pchHeaderFile, outputDir, fileSet);
            task.ExecuteTask();
        }

        public Resolve_PCH_Templates(Project project, OptionSet buildOptionSet, string pchFile, string pchHeaderFile, string outputDir, FileSet fileSet)
            : base(project)
        {
            if (buildOptionSet== null)
            {
                Error.Throw(Project, Location, "Resolve_PCH_Templates", "'BuildOptionSet' parameter is null.");
            }

            if (fileSet== null)
            {
                Error.Throw(Project, Location, "Resolve_PCH_Templates", "'FileSet' parameter is null.");
            }

            _buildOptionSet = buildOptionSet;
            PchFile = pchFile;
            PchHeaderFile = pchHeaderFile;
            OutputDir    = outputDir;
            _fileSet = fileSet;

        }

        public Resolve_PCH_Templates()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            if (_fileSet == null && !String.IsNullOrEmpty(FileSetName))
            {
                _fileSet = Project.NamedFileSets[FileSetName];
            }
            if (_buildOptionSet == null && !String.IsNullOrEmpty(BuildTypeName))
            {
                _buildOptionSet = Project.NamedOptionSets[BuildTypeName];
            }
            

            System.Collections.Specialized.StringCollection names =
                                             new System.Collections.Specialized.StringCollection();

            Replaceoptionset(_buildOptionSet, PchHeaderFile, PchFile, null, names);

            if (_fileSet != null && !String.IsNullOrEmpty(PchFile))
            {
                 foreach (FileSetItem fi in FileSet.Includes)
                {
                    Replaceoptionset(fi.OptionSet, PchHeaderFile, PchFile, OutputDir, names);
                }
            }
        }



        private void Replaceoptionset(string optionsetName, string pchHeaderFile, string pchFile, string outputDir, System.Collections.Specialized.StringCollection names)
        {
            if (optionsetName != null && !names.Contains(optionsetName))
            {
                Replaceoptionset(Project.NamedOptionSets[optionsetName], pchHeaderFile, pchFile, outputDir, names);
                names.Add(optionsetName);
            }
        }

        private void Replaceoptionset(OptionSet optionSet, string pchHeaderFile, string pchFile, string outputDir, System.Collections.Specialized.StringCollection names)
        {
            if (optionSet != null)
            {
                string ccOptions = optionSet.Options["cc.options"];
                if (!String.IsNullOrEmpty(ccOptions))
                {
                    if (String.IsNullOrEmpty(pchHeaderFile))
                    {
                        ccOptions = ccOptions.Replace("\"%pchheaderfile%\"", String.Empty);
                    }
                    else
                    {
                        ccOptions = ccOptions.Replace("%pchheaderfile%", NAnt.Core.Util.PathNormalizer.Normalize(NAnt.Core.Util.PathNormalizer.StripQuotes(pchHeaderFile), false));
                    }
                    pchFile = NAnt.Core.Util.PathNormalizer.Normalize(NAnt.Core.Util.PathNormalizer.StripQuotes(pchFile), false);
                    ccOptions = ccOptions.Replace("%pchfile%", pchFile);
                    if (!String.IsNullOrEmpty(outputDir))
                    {
                        ccOptions = ccOptions.Replace("%outputdir%", NAnt.Core.Util.PathNormalizer.Normalize(NAnt.Core.Util.PathNormalizer.StripQuotes(outputDir), false));
                    }
                    optionSet.Options["cc.options"] = ccOptions;

                    string createPch = optionSet.Options["create-pch"];
                    if (createPch != null && createPch == "on")
                    {
                        //Assign this option to use in nant compiler task  dependency 
                        optionSet.Options["pch-file"] = pchFile;
                    }
                }
            }
        }


        private string _buildTypeName;
        private string _pchFile;
        private string _pchHeaderFile;
        private string _outputDir;
        private string _fileSetName;

        FileSet   _fileSet;
        OptionSet _buildOptionSet; 

    }
}
