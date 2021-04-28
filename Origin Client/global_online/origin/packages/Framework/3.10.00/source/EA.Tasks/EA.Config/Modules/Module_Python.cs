using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Modules
{

    public class Module_Python : ProcessableModule
    {
        internal Module_Python(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildtype, IPackage package)
            : base(name, groupname, groupsourcedir, configuration, buildgroup, buildtype, package)
        {
            SetType(Module.Python);

            // python projects require platform to be Any CPU, otherwise visual studio displays errors
            package.Project.Properties.Add("visualstudio.platform.name", "Any CPU");
        }

        public Module_Python(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, IPackage package)
            : this(name, groupname, groupsourcedir, configuration, buildgroup, new BuildType("PythonProgram", "PythonProgram", String.Empty, false, false), package)
        {
        }

        public override void Apply(IModuleProcessor processor)
        {
            processor.Process(this);
        }

        public PathString OutputFile;

        public string SearchPath;
        public string StartupFile;
        public string WorkDir;
        public string ProjectHome;
        public string IsWindowsApp;

        public PythonInterpreter Interpreter
        {
            get { return _interpreter; }
            set
            {
                SetTool(value);
                _interpreter = value;
            }
        } private PythonInterpreter _interpreter;
    }
}