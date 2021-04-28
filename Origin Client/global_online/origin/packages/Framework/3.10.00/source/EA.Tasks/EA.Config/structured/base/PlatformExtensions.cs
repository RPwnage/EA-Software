using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Reflection;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;

using EA.Eaconfig.Modules;
using EA.Eaconfig.Build;


namespace EA.Eaconfig.Structured
{
    public class PlatformExtensions : Task
    {
        protected XmlNode _xmlNode;

        protected override void InitializeTask(System.Xml.XmlNode taskNode)
        {
            base.InitializeTask(taskNode);
            _xmlNode = taskNode;
        }

        protected override void ExecuteTask()
        {
        }

        public void ExecutePlatformTasks(ModuleBaseTask module)
        {
            if (_xmlNode != null)
            {
                string platformNodeName = Properties.GetPropertyOrDefault("structured-xml.platform-extension.name", Project.Properties["config-system"]);

                foreach (XmlNode childNode in _xmlNode)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
                    {
                        if ((platformNodeName ?? childNode.Name).Equals(childNode.Name, StringComparison.OrdinalIgnoreCase))
                        {
                            var task = CreatePlatformExtensionTask(childNode);
                            task.Parent = this;
                            task.Project = Project;
                            task.Init(module);
                            task.Initialize(childNode);
                            task.Execute();
                        }
                    }
                }
            }
        }

        private PlatformExtensionBase CreatePlatformExtensionTask(XmlNode node)
        {

            string taskName = "structured-extension-" + node.Name.ToLowerInvariant();

            var task = Project.TaskFactory.CreateTask(taskName, Project);

            if (task == null)
            {
                throw new BuildException(String.Format("Can't find platform extension task: '{0}' reflected from XML node '{1}'", taskName, node.Name), Location.GetLocationFromNode(node));
            }

            var extensiontask = task as PlatformExtensionBase;
            if (extensiontask == null)
            {
                throw new BuildException(String.Format("Platform extension task: '{0}' reflected from XML node '{1}' is not derived from PlatformExtensionBase class", taskName, node.Name), Location.GetLocationFromNode(node));
            }

            return extensiontask;
        }

    }

    abstract public class PlatformExtensionBase : Task
    {
        public ModuleBaseTask Module
        {
            get { return _module; }

        }private ModuleBaseTask _module;


        internal void Init(ModuleBaseTask module)
        {
            _module = module;
        }
    }

}
