using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Globalization;
using System.Security.Principal;
using System.Runtime.InteropServices;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.PackageCore;
using NAnt.Core.Writers;
using NAnt.Core.Events;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class ExternalVSProject : VSProjectBase
    {
        public ExternalVSProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            :
            base(solution, modules, GetProjectTypeGuid(modules))
        {
            var vsmodule = modules.FirstOrDefault() as Module_ExternalVisualStudio;
            _projectFile = vsmodule.VisualStudioProject;
            _projectPlatforms = new Dictionary<Configuration, string>();

            foreach (Module_ExternalVisualStudio module in Modules)
            {
                if (!String.IsNullOrEmpty(module.VisualStudioProjectPlatform))
                {
                    _projectPlatforms[module.Configuration] = module.VisualStudioProjectPlatform;
                }
            }
        }

        protected override Guid GetVSProjGuid()
        {
            var vsmodule = Modules.FirstOrDefault() as Module_ExternalVisualStudio;
            return vsmodule.VisualStudioProjectGuild;
        }

        /// <summary>
        /// module generator output directory relative to the to solution build root:
        /// </summary>
        internal override string RelativeDir
        {
            get
            {
                if (_relativeDir == null)
                {
                    _relativeDir = PathUtil.RelativePath(OutputDir.Path, BuildGenerator.OutputDir.Path);
                }
                return _relativeDir;
            }
        } private string _relativeDir;

        public override PathString OutputDir
        {
            get
            {
                if (_outputDir == null)
                {
                    _outputDir = PathString.MakeNormalized(Path.GetDirectoryName(_projectFile.Path));
                }
                return _outputDir;
            }

        } private PathString _outputDir;

        private static Guid GetProjectTypeGuid(IEnumerable<IModule> modules)
        {
            var vsmodule = modules.FirstOrDefault() as Module_ExternalVisualStudio;
            if (vsmodule != null)
            {
                var ext = Path.GetExtension(vsmodule.VisualStudioProject.Path).ToLowerInvariant();
                switch(ext)
                {
                    case ".csproj":
                        return CSPROJ_GUID;
                    case ".fsproj":
                        return FSPROJ_GUID;
                    case ".shfbproj":
                        return SANDCASTLE_GUID;
                    case ".vcxproj":
                    default:
                        return VCPROJ_GUID;
                }
            }
            return VCPROJ_GUID;
        }

        public override string ProjectFileName 
        {
            get { return Path.GetFileName(_projectFile.Path); }
        }


        protected override void PopulateConfigurationNameOverrides()
        {
            foreach (Module_ExternalVisualStudio module in Modules)
            {
               if(!String.IsNullOrEmpty(module.VisualStudioProjectConfig))
               {
                   ProjectConfigurationNameOverrides[module.Configuration] = module.VisualStudioProjectConfig;
               }
            }
        }

        internal override string GetProjectTargetPlatform(Configuration configuration)
        {
            return _projectPlatforms[configuration];
        }


        #region None of these functions used in the ExternalVSProject

        protected override IXmlWriterFormat ProjectFileWriterFormat { get { return null; } }

        protected override string Version { get { return null; } }

        protected override string UserFileName { get { return null; } }

        protected override IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>> GetUserData(ProcessableModule module) { return null; }

        protected override void WriteUserFile() { }

        protected override void WriteProject(IXmlWriter writer) { }

        public override void WriteProject() { }

        #endregion

        private readonly PathString _projectFile;

        private readonly IDictionary<Configuration, string> _projectPlatforms;
    }

}
