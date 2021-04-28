using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;


namespace EA.Eaconfig.Modules.Tools
{
    public class PostLink : Tool
    {
        public PostLink(PathString toolexe)
            : base("link.postlink", toolexe, Tool.TypePostLink)
        {
        }
        public PostLink(string name, PathString toolexe)
            : base(name, toolexe, Tool.TypePostLink)
        {
        }

    }
}
