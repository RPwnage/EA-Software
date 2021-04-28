using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Diagnostics;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;

using EA.CPlusPlusTasks;

namespace EA.Eaconfig
{

    /// <summary>
    /// Generates the .rso or static .elf file for an rso component.  The output depends
    ///     on whether a partially linked .elf file is passed in or if a statically linked
    ///     elf file is passed in.
    /// 
    ///   
    ///     rso             --> location of the makerso.exe
    /// 
    ///     rso.options     --> command line options to pass to the makerso.exe
    /// 
    /// </summary>
    [TaskName("wii-rso")]
    public class WiiRsoTask : Task, IExternalBuildTask
    {
        
        protected override void ExecuteTask()
        {
            throw new NotImplementedException("Use this as a build task.");
        }



        public void ExecuteBuild(BuildTask buildTask, OptionSet optionSet)
        {
            string lcfType = OptionSetUtil.GetOptionOrDefault(optionSet, "lcf.type", "");

            if (lcfType == "static")
            {

                string lcfLibFileset = WiiTaskUtil.ExpandProperties(buildTask, OptionSetUtil.GetOptionOrDefault(optionSet, "lcf.libraryfileset", "none"));
                FileSet lcfLibFiles = FileSetUtil.GetFileSet(Project, lcfLibFileset);

                if (!(lcfLibFiles != null && lcfLibFiles.FileItems.Count > 0))
                {
                    //Don't need to run this task if there are no RSO files
                    return;
                }
            }

            string sdkDir = WiiTaskUtil.GetSdkDir(Project, "RevolutionSDK");

            string program = WiiTaskUtil.ExpandProperties(buildTask, OptionSetUtil.GetOptionOrDefault(optionSet, "rso", Path.Combine(sdkDir, @"X86\bin\makerso.exe")));

            string commandLine = WiiTaskUtil.ExpandProperties(buildTask, OptionSetUtil.GetOptionOrFail(buildTask.Project, optionSet, "rso.options"));

            StringCollection allObjects = WiiTaskUtil.GetFilesFromCommandLine(commandLine);
            StringCollection allLibraries = new StringCollection();

            commandLine = Regex.Replace(commandLine, @"[\n\r]*[\s]+", " ").Trim();

            string[] rsoExtensions = { "rso", "sel" };
            string dependencyFilePath = Path.Combine(buildTask.BuildDirectory, buildTask.BuildName + "_rso" + Dependency.DependencyFileExtension);
            string reason;

            if (!Dependency.IsUpdateToDate(dependencyFilePath, buildTask.BuildDirectory, Path.GetFileNameWithoutExtension(buildTask.BuildName), program, commandLine, rsoExtensions, null, null, out reason))
            {
                RunProgram(program, commandLine);

                Log.WriteLine(LogPrefix + buildTask.BuildName);
                Log.WriteLineIf(Verbose, LogPrefix + "Creating RSO file because {0}", reason);

                Dependency.GenerateDependencyFile(dependencyFilePath, program, commandLine, allObjects, allLibraries);
            }
        }

        private void RunProgram(string program, string commandLine)
        {
            Log.WriteLineIf(Verbose, LogPrefix + "{0} {1}", program, commandLine);

            Process process = WiiTaskUtil.CreateProcess(program, commandLine);

            using (ProcessRunner processRunner = new ProcessRunner(process))
            {
                try
                {
                    processRunner.Start();
                }
                catch (Exception e)
                {
                    string msg = String.Format("Error starting rso tool '{0}'.", program);
                    throw new BuildException(msg, Location, e);
                }

                string output = processRunner.GetOutput();

                if (processRunner.ExitCode != 0)
                {
                    string msg = String.Format("Could not create rso.\n{0}", output);
                    throw new BuildException(msg, Location);
                }

                // If the word "warning" is in the output the program has issued a 
                // warning message but the return code is still zero because 
                // warnings are not errors.  We still need to display the message 
                // but doing this feels like such a hack.  See bug 415 for details.
                bool condition = ((Verbose && output.Length > 0) || (output.IndexOf("warning") != -1));
                int indentLevel = Log.IndentLevel;
                Log.IndentLevel = 0;
                Log.WriteLineIf(condition, output);
                Log.IndentLevel = indentLevel;
            }

        }



    }
}
