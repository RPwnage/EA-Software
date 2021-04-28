using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("runtime")]
    public class RuntimeTask : BuildGroupBaseTask
    {
        public RuntimeTask() : base("runtime") { }
    }

    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("tests")]
    public class TestsTask : BuildGroupBaseTask
    {
        public TestsTask() : base("test") { }
    }

    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("examples")]
    public class ExamplesTask : BuildGroupBaseTask
    {
        public ExamplesTask() : base("example") { }
    }

    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("tools")]
    public class ToolsTask : BuildGroupBaseTask
    {
        public ToolsTask() : base("tool") { }
    }


}
