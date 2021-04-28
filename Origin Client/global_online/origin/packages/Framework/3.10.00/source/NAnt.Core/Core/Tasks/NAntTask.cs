// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Scott Hernandez (ScottHernandez@hotmail.com)

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Xml;
using System.Text.RegularExpressions;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
//using NAnt.Core.Metrics;
using NAnt.Core.PackageCore;
using System.Reflection;

namespace NAnt.Core.Tasks
{

    /// <summary>Runs NAnt on a supplied build file.</summary>
    /// <remarks>
    ///     <para>This task can be used to build subprojects which have their own full build files.  See the
    ///     <see cref="DependsTask"/> for a good example on how to build sub projects only once per build.</para>
    /// </remarks>
    [TaskName("nant")]
    public class NAntTask : Task , IDisposable
    {
        //private const int DefaultIndentLevel = 1;
        private const int DefaultIndentLevel = 0;  // In framework 3 with parallel processing and non recursive model does not make sense to use ident on nant task invocation.
        private const string ProfilePropertyName = "profile";

        private string _buildFileName = null;
        private string _target = null;
        private string _optionSet = null;
        private int? _indentLevel = null;
        private GlobalPropertiesActionTypes _globalpropertiesaction = GlobalPropertiesActionTypes.propagate;
        private bool _startNewBuild = true;

        private Release _packageInfo = null;
        private Project _nestedProject = null;
        private XmlNode _taskNode = null;

        public enum GlobalPropertiesActionTypes { propagate, initialize };

        /// <summary>The name of the *.build file to use.</summary>
        [TaskAttribute("buildfile", Required=true)]
        public string BuildFileName {
            get { return _buildFileName; }
            set { _buildFileName = value; }
        }

        /// <summary>The target to execute.  To specify more than one target seperate targets with a space.  Targets are executed in order if possible.  Default to use target specified in the project's default attribute.</summary>
        [TaskAttribute("target")]
        public string DefaultTarget {
            get { return _target; }
            set { _target = value; }
        }

        /// <summary>The name of an optionset containing a set of properties to be passed into the supplied build file.</summary>
        [TaskAttribute("optionset")]
        public string OptionSetName {
            get { return _optionSet; }
            set { _optionSet = value; }
        }

        /// <summary>The log IndentLevel. Default is the current log IndentLevel + 1.</summary>
        [TaskAttribute("indent")]
        public int IndentLevel {
            get {
                if (_indentLevel == null) {
                    return Log.IndentLevel + DefaultIndentLevel;
                }
                return (int)_indentLevel;
            }
            set { _indentLevel = value; }
        }

        /// <summary>
        /// Defines how global properties are set up in the dependent project. 
        /// Valid values are <b>propagate</b> and <b>initialize</b>. Default is 'propagate'.
        /// </summary>
        /// <remarks>
        /// <para>
        /// 'propagate' means standard global properties propagation.
        /// </para>
        /// <para>
        /// when value is 'initialize' global properties in dependent project are set using values from masterconfig and nant command line
        /// the same way they are set at the start of nant process. Usuall used in combination with 'start-new-build'
        /// </para>
        /// </remarks>
        [TaskAttribute("global-properties-action", Required = false)]
        public GlobalPropertiesActionTypes GlobalPropertiesAction
        {
            get { return _globalpropertiesaction; }
            set { _globalpropertiesaction = value; }
        }

        /// <summary>
        /// Start new build graph in the dependent project. Default is 'false'
        /// </summary>
        /// <remarks>
        /// Normally dependent project is added to the current build graph. Setting 'start-new-build' 
        /// to true will create new build graph in dependent project. This is useful when nant task is used to invoke separate independent build.
        /// </remarks>
        [TaskAttribute("start-new-build", Required = false)]
        public bool StartNewBuild
        {
            get { return _startNewBuild; }
            set { _startNewBuild = value; }
        }

        public string PackageName
        {
            get { return _packageInfo == null ? String.Empty : _packageInfo.Name; }
        }

#if BPROFILE
        /// <summary>Return additional info identifying the task to the BProfiler.</summary>
        public override string BProfileAdditionalInfo
        {
            get { return BuildFileName + "-" + DefaultTarget; }
        }
#endif

        public Project NestedProject {
            get { return _nestedProject; }
            set { _nestedProject = value; }
        }

        [XmlElement("property", "NAnt.Core.Tasks.PropertyTask", AllowMultiple = true, Description = "Property to pass to the child nant invocation.")]
        protected override void InitializeTask(XmlNode taskNode)
        {
            _taskNode = taskNode;
            DoInitialize();
        }


        public void DoInitialize()
        {
            BuildFileName = Project.GetFullPath(BuildFileName);

            SetPackageInfo(BuildFileName);

            NestedProject = new Project(BuildFileName, Verbose? Log.LogLevel.Diagnostic :Log.Level, IndentLevel, Project);

            lock (Log.LogListenerStacks)
            {
                foreach (var listenerstack in Log.LogListenerStacks)
                {
                    listenerstack.RegisterLog(NestedProject.Log);
                }
            }
            NestedProject.Verbose = Verbose;

            //IMTODO
            //MetricsSetup.SetProject(NestedProject);

            PropagateProperty(ProfilePropertyName);
            PropagateProperty(Project.NANT_PROPERTY_PROJECT_MASTERCONFIGFILE);
            PropagateProperty(Project.NANT_PROPERTY_PROJECT_BUILDROOT);
            PropagateProperty(Project.NANT_PROPERTY_PROJECT_TEMPROOT);
            PropagateProperty(Project.NANT_PROPERTY_PROJECT_PACKAGEROOTS);
            var commanLine = PropagateOptionset(Project.NANT_OPTIONSET_PROJECT_CMDPROPERTIES);

            // initialize the property dictionary from property elements
            var localCommandLine = new HashSet<string>();
            if(_taskNode != null)
            {
                foreach (XmlNode propertyNode in _taskNode)
                {
                    if(propertyNode.Name.Equals("property"))
                    {
                        PropertyTask task = Project.TaskFactory.CreateTask(propertyNode, Project) as PropertyTask;
                        task.Initialize(propertyNode);
                        task.ReadOnly = true;
                        task.Project = NestedProject;
                        task.Execute();

                        if (StartNewBuild)
                        {
                            // Add current command line properties.
                            if (task.IfDefined && !task.UnlessDefined)
                            {
                                commanLine.Options[task.PropertyName] =task.Value;
                                localCommandLine.Add(task.PropertyName);
                            }
                        }
                    }
                }
            }

            // initialize the property dictionary from optionset
            if (OptionSetName != null)
            {
                OptionSet optionSet;
                if (Project.NamedOptionSets.TryGetValue(OptionSetName, out optionSet))
                {
                    foreach (KeyValuePair<string,string> entry in optionSet.Options)
                    {
                        string propName = Project.ExpandProperties(entry.Key);
                        string propValue = Project.ExpandProperties(entry.Value);
                        NestedProject.Properties.AddReadOnly(propName, propValue);

                        if (StartNewBuild)
                        {
                            if (!localCommandLine.Contains(propName))
                            {
                                commanLine.Options[propName] = propValue;
                            }
                        }
                    }
                }
                else
                {
                    string msg = String.Format("Unknown option set '{0}'.", OptionSetName);
                    throw new BuildException(msg, Location);
                }
            }

            if (GlobalPropertiesAction == GlobalPropertiesActionTypes.propagate)
            {
                // Passing global/group properties to the nested project associated with this nant task.

                if (!String.IsNullOrEmpty(PackageName))
                {
                    Release release = PackageMap.Instance.GetMasterRelease(PackageName, Project);
                    if (release != null)
                    {
                        if (release.IsMasterRelease)
                        {
                            foreach (string propertyName in release.Package.MasterPackage.Properties)
                            {
                                Project proj = Project;
                                while ((null != proj) && !PropagateProperty(propertyName, proj))
                                {
                                    // The project didn't have the property that we want to propagate defined.
                                    // This is more likely now with parallel building as we can't control (via ordering) who actually performs the dependent task.
                                    // For example, if package A depends on package B and C, and package B also depends on package C, we can't control whether
                                    // the processing of package A or package B ends up triggering the processing of package C.  Hence, if in the masterconfig.xml
                                    // we pass a property via the package <properties> tag, give the property a value in package A, it might not be propagated down to
                                    // package C.  It used to be we could control this by listing package C before B in the masterconfig, but now this is not the case.
                                    // So, instead, we now track each project's parent and walk up the project tree to see if the property we want is defined anywhere above.
                                    proj = proj.ParentProject;
                                }
                            }

                            // Passing down the group properties that is specific to this package
                            MasterConfig.GroupType group = release.Package.MasterPackage.EvaluateGrouptype(Project);
                            foreach (string propertyName in group.GroupProperties)
                            {
                                Project proj = Project;
                                while ((null != proj) && !PropagateProperty(propertyName, proj))
                                {
                                    proj = proj.ParentProject;
                                }
                            }
                        }
                    }
                }

                // Passing down the Global properties
                foreach (Project.GlobalPropertyDef gprop in Project.GlobalProperties.EvaluateExceptions(Project))
                {
                    PropagateProperty(gprop.Name, Project, gprop.IsReadonly, gprop.InitialValue);
                }
            }
            else if (GlobalPropertiesAction == GlobalPropertiesActionTypes.initialize)
            {
                // First need to set global properties that were passed through command line as readonly:
                OptionSet commandlineproperties;
                if (NestedProject.NamedOptionSets.TryGetValue(Project.NANT_OPTIONSET_PROJECT_CMDPROPERTIES, out commandlineproperties))
                {
                    foreach (var prop in commandlineproperties.Options)
                    {
                        if (!localCommandLine.Contains(prop.Key))
                        {
                            NestedProject.Properties.AddReadOnly(prop.Key, prop.Value);
                        }
                    }
                }

                // Set global properties from Masterconfig:
                foreach (Project.GlobalPropertyDef propdef in Project.GlobalProperties.EvaluateExceptions(Project))
                {
                    if (propdef.InitialValue != null && !NestedProject.Properties.Contains(propdef.Name))
                    {
                        // Top level project has all global properties except properties passed from the command line as writable.
                        NestedProject.Properties.Add(propdef.Name, propdef.InitialValue, readOnly: false);
                    }
                }
            }

            if (!StartNewBuild)
            {
                // Passing down the Global Named Objects
                lock (Project.GlobalNamedObjects)
                {
                    foreach (Guid id in Project.GlobalNamedObjects)
                    {
                        object obj;
                        if (Project.NamedObjects.TryGetValue(id, out obj))
                        {
                            NestedProject.NamedObjects[id] = obj;
                        }
                    }
                }
            }

            //IMTODO: do I need to copy all user tasks from parent?
            foreach (KeyValuePair<string,XmlNode> ent in Project.UserTasks)
            {
                NestedProject.UserTasks[ent.Key]= ent.Value;
            }

            NestedProject.BuildTargetNames.AddRange(StringUtil.ToArray(DefaultTarget));

        }

        protected override void ExecuteTask()
        {

            try {
                Log.Info.WriteLine(LogPrefix + "{0} {1}", BuildFileName, DefaultTarget);

                if (!NestedProject.Run()) {
                    throw new BuildException("Recursive build failed.");
                }
            } finally {
                try {
                    if (NestedProject != null)
                        _nestedProject.Dispose();
                        _nestedProject = null;
                } catch {
                }
            }
        }

        // This function is the same as ExecuteTask() except not disposing the project after execution.
        // Used in dependent task.
        //IMTODO - would be nice to get rid of it and insert dependency into parent package inside child <package> task.
        public void ExecuteTaskNoDispose()
        {
            try
            {
                if (!NestedProject.Run())
                {
                    throw new BuildException("Recursive build failed.");
                }
            }
            finally
            {
            }
        }

        protected void SetPackageInfo(string buildFileName)
        {
            if (!String.IsNullOrEmpty(buildFileName))
            {
                string buildDir = Path.GetDirectoryName(buildFileName);
                string packageName = Path.GetFileNameWithoutExtension(buildFileName);
                _packageInfo = PackageMap.Instance.Releases.FindByNameAndDirectory(packageName, buildDir);
            }
        }

        protected bool PropagateProperty(string name, Project project = null, bool readOnly = true, string initialval = null)
        {
            bool propagated = false;

            if (null == project)
            {
                project = Project;
            }

            string val = project.Properties[name] ?? initialval;
            if (val != null && !NestedProject.Properties.IsPropertyReadOnly(name))
            {
                NestedProject.Properties.Add(name, val, readOnly);
                propagated = true;
            }

            return (propagated);
        }

        protected OptionSet PropagateOptionset(string name)
        {
            OptionSet cmdlineoptions = null;
            OptionSet value;
            if (Project.NamedOptionSets.TryGetValue(name, out value))
            {
                cmdlineoptions = new OptionSet(value);
                cmdlineoptions.Project = NestedProject;
            }
            else
            {
                cmdlineoptions = new OptionSet();
            }
            NestedProject.NamedOptionSets.Add(name, cmdlineoptions);

            return cmdlineoptions;
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing)
                {
                    // Dispose managed resources
                    if (_nestedProject != null)
                    {
                        try
                        {
                            _nestedProject.Dispose();
                            _nestedProject = null;
                        }
                        catch { }
                    }
                }
            }
            _disposed = true;
        }

        private bool _disposed = false;

    }
}
