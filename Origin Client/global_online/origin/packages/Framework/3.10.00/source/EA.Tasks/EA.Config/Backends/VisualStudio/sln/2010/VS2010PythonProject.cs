using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using EA.FrameworkTasks.Model;
using NAnt.Core;
using EA.Eaconfig.Core;
using NAnt.Core.Writers;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using NAnt.Core.Util;
using System.IO;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class VS2010PythonProject : VSProjectBase
    {
        protected readonly Module StartupModule;

        internal VS2010PythonProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules, PYPROJ_GUID)
        {
            // Get project configuration that equals solution startup config:
            Configuration startupconfig;
            if (ConfigurationMap.TryGetValue(solution.StartupConfiguration, out startupconfig))
            {
                StartupModule = Modules.Single(m => m.Configuration == startupconfig) as Module;
            }
            else
            {
                StartupModule = Modules.First() as Module;
            }

            if (StartupModule != null && StartupModule is Module_Python)
            {
                Module_Python python_module = (StartupModule as Module_Python);
                SearchPath = python_module.SearchPath;
                StartupFile = python_module.StartupFile;
                WorkingDirectory = python_module.WorkDir;
                ProjectHome = python_module.ProjectHome;
                IsWindowsApplication = python_module.IsWindowsApp;
            }
        }

        public override string ProjectFileName
        {
            get
            {
                return ProjectFileNameWithoutExtension + ".pyproj";
            }
        }

        protected override string Version
        {
            get
            {
                return "10.00";
            }
        }

        protected virtual string Xmlns
        {
            get { return "http://schemas.microsoft.com/developer/msbuild/2003"; }
        }

        protected virtual string DefaultTarget
        {
            get { return "Build"; }
        }

        protected virtual string StartupConfiguration
        {
            get { return StartupModule.Configuration.Name; }
        }

        protected virtual string SchemaVersion
        {
            get { return "2.0"; }
        }

        protected virtual string ProjectHome
        {
            get { return _projecthome; }
            set { _projecthome = value; }
        } private string _projecthome = ".";

        protected virtual string SearchPath
        {
            get { return _searchpath; }
            set { _searchpath = value; }
        } private string _searchpath = String.Empty;

        protected virtual string WorkingDirectory
        {
            get { return _workdir; }
            set { _workdir = value; }
        } private string _workdir = ".";

        protected virtual string OutputPath
        {
            get { return "."; }
        }

        protected virtual string StartupFile
        {
            get { return _startupfile; }
            set { _startupfile = value; }
        } private string _startupfile = "";

        protected virtual string IsWindowsApplication
        {
            get { return _windowsapp; }
            set { _windowsapp = value; }
        } private string _windowsapp = "false";

        protected override IXmlWriterFormat ProjectFileWriterFormat
        {
            get { return _xmlWriterFormat; }
        }

        protected override string UserFileName
        {
            get { return ProjectFileName + ".user"; } 
        }

        protected override IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>> GetUserData(ProcessableModule module)
        {
            throw new NotImplementedException();
        }

        protected override void WriteUserFile()
        {
            
        }

        protected override void WriteProject(IXmlWriter writer)
        {
            writer.WriteStartElement("Project", Xmlns);
            writer.WriteAttributeString("DefaultTargets", DefaultTarget);

            WriteProjectProperties(writer);

            WriteConfigurations(writer);

            WriteFiles(writer);

            WriteImportTargets(writer);

            writer.WriteEndElement(); //Project
        }

        protected virtual void WriteProjectProperties(IXmlWriter writer)
        {
            writer.WriteStartElement("PropertyGroup");

            writer.WriteStartElement("Configuration");
            writer.WriteAttributeString("Condition", " '$(Configuration)' == '' ");
            writer.WriteString(StartupConfiguration);
            writer.WriteEndElement();

            foreach (var entry in GetProjectProperties())
            {
                writer.WriteElementString(entry.Key, entry.Value);
            }

            WriteSourceControlIntegration(writer);

            writer.WriteEndElement(); //PropertyGroup
        }

        protected virtual IDictionary<string, string> GetProjectProperties()
        {
            var data = new OrderedDictionary<string, string>();

            data.AddNonEmpty("SchemaVersion", SchemaVersion);
            data.Add("ProjectGuid", ProjectGuidString);
            data.Add("ProjectHome", ProjectHome);

            data.AddNonEmpty("StartupFile", StartupFile);
            data.AddNonEmpty("SearchPath", SearchPath);
            data.Add("WorkingDirectory", WorkingDirectory);
            data.Add("OutputPath", OutputPath);
            data.Add("Name", Name);
            data.Add("IsWindowsApplication", IsWindowsApplication);

            return data;
        }

        protected virtual void WriteConfigurations(IXmlWriter writer)
        {
            foreach (Module_Python module in Modules)
            {
                WriteConfigurationProperties(writer, module);
            }
        }

        protected virtual void WriteConfigurationProperties(IXmlWriter writer, Module_Python module)
        {
            writer.WriteStartElement("PropertyGroup");
            writer.WriteAttributeString("Condition", String.Format(" '$(Configuration)' == '{0}' ", StartupConfiguration));

            foreach (var entry in GetConfigurationProperties(module))
            {
                writer.WriteElementString(entry.Key, entry.Value);
            }

            writer.WriteEndElement(); //PropertyGroup
        }

        protected virtual IDictionary<string, string> GetConfigurationProperties(Module_Python module)
        {
            var data = new OrderedDictionary<string, string>();

            data.Add("DebugSymbols", "true");
            data.Add("EnableUnmanagedDebugging", "false");

            return data;
        }

        protected virtual void WriteFiles(IXmlWriter writer)
        {
            var files = GetAllFiles(tool =>
            {
                var filesets = new List<Tuple<FileSet, uint, Tool>>();
                filesets.Add(Tuple.Create((tool as PythonInterpreter).ContentFiles, FileEntry.ExcludedFromBuild, tool));
                filesets.Add(Tuple.Create((tool as PythonInterpreter).SourceFiles, FileEntry.Buildable, tool));

                return filesets;
            }).Select(e => e.Value);

            foreach (var group in files.GroupBy(f => f.ConfigEntries.OrderBy(ce => ce.Configuration.Name).Aggregate(new StringBuilder(), (result, ce) => result.AppendLine(ce.Configuration.Name)).ToString(), f => f))
            {
                if (group.Count() > 0)
                {
                    // Add Item File Items
                    writer.WriteStartElement("ItemGroup");
                    WriteConfigCondition(writer, group);

                    foreach (var fe in group)
                    {
                        var relPath = PathUtil.RelativePath(fe.Path.Path, OutputDir.Path);
                        var relPathKey = relPath.TrimExtension();

                        var firstFe = fe.ConfigEntries.First();

                        if (firstFe.IsKindOf(FileEntry.Buildable))
                        {
                            writer.WriteStartElement("Compile");
                            writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                            WriteLinkPath(writer, fe);
                            writer.WriteEndElement(); 
                        }
                        else if (firstFe.IsKindOf(FileEntry.ExcludedFromBuild))
                        {
                            writer.WriteStartElement("Content");
                            writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
                            WriteLinkPath(writer, fe);
                            writer.WriteEndElement(); 
                        }
                    }

                    writer.WriteEndElement(); //ItemGroup
                }
            }
        }

        protected void WriteLinkPath(IXmlWriter writer, FileEntry fe)
        {
            if (!fe.Path.IsPathInDirectory(OutputDir))
            {
                writer.WriteNonEmptyElementString("Link", GetLinkPath(fe));
            }
        }

        protected virtual string GetLinkPath(FileEntry fe)
        {
            string link_path = String.Empty;

            if (!String.IsNullOrEmpty(fe.FileSetBaseDir) && fe.Path.Path.StartsWith(fe.FileSetBaseDir))
            {
                link_path = PathUtil.RelativePath(fe.Path.Path, fe.FileSetBaseDir).TrimStart(new char[] { '/', '\\', '.' });
            }
            else
            {
                link_path = PathUtil.RelativePath(fe.Path.Path, OutputDir.Path).TrimStart(new char[] { '/', '\\', '.' });
            }

            if (!String.IsNullOrEmpty(link_path))
            {
                if (Path.IsPathRooted(link_path) || (link_path.Length > 1 && link_path[1] == ':'))
                {
                    link_path = link_path[0] + "_Drive" + link_path.Substring(2);
                }
            }

            return link_path;
        }

        protected void WriteConfigCondition(IXmlWriter writer, IEnumerable<FileEntry> files)
        {
            var fe = files.FirstOrDefault();
            if (fe != null)
            {
                WriteConfigCondition(writer, fe.ConfigEntries.Select(ce => ce.Configuration));
            }
        }

        protected void WriteConfigCondition(IXmlWriter writer, IEnumerable<Configuration> configs)
        {
            writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(configs));
        }

        internal string GetConfigCondition(Configuration config)
        {
            return " '$(Configuration)|$(Platform)' == '" + GetVSProjConfigurationName(config) + "' ";
        }

        protected string GetConfigCondition(IEnumerable<Configuration> configs)
        {
            var condition = MSBuildConfigConditions.GetCondition(configs, AllConfigurations);

            return MSBuildConfigConditions.FormatCondition("$(Configuration)|$(Platform)", condition, GetVSProjConfigurationName);
        }

        protected virtual void WriteImportTargets(IXmlWriter writer)
        {
            writer.WriteStartElement("Import");
            writer.WriteAttributeString("Project", String.Format(@"$(MSBuildToolsPath)\Microsoft.Common.targets"));
            writer.WriteEndElement(); //Import
        }

        private static readonly IXmlWriterFormat _xmlWriterFormat = new XmlFormat(
            identChar: ' ',
            identation: 2,
            newLineOnAttributes: false,
            encoding: new UTF8Encoding(false) // no byte order mask!
        );
    }
}
