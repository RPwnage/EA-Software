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
// File Mainatiners
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian@maclean.ms)

using System;
using System.IO;
using System.Xml;
using System.Collections;
using System.Collections.Specialized;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks
{

    /// <summary>Include an build file.</summary>
    /// <remarks>
    ///   <para>This task is used to break your build file into smaller chunks.  
    ///   You can load a partial build file and have it included into the main 
    ///   build file.</para>
    ///   <note>Any global (project level) tasks in the included build file are 
    ///   executed when this task is executed.  Tasks in target elements of the 
    ///   included build file are only executed if that target is executed.</note>
    ///   <note>The project element attributes in an included build
    ///   file are ignored.</note>
    ///   <note>If this task is used within a target, the include included file
    ///   should not have any targets (or include files with targets).  Doing so 
    ///   would compromise NAnt's knowledge of available targets.</note>
    /// </remarks>
    /// <include file='Examples/Include/Include.example' path='example'/>
    [TaskName("include")]
    public class IncludeTask : Task
    {

        string _fileName = null;
        bool _ignoremissing = false;

        public IncludeTask() : base("include") { }

        /// <summary>Build file to include.</summary>
        [TaskAttribute("file", Required = true)]
        public string FileName
        {
            get { return _fileName; }
            set { _fileName = Project.GetFullPath(value); }
        }

        /// <summary>Ignore if file does not exist</summary>
        [TaskAttribute("ignoremissing", Required = false)]
        public bool IgnoreMissing
        {
            get { return _ignoremissing; }
            set { _ignoremissing = value; }
        }

        public static void IncludeFile(Project project, PathString file)
        {
            if (file != null)
            {
                IncludeFile(project, file.Path);
            }
        }

        public static void IncludeFile(Project project, string file)
        {
            IncludeTask includeTask = new IncludeTask();
            includeTask.Project = project;
            includeTask.Verbose = project.Verbose;
            includeTask.FileName = file;
            includeTask.Execute();
        }

#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return FileName; }
		}
#endif

        protected override void ExecuteTask()
        {
            // Check if I am already included in the project.
            if (Project.IncludedFiles.Contains(FileName))
            {
                return;
            }

            try
            {
                if (IgnoreMissing) 
                {
                    if (!File.Exists(FileName))
                    {
                        return;
                    }
                }

                Log.Debug.WriteLine(LogPrefix + FileName);

                XmlDocument doc = LineInfoDocument.Load(FileName);

                using (Project fileLocalContextProject = new Project(Project))
                {
                    fileLocalContextProject.IncludeBuildFileDocument(doc);
                }
            }
            catch (Exception e)
            {
                throw new ContextCarryingException(e, Name, Location);
            }
        }
    }
}
