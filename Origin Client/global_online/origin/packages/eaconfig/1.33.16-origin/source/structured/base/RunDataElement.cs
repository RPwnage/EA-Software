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
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [ElementName("RunData", StrictAttributeCheck = true)]
    public class RunDataElement : Element
    {
        [NAnt.Core.Attributes.Description("Sets the current working directory from which the executeable needs to run")]
        [TaskAttribute("workingdir", Required = true)]
        public string WorkingDir
        {
            get { return _workingdir.Value; }
            set { _workingdir.Value = value; }
        }

        [NAnt.Core.Attributes.Description("Sets the command line arguments for an executeable")]
        [TaskAttribute("args", Required = true)]
        public string Args
        {
            get { return _args.Value; }
            set { _args.Value = value; }
        }

        [NAnt.Core.Attributes.Description("Sets the startup program")]
        [TaskAttribute("startupprogram", Required = true)]
        public string StartupProgram
        {
            get { return _startupprogram.Value; }
            set { _startupprogram.Value = value; }
        }

        [NAnt.Core.Attributes.Description("")]
        [Property("workingdir", Required = false)]
        public ConditionalPropertyElement WorkingDirProp
        {
            get { return _workingdir; }
        }private ConditionalPropertyElement _workingdir = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("")]
        [Property("args", Required = false)]
        public ConditionalPropertyElement ArgsProp
        {
            get { return _args; }
        }private ConditionalPropertyElement _args = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("")]
        [Property("startupprogram", Required = false)]
        public ConditionalPropertyElement StartupProgramProp
        {
            get { return _startupprogram; }
        }private ConditionalPropertyElement _startupprogram = new ConditionalPropertyElement();

    }

}
