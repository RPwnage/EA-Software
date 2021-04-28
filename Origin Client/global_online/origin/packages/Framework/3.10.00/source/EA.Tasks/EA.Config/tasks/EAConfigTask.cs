using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

// Base class with utility methods for eaconfig tasks.
namespace EA.Eaconfig
{
    [TaskName("EAConfigTask")]
    public abstract class EAConfigTask : Task
    {
        protected EAConfigTask(Project project)
            : base()
        {
            Project = project;
            InternalInit();
        }

        protected EAConfigTask()
            : base()
        {
        }


        protected override void InitializeTask(System.Xml.XmlNode taskNode)
        {
            base.InitializeTask(taskNode);
            InternalInit();
        }

        protected virtual void InternalInit()
        {
            Config = Properties["config"];
            ConfigSystem = Properties["config-system"];
            ConfigPlatform = Properties["config-platform"];
            ConfigCompiler = Properties["config-compiler"];
            ExternalBuild = PropertyUtil.GetBooleanPropertyOrDefault(Project, "eaconfig.externalbuild", false);
        }

        protected OptionSet CreateBuildOptionSetFrom(string buildSetName, string optionSetName, string fromOptionSet)
        {
            OptionSet optionset = new OptionSet();
            optionset.FromOptionSetName = fromOptionSet;
            optionset.Project = Project;
            optionset.Append(OptionSetUtil.GetOptionSet(Project, fromOptionSet));
            optionset.Options["buildset.name"] = buildSetName;
            if (Project.NamedOptionSets.Contains(optionSetName))
            {
                Project.NamedOptionSets[optionSetName].Append(optionset);
            }
            else
            {
                Project.NamedOptionSets[optionSetName] = optionset;
            }
            return optionset;
        }

        protected bool PropertyExists(string name)
        {
            return Project.Properties.Contains(name);
        }

        protected string GetRealBuildType(string buildType)
        {
            if (!String.IsNullOrEmpty(buildType))
            {
                if (buildType.Equals("Library", StringComparison.Ordinal))
                {
                    buildType = "StdLibrary";
                }
                else if (buildType.Equals("Program", StringComparison.Ordinal))
                {
                    buildType = "StdProgram";
                }
            }
            return buildType;
        }

        protected string PackageName
        {
            get
            {
                if(_package_name == null)
                {
                    _package_name = Properties["package.name"];
                }
                return _package_name;
            }
        } private string _package_name = null;

        protected string PackageVersion
        {
            get
            {
                if (_package_version == null)
                {
                    _package_version = Properties["package.version"];
                }
                return _package_version;
            }
        } private string _package_version = null;

        protected string Config;
        protected string ConfigSystem;
        protected string ConfigPlatform;
        protected string ConfigCompiler;
        protected bool ExternalBuild;
    }
}
