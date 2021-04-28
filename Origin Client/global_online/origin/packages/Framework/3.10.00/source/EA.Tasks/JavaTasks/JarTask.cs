// (c) Electronic Arts. All Rights Reserved.
using System;
using System.IO;
using System.Xml;
using System.Text;
using System.Collections;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;

namespace EA.JavaTasks
{
    /// <summary>Generates JNI headers using javah.</summary>
    [TaskName("jar")]
    public class JarTask : ExternalProgramBase
    {
        public JarTask() : base("jar") { }

        /// <summary>Enable verbose output. Default: false</summary>
        [TaskAttribute("verbose", Required = false)]
        public override bool Verbose
        {
            get { return _verbose; }
            set { _verbose = value; }
        } bool _verbose = false;

        /// <summary>mode: create/update/extract</summary>
        [TaskAttribute("mode", Required = true)]
        public string Mode
        {
            get { return _mode; }
            set
            {
                if (value != null)
                    _mode = value.ToLower();
                else
                    _mode = String.Empty;
            }
        } string _mode = "create";

        /// <summary>archive file</summary>
        [TaskAttribute("archive")]
        public string Archive
        {
            get { return _archive; }
            set { _archive = value; }
        } string _archive;

        /// <summary>manifest file</summary>
        [TaskAttribute("manifest")]
        public string Manifest
        {
            get { return _manifest; }
            set { _manifest = value; }
        } string _manifest;

        /// <summary>manifest file</summary>
        [TaskAttribute("entrypoint")]
        public string Entrypoint
        {
            get { return _entrypoint; }
            set { _entrypoint = value; }
        } string _entrypoint;

        /// <summary>The set of source files to archive.</summary>
        [TaskAttribute("inputdir")]
        public string InputDir
        {
            get { return _inputdir; }
            set { _inputdir = value; }
        } string _inputdir;


        /// <summary>The set of source files to archive.</summary>
        [TaskAttribute("inputfiles")]
        public string Inputs
        {
            get { return _inputs; }
            set { _inputs = value; }
        } string _inputs;

        /// <summary>
        /// The full path to the compiler. 
        /// </summary>
        [TaskAttribute("jartool", Required = false)]
        public string JarTool
        {
            get { return _jartool; }
            set { _jartool = PathNormalizer.Normalize(value); }
        } string _jartool;


        public override string ProgramFileName
        {
            get
            {
                if (JarTool == null)
                {
                    string javaHome = Project.Properties["package.jdk.appdir"];
                    if (javaHome == null)
                    {
                        throw new BuildException("'compiler' argument to 'javac' task or 'package.jdk.appdir' property are not defined. Can't locate Java compiler. Pass compiler path or execute dependency on 'jdk' package.");
                    }
                    return Path.Combine(Path.Combine(javaHome, "bin"), "jar");
                }
                return JarTool;
            }
        }

        protected virtual bool NeedsRunning()
        {
            return true;
        }


        public override string ProgramArguments
        {
            get
            {
                StringBuilder sb = new StringBuilder();

                switch (Mode)
                {
                    case "create":
                    case "update":

                        if (Mode == "create")
                            sb.Append('c');
                        else
                            sb.Append('u');

                        if (Verbose)
                        {
                            sb.Append('v');
                        }

                        if (!String.IsNullOrEmpty(Manifest))
                        {
                            sb.Append('m');
                        }
                        if (!String.IsNullOrEmpty(Archive))
                        {
                            sb.Append('f');
                        }

                        if (!String.IsNullOrEmpty(Manifest))
                        {
                            sb.Append(' ');
                            sb.Append(PathNormalizer.AddQuotes(PathNormalizer.Normalize(Manifest, false)));
                        }

                        if (!String.IsNullOrEmpty(Archive))
                        {
                            sb.Append(' ');
                            sb.Append(PathNormalizer.AddQuotes(PathNormalizer.Normalize(Archive, false)));
                        }

                        if (!String.IsNullOrEmpty(InputDir))
                        {
                            sb.Append(" -C ");
                            sb.Append(PathNormalizer.AddQuotes(PathNormalizer.Normalize(InputDir, false)));
                        }

                        if (!String.IsNullOrEmpty(Inputs))
                        {
                            sb.Append(' ');
                            sb.Append(Inputs);
                        }

                        if (!String.IsNullOrEmpty(Entrypoint))
                        {
                            sb.Append(" -e ");
                            sb.Append(Entrypoint);
                        }


                        break;

                    case "extract":
                        sb.Append('x');
                        if (Verbose)
                        {
                            sb.Append('v');
                        }

                        if (!String.IsNullOrEmpty(Archive))
                        {
                            sb.Append('f');
                            sb.Append(' ');
                            sb.Append(PathNormalizer.AddQuotes(PathNormalizer.Normalize(Archive, false)));
                        }

                        if (!String.IsNullOrEmpty(Inputs))
                        {
                            sb.Append(' ');
                            sb.Append(Inputs);
                        }

                        break;

                    default:

                        throw new BuildException("Invalud 'mode' attribute in jar task: mode='" + Mode + "'.", Location);
                }

                return sb.ToString();
            }
        }


        protected override void ExecuteTask()
        {
            if (NeedsRunning())
            {
                // call base class to do the work
                base.ExecuteTask();
            }
        }
    }

}
