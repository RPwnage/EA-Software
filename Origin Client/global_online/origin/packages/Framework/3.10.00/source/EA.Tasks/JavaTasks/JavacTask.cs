// (c) Electronic Arts. All Rights Reserved.
using System;
using System.IO;
using System.Collections.Specialized;
using System.Text;

using NAnt.Core.Attributes;
using NAnt.Core;
using NAnt.Core.Util;

namespace EA.JavaTasks
{

    /// <summary>Compiles Java.</summary>
    [TaskName("javac")]
    public class JavacTask : CompilerBase
    {
        public JavacTask() : base("javac") { }

        /// <summary>Generate debug output (<c>true</c>/<c>false</c>). Default is false.</summary>
        [TaskAttribute("debug")]
        public bool Debug
        {
            get { return _debug; }
            set { _debug = value; }
        } bool _debug = false;

        /// <summary>Generate class files for specific VM version.</summary>
        [TaskAttribute("target")]
        public string Target
        {
            get { return _target; }
            set { _target = value; }
        } string _target;

        /// <summary>New line or semicolon separated list of source directories.</summary>
        [TaskAttribute("sourcepath")]
        public string SourcePath
        {
            get { return _sourcepath; }
            set { _sourcepath = value; }
        } string _sourcepath;


        /// <summary>Specify character encoding used by source files. Default: 'ascii'</summary>
        [TaskAttribute("encoding")]
        public string Encoding
        {
            get { return _encoding; }
            set { _encoding = value; }
        } string _encoding;

        /// <summary>The set of source files for compilation.</summary>
        [FileSet("sources")]
        public FileSet Sources
        {
            get { return _sources; }
            set { _sources = value; }
        } FileSet _sources = new FileSet();

        public override string ProgramFileName
        {
            get
            {
                if (Compiler == null)
                {
                    string javaHome = Project.Properties["package.jdk.appdir"];
                    if (javaHome == null)
                    {
                        throw new BuildException("'compiler' argument to 'javac' task or 'package.jdk.appdir' property are not defined. Can't locate Java compiler. Pass compiler path or execute dependency on 'jdk' package.");
                    }

                    return Path.Combine(Path.Combine(javaHome, "bin"), "javac");
                }
                return Compiler;
            }
        }


        protected override void WriteOptions()
        {
            if (Debug)
                WriteOption("g");

            if (!String.IsNullOrEmpty(Target))
                WriteOption("target", Target);

            if (!String.IsNullOrEmpty(Encoding))
                WriteOption("encoding", Encoding);


            if (!String.IsNullOrEmpty(SourcePath))
            {
                StringBuilder sb = new StringBuilder();
                foreach (string dir in SourcePath.Split(new char[] { '\n', '\r', ';' }))
                {
                    string trimmedDir = dir.Trim();
                    if (!String.IsNullOrEmpty(trimmedDir))
                    {
                        sb.Append(trimmedDir);
                        sb.Append(';');
                    }
                }
                string srcPath = sb.ToString().Trim(new char[] { ';' });
                WritePathOption("sourcepath",srcPath);
            }

            // Source files to options:

            foreach (FileItem fileItem in Sources.FileItems)
            {
                WriteFile(fileItem.FileName);
            }
        }
    }
}
