using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;

using System;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;


namespace EA.Eaconfig.Structured
{
   [NAnt.Core.Attributes.XmlSchema]
   [NAnt.Core.Attributes.Description("Add Description")]
   [TaskName("BuildType")]
   public class BuildTypeTask : Task
    {
       protected BuildTypeElement _buildTypeElement;

        /// <summary>The name of an option set to use as a base buildset.</summary>
       [NAnt.Core.Attributes.Description("The name of an option set to use as a base buildset.")]
        [TaskAttribute("from", Required = false)]
        public string FromBuildType
        {
            get { return _fromBuildType; }
            set { _fromBuildType = value; }
        } private string _fromBuildType;

        [NAnt.Core.Attributes.Description("Sets the name for the base buildset")]
        [TaskAttribute("name", Required = false)]
        public string BuildTypeName
        {
            get { return _buildTypeName; }
            set { _buildTypeName = value; }
        } private string _buildTypeName;

        protected override void InitializeTask(XmlNode elementNode)
        {
            if (OptionSetUtil.OptionSetExists(Project, BuildTypeName))
            {
                string msg = Error.Format(Project, Name, "WARNING", "BuilOptionSet '{0}' exists and will be overwritten.", BuildTypeName);
                Log.WriteLine(msg);
            }

            _buildTypeElement = new BuildTypeElement(FromBuildType);
            _buildTypeElement.Project = Project;
            _buildTypeElement.LogPrefix = LogPrefix;
            _buildTypeElement.FromOptionSetName = _fromBuildType;

            _buildTypeElement.Initialize(elementNode);
        }

        protected override void ExecuteTask()
        {
            OptionSet os = _buildTypeElement as OptionSet;
            os.Options["buildset.name"] = BuildTypeName;
            Project.NamedOptionSets[BuildTypeName + "-temp"] = _buildTypeElement as OptionSet;
            GenerateBuildOptionset.Execute(Project, os, BuildTypeName+"-temp");
        }
    }

    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [ElementName("buildtype", StrictAttributeCheck = true)]
    public class BuildTypeElement : OptionSet
    {
        bool _ifDefined = true;
        bool _unlessDefined = false;
        OptionSet BuildOptionSet;
        ModuleBaseTask _module;
        private XmlNode _elementNode;

        public string LogPrefix = "[buildoptions] ";

        public BuildTypeElement(ModuleBaseTask module, OptionSet buildOptionSet)
            : base() 
        {
            _module = module;
            BuildOptionSet = buildOptionSet;
        }

        public BuildTypeElement(string fromBuildType)
            : base()
        {
            FromOptionSetName = fromBuildType;
            BuildOptionSet = null;
        }


        /// <summary>If true then the option will be included; otherwise skipped. Default is "true".</summary>
        [NAnt.Core.Attributes.Description("If true then the option will be included; otherwise skipped. Default is \"true\".")]
        [TaskAttribute("if")]
        public bool IfDefined
        {
            get { return _ifDefined; }
            set { _ifDefined = value; }
        }

        /// <summary>Opposite of if.  If false then the option will be included; otherwise skipped. Default is "false".</summary>
        [NAnt.Core.Attributes.Description("Opposite of if.  If false then the option will be included; otherwise skipped. Default is \"false\".")]
        [TaskAttribute("unless")]
        public bool UnlessDefined
        {
            get { return _unlessDefined; }
            set { _unlessDefined = value; }
        }

        /// <summary>Add all the child option elements.</summary>
        /// <param name="elementNode">Xml node to initialize from.</param>
        protected override void InitializeElement(XmlNode elementNode)
        {
            if (IfDefined && !UnlessDefined)
            {
                _elementNode = elementNode;

                if (_module == null)
                {
                    InternalInitializeElement(String.Empty);
                }
            }
        }

        internal void InternalInitializeElement(string basebuildtype)
        {
            if (_elementNode != null)
            {
                if (_module != null)
                {
                    FromOptionSetName = basebuildtype!= null? basebuildtype : _module.BuildType;
                }

                FromOptionSetName = Project.ExpandProperties(FromOptionSetName);

                if (FromOptionSetName == "Library")
                {
                    FromOptionSetName = "StdLibrary";
                }
                else if (FromOptionSetName == "Program")
                {
                    FromOptionSetName = "StdProgram";
                }

                OptionSet optionSet = OptionSetUtil.GetOptionSet(Project, FromOptionSetName);

                if (optionSet == null)
                {
                    Error.Throw(Project, Location, Name, "OptionSet '{0}' does not exist.", FromOptionSetName);
                }

                if (OptionSetUtil.GetBooleanOption(optionSet, "buildset.finalbuildtype"))
                {
                    string protoSetName = OptionSetUtil.GetOptionOrDefault(optionSet, "buildset.protobuildtype.name", String.Empty);

                    if (String.IsNullOrEmpty(protoSetName))
                    {
                        Error.Throw(Project, Location, Name, "Build OptionSet '{0}' does not specify proto-OptionSet name.", FromOptionSetName);
                    }
                    optionSet = OptionSetUtil.GetOptionSet(Project, protoSetName);

                    if (optionSet == null)
                    {
                        Error.Throw(Project, Location, Name, "Proto-OptionSet '{0}' specified by Build OptionSet '{1}' does not exist.", protoSetName, FromOptionSetName);
                    }
                    else if (OptionSetUtil.GetBooleanOption(optionSet, "buildset.finalbuildtype"))
                    {
                        String format =
                         "Proto OptionSet '{0}' specified in Build OptionSet '{1} is not a proto-buildtype optionset, and can not be used to generate build types.\n" +
                         "\n" +
                         "Possible reasons: '{0}' might be derived from a non-proto-buildtype optionset.\n" +
                         "\n" +
                         "Examples of proto-buildtypes are 'config-options-program', 'config-options-library' etc.\n" +
                         "and any other optionsets that are derived from proto-buildtypes.\n";

                        Error.Throw(Project, Location, Name, format, protoSetName, FromOptionSetName);
                    }

                    FromOptionSetName = protoSetName;

                }
                base.InitializeElement(_elementNode);
                if (BuildOptionSet != null)
                {
                    BuildOptionSet.Append(this);
                }
            }
        }

    }

    

}
