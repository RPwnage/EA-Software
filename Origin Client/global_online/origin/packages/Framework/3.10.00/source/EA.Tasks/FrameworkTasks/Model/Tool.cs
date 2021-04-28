using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
    public class Tool : CommandWithDependencies
    {
        public const uint TypeBuild = 1;
        public const uint TypePreCompile = 2;
        public const uint TypePreLink = 4;
        public const uint TypePostLink = 8;
        public const uint AppPackage = 16;
        public const uint TypeDeploy = 32;

        protected Tool(string toolname, PathString toolexe, uint type, string description = "",  PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
            : base(toolexe, new List<string>(), description, workingDir, env, createdirectories)
        {
            SetType(type);
            ToolName = toolname;            
            Templates = new Dictionary<string, string>();
        }
        
        protected Tool(string toolname, string script, uint type, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
            : base(script, description, workingDir, env, createdirectories)
        {
            SetType(type);
            ToolName = toolname;
            Templates = new Dictionary<string, string>();
        }
        
        public new List<string> Options 
        { 
             get {return base.Options as List<String>;}
        }

        public override string Message
        {
            get { return String.Format("      [{0}] {1}", ToolName, Description??String.Empty); }
        }

        public readonly string ToolName;
        public readonly Dictionary<string, string> Templates; // This property is for command line templates is used by build tasks.

    }
}
