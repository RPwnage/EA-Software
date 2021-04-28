using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EA.FrameworkTasks.Model
{
    public class SourceControl
    {
        public enum Types { Perforce };

        public SourceControl(bool usingSourceControl = false, Types type = Types.Perforce)
        {
            UsingSourceControl = usingSourceControl;
            Type = type;
        }

        public readonly bool UsingSourceControl;

        public readonly Types Type;
    }
}
