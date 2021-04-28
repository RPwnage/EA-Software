using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Backends;

using EA.Eaconfig.Backends.VisualStudio;

namespace EA.Eaconfig.Backends.VisualStudio.sln_taskdef
{

    internal class TaskDefProject : VSDotNetProject
    {
        internal TaskDefProject(TaskDefSolution solution, IEnumerable<IModule> modules)
            : base(solution, modules, CSPROJ_GUID)
        {
            ScriptFile = StartupModule.ExcludedFromBuild_Files.FileItems.First().Path;
        }

        internal readonly PathString ScriptFile;


        public override string ProjectFileName
        {
            get
            {
                return Name + ".csproj";
            }
        }

        public override PathString OutputDir
        {
            get { return BuildGenerator.OutputDir; }
        }

        protected override PathString GetDefaultOutputDir()
        {
            return BuildGenerator.OutputDir;
        }

        protected override string TargetFrameworkVersion
        {
            get
            {
                return String.Format("v{0}.{1}", Environment.Version.Major, Environment.Version.MajorRevision);
            }
        }

        protected override string ToolsVersion
        {
            get
            {
                return "4.0";
            }
        }

        protected override string DefaultTargetFrameworkVersion
        {
            get { throw new BuildException("Internal error. This method should not be called"); }
        }

        protected override string ProductVersion
        {
            get { return "8.0.30703"; }
        }

        protected override string GetLinkPath(FileEntry fe)
        {
            string link_path = String.Empty;

            if (!String.IsNullOrEmpty(PackageDir))
            {
                link_path = PathUtil.RelativePath(fe.Path.Path, PackageDir).TrimStart(new char[] { '/', '\\', '.' });

                if (!String.IsNullOrEmpty(link_path))
                {
                    if (Path.IsPathRooted(link_path) || (link_path.Length > 1 && link_path[1] == ':'))
                    {
                        link_path = link_path[0] + "_Drive" + link_path.Substring(2);
                    }
                }

                return link_path;

            }
            return base.GetLinkPath(fe);
        }

        protected override string Version
        {
            get
            {
                return "10.00";
            }
        }

    }
}
