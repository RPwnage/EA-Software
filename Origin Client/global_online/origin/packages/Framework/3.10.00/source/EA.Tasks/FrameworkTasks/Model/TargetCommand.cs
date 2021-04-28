using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;


namespace EA.FrameworkTasks.Model
{
    public class TargetCommand
    {
        public TargetCommand(string target, TargetInput targetInput, bool nativeNantOnly)
        {
            Target = target;
            TargetInput = targetInput;
            NativeNantOnly = nativeNantOnly;
        }

        public readonly string Target;
        public readonly TargetInput TargetInput;
        public readonly bool NativeNantOnly;
    }
}
