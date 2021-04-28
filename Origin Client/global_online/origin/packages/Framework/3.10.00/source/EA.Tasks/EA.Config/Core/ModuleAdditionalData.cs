using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;


namespace EA.Eaconfig.Core
{
    // Data used for debugging, various generations, etc.
    public class ModuleAdditionalData
    {
        public DebugData DebugData;
        public GeneratorTemplatesData GeneratorTemplatesData;
    }


    public class DebugData
    {
        public static DebugData CreateDebugData(string executable, string options, string workingDir, IDictionary<string, string> env, string remoteMachine, bool? enableUnmanagedDebugging = null)
        {
            if (String.IsNullOrEmpty(executable) && String.IsNullOrEmpty(options) && String.IsNullOrEmpty(workingDir) && String.IsNullOrEmpty(remoteMachine) && enableUnmanagedDebugging == null)
            {
                return null;                
            }
            return new DebugData(executable, options, workingDir, env, remoteMachine, enableUnmanagedDebugging);
        }

        private DebugData(string executable, string options, string workingDir, IDictionary<string, string> env, string remoteMachine, bool? enableUnmanagedDebugging)
        {
            Command = new Command(new PathString(executable), new string[] { options }, String.Empty, new PathString(workingDir), env);
            RemoteMachine = remoteMachine;
            EnableUnmanagedDebugging = enableUnmanagedDebugging??false;
        }

        public readonly Command Command;
        public readonly string RemoteMachine;
        public readonly bool EnableUnmanagedDebugging; // For .Net projects
    }

    public class GeneratorTemplatesData
    {
        public GeneratorTemplatesData(string slnDirTemplate = null, string slnNameTemplate = null, string projectDirTemplate = null, string projectFileNameTemplate = null)
        {
            SlnDirTemplate = slnDirTemplate ?? String.Empty;
            SlnNameTemplate = slnNameTemplate ?? String.Empty;
            ProjectDirTemplate = projectDirTemplate ?? String.Empty;
            ProjectFileNameTemplate = projectFileNameTemplate ?? String.Empty;
        }

        public static GeneratorTemplatesData CreateGeneratorTemplateData(Project project, BuildGroups buildgroup)
        {
            return new GeneratorTemplatesData
                    (
                        slnDirTemplate: project.Properties[buildgroup + ".gen.slndir.template"].TrimWhiteSpace(),
                        slnNameTemplate: project.Properties[buildgroup + ".gen.slnname.template"].TrimWhiteSpace(),
                        projectDirTemplate: project.Properties[buildgroup + ".gen.projectdir.template"].TrimWhiteSpace(),
                        projectFileNameTemplate: project.Properties[buildgroup + ".gen.projectname.template"].TrimWhiteSpace()
                    );
        }

        public readonly string SlnDirTemplate;
        public readonly string SlnNameTemplate;
        public readonly string ProjectDirTemplate;
        public readonly string ProjectFileNameTemplate;

    }
}
