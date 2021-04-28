using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;


namespace EA.Eaconfig.Core
{
    public interface IBuildModuleTask
    {
        ProcessableModule Module { get; set; }
        Tool Tool { get; set; }
        OptionSet BuildTypeOptionSet { get; set; }
    }
}
