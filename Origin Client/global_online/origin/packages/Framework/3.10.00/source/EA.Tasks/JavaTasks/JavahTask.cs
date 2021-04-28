// (c) Electronic Arts. All Rights Reserved.
using System;
using System.IO;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace EA.JavaTasks {

    /// <summary>Generates JNI headers using javah.</summary>
    [TaskName("javah")]
    public class JavahTask : CompilerBase {

        public JavahTask() : base("javah") { }

        /// <summary>Generate JNI-style header file (default)</summary>
        [TaskAttribute("jni")]
        public bool Jni
        {
            get { return _jni; }
            set { _jni = value; }
        } bool _jni = true;

        /// <summary>New line or semicolon separated list of source directories.</summary>
        [TaskAttribute("classes")]
        public string Classes
        {
            get { return _classes; }
            set { _classes = value; }
        } string _classes;

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
                    return Path.Combine(Path.Combine(javaHome, "bin"), "javah");
                }
                return Compiler;
            }
        }


        protected override void WriteOptions()
        {
            if (Jni)
                WriteOption("jni");


            if (!String.IsNullOrEmpty(Classes))
            {
                foreach (string cls in Classes.Split(new char[] { '\n', '\r', '\t' }))
                {
                    string trimmedCls = cls.Trim();
                    if (!String.IsNullOrEmpty(trimmedCls))
                    {
                        WriteFile(trimmedCls);
                    }
                }
            }
        }
    }
}
