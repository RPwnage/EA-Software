using System;
using System.Reflection;
using System.IO;

namespace EA.PackageSystem.PackageManager {

    public class ResourceLoader {
        public static readonly ResourceLoader Instance = new ResourceLoader();

        private ResourceLoader() {
        }

        public string LoadTextFile(string resourceName) {
            try {
                using (StreamReader reader = new StreamReader(OpenStream(resourceName))) {
                    return reader.ReadToEnd();
                }
            } catch {
                return String.Format("Could not load resource '{0}'.", resourceName);
            }
        }

        public Stream OpenStream(string resourceName) {
            resourceName = "EA.PackageSystem.PackageManager.Resources." + resourceName;
            return Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName);
        }
    }
}
