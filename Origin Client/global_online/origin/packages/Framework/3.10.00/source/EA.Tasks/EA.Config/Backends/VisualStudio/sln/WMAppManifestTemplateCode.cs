using System;
using System.Collections.Generic;

namespace EA.Eaconfig.Backends.VisualStudio
{
    public partial class WMAppManifestTemplate
    {
        public Guid ProductId { get; set; }
        public string RuntimeType { get; set; }
        public string AppVersion { get; set; }
        public string AppGenre { get; set; }
        public string AppDescription { get; set; }
        public string Title { get; set; }
        public string Author { get; set; }
        public string Publisher { get; set; }
        public Guid PublisherId { get; set; }
        public string ImagePath { get; set; }
        public string TokenId { get; set; }
        public string IconPath { get; set; }
        public string SmallImageUri { get; set; }
        public string ActivationPolicy { get; set; }
        public string BackgroundImageUri { get; set; }
        public List<string> Capabilities { get; set; }
        public List<string> FunctionalCapabilities { get; set; }
        public string ActivatableClasses { get; set; }
        public bool IsDotNetProject { get; set; }

        public WMAppManifestTemplate()
        {
            //ProductId = Guid.NewGuid();
            RuntimeType = "Modern Native";
            AppDescription = "App Description";
            AppGenre = "apps.normal";
            AppVersion = "1.0.0.0";
            Author = "EA";
            Publisher = "EA";
            //PublisherId = Guid.NewGuid();
            IsDotNetProject = false;
        }
    }
}
