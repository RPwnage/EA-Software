using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Diagnostics;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;

using EA.CPlusPlusTasks;

namespace EA.Eaconfig
{

    /// <summary>
    /// Generates the Static or Partial link file for an rso component.
    /// 
    /// The following options can be specified in a named optionset:
    /// 
    /// Both
    /// ====
    ///     lcf.type        --> can be either partial or static         [optional] [default=partial]
    /// 
    /// 
    /// Just Partial
    /// ============
    /// 
    ///     lcf.path        --> the output path for the generated lcf file, this is the combined
    ///                         template, the FORCEFILES section and the FORCEACTIVE section below
    ///                         
    /// 
    ///     lcf.template    --> template lcf file to use, these are typically in 2 flavours:
    ///                             rvl_sdk\include\revolution\eppc.RVL.lcf [default]
    ///                             rvl_sdk\include\dolphin\eppc.lcf
    ///                         [required]
    /// 
    ///     lcfObjects      --> specify objects that should be included in the FORCELINK section
    ///                         these would be object files from a related rso library component
    ///                         for instance if 2 lib components a <--> b you would need to specify
    ///                         both a *and* b object files in this block.  The task will take care
    ///                         of filtering out which are present and which need to be preserved via
    ///                         relative paths.  Because these paths are relative this *may* break
    ///                         in other environments or if the static link module is in a different
    ///                         relative path as compared to a and b...for example it depends that:
    ///                             build
    ///                                 \a
    ///                                 \b
    ///                                 \static
    /// 
    /// Just Static
    /// ===========
    ///     lcf             --> location of the makelcf.exe
    /// 
    ///     lcf.options     --> the options to be used for the makelcf.exe application.
    /// 
    /// 
    ///     lcf.libraries   --> path(s) to the rso library(ies) that this static module will reference.
    /// 
    /// </summary>
    /// 
    [TaskName("wii-lcf")]
    public class WiiLcfTask : Task, IExternalBuildTask
    {
        protected override void ExecuteTask()
        {
            throw new NotImplementedException("Use this as a build task.");
        }

        public void ExecuteBuild(BuildTask buildTask, OptionSet optionSet)
        {
            string lcfType = OptionSetUtil.GetOptionOrDefault(optionSet, "lcf.type", "partial");

            switch (lcfType)
            {
                case "partial":
                    ExecutePartialLcf(buildTask, optionSet);
                    break;
                case "static":
                    ExecuteStaticLcf(buildTask, optionSet);
                    break;
                default:
                    string msg = string.Format("Unknown lcf type {0}", lcfType);
                    throw new BuildException(msg);
            }
        }

        void ExecutePartialLcf(BuildTask buildTask, OptionSet optionSet)
        {
            string sdkDir = WiiTaskUtil.GetSdkDir(Project, "RevolutionSDK");
            string lcfPath = WiiTaskUtil.ExpandProperties(buildTask, OptionSetUtil.GetOptionOrFail(Project, optionSet, "lcf.path"));
            string lcfTemplatePath = WiiTaskUtil.ExpandProperties(buildTask, OptionSetUtil.GetOptionOrDefault(optionSet, "lcf.template", Path.Combine(sdkDir, @"include\dolphin\eppc.lcf")));
            string lcfObjects = OptionSetUtil.GetOptionOrDefault(optionSet, "lcf.objects", String.Empty);

            string outputDir = Path.GetDirectoryName(lcfPath);
            string filename = Path.GetFileName(lcfPath);
            string[] lcfExtensions = { "lcf"};
            string dependencyFilePath = Path.Combine(outputDir, filename + Dependency.DependencyFileExtension);
            string reason;

            if (Dependency.IsUpdateToDate(dependencyFilePath, outputDir, Path.GetFileNameWithoutExtension(filename), "wii-lcf", "", lcfExtensions, null, null, out reason))
            {
                return;
            }

            Log.WriteLine(LogPrefix + buildTask.BuildName);
            Log.WriteLineIf(Verbose, LogPrefix + "Creating partial lcf file because {0}", reason);

            if (!File.Exists(lcfTemplatePath))
            {
                Error.Throw(Project, "Unable to find LCF template file: '{0}'", lcfTemplatePath);
            }

            string template;
            using (TextReader tr = new StreamReader(lcfTemplatePath))
            {
                template = tr.ReadToEnd();
            }

            if (template == null)
            {
                Error.Throw(Project, "Unable to find LCF template file: '{0}'", lcfTemplatePath);
            }

            StringCollection allObjects = new StringCollection();
            StringCollection allLibraries = new StringCollection();

            using (TextWriter tw = new StreamWriter(lcfPath))
            {
                tw.WriteLine(template);
                tw.WriteLine("FORCEFILES");
                tw.WriteLine("{");

                // 1. grab any local 'Objects' included in the build task itself
                foreach (FileItem fileItem in buildTask.Objects.FileItems)
                {
                    tw.WriteLine("\t{0}", PathNormalizer.MakeRelative(fileItem.FullPath, buildTask.BuildDirectory).Replace("/", "\\"));
                    allObjects.Add(fileItem.FullPath);
                }
                

                // 2. now include the source files that we have compiled into objects
                FileItemList objectFiles = GetObjectFiles(buildTask);
                foreach (FileItem fileItem in objectFiles)
                {
                    tw.WriteLine("\t{0}", PathNormalizer.MakeRelative(fileItem.FullPath, buildTask.BuildDirectory).Replace("/", "\\"));
                    allObjects.Add(fileItem.FullPath);
                }
                

                // 3. allow the user to specify additional forcefiles to be included
                if (lcfObjects != null)
                {
                    foreach (string line in lcfObjects.Split())
                    {
                        if (line == null || line.Trim().Length == 0)
                        {
                            continue;
                        }

                        // resolve objPath
                        StringBuilder objPath = new StringBuilder(line);
                        objPath.Replace("%outputdir%", buildTask.BuildDirectory);
                        objPath.Replace("%outputname%", buildTask.BuildName);

                        //
                        // THIS IS WORTH A READ
                        //
                        // because it is confusing, but basically given to rso modules:
                        //      a
                        //      b
                        //
                        //  you need to the partial.lcf file know about each other's
                        //  objects so you really have a <--> b.  Since the build task
                        //  builds 1 then the other this isn't really possible at the
                        //  time each is built.  
                        //
                        //  It *is* possible however to include an arbitrary number of
                        //  object files as long as they are all resolved at the time
                        //  the static module is linked.
                        //
                        //  So the following just makes sure that we aren't doubly 
                        //  including an object in our partial lcf file (which is also
                        //  bad) and if not then we write out the relative path.
                        //
                        //  Of course if the path is no longer relative to the static
                        //  module I think we will run into other issues but this seems
                        //  to work for our system.
                        string objectPath = PathNormalizer.Normalize(objPath.ToString());
                        if (File.Exists(objectPath))
                        {
                            if (FindObjectFile(objectFiles, objectPath) != null)
                            {
                                continue;
                            }
                        }

                        string destPath = Path.Combine(buildTask.BuildDirectory, Path.GetFileName(objectPath));
                        tw.WriteLine("\t{0}", PathNormalizer.MakeRelative(objectPath, destPath).Replace("/", "\\"));
                        allObjects.Add(objectPath);
                    }
                }

                tw.WriteLine("}");
                tw.WriteLine("FORCEACTIVE {");
                tw.WriteLine("    __global_destructor_chain");
                tw.WriteLine("}");
            }

            if (allObjects.Count == 0)
            {
                Error.Throw(Project, "wii-lcf", "No object files specified.");
            }

            Dependency.GenerateDependencyFile(dependencyFilePath, "wii-lcf", "", allObjects, allLibraries);

        }

        void ExecuteStaticLcf(BuildTask buildTask, OptionSet optionSet)
        {
            // Get Rso files
            string lcfLibFileset = WiiTaskUtil.ExpandProperties(buildTask, OptionSetUtil.GetOptionOrFail(Project, optionSet, "lcf.libraryfileset"));

            FileSet lcfLibFiles = FileSetUtil.GetFileSet(Project, lcfLibFileset);

            if (lcfLibFiles != null && lcfLibFiles.FileItems.Count > 0)
            {

                string sdkDir = WiiTaskUtil.GetSdkDir(Project, "RevolutionSDK");
                string program = WiiTaskUtil.ExpandProperties(buildTask, OptionSetUtil.GetOptionOrDefault(optionSet, "lcf", Path.Combine(sdkDir, @"X86\bin\makelcf.exe")));

                string[] lcfExtensions = { "lcf" };
                string dependencyFilePath = Path.Combine(buildTask.BuildDirectory, buildTask.BuildName+"_lcf" + Dependency.DependencyFileExtension);
                string reason;


                StringCollection allObjects = new StringCollection();
                StringCollection allLibraries = new StringCollection();

                if (!File.Exists(program))
                {
                    Error.Throw(Project, "File not found {0}", program);
                }

                string lcfOptions = WiiTaskUtil.ExpandProperties(buildTask, OptionSetUtil.GetOptionOrFail(Project, optionSet, "lcf.options"));

                StringBuilder lcfLibraries = new StringBuilder();

                foreach (FileItem fileItem in lcfLibFiles.FileItems)
                {
                    lcfLibraries.AppendLine(PathNormalizer.StripQuotes(fileItem.FullPath));
                    allObjects.Add(fileItem.FullPath);
                }

                string commandLine = string.Format("{0} {1}", lcfOptions, lcfLibraries);

                commandLine = Regex.Replace(commandLine, @"[\n\r]*[\s]+", " ").Trim();

                if (!Dependency.IsUpdateToDate(dependencyFilePath, buildTask.BuildDirectory, Path.GetFileNameWithoutExtension(buildTask.BuildName), program, commandLine, lcfExtensions, null, null, out reason))
                {
                    RunProgram(program, commandLine);
                    Log.WriteLine(LogPrefix + buildTask.BuildName);
                    Log.WriteLineIf(Verbose, LogPrefix + "Creating static lcf file because {0}", reason);

                    Dependency.GenerateDependencyFile(dependencyFilePath, program, commandLine, allObjects, allLibraries);
                }


                WiiTaskUtil.SetLcfLinkOption(optionSet, "link.options");

                if(OptionSetUtil.GetOption(optionSet, "usedebuglibs") == "on")
                {
                    OptionSetUtil.AppendOption(optionSet, "link.libraries", Path.Combine(sdkDir, @"RVL\lib\rsoD.a"));
                }
                else
                {
                    OptionSetUtil.AppendOption(optionSet, "link.libraries", Path.Combine(sdkDir, @"RVL\lib\rso.a"));
                }

                
            }
        }

        FileItemList GetObjectFiles(BuildTask buildTask)
        {
            return WiiTaskUtil.GetObjectFiles(buildTask);
        }

        FileItem FindObjectFile(FileItemList fileItems, string path)
        {
            foreach (FileItem fileItem in fileItems)
            {
                if (fileItem.FullPath == path)
                {
                    return fileItem;
                }
            }
            return null;
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
                    string msg = String.Format("Error starting lcf tool '{0}'.", program);
                    throw new BuildException(msg, Location, e);
                }

                string output = processRunner.GetOutput();

                if (processRunner.ExitCode != 0)
                {
                    string msg = String.Format("Could not create lsf:\n{0}", output);
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
