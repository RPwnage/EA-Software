using NAnt.Core;

using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

namespace EA.Eaconfig
{
    public class Error
    {
        public static void Throw(Project project, string module, string message)
        {
            throw new BuildException(Format(project, module, "ERROR", message));
        }

        public static void Throw(Project project, Location location, string module, string message)
        {
            throw new BuildException(Format(project, module, "ERROR", message), location);
        }


        public static void Throw(Project project, string module, string format, params object[] args)
        {
            string message = String.Format(format, args);
            Throw(project, module, message);
        }

        public static void Throw(Project project, Location location, string module, string format, params object[] args)
        {
            string message = String.Format(format, args);
            Throw(project, location, module, message);
        }


        public static string Format(Project project, string module, string type, string message)
        {
            String package_name = project.Properties["package.name"];
            String package_version = project.Properties["package.version"];
            String build_module = project.Properties["build.module"];

            if (String.IsNullOrEmpty(package_name))
            {
                package_name = "????";
            }
            if (String.IsNullOrEmpty(package_version))
            {
                package_version = "??";
            }
            if (!String.IsNullOrEmpty(build_module) && build_module != package_name)
            {
                package_version += " (" + build_module + ") ";
            }
            String err_header = type;
            if (!String.IsNullOrEmpty(module))
            {
                err_header = type + " in " + module;
            }

            String msg = String.Format("\n  [{0}-{1}] {2} : {3}\n", package_name, package_version, err_header, message);
            return msg;
        }

        public static string Format(Project project, string module, string type, string format, params object[] args)
        {
            string message = String.Format(format, args);
            return Format(project, module, type, message);
        }
    }
}


