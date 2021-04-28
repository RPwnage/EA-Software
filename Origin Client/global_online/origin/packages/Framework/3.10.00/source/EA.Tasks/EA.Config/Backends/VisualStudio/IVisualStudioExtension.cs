using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Backends.VisualStudio;
using EA.Eaconfig.Backends;

namespace EA.Eaconfig.Core
{


    public abstract class IMCPPVisualStudioExtension : IVisualStudioExtensionBase
    {
        #region Interface

        public virtual void WriteExtensionItems(IXmlWriter writer)
        {
        }

        public virtual void WriteExtensionToolProperties(IXmlWriter writer)
        {
        }

        public virtual void SetVCPPDirectories(VCPPDirectories directories)
        {
        }

        public virtual void ProjectGlobals(IDictionary<string, string> projectGlobals)
        {
        }

        public virtual void UserData(IDictionary<string, string> userData)
        {
        }

        public virtual void ProjectConfiguration(IDictionary<string, string> ConfigurationElements)
        {
        }

        public virtual void WriteMsBuildOverrides(IXmlWriter writer)
        {
        }

        #endregion
    }

    public abstract class IDotNetVisualStudioExtension : IVisualStudioExtensionBase
    {
        #region Interface

        public virtual void WriteExtensionItems(IXmlWriter writer)
        {
        }

        #endregion
    }



    public abstract class IVisualStudioExtensionBase : Task
    {
        public ProcessableModule Module
        {
            get { return _module; }
            set { _module = value; }
        } private ProcessableModule _module;

        public IProjectGenerator Generator
        {
            get { return _generator; }
            set { _generator = value; }
        } private IProjectGenerator _generator;

        private new void Execute()
        {
        }
        protected override void ExecuteTask()
        {
        }

        protected string PropGroupName(string name)
        {
            if (name.StartsWith("."))
            {
                return Module.GroupName + name;
            }
            return Module.GroupName + "." + name;
        }

        protected string PropGroupValue(string name, string defVal = "")
        {
            string val = null;
            if (!String.IsNullOrEmpty(name))
            {
                val = Project.Properties[PropGroupName(name)];
            }
            return val??defVal;
        }

        protected string PropGroupBoolValue(string name, string defVal = "")
        {
            string val = String.Empty;
            if (!String.IsNullOrEmpty(name))
            {
                var tval = Project.Properties[PropGroupName(name)].TrimWhiteSpace();
                if (tval.Equals("on", StringComparison.OrdinalIgnoreCase) || tval.Equals("true", StringComparison.OrdinalIgnoreCase) || tval.Equals("yes", StringComparison.OrdinalIgnoreCase))
                {
                    val = "true";
                }
                else if (tval.Equals("off", StringComparison.OrdinalIgnoreCase) || tval.Equals("false", StringComparison.OrdinalIgnoreCase) || tval.Equals("no", StringComparison.OrdinalIgnoreCase))
                {
                    val = "false";
                }
            }
            return val;
        }


        protected FileSet PropGroupFileSet(string name)
        {

            if (String.IsNullOrEmpty(name))
            {
                return null;
            }
            return Project.NamedFileSets[PropGroupName(name)];
        }

        public interface IProjectGenerator
        {
            ModuleGenerator Interface {get;}

            string GetVSProjConfigurationName(Configuration configuration);

            string GetConfigCondition(Configuration config);

        }

        public class MCPPProjectGenerator : IProjectGenerator
        {
            private VSCppMsbuildProject _generator;

            internal MCPPProjectGenerator(VSCppMsbuildProject project)
            {
                _generator = project;
            }

            public ModuleGenerator Interface
            {
                get
                {
                    return _generator;
                }
            }

            public string GetVSProjConfigurationName(Configuration configuration)
            {
                return _generator.GetVSProjConfigurationName(configuration);
            }

            public string GetConfigCondition(Configuration config)
            {
                return  _generator.GetConfigCondition(config);
            }

        }

        public class DotNetProjectGenerator : IProjectGenerator
        {
            private VSDotNetProject _generator;

            internal DotNetProjectGenerator(VSDotNetProject project)
            {
                _generator = project;
            }

            public ModuleGenerator Interface
            {
                get
                {
                    return _generator;
                }
            }

            public string GetVSProjConfigurationName(Configuration configuration)
            {
                return _generator.GetVSProjConfigurationName(configuration);
            }

            public string GetConfigCondition(Configuration config)
            {
                return  _generator.GetConfigCondition(config);
            }

        }


    }

}
