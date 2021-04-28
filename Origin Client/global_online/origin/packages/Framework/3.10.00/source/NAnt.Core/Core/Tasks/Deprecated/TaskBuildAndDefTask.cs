using System;
using System.Linq;
using System.Collections.Specialized;
using System.IO;
using System.Reflection;
using System.Xml;
using System.CodeDom;
using System.CodeDom.Compiler;


using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;

namespace NAnt.Core.Tasks 
{
    [TaskName("taskbuilddef")]
    public class TaskBuildAndDefTask : TaskDefTask
    {
        /// <summary>
        /// This attribute is deprecated, for backwars compatibility only.
        /// </summary>
        [TaskAttribute("referenceconfigassemby", Required = false)]
        public bool ReferenceConfigAssembly
        {
            get { return false; }
            set { }
        }

        public TaskBuildAndDefTask()
            : base()
        {
            
        }
    }
}
