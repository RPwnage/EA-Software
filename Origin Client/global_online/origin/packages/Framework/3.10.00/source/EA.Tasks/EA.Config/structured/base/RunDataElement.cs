using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;


namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [ElementName("RunData", StrictAttributeCheck = true)]
    public class RunDataElement : Element
    {
        /// <summary>Sets the current working directory from which the executeable needs to run</summary>
        [TaskAttribute("workingdir", Required = true)]
        public string WorkingDir
        {
            get { return _workingdir.Value; }
            set { _workingdir.Value = value; }
        }

        /// <summary>Sets the command line arguments for an executeable</summary>
        [TaskAttribute("args", Required = true)]
        public string Args
        {
            get { return _args.Value; }
            set { _args.Value = value; }
        }

        /// <summary>Sets the startup program</summary>
        [TaskAttribute("startupprogram", Required = true)]
        public string StartupProgram
        {
            get { return _startupprogram.Value; }
            set { _startupprogram.Value = value; }
        }

        /// <summary></summary>
        [Property("workingdir", Required = false)]
        public ConditionalPropertyElement WorkingDirProp
        {
            get { return _workingdir; }
        }private ConditionalPropertyElement _workingdir = new ConditionalPropertyElement();

        /// <summary></summary>
        [Property("args", Required = false)]
        public ConditionalPropertyElement ArgsProp
        {
            get { return _args; }
        }private ConditionalPropertyElement _args = new ConditionalPropertyElement();

        /// <summary></summary>
        [Property("startupprogram", Required = false)]
        public ConditionalPropertyElement StartupProgramProp
        {
            get { return _startupprogram; }
        }private ConditionalPropertyElement _startupprogram = new ConditionalPropertyElement();

    }

}
