using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;


namespace EA.FrameworkTasks.Model
{
    public class Command : BitMask
    {
        public const uint ShellScript = 16384;
        public const uint Program     = 32768;
        public const uint ExcludedFromBuild = 65536;

        public readonly string Script;
        public readonly PathString Executable;
        public readonly IEnumerable<string> Options;
        public readonly PathString WorkingDir;
        public readonly IDictionary<string, string> Env;
        public readonly IList<PathString> CreateDirectories;
        public readonly string Description;

        public Command(string script, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
            : base(ShellScript)
        {
#if FRAMEWORK_PARALLEL_TRANSITION
            Script = script;
#else
            Script = script.NormalizeNewLineChars();
#endif
            Executable = GetScriptExecutable();
            Options = GetScriptOptions();
            WorkingDir = workingDir;
            Env = env;
            CreateDirectories = createdirectories == null ? new List<PathString>() : new List<PathString>(createdirectories);
            Description = description.NormalizeNewLineChars();
        }

        public Command(PathString executable, IEnumerable<string> options, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
            : base(Program)
        {
            Executable = executable;
            Options = options;
            WorkingDir = workingDir;
            Env = env;
            CreateDirectories = createdirectories == null ? new List<PathString>() : new List<PathString>(createdirectories); ;
            Description = description;
        }

        public string CommandLine
        {
            get
            {
                if (IsKindOf(Program))
                {
                    return Executable.Path + " " + Options.ToString(" ");
                }
                else
                {
                    return Script;
                }
            }
        }

        private List<string> GetScriptOptions()
        {
            var options = new List<string>();
            switch(PlatformUtil.Platform)
            {
                case PlatformUtil.Windows:
                    options.Add("/c");
                    options.Add(Script);
                    break;
                case PlatformUtil.Unix:
                case PlatformUtil.OSX:
                    options.Add(Script);
                    break;
                default:
                    break;
            }
            return options;
        }

        private PathString GetScriptExecutable()
        {
            string exec = "undefined";
            switch (PlatformUtil.Platform)
            {
                case PlatformUtil.Windows:
                    exec = "cmd";
                    break;
                case PlatformUtil.Unix:
                case PlatformUtil.OSX:
                    exec = "bash";
                    break;
                default:
                    break;
            }
            return new PathString(exec);
        }

        public virtual string Message
        {
            get { return Description ?? Executable.Path; }
        }

    }
}
