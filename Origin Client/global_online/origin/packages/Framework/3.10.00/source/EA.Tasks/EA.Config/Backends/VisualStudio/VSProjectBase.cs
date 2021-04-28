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
using NAnt.Core.Reflection;
using NAnt.Core.Events;
using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Backends;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal abstract class VSProjectBase : ModuleGenerator
    {

        protected VSProjectBase(VSSolutionBase solution, IEnumerable<IModule> modules, Guid projectTypeGuid) : base(solution, modules)
        {
            LogPrefix = "[visualstudio] ".PadLeft(Log.IndentSize*3);
            // Compute project GUID based on the ModuleKey
            ProjectGuid = GetVSProjGuid();

            ProjectGuidString = solution.ToString(ProjectGuid);

            ProjectTypeGuid = projectTypeGuid;

            SolutionGenerator = solution;

            Platforms = Modules.Select(m => GetProjectTargetPlatform(m.Configuration)).Distinct().OrderBy(s => s);

            ProjectConfigurationNameOverrides = new Dictionary<Configuration, string>();

            PopulateConfigurationNameOverrides();

            PopulateUserData();
        }

        protected abstract IXmlWriterFormat ProjectFileWriterFormat { get; }

        protected abstract string Version { get; }

        protected abstract string UserFileName { get; }

        protected abstract IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string,string>>>> GetUserData(ProcessableModule module);

        /// <summary>The prefix used when sending messages to the log.</summary>
        public readonly string LogPrefix;

        internal readonly IEnumerable<string> Platforms;

        internal readonly Guid ProjectGuid;

        internal readonly Guid ProjectTypeGuid;

        internal readonly string ProjectGuidString;

        internal SolutionFolders.SolutionFolder SolutionFolder = null;

        protected readonly VSSolutionBase SolutionGenerator;

        internal virtual string GetProjectTargetPlatform(Configuration configuration)
        {
            return SolutionGenerator.GetTargetPlatform(configuration);
        }

        protected virtual Guid GetVSProjGuid()
        {
            return Hash.MakeGUIDfromString(Name);
        }


        internal virtual string GetVSProjConfigurationName(Configuration configuration)
        {
            return String.Format("{0}|{1}", GetVSProjTargetConfiguration(configuration), GetProjectTargetPlatform(configuration));
        }

        internal string GetVSProjTargetConfiguration(Configuration configuration)
        {
            string targetconfig;
            if (!ProjectConfigurationNameOverrides.TryGetValue(configuration, out targetconfig))
            {
                targetconfig = configuration.Name;
            }
            return targetconfig;
        }

        protected void OnProjectFileUpdate(object sender, CachedWriterEventArgs e)
        {
            if (e != null)
            {
                Log.Status.WriteLine("{0}{1} project file  '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
            }
        }

        protected virtual void PopulateConfigurationNameOverrides()
        {
            foreach (var module in Modules)
            {
                if (module.Package.Project != null)
                {
                    var configName = module.Package.Project.Properties[module.GroupName + ".vs-config-name"] ?? module.Package.Project.Properties["eaconfig.vs-config-name"];
                    if (!String.IsNullOrEmpty(configName))
                    {
                        ProjectConfigurationNameOverrides[module.Configuration] = configName;
                    }
                }
            }
        }

        protected virtual void PopulateUserData()
        {
        }

        internal virtual bool SupportsDeploy(Configuration config)
        {
            return false;
        }

        protected abstract void WriteUserFile();

        protected abstract void WriteProject(IXmlWriter writer);

        public override void WriteProject()
        {
            using (IXmlWriter writer = new NAnt.Core.Writers.XmlWriter(ProjectFileWriterFormat))
            {
                writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnProjectFileUpdate);

                writer.FileName = Path.Combine(OutputDir.Path, ProjectFileName);

                writer.WriteStartDocument();

                WriteProject(writer);
            }

            WriteUserFile();
        }


        protected virtual void WriteSourceControlIntegration(IXmlWriter writer)
        {
            if (Package.SourceControl.UsingSourceControl && SourceControl != null)
            {
                writer.WriteElementString("SccProjectName", Package.SourceControl.Type.ToString() + " Project");
                writer.WriteElementString("SccAuxPath", PathUtil.RelativePath(SourceControl.SccAuxPath, OutputDir.Path));
                writer.WriteElementString("SccLocalPath", PathUtil.RelativePath(SourceControl.SccLocalPath, OutputDir.Path));
                writer.WriteElementString("SccProvider", SourceControl.SccProvider);
            }
        }

        protected override SourceControlData CreateSourceControlData()
        {
            SourceControlData sc = null;
            if(Package.SourceControl.UsingSourceControl)
            {
                sc = new SourceControlData();
                //IMTODO
                sc.SccLocalPath = "ssclocal" ; // GetSccLocalPath(string commonRoot, string projRoot)
                sc.SccAuxPath = String.Empty;
                sc.SccProjectName = Path.Combine(RelativeDir, ProjectFileName);

                switch(Package.SourceControl.Type)
                {
                    case FrameworkTasks.Model.SourceControl.Types.Perforce:
                        sc.SccProvider = "MSSCCI:Perforce SCM";
                        break;
                    default:
                        sc.SccProvider = "MSSCCI:Unknown SCM";
                        break;
                }            
            }
            return sc;
        }

        protected string GetProjDirPath(PathString path)
        {
            string relative = PathUtil.RelativePath(path.Path, OutputDir.Path);

            if (!Path.IsPathRooted(relative))
            {
                relative = (String.IsNullOrEmpty(relative) || relative[0] == '\\' || relative[0] == '/') ? 
                    String.Format("{0}{1}", "$(ProjectDir)", relative):
                    String.Format("{0}{1}{2}", "$(ProjectDir)", Path.DirectorySeparatorChar, relative);
            }
            return relative;
        }

        protected string GetProjectPath(PathString path, bool addDot=false)
        {
            if (path == null)
            {
                return String.Empty;
            }

            return GetProjectPath(path.Normalize(PathString.PathParam.NormalizeOnly).Path, addDot);
        }


        protected string GetProjectPath(string path, bool addDot=false)
        {
            string projectPath = path;

            switch (BuildGenerator.PathMode)
            {
                case Generator.PathModeType.Auto:
                    {
                        if (PathUtil.IsPathInDirectory(path, OutputDir.Path))
                        {
                            projectPath = PathUtil.RelativePath(path, OutputDir.Path, addDot:addDot);
                        }
                        else
                        {
                            projectPath = path;
                        }
                    }
                    break;
                case Generator.PathModeType.Relative:
                    {
                        bool isSdk = false;
                        if (BuildGenerator.IsPortable && !PathUtil.IsPathInDirectory(path, OutputDir.Path))
                        {
                            // Check if path is an SDK path
                            projectPath = BuildGenerator.PortableData.FixPath(path, out isSdk);
                        }
                        if (path.StartsWith("%") || path.StartsWith("\"%"))
                        {
                            isSdk = true;
                        }

                        if (!isSdk)
                        {
                            projectPath = PathUtil.RelativePath(path, OutputDir.Path, addDot: addDot);

                            // Need to check the combined length which the relative path is going to have
                            // before choosing whether to relativize or not.  Before we had this check in place
                            // it was possible for <nanttovcproj> to generate an unbuildable VCPROJ, because
                            // the relativized path overflowed the maximum path length limit.
                            string combinedFilePath = Path.Combine(OutputDir.Path, projectPath);
                            if (combinedFilePath.Length >= 260)
                            {
                                Log.Warning.WriteLine("Relative path combined with project path is longer than 260 characters which can lead to unbuildable project: {0}", combinedFilePath);
                            }
                        }
                    }
                    break;
                case Generator.PathModeType.Full:
                    {
                        projectPath = path;
                    }
                    break;
            }

            return projectPath.TrimQuotes();
        }


        protected IDictionary<PathString, FileEntry> GetAllFiles(Func<Tool, IEnumerable<Tuple<FileSet, uint, Tool>>> func)
        {
            var files = new Dictionary<PathString, FileEntry>();

            foreach (var module in Modules.Cast<Module>())
            {
                foreach (Tool tool in module.Tools)
                {
                    UpdateFiles(files, tool, module.Configuration, func);
                }

                // Add non-buildable source files:
                var excluded_sources_flag = module.IsKindOf(Module.MakeStyle) ? 0 : FileEntry.ExcludedFromBuild;
                foreach (var fileitem in module.ExcludedFromBuild_Sources.FileItems)
                {
                    UpdateFileEntry(files, fileitem.Path, fileitem, module.ExcludedFromBuild_Sources.BaseDirectory, module.Configuration, flags: excluded_sources_flag);
                }

                foreach (var fileitem in module.ExcludedFromBuild_Files.FileItems)
                {
                    uint flags = (fileitem.IsKindOf(FileData.Header | FileData.Resource) || module.IsKindOf(Module.MakeStyle)) ? 0 : FileEntry.ExcludedFromBuild; 

                    UpdateFileEntry(files, fileitem.Path, fileitem, module.ExcludedFromBuild_Files.BaseDirectory, module.Configuration, flags: flags);
                }

                foreach (var fileitem in module.Assets.FileItems)
                {
                    UpdateFileEntry(files, fileitem.Path, fileitem, module.Assets.BaseDirectory, module.Configuration, flags: FileEntry.Assets);
                }
            }
            if (IncludeBuildScripts)
            {
                foreach (var module in Modules)
                {
                    // Dynamically created packages may use exising projects from different packages. Skip such projects
                    if (module.Package.Name == module.Package.Project.Properties["package.name"] && module.Configuration.Name == module.Package.Project.Properties["config"])
                    {
                        string basedir = Package.Dir.Path;

                        string scriptsdir = Path.Combine(Package.Dir.Path, "scripts");

                        FileSet buildscripts = new FileSet();

                        if (module.ScriptFile != null && !String.IsNullOrEmpty(module.ScriptFile.Path))
                        {
                            basedir = Path.GetDirectoryName(module.ScriptFile.Path);
                        }

                        buildscripts.BaseDirectory = basedir;

                        basedir = basedir.EnsureTrailingSlash();

                        var currentbuildfile = module.ScriptFile != null ? module.ScriptFile.Path : module.Package.Project.BuildFileLocalName;

                        buildscripts.Include(currentbuildfile, failonmissing: false);

                        var included = module.Package.Project.IncludedFiles.GetIncludedFiles(currentbuildfile);
                        if (included != null)
                        {
                            foreach (var path in included.Where(p => p.StartsWith(basedir, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase)))
                            {
                                buildscripts.Include(path, failonmissing: false);
                            }
                        }

                        if (scriptsdir.StartsWith(basedir, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase))
                        {
                            buildscripts.Include(scriptsdir + "/**");
                        }

                        foreach (var fileitem in buildscripts.FileItems)
                        {
                            UpdateFileEntry(files, fileitem.Path, fileitem, buildscripts.BaseDirectory, module.Configuration, flags: FileEntry.NonBuildable);
                        }
                    }
                }
            }

            return files;
        }

        private void UpdateFiles(Dictionary<PathString, FileEntry> files, Tool tool, Configuration config, Func<Tool, IEnumerable<Tuple<FileSet, uint, Tool>>> func)
        {
            foreach (var set in func(tool))
            {
                foreach (var fileitem in set.Item1.FileItems)
                {
                    if (!UpdateFileEntry(files, fileitem.Path, fileitem, set.Item1.BaseDirectory, config, tool: set.Item3, flags: set.Item2))
                    {
                        FileEntry entry;
                        if (files.TryGetValue(fileitem.Path, out entry))
                        {
                            var configEntry = entry.FindConfigEntry(config);
                            if (configEntry != null && configEntry.Tool != null)
                            {
                                Log.Warning.WriteLine("{0}-{1} ({2}) module '{3}' : More than one tool ('{4}', '{5}') includes same file '{6}'.", Package.Name, Package.Version, config.Name, ModuleName, configEntry.Tool.ToolName, tool.ToolName, fileitem.Path);
                            }
                        }
                    }
                }
            }
        }

        protected bool UpdateFileEntry(IDictionary<PathString, FileEntry> files, PathString key, FileItem fileitem, string basedirectory, Configuration config, Tool tool = null, uint flags = 0)
        {
            FileEntry entry;
            if (!files.TryGetValue(key, out entry))
            {
                entry = new FileEntry(fileitem.Path, fileitem.BaseDirectory??basedirectory);
                files.Add(key, entry);
            }
            if (fileitem.IsKindOf(FileData.BulkBuild))
            {
                flags |= FileEntry.BulkBuild; 
            }
            if (fileitem.IsKindOf(FileData.Header))
            {
                flags |= FileEntry.Headers; 
            }
            if (fileitem.IsKindOf(FileData.Asset))
            {
                flags |= FileEntry.Assets;
            }
            if (fileitem.IsKindOf(FileData.Resource))
            {
                flags |= FileEntry.Resources;
            }

            if (tool is EA.Eaconfig.Modules.Tools.BuildTool)
            {
                flags |= FileEntry.CustomBuildTool;
            }

            return entry.TryAddConfigEntry(fileitem, config, tool, flags);
        }


        internal readonly IDictionary<Configuration, string> ProjectConfigurationNameOverrides;

        #region Project Type Guid definitions
        internal static readonly Guid VCPROJ_GUID = new Guid("8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942");
        internal static readonly Guid CSPROJ_GUID = new Guid("FAE04EC0-301F-11D3-BF4B-00C04F79EFBC");
        internal static readonly Guid FSPROJ_GUID = new Guid("F2A71F9B-5D33-465A-A702-920D77279786");
        internal static readonly Guid WEBAPP_GUID = new Guid("349C5851-65DF-11DA-9384-00065B846F21");
        internal static readonly Guid SANDCASTLE_GUID = new Guid("7CF6DF6D-3B04-46F8-A40B-537D21BCA0B4");
        internal static readonly Guid PYPROJ_GUID = new Guid("888888A0-9F3D-457C-B088-3A5042F75D52");
        //Subtypes
        internal static readonly Guid TEST_GUID = new Guid("3AC096D0-A1C2-E12C-1390-A8335801FDAB");
        internal static readonly Guid CSPROJ__WORKFLOW_GUID = new Guid("14822709-B5A1-4724-98CA-57A101D1B079");
        #endregion Project Type Guid definitions

    }
}
