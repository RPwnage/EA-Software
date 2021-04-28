// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// File Maintainer:
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections;
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks {

    /// <summary>Performs a generic dependency check.</summary>
    /// <remarks>
    /// <para>When properly used, this task sets the specified property to true if any file in the <c>inputs</c> is newer than any
    /// file in the <c>outputs</c> fileset.</para>
    /// <para>
    /// Proper usage requires that at least one file needs to be present in the inputs fileset, as well as the outputs fileset.  
    /// </para>
	/// <para>
	/// If either fileset is empty, the property will be set to true.
	/// Note that a fileset can be empty if it includes a nonexistent file(name) "asis" 
	/// (by setting the attribute asis="true").
	/// Since the asis attribute is false by default, it's impossible to create
	/// a fileset that includes a nonexistent file without overriding asis.
	/// </para>
	/// <para>
	/// Additionally, if any file included in the outputs fileset does not exist, the 
	/// property will be set to true.</para>
    /// </remarks>
    /// <include file='Examples/Depends/Advanced.example' path='example'/>
    [TaskName("depends")]
    public class DependsTask : Task {

        FileSet _inputs = new FileSet();
        FileSet _outputs = new FileSet();
        string _propertyName;

        public DependsTask() {
            Inputs.FailOnMissingFile = true;
            Outputs.FailOnMissingFile = false;
        }

        /// <summary>The property name to set to hold the result of the dependency check.  The value in 
        /// this property after the task has run successfully will be either <c>true</c> or 
        /// <c>false</c>.</summary>
        [TaskAttribute("property", Required=true)]
        public string PropertyName {
            get { return _propertyName; }
            set { _propertyName = value; }
        }

        /// <summary>Set of input files to check against.</summary>
        [FileSet("inputs")]
        public FileSet Inputs {
            get { return _inputs; }
        }

        /// <summary>Set of output files to check against.</summary>
        [FileSet("outputs")]
        public FileSet Outputs {
            get { return _outputs; }
        }

        public static bool IsTaskNeedsRunning(Project project, FileSet inputs, FileSet outputs)
        {
            DependsTask depends = new DependsTask();
            depends.Project = project;

            depends.Inputs.IncludeWithBaseDir(inputs);
            depends.Outputs.IncludeWithBaseDir(outputs);

            return depends.TaskNeedsRunning();
        }

#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return PropertyName; }
		}
#endif

        /// <summary>Determine if the task needs to run.</summary>
        /// <returns><c>true</c> if we should run the program (dependents missing or not up to date), otherwise <c>false</c>.</returns>
        private bool TaskNeedsRunning()
        {

            if (Inputs.FileItems.Count < 1)
            {
                return true;
            }
            if (Outputs.FileItems.Count < 1)
            {
                return true;
            }

            // Find the newest input file timestamp
            DateTime newestInputFileDateStamp = new DateTime(1900, 1, 1);
            foreach (FileItem fileItem in Inputs.FileItems)
            {
                DateTime inputFileDateStamp = File.GetLastWriteTime(fileItem.FileName);
                if (inputFileDateStamp > newestInputFileDateStamp)
                {
                    newestInputFileDateStamp = inputFileDateStamp;
                }
            }

            // If any output file is older than the newest input file, we need to rebuild
            foreach (FileItem fileItem in Outputs.FileItems)
            {
                if (!File.Exists(fileItem.FileName))
                {
                    Log.Info.WriteLine("'{0}' does not exist.", fileItem.FileName);
                    return true;
                }

                DateTime outputDateStamp = File.GetLastWriteTime(fileItem.FileName);
                if (outputDateStamp < newestInputFileDateStamp)
                {
                    Log.Info.WriteLine(LogPrefix + "Output file {0} with timestamp {1} is older than the newest input file", fileItem.FileName, outputDateStamp.ToString());
                    return true;
                }

            }
            return false;
        }

        protected override void ExecuteTask() {
            if (TaskNeedsRunning()) {
                Project.Properties.Add(PropertyName, "true");
            } else {
                Project.Properties.Add(PropertyName, "false");
            }
        }
    }
}
