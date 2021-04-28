// NAnt - A .NET build tool
// Copyright (C) 2002-2003 Gerry Shaw
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
// File Maintainer
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections;
using System.IO;
using System.Reflection;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    /// <summary>A task that records the build's output to a property.</summary>
    /// <remarks>
    /// This task allows you to record the build's output, or parts of it to 
    /// a named property.  Using the <see cref="EchoTask"/> task you can output 
    /// the property to a file.
    /// </remarks>
    /// <include file='Examples/Record/Simple.example' path='example'/>
    [TaskName("record")]
    public class RecordTask : TaskContainer {

        string _propertyName;		
		bool _silent = false;

        /// <summary>Name of the property to record output.</summary>
        [TaskAttribute("property", Required=true)]
        public string PropertyName { 
            get { return _propertyName; } 
            set { _propertyName = value; } 
        }
		
        /// <summary>If set to true, no other output except of property is produced. Console or other logs are suppressed</summary>
        [TaskAttribute("silent", Required=false)]
        public bool Silent { 
            get { return _silent; } 
            set { _silent = value; } 
        }
		
        
        protected override void ExecuteTask() {
            StringLogger stringLogger = new StringLogger();
            try {
                using(LogListenersStack temp = new LogListenersStack(Silent, stringLogger))
                {
                    temp.RegisterLog(Log);
                    base.ExecuteTask();
                }
            } finally {
				
                Project.Properties.Add(PropertyName, stringLogger.Log.Trim());
            }
        }
    }

    /// <summary>Implementation of LogListener that writes information to a string.</summary>
    internal class StringLogger : ILogListener {
        StringBuilder _log = new StringBuilder();

        public string Log {
            get { return _log.ToString(); }
        }

        public string GetBuffer()
        {
            return _log.ToString();
        }


        public virtual void Write(string message) {
            _log.Append(message);
        }

        public virtual void WriteLine(string message) {
            _log.Append(message);
            _log.Append(Environment.NewLine);
        }
    }
}
