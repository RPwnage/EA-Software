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
    /// <summary>
    /// Contains 'runtime' modules in structured XML. Module definitions inside this tag will belong to 'runtime' group. 
    /// runtime group elent is optional, structured XML modules belong to 'runtime' group by default.
    /// </summary>
    [TaskName("runtime")]
    public class RuntimeTask : BuildGroupBaseTask
    {
        public RuntimeTask() : base("runtime") { }
    }

    /// <summary>
    /// Contains 'test' modules in structured XML. Module definitions inside this tag will belong to 'test' group.
    /// </summary>
    [TaskName("tests")]
    public class TestsTask : BuildGroupBaseTask
    {
        public TestsTask() : base("test") { }
    }

    /// <summary>
    /// Contains 'example' modules in structured XML. Module definitions inside this tag will belong to 'example' group.
    /// </summary>
    [TaskName("examples")]
    public class ExamplesTask : BuildGroupBaseTask
    {
        public ExamplesTask() : base("example") { }
    }

    /// <summary>
    /// Contains 'tool' modules in structured XML. Module definitions inside this tag will belong to 'tool' group.
    /// </summary>
    [TaskName("tools")]
    public class ToolsTask : BuildGroupBaseTask
    {
        public ToolsTask() : base("tool") { }
    }


}
