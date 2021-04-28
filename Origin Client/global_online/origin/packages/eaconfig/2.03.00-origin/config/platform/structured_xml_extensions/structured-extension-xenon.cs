using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.Eaconfig.Structured;

namespace EA.Eaconfig.Structured
{
    [TaskName("structured-extension-xenon")]
    class StructuredExtensionXenon : PlatformExtensionBase
    {
        /// <summary>
        /// image builder options.
        /// </summary>
        [Property("imagebuilder")]
        public ImageBuilder ImageBuilder
        {
            get 
            { 
                return _imageBuilder; 
            }
        }
        private ImageBuilder _imageBuilder = new ImageBuilder();

        /// <summary>
        /// Xenon deployment options.
        /// </summary>
        [Property("deployment")]
        public Deployment Deploy
        {
            get 
            { 
                return _deploy; 
            }
        }
        private Deployment _deploy = new Deployment();



        protected override void ExecuteTask()
        {
            SetImageBuilder();

            SetDeployment();
        }

        private void SetImageBuilder()
        {
            if (!String.IsNullOrEmpty(ImageBuilder.ConfigFile))
            {
                Module.SetModuleProperty("imgbld.projectdefaults", ImageBuilder.ConfigFile.TrimWhiteSpace());
            }

            var options = new StringBuilder();
            if (!String.IsNullOrEmpty(ImageBuilder.TitleId))
            {
                Module.SetModuleProperty("imgbld.titleid", ImageBuilder.TitleId.TrimWhiteSpace());
                options.AppendLine("-titleid:" + ImageBuilder.TitleId.TrimWhiteSpace());
            }

            if (!String.IsNullOrEmpty(ImageBuilder.LanKey))
            {
                Module.SetModuleProperty("imgbld.lankey", ImageBuilder.LanKey.TrimWhiteSpace());
                options.AppendLine("-lankey:" + ImageBuilder.LanKey.TrimWhiteSpace());
            }

            options.Append(ImageBuilder.Options.Value ?? String.Empty);
            if (ImageBuilder.Options.Append)
            {
                options.Append(Module.GetModuleProperty("imgbld.options"));
            }

            var finaloptions = options.ToString().LinesToArray().OrderedDistinct();

            if (finaloptions.Count() > 0)
            {
                Module.SetModuleProperty("imgbld.options", finaloptions.ToString(Environment.NewLine), append: false);
            }
        }

        private void SetDeployment()
        {
            if (Deploy._enable != null)
            {
                Properties["package.consoledeployment"] = Deploy.Enable.ToString();
            }

            if (Deploy.DeploymentType == Deployment.DeploymentTypes.DVD)
            {
                if (!String.IsNullOrEmpty(Deploy.LayoutFile.Value))
                {
                    Module.SetModuleProperty("dvdemulationroot", Project.ExpandProperties(Deploy.LayoutFile.Value).TrimWhiteSpace());
                }
                else if (!String.IsNullOrEmpty(Deploy.RemoteRoot.Value))
                {
                    Module.SetModuleProperty("dvdemulationroot", Project.ExpandProperties(Deploy.RemoteRoot.Value).TrimWhiteSpace());
                }
            }
            else
            {
                if (!String.IsNullOrEmpty(Deploy.RemoteRoot.Value))
                {
                    Module.SetModuleProperty("customvcprojremoteroot", Project.ExpandProperties(Deploy.RemoteRoot.Value).TrimWhiteSpace());
                }

                if (!String.IsNullOrEmpty(Deploy.DeploymentFiles.Value))
                {
                    Module.SetModuleProperty("deploymentfiles", Project.ExpandProperties(Deploy.DeploymentFiles.Value).TrimWhiteSpace());
                }
            }
        }

    }

    public class ImageBuilder : Element
    {
        [Property("options")]
        public ConditionalPropertyElement Options
        {
            get
            {
                return _options;
            }
        }
        private ConditionalPropertyElement _options = new ConditionalPropertyElement();

        [TaskAttribute("configfile")]
        public String ConfigFile
        {
            get { return _configFile; }
            set { _configFile = value; }
        }
        private String _configFile;

        [TaskAttribute("titleid")]
        public String TitleId
        {
            get { return _titleid; }
            set { _titleid = value; }
        }
        private String _titleid;

        [TaskAttribute("lankey")]
        public String LanKey
        {
            get { return _lankey; }
            set { _lankey = value; }
        }
        private String _lankey;

    }

    public class Deployment : Element
    {
        public enum DeploymentTypes { HDD, DVD };

        [TaskAttribute("enable")]
        public bool Enable
        {
            get { return (bool)_enable; }
            set { _enable = value; }
        }
        internal bool? _enable = null;

        [TaskAttribute("deploymenttype")]
        public DeploymentTypes DeploymentType
        {
            get { return _deploymenttype; }
            set { _deploymenttype = value; }
        }
        private DeploymentTypes _deploymenttype = DeploymentTypes.HDD;

        [Property("remoteroot")]
        public ConditionalPropertyElement RemoteRoot
        {
            get
            {
                return _remoteroot;
            }
        }
        private ConditionalPropertyElement _remoteroot = new ConditionalPropertyElement();


        [Property("layoutfile")]
        public ConditionalPropertyElement LayoutFile
        {
            get
            {
                return _layoutfile;
            }
        }
        private ConditionalPropertyElement _layoutfile = new ConditionalPropertyElement();


        [Property("deploymentfiles")]
        public ConditionalPropertyElement DeploymentFiles
        {
            get { return _deploymentfiles; }
        }
        private ConditionalPropertyElement _deploymentfiles = new ConditionalPropertyElement();


    }


}
