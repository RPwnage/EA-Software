// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
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
// Jay Turpin (jayturpin@hotmail.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.IO;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    /// <summary>
    /// Touch a file and/or fileset(s); corresponds to the Unix
	/// <i>touch</i> command.
    /// </summary>
	/// <remarks>If the file exists, <c>touch</c> changes the <i>last access</i> and <i>last write</i> timestamps to the current time.  If no file of that name exists, <c>touch</c> will create an empty file with that name, and set the <i>create</i>, <i>last access</i> and <i>last write</i> timestamps to the current time.
	/// </remarks>
    /// <include file='Examples/Touch/Touch1File.example' path='example'/>
    /// <include file='Examples/Touch/TouchDirectory.example' path='example'/>
    [TaskName("touch")]
    public class TouchTask : Task {
        
        string _file = null;
        string _millis = null;
        string _datetime = null;
        FileSet _fileset = new FileSet();

        /// <summary>Assembly Filename (required unless a fileset is specified).</summary>
        [TaskAttribute("file")]
        public string FileName  { get { return _file; } set { _file = value; } }

        /// <summary>
        /// Specifies the new modification time of the file in milliseconds since midnight Jan 1 1970.
		/// The FAT32 file system has limitations on the date value it can hold. The smallest is 
		/// <code>"12/30/1979 11:59:59 PM"</code>, and largest is <code>"12/30/2107, 11:59:58 PM"</code>.
        /// </summary>
        [TaskAttribute("millis")]
        public string Millis { 
            get { return _millis; } 
            set { _millis = value; } 
        }

        /// <summary>Specifies the new modification time of the file in the format MM/DD/YYYY HH:MM AM_or_PM.</summary>
        [TaskAttribute("datetime")]
        public string Datetime { 
            get { return _datetime; } 
            set { _datetime = value; } 
        }

        /// <summary>Fileset to use instead of single file.</summary>
        [FileSet("fileset")]
        public FileSet TouchFileSet { 
            get { return _fileset; } 
        }

        ///<summary>Initializes task and ensures the supplied attributes are valid.</summary>
        ///<param name="taskNode">Xml node used to define this task instance.</param>
        protected override void InitializeTask(System.Xml.XmlNode taskNode) {
            // limit task to either millis or a date string
            if (_millis != null && _datetime != null) {
                throw new BuildException("Cannot specify 'millis' and 'datetime'.", Location);
            }
        }

        protected override void ExecuteTask() {

            DateTime touchDateTime = DateTime.Now;
			if (_millis != null) 
			{
				try {
					touchDateTime = GetDateTime(Convert.ToDouble(_millis));
				} catch(Exception e) {
					string msg = String.Format("Could not convert time '{0}'.", _millis);
					throw new BuildException(msg, Location, e);
				}
			}

			if (_datetime != null) {
				touchDateTime = GetDateTime(_datetime);
			}

            // try to touch specified file
            if (FileName != null) {
                string path = null;
                try {
                    path = Project.GetFullPath(FileName);
                } catch (Exception e) {
					string msg = String.Format("Could not determine path from '{0}'.", FileName);
					throw new BuildException(msg, Location, e);
				}

                // touch file(s)
                TouchFile(path, touchDateTime);

            } else {
                // touch files in fileset
                // only use the file set if file attribute has NOT been set
                foreach (FileItem fileItem in TouchFileSet.FileItems) {
                    TouchFile(fileItem.FileName, touchDateTime);
                }
            }
        }

        void TouchFile(string path, DateTime touchDateTime) {
			bool fileCreated = false;
			try {
                Log.Info.WriteLine(LogPrefix + path);

                // create any directories needed
                Directory.CreateDirectory(Path.GetDirectoryName(path));

				if (!File.Exists(path)) {
                    FileStream f = File.Create(path);
                    f.Close();
					fileCreated = true;
                }

                // touch should set both write and access time stamp
				File.SetLastWriteTime(path, touchDateTime);
                File.SetLastAccessTime(path, touchDateTime);

            } catch (Exception e) {
				if(fileCreated == true && File.Exists(path)) {
					File.Delete(path);
				}
				string msg = String.Format("Could not touch file '{0}'.", path);
				throw new BuildException(msg, Location, e);
            }
        }

        private DateTime GetDateTime(string dateText){
            DateTime touchDateTime = new DateTime();
            if (dateText != "") {
                // 'touch' expects the following format: 'mm/dd/yy' - this is the US/Invariant one
                // if you don't specifically pass the format, DateTime.Parse assumes the current locale setting
                // for the date, which may be 'dd/mm/yy', as set in Regional Options control panel
                // on systems set to 'dd/mm/yy', <touch file="foo.obj" datetime="12/30/2000"/> will fail if using
                // DateTime.Parse(dateText) with 'String was not recognized as a valid DateTime',
                // since '30' is parsed as 'mm'
				touchDateTime = DateTime.Parse(dateText,
								System.Globalization.DateTimeFormatInfo.InvariantInfo,
								System.Globalization.DateTimeStyles.NoCurrentDateDefault);
            } else {
                touchDateTime = DateTime.Now;
            }
            return touchDateTime;
        }

        private DateTime GetDateTime(double milliSeconds) {
            DateTime touchDateTime = GetDateTime("01/01/1970 00:00:00");
			return touchDateTime.AddMilliseconds(milliSeconds);
        }
    }
}
