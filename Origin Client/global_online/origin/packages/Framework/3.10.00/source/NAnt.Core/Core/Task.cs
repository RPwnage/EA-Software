using System;
using System.Xml;
using System.Collections.Generic;

using NAnt.Core.Events;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;


namespace NAnt.Core
{
    public abstract class Task : Element
    {
        bool _failOnError = true;
        bool _verbose = false;
        bool _ifDefined = true;
        bool _unlessDefined = false;
        bool _taskSuccess = true;

        //public const string TaskSuccessProperty = "task.success";

        public Task() : base() { }

        public Task(string name) : base(name) { }


        /// <summary>Determines if task failure stops the build, or is just reported. Default is "true".</summary>
        [TaskAttribute("failonerror")]
        public virtual bool FailOnError
        {
            get { return _failOnError; }
            set { _failOnError = value; }
        }

        /// <summary>Task reports detailed build log messages.  Default is "false".</summary>
        [TaskAttribute("verbose")]
        public virtual bool Verbose
        {
            get { return (_verbose || Project.Verbose); }
            set { _verbose = value; }
        }

        /// <summary>If true then the task will be executed; otherwise skipped. Default is "true".</summary>
        [TaskAttribute("if")]
        public virtual bool IfDefined
        {
            get { return _ifDefined; }
            set { _ifDefined = value; }
        }

        /// <summary>Opposite of if.  If false then the task will be executed; otherwise skipped. Default is "false".</summary>
        [TaskAttribute("unless")]
        public virtual bool UnlessDefined
        {
            get { return _unlessDefined; }
            set { _unlessDefined = value; }
        }

        /// <summary>Dummy atribute for bacwards compatibility.</summary>
        [TaskAttribute("nopackagelog")]
        public bool NoPackageLog
        {
            set
            {
                if (Log.WarningLevel >= Log.WarnLevel.Deprecation)
                {
                    Log.Warning.WriteLine("at {0} Task attribute 'nopackagelog' is deprecated and has no effect", Location.ToString());
                }
            }
        }

        /// <summary>Returns true if the task succeded.</summary>
        public bool TaskSuccess
        {
            get { return _taskSuccess; }
            set { _taskSuccess = value; }
        }

        /// <summary>The prefix used when sending messages to the log.</summary>
        public virtual string LogPrefix
        {
            get
            {
                string prefix = "[" + Name + "] ";
                return prefix.PadLeft(prefix.Length + Log.IndentSize);
            }
        }

        /// <summary>Executes the task unless it is skipped.</summary>
        public void Execute()
        {
            if (IfDefined && !UnlessDefined)
            {
                try
                {
                    Project.OnTaskStarted(new TaskEventArgs(this));

                    // initialize error conditions
                    TaskSuccess = true;

                    ExecuteTask();

                }
                catch (BuildException e)
                {
                    // if the exception was anything lower than a BuildException than its an internal error
                    bool continueOnFail = ConvertUtil.ToBoolean(Project.Properties.ExpandProperties(Project.Properties[Project.NANT_PROPERTY_CONTINUEONFAIL]));

                    // update error conditions
                    TaskSuccess = false;
                    Project.LastError = e;

                    if (FailOnError && !continueOnFail)
                    {
                        // If contiue on Fail is false we throw, otherwise we have to go on.
                        // Local FailOnError properties override.
                        // If the exception doesn't have a location give it one.
                        if (e.Location == Location.UnknownLocation)
                        {
                            e = new BuildException(e.Message, Location, e.InnerException, e.Type, e.StackTraceFlags);
                        }
                        throw new ContextCarryingException(e, Name);
                    }
                    else
                    {
                        // If local FailOnError is false, we just print the message.
                        // If continueOnFail is true and we are here and need to also set the global success flag.
                        if (continueOnFail)
                        {
                            Project.GlobalSuccess = false;
                        }
                        Log.Error.WriteLine(e.Message);
                        if (e.InnerException != null)
                        {
                            Log.Error.WriteLine(e.InnerException.Message);
                        }
                    }
                }
                catch (ContextCarryingException e)
                {
                    ContextCarryingException newException = e.PropagateException(this);
                    if (newException != null)
                        throw newException;
                    else
                        throw;
                }
                catch (Exception e)
                {
                    throw new ContextCarryingException(e, Name, Location);
                }
                finally
                {
                    Project.OnTaskFinished(new TaskEventArgs(this));
                    // Updating this property has huge perfomance hit on Mono. Do not update it.
                    //Project.Properties.UpdateReadOnly(TaskSuccessProperty, TaskSuccess.ToString());
                }
            }

        }

        protected override void InitializeElement(XmlNode elementNode)
        {
            if (IfDefined && !UnlessDefined)
            {
                // Just defer for now so that everything just works
                InitializeTask(elementNode);
            }
        }

        /// <summary>Initializes the task and checks for correctness.</summary>
        protected virtual void InitializeTask(XmlNode taskNode)
        {
        }


        protected bool OptimizedTaskElementInit(string name, string value)
        {
            bool ret = true;
            switch (name)
            {
                case "failonerror": // REq
                    _failOnError = ElementInitHelper.InitBoolAttribute(this, name, value);
                    break;
                case "verbose":  // ExpandProperties = false
                    _verbose = ElementInitHelper.InitBoolAttribute(this, name, value);
                    break;
                case "if":
                    _ifDefined = ElementInitHelper.InitBoolAttribute(this, name, value);
                    break;
                case "unless":
                    _unlessDefined = ElementInitHelper.InitBoolAttribute(this, name, value);
                    break;
                default:
                    ret = false;
                    break;
            }
            return ret;
        }


        /// <summary>Executes the task.</summary>
        protected abstract void ExecuteTask();


    }
}
