//-----------------------------------------------------------------------------
// TargetTask.cs
//
// A NAnt custom task which allows you to nest <target> definitions within
// tasks, so you can add targets to a project dynamically.  This task was
// originally written by Gerry Shaw.  Bob Summerwill then added support for
// all <target> attributes and moved it into the NAnt core.
//
// Gerry Shaw (grshaw@ea.com)
// Bob Summerwill (bobsummerwill@ea.com)
//
// (c) 2004-2005 Electronic Arts Inc.
//-----------------------------------------------------------------------------

using System;
using System.IO;
using System.Xml;
//using System.Windows.Forms;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks {

    public class DynamicTarget : Target {
        public void SetXmlNode(XmlNode xmlNode) {
            _xmlNode = xmlNode;
            // useful for error messages
            IXmlLineInfo iLineInfo = xmlNode as IXmlLineInfo;
            if( null != iLineInfo && iLineInfo.HasLineInfo() )
            {
                _location = new Location(xmlNode.BaseURI, ((IXmlLineInfo)xmlNode).LineNumber, ((IXmlLineInfo)xmlNode).LinePosition);
            }
            else
            {
                Log.Status.WriteLine(LogPrefix + "Warning: Missing line info in xml node. There is likely a bug in the task code.  Please use a class which implements IXmlLineInfo.");
            }
        }
    }

	/// <summary>
	/// Create a dynamic target. 
	/// </summary>
	/// <remarks>
	/// <para>With Framework 1.x, you can declare &lt;taget&gt; within a &lt;project&gt;, but not within any
	/// task. Moreover, the target name can't be variable. But with TargetTask, you can declare a target with variable
	/// name, or within any task that supports probing. You declare a dynamic target, and call it usnig &lt;call&gt;.
	/// </para>
	/// </remarks>
	/// <include file='Examples/CreateTarget/simple.example' path='example'/>
    [TaskName("target")]
    public class TargetTask : Task {
        string _targetName;
		string _dependencyList;
		string _description;
		bool _ifDefined = true;
		bool _unlessDefined = false;
		bool _hidden = false;
		string _style;
		XmlNode _targetNode;
        bool _override = false;
        bool _allowoverride = false;

		/// <summary>The name of the target.</summary>
        [TaskAttribute("name", Required=true)]
        public string TargetName {
            get { return _targetName; }
            set { _targetName = value; }
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
		public override bool IfDefined
		{
			get { return _ifDefined; }
			set { _ifDefined = value; }
		}

		/// <summary>Prevents the target from being listed in the projecthelp. Default is false.</summary>
		[TaskAttribute("hidden")]
		public bool Hidden
		{
			get { return _hidden; }
			set { _hidden = value; }
		}

        /// <summary>
        /// Override target with the same name if it already exists. Default is 'false'. 
        /// Depends on the 'allowoverride' seeting in target it tries to override.
        /// </summary>
        [TaskAttribute("override")]
        public bool Override
        {
            get { return _override; }
            set { _override = value; }
        }

        /// <summary>Defines whether target can be overriden by other target with same name. Default is 'false'</summary>
        [TaskAttribute("allowoverride")]
        public bool AllowOverride
        {
            get { return _allowoverride; }
            set { _allowoverride = value; }
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
		public override bool UnlessDefined
		{
			get { return _unlessDefined; }
			set { _unlessDefined = value; }
		}


#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return TargetName; }
		}
#endif

        [XmlTaskContainer("*")]
		protected override void InitializeTask(XmlNode taskNode) 
		{
			_targetNode= taskNode;
		}

        protected override void ExecuteTask() 
		{
			if (Project.Properties["package.frameworkversion"] == "1")
			{
				throw new BuildException("Nonglobal <target> isn't supported in Framework 1.x packages.  This is a Framework 2.x feature.", Location);
			}
	
            DynamicTarget target = new DynamicTarget();
			target.BaseDirectory = Project.BaseDirectory;
			target.Name = TargetName;
			target.Hidden = Hidden;
			target.UnlessDefined = UnlessDefined;
			target.IfDefined = IfDefined;
			target.Project = Project;
            target.Override = Override;
            target.AllowOverride = AllowOverride;
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
