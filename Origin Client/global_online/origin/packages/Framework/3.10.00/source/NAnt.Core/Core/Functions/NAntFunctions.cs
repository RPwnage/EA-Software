using System;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Functions
{
    /// <summary>
    /// Collection of NAnt Project routines.
    /// </summary>
    [FunctionClass("NAnt Functions")]
    public class NAntFunctions : FunctionClassBase
    {
        /// <summary>
        /// Tests whether Framework (NAnt) is runnting in parallel mode .
        /// </summary>
        /// <param name="project"></param>
        /// <returns>Returns true if Framework is in paralel mode (default).</returns>
        /// <remarks>Parallel mode can be switched off bynant command line parameter -noparallel.</remarks>
        [Function()]
        public static string NAntIsParallel(Project project)
        {
            return Project.NoParallelNant ? "false" : "true";
        }

        /// <summary>
        /// Gets log file name paths.
        /// </summary>
        /// <param name="project"></param>
        /// <returns>Returns new line separated list of log file names or an empty string when log is not redirected to a file</returns>
        [Function()]
        public static string GetLogFilePaths(Project project)
        {
            var paths = new List<string>();

            foreach (var listener in project.Log.Listeners)
            {
                var fileListener = listener as FileListener;
                if (fileListener != null)
                {
                    paths.Add(fileListener.LogFilePath);
                }
            }

            return paths.OrderedDistinct().ToString(Environment.NewLine);
        }


    }
}