using System.Collections.Generic;
namespace EA.Eaconfig.Backends.VisualStudio
{
    public partial class AppXManifestTemplate
    {
        public string Name { get; set; }
        public string Publisher { get; set; }
        public string Version { get; set; }
        public string PropertiesDisplayName { get; set; }
        public string PublisherDisplayName { get; set; }
        public string PropertiesLogo { get; set; }
        public string PropertiesDescription { get; set; }
        public string ApplicationId { get; set; }
        public string Executable { get; set; }
        public string EntryPoint { get; set; }
        public string VisualElementsDisplayName { get; set; }
        public string VisualElementsLogo { get; set; }
        public string VisualElementsSmallLogo { get; set; }
        public string VisualElementsDescription { get; set; }
        public string WideLogo { get; set; }
        public string SplashScreen { get; set; }
        public List<string> Capabilities { get; set; }
        public List<string> DeviceCapabilities { get; set; }
        public List<string> Extensions { get; set; }
    }
}
