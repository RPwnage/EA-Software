using System;
using System.Linq;
using System.Text;
using System.IO;

using EA.FrameworkTasks.Model;

using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using System.Collections.Generic;
using EA.Eaconfig.Modules;
using NAnt.Core.Logging;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
    // The 'WMAppManifest.xml' file is required by the Windows Phone MSBuild assemblies.
    // The content of the file is defined here:
    // 
    // Application Manifest File for Windows Phone
    // http://msdn.microsoft.com/en-us/library/ff769509(v=vs.92).aspx
        
    internal class WMAppManifest : AppManifestBase
    {
        private static object _sync = new Object();

        internal static WMAppManifest Generate(Log log, IModule module, PathString manifestFolder, PathString vcProjectDir, string projectName)
        {
            lock(_sync)
            {
                var manifest = new WMAppManifest(log, module, vcProjectDir);
                manifest.DoGenerate(manifestFolder, projectName, projectName, vcProjectDir);
                return manifest;
            }
        }

        private WMAppManifest(Log log, IModule module, PathString vcProjectDir)
            : base(log, module, module.GroupName + ".wmappmanifestoptions", "config-options-wmappmanifestoptions")
        {
        }

        protected override PathString CreateImages(OptionSet manifestOptions, PathString projectFolder)
        {
            _imageFiles.Add(CopyOrCreateImage(manifestOptions.Options["app.iconpath"], projectFolder, 100, 100));
            _imageFiles.Add(CopyOrCreateImage(manifestOptions.Options["app.templateflip.smallimageuri"], projectFolder, 159, 159));
            _imageFiles.Add(CopyOrCreateImage(manifestOptions.Options["app.templateflip.backgroundimageuri"], projectFolder, 336, 336));

            return projectFolder;
        }

        protected override string ExecuteTemplate(string vsProjectFileNameWithoutExtension, string appExecutableName, OptionSet options, string imageRelPath)
        {
            var imagePath = vsProjectFileNameWithoutExtension;
            var runtimeType = "Modern Native";

            if (Module != null && Module.IsKindOf(EA.FrameworkTasks.Model.Module.DotNet))
            {
                imagePath = Module.Package.Project.GetPropertyValue(Module.GroupName + ".navigatorpage");

                if(String.IsNullOrEmpty(imagePath))
                {
                    throw new BuildException(String.Format("ERROR GENERATING MANIFEST: NavigatorPage (xaml file name) must be defined in property {0}", Module.GroupName + ".navigatorpage"));
                }

                runtimeType = "Silverlight";
            }

            Guid productID;
            Guid publisherID;

            if (!Guid.TryParse(options.Options["app.product.id"]??"", out productID))
            {
                productID = Guid.NewGuid();
            }
            if (!Guid.TryParse(options.Options["app.publisher.id"]??"", out publisherID))
            {
                publisherID = Guid.NewGuid();
            }

            var template = new WMAppManifestTemplate() {
                RuntimeType = runtimeType,
                ProductId = productID,
                PublisherId = publisherID,
                Title = options.Options["app.title"]??vsProjectFileNameWithoutExtension,
                AppDescription = options.Options["app.description"]??"Simple Direct3D application",
                AppGenre = options.Options["app.genre"]??"apps.normal",
                AppVersion = options.Options["app.version"]??"1.0.0.0",
                ImagePath = imagePath,
                TokenId = vsProjectFileNameWithoutExtension + "Token",
                IconPath = options.Options["app.iconpath"],
                SmallImageUri = options.Options["app.templateflip.smallimageuri"],
                ActivationPolicy = options.Options["app.activationpolicy"],
                BackgroundImageUri = options.Options["app.templateflip.backgroundimageuri"],
                Capabilities = new List<string>(options.Options["capabilities"].ToArray()),
                FunctionalCapabilities = new List<string>(options.Options["functionalcapabilities"].ToArray()),
                ActivatableClasses = options.Options["activatable.classes"].TrimWhiteSpace(),
                IsDotNetProject = Module.IsKindOf(EA.FrameworkTasks.Model.Module.DotNet)
            };
            return template.TransformText();
        }
    }
}
