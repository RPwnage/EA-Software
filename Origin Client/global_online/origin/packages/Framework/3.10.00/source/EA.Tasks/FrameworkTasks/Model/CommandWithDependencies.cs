using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;


namespace EA.FrameworkTasks.Model
{
    public class CommandWithDependencies : Command
    {
        public CommandWithDependencies(string script, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
            : base(script, description, workingDir, env, createdirectories)
        {
            InputDependencies = new FileSet();
            OutputDependencies = new FileSet();
        }

        public CommandWithDependencies(PathString executable, IEnumerable<string> options, string description = "", PathString workingDir = null, IDictionary<string, string> env = null, IEnumerable<PathString> createdirectories = null)
            : base(executable, options, description, workingDir, env, createdirectories)
        {
            InputDependencies = new FileSet();
            OutputDependencies = new FileSet();
// Why is the executable path an input dependency?
// For example, if we add this, then what do we do with the WriteX360DeploymentTool function?
// Because as that function is currently written, it expects InputDependencies to be the DeploymentFiles
//            InputDependencies.Include(Executable.Path);
        }

        public readonly FileSet InputDependencies;
        public readonly FileSet OutputDependencies;

        internal virtual bool NeedsRunning(Project project)
        {
            return DependsTask.IsTaskNeedsRunning(project, InputDependencies, OutputDependencies);
        }

    }
}
