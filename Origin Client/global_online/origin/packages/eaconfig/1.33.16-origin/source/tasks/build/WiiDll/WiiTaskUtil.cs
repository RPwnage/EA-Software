using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Reflection;
using System.Security.Permissions;
using System.Diagnostics;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Tasks;

using EA.CPlusPlusTasks;

namespace EA.Eaconfig
{
    public class WiiTaskUtil
    {
        public static string ExpandProperties(BuildTask buildTask, string input)
        {
            StringBuilder output = new StringBuilder(input);
            output.Replace("%outputdir%", buildTask.BuildDirectory);
            output.Replace("%outputname%", buildTask.BuildName);

            return buildTask.Project.ExpandProperties(output.ToString());
        }

        /// <summary>
        /// Get a list of the object files for the buildTask.
        /// 
        /// Note: This would be a lot easier if all of these were availble in the Objects property
        /// of the buildTask, unfortunately we have to look at the _compilerTask and the _asemmblerTask.
        /// </summary>
        /// <param name="buildTask"></param>
        /// <returns></returns>
        public static FileItemList GetObjectFiles(BuildTask buildTask)
        {
            AsTask asTask = GetRequiredPrivateFieldValue(buildTask.Project, buildTask, "_assemblerTask") as AsTask;
            CcTask ccTask = GetRequiredPrivateFieldValue(buildTask.Project, buildTask, "_complierTask") as CcTask;

            if(asTask == null)
            {
                 Error.Throw(buildTask.Project, "WiiTaskUtil.GetObjectFiles", "AsTask should not be null.");
            }
            if(ccTask == null)
            {
                 Error.Throw(buildTask.Project, "WiiTaskUtil.GetObjectFiles", "CcTask should not be null.");
            }

            FileItemList objectFiles = new FileItemList();
            foreach (FileItem fileItem in buildTask.Sources.FileItems)
            {
                FileItem objectPath = new FileItem(ccTask.GetObjectPath(fileItem));
                objectFiles.Add(objectPath);
            }

            foreach (FileItem fileItem in buildTask.AsmSources.FileItems)
            {
                FileItem objectPath = new FileItem(asTask.GetObjectPath(fileItem));
                objectFiles.Add(objectPath);
            }

            return objectFiles;

        }

        /// <summary>
        /// Retrieve the value from a field in the given object.
        /// </summary>
        /// <param name="obj">The object to retrieve the field from.</param>
        /// <param name="name">The name of the field.</param>
        /// <returns>The value of the given field.</returns>
        [ReflectionPermission(SecurityAction.LinkDemand)]
        public static object GetRequiredPrivateFieldValue(Project project, object obj, string name)
        {
            if(obj == null)
            {
                 Error.Throw(project, "WiiTaskUtil.GetRequiredPrivateFieldValue", "Object was null.");
            }
            if(String.IsNullOrEmpty(name))
            {
                 Error.Throw(project, "WiiTaskUtil.GetRequiredPrivateFieldValue", "Specify fieldName.");
            }

            FieldInfo fieldInfo = obj.GetType().GetField(name,
                BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.FlattenHierarchy);

            return fieldInfo.GetValue(obj);
        }

        public static string GetSdkDir(Project project, string name)
        {
            string packageDir = project.Properties["package." + name + ".appdir"];

            if (String.IsNullOrEmpty(packageDir))
            {
                Error.Throw(project, "Package '{0}' directory is not defined. Execute dependency on the package.", name);
            }

            return packageDir;
        }

        public static void SetLcfLinkOption(OptionSet optionSet, string optionName)
        {
            string linkOptions = OptionSetUtil.GetOption(optionSet, optionName);
            string lcfPath = OptionSetUtil.GetOption(optionSet, "lcf.path");

            if (!String.IsNullOrEmpty(linkOptions) && !String.IsNullOrEmpty(lcfPath))
            {
                IList<string> options = StringUtil.ToArray(linkOptions, "\r\n\t");

                bool hasLcf = false;
                for (int i = 0; i < options.Count; i++)
                {
                    if (options[i] != null && options[i].StartsWith("-lcf"))
                    {
                        options[i] = "-lcf " + lcfPath;
                        hasLcf = true;
                        break;
                    }
                }
                if (!hasLcf)
                {
                    options.Add("-lcf " + lcfPath);
                }

                StringBuilder sb = new StringBuilder();
                foreach (string val in options)
                {
                    sb.AppendLine(val);
                }
                optionSet.Options[optionName] = sb.ToString();
            }
            else if (!String.IsNullOrEmpty(lcfPath))
            {
                OptionSetUtil.AppendOption(optionSet, optionName, "-lcf " + lcfPath);
            }
        }

        public static StringCollection GetFilesFromCommandLine(string command)
        {
            StringCollection files = new StringCollection();
            if (!String.IsNullOrEmpty(command))
            {
                foreach(string option in StringUtil.ToArray(command, "\r\n\t"))
                {
                    if(String.IsNullOrEmpty(option))
                    {
                        continue;
                    }

                    string val = option.Trim(new Char[] { '\r', '\n', '\t', ' ' });
                    if (-1 != val.IndexOfAny(new Char[] { '\\', '/' }))
                    {
                        if (val.StartsWith("-"))
                        {
                            int ind = val.IndexOf(' ');
                            if (ind > 0 && ind < val.Length - 1)
                            {
                                val = val.Substring(ind + 1);

                            }
                        }
                        files.Add(val);
                    }
                }
            }
            return files;
        }

        public static Process CreateProcess(string program, string arguments)
        {
            Process p = new Process();
            p.StartInfo.FileName = program;
            p.StartInfo.Arguments = arguments;

            p.StartInfo.RedirectStandardInput = true;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = true;
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.CreateNoWindow = true;

            return p;
        }




    }
}
