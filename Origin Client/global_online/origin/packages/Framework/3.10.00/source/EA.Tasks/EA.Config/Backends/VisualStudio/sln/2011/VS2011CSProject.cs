using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using EA.Eaconfig.Build;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{

    internal class VS2011CSProject : VS2008CSProject
    {
        internal VS2011CSProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules)
        {
        }

        private bool IsWinPhone
        {
            get
            {
                if (_isWinPhone == null)
                {
                    _isWinPhone = false;
                    var winphone_modules = Modules.Count(m => m.Configuration.System == "winprt");
                    if (winphone_modules > 0)
                    {
                        _isWinPhone = true;
                        if (winphone_modules < Modules.Count())
                        {
                            Log.Warning.WriteLine(LogPrefix + "Project '{0}' contains mixture of Windows Phone configurations and PC configurations which is not allowed", Name);
                        }
                    }
                }
                return _isWinPhone.Value;
            }
        }

        private bool IsXboxLiveServices(IModule module)
        {
            if (module.Package.Project == null)
            {
                return false;
            }

            var val = 
                module.Package.Project.Properties[module.GroupName +"." + module.Configuration.System + ".xbox-live-services"] 
                ?? (module.Package.Project.Properties[module.GroupName + ".xbox-live-services"]
                ?? module.Package.Project.Properties[module.Configuration.System + ".xbox-live-services"]);

            return val.ToBooleanOrDefault(false);
        }

        private PathString XboxLiveServicesSpaFile(IModule module)
        {
            var fs = 
                module.Package.Project.NamedFileSets[module.GroupName + "." + module.Configuration.System + ".xbox-live-services.spa"]
                ?? module.Package.Project.NamedFileSets[module.GroupName + ".xbox-live-services.spa"];

            if (fs != null && fs.FileItems.FirstOrDefault() != null)
            {
                return fs.FileItems.FirstOrDefault().Path;
            }

            return null;
        }

        private string CopySpaFile(IModule module)
        {
            var srcSpaFile = XboxLiveServicesSpaFile(module);
            string dstSpaFile = null;

            if (srcSpaFile != null)
            {
                dstSpaFile = Path.Combine(this.OutputDir.Path, Path.GetFileName(srcSpaFile.Path));

                CopyTask task = new CopyTask();
                task.Project = module.Package.Project;
                task.SourceFile = srcSpaFile.Path;
                task.ToFile = dstSpaFile;

                Log.Info.WriteLine(LogPrefix + "{0}: copy SPA file '{1}' to '{2}'", module.ModuleIdentityString(), task.SourceFile, task.ToFile);

                task.Execute();
            }
            return dstSpaFile;
        }


        protected override string DefaultTargetFrameworkVersion
        {
            get { return IsWinPhone ? "v8.0" : "v4.5"; }
        }

        //IMTODO: THis is temp override. WE need to mane new DotNet package for Winphone
        protected override string TargetFrameworkVersion
        {
            get { return IsWinPhone ? DefaultTargetFrameworkVersion : base.TargetFrameworkVersion; }
        }

        protected override string ToolsVersion
        {
            get { return "4.0"; }
        }

        protected override string ProductVersion
        {
            get { return "10.0.20506"; }
        }

        protected override string Version
        {
            get
            {
                return "11.00";
            }
        }

        protected override ICollection<Guid> ProjectTypeGuids
        {
            get
            {
                var guids = base.ProjectTypeGuids;
                if (IsWinPhone)
                {
                    guids.Add(CSPROJ_WINPHONE_GUID);
                }
                return guids;
            }
        }

        protected virtual ICollection<string> SupportedCultures
        {
            get
            {
                return new List<string>();
            }
        }

        internal override bool SupportsDeploy(Configuration config)
        {
            return IsWinPhone;
        }


        protected override string GetConfigPlatformMapping(Module_DotNet module)
        {
            if (IsWinPhone)
            {
                var platform = module.Compiler.GetOptionValue("platform:");

                switch (platform)
                {
                    case "x86": return "x86";
                    case "arm": return "ARM";
                    default:
                    case "anycpu": return "AnyCPU";
                }
            }

            return base.GetConfigPlatformMapping(module);
        }

        protected override DotNetCompiler.Targets OutputType
        {
            get
            {
                var target = base.OutputType;

                if (IsWinPhone)
                {
                    if (target == DotNetCompiler.Targets.Exe || target == DotNetCompiler.Targets.WinExe)
                    {
                        target = DotNetCompiler.Targets.Library;
                    }
                }

                return target;
            }
        }

        protected override IDictionary<string, string> GetProjectProperties()
        {
            var data = base.GetProjectProperties();

            if (IsWinPhone)
            {
                data.Add("TargetFrameworkIdentifier", "WindowsPhone");

                if (StartupModule.Compiler.Target == DotNetCompiler.Targets.Exe || StartupModule.Compiler.Target == DotNetCompiler.Targets.WinExe)
                {
                    data.Add("SilverlightVersion", "$(TargetFrameworkVersion)");
                    data.Add("SilverlightApplication", true);
                    data.AddNonEmpty("SupportedCultures", SupportedCultures.ToString(Environment.NewLine));

                    data.Add("XapOutputs", true);

                    data.Add("GenerateSilverlightManifest", true);
                    data.Add("XapFilename", String.Format("{0}_$(Configuration)_$(Platform).xap", StartupModule.Compiler.OutputName));
                    data.Add("SilverlightManifestTemplate", @"Properties\AppManifest.xml");
                    data.Add("SilverlightAppEntry", StartupModule.Compiler.OutputName + ".App");
                }
            }

            data.Add("ValidateXaml", true);
            data.Add("MinimumVisualStudioVersion", Version);
            data.Add("ThrowErrorsInValidation", true);

            return data;
        }

        protected override IDictionary<string, string> GetConfigurationProperties(Module_DotNet module)
        {
            var data = base.GetConfigurationProperties(module);

            if (IsWinPhone)
            {
                //When .Net modules are not "anycpu" deployment fails
                data["PlatformTarget"] = "anycpu";
            }

            return data;
        }

        protected override IEnumerable<string> GetDefaultSystemAssemblies(Module_DotNet module)
        {
            if (IsWinPhone)
            {
                return new List<string>();
            }
            return base.GetDefaultSystemAssemblies(module);
        }


        protected override void WriteImportStandardTargets(IXmlWriter writer)
        {
            // On Winphone targets should be written after files.
            if (!IsWinPhone)
            {
                base.WriteImportStandardTargets(writer);
            }
        }

        protected override void WriteFiles(IXmlWriter writer)
        {
            base.WriteFiles(writer);

            WriteAppPackage(writer);

            // On Winphone targets should be written after files
            if (IsWinPhone)
            {
                writer.WriteStartElement("Import");
                writer.WriteAttributeString("Project", @"$(MSBuildExtensionsPath)\Microsoft\$(TargetFrameworkIdentifier)\$(TargetFrameworkVersion)\Microsoft.$(TargetFrameworkIdentifier).$(TargetFrameworkVersion).Overrides.targets");
                writer.WriteEndElement(); //Import
                writer.WriteStartElement("Import");
                writer.WriteAttributeString("Project", @"$(MSBuildExtensionsPath)\Microsoft\$(TargetFrameworkIdentifier)\$(TargetFrameworkVersion)\Microsoft.$(TargetFrameworkIdentifier).CSharp.targets");
                writer.WriteEndElement(); //Import

                if (Modules.Any(m => IsXboxLiveServices(m)))
                {
                    writer.WriteStartElement("Import");
                    writer.WriteAttributeString("Project", @"$(MSBuildExtensionsPath)\Microsoft\XNA Game Studio\v4.0\Microsoft.Xna.GameStudio.XBL.targets");
                    writer.WriteEndElement(); //Import
                }
            }
        }

        protected virtual void WriteAppPackage(IXmlWriter writer)
        {
            //IM. Do we need separate manifest files by configuration?
            //foreach (Module module in Modules)
            var module = StartupModule;
            {
                if (IsWinPhone && (base.OutputType == DotNetCompiler.Targets.Exe || base.OutputType == DotNetCompiler.Targets.WinExe))
                {
                    // Add reference to WMAppManifest.xml for Windows Phone
                    var wmAppManifest = WMAppManifest.Generate(Log, module, PathString.MakeNormalized(ManifestDir), OutputDir, Name);
                    PathString wmAppManifestPath = wmAppManifest.ManifestFilePath;

                    var silverlightManifest = WriteSilverlightManifest("AppManifest.xml");

                    // Write Assets
                    writer.WriteStartElement("ItemGroup");
                    //writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(module.Configuration));
                    foreach (var image in wmAppManifest.ImageFiles)
                    {
                        writer.WriteStartElement("Content");
                        writer.WriteAttributeString("Include", GetProjectPath(image));
                        
                        writer.WriteElementString("CopyToOutputDirectory", "PreserveNewest");
                        writer.WriteEndElement();
                    }
                    writer.WriteEndElement();

                    // Write manifest files

                    writer.WriteStartElement("ItemGroup");
                    //writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(module.Configuration));

                    writer.WriteStartElement("None");
                    writer.WriteAttributeString("Include", GetProjectPath(wmAppManifestPath));

                    writer.WriteEndElement(); //Content

                    writer.WriteStartElement("None");
                    writer.WriteAttributeString("Include", GetProjectPath(silverlightManifest));
                    if (IsXboxLiveServices(module))
                    {
                        var spaFile = CopySpaFile(module);

                        if(!String.IsNullOrEmpty(spaFile))
                        {
                            writer.WriteNonEmptyElementString("LiveGameConfig", GetProjectPath(spaFile));
                        }
                    }
                    writer.WriteEndElement(); //Content

                    writer.WriteEndElement();
                }
            }
        }

        private PathString WriteSilverlightManifest(string manifestName)
        {
            var manifestfile = PathString.MakeCombinedAndNormalized(ManifestDir, manifestName);

            using (IXmlWriter writer = new NAnt.Core.Writers.XmlWriter(ProjectFileWriterFormat))
            {
                writer.FileName = manifestfile.Path;

                writer.WriteStartElement("Deployment", "http://schemas.microsoft.com/client/2007/deployment");
                writer.WriteAttributeString("xmlns", "x", null, "http://schemas.microsoft.com/winfx/2006/xaml");

                writer.WriteStartElement("Deployment.Parts");
                writer.WriteFullEndElement();
            }

            return manifestfile;
        }

        private string ManifestDir
        {
            get
            {
                var dir = Path.Combine(OutputDir.Path, "Properties");
                Directory.CreateDirectory(dir);
                return dir;
            }
        }

        #region Project Type Guid definitions
        // Subtypes
        internal static readonly Guid CSPROJ_WINPHONE_GUID = new Guid("C089C8C0-30E0-4E22-80C0-CE093F111A43");
        #endregion Project Type Guid definitions

        private bool? _isWinPhone = null;
    }
}
