using NAnt.Core;
using NAnt.Core.Attributes;
using EA.FrameworkTasks;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace EA.Eaconfig
{
    [TaskName("EaconfigBuildTask")]
    public abstract class EaconfigBuildTask : EAConfigTask
    {

        [TaskAttribute("groupname", Required = true)]
        public string GroupName
        {
            get { return _groupName; }
            set { _groupName = value; }
        }

        [TaskAttribute("buildmodule", Required = true)]
        public string BuildModule
        {
            get { return _buildModule; }
            set { _buildModule = value; }
        }

        // eaconfig.build.group        
        public string BuildGroup
        {
            get
            {
                if (_buildGroup == null)
                {
                    _buildGroup = Project.Properties["eaconfig.build.group"];
                }
                return _buildGroup;
            }
        }

        // eaconfig.build.group.groupname        
        public string BuildGroupName
        {
            get
            {
                if (_buildGroupName == null)
                {
                    _buildGroupName = Project.Properties["eaconfig." + BuildGroup + ".groupname"];
                }
                return _buildGroupName;
            }
        }


        protected EaconfigBuildTask(Project project, string groupname, string buildModule)
            : base(project)
        {
            GroupName = groupname;
            BuildModule = buildModule;

            if (String.IsNullOrEmpty(GroupName))
            {
                Error.Throw(Project, Location, Name, "'GroupName' parameter is empty.");
            }
            if (String.IsNullOrEmpty(BuildModule))
            {
                Error.Throw(Project, Location, Name, "'BuildModule' parameter is empty.");
            }
        }

        protected EaconfigBuildTask()
            : base()
        {
        }

        protected string PropGroupName(string name)
        {
            if (name.StartsWith("."))
            {
                return GroupName + name;
            }
            return GroupName + "." + name;
        }

        protected string PropBuildGroup(string name)
        {
            if (name.StartsWith("."))
            {
                return BuildGroupName + name;
            }
            return BuildGroupName + "." + name;
        }


        private string _buildGroup;

        private string _buildGroupName;

        private string _groupName;

        private string _buildModule;

    }
}
