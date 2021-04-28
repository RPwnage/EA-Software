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
using NAnt.Core.Writers;
using NAnt.Core.Events;
using Model = EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
    [TaskName("winrt-vc-postprocess")]
    class WinRT_VC_Postprocess : PC_VC_PostprocessOptions 
    {

        protected override string DefaultVinver
        {
            get { return "0x0602"; }
        }

        public override void Process(Module_Native module)
        {
            base.Process(module);

        }
    }


}
