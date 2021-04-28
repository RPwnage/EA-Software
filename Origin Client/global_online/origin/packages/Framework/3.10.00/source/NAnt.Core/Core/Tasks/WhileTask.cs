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
// File Maintainer
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.IO;

using NAnt.Core.Attributes;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks {

    /// <summary>Allows wrapping of a group of tasks to be repeatedly executed based on a conditional.</summary>
    /// <include file='Examples\While\Simple.example' path='example'/>
    [TaskName("while")]
    public class WhileTask : TaskContainer {

        string _condition = null;

        /// <summary>The expression used to test for termination criteria.</summary>
        [TaskAttribute("condition", Required=true, ExpandProperties=false)]
        public string Condition { 
            get { return _condition; }
            set {_condition = value; }
        }

        protected override void ExecuteTask() {
            try 
            {                
                while(Expression.Evaluate(Project.ExpandProperties(Condition)))
                {
                    ExecuteChildTasks();                    
                }
            } 
            catch (BuildException e) {
                // if build exception is missing a location give it one
                if (e.Location == Location.UnknownLocation) {
                    e = new BuildException(e.Message, Location, e.InnerException, e.StackTraceFlags);
                }
                throw e;
            }
        }
    }
}
