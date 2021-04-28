using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;


namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [ElementName("BuildTarget", StrictAttributeCheck = true, Mixed = true)]
    public class BuildTargetElement : Element
    {
        private TargetElement _target;

        public BuildTargetElement(string defaultTargetName)
            : base()
        {
            _target = new TargetElement(defaultTargetName);
        }

        /// <summary>Sets the target name</summary>
        [TaskAttribute("targetname")]
        public string TargetName 
        { 
            get { return _targetname; } 
            set { _targetname = value; }
        } private string _targetname;

        /// <summary>Autoconvert target to command when needed in case command is not defined. Default is 'true'</summary>
        [TaskAttribute("nant-build-only")]
        public bool NantBuildOnly
        {
            get { return _nantbuildonly;  }
            set { _nantbuildonly = value; }
        } private bool _nantbuildonly = false;

        /// <summary>Sets the target</summary>
        [Property("target", Required = false)]
        public TargetElement Target
        {
            get { return _target; }
        }

        /// <summary>Sets the comand</summary>
        [Property("command", Required = false)]
        public ConditionalPropertyElement Command
        {
            get { return _command; }
        }
        private ConditionalPropertyElement _command = new ConditionalPropertyElement();

        protected override void InitializeElement(XmlNode node)
        {
            base.InitializeElement(node);

            if(Target.HasTargetElement && !String.IsNullOrEmpty(TargetName))
            {
                Error.Throw(Project, Location, Name, "Can not define both TargetName[{0}] and <target> element.", TargetName);
            }
        }
    }

    /// <summary>Create a dynamic target. This task is a Framework 2 feature</summary>
    [ElementName("target")]
    public class TargetElement : Element {
        string _targetName;
        string _dependencyList;
        string _description;
        bool _ifDefined = true;
        bool _unlessDefined = false;
        bool _hidden = true;
        string _style;
        XmlNode _targetNode;

        public TargetElement(string targetName) : base()
        {
            _targetName = targetName;
        }
        public string TargetName {
            get { return _targetName; }
        }

        /// <summary>A space seperated list of target names that this target depends on.</summary>
        [TaskAttribute("depends")]
        public string DependencyList {
            get { return _dependencyList; }
            set { _dependencyList = value; }
        }

        /// <summary>The Target description.</summary>
        [TaskAttribute("description")]
        public string Description 
        {
            get { return _description; }
            set { _description = value; }
        }

        /// <summary>If true then the target will be executed; otherwise skipped. Default is "true".</summary>
        [TaskAttribute("if")]
        public bool IfDefined
        {
            get { return _ifDefined; }
            set { _ifDefined = value; }
        }

        /// <summary>Prevents the target from being listed in the projecthelp. Default is true.</summary>
        [TaskAttribute("hidden")]
        public bool Hidden
        {
            get { return _hidden; }
            set { _hidden = value; }
        }

        /// <summary>Style can be 'use', 'build', or 'clean'.   See 'Auto Build Clean' 
        /// page in the Reference/NAnt/Fundamentals section of the help doc for details.</summary>
        [TaskAttribute("style")]
        public string Style 
        {
            get { return _style; }
            set { _style = value; }
        }

        /// <summary>Opposite of if.  If false then the target will be executed; otherwise skipped. Default is "false".</summary>
        [TaskAttribute("unless")]
        public bool UnlessDefined
        {
            get { return _unlessDefined; }
            set { _unlessDefined = value; }
        }

        public bool HasTargetElement
        {
            get { return _targetNode != null; }
        }

        public override void Initialize(XmlNode elementNode)
        {
            _ifDefined = true;
            _unlessDefined = false;
            base.Initialize(elementNode);
        }

        protected override void InitializeElement(XmlNode elementNode) 
        {
            _targetNode= elementNode;
        }

        public void Execute(string targetName) 
        {
            if (Project.Properties["package.frameworkversion"] == "1")
            {
                throw new BuildException("Nonglobal <target> isn't supported in Framework 1.x packages.  This is a Framework 2.x feature.", Location);
            }

            if (!string.IsNullOrEmpty(targetName))
            {
                _targetName = targetName;
            }
    
            DynamicTarget target = new DynamicTarget();
            target.BaseDirectory = Project.BaseDirectory;
            target.Name = TargetName;
            target.Hidden = Hidden;
            target.UnlessDefined = UnlessDefined; 
            target.IfDefined = IfDefined;
            target.Project = Project;
            if (DependencyList != null) 
            {
                target.DependencyList = DependencyList;
            }
            if (Description != null) 
            {
                target.Description = Description;
            }
            if (Style != null) 
            {
                target.Style = Style;
            }

            target.SetXmlNode(_targetNode);
            Project.Targets.Add(target);
        }
    }
}
