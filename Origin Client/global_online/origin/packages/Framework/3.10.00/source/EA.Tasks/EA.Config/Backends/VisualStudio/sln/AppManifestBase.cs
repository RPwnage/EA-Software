using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NAnt.Core.Util;
using System.IO;
using EA.Eaconfig.Modules;
using NAnt.Core;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
    abstract class AppManifestBase
    {
        public PathString ManifestFilePath { get; protected set; }
        private static object sLock = new object();
        private Log Log;

        public IEnumerable<PathString> ImageFiles
        {
            get { return _imageFiles; }
        }

        protected AppManifestBase(Log log, IModule module, string userOptionsetName, string defaultOptionsetName)
        {
            Log = log;
            Module = module;
            Project = module.Package.Project;
            UserOptionSetName = userOptionsetName;
            DefaultOptionSetName = defaultOptionsetName;
            _imageFiles = new List<PathString>();
        }

        protected OptionSet CreateManifestOptions(PathString appManifestFolder, string appName)
        {
            var manifestOptions = Project.NamedOptionSets[DefaultOptionSetName]??new OptionSet();

            if (manifestOptions.Options.Count == 0)
            {
                Log.Warning.WriteLine("Did not find named optionset {0}", DefaultOptionSetName);
            }

            var useroptions = Project.NamedOptionSets[UserOptionSetName];
            if(useroptions != null)
            {
                manifestOptions.Append(useroptions);
            }

            return manifestOptions;
        }

        protected abstract PathString CreateImages(OptionSet manifestOptions, PathString imageFolder);
        protected abstract string ExecuteTemplate(string vsProjectFileNameWithoutExtension, string appExecutableName, OptionSet options, string imageRelPath);

        protected void WriteAppXManifestFile(PathString appManifestFolder, string appManifestFile, string vsProjectFileNameWithoutExtension, string appExecutableName, OptionSet options, string imageRelPath)
        {
            ManifestFilePath = PathString.MakeCombinedAndNormalized(appManifestFolder.Path, appManifestFile);

            Directory.CreateDirectory(appManifestFolder.Path);

            using (var writer = new StreamWriter(ManifestFilePath.Path))
            {
                writer.Write(ExecuteTemplate(vsProjectFileNameWithoutExtension, appExecutableName, options, imageRelPath));
            }
        }

        protected void DoGenerate(PathString appManifestFolder, string vsProjectFileNameWithoutExtension, string appName, PathString vsProjectDir)
        {
            var manifestOptions = CreateManifestOptions(appManifestFolder, appName);
            var imagefolder = CreateImages(manifestOptions, vsProjectDir);
            var appExecutableName = AppExecutableName(Module);
            WriteAppXManifestFile(appManifestFolder, manifestOptions.Options["manifest.filename"], vsProjectFileNameWithoutExtension, appExecutableName, manifestOptions, PathUtil.RelativePath(imagefolder.Path, vsProjectDir.Path));
        }

        protected PathString CopyOrCreateImage(string src, PathString destfolder, int width, int height)
        {
            var destpng = Path.IsPathRooted(src) ? 
                PathString.MakeCombinedAndNormalized(destfolder.Path, Path.GetFileName(src)) :
                PathString.MakeCombinedAndNormalized(destfolder.Path, src);

            if (!Directory.Exists(Path.GetDirectoryName(destpng.Path)))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(destpng.Path));
            }

            if (Path.IsPathRooted(src))
            {
                if (destpng == PathString.MakeNormalized(src))
                {
                    Log.Error.WriteLine("CopyOrCreateImage() - src path equals dst path: {0}", destpng.Path);
                }
                else
                {
                    // if we have a rooted path, then copy that file to the dest folder
                    if (File.Exists(destpng.Path))
                    {
                        File.SetAttributes(destpng.Path, FileAttributes.Normal);
                    }
                    File.Copy(src, destpng.Path, true);
                }
            }
            else
            {
                lock (sLock)
                {
                    System.Drawing.Bitmap logo = new System.Drawing.Bitmap(width, height);
                    logo.Save(destpng.Path, System.Drawing.Imaging.ImageFormat.Png);
                }
            }
            return destpng;
        }

        protected virtual string AppExecutableName(IModule imodule)
        {
            Module module = imodule as Module;

            string appExecutableName = String.Empty;

            if (module != null && module.Tools != null)
            {
                Linker link = module.Tools.SingleOrDefault(t => t.ToolName == "link") as Linker;

                if (link != null)
                {
                    appExecutableName = link.OutputName + link.OutputExtension;
                }
            }

            return appExecutableName;
        }

        protected readonly IModule Module;
        protected readonly Project Project;
        protected readonly string UserOptionSetName;
        protected readonly string DefaultOptionSetName;
        
        protected ICollection<PathString> _imageFiles;
    }
}
