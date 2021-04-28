// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal sealed class VSPythonProject : VSProjectBase
	{
		private readonly Module StartupModule;

		internal VSPythonProject(VSSolutionBase solution, IEnumerable<IModule> modules)
			: base(solution, modules, PYPROJ_GUID)
		{
			// Get project configuration that equals solution startup config:
			if (solution.StartupConfiguration != null)
			{
				StartupModule = Modules.Single(m => m.Configuration == solution.StartupConfiguration) as Module;
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
				ProjectHome = python_module.ProjectHome;
				IsWindowsApplication = python_module.IsWindowsApp;
				Environment = python_module.Environment;
			}
		}

		public override string ProjectFileName
		{
			get
			{
				return ProjectFileNameWithoutExtension + ".pyproj";
			}
		}

		internal override string RootNamespace => Name;

		private string Xmlns
		{
			get { return "http://schemas.microsoft.com/developer/msbuild/2003"; }
		}

		private string DefaultTarget
		{
			get { return "Build"; }
		}

		private string SchemaVersion
		{
			get { return "2.0"; }
		}

		private string ProjectHome
		{
			get { return _projecthome; }
			set { _projecthome = value; }
		}
		private string _projecthome = ".";

		private string SearchPath
		{
			get { return _searchpath; }
			set { _searchpath = value; }
		}
		private string _searchpath = String.Empty;


		private string OutputPath
		{
			get { return "."; }
		}

		private string StartupFile
		{
			get { return _startupfile; }
			set { _startupfile = value; }
		}
		private string _startupfile = "";

		private string IsWindowsApplication
		{
			get { return _windowsapp; }
			set { _windowsapp = value; }
		}
		private string _windowsapp = "false";

		private OptionSet Environment
		{
			get { return m_Environment; }
			set { m_Environment = value; }
		}
		private OptionSet m_Environment = new OptionSet();

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
			writer.WriteAttributeString("ToolsVersion", "4.0");

			WriteProjectProperties(writer);

			WriteConfigurations(writer);

			WriteFiles(writer);

			WriteImportTargets(writer);

			writer.WriteEndElement(); //Project
		}

		private void WriteProjectProperties(IXmlWriter writer)
		{
			writer.WriteStartElement("PropertyGroup");

			writer.WriteStartElement("Configuration");
			writer.WriteAttributeString("Condition", " '$(Configuration)' == '' ");
			writer.WriteString(GetVSProjTargetConfiguration(BuildGenerator.StartupConfiguration));
			writer.WriteEndElement();

			foreach (var entry in GetProjectProperties())
			{
				writer.WriteElementString(entry.Key, entry.Value);
			}

			writer.WriteEndElement(); //PropertyGroup
		}

		private IDictionary<string, string> GetProjectProperties()
		{
			var data = new OrderedDictionary<string, string>();

			data.AddNonEmpty("SchemaVersion", SchemaVersion);
			data.Add("ProjectGuid", ProjectGuidString);
			data.Add("ProjectHome", ProjectHome);

			data.AddNonEmpty("StartupFile", StartupFile);
			data.AddNonEmpty("SearchPath", SearchPath);
			if (StartupModule is Module_Python m)
			{
				data.Add("WorkingDirectory", m.WorkDir);
			}
			data.Add("OutputPath", OutputPath);
			data.Add("Name", Name);
			data.Add("IsWindowsApplication", IsWindowsApplication);

			if (Environment != null && Environment.Options.Count > 0)
			{
				StringBuilder env = new StringBuilder();
				foreach (var opt in Environment.Options)
				{
					env.AppendLine(string.Format("{0}={1}", opt.Key, opt.Value));
				}
				data.Add("Environment", env.ToString().TrimEnd());
			}

			return data;
		}

		private void WriteConfigurations(IXmlWriter writer)
		{
			foreach (Module_Python module in Modules)
			{
				WriteConfigurationProperties(writer, module);
			}
		}

		private void WriteConfigurationProperties(IXmlWriter writer, Module_Python module)
		{

			writer.WriteStartElement("PropertyGroup");
			writer.WriteAttributeString("Condition", GetConfigCondition(module.Configuration));
			
			foreach (var entry in GetConfigurationProperties(module))
			{
				writer.WriteElementString(entry.Key, entry.Value);
			}

			writer.WriteEndElement(); //PropertyGroup
		}

		private IDictionary<string, string> GetConfigurationProperties(Module_Python module)
		{
			var data = new OrderedDictionary<string, string>();

			data.Add("DebugSymbols", "true");
			data.Add("EnableUnmanagedDebugging", "false");
			data.Add("WorkingDirectory", module.WorkDir);
			return data;
		}

		private void WriteFiles(IXmlWriter writer)
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

		private void WriteLinkPath(IXmlWriter writer, FileEntry fe)
		{
			if (!fe.Path.IsPathInDirectory(OutputDir))
			{
				writer.WriteNonEmptyElementString("Link", GetLinkPath(fe));
			}
		}

		private string GetLinkPath(FileEntry fe)
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

		private void WriteConfigCondition(IXmlWriter writer, IEnumerable<FileEntry> files)
		{
			var fe = files.FirstOrDefault();
			if (fe != null)
			{
				WriteConfigCondition(writer, fe.ConfigEntries.Select(ce => ce.Configuration));
			}
		}

		private void WriteConfigCondition(IXmlWriter writer, IEnumerable<Configuration> configs)
		{
			writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(configs));
		}

		private void WriteImportTargets(IXmlWriter writer)
		{
			writer.WriteStartElement("Import");
			writer.WriteAttributeString("Project", @"$(PtvsTargetsFile)");
			writer.WriteAttributeString("Condition", @"Exists($(PtvsTargetsFile))");
			writer.WriteEndElement(); //Import

			writer.WriteStartElement("Import");
			writer.WriteAttributeString("Project", @"$(MSBuildToolsPath)\Microsoft.Common.targets");
			writer.WriteAttributeString("Condition", @"!Exists($(PtvsTargetsFile))");
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