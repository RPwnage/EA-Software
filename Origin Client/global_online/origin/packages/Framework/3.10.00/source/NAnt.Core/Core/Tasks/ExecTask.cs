// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// File Maintainer:
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks
{

    /// <summary>Executes a system command.</summary>
    /// <remarks>
    /// <para>Any options in the <c>env</c> option set get added as environment variables when
    /// the program is run.</para>
    ///
    /// <para>You can use <![CDATA[<arg value="switch"/> or <arg file="filename.txt"/>]]> to
    /// specify command line options in a more manageable way. The values in these elements get
    /// appended to whatever value is in the commandline attribute if that attribute exists.
    /// File values specified using the <c>file</c> attribute get expanded to their full paths.</para>
    ///
    /// <para>The <c>inputs</c> and <c>outputs</c> file sets allow you to perform dependency checking.
    /// The <c>program</c> will only get executed if one of the inputs is newer than all of the
    /// outputs.</para>
    /// 
    /// <para>After each <see cref="ExecTask"/> task is completed the read-only property <c>${exec.exitcode}</c> 
    /// is set to the value that the process specified when it terminated. The default value is -1 and 
    /// is specified when the process fails to return a valid value.</para>
    ///
    /// <para>Note: By default errors will not be generated if files are missing from the output file
    /// set.  Normally filesets will throw a build exception if a specifically named file does not
    /// exist.  This behaviour is controlled by the <c>failonmissing</c> file set attribute, which
    /// defaults to <c>true</c>.  If set to <c>false</c>, no build exception will be thrown when a
    /// specifically named file is not found.</para>
    ///
    /// <para>Note: Dependency checking requires at least one file in the <c>inputs</c> fileset and one
    /// file in the <c>outputs</c> fileset.  If either of these filesets is empty, no dependency
    /// checking will be performed.</para>
    /// </remarks>
    /// <include file='Examples/Exec/Simple.example' path='example'/>
    /// <include file='Examples/Exec/OptionSet.example' path='example'/>
    /// <include file='Examples/Exec/DependencyChecking.example' path='example'/>
    [TaskName("exec")]
    public class ExecTask : ExternalProgramBase
    {

        public static readonly string EnvironmentVariablePropertyNamePrefix = "exec.env";
        public static readonly string InputsPropertyNamePrefix = "exec.inputs";
        public static readonly string OutputsPropertyNamePrefix = "exec.outputs";
        public static readonly string ExitCodeProperty = "exec.exitcode";

        private static readonly string ProgramVariable = "%program%";
        private static readonly string CommandLineVariable = "%commandline%";
        private static readonly string ResponseFileVariable = "%responsefile%";

        string _program = null;
        string _message = null;
        string _commandline = null;
        string _workingDirectory = null;
        int _timeout = Int32.MaxValue;
        string _outputFile = null;
        string _responseFile = null;
        string _commandLineTemplate = CommandLineVariable + " " + ResponseFileVariable;
        bool _stdoutAlways = true;
        bool _outputAppend = false;
        FileSet _inputs = new FileSet();
        FileSet _outputs = new FileSet();
        bool _clearEnv = false;
        bool _redirectOut = true;
        bool _redirectIn = true;
        bool _createInWindow = true;
        OptionSet _environmentVariables = new OptionSet();

        public ExecTask()
            : base("exec")
        {
            Inputs.FailOnMissingFile = true;
            Outputs.FailOnMissingFile = false;
        }

        public ExecTask(Project project)
            : base("exec")
        {
            Project = project;
            Inputs.FailOnMissingFile = true;
            Outputs.FailOnMissingFile = false;
        }


        // ExternalProgramBase implementation
        public override string ProgramFileName
        {
            get
            {

                // In the concurrent environment we can't use Environment.CurrentDirectory = Project.GetFullPath(WorkingDirectory);
                // If path is relative path try to find program tn the working dir.
                if (!Path.IsPathRooted(Program))
                {
                    var tryPath = PathNormalizer.Normalize(Path.Combine(Project.GetFullPath(WorkingDirectory), Program), false);
                    if (File.Exists(tryPath))
                    {
                        return tryPath;
                    }
                }
                // if the program is just a file name (no path) than don't get the
                // full path - user is likely calling a system program that is in the path
                if (Path.GetDirectoryName(Program) == "")
                {
                    return PathNormalizer.Normalize(Program, false);
                }
                else
                {
                    // if program has a path, get the full path for it.
                    return Project.GetFullPath(Program);
                }
            }
        }

        public override string ProgramArguments
        {
            get { return _commandline; }
        }

        /// <summary>The program to execute. Specify the fully qualified name unless the program is in the path.</summary>
        [TaskAttribute("program", Required = true)]
        public string Program
        {
            get { return _program; }
            set { _program = PathNormalizer.StripQuotes(value); }
        }

        /// <summary>The message to display to the log.  If present then no other output will be displayed unless there is an error or the verbose attribute is true.</summary>
        [TaskAttribute("message")]
        public string Message
        {
            get { return _message; }
            set { _message = value; }
        }

        /// <summary>The command line arguments for the program.  Consider using <c>arg</c> elements (see task description) for improved readability.</summary>
        [TaskAttribute("commandline")]
        public string Arguments
        {
            get { return _commandline; }
            set { _commandline = value; }
        }

        /// <summary>Use the contents of this file as input to the program.</summary>
        [TaskAttribute("responsefile", Required = false)]
        public string ResponseFile
        {
            get { return _responseFile; }
            set { _responseFile = value; }
        }

        /// <summary>The template used to form the command line. Default is '%commandline% %responsefile%'.</summary>
        [TaskAttribute("commandlinetemplate", Required = false)]
        public string CommandLineTemplate
        {
            get { return _commandLineTemplate; }
            set { _commandLineTemplate = value; }
        }

        /// <summary>The file to which the standard output will be redirected. By default, the standard output is redirected to the console.</summary>
        [TaskAttribute("output", Required = false)]
        public string Output
        {
            get { return _outputFile; }
            set { _outputFile = Project.GetFullPath(value); }
        }

        /// <summary>true if the output file is to be appended to. Default = "false".</summary>
        [TaskAttribute("append", Required = false)]
        public bool Append
        {
            get { return _outputAppend; }
            set { _outputAppend = value; }
        }

        /// <summary>Write to standard output. Default is true.</summary>
        [TaskAttribute("stdout", Required = false)]
        public override bool Stdout
        {
            get { return _stdoutAlways; }
            set { _stdoutAlways = value; }
        }

        /// <summary>Set true to redirect stdout and stderr. When set to false, this will disable options such as stdout, outputfile etc.  Default is true.</summary>
        [TaskAttribute("redirectout")]
        public override bool RedirectOut
        {
            get { return _redirectOut; }
            set { _redirectOut = value; }
        }

        /// <summary>Set true to redirect stdin. Default is true.</summary>
        [TaskAttribute("redirectin")]
        public override bool RedirectIn
        {
            get { return _redirectIn; }
            set { _redirectIn = value; }
        }

        /// <summary>Set true to create process in a new window.. Default is false.</summary>
        [TaskAttribute("createinwindow")]
        public override bool CreateInWindow
        {
            get { return _createInWindow; }
            set { _createInWindow = value; }
        }

        /// <summary>The directory in which the command will be executed. The workingdir will be evaluated relative to the build file's directory.</summary>
        [TaskAttribute("workingdir")]
        public virtual string WorkingDirectory
        {
            get { return Project.GetFullPath(_workingDirectory); }
            set { _workingDirectory = value; }
        }

        /// <summary>Set true to clear all environmental variable before executing, environmental variable specified in env OptionSet will still be added as environmental variable.</summary>
        [TaskAttribute("clearenv")]
        public bool ClearEnv
        {
            get { return _clearEnv; }
            set { _clearEnv = value; }
        }

        /// <summary>The set of environment variables for when the program runs.
        /// Benefit of setting variables, like "Path", here is that it will be
        /// local to this program execution (i.e. global path is unaffected).</summary>
        [OptionSet("env")]
        public OptionSet EnvironmentVariables
        {
            get { return _environmentVariables; }
            set { _environmentVariables = value; }
        }

        public override string OutputFile
        {
            get { return _outputFile; }
        }

        public override bool OutputAppend
        {
            get { return _outputAppend; }
        }

        /// <summary>Stop the build if the command does not finish within the specified time.  Specified in milliseconds.  Default is no time out.</summary>
        [TaskAttribute("timeout")]
        public override int TimeOut
        {
            get { return _timeout; }
            set { _timeout = value; }
        }

        /// <summary>Program inputs.</summary>
        [FileSet("inputs")]
        public FileSet Inputs
        {
            get { return _inputs; }
        }

        /// <summary>Program outputs.</summary>
        [FileSet("outputs")]
        public FileSet Outputs
        {
            get { return _outputs; }
        }

        /// <summary>A set of command line arguments.</summary>
        [ArgumentSet("args", Initialize = false)]
        public ArgumentSet ArgSet
        {
            get { return _argSet; }
        }

        private void AddProperties(string prefix, StringCollection fileNames)
        {
            StringBuilder all = new StringBuilder();
            int index = 0;

            foreach (string fileName in fileNames)
            {
                Project.Properties.Add(Property.Combine(prefix, index.ToString()), fileName);
                all.Append(fileName);
                all.Append(" ");
                index++;
            }
            Project.Properties.Add(Property.Combine(prefix, "all"), all.ToString(), readOnly: false, deferred: false, local: true);
        }

        private void RemoveProperties(string prefix)
        {
            StringCollection toDelete = new StringCollection();
            foreach (Property property in Project.Properties)
            {
                if (property.Prefix == prefix)
                {
                    toDelete.Add(property.Name);
                }
            }
            foreach (string name in toDelete)
                Project.Properties.Remove(name);
            Project.Properties.Remove(Property.Combine(prefix, "all"));
        }

        protected override void InitializeTask(XmlNode node)
        {
            // Iterate over the inputs and output file sets make their contents available as properties
            // in the form of exec.inputs.# and exec.outputs.#
            AddProperties(InputsPropertyNamePrefix, Inputs.FileItems.ToStringCollection());
            AddProperties(OutputsPropertyNamePrefix, Outputs.FileItems.ToStringCollection());

            // The 'args' build element is set to not be initialize be default, meaning we need to 
            // initialize it ourselves. The reason being that we need the above properties to be
            // set prior to initialize it.
            InitializeBuildElement(node, "args");


            // Initialize the task as normal.
            base.InitializeTask(node);
        }

        /// <summary>Determine if the task needs to run.</summary>
        /// <returns><c>true</c> if we should run the program (dependents missing or not up to date), otherwise <c>false</c>.</returns>
        private bool TaskNeedsRunning()
        {
            if (Inputs.FileItems.Count < 1)
            {
                return true;
            }
            if (Outputs.FileItems.Count < 1)
            {
                return true;
            }

            foreach (FileItem fileItem in Outputs.FileItems)
            {
                if (!File.Exists(fileItem.FileName))
                {
                    Log.Info.WriteLine("Running because '{0}' does not exist.", fileItem.FileName);
                    return true;
                }

                DateTime outputDateStamp = File.GetLastWriteTime(fileItem.FileName);
                string newerFileName = FileSet.FindMoreRecentLastWriteTime(Inputs.FileItems.ToStringCollection(), outputDateStamp);
                if (newerFileName != null)
                {
                    // at least one input filename has a last write time more recent than the output file we are checking
                    Log.Info.WriteLine("Running because '{0}' is newer than '{1}'.", newerFileName, fileItem.FileName);
                    return true;
                }
            }
            if (Log.Level >= Logging.Log.LogLevel.Detailed)
            {
                Log.Info.WriteLine(LogPrefix + "Not running {0} {1} because output is up-to-date.", ProgramFileName, GetCommandLine());
            }

            return false;
        }

        string GetResponseFile()
        {
            StringBuilder arguments = new StringBuilder();
            if (ResponseFile != null)
            {
                try
                {
                    using (StreamReader sr = new StreamReader(ResponseFile))
                    {
                        string line;
                        while ((line = sr.ReadLine()) != null)
                        {
                            arguments.Append(line);
                            arguments.Append(" "); // treat newline as whitespace
                        }
                    }
                }
                catch (System.Exception e)
                {
                    throw new BuildException("Error reading response file.", e);
                }
            }
            return arguments.ToString();
        }

        public override string GetCommandLine()
        {
            StringBuilder commandLine = new StringBuilder(CommandLineTemplate);
            commandLine.Replace(CommandLineVariable, base.GetCommandLine());
            commandLine.Replace(ResponseFileVariable, GetResponseFile());

            return commandLine.ToString();
        }

        protected override void ExecuteTask()
        {
            if (!String.IsNullOrEmpty(WorkingDirectory))
            {
                if (Directory.Exists(WorkingDirectory) == false)
                {
                    string msg = String.Format("Could not find working directory {0}", WorkingDirectory);
                    throw new BuildException(msg, Location);
                }
            }

            try
            {
                if (TaskNeedsRunning())
                {
                    DisplayTaskBanner();
                    base.ExecuteTask();
                }

                // Remove any exec.inputs.* and exec.outputs.* properties
                RemoveProperties(InputsPropertyNamePrefix);
                RemoveProperties(OutputsPropertyNamePrefix);
            }
            catch (BuildException e)
            {
                BuildException be = e;
                if (Output != null)
                {
                    string msg = e.BaseMessage + String.Format("  Output was saved at:\n{0}", UriFactory.CreateUri(Output).ToString());
                    be = new BuildException(msg, e.Location);
                }
                throw be;
            }
            catch (Exception e)
            {
                throw new BuildException(e.Message, Location);
            }
            finally
            {
                Project.Properties.UpdateReadOnly(ExitCodeProperty, base.ExitCode.ToString());
            }
        }

        void DisplayTaskBanner()
        {
            if (Message != null)
            {
                string msg = Message;
                msg = msg.Replace(ProgramVariable, ProgramFileName);
                msg = msg.Replace(CommandLineVariable, GetCommandLine());
                Log.Status.WriteLine(LogPrefix + msg);
            }
            if (Message == null || Log.Level >= Logging.Log.LogLevel.Diagnostic)
            {
                Log.Status.WriteLine(LogPrefix + "{0} {1}", ProgramFileName, GetCommandLine());
            }
        }

        protected override void PrepareProcess(ref System.Diagnostics.Process process)
        {
            base.PrepareProcess(ref process);
            if (!String.IsNullOrEmpty(WorkingDirectory))
            {
                process.StartInfo.WorkingDirectory = Project.GetFullPath(WorkingDirectory);
            }

            Dictionary<string, string> envMap = new Dictionary<string, string>();
            if (!ClearEnv)
            {
                Log.Info.WriteLine(LogPrefix + "<exec> {0} without clearing environmental variable.", process.StartInfo.FileName);

                // LEAVE IN FOR BACKWARDS COMPATIBILITY (old school style of passing env vars)
                // Set all the property values in exec.env.* to the Process's environment.
                // This allows people to be pass a specific environment to the program being executed.
                foreach (Property property in Project.Properties)
                {
                    if (property.Prefix == EnvironmentVariablePropertyNamePrefix)
                    {
                        envMap[property.Suffix] = property.Value;
                    }
                }
            }
            else
            {
                process.StartInfo.EnvironmentVariables.Clear();
            }

            // set each option in the env option set as an environment variable
            foreach (KeyValuePair<string, string> entry in EnvironmentVariables.Options)
            {
                envMap[entry.Key] = entry.Value;
            }

            ProcessUtil.SetEnvironmentVariables(envMap, process.StartInfo, ClearEnv, Log);
        }
    }
}
