using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using NAnt.Core.Events;
using EA.FrameworkTasks;

using System;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System.Xml;
using System.Reflection;

namespace EA.Eaconfig
{
    public class TaskUtil
    {
        public static void Dependent(Project project, string name, bool initialize)
        {
            XmlElement dependentTaskElement = xmldoc.CreateElement("dependent");
            dependentTaskElement.SetAttribute("name", name);
            targetEl.RemoveAll();
            targetEl.AppendChild(dependentTaskElement);

            DependentTask dependentTask = new DependentTask();
            dependentTask.Project = project;
            dependentTask.Verbose = project.Verbose;
            dependentTask.PackageName = name;
            dependentTask.InitializeScript = initialize;
            dependentTask.Initialize(dependentTaskElement);

            dependentTask.Execute();
        }

        public static void Dependent(Project project, string name)
        {
            Dependent(project, name, true);
        }

        public static void Dependent(Project project, string name, Project.TargetStyleType style)
        {
            Project.TargetStyleType currentStyle = project.TargetStyle;

            try
            {
                project.TargetStyle = style;
                Dependent(project, name);
            }
            finally
            {
                project.TargetStyle = currentStyle;
            }
        }

        public static void Dependent(Project project, string name, Project.TargetStyleType style, bool initialize)
        {
            Project.TargetStyleType currentStyle = project.TargetStyle;

            try
            {
                project.TargetStyle = style;
                Dependent(project, name, initialize);
            }
            finally
            {
                project.TargetStyle = currentStyle;
            }
        }

        public static void ExecuteScriptTask(Project project, string name, string paramName, string paramValue)
        {
            IDictionary<string, string> parameters = new Dictionary<string, string>() { { paramName, paramValue } };

            ExecuteScriptTask(project, name, parameters);
        }

        public static void ExecuteScriptTask(Project project, string name, IDictionary<string, string> parameters)
        {
            EaconfigRunTask task = new EaconfigRunTask(project, name, parameters);

            task.Init();

            task.Execute();
        }

        // Helper class for RunScriptTask:
        internal class EaconfigRunTask : RunTask
        {
            internal EaconfigRunTask(Project project, string name, IDictionary<string, string> parameters)
            {
                Project = project;
                TaskName = name;
                _parameters = parameters;
            }

            internal void Init()
            {
                string declarationPropertyName = String.Format("Task.{0}", TaskName);
                string paramOptionSetName = String.Format("Task.{0}.Parameters", TaskName);
            

                string declaration = Project.Properties[declarationPropertyName];
                if (declaration == null) {
                    string msg = String.Format("Missing property '{0}'.  Task '{1}' has not been declared.", declarationPropertyName, TaskName);
                    throw new BuildException(msg);
                }

                OptionSet parameterTypes = Project.NamedOptionSets[paramOptionSetName];
                if (parameterTypes == null) {
                    string msg = String.Format("Missing option set '{0}'.  Task '{1}' has not been declared.", paramOptionSetName, TaskName);
                    throw new BuildException(msg);
                }

                // look up parameter option set and set parameters for this instance
                foreach (DictionaryEntry entry in parameterTypes.Options)
                {
                    string name = (string)(entry.Key);
                    string value = (string)(entry.Value);

                    // check if parameter is passed in the input:
                    string parValue;
                    if (_parameters.TryGetValue(name, out parValue))
                    {
                        value = parValue;
                    }
                    else if (value == "Required")
                    {
                        string msg = String.Format("'{0}' is a required attribute/element for task '{1}'.", name, TaskName);
                        throw new BuildException(msg);
                    }

                    ParameterValues.Options.Add(name, Project.ExpandProperties(value));
                }
            }

            protected override void ExecuteTask()
            {
                XmlElement _codeElement = (XmlElement)Project.UserTasks[TaskName];

                if (Project.Properties["package.frameworkversion"] == "1")
                {
                    throw new BuildException("<task> isn't supported in Framework 1.x packages.  This is a Framework 2.x feature.");
                }

                Log.WriteLineIf(Project.Verbose, Project.LogPrefix + "Running task '{0}'", TaskName);


                Project.OnUserTaskStarted(new TaskEventArgs(this));

                // set properties for each param
                foreach (DictionaryEntry entry in ParameterValues.Options)
                {
                    string name = String.Format("{0}.{1}", TaskName, entry.Key);
                    string value = (string)(entry.Value);

                    if (Project.Properties[name] != null)
                    {
                        string msg = String.Format("Property '{0}' exists but should not.", name);
                        throw new BuildException(msg);
                    }
                    Project.Properties.Add(name, value);
                }

                try
                {
                    try
                    {
                        // run tasks (taken from Target.cs in nant)
                        foreach (XmlNode node in _codeElement)
                        {
                            if (node.NodeType == XmlNodeType.Element)
                            {
                                Task task = Project.CreateTask(node, null);
                                if (task != null)
                                {

                                    task.Execute();
                                }
                            }
                        }
                    }
                    catch (BuildException e)
                    {
                        throw new BuildException("", Location.UnknownLocation, e);
                    }
                }
                finally
                {
                    // remove properties for each param
                    foreach (DictionaryEntry entry in ParameterValues.Options)
                    {
                        string name = String.Format("{0}.{1}", TaskName, entry.Key);
                        if (!Project.Properties.Contains(name))
                        {
                            string msg = String.Format("Property '{0}' does not exist but should.", name);
                            throw new BuildException(msg);
                        }
                        Project.Properties.Remove(name);
                    }
                    Project.OnUserTaskFinished(new TaskEventArgs(this));
                }
            }

            private IDictionary<string, string> _parameters;
        }

        static TaskUtil()
        {
            xmldoc = new XmlDocument();
            targetEl = xmldoc.CreateElement("target");
            targetEl.SetAttribute("name", "dummy");

            taskEl = xmldoc.CreateElement("task");
            taskEl.SetAttribute("name", "dummy");
            
        }

        private static XmlDocument xmldoc;
        private static XmlElement targetEl;
        private static XmlElement taskEl;
    }

}


