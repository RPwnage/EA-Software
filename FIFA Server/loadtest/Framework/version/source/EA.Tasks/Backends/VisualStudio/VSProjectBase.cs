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
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Writers;

using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal abstract class VSProjectBase : ModuleGenerator, IVSProject
	{
		protected string Version { get => SolutionGenerator.VSVersion.Substring(0, 2) + ".00"; }

		// TODO vs project class hierarhcy clean up
		// 2016/04/18
		// this is a pretty general todo, but all the derived classes of VSProject base have a lot of duplicated 
		// code relating to MSBuild generation - all the VS versions we support use MSBuild now so much of this
		// could be moved the base class
		protected VSProjectBase(VSSolutionBase solution, IEnumerable<IModule> modules, Guid projectTypeGuid) : base(solution, modules)
		{
			LogPrefix = " [visualstudio] ";
			// Compute project GUID based on the ModuleKey
			ProjectGuid = GetVSProjGuid();

			ProjectGuidString = VSSolutionBase.ToString(ProjectGuid);

			ProjectTypeGuid = projectTypeGuid;

			SolutionGenerator = solution;

			Platforms = Modules.Select(m => GetProjectTargetPlatform(m.Configuration)).Distinct().OrderBy(s => s);

			ProjectConfigurationNameOverrides = new Dictionary<Configuration, string>();

			PopulateConfigurationNameOverrides();

			PopulateUserData();
		}

		public string ProjectGuidString { get; internal set; }

		protected abstract IXmlWriterFormat ProjectFileWriterFormat { get; }

		protected abstract string UserFileName { get; }

		internal abstract string RootNamespace { get; }

		protected abstract IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string,string>>>> GetUserData(ProcessableModule module);

		public virtual IEnumerable<SolutionEntry> SolutionEntries => new SolutionEntry[]
		{
			new SolutionEntry(Name, ProjectGuidString, ProjectFileName, RelativeDir, ProjectTypeGuid, deployConfigs: ConfigurationMap.Values.Where(config => SupportsDeploy(config)).ToArray())
		};

		/// <summary>The prefix used when sending messages to the log.</summary>
		public readonly string LogPrefix;

		public Guid ProjectTypeGuid { get; }

		internal readonly IEnumerable<string> Platforms;

		internal readonly Guid ProjectGuid;

		internal SolutionFolders.SolutionFolder SolutionFolder = null;
		protected readonly VSSolutionBase SolutionGenerator;

		protected readonly List<CopyLocalInfo> HandledCopyLocalOperations = new List<CopyLocalInfo>(); // tracks copy local operations that have been handled by 
		// regular item group references and do not need a custom copy local entry

		public virtual string GetProjectTargetPlatform(Configuration configuration)
		{
			return SolutionGenerator.GetTargetPlatform(configuration);
		}

		protected virtual Guid GetVSProjGuid()
		{
			return Hash.MakeGUIDfromString(Name);
		}


		public virtual string GetVSProjConfigurationName(Configuration configuration)
		{
			return String.Format("{0}|{1}", GetVSProjTargetConfiguration(configuration), GetProjectTargetPlatform(configuration));
		}

		public string GetVSProjTargetConfiguration(Configuration configuration)
		{
			if (!ProjectConfigurationNameOverrides.TryGetValue(configuration, out string targetconfig))
			{
				targetconfig = configuration.Name;
			}
			return targetconfig;
		}

		protected void OnProjectFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} project file  '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
					Log.Minimal.WriteLine(message);
				else
					Log.Status.WriteLine(message);
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
			using (IXmlWriter writer = new XmlWriter(ProjectFileWriterFormat))
			{
				writer.CacheFlushed += new CachedWriterEventHandler(OnProjectFileUpdate);

				writer.FileName = Path.Combine(OutputDir.Path, ProjectFileName);

				writer.WriteStartDocument();

				WriteProject(writer);
			}

			WriteUserFile();
		}

		/// <summary>Allows users to inject custom elements into their vcxproj file, so they don't need to wait for us to update framework</summary>
		protected virtual void AddCustomVsOptions(IXmlWriter writer, Module module)
		{
			Dictionary<string, string> config_option_dictionary = new Dictionary<string, string>();

			OptionSet options = module.GetModuleOptions();
			if (options != null)
			{
				string msbuildoptions = options.GetOptionOrDefault("msbuildoptions", null);
				if (msbuildoptions.IsNullOrEmpty() == false) 
				{
					string[] elements = msbuildoptions.Split(new char[] { '\n', '\r' }, StringSplitOptions.RemoveEmptyEntries);
					foreach (string element in elements)
					{
						string[] tokens = element.Trim().Split(new char[] { '=' }, StringSplitOptions.RemoveEmptyEntries);
						if (tokens.Length == 2 && config_option_dictionary.ContainsKey(tokens[0]) == false)
						{
							config_option_dictionary.Add(tokens[0], tokens[1]);
						}
					}
				}
			}

			OptionSet configbuildoptions = module.Package.Project.NamedOptionSets[module.GroupName + ".msbuildoptions"];
			if (configbuildoptions != null)
			{
				foreach (var option in configbuildoptions.Options)
				{
					if (config_option_dictionary.ContainsKey(option.Key) == false)
					{
						config_option_dictionary.Add(option.Key, option.Value);
					}
				}
			}

			foreach (KeyValuePair<string, string> kv in config_option_dictionary)
			{
				writer.WriteElementString(kv.Key, kv.Value);
			}
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

		protected IDictionary<PathString, FileEntry> GetAllFiles(Func<Tool, IEnumerable<Tuple<FileSet, uint, Tool>>> func)
		{
			var files = new Dictionary<PathString, FileEntry>();

			foreach (var module in Modules.Cast<Module>())
			{
				foreach (Tool tool in module.Tools)
				{
					UpdateFiles(module, files, tool, module.Configuration, func);
				}

				// Add non-buildable source files:
				var excluded_sources_flag = module.IsKindOf(Module.MakeStyle) ? 0 : FileEntry.ExcludedFromBuild;
				foreach (var fileitem in module.ExcludedFromBuild_Sources.FileItems)
				{
					UpdateFileEntry(module, files, fileitem.Path, fileitem, module.ExcludedFromBuild_Sources.BaseDirectory, module.Configuration, flags: excluded_sources_flag);
				}

				foreach (var fileitem in module.ExcludedFromBuild_Files.FileItems)
				{
					uint flags = (fileitem.IsKindOf(FileData.Header | FileData.Resource) || module.IsKindOf(Module.MakeStyle)) ? 0 : FileEntry.ExcludedFromBuild;

					UpdateFileEntry(module, files, fileitem.Path, fileitem, module.ExcludedFromBuild_Files.BaseDirectory, module.Configuration, flags: flags);
				}

				foreach (var fileitem in module.Assets.FileItems)
				{
					UpdateFileEntry(module, files, fileitem.Path, fileitem, module.Assets.BaseDirectory, module.Configuration, flags: FileEntry.Assets);
				}

				var native = module as Module_Native;
				if (native != null && native.ResourceFiles != null)
				{
					foreach (var fileitem in native.ResourceFiles.FileItems)
					{
						uint flags = FileEntry.Resources;

						UpdateFileEntry(module, files, fileitem.Path, fileitem, fileitem.BaseDirectory ?? native.ResourceFiles.BaseDirectory, module.Configuration, flags: flags);
					}
				}
			}

			if (IncludeBuildScripts)
			{
				foreach (var module in Modules)
				{
					if (module.Package.Name != module.Package.Project.Properties["package.name"] || module.Configuration.Name != module.Package.Project.Properties["config"])
					{
						// Dynamically created packages may use existing projects from different packages. Skip such projects
						continue;
					}

					FileSet buildScripts = new FileSet
					{
						Sort = true
					};
					{
						// determine scripts base dir - this is the base directory scripts should be located under to be included in the project. Can overridden by user property, 
						// if not, use the the location of the module's script file and then finally the package dir. Included scripts will show up in project relative to this dir
						string scriptsBaseDir = null;
						string scriptBaseDirPropertyValue = module.PropGroupValue("buildscript.basedir");
						if (!String.IsNullOrEmpty(scriptBaseDirPropertyValue))
						{
							scriptsBaseDir = PathNormalizer.Normalize(scriptBaseDirPropertyValue);
						}
						else if (!module.ScriptFile.IsNullOrEmpty())
						{
							scriptsBaseDir = PathNormalizer.Normalize(Path.GetDirectoryName(module.ScriptFile.Path));
						}
						else
						{
							scriptsBaseDir = Package.Dir.NormalizedPath;
						}
						buildScripts.BaseDirectory = scriptsBaseDir;

						// determine script file - use build script file if set, otherwise use package's .build file
						string moduleScriptFile = null;
						if (!module.ScriptFile.IsNullOrEmpty())
						{
							moduleScriptFile = module.ScriptFile.Path; // (ScriptFile set by structured module definition normally)
						}
						else
						{
							moduleScriptFile = module.Package.Project.BuildFileLocalName;
						}

						// if module has a script file se and it's in the specified base dir,
						// add it to the set of scripts
						if (PathUtil.IsPathInDirectory(moduleScriptFile, scriptsBaseDir))
						{
							buildScripts.Include(moduleScriptFile, baseDir: scriptsBaseDir, failonmissing: false);
						}

						// add the /scripts folder from the package - but only if the base dir contains / is the scripts dir
						string packageScriptsDir = Path.Combine(Package.Dir.Path, "scripts");
						if (PathUtil.IsPathInDirectory(packageScriptsDir, scriptsBaseDir))
						{
							buildScripts.Include(packageScriptsDir + "/**", failonmissing: false);
						}

						// finally add any files included from the script file that are also inside the base dir
						foreach (string include in module.Package.Project.IncludedFiles.GetIncludedFiles(moduleScriptFile, recursive: true))
						{
							if (PathUtil.IsPathInDirectory(include, scriptsBaseDir))
							{
								buildScripts.Include(include, failonmissing: false);
							}
						}
					}

					foreach (FileItem fileitem in buildScripts.FileItems)
					{
						UpdateFileEntry(module, files, fileitem.Path, fileitem, buildScripts.BaseDirectory, module.Configuration, flags: FileEntry.NonBuildable);
					}
				}
			}

			return files;
		}

		private void UpdateFiles(IModule module, Dictionary<PathString, FileEntry> files, Tool tool, Configuration config, Func<Tool, IEnumerable<Tuple<FileSet, uint, Tool>>> func)
		{
			foreach (var set in func(tool))
			{
				foreach (var fileitem in set.Item1.FileItems)
				{
					if (!UpdateFileEntry(module, files, fileitem.Path, fileitem, set.Item1.BaseDirectory, config, tool: set.Item3, flags: set.Item2))
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

		protected bool UpdateFileEntry(IModule module, IDictionary<PathString, FileEntry> files, PathString key, FileItem fileitem, string basedirectory, Configuration config, Tool tool = null, uint flags = 0)
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

			if (tool is Modules.Tools.BuildTool)
			{
				flags |= FileEntry.CustomBuildTool;
			}

			// Hidden heavy operation in this one :-)
			var keyNormalizedPath = key.NormalizedPath;

			// find all matching copy local references that match this entry
			IEnumerable<CopyLocalInfo> copyLocalInfos = module.CopyLocalFiles.Where(cpi => cpi.AbsoluteSourcePath == keyNormalizedPath);

			return entry.TryAddConfigEntry((module.Package == null ? (Project)null : module.Package.Project), fileitem, config, tool, copyLocalInfos, flags);
		}

		protected bool AddResourceWithTool(IModule module, PathString path, string optionsetname, string visualStudioTool, bool excludedFromBuildRespected=true, string basedir = null, IDictionary<string, string> toolProperties = null)
		{
			return AddResourceWithTool(module, path.Path, optionsetname, visualStudioTool, excludedFromBuildRespected, basedir, toolProperties);
		}

		protected bool AddResourceWithTool(IModule module, string path, string optionsetname, string visualStudioTool, bool excludedFromBuildRespected=true, string basedir = null, IDictionary<string,string> toolProperties = null)
		{
			FileSet resourcefiles = null;

			var native_module = module as Module_Native;

			if (native_module != null)
			{
				if (native_module.ResourceFiles == null)
				{
					native_module.ResourceFiles = new FileSet();
				}
				resourcefiles = native_module.ResourceFiles;
			}
			else
			{
				var dotnet_module = module as Module_DotNet;

				if (dotnet_module != null)
				{
					resourcefiles = dotnet_module.Compiler.Resources;
				}
			}

			if (resourcefiles != null && module.Package.Project != null)
			{
				var optionset = new OptionSet();

				optionset.Options["visual-studio-tool"] = visualStudioTool;
				if (toolProperties != null)
				{
					optionset.Options["visual-studio-tool-properties"] = toolProperties.ToString(Environment.NewLine, e=>String.Format("{0}={1}", e.Key, e.Value));
				}
				if (!excludedFromBuildRespected)
				{
					optionset.Options["visual-studio-tool-excludedfrombuild-respected"] = "false";
				}

				module.Package.Project.NamedOptionSets.TryAdd(optionsetname, optionset);
				resourcefiles.Include(path, optionSetName: optionsetname, baseDir: basedir??Path.GetDirectoryName(path));
				return true;
			}
			return false;
		}


		protected string GetFileEntryOptionOrDefault(FileEntry fe, string optionName, string defaultVal)
		{
			
			var optionset = GetFileEntryOptionset(fe);
			if(optionset != null)
			{
				return optionset.Options[optionName] ?? defaultVal;
			}
			return defaultVal;
		}

		protected OptionSet GetFileEntryOptionset(FileEntry fe)
		{
			OptionSet options = null;
			if (fe != null)
			{
				var firstFe = fe.ConfigEntries.FirstOrDefault();
				if (firstFe != null)
				{
					if (!String.IsNullOrEmpty(firstFe.FileItem.OptionSetName) && firstFe.Project != null)
					{
						if(!firstFe.Project.NamedOptionSets.TryGetValue(firstFe.FileItem.OptionSetName, out options))
						{
							options = null;
						}
					}
				}
			}
			return options;

		}

		protected string GetCommandScriptWithCreateDirectories(Command command)
		{
			var commandstring = new StringBuilder();

			if (command.CreateDirectories != null && command.CreateDirectories.Count() > 0)
			{

				string format = "@if not exist \"{0}\" mkdir \"{0}\" & SET ERRORLEVEL=0";

				switch (Environment.OSVersion.Platform)
				{
					case PlatformID.MacOSX:
					case PlatformID.Unix:
						format = "mkdir -p \"{0}\"";
						break;
				}

				foreach (var createdir in command.CreateDirectories)
				{
					commandstring.AppendFormat(format, GetProjectPath(createdir.Normalize(PathString.PathParam.NormalizeOnly)));
					commandstring.AppendLine();
				}
			}

			if (BuildGenerator.IsPortable)
			{
				commandstring.AppendLine(BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(command.CommandLine, OutputDir.Path));
			}
			else
			{
				commandstring.AppendLine(command.CommandLine);
			}
			return commandstring.ToString();
		}

		protected void WriteElementStringWithConfigCondition(IXmlWriter writer, string name, string value, string condition = null)
		{

			if (!String.IsNullOrEmpty(condition))
			{
				writer.WriteStartElement(name);
				writer.WriteAttributeString("Condition", condition);
				writer.WriteString(value);
				writer.WriteEndElement();
			}
			else
			{
				writer.WriteElementString(name, value);
			}
		}

		protected void WriteElementStringWithConfigConditionNonEmpty(IXmlWriter writer, string name, string value, string condition = null)
		{
			if (!String.IsNullOrWhiteSpace(value))
			{
				WriteElementStringWithConfigCondition(writer, name, value, condition);
			}
		}

		internal readonly IDictionary<Configuration, string> ProjectConfigurationNameOverrides;

		#region Project Type Guid definitions
		internal static readonly Guid VCPROJ_GUID = new Guid("8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942");
		internal static readonly Guid VDPROJ_GUID = new Guid("54435603-DBB4-11D2-8724-00A0C9A8B90C");
		internal static readonly Guid CSPROJ_GUID = new Guid("FAE04EC0-301F-11D3-BF4B-00C04F79EFBC");
		internal static readonly Guid FSPROJ_GUID = new Guid("F2A71F9B-5D33-465A-A702-920D77279786");
		internal static readonly Guid WEBAPP_GUID = new Guid("349C5851-65DF-11DA-9384-00065B846F21");
		internal static readonly Guid PYPROJ_GUID = new Guid("888888A0-9F3D-457C-B088-3A5042F75D52");
		internal static readonly Guid ANDROID_PACKAGING_GUID = new Guid("39E2626F-3545-4960-A6E8-258AD8476CE5");
        internal static readonly Guid WIXPROJ_GUID = new Guid("930C7802-8A8C-48F9-8165-68863BCCD9DD");
		//Subtypes
		internal static readonly Guid TEST_GUID = new Guid("3AC096D0-A1C2-E12C-1390-A8335801FDAB");
		internal static readonly Guid CSPROJ__WORKFLOW_GUID = new Guid("14822709-B5A1-4724-98CA-57A101D1B079");
		#endregion Project Type Guid definitions

		internal string GetConfigCondition(Configuration config)
		{
			return " '$(Configuration)|$(Platform)' == '" + GetVSProjConfigurationName(config) + "' ";
		}

		internal string GetConfigCondition(IEnumerable<Configuration> configs)
		{
			MSBuildConfigConditions.Condition condition = MSBuildConfigConditions.GetCondition(configs, AllConfigurations);
			return MSBuildConfigConditions.FormatCondition("$(Configuration)|$(Platform)", condition, GetVSProjConfigurationName);
		}

		internal string GetConfigCondition(IEnumerable<FileEntry> files)
		{
			var fe = files.FirstOrDefault();
			if (fe != null)
			{
				return GetConfigCondition(fe.ConfigEntries.Select(ce => ce.Configuration));
			}
			return String.Empty;
		}

		internal string GetConfigCondition(IEnumerable<ProjectRefEntry> projectRefs)
		{
			var pre = projectRefs.FirstOrDefault();
			if (pre != null)
			{
				return GetConfigCondition(pre.ConfigEntries.Select(ce => ce.Configuration));
			}
			return String.Empty;
		}

		protected virtual void WriteCopyLocalFiles(IXmlWriter writer, bool isNativeBuild)
		{
			MSBuildConfigItemGroup copyLocalCustomFilesGroup = new MSBuildConfigItemGroup(AllConfigurations, "Copy Local Files");
			MSBuildConfigItemGroup fastUpToDateTriggers = new MSBuildConfigItemGroup(AllConfigurations, "Copy Local Fast Up To Date Triggers");
			MSBuildConfigItemGroup timestampUpdateForUpToDateCheck = new MSBuildConfigItemGroup(AllConfigurations, "Timestamp Update For Up To Date Check");

			foreach (IModule module in Modules)
			{
				IEnumerable<CopyLocalInfo> copyLocalInfos = module.CopyLocalFiles.Except(HandledCopyLocalOperations);

				// when doing post build copy local module can copy a single file to multiple destinations
				// visual studio doesn't like mutliple itemgroup definitions for the same path with different metadata so
				// we have to group by source path, this means we must condense metadata options also, for example if
				// one destination cannot be hard linked then we hard link none
				IEnumerable<IGrouping<string, CopyLocalInfo>> copyLocalInfosBySource = copyLocalInfos.GroupBy(cpi => cpi.AbsoluteSourcePath).OrderBy(grp => grp.Key);
				foreach (IGrouping<string, CopyLocalInfo> copyLocalInfoGroup in copyLocalInfosBySource)
				{
					string absoluteSourcePath = copyLocalInfoGroup.Key;

					IEnumerable<string> absolutePreBuildDestPaths = copyLocalInfoGroup.Where(cpi => !cpi.PostBuildCopyLocal).Select(cpi => cpi.AbsoluteDestPath).OrderBy(s => s);
					if (absolutePreBuildDestPaths.Count() > 1 && module.Package.Project != null)
					{
						module.Package.Project.Log.ThrowWarning(Log.WarningId.DuplicateCopyLocalDestination, Log.WarnLevel.Normal, "Module {0} is copying file '{1}' to multiple locations in it's output directory: '{2}'. This may be an error in module declaration.", module.Name, Path.GetFileName(absoluteSourcePath), String.Join(", ", absolutePreBuildDestPaths));
					}

					// add the actual copy entry
					Dictionary<string, string> copyMetaData = GetCopyLocalFilesMetaData(absoluteSourcePath, copyLocalInfoGroup);
					// don't clutter the solution explorer
					copyMetaData["Visible"] = "false";
					bool usingHardLink = false;
					if (copyMetaData.TryGetValue("UseHardLink", out string useHardLink))
					{
						usingHardLink = useHardLink.ToLowerInvariant() == "true";
					}
					if (!usingHardLink)
					{
						// If hard link is not explicitly specified, we explicitly specify it to false and then have "UpToDateCheckOutput"
						// conditionally being added only when hard link is not used.  We need to change the copied file's timestamp if
						// we are to use Visual Studio's fast up to date check mechanism using UpToDateCheckOutput.

						// IMPORTANT NOTE: Allowing hard link can lead to risk that some projects listed the copy using hard link
						// while another project listed to not use hard link (for the same source file and destination).  Currently Framework
						// don't have mechanism to test for consistency between different vcxproj in the same solution.
						copyMetaData["UseHardLink"] = "false";
						copyMetaData["PreserveTimestamp"] = "false";
					}
					copyLocalCustomFilesGroup.AddItem(module.Configuration, "FrameworkCopyLocal", GetProjectPath(absoluteSourcePath), copyMetaData);

					// add a fast-up-to-check entry for the input
					{
						Dictionary<string, string> metaData = new Dictionary<string, string>() { { "Visible", "false" } };
						/*if (absoluteSourcePath.ToLowerInvariant().EndsWith(".obj")) // TODO: if FB works without this then we can remove it
						{
							// If a file is marked for copylocal, it probably shouldn't be used for linking even though it has a .obj ending.
							// Frostbite has some "contentfiles" with .obj ending but they are not really C++ object files.
							// NOTE: "CustomBuild" will also trigger link with .lib, .res, and .rsc.  Since those files really shouldn't be
							//       copylocal, so we're not testing for those and let the build fail to flag user of possible issue.
							metaData.Add("LinkObjects", "false");
						}*/
						fastUpToDateTriggers.AddItem(module.Configuration, "UpToDateCheckInput", GetProjectPath(absoluteSourcePath), metaData);
					}

					// add fast-up-to-date-check entry for outputs
					if (!usingHardLink)
					{
						foreach (string absDestPath in absolutePreBuildDestPaths)
						{
							// add output as if they were inputs - we do this so if copy output is missing fast up-to-check will trigger  
							Dictionary<string, string> metaData = new Dictionary<string, string>() { { "Visible", "false" } };
							/*if (absDestPath.ToLowerInvariant().EndsWith(".obj")) // TODO: if FB works without this then we can remove it
							{
								// If a file is marked for copylocal, it probably shouldn't be used for linking even though it has a .obj ending.
								// Frostbite has some "contentfiles" with .obj ending but they are not really C++ object files.
								// NOTE: "CustomBuild" will also trigger link with .lib, .res, and .rsc.  Since those files really shouldn't be
								//       copylocal, so we're not testing for those and let the build fail to flag user of possible issue.
								metaData.Add("LinkObjects", "false");
							}*/
							fastUpToDateTriggers.AddItem(module.Configuration, "UpToDateCheckOutput", GetProjectPath(absDestPath), metaData);

							// All files listed in this UpToDateCheckOutput need to have timestamp updated to make sure that their timestamps are
							// newer than the module build output's timestamp. Otherwise, Visual Studio's "project level" up to date check won't 
							// function properly and get into a situation where the "UpToDateCheckOutput" files are older than the input files. 
							// Visual Studio added intermediate temp .cs files as input file to check which has similar timestamp as the build output.
							// So those "temp" input files has newer timestamp ask the copied local files.  So we add a task after build to 
							// re-touch these copied local files.
							Dictionary<string, string> timestampUpdateMetaData = new Dictionary<string, string>();
							timestampUpdateMetaData.Add("BaseTimeFile", module.PrimaryOutput());
							timestampUpdateMetaData.Add("UpdateFile", absDestPath);
							timestampUpdateMetaData.Add("Visible", "false");
							// Even though absDestPath is the file we want to update, make sure we set up the FrameworkTimesstampUpdate item 
							// using "absoluteSourcePath" instead of "absDestPath".  Otherwise, Visual Studio will think absDestPath is 
							// an input file and check the timestamp of that file as project input dependency (and we end up output always older
							// than input).
							timestampUpdateForUpToDateCheck.AddItem(module.Configuration, "FrameworkTimestampUpdate", GetProjectPath(absoluteSourcePath), timestampUpdateMetaData);
						}
					}
				}
			}
			copyLocalCustomFilesGroup.WriteTo(writer, c => GetVSProjConfigurationName(c), s => GetProjectPath(s));
			timestampUpdateForUpToDateCheck.WriteTo(writer, c => GetVSProjConfigurationName(c), s => GetProjectPath(s));
			fastUpToDateTriggers.WriteTo(writer, c => GetVSProjConfigurationName(c), s => GetProjectPath(s));
		}

		protected Dictionary<string, string> GetCopyLocalFilesMetaData(string absoluteSourcePath, IEnumerable<CopyLocalInfo> copyLocalInfos)
		{
			IEnumerable<string> absolutePreBuildDestPaths = copyLocalInfos.Where(cpi => !cpi.PostBuildCopyLocal).Select(cpi => cpi.AbsoluteDestPath).OrderBy(s => s);
			bool hasPrebuild = absolutePreBuildDestPaths.Any();
			IEnumerable<string> absolutePostBuildDestPaths = copyLocalInfos.Where(cpi => cpi.PostBuildCopyLocal).Select(cpi => cpi.AbsoluteDestPath).OrderBy(s => s);
			bool hasPostBuild = absolutePostBuildDestPaths.Any();   

			Dictionary<string, string> copyMetaData = new Dictionary<string, string>();

			// hard link
			bool useHardLinkIfPossible = copyLocalInfos.All
			(
				cpi => 
					(cpi.UseHardLink) &&	// module being copied to, specifies copy local should be hard links
					(cpi.AllowLinking)	// this file can be a hard link
			);
			bool useHardLink = useHardLinkIfPossible && copyLocalInfos.All(cpi => PathUtil.TestAllowCopyWithHardlinkUsage(cpi.DestinationModule.Package.Project, absoluteSourcePath, cpi.AbsoluteDestPath));
			copyMetaData.Add("UseHardLink", useHardLink.ToString().ToLowerInvariant());

			// destinations to copy dependencies to before build
			string formattedPreBuildDestinationPaths = String.Join(";", absolutePreBuildDestPaths.Select(path => GetProjectPath(path))); // use semicolon delimited list (msbuild string respresentation of itemgroup)
			if (hasPrebuild)
			{
				copyMetaData.Add("PrebuildDestinations", formattedPreBuildDestinationPaths);
			}

			// desintation to copy the output of this module to after is has built	
			string formattedPostBuildDestinationPaths = String.Join(";", absolutePostBuildDestPaths.Select(path => GetProjectPath(path))); // use semicolon delimited list (msbuild string respresentation of itemgroup)						
			if (hasPostBuild)
			{
				copyMetaData.Add("PostBuildDestinations", formattedPostBuildDestinationPaths);
			}

			// non critical - unless true a failed copy will fail the build
			bool nonCritical = copyLocalInfos.All(cpi => cpi.NonCritical); // only if all are not critical do we set this
			if (nonCritical) // this defaults to false, only write if true to reduce clutter
			{
				copyMetaData.Add("NonCritical", "true");
			}

			// skip unchanged - if true will not copy if destination file is newer
			bool skipUnchanged = copyLocalInfos.All(cpi => cpi.SkipUnchanged);
			if (!skipUnchanged) // this defaults to true, only write if false to reduce clutter
			{
				copyMetaData.Add("SkipUnchanged", "false");
			}
			copyMetaData.Add("Private", "true");
			return copyMetaData;
		}
	}
}
