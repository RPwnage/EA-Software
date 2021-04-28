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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Xml;

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks {

    /// <summary>
    /// Calls a NAnt target.  BEWARE default 'force' behavior differs between Framework
    /// 1 and 2 packages.
    /// </summary>
	/// <include file='Examples\Call\Simple.example' path='example'/>
	/// <include file='Examples\Call\DebugRelease.example' path='example'/>
    [TaskName("call")]
    public class CallTask : Task {

        string _target = null;
		bool _force = false;
		bool _forceSetByFramework2Project = false;

        // Attribute properties
        /// <summary>NAnt target to call.</summary>
        [TaskAttribute("target", Required=true)]
        public string TargetName { get { return _target; } set { _target = value; }}

        /// <summary>Force an Execute even if the target 
        /// has already been executed.  Forced execution occurs on a copy of the unique
        /// target, since the original can only be executed once.
        /// Default value is false for a Framework 1 package,
        /// but true for a Framework 2 package.</summary>
        [TaskAttribute("force")]
        public bool ForceExecute 
		{ 
			set 
			{ 
				_force= value; 

				if (Project.Properties["package.frameworkversion"] == "2")
				{
					_forceSetByFramework2Project= true;
				}
			}
		}


#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return TargetName; }
		}
#endif

        protected override void ExecuteTask()
        {
            //init
            bool force = _force;

            try
            {
                //if in Framework 2 package and 'force' is not set by user
                if (Project.Properties["package.frameworkversion"] == "2" &&
                    false == _forceSetByFramework2Project)
                {
                    //default to true for a Framework 2 package if 'force' is not set by user
                    force = true;
                }
                if (force)
                {
                    Target t = Project.Targets.Find(_target);
                    if (t == null)
                    {
                        // if we can't find it, then neither should Project.Execute.
                        // Let them do the error handling and exception generation.
                        Project.Execute(_target);
                    }

                    //Execute a copy.
                    t.Copy().Execute(Project);

                }
                else
                {
                    Project.Execute(TargetName);
                }
            }
            catch (ContextCarryingException e)
            {
                //Reset location
                var frame = e.PopNAntStackFrame();
                e.PushNAntStackFrame(frame.MethodName, frame.ScopeType, Location);
                //throw e instead of throw to reset the stacktrace
                throw e;
            }
        }
    }
}
