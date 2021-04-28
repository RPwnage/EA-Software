// NAnt - A .NET build tool
// Copyright (C) 2001 Gerry Shaw
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
// Ian MacLean (ian@maclean.ms)
using System;
using System.Xml;
using System.Threading;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    /// <summary>
    /// A task for sleeping a specified period of time, useful when a build or deployment process
    /// requires an interval between tasks. If none of the time attributes are specified then the task sleeps for 0 millseconds.
    /// </summary>
    /// <include file='Examples/Sleep/ShortSleep.example' path='example'/>
    /// <include file='Examples/Sleep/LongSleep.example' path='example'/>
    [TaskName("sleep")]
    public class SleepTask : Task   {
       
        int _hours = 0;
        int _minutes = 0;
        int _seconds = 0;
        int _milliseconds = 0;

        /// <summary>Hours to add to the sleep time.</summary>
        [TaskAttribute("hours"),Int32Validator(0,Int32.MaxValue)]
        public int Hours                  { get { return _hours; } set {_hours = value; } }
        
        /// <summary>Minutes to add to the sleep time.</summary>
        [TaskAttribute("minutes"),Int32Validator(0,Int32.MaxValue)]
        public int Minutes                { get { return _minutes; } set {_minutes = value; } }
        
        /// <summary>Seconds to add to the sleep time.</summary>
        [TaskAttribute("seconds"),Int32Validator(0,Int32.MaxValue)]
        public int Seconds                { get { return _seconds; } set {_seconds = value; } }
        
        /// <summary>Milliseconds to add to the sleep time.</summary>
        [TaskAttribute("milliseconds"),Int32Validator(0,Int32.MaxValue)]
        public int Milliseconds           { get { return _milliseconds; } set {_milliseconds = value; } }

        /// <summary>Return time to sleep.</summary>
        private int GetSleepTime() {
            return ((((int) Hours * 60) + Minutes) * 60 + Seconds) * 1000 + Milliseconds;
        }

        /// <summary>Sleep the specified number of milliseconds.</summary>
        /// <param name="millis">Milliseconds to sleep.</param>
        private void DoSleep(int millis ) {
            Thread.Sleep(millis);
        }

        /// <summary>
        ///  Verify parameters.
        /// </summary>
        /// <param name="taskNode"> taskNode used to define this task instance </param>
        protected override void InitializeTask(XmlNode taskNode) {
            if (GetSleepTime() < 0) {
                throw new BuildException("Negative sleep periods are not supported.", Location);
            }
        }

        protected override void ExecuteTask() {
            int sleepTime = GetSleepTime();
            Log.Status.WriteLine(LogPrefix + "Sleeping for {0} milliseconds", sleepTime);
            DoSleep(sleepTime);
        }
    }
}
