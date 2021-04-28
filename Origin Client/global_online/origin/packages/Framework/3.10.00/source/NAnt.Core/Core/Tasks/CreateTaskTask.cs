// Copyright (C) 2004-2005 Electronic Art
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;
using NAnt.Core.Events;

namespace NAnt.Core.Tasks
{

    /// <summary>
    /// Executes a task created by &lt;createtask&gt;. 
    /// </summary>
    /// <remarks>
    /// 
    /// <para>Use &lt;createtask&gt; to create a new 'task' that is made of Nant 
    /// script.  Then use the &lt;task&gt; task to call that task with the required parameters.</para>
    /// 
    /// <para>Each parameter may be either an attribute (eg. Message in example below) or a nested text element
    /// (eg. DefList in example below) that allows multiple lines of text.  Multiple lines may be needed for certain task elements like
    /// &lt;includedirs>, &lt;usingdirs>, and &lt;defines> in &lt;cc>.	</para>
    /// </remarks>
    /// <include file='Examples/CreateTask/simple.example' path='example'/>
    [TaskName("task", StrictAttributeCheck = false)]
    public class RunTask : Task
    {
        string _taskName;
        OptionSet _parameterValues = new OptionSet();
        XmlElement _codeElement;

        /// <summary>The name of the task being declared.</summary>
        [TaskAttribute("name", Required = true)]
        public string TaskName
        {
            get { return _taskName; }
            set { _taskName = value; }
        }

        /// <summary>
        /// The parameters to be passed to &lt;createtask&gt;
        /// </summary>
        public OptionSet ParameterValues
        {
            get { return _parameterValues; }
        }

        protected override void InitializeTask(XmlNode taskNode)
        {
            base.InitializeTask(taskNode);

            string declarationPropertyName = String.Format("Task.{0}", TaskName);
            string paramOptionSetName = String.Format("Task.{0}.Parameters", TaskName);

            string declaration = Project.Properties[declarationPropertyName];
            if (declaration == null)
            {
                string msg = String.Format("Missing property '{0}'.  Task '{1}' has not been declared.", declarationPropertyName, TaskName);
                throw new BuildException(msg);
            }

            OptionSet parameterTypes;
            if (!Project.NamedOptionSets.TryGetValue(paramOptionSetName, out parameterTypes))
            {
                string msg = String.Format("Missing option set '{0}'.  Task '{1}' has not been declared.", paramOptionSetName, TaskName);
                throw new BuildException(msg);
            }

            // look up parameter option set and set parameters for this instance
            foreach (KeyValuePair<string,string> entry in parameterTypes.Options)
            {
                string name = entry.Key;
                string value = entry.Value;

                // check if parameter is passed in the XmlAttributes
                XmlAttribute parameterAttribute = (XmlAttribute)taskNode.Attributes[name];
                if (parameterAttribute != null)
                {
                    value = parameterAttribute.Value;
                }
                else
                {
                    // check if parameter is in child element
                    bool found = false;
                    foreach (XmlNode child in taskNode.ChildNodes)
                    {
                        if (child.Name == name)
                        {
                            value = child.InnerText.Trim();
                            found = true;
                        }
                    }
                    if (!found && value == "Required")
                    {
                        string msg = String.Format("'{0}' is a required attribute/element for task '{1}'.", name, TaskName);
                        throw new BuildException(msg);
                    }
                }

                ParameterValues.Options.Add(name, Project.ExpandProperties(value));
            }
            _codeElement = (XmlElement)Project.UserTasks[TaskName]; 
        }

        protected override void ExecuteTask()
        {
            if (Project.Properties["package.frameworkversion"] == "1")
            {
                throw new BuildException("<task> isn't supported in Framework 1.x packages.  This is a Framework 2.x feature.", Location);
            }

            Log.Debug.WriteLine(LogPrefix + "Running task '{0}'", TaskName);

            using (Project taskContextProject = new Project(Project, propagateLocalProperties:true))
            {
                taskContextProject.OnUserTaskStarted(new TaskEventArgs(this));

                // set properties for each param
                foreach (KeyValuePair<string,string> entry in ParameterValues.Options)
                {
                    string name = String.Format("{0}.{1}", TaskName, entry.Key);
                    string value = (string)(entry.Value);

                    if (taskContextProject.Properties[name] != null)
                    {
                        string msg = String.Format("Property '{0}' exists but should not.", name);
                        throw new BuildException(msg);
                    }
                    taskContextProject.Properties.Add(name, value, false, false, true);
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
                                Task task = taskContextProject.CreateTask(node, null);
                                if (task != null)
                                {
                                    task.Execute();
                                }
                            }
                        }
                    }
                    catch (ContextCarryingException e)
                    {
                        e.PushNAntStackFrame(TaskName, NAntStackScopeType.Task, Location);
                        //throw e instead of throw to reset the stacktrace
                        throw e;
                    }
                    catch (BuildException e)
                    {
                        throw new ContextCarryingException(e, Name);
                    }
                }
                finally
                {
                    taskContextProject.OnUserTaskFinished(new TaskEventArgs(this));
                }
            }
        }



#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return TaskName; }
		}
#endif
    }

    /// <summary>Create a task that is made of Nant script and can be used by &lt;task&gt;. <b>This task is a Framework 2
    /// feature.</b>
    /// </summary>
    /// <remarks>
    /// 
    /// <para>Use the &lt;createtask&gt; to create a new 'task' that is made of Nant 
    /// script.  Then use the &lt;task&gt; task to call that task with the required parameters.</para>
    /// 
    /// <para>The following unique properties will be created, for use by &lt;task>:</para>
    /// <list type="bullet">
    /// <item><b>Task.{name}</b></item>
    /// <item><b>Task.{name}.Code</b></item>
    /// </list>
    /// The following named optionset will be created, for use by &lt;task>:
    /// <list type="bullet">
    /// <item><b>Task.{name}.Parameters</b></item>
    /// </list>
    /// </remarks>
    /// <example>
    ///     <code>
    /// &lt;createtask name="BasicTask">
    ///    &lt;parameters>
    ///        &lt;option name="DummyParam" value="Required"/>
    ///        &lt;option name="Indentation"/>
    ///    &lt;/parameters>
    ///    &lt;code>
    ///        &lt;echo message="${BasicTask.Indentation}Start BasicTask."/>
    ///        &lt;echo message="${BasicTask.Indentation}    DummyParam value = ${BasicTask.DummyParam}"/>
    ///        &lt;echo message="${BasicTask.Indentation}Finished BasicTask."/>
    ///    &lt;/code>
    /// &lt;/createtask>
    ///     </code>
    /// </example>
    /// <include file='Examples/CreateTask/simple.example' path='example'/>
    [TaskName("createtask")]
    public class CreateTaskTask : Task
    {
        string _taskName;
        XmlPropertyElement _code = new XmlPropertyElement();
        OptionSet _parameters = new OptionSet();

        /// <summary>The name of the task being declared.</summary>
        [TaskAttribute("name", Required = true)]
        public string TaskName
        {
            get { return _taskName; }
            set { _taskName = value; }
        }

        /// <summary>Overload existing definition.</summary>
        [TaskAttribute("overload", Required = false)]
        public bool Overload
        {
            get { return _overload; }
            set { _overload = value; }
        } private bool _overload = false;


        /// <summary>The set of task parameters.</summary>
        [OptionSet("parameters")]
        public OptionSet Parameters
        {
            get { return _parameters; }
        }

        /// <summary>
        /// The Nant script this task is made out from
        /// </summary>
        [Property("code")]
        public XmlPropertyElement Code
        {
            get { return _code; }
            set { _code = value; }
        }

#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return TaskName; }
		}
#endif

        protected override void InitializeTask(XmlNode taskNode)
        {
            // find the <Code> element and get all the xml without expanding any text
            XmlElement codeElement = (XmlElement)taskNode.GetChildElementByName("code");
            if (codeElement == null)
            {
                string msg = String.Format("Missing required <code> element.");
                throw new BuildException(msg);
            }
            Project.UserTasks[TaskName] = codeElement;
        }


        protected override void ExecuteTask()
        {
            if (Project.Properties["package.frameworkversion"] == "1")
            {
                throw new BuildException("<createtask> isn't supported in Framework 1.x packages.  This is a Framework 2.x feature.", Location);
            }
            Log.Debug.WriteLine(LogPrefix + "Adding task '{0}'", TaskName);

            string declarationPropertyName = String.Format("Task.{0}", TaskName);
            string paramOptionSetName = String.Format("Task.{0}.Parameters", TaskName);

            // ensure properties and option set don't exist
            var declarationLocation = Project.Properties[declarationPropertyName];
            if (declarationLocation != null)
            {
                if (!Overload)
                {
                    string msg = String.Format("Task '{0}' aready defined in {1}.", TaskName, declarationLocation);
                    throw new BuildException(msg, Location);
                }
                else
                {
                    Log.Debug.WriteLine("Task '{0}' defined at {1} is overloaded from {2}.", TaskName, declarationLocation, Location.ToString());
                }
            }


            if (Project.NamedOptionSets.Remove(paramOptionSetName))
            {
                if (!Overload)
                {
                    string msg = String.Format("Option Set '{0}' exists.", paramOptionSetName);
                    throw new BuildException(msg);
                }
            }

            // ensure Code XML is valid

            // create optionset for task parameter list
            Project.NamedOptionSets.Add(paramOptionSetName, Parameters);

            if (Overload && Project.Properties.Contains(declarationPropertyName))
            {
                Project.Properties.UpdateReadOnly(declarationPropertyName, Location.ToString() + "Task Overloaded");
                return;
            }

            // create property for task name
            Project.Properties.AddReadOnly(declarationPropertyName, Location.ToString() + "Task Declared");
        }


    }
}
