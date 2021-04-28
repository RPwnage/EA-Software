using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
    [TaskName("all-platforms-preprocess")]
    class All_Platforms_PreprocessOptions : AbstractModuleProcessorTask
    {
        public override void Process(Module_Native module)
        {
        }
    }


    [TaskName("all-platforms-postprocess")]
    class All_Platforms_PostprocessOptions : AbstractModuleProcessorTask
    {
        public override void Process(Module_Native module)
        {
        }
    }

}
