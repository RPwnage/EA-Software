// (c) Electronic Arts. All Rights Reserved.
using System;
using System.IO;
using System.Diagnostics;
using System.Collections;
using System.Collections.Specialized;
using System.Text.RegularExpressions;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace EA.JavaTasks
{

    /// <summary>Provides the abstract base class for a java compiler task.</summary>
    public abstract class CompilerBase : ExternalProgramBase
    {
        public CompilerBase(string name) : base(name) { }

        /// <summary>Output directory to put generated class files.</summary>
        [TaskAttribute("outputdir", Required = true)]
        public string OutputDir
        {
            get { return _outputDir; }
            set { _outputDir = PathNormalizer.Normalize(value, false); }
        } string _outputDir = null;

        /// <summary>Enable verbose output. Default: false</summary>
        [TaskAttribute("verbose", Required = false)]
        public override bool Verbose
        {
            get { return _verbose; }
            set { _verbose = value; }
        } bool _verbose = false;

        /// <summary>Specify where to find user class files and annotation processors.</summary>
        [TaskAttribute("classpath", Required = false)]
        public string ClassPath
        {
            get { return _classPath; }
            set { _classPath = value; }
        } string _classPath = null;

        /// <summary>location of bootstrap class files.</summary>
        [TaskAttribute("bootclasspath", Required = false)]
        public string BootClassPath
        {
            get { return _bootclasspath; }
            set { _bootclasspath = value; }
        } string _bootclasspath;


        /// <summary>
        /// The full path to the compiler. 
        /// </summary>
        [TaskAttribute("compiler", Required = false)]
        public string Compiler
        {
            get { return _compiler; }
            set { _compiler = PathNormalizer.Normalize(value); }
        } string _compiler;


        public override string ProgramFileName
        {
            get
            {
                if (Compiler == null)
                {

                    string msg = String.Format("Compiler is not defined in task '{0}'.", Name);
                    throw new BuildException(msg, Location);
                }
                return Compiler;
            }
        }

        public override string ProgramArguments
        {
            get { return argumentBuilder.ToString(); }
        } StringBuilder argumentBuilder = new StringBuilder();

#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return Output; }
		}
#endif
        /// <summary>Allows derived classes to provide compiler-specific options.</summary>
        protected virtual void WriteOptions()
        {
        }

        /// <summary>Write an option using the default output format.</summary>
        protected virtual void WriteOption(string name)
        {
            argumentBuilder.AppendFormat("-{0} ", name);
        }

        /// <summary>Write </summary>
        protected virtual void WriteFile(string file)
        {
            argumentBuilder.Append(PathNormalizer.AddQuotes(file));
            argumentBuilder.Append(' ');
        }


        /// <summary>Write an option and its value using the default output format.</summary>
        protected virtual void WriteOption(string name, string arg)
        {
            // Always quote arguments ( )
            argumentBuilder.AppendFormat("-{0} {1} ", name, arg);
        }

        protected virtual void WritePathOption(string name, string arg)
        {
            string normalized = PathNormalizer.AddQuotes(arg);
            argumentBuilder.AppendFormat("-{0} {1} ", name, normalized);
        }



        /// <summary>Determines whether compilation is needed.</summary>
        protected virtual bool NeedsCompiling()
        {
            // return true as soon as we know we need to compile
            /*
                        FileInfo outputFileInfo = new FileInfo(GetOutputPath());
                        if (!outputFileInfo.Exists) {
                            return true;
                        }

                        //Sources Updated?
                        string fileName = FileSet.FindMoreRecentLastWriteTime(Sources.FileItems.ToStringCollection(), outputFileInfo.LastWriteTime);
                        if (fileName != null) {
                            Log.WriteLineIf(Verbose, LogPrefix + "{0} is out of date, recompiling.", fileName);
                            return true;
                        }

                        //References Updated?
                        fileName = FileSet.FindMoreRecentLastWriteTime(References.FileItems.ToStringCollection(), outputFileInfo.LastWriteTime);
                        if (fileName != null) {
                            Log.WriteLineIf(Verbose, LogPrefix + "{0} is out of date, recompiling.", fileName);
                            return true;
                        }

                        //Modules Updated?
                        fileName = FileSet.FindMoreRecentLastWriteTime(Modules.FileItems.ToStringCollection(), outputFileInfo.LastWriteTime);
                        if (fileName != null) {
                            Log.WriteLineIf(Verbose, LogPrefix + "{0} is out of date, recompiling.", fileName);
                            return true;
                        }

                        //Resources Updated?
                        fileName = FileSet.FindMoreRecentLastWriteTime(Resources.FileItems.ToStringCollection(), outputFileInfo.LastWriteTime);
                        if (fileName != null) {
                            Log.WriteLineIf(Verbose, LogPrefix + "{0} is out of date, recompiling.", fileName);
                            return true;
                        }
 
                        // check the args for /res or /resource options.
                        StringCollection resourceFileNames = new StringCollection();
                        foreach (string arg in Args) {
                            if (arg.StartsWith("/res:") || arg.StartsWith("/resource:")) {
                                string path = arg.Substring(arg.IndexOf(':') + 1);

                                int indexOfComma = path.IndexOf(',');
                                if (indexOfComma != -1) {
                                    path = path.Substring(0, indexOfComma);
                                }
                                resourceFileNames.Add(path);
                            }
                        }
                        fileName = FileSet.FindMoreRecentLastWriteTime(resourceFileNames, outputFileInfo.LastWriteTime);
                        if (fileName != null) {
                            Log.WriteLineIf(Verbose, LogPrefix + "{0} is out of date, recompiling.", fileName);
                            return true;
                        }

                        // if we made it here then we don't have to recompile
                        return false;
             */
            return true;
        }

        protected override void ExecuteTask()
        {
            if (NeedsCompiling())
            {

                //Log.WriteLine(LogPrefix + Path.GetFileName(GetOutputPath()));
                if (!Directory.Exists(OutputDir))
                {
                    Directory.CreateDirectory(OutputDir);
                }

                if (Verbose)
                    WriteOption("verbose");

                WriteOption("d", OutputDir);

                WriteOption("classpath", ClassPath);

                WriteOption("bootclasspath", BootClassPath);

                // write compiler options
                WriteOptions();

                // call base class to do the work
                base.ExecuteTask();
            }
        }

    }
}
