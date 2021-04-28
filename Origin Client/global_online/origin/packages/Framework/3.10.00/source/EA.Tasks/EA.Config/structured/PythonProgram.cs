using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using System;

namespace EA.Eaconfig.Structured
{
    /// <summary>A program that runs through a python interpreter</summary>
    [TaskName("PythonProgram")]
    public class PythonProgramTask : ModuleBaseTask
    {
        public PythonProgramTask()
            : this("PythonProgram")
        {
        }

        protected PythonProgramTask(string buildtype)
            : base(buildtype)
        {
        }

        /// <summary>The python file where the programs execution starts.</summary>
        [TaskAttribute("startupfile", Required = false)]
        public string StartupFile
        {
            get { return _startupfile; }
            set { _startupfile = value; }
        }private string _startupfile = String.Empty;

        /// <summary>Is this project a windows application, defaults to false.</summary>
        [TaskAttribute("windowsapp", Required = false)]
        public string WindowsApp
        {
            get { return _windowsapp; }
            set { _windowsapp = value; }
        }private string _windowsapp = "false";

        /// <summary>Sets the working directory for this project.</summary>
        [TaskAttribute("workdir", Required = false)]
        public string WorkDir
        {
            get { return _workdir; }
            set { _workdir = value; }
        }private string _workdir = ".";

        /// <summary>The projects home directory, search paths and the startup file path
        /// need to be relative to this directory. By default it is the directory containing the
        /// visual studio project files.</summary>
        [TaskAttribute("projecthome", Required = false)]
        public string ProjectHome
        {
            get { return _projecthome; }
            set { _projecthome = value; }
        }private string _projecthome = ".";

        /// <summary>Adds the list of sourcefiles.</summary>
        [FileSet("sourcefiles", Required = false)]
        public StructuredFileSet SourceFiles
        {
            get { return _sourcefiles; }

        }private StructuredFileSet _sourcefiles = new StructuredFileSet();

        /// <summary>Adds the list of contentfiles.</summary>
        [FileSet("contentfiles", Required = false)]
        public StructuredFileSet ContentFiles
        {
            get { return _contentfiles; }

        }private StructuredFileSet _contentfiles = new StructuredFileSet();

        /// <summary>A semicolon separated list of directories that will be added to the Search Path.</summary>
        [Property("searchpaths", Required = false)]
        public PropertyElement SearchPaths
        {
            get { return _searchpaths; }

        }private PropertyElement _searchpaths = new PropertyElement();

        protected override void SetupModule() 
        {
            SetModuleProperty("startupfile", StartupFile, false);
            SetModuleProperty("windowsapp", WindowsApp, false);
            SetModuleProperty("workdir", WorkDir, false);
            SetModuleProperty("projecthome", ProjectHome, false);

            SetModuleFileset("sourcefiles", SourceFiles);
            SetModuleFileset("contentfiles", ContentFiles);
            SetModuleProperty("searchpaths", SearchPaths, false);
        }

        protected override void InitModule() { }

        protected override void FinalizeModule() { }
    }
}