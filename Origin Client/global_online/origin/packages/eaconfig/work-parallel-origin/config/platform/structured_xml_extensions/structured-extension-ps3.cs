using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.Eaconfig.Structured;

namespace EA.Eaconfig.Structured
{
    [TaskName("structured-extension-ps3")]
    class StructuredExtensionXenon : PlatformExtensionBase
    {
        /// <summary>
        /// image builder options.
        /// </summary>
        [Property("debugging")]
        public PS3Debugging Debugging
        {
            get 
            { 
                return _debugging; 
            }
        }
        private PS3Debugging _debugging = new PS3Debugging();




        protected override void ExecuteTask()
        {
            SetDebugSettings();
        }

        private void SetDebugSettings()
        {

            var commandargs = new StringBuilder();

            if (!String.IsNullOrWhiteSpace(Debugging.ExeArguments))
            {
                commandargs.AppendLine("DbgElfArgs="+Debugging.ExeArguments.TrimWhiteSpace());
            }

            if (!String.IsNullOrWhiteSpace(Debugging.DebuggerCmd))
            {
                commandargs.AppendLine("DbgCmdLine="+Debugging.DebuggerCmd.TrimWhiteSpace());
            }

            if (!String.IsNullOrWhiteSpace(Debugging.RunCmd))
            {
                commandargs.AppendLine("RunCmdLine="+Debugging.RunCmd.TrimWhiteSpace());
            }
            if (commandargs.Length > 0)
            {
                Module.SetModuleProperty("commandargs", commandargs.ToString());
            }

            var workingdir = new StringBuilder();

            if (!String.IsNullOrWhiteSpace(Debugging.HomeDir))
            {
                workingdir.AppendLine("HomeDir=" + Debugging.HomeDir.TrimWhiteSpace());
            }

            if (!String.IsNullOrWhiteSpace(Debugging.FileServingDir))
            {
                workingdir.AppendLine("FileServDir=" + Debugging.FileServingDir.TrimWhiteSpace());
            }

            if (workingdir.Length > 0)
            {
                Module.SetModuleProperty("workingdir", workingdir.ToString());
            }
        }
    }

    public class PS3Debugging : Element
    {
        [TaskAttribute("executable-arguments")]
        public String ExeArguments
        {
            get { return _exeArgs; }
            set { _exeArgs = value; }
        }
        private String _exeArgs;

        [TaskAttribute("debugger-command-line")]
        public String DebuggerCmd
        {
            get { return _debuggerCmd; }
            set { _debuggerCmd = value; }
        }
        private String _debuggerCmd;

        [TaskAttribute("run-command-line")]
        public String RunCmd
        {
            get { return _runCmd; }
            set { _runCmd = value; }
        }
        private String _runCmd;

        [TaskAttribute("homedir")]
        public String HomeDir
        {
            get { return _homedir; }
            set { _homedir = value; }
        }
        private String _homedir;

        [TaskAttribute("fileservdir")]
        public String FileServingDir
        {
            get { return _fileservdir; }
            set { _fileservdir = value; }
        }
        private String _fileservdir;


    }
}
