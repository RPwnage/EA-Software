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
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.IO;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    /// <summary>Writes a message to the build log.</summary>
    /// <remarks>
    ///   <para>A copy of the message will be sent to every defined
	/// build log and logger on the system.  Property references in the message will be expanded.</para>
    /// </remarks>
    /// <include file='Examples\Echo\HelloWorld.example' path='example'/>
    /// <include file='Examples\Echo\Macro.example' path='example'/>
    /// <include file='Examples\Echo\FileAppend.example' path='example'/>
    [TaskName("echo", Mixed = true)]
    public class EchoTask : Task {

        string _message  = null;
        string _fileName = null;
        bool   _append   = false;

        /// <summary>The message to display.  For longer messages use the inner text element of the task.</summary>
        [TaskAttribute("message", Required=false, ExpandProperties=false)]
        public string Message {
            get { return _message; }
            set { _message = value; }
        }

        /// <summary>The name of the file to write the message to.  If empty write message to log.</summary>
        [TaskAttribute("file", Required=false)]
        public string FileName {
            get { return _fileName; }
            set { _fileName = value; }
        }

        /// <summary>Indicates if message should be appended to file.  Default is "false".</summary>
        [TaskAttribute("append", Required=false)]
        public bool Append
        {
            get { return _append; }
            set { _append = value; }
        }

#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo {
			get { return Message.Substring(0, Math.Min(20, Message.Length)); }
		}
#endif

        /// <summary>Initializes the task.</summary>
        protected override void InitializeTask(XmlNode taskNode) {
            if (Message != null && taskNode.InnerText.Length > 0) {
                throw new BuildException("Cannot specify a message attribute and element value, use one or the other.", Location);
            }

            // use the element body if the message attribute was not used.
            if (Message == null) {
                if (taskNode.InnerText.Length == 0) {
                    throw new BuildException("Need to specify a message attribute or have an element body.", Location);
                }
                Message = Project.ExpandProperties(taskNode.InnerText);
            }
        }

        protected override void ExecuteTask() {
			Message = Project.ExpandProperties(Message);
            if (FileName == null) {
                if (Project.TraceEcho)
                {
                    Message = string.Format(LogPrefix + "{0} {1}", this.Location, Message);
                }
                else
                {
                    Log.Status.WriteLine(LogPrefix + Message);
                }

            } else {
                string path = Project.GetFullPath(FileName);
                StreamWriter streamWriter = null;
                try {
                    streamWriter = new StreamWriter(new FileStream(path, Append ? FileMode.Append : FileMode.Create, FileAccess.Write));
                    streamWriter.WriteLine(Message);

                } catch (Exception e) {
                    string msg = string.Format("Error writing to file '{0}'.", path);
                    throw new BuildException(msg, Location, e);

                } finally {
                    if (streamWriter != null) {
                        streamWriter.Close();
                    }
                }
            }
        }
    }
}
