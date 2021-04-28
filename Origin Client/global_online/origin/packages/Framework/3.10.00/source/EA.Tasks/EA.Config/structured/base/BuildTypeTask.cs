using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using NAnt.Core.Threading;

using System;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;


namespace EA.Eaconfig.Structured
{
    /// <summary>Add Description</summary>
    [TaskName("BuildType")]
    public class BuildTypeTask : Task
    {
       protected BuildTypeElement _buildTypeElement;

        /// <summary>The name of a buildtype ('Library', 'Program', etc.) to derive new build type from.</summary>
        [TaskAttribute("from", Required = false)]
        public string FromBuildType
        {
            get { return _fromBuildType; }
            set { _fromBuildType = value; }
        } private string _fromBuildType;

        /// <summary>Sets the name for the new buildtype</summary>
        [TaskAttribute("name", Required = false)]
        public string BuildTypeName
        {
            get { return _buildTypeName; }
            set { _buildTypeName = value; }
        } private string _buildTypeName;

        [XmlElement("option", "NAnt.Core.OptionSet+Option", AllowMultiple = true)]
        protected override void InitializeTask(XmlNode elementNode)
        {
            if (OptionSetUtil.OptionSetExists(Project, BuildTypeName))
            {
                string msg = Error.Format(Project, Name, "WARNING", "BuilOptionSet '{0}' exists and will be overwritten.", BuildTypeName);
                Log.Warning.WriteLine(msg);
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

            if (os.Options.Contains("ps3-spu"))
            {
                GenerateBuildOptionsetSPU.Execute(Project, os, BuildTypeName + "-temp");
            }
            else
            {
                GenerateBuildOptionset.Execute(Project, os, BuildTypeName + "-temp");
            }
        }

        public static OptionSet ExecuteTask(Project project, string buildTypeName, string fromBuildType, OptionSet flags, bool saveFinalToproject = true, Location location = null, string name = null)
        {
            string finalFromName;
            var fromoptionset = BuildTypeElement.GetFromOptionSetName(project, fromBuildType, out finalFromName, location, name);

            OptionSet os = new OptionSet(fromoptionset);
            if (flags != null)
            {
                os.Append(flags);
            }
            os.Options["buildset.name"] = buildTypeName;
            project.NamedOptionSets[buildTypeName + "-temp"] = os;

            if (os.Options.Contains("ps3-spu"))
            {
                GenerateBuildOptionsetSPU.Execute(project, os, buildTypeName + "-temp");
                return project.NamedOptionSets[buildTypeName];
            }

            return GenerateBuildOptionset.Execute(project, os, buildTypeName + "-temp", saveFinalToproject: false);
        }


       
    }

    /// <summary></summary>
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
        [TaskAttribute("if")]
        public bool IfDefined
        {
            get { return _ifDefined; }
            set { _ifDefined = value; }
        }

        /// <summary>Opposite of if.  If false then the option will be included; otherwise skipped. Default is "false".</summary>
        [TaskAttribute("unless")]
        public bool UnlessDefined
        {
            get { return _unlessDefined; }
            set { _unlessDefined = value; }
        }

        public override void Initialize(XmlNode elementNode)
        {
            if (Project == null)
            {
                throw new InvalidOperationException("Element has invalid (null) Project property.");
            }

            // Save position in buildfile for reporting useful error messages.
            if (!String.IsNullOrEmpty(elementNode.BaseURI))
            {
                try
                {
                    Location = Location.GetLocationFromNode(elementNode);
                }
                catch (ArgumentException ae)
                {
                    Log.Warning.WriteLine("Can't find node '{0}' location, file: '{1}'{2}{3}", elementNode.Name, elementNode.BaseURI, Environment.NewLine, ae.ToString());
                }

            }

            try
            {
                _ifDefined = true;
                _unlessDefined = false;

                foreach (XmlAttribute attr in elementNode.Attributes)
                {
                    switch (attr.Name)
                    {
                        case "if":
                            _ifDefined = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
                            break;
                        case "unless":
                            _unlessDefined = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
                            break;
                        default:
                            break;
                    }
                }

                InitializeElement(elementNode);
            }
            catch (BuildException e)
            {
                if (e.Location == Location.UnknownLocation)
                {
                    e = new BuildException(e.Message, this.Location, e.InnerException);
                }
                throw new ContextCarryingException(e, Name);
            }
            catch (System.Exception e)
            {
                Exception ex = ThreadUtil.ProcessAggregateException(e);

                if (ex is BuildException)
                {
                    BuildException be = ex as BuildException;
                    if (be.Location == Location.UnknownLocation)
                    {
                        be = new BuildException(be.Message, Location, be.InnerException, be.StackTraceFlags);
                    }
                    throw new ContextCarryingException(be, Name);
                }

                throw new ContextCarryingException(ex, Name, Location);
            }
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

        internal static OptionSet GetFromOptionSetName(Project project, string fromOptionSetName, out string finalFromName, Location location = null, string name = null)
        {

            OptionSet optionSet = null;

            if (fromOptionSetName == "Library")
            {
                fromOptionSetName = "StdLibrary";
            }
            else if (fromOptionSetName == "Program")
            {
                fromOptionSetName = "StdProgram";
            }

            optionSet = OptionSetUtil.GetOptionSet(project, fromOptionSetName);

            if (optionSet == null)
            {
                Error.Throw(project, location, name, "OptionSet '{0}' does not exist.", fromOptionSetName);
            }

            if (OptionSetUtil.GetBooleanOption(optionSet, "buildset.finalbuildtype"))
            {
                string protoSetName = OptionSetUtil.GetOptionOrDefault(optionSet, "buildset.protobuildtype.name", String.Empty);

                if (String.IsNullOrEmpty(protoSetName))
                {
                    Error.Throw(project, location, name, "Build OptionSet '{0}' does not specify proto-OptionSet name.", fromOptionSetName);
                }
                optionSet = OptionSetUtil.GetOptionSet(project, protoSetName);

                if (optionSet == null)
                {
                    Error.Throw(project, location, name, "Proto-OptionSet '{0}' specified by Build OptionSet '{1}' does not exist.", protoSetName, fromOptionSetName);
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

                    Error.Throw(project, location, name, format, protoSetName, fromOptionSetName);
                }

                fromOptionSetName = protoSetName;
            }
            finalFromName = fromOptionSetName;

            return optionSet;

        }

        internal void InternalInitializeElement(string basebuildtype)
        {
            if (_elementNode != null || Options.Count > 0)
            {
                if (_module != null)
                {
                    FromOptionSetName = basebuildtype != null ? basebuildtype : _module.BuildType;

                    Project = Project ?? _module.Project;
                }

                FromOptionSetName = Project.ExpandProperties(FromOptionSetName);

                string finalFromOptionSetName;
                var fromOptionSet = GetFromOptionSetName(Project, FromOptionSetName, out finalFromOptionSetName, Location, Name);

                FromOptionSetName = null;

                OptionSet inputOptions = null; // These options can be set by derived Module task
                if (Options.Count > 0)
                {
                    inputOptions = new OptionSet(this);
                }


                Append(fromOptionSet);

                if (inputOptions != null)
                {
                    Append(inputOptions);
                }

                if (_elementNode != null)
                {
                    base.InitializeElement(_elementNode);
                }

                FromOptionSetName = finalFromOptionSetName;

                if (BuildOptionSet != null)
                {
                    BuildOptionSet.Append(this);
                }
            }
        }
    }
}
