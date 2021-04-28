// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// File Maintianers:
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Scott Hernandez (ScottHernandez@hotmail.com)
// William E. Caputo (wecaputo@thoughtworks.com | logosity@yahoo.com)

using System;
using System.IO;
using System.Xml;
using System.Collections.Specialized;

using NAnt.Core.Attributes;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core
{

    public class Target : Element, ICloneable, IComparable
    {
        //most of these members should be copied in copy constructor!
        protected XmlNode _xmlNode = null;
        string _name = null;
        string _desc = null;
        string _style = null;
        string _baseDirectory = null;
        bool _hasExecuted = false;
        bool _ifDefined = true;
        bool _unlessDefined = false;
        bool _hidden = false;
        bool _override = false;
        bool _allowoverride = false;
        StringCollection _dependencies = new StringCollection();
        PropertyStack _propertyStack = new PropertyStack("target.name");

        public Target()
        {
        }

        /// <summary>Copy constructor.</summary>
        /// <param name="t">The target to copy values from.</param>
        protected Target(Target t)
            : base((Element)t)
        {
            this._xmlNode = t._xmlNode;
            this._name = t._name;
            this._desc = t._desc;
            this._style = t._style;
            this._override = t._override;
            this._allowoverride = t._allowoverride;
            this._baseDirectory = t._baseDirectory;
            this._dependencies = t._dependencies;
            this._ifDefined = t._ifDefined;
            this._unlessDefined = t._unlessDefined;
            this._hasExecuted = false;
        }

        /// <summary>The name of the target.</summary>
        /// <remarks>
        ///   <para>Hides <see cref="Element.Name"/> to have <c>Target</c> return the name of target, not the name of Xml element - which would always be <c>target</c>.</para>
        ///   <para>Note: Properties are not allowed in the name.</para>
        /// </remarks>
        [TaskAttribute("name", Required = true, ExpandProperties = false)]
        public new string Name
        {
            get { return _name; }
            set { _name = value; }
        }

        /// <summary>If true then the target will be executed; otherwise skipped. Default is "true".</summary>
        [TaskAttribute("if")]
        public bool IfDefined
        {
            get { return _ifDefined; }
            set { _ifDefined = value; }
        }

        /// <summary>Opposite of if.  If false then the target will be executed; otherwise skipped. Default is "false".</summary>
        [TaskAttribute("unless")]
        public bool UnlessDefined
        {
            get { return _unlessDefined; }
            set { _unlessDefined = value; }
        }

        /// <summary>The Target description.</summary>
        [TaskAttribute("description")]
        public string Description
        {
            set { _desc = value; }
            get { return _desc; }
        }

        /// <summary>Framework 2 packages only: Style can be 'use', 'build', or 'clean'.  Default value is either 'use' or inherited from parent target.</summary>
        [TaskAttribute("style")]
        public string Style
        {
            set
            {
                if (Project.Properties["package.frameworkversion"] == "2")
                {
                    _style = value;
                }
                else
                {
                    Log.Warning.WriteLine("WARNING: <target> 'style' attribute not supported for Framework 1 packages.");
                }
            }
            get { return _style; }
        }

        /// <summary>A space seperated list of target names that this target depends on.</summary>
        [TaskAttribute("depends")]
        public string DependencyList
        {
            set
            {
                foreach (string str in value.Split(new char[] { ' ', ',', '\t', '\n' }))
                { // TODO: change this to just ' ' before 1.0
                    string dependency = str.Trim();
                    if (dependency.Length > 0)
                    {
                        Dependencies.Add(dependency);
                    }
                }
            }
        }

        /// <summary>Prevents the target from being listed in the projecthelp. Default is false.</summary>
        [TaskAttribute("hidden")]
        public bool Hidden
        {
            get { return _hidden; }
            set { _hidden = value; }
        }

        /// <summary>Override target with the same name if it already exists.</summary>
        [TaskAttribute("override")]
        public bool Override
        {
            get { return _override; }
            set { _override = value; }
        }

        /// <summary>Override target with the same name if it already exists.</summary>
        [TaskAttribute("allowoverride")]
        public bool AllowOverride
        {
            get { return _allowoverride; }
            set { _allowoverride = value; }
        }

        /// <summary>The base directory to use when executing tasks in this target.</summary>
        public string BaseDirectory
        {
            get { return _baseDirectory; }
            set { _baseDirectory = value; }
        }

        /// <summary>Indicates if the target has been executed.</summary>
        /// <remarks>
        ///   <para>Targets that have been executed will not execute a second time.</para>
        /// </remarks>
        public bool HasExecuted
        {
            get { return _hasExecuted; }
        }

        /// <summary>A collection of target names that must be executed before this target.</summary>
        public StringCollection Dependencies
        {
            get { return _dependencies; }
        }

        /// <summary>The xml used to initialize this Target.</summary>
        protected XmlNode TargetNode
        {
            get { return _xmlNode; }
        }

        public virtual int CompareTo(object obj)
        {
            return CompareTo((Target)obj);
        }

        public virtual int CompareTo(Target target)
        {
            return String.Compare(this.Name, target.Name);
        }


        /// <summary>The prefix used when sending messages to the log.</summary>
        public virtual string LogPrefix
        {
            get
            {
                string prefix = "[" + Name + "] ";
                return prefix.PadLeft(Log.IndentSize * 3);
            }
        }

        public bool TargetSuccess = true;

        /// <summary>Executes dependent targets first, then the target.</summary>
        public virtual void Execute(Project project)
        {
            if (!HasExecuted && IfDefined && !UnlessDefined)
            {
                // set right at the start or a <call> task could start an infinite loop
                _hasExecuted = true;

                using (Project targetContextProject = new Project(project))
                {
                    // add the target.name property
                    _propertyStack.Push(targetContextProject, Name);


                    if (targetContextProject.Properties["package.frameworkversion"] == "2")
                    {
                        if (Name == "build" || Name == "buildall")
                        {
                            targetContextProject.TargetStyle = Project.TargetStyleType.Build;
                        }
                        else if (Name == "clean" || Name == "cleanall")
                        {
                            targetContextProject.TargetStyle = Project.TargetStyleType.Clean;
                        }
                        // This else is to prevent Style overriding build/buildall/clean/cleanall
                        else if (Style != null)
                        {
                            if (Style == "build")
                            {
                                targetContextProject.TargetStyle = Project.TargetStyleType.Build;
                            }
                            else if (Style == "clean")
                            {
                                targetContextProject.TargetStyle = Project.TargetStyleType.Clean;
                            }
                            else if (Style == "use")
                            {
                                targetContextProject.TargetStyle = Project.TargetStyleType.Use;
                            }
                        }
                    }
#if BPROFILE
				NAnt.Core.Util.HiResTimer timer = new NAnt.Core.Util.HiResTimer() ;
				timer.Start() ;
				Console.WriteLine("%%\t{0}({1})\t<target>\t[{2}]", Location.FileName, Location.LineNumber, Name);
#endif
                    foreach (string targetName in Dependencies)
                    {
                        Target target;
                        if (targetContextProject.Targets.TryGetValue(targetName, out target) && target != null)
                        {
                            target.Execute(targetContextProject);
                        }
                        else                       
                        {
                            throw new BuildException(String.Format("Unknown dependent target '{0}' of target '{1}'", targetName, Name), Location);
                        }
                    }

                    try
                    {
                        targetContextProject.OnTargetStarted(new TargetEventArgs(this));
                        TargetSuccess = true;
                        targetContextProject.PushBaseDirectory(BaseDirectory);

                        Log.Info.WriteLine("   [target] {0}", Name);

                        // select all the task elements and execute them
                        foreach (XmlNode taskNode in _xmlNode)
                        {
                            if (taskNode.NodeType == XmlNodeType.Element)
                            {
                                Task task = targetContextProject.CreateTask(taskNode, this);
                                if (task != null)
                                {
                                    task.Execute();
                                }
                            }
                        }

                        // remove target.name property
                        _propertyStack.Pop(targetContextProject);

                    }
                    catch (BuildException e)
                    {
                        // if the exception was anything lower than a BuildException than its an internal error

                        // clear all properties on error
                        _propertyStack.Clear(targetContextProject);

                        TargetSuccess = false;

                        // If the exception doesn't have a location give it one.
                        if (e.Location == Location.UnknownLocation)
                        {
                            throw new BuildException(e.Message, Location, e.InnerException, e.StackTraceFlags);
                        }
                        throw;
                    }
                    catch (ContextCarryingException e)
                    {

                        // clear all properties on error
                        _propertyStack.Clear(targetContextProject);

                        TargetSuccess = false;

                        e.PushNAntStackFrame(Name, NAntStackScopeType.Target, Location);

                        //throw e instead of throw to reset the stacktrace
                        throw e;
                    }
                    catch (Exception e)
                    {
                        throw new ContextCarryingException(e, Name, Location);
                    }
                    finally
                    {

                        targetContextProject.OnTargetFinished(new TargetEventArgs(this));

                        targetContextProject.PopBaseDirectory();
                    }
                }
            }
        }

        /// <summary>
        /// Creates a deep copy by calling Copy().
        /// </summary>
        /// <returns></returns>
        object ICloneable.Clone()
        {
            return (object)Copy();
        }

        /// <summary>
        /// Creates a new (deep) copy.
        /// </summary>
        /// <returns>A copy with the _hasExecuted set to false. This allows the new Target to be Executed.</returns>
        public Target Copy()
        {
            return new Target(this);
        }

        protected override void InitializeElement(XmlNode elementNode)
        {
            _xmlNode = elementNode;
        }

        /// <summary>The list of tasks within this target.</summary>
        public TaskCollection EnumerateTasks()
        {
            TaskCollection collection = null;

            foreach (XmlNode taskNode in _xmlNode)
            {
                if (taskNode.NodeType == XmlNodeType.Element)
                {
                    Task task = Project.CreateTask(taskNode, this);
                    if (task != null)
                    {
                        if (collection == null)
                            collection = new TaskCollection();

                        collection.Add(task);
                    }
                }
            }

            return collection;
        }
    }
}
