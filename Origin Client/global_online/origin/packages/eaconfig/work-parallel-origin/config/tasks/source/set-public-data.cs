using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
    [TaskName("set-package-public-data")]
    public class SetPackagePublicData : Task
    {
        private string _packageName;
        private string _processors;
        private string _extralibs;
        private string _modules;
        private FileSet _extralibsFileset= new FileSet();

        [TaskAttribute("package-name", Required = true)]
        public string PackageName
        {
            get { return _packageName; }
            set { _packageName = value; }
        }

        [TaskAttribute("processors", Required = false)]
        public string Processors
        {
            get { return _processors; }
            set { _processors = value; }
        }

        [TaskAttribute("extralibs", Required = false)]
        public string ExtraLibs
        {
            get { return _extralibs; }
            set { _extralibs = value; }
        }

        [TaskAttribute("modules", Required = false)]
        public string Modules
        {
            get { return _modules; }
            set { _modules = value; }
        }


        /// <summary></summary>
        [FileSet("extralibs")]
        public FileSet ExtraLibsFileset
        {
            get { return _extralibsFileset; }
        }


        protected override void ExecuteTask()
        {
            
        }


    }


    public abstract class SetPackagePublicDataBase : Task
    {
        private string _packageName;
        private string _processors;
        private string _modules;
        private string _extralibs;
        private string _group = "runtime";
        private FileSet _extralibsFileset = new FileSet();

        [TaskAttribute("package-name", Required = true)]
        public string PackageName
        {
            get { return _packageName; }
            set { _packageName = value; }
        }

        [TaskAttribute("processors", Required = false)]
        public string Processors
        {
            get { return _processors; }
            set { _processors = value; }
        }

        [TaskAttribute("modules", Required = false)]
        public string Modules
        {
            get { return _modules; }
            set { _modules = value; }
        }


        [TaskAttribute("extralibs", Required = false)]
        public string ExtraLibs
        {
            get { return _extralibs; }
            set { _extralibs = value; }
        }

        [TaskAttribute("group", Required = false)]
        public string Group
        {
            get { return _group; }
            set { _group = value; }
        }


        /// <summary>All the files in this fileset will have their file attributes set.</summary>
        [FileSet("extralibs")]
        public FileSet ExtraLibsFileset
        {
            get { return _extralibsFileset; }
        }

        protected override void ExecuteTask()
        {
            var modules = Modules.ToArray();

            foreach (var processor in Processors.ToArray())
            {

                if (modules.Count == 0)
                {
                    SetPackagePublicData(processor);
                }
                else
                {
                    foreach (var module in modules)
                    {
                        SetModulePublicData(module, processor);
                    }
                }
            }
        }


        protected void SetPackagePublicData(string processor)
        {
            var includedirs = Environment.NewLine + Properties[PackagePropertyName("dir")] + "/include";

            if (!String.IsNullOrEmpty(processor))
            {
                includedirs += Environment.NewLine + Properties[PackagePropertyName("dir")] + "/include_" + processor;
            }

            Properties[PackagePropertyName("includedirs", processor)] += includedirs + Environment.NewLine;


            var libdir = Path.Combine(Properties[PackagePropertyName("builddir")], Properties["config"], "lib", Project.Properties["eaconfig." + Group + ".outputfolder"].TrimLeftSlash() ?? String.Empty);

            Properties[PackagePropertyName("libdir", processor)] = libdir;

            var libs = new FileSet();
            libs.BaseDirectory = libdir;

            foreach (var libname in (PackageName + " " + ExtraLibs ?? String.Empty).ToArray())
            {

                if (String.IsNullOrEmpty(processor))
                {
                    libs.Include(Path.Combine(libdir, Properties["lib-prefix"] + libname + Properties["lib-suffix"]), asIs: true);
                }
                else
                {
                    libs.Include(Path.Combine(libdir, Properties["lib-prefix"] + libname + "_" + processor + Properties["lib-suffix"]), asIs: true);
                }
            }

            libs.Include(ExtraLibsFileset);

            Project.NamedFileSets[PackagePropertyName("libs", processor)] = libs;
            Project.NamedFileSets[PackagePropertyName("libs")] = libs;
        }

        protected void SetModulePublicData(string module, string processor)
        {
            var includedirs = Environment.NewLine + Properties[PackagePropertyName("dir")] + "/include";

            if (!String.IsNullOrEmpty(processor))
            {
                includedirs += Environment.NewLine + Properties[PackagePropertyName("dir")] + "/include_" + processor;
            }

            Properties[ModulePropertyName("includedirs", module, processor)] += includedirs + Environment.NewLine;

            var libdir = Path.Combine(Properties[PackagePropertyName("builddir")], Properties["config"], "lib", Project.Properties["eaconfig." + Group + ".outputfolder"].TrimLeftSlash() ?? String.Empty);

            Properties[ModulePropertyName("libdir", module, processor)] = libdir;

            var libs = new FileSet();
            libs.BaseDirectory = libdir;

            foreach (var libname in (module + " " + ExtraLibs ?? String.Empty).ToArray())
            {

                if (String.IsNullOrEmpty(processor))
                {
                    libs.Include(Path.Combine(libdir, Properties["lib-prefix"] + libname + Properties["lib-suffix"]), asIs: true);
                }
                else
                {
                    libs.Include(Path.Combine(libdir,Properties["lib-prefix"] + libname + "_" + processor + Properties["lib-suffix"]), asIs: true);
                }
            }

            libs.Include(ExtraLibsFileset);

            Project.NamedFileSets[ModulePropertyName("libs", module, processor)] = libs;
            Project.NamedFileSets[ModulePropertyName("libs", module)] = libs;

        }


        protected string PackagePropertyName(string propertyname, string processor = null)
        {
            return string.IsNullOrEmpty(processor)? "package."+PackageName + "." + propertyname : "package."+PackageName + "." + propertyname + "." +  processor;
        }

        protected string ModulePropertyName(string propertyname, string module, string processor = null)
        {
            return string.IsNullOrEmpty(processor)? "package."+PackageName + "." + module + "." + propertyname : "package."+PackageName + "." + module + "." + propertyname + "." +  processor;
        }

    }

}
