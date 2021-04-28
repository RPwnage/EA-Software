using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
    public enum NAntStackFrameType
    {
        NAnt,
        CSharp
    };

    public enum NAntStackScopeType
    {
        Task,
        Target,
        CSharp,
        Project
    };

    [Serializable]
    public class NAntStackFrame
    {
        public readonly NAntStackFrameType Type;
        public readonly String MethodName;
        public readonly NAntStackScopeType ScopeType;
        public readonly Location Location;

        public NAntStackFrame(NAntStackFrameType type, String methodName, Location location, NAntStackScopeType scopeType = NAntStackScopeType.CSharp)
        {
            Type = type;
            MethodName = methodName;
            ScopeType = scopeType;
            Location = location;
        }
    }
}