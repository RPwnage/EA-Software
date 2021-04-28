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
using NAnt.Core.Writers;
using NAnt.Core.Events;
using Model = EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class AppXManifest : AppManifestBase
    {
        internal static AppXManifest Generate(Log log, Module_Native module, PathString vsProjectDir, string vsProjectName, string vsProjectFileNameWithoutExtension)
        {
            if (module != null)
            {
                var appManifestFolder = module.IntermediateDir;
                var appName = module.OutputName;

                var manifest = new AppXManifest(log, module);
                manifest.DoGenerate(appManifestFolder, vsProjectFileNameWithoutExtension, appName, vsProjectDir);
                return manifest;
            }

            return null;
        }

        private AppXManifest(Log log, Module_Native module)
            : base(log, module, module.GroupName + ".appxmanifestoptions", "config-options-appxmanifestoptions")
        {
        }

        protected override PathString CreateImages(OptionSet manifestOptions, PathString appManifestFolder)
        {
            _imageFiles.Add(CopyOrCreateImage(manifestOptions.Options["properties.logo"], appManifestFolder, 50, 50));
            _imageFiles.Add(CopyOrCreateImage(manifestOptions.Options["visualelements.logo"], appManifestFolder, 150, 150));
            _imageFiles.Add(CopyOrCreateImage(manifestOptions.Options["visualelements.smalllogo"], appManifestFolder, 30, 30));
            _imageFiles.Add(CopyOrCreateImage(manifestOptions.Options["visualelements.splashscreen"], appManifestFolder, 620, 300));

            if (manifestOptions.Options.Contains("visualelements.widelogo"))
            {
                _imageFiles.Add(CopyOrCreateImage(manifestOptions.Options["visualelements.widelogo"], appManifestFolder, 310, 150));
            }

            return appManifestFolder;
        }

        protected override string ExecuteTemplate(string vsProjectFileNameWithoutExtension, string appExecutableName, OptionSet options, string imageRelPath)
        {
            var extensionOptions = options.Options["extension_options"].ToArray();
            
            var template = new AppXManifestTemplate()
            {
                Name = options.Options["identity.name"],
                Publisher = options.Options["identity.publisher"],
                Version = options.Options["identity.version"],
                PropertiesDisplayName = options.Options["properties.displayname"],
                PublisherDisplayName = options.Options["properties.publisherdisplayname"],
                PropertiesLogo = Path.Combine(imageRelPath, Path.GetFileName(options.Options["properties.logo"])),
                PropertiesDescription = options.Options["properties.description"],
                ApplicationId = options.Options.ContainsKey("application.id") ? options.Options["application.id"] : vsProjectFileNameWithoutExtension + ".App",
                Executable = options.Options.ContainsKey("application.executable") ? options.Options["application.executable"] : appExecutableName,
                EntryPoint = appExecutableName,
                VisualElementsDisplayName = options.Options["visualelements.displayname"],
                VisualElementsLogo = Path.Combine(imageRelPath, Path.GetFileName(options.Options["visualelements.logo"])),
                VisualElementsSmallLogo = Path.Combine(imageRelPath, Path.GetFileName(options.Options["visualelements.smalllogo"])),
                VisualElementsDescription = options.Options["visualelements.description"],
                SplashScreen = Path.Combine(imageRelPath, Path.GetFileName(options.Options["visualelements.splashscreen"])),
                Capabilities = new List<string>(options.Options["capabilities"].ToArray()),
                DeviceCapabilities = new List<string>(options.Options["devicecapabilities"].ToArray()),
                Extensions = new List<string>(options.Options.Where(o=> extensionOptions.Contains(o.Key)).Select(o=>o.Value)),
            };
            if (options.Options.Contains("visualelements.widelogo"))
            {
                template.WideLogo = Path.Combine(imageRelPath, Path.GetFileName(options.Options["visualelements.widelogo"]));
            }
            return template.TransformText();
        }
    }
}
