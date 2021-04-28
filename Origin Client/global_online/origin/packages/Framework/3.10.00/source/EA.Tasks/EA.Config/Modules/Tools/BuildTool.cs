using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;


namespace EA.Eaconfig.Modules.Tools
{
    public class BuildTool : Tool
    {
        public BuildTool(Project project, string toolname, PathString toolexe, uint type, string description = "", PathString outputdir = null, PathString intermediatedir = null, IEnumerable<PathString> createdirectories = null, bool treatoutputascontent = false)
            : base(toolname, toolexe, type, description, workingDir:intermediatedir,
            env: Environment(project),
            createdirectories:createdirectories)
        {
            Files = new FileSet();
            OutputDir = outputdir;
            IntermediateDir = intermediatedir;
            TreatOutputAsContent = treatoutputascontent;
        }

        public BuildTool(Project project, string toolname, string script, uint type, string description = "", PathString outputdir = null, PathString intermediatedir = null, IEnumerable<PathString> createdirectories = null, bool treatoutputascontent = false)
            : base(toolname, script, type, description, workingDir: intermediatedir,
            env: Environment(project),
            createdirectories: createdirectories)
        {
            Files = new FileSet();
            OutputDir = outputdir;
            IntermediateDir = intermediatedir;
            TreatOutputAsContent = treatoutputascontent;
        }
        
        // Generation properties - Used in VS solution generation
        public readonly FileSet Files;  
        public readonly PathString OutputDir;
        public readonly PathString IntermediateDir;
        public readonly bool TreatOutputAsContent;


        private static IDictionary<string, string> Environment(Project project)
        {
            var optionset = project.NamedOptionSets["build.env"];
            return optionset == null ? null : optionset.Options;
        }

        internal override bool NeedsRunning(Project project)
        {
            FileSet indep = new FileSet();
            indep.IncludeWithBaseDir(InputDependencies);
            indep.Include(Files);
            return DependsTask.IsTaskNeedsRunning(project, indep, OutputDependencies);
        }
    }
}