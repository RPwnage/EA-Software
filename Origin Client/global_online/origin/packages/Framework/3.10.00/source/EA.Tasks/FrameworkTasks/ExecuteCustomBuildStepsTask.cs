// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;
using System.Linq;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;

namespace EA.FrameworkTasks
{
    /// <summary>Executes a named custom build step.</summary>
    /// <remarks>
    /// <para>
    /// Note: This task is intended for internal use within eaconfig to allow 
    /// certain build steps to be called at the beginning or end of specific
    /// eaconfig targets.
    /// </para>
    /// <para>
    /// Executes a custom build step which may either consist of nant tasks or
    /// batch/shell script commands.
    /// </para>
    /// <para>
    /// The custom build step is defined using the property 
    /// runtime.[module].vcproj.custom-build-tool.
    /// </para>
    /// </remarks>
    [TaskName("ExecuteCustomBuildSteps")]
    public class ExecuteCustomBuildSteps : Task
    {
        private string _groupname = null;

        /// <summary>The name of the module whose build steps we want to execute.</summary>
        [TaskAttribute("groupname", Required = true)]
        public string GroupName { get { return _groupname; } set { _groupname = value; } }

        protected override void InitializeTask(XmlNode taskNode)
        {
            
        }

        protected string PropGroupName(string name)
        {
            return String.Format("{0}.{1}", GroupName, name);
        }

        protected string PropGroupValue(string name, string defVal = "")
        {
            string val = null;
            if(!String.IsNullOrEmpty(name))
            {
                val = Project.Properties[PropGroupName(name)];
            }
            return val ?? defVal;
        }

        private FileSet PropGroupFileSet(string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                return null;
            }
            return Project.NamedFileSets[PropGroupName(name)];
        }

        protected override void ExecuteTask()
        {
            string customcmdline = PropGroupValue("vcproj.custom-build-tool");
            if (!String.IsNullOrWhiteSpace(customcmdline))
            {
                BuildStep step = new BuildStep("custombuild", BuildStep.None);
                step.Commands.Add(new Command(customcmdline));
                step.OutputDependencies = PropGroupValue("vcproj.custom-build-outputs").LinesToArray().Select(file => PathString.MakeNormalized(file)).OrderedDistinct().ToList();
                step.InputDependencies = PropGroupValue("vcproj.custom-build-dependencies").LinesToArray().Select(file => PathString.MakeNormalized(file)).OrderedDistinct().ToList();
                var inputs = PropGroupFileSet("vcproj.custom-build-dependencies");
                if (inputs != null)
                {
                    step.InputDependencies.AddRange(inputs.FileItems.Select(fi => fi.Path));
                }

                Project.ExecuteBuildStep(step);
            }
        }
    }
}

