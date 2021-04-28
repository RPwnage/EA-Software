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
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.NuGet;

using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
	public abstract partial class VSDotNetProject : VSProjectBase
	{
		internal VSDotNetProject(VSSolutionBase solution, IEnumerable<IModule> modules, Guid projectTypeGuid)
			: base(solution, modules, projectTypeGuid)
		{
			// Get project configuration that equals solution startup config:
			if (ConfigurationMap.TryGetValue(solution.StartupConfiguration, out Configuration startupconfig))
			{
				StartupModule = Modules.Single(m => m.Configuration == startupconfig) as Module_DotNet;
			}
			else
			{
				StartupModule = Modules.First() as Module_DotNet;
			}

			ConfigPlatformMap = new Dictionary<Configuration, string>();

			foreach (var module in modules)
			{
				ConfigPlatformMap.Add(module.Configuration, GetConfigPlatformMapping(module as Module_DotNet));
			}

			AnyCpuConfigurations = Enumerable.Empty<Configuration>();
			foreach (var group in ConfigPlatformMap.GroupBy(e => e.Value))
			{
				if (group.Key == "AnyCPU")
					AnyCpuConfigurations = group.Select(e => e.Key).ToList();
			}
		}

		protected readonly Module_DotNet StartupModule;
		protected readonly Dictionary<Configuration, string> ConfigPlatformMap;
		protected readonly IEnumerable<Configuration> AnyCpuConfigurations;

		#region Abstract  Methods

		protected abstract string ToolsVersion { get; }

		protected abstract string DefaultTargetFrameworkVersion { get; }

		protected abstract string ProductVersion { get; }

		#endregion Abstract Methods

		#region Visual Studio version info

		protected virtual string Xmlns
		{
			get { return "http://schemas.microsoft.com/developer/msbuild/2003"; }
		}

		protected virtual string SchemaVersion
		{
			get { return "2.0"; }
		}

		#endregion Visual Studio version info

		// backwards compatibility, IDotNetVisualStudioExtension used to have a layer of indirection,
		// x.Generator.Interace where Generator was a wrapper class for VSDotNetProject and Interface
		// was the actual project. This indirection has been remove so that Generator and Interface are
		// the same thing, but to keep the old API Interface member is kept but just refers to the same
		// object as generator
		public VSDotNetProject Interface => this; 

		protected virtual string DefaultTarget
		{
			get { return "Build"; }
		}

		protected virtual string DefaultTargetSchema
		{
			get { return "IE50"; }
		}

		protected virtual string StartupConfiguration
		{
			get { return StartupModule.Configuration.Name; }
		}

		protected virtual string StartupPlatform
		{
			get { return GetProjectTargetPlatform(StartupModule.Configuration); }
		}

		protected virtual string TargetFrameworkVersion
		{
			get
			{
				var frameworkversions = Modules.Cast<Module_DotNet>().Select(m => m.TargetFrameworkVersion).Where(tv => !String.IsNullOrEmpty(tv)).OrderedDistinct();

				if (frameworkversions.Count() > 1)
				{
					throw new BuildException(String.Format("DotNet Module '{0}' has different TargetFramework versions ({1}) defined in different configurations. VisualStudio does not support this feature.", ModuleName, frameworkversions.ToString(", ", t => t.ToString())));
				}

				var targetversion = frameworkversions.FirstOrDefault();

				if (String.IsNullOrEmpty(targetversion))
				{
					targetversion = DefaultTargetFrameworkVersion;
				}

				if (Modules.First().Package.Project.UseNewCsProjFormat && targetversion.StartsWith("net"))
				{
					return targetversion;
				}
				if (!targetversion.StartsWith("v") && targetversion.Length >= 3)
				{
					targetversion = "v" + targetversion.Substring(0, 3);
				}

				return targetversion;
			}
		}

		protected virtual DotNetFrameworkFamilies TargetFrameworkFamily
		{
			get
			{
				return StartupModule.TargetFrameworkFamily;
			}
		}

		protected virtual ICollection<Guid> ProjectTypeGuids
		{
			get
			{
				var guids = new List<Guid>();

				if (StartupModule.IsProjectType(DotNetProjectTypes.UnitTest))
				{
					guids.Add(TEST_GUID);
				}
				else if (StartupModule.IsProjectType(DotNetProjectTypes.WebApp))
				{
					guids.Add(WEBAPP_GUID);
				}
				else if (StartupModule.IsProjectType(DotNetProjectTypes.Workflow))
				{
					guids.Add(CSPROJ__WORKFLOW_GUID);
				}
				return guids;
			}
		}

		internal override string RootNamespace { get => StartupModule.RootNamespace ?? StartupModule.Compiler.OutputName.Replace('-', '_'); }

		#region Implementation of Inherited Abstract Methods

		protected override PathString GetDefaultOutputDir()
		{
			var module = Modules.First() as Module_DotNet;
			if (Package.Project.Properties.GetBooleanPropertyOrDefault("package." + Package.Name + ".designermode", false))
			{
				var customProjectDir = module.Compiler.Resources.BaseDirectory;
				if (String.IsNullOrEmpty(customProjectDir))
				{
					customProjectDir = module.Compiler.SourceFiles.BaseDirectory;
				}
				if (!String.IsNullOrEmpty(customProjectDir))
				{
					return PathString.MakeNormalized(customProjectDir);
				}
				else
				{
					Log.Warning.WriteLine("{0}/{1}.{2}, designermode is 'ON', but both SourceFiles.BaseDirectory and Resources.BaseDirectory are empty, 'csproj' file will be generated in the build folder.", module.Package.Name, module.BuildGroup, module.Name);
				}
			}

			PathString defaultPathString = BuildGenerator.GenerateSingleConfig ? Package.PackageConfigBuildDir : Package.PackageBuildDir;
			return PathString.MakeNormalized(Path.Combine(defaultPathString.Path, Name));
		}

		protected override string UserFileName
		{
			get { return ProjectFileName + ".user"; }
		}

		public override string GetProjectTargetPlatform(Configuration configuration)
		{
			string platform;
			if (!ConfigPlatformMap.TryGetValue(configuration, out platform))
			{
				platform = "AnyCPU";
			}
			return platform;
		}

		public string GetProjectTargetConfigurationList()
		{
			return string.Join(";", ConfigPlatformMap.Keys.Select(x => GetVSProjTargetConfiguration(x)).Distinct());
		}

		public string GetProjectTargetPlatformList()
		{
			return string.Join(";", ConfigPlatformMap.Values.Distinct());
		}

		partial void WriteSnowCache(IXmlWriter writer);

		protected override void WriteProject(IXmlWriter writer)
		{
			if (UseNewCSProjFormat())
			{
				writer.WriteStartElement("Project");
				// a typical new style .csproj would have this format:
				/*
					<Project Sdk="Microsoft.NET.Sdk">
				*/
				// but we need to unroll it (https://docs.microsoft.com/en-us/dotnet/core/project-sdk/overview) because we need to set some properties
				// before the default sdk imports
				writer.WriteStartElement("PropertyGroup");
				foreach (IModule module in Modules)
				{
					writer.WriteStartElement("BaseIntermediateOutputPath");
					writer.WriteAttributeString("Condition", $" '$(Configuration)' == '{GetVSProjTargetConfiguration(module.Configuration)}' ");
					writer.WriteString(GetProjectPath(module.GetVSTmpFolder().Path, addDot: true));
					writer.WriteEndElement();
				}
				writer.WriteEndElement();

				WriteUsingTaskOverrides(writer);

				writer.ScopedElement("Import").Attribute("Project", "Sdk.props").Attribute("Sdk", GetNewStyleSdk()).Dispose();
			}
			else
			{
				writer.WriteStartElement("Project", Xmlns);
				writer.WriteAttributeString("ToolsVersion", ToolsVersion);

				WriteUsingTaskOverrides(writer);
			}

			WriteNugetProps(writer); // nuget spec mandates that these target go at the start of the file so do this here before WriteImportProperties

			WriteProjectProperties(writer);

			WriteConfigurations(writer);

			WriteImportProperties(writer);
			WriteImportTargets(writer); // Need to have it before build steps, otherwise some properties like $(TargetPath) may not be defined when steps are evaluated.

			WriteFrameworkCustomCSharpTargetDependency(writer); // insert framework build targets into build process

			WriteSnowCache(writer);

			WriteBuildSteps(writer);

			WriteReferences(writer);

			WriteFiles(writer);
			WriteNugetContent(writer);

			WriteCopyLocalFiles(writer, isNewProjectSystem: UseNewCSProjFormat(), isCsProj: true);  // copy local files write must come after other file writes, other file writes can mark certain copy local operations
																// as not requiring custom copy local entries

			// Invoke Visual Studio extensions
			foreach (IModule module in Modules)
			{
				ExecuteExtensions(module, extension => extension.WriteExtensionItems(writer));
			}

			WriteWebConfigCopyStep(writer);

			WriteBootstrapperPackageInfo(writer);

			WriteSdkTargetsImport(writer);
			WriteNugetTargets(writer); // nuget spec mandates that these target go at the end of the file so do this here rather than in WriteImportTargets
			WriteMsBuildTargetOverrides(writer);

			writer.WriteEndElement(); //Project
		}

		private string GetNewStyleSdk()
		{
			/* 
			note about *version* targeting in the new SDK format
			there are 3 ways:
				1. Add a Version="" attribute to Sdk="" style Project or Import statement
					- This does not work because earlier Imports in MSBuild can trigger before which don't set version
					  and build uses a caching reoslution which will stomp later imports with the cached version
				2. Add a global.json next to project file (or higher in folder struct)
					- Beats method 1. because it affects all sdk resolves in the project but (also like above) only allows 
					us to set *version* not *location*
				3. set DOTNET_MSBUILD_SDK_RESOLVER_SDKS_DIR and DOTNET_MSBUILD_SDK_RESOLVER_SDKS_VER environment variables
					- less than ideal because we can't be sure sure user has launched vs/mbuild with this is environment set
					(if launched via nant or fbcli for frostbite then we ensure this enviroment) HOWEVER this is the only method
					that gives us location control as far as we know which is important for non-proxy workflows so
					it's the one we've gone for now - note this is all set up external to the csproj (see fbcli / dotnetsdk package)
			*/

			if (TargetFrameworkFamily != DotNetFrameworkFamilies.Framework)
			{
				return "Microsoft.NET.Sdk";
			}

			// if targeting .net framework we need set the sdk to windows desktop for wpf to work
			// without it the DependentUpon in blocks like the one below doesn't show up in the solution
			// explorer or undergo .xaml => .cs generation
			/*
			 <Compile Include="F:\dev\na-cm\TnT\LocalPackages\BloxWpfToolkit\dev\external\WpfToolkit\source\CollectionControl\Implementation\CollectionControlDialog.xaml.cs">
				<SubType>Code</SubType>
				<DependentUpon>CollectionControlDialog.xaml</DependentUpon>
				<Link>CollectionControl\Implementation\CollectionControlDialog.xaml.cs</Link>
			</Compile>
			*/
			bool useWPF = StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".usewpf", false);
			bool useWindowsForms = StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".usewindowsforms", false);
			string sdk = (useWPF || useWindowsForms) ?
				"Microsoft.NET.Sdk.WindowsDesktop" :
				"Microsoft.NET.Sdk";
			return sdk;
		}

		protected bool UseNewCSProjFormat() => StartupModule.Package.Project.UseNewCsProjFormat;

		// note if this is true UseNewCSProjFormat() check is also implicitly true
		protected bool IsTargetingNetCoreStandard() => StartupModule.TargetFrameworkFamily == DotNetFrameworkFamilies.Core || StartupModule.TargetFrameworkFamily == DotNetFrameworkFamilies.Standard; 

		private void WriteNugetContent(IXmlWriter writer)
		{
			MSBuildConfigItemGroup nugetContent = new MSBuildConfigItemGroup(Modules.Select(mod => mod.Configuration).ToList(), "Nuget Content");
			foreach (IModule module in Modules)
			{
				foreach (string dependentNugetPackage in module.Dependents.Select(dependency => dependency.Dependent.Package.Name).Distinct())
				{
					FileSet nugetFileSet = module.Package.Project.NamedFileSets[String.Format("package.{0}.{1}", dependentNugetPackage, "nuget-content-files")];
					if (nugetFileSet != null)
					{
						foreach (FileItem item in nugetFileSet.FileItems)
						{
							string fullPath = item.Path.Path;
							string linkPath = PathNormalizer.MakeRelative(item.Path.Path, item.BaseDirectory ?? nugetFileSet.BaseDirectory ?? String.Empty);

							IEnumerable<CopyLocalInfo> copyLocalOperations = module.CopyLocalFiles.Where(cpi => cpi.AbsoluteSourcePath == fullPath).ToList(); // ToList() is important here for performance
							Dictionary<string, string> copyMetaData = null;
							if (copyLocalOperations.Any())
							{
								copyMetaData = GetCopyLocalFilesMetaData(fullPath, copyLocalOperations);
							}
							else
							{
								copyMetaData = new Dictionary<string, string>();
							}
							copyMetaData.Add("Link", linkPath);
							nugetContent.AddItem(module.Configuration, "Content", fullPath, copyMetaData);

							HandledCopyLocalOperations.AddRange(copyLocalOperations);
						}
					}
				}
			}
			nugetContent.WriteTo(writer, c => GetVSProjConfigurationName(c), s => GetProjectPath(s));
		}

		private void WriteNugetProps(IXmlWriter writer)
		{
			if (StartupModule.Package.Project.UseNugetResolution)
			{
				bool usePackageReferences = StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_USE_VS_PACKAGE_REFERENCES, false);
				if (usePackageReferences)
				{
					// delete any files we may have geneated from non-reference
					// paths, they are picked up by pattern and will cause issues
					foreach (IModule module in Modules)
					{
						DeleteNugetImportFile(module, ".props");
					}
				}
				else
				{
					foreach (IModule module in Modules)
					{
						List<string> distinctImports = new List<string>();
						// NUGET-TODO clean up refactor - either convenience for refs or go straight to nuget contents
						List<string> nugetReferences = null;
						if (module is Module_DotNet moduleDotNet)
						{
							nugetReferences = moduleDotNet.NuGetReferences;
						}
						else if (module is Module_Native moduleNative && moduleNative.IsKindOf(Module.Managed))
						{
							nugetReferences = moduleNative.NuGetReferences;
						}
						if (nugetReferences != null)
						{
							NugetPackage[] flattenedContents;
							try
							{
								flattenedContents = PackageMap.Instance.GetNugetPackages(module.Package.Project.BuildGraph().GetTargetNetVersion(module.Package.Project), nugetReferences).ToArray();
							}
							catch (NugetException nugetEx)
							{
								throw new BuildException($"Error while resolving NuGet dependencies of module '{module.Name}'.", nugetEx);
							}
							foreach (NugetPackage packageContents in flattenedContents)
							{
								distinctImports.AddUnique(packageContents.PropItems.Select(item => item.FullName));
							}
							WriteNugetImportFile(module, distinctImports, ".props");
						}
					}
				}
			}
			else
			{
				List<FileSet> propFiles = new List<FileSet>();
				GatherNugetFilesets("nuget-prop-files", null, ref propFiles, false);

				IEnumerable<string> distinctImportPaths = propFiles
					.SelectMany(fs => fs.FileItems)
					.Select(fi => fi.Path.NormalizedPath.TrimQuotes())
					.Distinct();
				foreach (string import in distinctImportPaths)
				{
					writer.WriteStartElement("Import");
					writer.WriteAttributeString("Project", import);
					writer.WriteEndElement(); //Import
				}
			}
		}

		private void WriteNugetImportFile(IModule module, IEnumerable<string> importPaths, string fileExtension)
		{
			if (importPaths.Any())
			{
				using (IXmlWriter nugetPropsWriter = new NAnt.Core.Writers.XmlWriter(ProjectFileWriterFormat))
				{
					string frameworkNugetImports = GetProjectPath(Path.Combine(module.GetVSTmpFolder().Path, $"{ProjectFileName}.framework.g.{fileExtension.TrimStart('.')}"));

					nugetPropsWriter.CacheFlushed += new CachedWriterEventHandler(OnProjectFileUpdate);
					nugetPropsWriter.FileName = frameworkNugetImports;

					nugetPropsWriter.WriteStartDocument();
					nugetPropsWriter.WriteStartElement("Project", Xmlns);
					nugetPropsWriter.WriteAttributeString("ToolsVersion", ToolsVersion);
					{
						nugetPropsWriter.WriteStartElement("ImportGroup");
						{
							foreach (string import in importPaths)
							{
								nugetPropsWriter.WriteStartElement("Import");
								nugetPropsWriter.WriteAttributeString("Project", GetProjectPath(import));
								nugetPropsWriter.WriteEndElement(); //Import
							}
						}
						nugetPropsWriter.WriteEndElement();
					}
					nugetPropsWriter.WriteEndElement();
				}
			}
		}

		private void DeleteNugetImportFile(IModule module, string fileExtension)
		{
			string frameworkNugetImports = GetProjectPath(Path.Combine(module.GetVSTmpFolder().Path, $"{ProjectFileName}.framework.g.{fileExtension.TrimStart('.')}"));
			if (File.Exists(frameworkNugetImports))
			{
				File.Delete(frameworkNugetImports);
			}
		}

		private void WriteSdkTargetsImport(IXmlWriter writer)
		{
			if (UseNewCSProjFormat())
			{
				writer.ScopedElement("Import").Attribute("Project", "Sdk.targets").Attribute("Sdk", GetNewStyleSdk()).Dispose();
			}
		}

		private void WriteNugetTargets(IXmlWriter writer)
		{
			if (StartupModule.Package.Project.UseNugetResolution)
			{
				bool usePackageReferences = StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_USE_VS_PACKAGE_REFERENCES, false);
				if (usePackageReferences)
				{
					foreach (IModule module in Modules)
					{
						// delete any files we may have geneated from non-reference
						// paths, they are picked up by pattern and will cause issues
						DeleteNugetImportFile(module, ".targets");
					}
				}
				else
				{
					foreach (IModule module in Modules)
					{
						List<string> distinctImports = new List<string>();
						// NUGET-TODO clean up refactor - either convenience for refs or go straight to nuget contents
						List<string> nugetReferences = null;
						if (module is Module_DotNet moduleDotNet)
						{
							nugetReferences = moduleDotNet.NuGetReferences;
						}
						else if (module is Module_Native moduleNative && moduleNative.IsKindOf(Module.Managed))
						{
							nugetReferences = moduleNative.NuGetReferences;
						}
						if (nugetReferences != null)
						{
							NugetPackage[] flattenedContents;
							try
							{
								flattenedContents = PackageMap.Instance.GetNugetPackages(module.Package.Project.BuildGraph().GetTargetNetVersion(module.Package.Project), nugetReferences).ToArray();
							}
							catch (NugetException nugetEx)
							{
								throw new BuildException($"Error while resolving NuGet dependencies of module '{module.Name}'.", nugetEx);
							}
							foreach (NugetPackage packageContents in flattenedContents)
							{
								distinctImports.AddUnique(packageContents.TargetItems.Select(item => GetProjectPath(item.FullName)));
							}
						}
						WriteNugetImportFile(module, distinctImports, ".targets");
					}
				}
			}
			else
			{
				List<FileSet> targetFiles = new List<FileSet>();
				GatherNugetFilesets("nuget-target-files", null, ref targetFiles, false);

				IEnumerable<string> distinctImportPaths = targetFiles
					.SelectMany(fs => fs.FileItems)
					.Select(fi => fi.Path.NormalizedPath.TrimQuotes())
					.Distinct();
				foreach (string import in distinctImportPaths)
				{
					writer.WriteStartElement("Import");
					writer.WriteAttributeString("Project", import);
					writer.WriteEndElement(); //Import
				}
			}
		}

		private void WriteMsBuildTargetOverrides(IXmlWriter writer)
		{
			// NUGET-TODO: would be cleaner to actually redfine this tasks to do what we want rather than nop'ing here
			// and then tagging our own custom targets on with BeforeTargets / AfterTargets (see FrameworkMsbuild.tasks)
			if (IsTargetingNetCoreStandard())
			{
				// stomp the _CopyFilesMarkedCopyLocal target under .net core project, we need to mark
				// out references for copylocal in order to have VisualStudio write the entries to the deps file
				// but want to handle copy outselves
				using (writer.ScopedElement("Target").Attribute("Name", "_CopyFilesMarkedCopyLocal"))
				{ }
			}
		}

		public override void PostGenerate()
		{
			base.PostGenerate();
			ProcessNugetPostGenerate();
		}

		public override void AddExcludedDependency(IModule from, IModule to)
		{
			if (to is Module_DotNet)
			{
				var assembly = ((Module_DotNet)to).DynamicLibOrAssemblyOutput();
				((Module_DotNet)from).Compiler.DependentAssemblies.Include(assembly.Path);
			}
		}

		// gather all nuget scripts from a fileset pattern from a single dot net project generator
		// TODO: Remove the suppressionProperties array, reverting to a single property, for the next
		//		 major Framework release.
		//		 -mburke 2015-12-21
		internal void GatherNugetFilesets(string filesetName, string[] suppressionProperties, ref List<FileSet> nugetFiles, bool recursive)
		{
			IEnumerable<Module_DotNet> dotNetModules = Modules.Where(module => module is Module_DotNet).Cast<Module_DotNet>();
			foreach (Module_DotNet dotNetModule in dotNetModules)
			{
				foreach (Dependency<IModule> dependent in dotNetModule.Dependents)
				{
					if (dependent.Dependent is Module_UseDependency useModule)
					{
						GatherNugetFiles(dotNetModule.Package.Project, useModule.Package.Name, filesetName, suppressionProperties, ref nugetFiles, recursive);
					}
				}
			}
		}

		// gather all nuget scripts from a fileset pattern from a package and it's nuget dependencies (via recursion)
		private static void GatherNugetFiles(Project project, string packageName, string filesetName, string[] suppressionProperties, ref List<FileSet> nugetFiles, bool recursive)
		{
			// check if nuget scripts are suppressed for this package
			bool scriptsSuppressed = false;
			if (suppressionProperties != null)
			{
				foreach (string suppressionProperty in suppressionProperties)
				{
					if (project.Properties.GetBooleanPropertyOrDefault(String.Format("package.{0}.{1}", packageName, suppressionProperty), false))
					{
						scriptsSuppressed = true;
						break;
					}
				}
			}

			if (!scriptsSuppressed)
			{
				String key = "package." + packageName + "." + filesetName;
				FileSet nugetFileSet = project.NamedFileSets[key];
				if (nugetFileSet != null && !nugetFiles.Contains(nugetFileSet))
				{
					if (nugetFileSet.FileItems.Count > 0)
					{
						nugetFiles.Add(nugetFileSet);
					}
				}
			}

			// gather scripts from dependent nuget packages
			if (recursive)
			{
				string dependenciesProperty = project.Properties[String.Format("package.{0}.nuget-dependencies", packageName)] ?? String.Empty;
				foreach (string dependency in dependenciesProperty.ToArray()) // ToArray is framework extension that splits property strings on whitespace
				{
					GatherNugetFiles(project, dependency, filesetName, suppressionProperties, ref nugetFiles, recursive);
				}
			}
		}

		private static void GatherTransitiveNugetDependencies(Project project, string packageName, ref HashSet<string> transitiveNugetPackageDependencies)
		{
			transitiveNugetPackageDependencies.Add(packageName);
			string dependenciesProperty = project.Properties[String.Format("package.{0}.nuget-dependencies", packageName)] ?? String.Empty;
			foreach (string dependency in dependenciesProperty.ToArray())
			{
				GatherTransitiveNugetDependencies(project, dependency, ref transitiveNugetPackageDependencies);
			}
		}

		private void ProcessNugetPostGenerate()
		{
			// collect distinct nuget *install* scripts
			List<FileSet> nugetInstallFiles = new List<FileSet>();
			GatherNugetFilesets("nuget-install-scripts", new string[] { "supress-nuget-install", "suppress-nuget-install" }, ref nugetInstallFiles, true);
			string projectPath = Path.Combine(OutputDir.NormalizedPath, ProjectFileName);

			bool useVSForInstallContext = Modules.First().Package.Project.Properties.GetBooleanPropertyOrDefault("nuget-install-use-vs", false);
			if (useVSForInstallContext)
			{
				// this path has been deprecated due to dependencies on EnvDTE assembly being difficult to deal with and ensure that the dll is installed.
				throw new BuildException("The 'nuget-install-use-vs' property has been deprecated/removed. Please do not use this property anymore.  If you have issues with nuget package install and think you require the 'nuget-install-use-vs' property please contact the #frostbite-ew slack channel.");
			}

			// run install scripts
			IEnumerable<FileItem> scripts = nugetInstallFiles.SelectMany(fs => fs.FileItems);
			if (scripts.Any())
			{
				Log.Status.WriteLine("Executing {0} post-generate nuget install scripts for {1} ({2})", scripts.Count(), projectPath,
					String.Join(", ", nugetInstallFiles.SelectMany(kvp => kvp.FileItems.Select(item => item.FullPath)))
				);
				foreach (FileSet scriptSet in nugetInstallFiles)
				{
					foreach (FileItem script in scriptSet.FileItems)
					{
						// path tp init.ps1 script
						string scriptPath = script.Path.NormalizedPath;
						FileInfo fileInfo = new FileInfo(scriptPath);

						// parameters for init script
						string toolsPath = fileInfo.Directory.FullName;
						string installPath = fileInfo.Directory.Parent.FullName;

						// run script, NugetVSEnv is only valid during PostGenerate
						Log.Info.WriteLine("Executing {0}.", scriptPath);
						((VSSolutionBase)BuildGenerator).NugetVSEnv.CallInstallScript(scriptPath, installPath, toolsPath, null, projectPath);
					}
				}
			}
		}

		private void WriteFrameworkCustomCSharpTargetDependency(IXmlWriter writer)
		{
			writer.WriteStartElement("PropertyGroup");
			writer.WriteElementString("CompileDependsOn", "FrameworkCSharpCustomBuild;$(CompileDependsOn)");
			writer.WriteEndElement(); //PropertyGroup
		}

		protected override IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>> GetUserData(ProcessableModule module)
		{
			var userdata = new List<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>>();
			return userdata;
		}

		protected override void PopulateConfigurationNameOverrides()
		{
			foreach (var module in Modules)
			{
				if (module.Package.Project != null)
				{
					var configName = module.Package.Project.Properties[module.GroupName + ".vs-config-name"] ?? module.Package.Project.Properties["eaconfig.dotnet.vs-config-name"];
					if (!String.IsNullOrEmpty(configName))
					{
						ProjectConfigurationNameOverrides[module.Configuration] = configName;
					}
				}
			}
		}

		#endregion Implementation of Inherited Abstract Methods

		#region Set Properties Methods
		protected virtual IDictionary<string, string> GetConfigurationProperties(Module_DotNet module)
		{
			var data = new OrderedDictionary<string, string>();

			bool debug = module.Compiler.HasOption("debug") && !module.Compiler.HasOption("debug-");

			string debugtypeString = "none";
			if (debug)
			{
				string debugtype = module.Compiler.GetOptionValue("debug");
				if (debugtype == "full" || String.IsNullOrEmpty(debugtype))
				{
					debugtypeString = "full";
				}
				else if (debugtype == "pdbonly")
				{
					debugtypeString = "pdbonly";
				}
				else
				{
					this.Log.Warning.WriteLine("{0}: invalid value of /debug switch /debug:{1}", module.Name, debugtype);
				}
			}
			data.Add("DebugType", debugtypeString);

			if (!UseNewCSProjFormat())
			{
				data.Add("DebugSymbols", debug);
				data.Add("WarningLevel", "4");
				data.Add("AllowUnsafeBlocks", module.Compiler.HasOption("unsafe"));
				data.Add("BaseIntermediateOutputPath", GetProjectPath(module.GetVSTmpFolder().Path, addDot: true));
				data.Add("OutDir", GetProjectPath(module.OutputDir.NormalizedPath.EnsureTrailingSlash(defaultPath: "."), addDot: true)); // this should be covered by OutputPath but in old project format gets confused translating to OutDir
			}
			else
			{
				data.Add("AppendTargetFrameworkToOutputPath", "false");
			}

			data.Add("IntermediateOutputPath", "$(BaseIntermediateOutputPath)");
			data.Add("DefineConstants", module.Compiler.Defines.ToString(";") + ";EA_SLN_BUILD");
			data.Add("OutputPath", GetProjectPath(module.OutputDir.NormalizedPath.EnsureTrailingSlash(defaultPath: "."), addDot: true));
			data.Add("FileAlignment", "4096");
			bool hasOptimize = module.Compiler.HasOption("optimize", exact: true) || module.Compiler.HasOption("optimize+", exact: true);
			bool hasDeOptimize = module.Compiler.HasOption("optimize-", exact: true);
			if (hasOptimize && !hasDeOptimize)
			{
				data.Add("Optimize", true);
			}
			else if (hasDeOptimize && !hasOptimize)
			{
				data.Add("Optimize", false);
			}

			data.Add("BaseAddress", "285212672");
			data.Add("CheckForOverflowUnderflow", "false");
			data.Add("ConfigurationOverrideFile", "");
			if (!module.Compiler.DocFile.IsNullOrEmpty())
			{
				data.Add("DocumentationFile", GetProjectPath(module.Compiler.DocFile));
			}
			data.Add("NoStdLib", module.Compiler.GetBoolenOptionValue("nostdlib"));
			data.AddIfTrue("NoConfig", module.Compiler.GetBoolenOptionValue("noconfig"));
			data.AddNonEmpty("NoWarn", module.Compiler.GetOptionValues("nowarn", ","));
			data.Add("RegisterForComInterop", "false");
			data.Add("RemoveIntegerChecks", "false");

			string warnaserrorlist = module.Compiler.GetOptionValue("warnaserror:");
			if (!String.IsNullOrEmpty(warnaserrorlist))
			{
				data.Add("TreatWarningsAsErrors", "false");
				data.Add("WarningsAsErrors", warnaserrorlist);
			}
			else
			{
				data.Add("TreatWarningsAsErrors", module.Compiler.HasOption("warnaserror"));

				string warningsnotaserrors = module.Compiler.GetOptionValue("warnaserror-");
				if (!String.IsNullOrEmpty(warningsnotaserrors))
				{
					data.Add("WarningsNotAsErrors", warningsnotaserrors);
				}
			}

			data.AddNonEmpty("PlatformTarget", module.Compiler.GetOptionValue("platform"));

			if (module.DisableVSHosting)
			{
				data.Add("UseVSHostingProcess", (!module.DisableVSHosting));
			}

			var rspFile = CreateResponseFileIfNeeded(module);
			if (!String.IsNullOrEmpty(rspFile))
			{
				data.AddNonEmpty("CompilerResponseFile", GetProjectPath(rspFile));
			}

			if (module.GenerateSerializationAssemblies != DotNetGenerateSerializationAssembliesTypes.None)
			{
				data.Add("GenerateSerializationAssemblies", module.GenerateSerializationAssemblies.ToString());
			}

			if (EnableFxCop(module))
			{
				data.AddNonEmpty("CodeAnalysisRuleSet", Package.Project.Properties.GetPropertyOrDefault(module.GroupName + ".fxcop.ruleset",
																		 Package.Project.Properties.GetPropertyOrDefault("package.fxcop.ruleset", String.Empty)));

				data.Add("RunCodeAnalysis", "true");
			}

			if (module.Compiler.HasOption("nowin32manifest"))
			{
				data.Add("NoWin32Manifest", "true");
			}

			bool setDisableUpToDateCheck = module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.dotnet.set-disable-uptodatecheck",
																		module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.set-disable-uptodatecheck", false));

			if (setDisableUpToDateCheck)
			{
				data.Add("DisableFastUpToDateCheck", "true");
			}

			if (module.Compiler.HasOption("deterministic"))
			{
				data.Add("Deterministic", "true");
			}

			if (!UseNewCSProjFormat())
			{
				if (module.GenerateAutoBindingRedirects())
				{
					data.Add("AutoGenerateBindingRedirects", true);
					if (!module.IsKindOf(Module.Program)) // AutoGenerateBindingRedirects doesn't do anything for non-application project types unles the below is set
					{
						data.Add("GenerateBindingRedirectsOutputType", true);
					}
				}
			}

			return data;
		}

		public string ConvertToNewTargetFramework(string targetFrameworkVersion)
		{
			if (targetFrameworkVersion.StartsWith("net"))
			{
				return targetFrameworkVersion;
			}
			return "net" + targetFrameworkVersion.TrimStart('v').Replace(".", "");
		}

		private static bool HasWindowsDependencies(IModule module)
        {
			if (module.Package.Project != null && module.Package.Project.Properties.GetBooleanPropertyOrDefault(module.GroupName + ".usewpf", false))
			{
				return true;
			}
			if (module.Package.Project != null && module.Package.Project.Properties.GetBooleanPropertyOrDefault(module.GroupName + ".usewindowsforms", false))
			{
				return true;
			}
			foreach (var d in module.Dependents)
			{
				// Check dependent modules to see if they have wpf/winforms dependencies
				if (HasWindowsDependencies(d.Dependent)) // recursive
					return true;
			}
			return false;
		}

		protected virtual IDictionary<string, string> GetProjectProperties()
		{
			var data = new OrderedDictionary<string, string>();

			data.AddNonEmpty("ProductVersion", ProductVersion);
			data.AddNonEmpty("SchemaVersion", SchemaVersion);

			data.AddNonEmpty("ApplicationManifest", StartupModule.ApplicationManifest);
			data.AddNonEmpty("ApplicationIcon", GetProjectPath(StartupModule.Compiler.ApplicationIcon));
			data.AddNonEmpty("AssemblyKeyContainerName", String.Empty);

			var keyfile = StartupModule.Compiler.GetOptionValue("keyfile");
			data.Add("SignAssembly", !String.IsNullOrEmpty(keyfile));
			data.AddNonEmpty("AssemblyOriginatorKeyFile", GetProjectPath(keyfile));

			data.AddNonEmpty("DefaultClientScript", "JScript");
			data.AddNonEmpty("DefaultHTMLPageLayout", "Grid");
			data.AddNonEmpty("DefaultTargetSchema", DefaultTargetSchema);
			data.AddNonEmpty("DelaySign", "false");

			if (!UseNewCSProjFormat())
			{
				data.Add("OutputType", OutputType.ToString());
				data.Add("ProjectGuid", ProjectGuidString);
			}
			else
			{
				if (OutputType.ToString() != "Library")
				{
					data.Add("OutputType", OutputType.ToString());
				}
			}

			data.AddNonEmpty("RootNamespace", RootNamespace);
			data.Add("AssemblyName", StartupModule.Compiler.OutputName);

			if (UseNewCSProjFormat())
			{
				data.Add("PackageId", StartupModule.Compiler.OutputName + "-" + StartupModule.BuildGroup);

				// disable default items for now
				// the new csproj format automatically includes all files under the project directory
				// snowcache also currently requires each file to be listed explicitly
				data.AddNonEmpty("EnableDefaultItems", "false");

				data.AddNonEmpty("GenerateAssemblyInfo", "false");

				data.Add("RuntimeIdentifiers", "win7-x64");

				if (StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".usewpf", false))
				{
					data.Add("UseWpf", "true");
				}
				if (StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".usewindowsforms", false))
				{
					data.Add("UseWindowsForms", "true");
				}

				string framework = ConvertToNewTargetFramework(TargetFrameworkVersion);
				Regex netversionregex = new Regex(@"net(?<major>[0-9]+)\..*");
				Match m = netversionregex.Match(framework);
				int majorVersion = m.Success ? int.Parse(m.Groups["major"].Value) : 0;
				// In .net5 (and higher potentially), we need to target net5.0-windows to be able to use winforms/wpf
				if (majorVersion >= 5 && HasWindowsDependencies(StartupModule))
                {
					framework = framework + "-windows";

				}
				data.AddNonEmpty("TargetFramework", framework);
			}
			else
			{
				data.AddNonEmpty("TargetFrameworkVersion", TargetFrameworkVersion);
			}

			data.Add("RestorePackagesPath", Path.Combine(PackageMap.Instance.PackageRoots.OnDemandRoot.FullName, "Nuget"));

			if (!BuildGenerator.IsPortable)
			{
				// Point visual studio at the WindowsSDK for the case where the user is using the non-proxy and has removed the installed dotnet tools
				string toolsDir = StartupModule.Package.Project.Properties["package.WindowsSDK.dotnet.tools.dir"];
				if (!String.IsNullOrEmpty(toolsDir))
				{
					if (Environment.Is64BitOperatingSystem)
					{
						toolsDir = Path.Combine(toolsDir, "x64");
					}
					data.Add("TargetFrameworkSDKToolsDirectory", toolsDir);
				}

				if (TargetFrameworkFamily == DotNetFrameworkFamilies.Framework)
				{
					// If set, don't let VS try to resolve installed .net sdk and use explicitly root
					string referenceAssemblyDir = StartupModule.Package.Project.Properties["package.DotNet.vs-referencedir-override"];
					data.AddNonEmpty("TargetFrameworkRootPath", referenceAssemblyDir);
				}
			}

			data.AddNonEmpty("RunPostBuildEvent", StartupModule.RunPostBuildEventCondition);
			data.AddNonEmpty("StartupObject", String.Empty);
			data.AddNonEmpty("FileUpgradeFlags", String.Empty);
			data.AddNonEmpty("UpgradeBackupLocation", String.Empty);
			data.AddNonEmpty("AppDesignerFolder", StartupModule.AppDesignerFolder.IsNullOrEmpty() ? "Properties" : StartupModule.AppDesignerFolder.Path);

			// We add this property because we'll be copying content files from source tree (which has read only attributes) to destination
			// output folder.  To prevent incremental perforce sync / incremental build issue, we'll need to set a global OverwriteReadOnlyFiles
			// MSBuild property to true so that the MSBuild's CopyToOutputDirectory task will copy the content files with read only file overwrite
			// turned on.  If we want to remove this property, we'll need to figure out other means of solving this use case.
			data.AddNonEmpty("OverwriteReadOnlyFiles", "true");

			if (!UseNewCSProjFormat())
			{
				var projectTypeGuids = ProjectTypeGuids.OrderedDistinct();
				string customProjectTypeGuids = StartupModule.PropGroupValue("projecttypeguids", null);
				if (customProjectTypeGuids != null)
				{
					data.Add("ProjectTypeGuids", customProjectTypeGuids);
				}
				else if (projectTypeGuids.Count() > 0)
				{
					data.Add("ProjectTypeGuids", String.Format("{0};{1}", projectTypeGuids.ToString(";", g => VSSolutionBase.ToString(g)), VSSolutionBase.ToString(ProjectTypeGuid)));
				}
			}

			// Invoke Visual Studio extensions to allow users to add custom globals
			foreach (var module in Modules)
			{
				ExecuteExtensions(module, extension => extension.ProjectGlobals(data));
			}

			return data;
		}

		#endregion

		#region Write Methods

		private void WriteLinkPath(IXmlWriter writer, FileEntry fe)
		{
			if (!fe.Path.IsPathInDirectory(OutputDir))
			{
				writer.WriteNonEmptyElementString("Link", GetLinkPath(fe));
			}
		}

		protected void WriteNonEmptyElementString(IXmlWriter writer, string name, string value)
		{
			if (!String.IsNullOrWhiteSpace(value))
			{
				writer.WriteElementString(name, value);
			}
		}

		protected virtual void WriteProjectProperties(IXmlWriter writer)
		{
			writer.WriteStartElement("PropertyGroup");

			if (!UseNewCSProjFormat())
			{
				writer.WriteStartElement("Configuration");
				writer.WriteAttributeString("Condition", " '$(Configuration)' == '' ");
				writer.WriteString(GetVSProjTargetConfiguration(BuildGenerator.StartupConfiguration));
				writer.WriteEndElement();

				writer.WriteStartElement("Platform");
				writer.WriteAttributeString("Condition", " '$(Platform)' == '' ");
				writer.WriteString(StartupPlatform);
				writer.WriteEndElement();
			}
			else
			{
				writer.WriteStartElement("Configurations");
				writer.WriteString(GetProjectTargetConfigurationList());
				writer.WriteEndElement();

				writer.WriteStartElement("Platforms");
				writer.WriteString(GetProjectTargetPlatformList());
				writer.WriteEndElement();
			}

			foreach (var entry in GetProjectProperties())
			{
				writer.WriteElementString(entry.Key, entry.Value);
			}

			writer.WriteEndElement(); //PropertyGroup
		}

		protected virtual void WriteConfigurations(IXmlWriter writer)
		{
			foreach (Module_DotNet module in Modules)
			{
				WriteOneConfiguration(writer, module);
			}
		}

		protected virtual void WriteOneConfiguration(IXmlWriter writer, Module_DotNet module)
		{
			WriteConfigurationProperties(writer, module);

		}

		protected virtual void WriteConfigurationProperties(IXmlWriter writer, Module_DotNet module)
		{
			// Visual studio doesn't like when these are in the conditional propertygroups when using the new csproj format
			writer.WriteStartElement("PropertyGroup");
			if (UseNewCSProjFormat())
			{
				writer.WriteElementString("AllowUnsafeBlocks", module.Compiler.HasOption("unsafe"));
			}
			writer.WriteEndElement(); //PropertyGroup

			writer.WriteStartElement("PropertyGroup");
			writer.WriteAttributeString("Condition", String.Format(" '$(Configuration)|$(Platform)' == '{0}' ", GetVSProjConfigurationName(module.Configuration)));

			foreach (var entry in GetConfigurationProperties(module))
			{
				writer.WriteElementString(entry.Key, entry.Value);
			}

			AddCustomVsOptions(writer, module);

			writer.WriteEndElement(); //PropertyGroup
		}


		protected virtual void WriteBuildSteps(IXmlWriter writer)
		{

			var buildstepgroups = GroupBuildSteps();

			foreach (var group in buildstepgroups)
			{
				writer.WriteStartElement("PropertyGroup");

				WriteConfigCondition(writer, group.Configurations);

				StringBuilder preBuildEvents = new StringBuilder();
				StringBuilder postBuildEvents = new StringBuilder();

				foreach (var buildstepcmd in group.BuildSteps)
				{
					if (buildstepcmd.BuildStep.IsKindOf(BuildStep.PreBuild))
					{
						preBuildEvents.AppendLine(buildstepcmd.Cmd);
					}
					if (buildstepcmd.BuildStep.IsKindOf(BuildStep.PostBuild))
					{
						postBuildEvents.AppendLine(buildstepcmd.Cmd);
					}
				}

				if (preBuildEvents.Length != 0)
				{
					writer.WriteElementString("PreBuildEvent", preBuildEvents.ToString());
				}

				if (postBuildEvents.Length != 0)
				{
					writer.WriteElementString("PostBuildEvent", postBuildEvents.ToString());
				}

				writer.WriteEndElement(); //PropertyGroup
			}

		}


		protected virtual void WriteReferences(IXmlWriter writer)
		{
			IEnumerable<ProjectRefEntry> projectReferences;
			IDictionary<PathString, FileEntry> references;

			GetReferences(out projectReferences, out references);

			if (projectReferences.FirstOrDefault() != null || references.Count > 0 && AnyCpuConfigurations.Any())
			{
				// Supress warning
				// MSB3270: There was a mismatch between the processor architecture of the project being built "MSIL" and the processor architecture of the reference
				writer.WriteStartElement("PropertyGroup");
				writer.WriteNonEmptyAttributeString("Condition", GetConfigCondition(AnyCpuConfigurations));
				writer.WriteElementString("ResolveAssemblyWarnOrErrorOnTargetArchitectureMismatch", "None");
				writer.WriteEndElement();
			}

			WriteAssemblyReferences(writer, references.Values);
			WriteProjectReferences(writer, projectReferences);
			WriteComReferences(writer);
			WriteWebReferences(writer);
			WriteNuGetReferences(writer);

		}

		private void WriteAssemblyReferences(IXmlWriter writer, IEnumerable<FileEntry> references)
		{
			foreach (var group in references.GroupBy(f => f.ConfigSignature, f => f))
			{
				if (group.Count() > 0)
				{
					writer.WriteStartElement("ItemGroup");

					var first = group.FirstOrDefault();

					var conditionString = (first != null) ? GetConfigCondition(first.ConfigEntries.Select(ce => ce.Configuration)) : String.Empty;

					WriteConfigCondition(writer, group);

					foreach (FileEntry reference in group.OrderBy(fe => fe.Path))
					{
						WriteReference(writer, reference, conditionString);
					}
					writer.WriteEndElement(); //ItemGroup
				}
			}
		}

		private void WriteReference(IXmlWriter writer, FileEntry reference, string conditionString)
		{
			string refNamespace = Path.GetFileNameWithoutExtension(reference.Path.Path);
			string refName = Path.GetFileNameWithoutExtension(reference.Path.Path);
			string refHintPath = GetProjectPath(reference.Path);

			bool isSystem = String.IsNullOrEmpty(Path.GetDirectoryName(reference.Path.Path)) || reference.Path.Path.Contains(@"\GAC_MSIL\");
			if (isSystem)
			{
				refHintPath = Path.GetFileName(reference.Path.Path);
			}

			bool isCopyLocal = null != reference.ConfigEntries.Find(ce => ce.IsKindOf(FileEntry.CopyLocal));

			FileEntry.ConfigFileEntry firstConfigFileEntry = reference.ConfigEntries.First();
			OptionSet options = null;
			if (firstConfigFileEntry.FileItem.OptionSetName != null)
			{
				options = firstConfigFileEntry.Project.NamedOptionSets[firstConfigFileEntry.FileItem.OptionSetName];
			}

			writer.WriteStartElement("Reference");

			Dictionary<string, string> referenceAttributes = new Dictionary<string, string>();
			Dictionary<string, string> referenceProperties = new Dictionary<string, string>();
			if (options != null)
			{
				var assemblyAttributes = options.GetOptionDictionary("visual-studio-assembly-attributes");
				if (assemblyAttributes != null)
				{
					referenceAttributes = assemblyAttributes;
				}
				var assemblyProperties = options.GetOptionDictionary("visual-studio-assembly-properties");
				if (assemblyProperties != null)
				{
					referenceProperties = assemblyProperties;
				}
			}

			if (!referenceAttributes.ContainsKey("Condition"))
			{
				referenceAttributes.AddNonEmpty("Condition", conditionString);
			}

			if (!referenceAttributes.ContainsKey("Include"))
			{
				referenceAttributes.Add("Include", refNamespace);
			}

			if (!referenceProperties.ContainsKey("Name") && !UseNewCSProjFormat())
			{
				referenceProperties.Add("Name", refName);
			}

			if (!referenceProperties.ContainsKey("HintPath"))
			{
				referenceProperties.Add("HintPath", refHintPath);
			}

			if (!referenceProperties.ContainsKey("Private"))
			{
				if (IsTargetingNetCoreStandard())
				{
					referenceProperties.Add("Private", true); // set copylocal to true in order to get VS to add this to the .deps.json for .net core project
				}
				else
				{
					referenceProperties.Add("Private", false);
				}
			}

			foreach (var attr in referenceAttributes)
			{
				writer.WriteAttributeString(attr.Key, attr.Value);
			}
			foreach (var prop in referenceProperties)
			{
				writer.WriteElementString(prop.Key, prop.Value);
			}

			writer.WriteEndElement(); //Reference
		}

		internal virtual void WriteProjectReferences(IXmlWriter writer, IEnumerable<ProjectRefEntry> projectReferences)
		{
			if (projectReferences.Count() > 0)
			{
				var projrefGroups = GroupProjectReferencesByConfigConditions(projectReferences);

				foreach (var entry in projrefGroups)
				{
					if (entry.Value.Count() > 0)
					{
						writer.WriteStartElement("ItemGroup");
						writer.WriteNonEmptyAttributeString("Condition", entry.Key);

						foreach (var projRefWithType in entry.Value)
						{
							var projRefEntry = projRefWithType.Item1;
							var projRefType = new BitMask(projRefWithType.Item2);

							writer.WriteStartElement("ProjectReference");
							{
								writer.WriteAttributeString("Include", PathUtil.RelativePath(Path.Combine(projRefEntry.ProjectRef.OutputDir.Path, projRefEntry.ProjectRef.ProjectFileName), OutputDir.Path));
								if (UseNewCSProjFormat())
								{
									writer.WriteAttributeString("PrivateAssets", "All");
								}
								else
								{
									writer.WriteElementString("Project", projRefEntry.ProjectRef.ProjectGuidString);
									writer.WriteElementString("Name", projRefEntry.ProjectRef.Name);
								}

								if (IsTargetingNetCoreStandard())
								{
									writer.WriteAttributeString("Private", true); // need to set this to true for .net core apps otherwise they don't get put into the
									// .deps.json file, we stomp the entire copy phase in VS to allow our own to handle everything
									// NUGET-TODO: blinding setting this^ true is probably a bit much - need to test partial copy local trees
								}
								else
								{
									writer.WriteElementString("Private", false); // disable copy local at reference level, we handle this via custom mechanism
								}

								if (!projRefType.IsKindOf(ProjectRefEntry.ReferenceOutputAssembly))
								{
									// by default output assembly is reference but in some cases we want a build dependency without using the output
									writer.WriteElementString("ReferenceOutputAssembly", "false");
								}

							}
							writer.WriteEndElement(); // ProjectReference
						}
						writer.WriteEndElement(); // ItemGroup (ProjectReferences)
					}
				}
			}
		}

		private enum RegKind
		{
			RegKind_Default = 0,
			RegKind_Register = 1,
			RegKind_None = 2
		}

		[DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
		private static extern void LoadTypeLibEx(string strTypeLibName, RegKind regKind, out System.Runtime.InteropServices.ComTypes.ITypeLib typeLib);

		protected virtual void WriteComReferences(IXmlWriter writer)
		{
			var comreferences = new Dictionary<PathString, FileEntry>();
			foreach (var module in Modules.Cast<Module_DotNet>())
			{
				foreach (var comref in module.Compiler.ComAssemblies.FileItems)
				{
					var key = new PathString(Path.GetFileName(comref.Path.Path));
					UpdateFileEntry(module, comreferences, key, comref, module.Compiler.ComAssemblies.BaseDirectory, module.Configuration);
				}
			}

			foreach (var group in comreferences.Values.GroupBy(f => f.ConfigSignature, f => f))
			{
				if (group.Count() > 0)
				{
					writer.WriteStartElement("ItemGroup");

					WriteConfigCondition(writer, group);

					foreach (var fe in group.OrderBy(fe => fe.Path))
					{

						if (!File.Exists(fe.Path.Path))
						{
							string msg = String.Format("Unable to generate interop assembly for COM reference, file '{0}' not found", fe.Path.Path);
							throw new BuildException(msg);
						}

						System.Runtime.InteropServices.ComTypes.ITypeLib typeLib = null;
						try
						{
							LoadTypeLibEx(fe.Path.Path, RegKind.RegKind_None, out typeLib);
						}
						catch (Exception ex)
						{
							string msg = String.Format("Unable to generate interop assembly for COM reference '{0}'.", fe.Path.Path);
							throw new BuildException(msg, ex);
						}
						if (typeLib == null)
						{
							string msg = String.Format("Unable to generate interop assembly for COM reference '{0}'.", fe.Path.Path);
							throw new BuildException(msg);
						}
						IntPtr pTypeLibAttr = IntPtr.Zero;
						typeLib.GetLibAttr(out pTypeLibAttr);

						System.Runtime.InteropServices.ComTypes.TYPELIBATTR typeLibAttr =
							(System.Runtime.InteropServices.ComTypes.TYPELIBATTR)Marshal.PtrToStructure(pTypeLibAttr, typeof(System.Runtime.InteropServices.ComTypes.TYPELIBATTR));

#if NETFRAMEWORK
						string libName = Marshal.GetTypeLibName(typeLib);
						Guid libGuid = Marshal.GetTypeLibGuid(typeLib);
#else
						Guid libGuid = typeLibAttr.guid;

						// copied from reference source...
						typeLib.GetDocumentation(-1, out string libName, out string unused1, out int unused2, out string unused3);
#endif

						writer.WriteStartElement("COMReference");
						writer.WriteAttributeString("Include", GetProjectPath(PathNormalizer.Normalize(libName)));
						writer.WriteElementString("Guid", "{" + libGuid.ToString() + "}");
						writer.WriteElementString("VersionMajor", typeLibAttr.wMajorVerNum.ToString());
						writer.WriteElementString("VersionMinor", typeLibAttr.wMinorVerNum.ToString());
						writer.WriteElementString("Lcid", typeLibAttr.lcid.ToString());
						writer.WriteElementString("Isolated", "False");
						writer.WriteElementString("WrapperTool", "tlbimp");
						writer.WriteElementString("EmbedInteropTypes", "true");
						writer.WriteEndElement(); //COMReference
					}
				}

				writer.WriteFullEndElement(); //References
			}
		}

		protected virtual void WriteWebReferences(IXmlWriter writer)
		{
			if (StartupModule.WebReferences != null && StartupModule.WebReferences.Options.Count > 0)
			{
				var webrefs = StartupModule.WebReferences;
				if (webrefs.Options.Count > 0)
				{
					string refOutput = "webreferences";
					writer.WriteStartElement("ItemGroup");
					writer.WriteStartElement("WebReferences");
					writer.WriteAttributeString("Include", PathNormalizer.Normalize(refOutput, false));
					writer.WriteEndElement(); //WebReferences
					writer.WriteEndElement(); //ItemGroup

					writer.WriteStartElement("ItemGroup");
					foreach (var webref in webrefs.Options)
					{
						var refName = webref.Key;
						var refUrl = webref.Value;

						writer.WriteStartElement("WebReferenceUrl");
						writer.WriteAttributeString("Include", refUrl);
						writer.WriteElementString("UrlBehavior", "Dynamic");
						writer.WriteElementString("RelPath", Path.Combine(refOutput, refName));
						writer.WriteElementString("UpdateFromURL", refUrl);
						writer.WriteEndElement(); //WebReferenceUrl
					}
					writer.WriteEndElement(); //ItemGroup

					writer.WriteStartElement("ItemGroup");

					foreach (var webref in StartupModule.WebReferences.Options)
					{
						var refName = webref.Key.ToString();
						var refUrl = webref.Value;

						string path = System.IO.Path.Combine(refOutput, refName);
						string map = System.IO.Path.Combine(path, "Reference.map");
						string disco = System.IO.Path.Combine(path, Path.GetFileNameWithoutExtension(refUrl));

						writer.WriteStartElement("None");
						writer.WriteAttributeString("Include", map);
						writer.WriteElementString("Generator", "MSDiscoCodeGenerator");
						writer.WriteElementString("LastGenOutput", "Reference.cs");
						writer.WriteEndElement(); //None
						writer.WriteStartElement("None");
						writer.WriteAttributeString("Include", disco);
						writer.WriteEndElement(); //None
					}
					writer.WriteEndElement(); //ItemGroup
				}
			}
		}

		protected virtual void WriteNuGetReferences(IXmlWriter writer)
		{
			if (!StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_USE_NUGET_RESOLUTION, false) ||
				!StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_USE_VS_PACKAGE_REFERENCES, false))
				return;

			if (StartupModule.NuGetReferences != null)
			{
				writer.WriteStartElement("ItemGroup");

				foreach (NugetPackage reference in PackageMap.Instance.GetNugetPackages(
					StartupModule.Package.Project.BuildGraph().GetTargetNetVersion(StartupModule.Package.Project), 
					StartupModule.NuGetReferences))
				{
					writer.WriteStartElement("PackageReference");
					writer.WriteAttributeString("Include", reference.Id);
					writer.WriteAttributeString("Version", reference.Version);
					writer.WriteEndElement(); //None
				}
				writer.WriteEndElement(); //ItemGroup
			}
		}

		// FileData bit mask currently goes from 0 to 64 (0x0040)
		private const uint Resource = 8388608;      // 0x00800000
		private const uint Source = 16777216;     // 0x01000000
		private const uint Forms = 33554432;     // 0x02000000
		private const uint Xaml = 67108864;     // 0x04000000
		private const uint Xoml = 134217728;    // 0x08000000

		protected virtual void WriteFiles(IXmlWriter writer)
		{

			// [group].[module].resourcefiles.notembed saved to NonEmbeddedResources fileset (only get used in DotNet sln build).
			// [group].[module].resourcefiles saved to Resources fileset
			// [group].[module].contentfiles saved to ContentFiles fileset
			// The WriteResourceFile() function depending on file extension may re-assign the file with <EmbeddedResource>,
			// <Content>, <Resource> or <None> tool in the generated csproj file depending on some known file extension.
			// You can override this behaviour by defining your own tool with 'visual-studio-tool' (search for that keyword
			// in the chm doc).

			var files = GetAllFiles(tool =>
			{
				List<Tuple<FileSet, uint, Tool>> filesets = new List<Tuple<FileSet, uint, Tool>>();
				DotNetCompiler compilerTool = tool as DotNetCompiler;
				if (compilerTool != null)
				{
					filesets.Add(Tuple.Create(compilerTool.Resources as FileSet, Resource, tool));
					filesets.Add(Tuple.Create(compilerTool.NonEmbeddedResources, Resource, tool));
					filesets.Add(Tuple.Create(compilerTool.ContentFiles, Resource, tool));
					filesets.Add(Tuple.Create(compilerTool.SourceFiles, Source, tool));
				}

				return filesets;
			}).Select(e => e.Value);

			PrepareSubTypeMappings(writer);

			var textTemplateServiceConfigs = new List<Configuration>();

			using (var xamlFileTracker = new DuplicateXamlFileTracker(IsTargetingNetCoreStandard()))
			{
				foreach
				(
					// process files in groups based upon which configs they apply to, as we need to handle whole configurations conditions together in order
					// to deduce relationship attributes for code behind and xaml / resources
					IEnumerable<FileEntry> group in files.GroupBy
					(
						f => String.Join(Environment.NewLine, f.ConfigEntries.Select(ce => ce.Configuration.Name).OrderBy(s => s)) // group by string key of config names ordered alphabetically and delimited by new line
					)
				)
				{
					if (group.Count() > 0)
					{
						// Compute these hashes to use in code behind:
						IDictionary<string, string> srcHash;
						IDictionary<string, string> resHash;
						IDictionary<string, string> xamlHash;
						IDictionary<string, string> xomlHash;
						IDictionary<string, string> ttHash;

						ComputeCodeBehindCache(group, out srcHash, out resHash, out xamlHash, out xomlHash, out ttHash);

						// Add Item File Items
						writer.WriteStartElement("ItemGroup");
						WriteConfigCondition(writer, group);
						bool requiresWpf = false;

						foreach (FileEntry fe in group)
						{
							string projectPath = GetProjectPath(fe.Path);
							// The path key need to be all lower case due to the fact that Windows is case insensitve file system and we have people
							// specifying source files and resource files in different case causing code behind code doesn't match up.
							string relPathKey = projectPath.TrimExtension().ToLowerInvariant();

							var firstFe = fe.ConfigEntries.First();

							if (!firstFe.IsKindOf(FileEntry.ExcludedFromBuild))
							{
								if (firstFe.IsKindOf(Resource))
								{
									bool addttService;
									bool resourceRequiresWpf;
									WriteResourceFile(writer, fe, projectPath, relPathKey, resHash, srcHash, xomlHash, out addttService, out resourceRequiresWpf);
									requiresWpf |= resourceRequiresWpf;
									if (addttService)
									{
										textTemplateServiceConfigs.AddRange(fe.ConfigEntries.Select(ce => ce.Configuration));
									}
								}
								else if (firstFe.IsKindOf(Xaml))
								{
									xamlFileTracker.Add(fe);
									WriteXamlFile(writer, fe, projectPath);
								}
								else if (firstFe.IsKindOf(Xoml))
								{
									WriteXomlFile(writer, fe, projectPath);
								}
								else if (firstFe.IsKindOf(Source))
								{
									WriteSourceFile(writer, fe, projectPath, relPathKey, resHash, srcHash, xomlHash, xamlHash, ttHash);
								}
								else
								{
									writer.WriteStartElement("None");
									writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
									WriteLinkPath(writer, fe);
									writer.WriteEndElement(); // None
								}
							}
							else
							{
								writer.WriteStartElement("None");
								writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
								WriteLinkPath(writer, fe);
								writer.WriteEndElement(); // None
							}

						}
						writer.WriteEndElement(); //ItemGroup

						// If there are resource files requiring usewpf, enable wpf
						if (requiresWpf && !StartupModule.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".usewpf", false))
						{
							writer.WriteStartElement("PropertyGroup");
								writer.WriteElementString("UseWpf", "true");
							writer.WriteEndElement();
						}
					}
				}

				if (textTemplateServiceConfigs.Count > 0)
				{
					writer.WriteStartElement("ItemGroup");
					WriteConfigCondition(writer, textTemplateServiceConfigs.OrderedDistinct());
					writer.WriteStartElement("Service");
					writer.WriteAttributeString("Include", "{508349B6-6B84-4DF5-91F0-309BEEBAD82D}");
					writer.WriteEndElement(); //Service
					writer.WriteEndElement(); //ItemGroup
				}

				// custom build files
				IEnumerable<FileEntry> customBuildFiles = GetAllFiles(tool =>
					{
						List<Tuple<FileSet, uint, Tool>> filesets = new List<Tuple<FileSet, uint, Tool>>();
						if (tool is BuildTool) filesets.Add(Tuple.Create((tool as BuildTool).Files, FileEntry.Buildable, tool));
						return filesets;
					}).Select(e => e.Value);
				if (customBuildFiles.Any())
				{
					writer.WriteStartElement("ItemGroup");
					{
						foreach (FileEntry customFE in customBuildFiles)
						{
							foreach (FileEntry.ConfigFileEntry configEntry in customFE.ConfigEntries)
							{
								Tool tool = configEntry.FileItem.GetTool();
								BuildTool buildTool = tool as BuildTool;
								if (buildTool != null)
								{
									string command = GetCommandScriptWithCreateDirectories(buildTool).TrimWhiteSpace();
									if (!String.IsNullOrEmpty(command))
									{
										string condition = GetConfigCondition(configEntry.Configuration);
										writer.WriteStartElement("CustomBuild");
										{
											writer.WriteAttributeString("Include", GetProjectPath(customFE.Path));
											WriteLinkPath(writer, customFE);
											WriteElementStringWithConfigCondition(writer, "Command", command, condition);
											WriteElementStringWithConfigConditionNonEmpty(writer, "Message", buildTool.Description.TrimWhiteSpace(), condition);
											WriteElementStringWithConfigConditionNonEmpty(writer, "AdditionalInputs", buildTool.InputDependencies.FileItems.Select(fi => fi.Path.Path).OrderedDistinct().ToString(";", path => path).TrimWhiteSpace(), condition);
											WriteElementStringWithConfigConditionNonEmpty(writer, "Outputs", buildTool.OutputDependencies.ThreadSafeFileItems().Select(fi => fi.Path.Path).ToString(";", path => path).TrimWhiteSpace(), condition);
										}
										writer.WriteEndElement(); // CustomBuild
									}
								}
							}
						}
					}
					writer.WriteEndElement(); // ItemGroup
				}
			}
		}

		private void WriteXamlFile(IXmlWriter writer, FileEntry fe, string projectPath)
		{
			string filename = fe.Path.Path;

			if (File.Exists(filename))
			{
				string elementName = "Page";
				try
				{
					using (var reader = new XmlTextReader(filename))
					{
						while (reader.Read())
						{
							// Get first element:
							if (reader.NodeType == XmlNodeType.Element)
							{
								if (0 == String.Compare(reader.LocalName, "App") || 0 == String.Compare(reader.LocalName, "Application"))
								{
									elementName = "ApplicationDefinition";
								}
								else if (0 == String.Compare(reader.LocalName, "Activity"))
								{
									elementName = "XamlAppDef";
								}

								break;
							}
						}
					}
				}
				catch (Exception e)
				{
					StringBuilder builder = new StringBuilder();
					builder.AppendLine(string.Format("ERROR: Exception thrown when attempting to read XAML file {0}", filename));
					builder.AppendLine(string.Format("Exception message: {0}", e.Message));

					throw new BuildException(builder.ToString());
				}

				var subType = _savedCsprojData.GetSubTypeMapping(elementName, projectPath, "Designer");

				var options = GetFileEntryOptionset(fe);

				writer.WriteStartElement(elementName);
				writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
				writer.WriteElementString("Generator", "MSBuild:Compile");
				writer.WriteElementString("SubType", subType);
				WriteLinkPath(writer, fe);

				WriteToolProperties(writer, options);

				writer.WriteEndElement(); //ElementName
			}

		}

		private void WriteXomlFile(IXmlWriter writer, FileEntry fe, string projectPath)
		{
			string filename = fe.Path.Path;
			if (File.Exists(filename))
			{
				var elementName = "Content";
				var subType = _savedCsprojData.GetSubTypeMapping(elementName, projectPath, "Component");
				writer.WriteStartElement(elementName);
				writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
				writer.WriteElementString("SubType", subType);
				WriteLinkPath(writer, fe);

				writer.WriteEndElement();
			}
		}

		private void WriteResourceFile(IXmlWriter writer, FileEntry fe, string relPath, string relPathKey, IDictionary<string, string> resHash, IDictionary<string, string> srcHash, IDictionary<string, string> xomlHash, out bool addttService, out bool requiresWpf)
		{
			string filename = fe.Path.Path;

			bool isGlobalResx = false;
			bool isWebConfig = false;
			bool isCopyLocalSupported = false;
			string linkedFileName = null;
			string langExt = String.Empty;
			addttService = false;
			requiresWpf = false;

			// If user specified a specific tool, we should always use that.
			OptionSet options = null;
			string vstool = null;
			var firstFe = fe.ConfigEntries.First();
			if (!String.IsNullOrEmpty(firstFe.FileItem.OptionSetName) && firstFe.Project != null && firstFe.Project.NamedOptionSets.TryGetValue(firstFe.FileItem.OptionSetName, out options))
			{
				vstool = options.Options["visual-studio-tool"].TrimWhiteSpace();
			}

			if (!String.IsNullOrEmpty(vstool) &&
				Path.GetExtension(filename) != ".tt")  // The template .tt files already expect the usage of visual-studio-tool and requires a very specific setup.
			{
				// If custom tool exists, use the custom tool.  But this also override default behaviour of some known file extension.
				writer.WriteStartElement(vstool);
				isCopyLocalSupported = vstool == "Content";
				writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
				WriteToolAttributes(writer, options);
				WriteToolProperties(writer, options);
			}
			else
			{
				// Default back to existing behaviour.
				switch (Path.GetExtension(filename))
				{
					case ".asax":
					case ".asmx":
					case ".aspx":
					case ".css":
						writer.WriteStartElement("Content");
						isCopyLocalSupported = true;
						writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
						break;

					case ".cs":
						{
							writer.WriteStartElement("Compile");
							writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
							if (srcHash.ContainsKey(relPathKey) || resHash.ContainsKey(relPathKey))
							{
								string baseName = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(filename));
								writer.WriteElementString("DependentUpon", baseName + ".resx");
							}
						}
						break;

					case ".resx":
						writer.WriteStartElement("EmbeddedResource");
						writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
						if (srcHash.ContainsKey(relPathKey) && srcHash[relPathKey] == ".cs")
						{
							writer.WriteElementString("DependentUpon", Path.GetFileNameWithoutExtension(filename) + ".cs");
						}
						else // very likely it's a global resx file
						{
							string designerFileName = filename.Replace("resx", "Designer.cs");
							if (File.Exists(designerFileName))
							{
								isGlobalResx = true;
								linkedFileName = designerFileName;
							}
							else
							{
								// Check for language indexes (like Resources.de.resx):
								var designerDir = Path.GetDirectoryName(filename);
								designerFileName = Path.GetFileNameWithoutExtension(filename);
								langExt = Path.GetExtension(designerFileName);
								// Not very clean. I don't know if this is language extension or just file name with dot.
								if (!String.IsNullOrEmpty(langExt))
								{
									designerFileName = Path.Combine(designerDir, Path.GetFileNameWithoutExtension(designerFileName) + ".Designer.cs");
									if (File.Exists(designerFileName))
									{
										isGlobalResx = true;
										linkedFileName = designerFileName;
									}
								}
							}
							if (isGlobalResx && Version.StrCompareVersions("10.00") >= 0)
							{
								writer.WriteElementString("Generator", "ResXFileCodeGenerator");
								writer.WriteElementString("LastGenOutput", Path.GetFileName(designerFileName));
								writer.WriteElementString("SubType", _savedCsprojData.GetSubTypeMapping("EmbeddedResource", relPath, "Designer"));
							}
						}
						break;

					case ".layout":
					case ".rules":
						writer.WriteStartElement("EmbeddedResource");
						writer.WriteAttributeString("Include", GetProjectPath(fe.Path));

						string hashedExt;
						if (srcHash.TryGetValue(relPathKey, out hashedExt))
						{
							if (hashedExt == ".cs")
								writer.WriteElementString("DependentUpon", Path.GetFileNameWithoutExtension(filename) + hashedExt);
						}
						else if (xomlHash.TryGetValue(relPathKey, out hashedExt))
						{
							writer.WriteElementString("DependentUpon", Path.ChangeExtension(GetProjectPath(fe.Path), ".xoml"));
						}
						break;

					case ".settings":

						var flags = GetFileEntryFlags(fe);
						if (flags != null && flags.IsKindOf(FileData.ContentFile))
						{
							writer.WriteStartElement("Content");
							isCopyLocalSupported = true;
						}
						else
						{
							writer.WriteStartElement("None");
						}

						writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
						writer.WriteElementString("Generator", GetFileEntryOptionOrDefault(fe, "Generator", "SettingsSingleFileGenerator"));
						writer.WriteElementString("LastGenOutput", Path.GetFileNameWithoutExtension(filename) + ".Designer.cs");
						break;

					case ".config":

						var flags1 = GetFileEntryFlags(fe);
						if (flags1 != null && flags1.IsKindOf(FileData.ContentFile))
						{
							writer.WriteStartElement("Content");
							isCopyLocalSupported = true;
						}
						else
						{
							writer.WriteStartElement("None");
						}

						if (fe.Path.Path.EndsWith("Web.config"))
						{
							isWebConfig = true;
							_webConfigPath = GetProjectPath(fe.Path);
							writer.WriteAttributeString("Include", "Web.config");
						}
						else
						{
							writer.WriteAttributeString("Include", GetProjectPath(fe.Path));
						}

						break;

					case ".tt":

						var genAction = "Compile"; // "Content"
						var genExt = ".cs";
						OptionSet fileoptions;

						ParseTextTemplate(fe, out genExt, out genAction, out fileoptions, out addttService);

						var genFile = Path.ChangeExtension(filename, genExt);

						// Write generated file, it should be placed next to the template file

						writer.WriteStartElement(genAction);
						isCopyLocalSupported = genAction == "Content";
						writer.WriteAttributeString("Include", genFile);

						writer.WriteElementString("AutoGen", "True");
						writer.WriteElementString("DesignTime", "True");
						writer.WriteElementString("DependentUpon", Path.GetFileName(filename));
						WriteToolProperties(writer, fileoptions);

						// Write link
						bool needLink = !String.IsNullOrEmpty(fe.FileSetBaseDir) && fe.Path.Path.StartsWith(fe.FileSetBaseDir);
						if (needLink)
						{
							if (!fe.Path.IsPathInDirectory(OutputDir))
							{
								var linkPath = GetLinkPath(fe);
								if (!String.IsNullOrEmpty(linkPath))
								{
									writer.WriteNonEmptyElementString("Link", Path.ChangeExtension(linkPath, genExt));
								}
							}
						}

						writer.WriteEndElement();

						// Write template file

						writer.WriteStartElement("None");
						writer.WriteAttributeString("Include", GetProjectPath(fe.Path));

						writer.WriteElementString("Generator", "TextTemplatingFileGenerator");
						writer.WriteElementString("LastGenOutput", Path.GetFileName(genFile));

						break;

					default:

						string defaultResourceElement = "Resource";
						var fileFlags = GetFileEntryFlags(fe);
						if (fileFlags != null && fileFlags.IsKindOf(FileData.EmbeddedResource))
						{
							defaultResourceElement = "EmbeddedResource";
						}
						else if (fileFlags != null && fileFlags.IsKindOf(FileData.ContentFile))
						{
							defaultResourceElement = "Content";
						}
						else
						{
							// Resource is a WPF concept, so ensure the project gets tagged as UseWpf
							// See: https://docs.microsoft.com/en-us/dotnet/desktop/wpf/app-development/wpf-application-resource-content-and-data-files?view=netframeworkdesktop-4.8
							requiresWpf = true;
						}
						isCopyLocalSupported = true;
						writer.WriteStartElement(defaultResourceElement);
						writer.WriteAttributeString("Include", GetProjectPath(fe.Path));

						break;
				}
			}

			bool linkResources = !String.IsNullOrEmpty(fe.FileSetBaseDir) && fe.Path.Path.StartsWith(fe.FileSetBaseDir) && !isWebConfig;
			if (linkResources)
			{
				if (!isGlobalResx)
				{
					WriteLinkPath(writer, fe);
				}
				else if (fe.Path.IsPathInDirectory(PathString.MakeNormalized(fe.FileSetBaseDir)))
				{
					if (Path.GetExtension(filename) == ".resx")
					{
						if (linkedFileName == null)
							linkedFileName = filename.Replace("resx", "Designer.cs");

						string linkPath = ConvertNamespaceToLinkPath(linkedFileName);
						if (linkPath == null)
						{
							// Sometimes language specific resources have corresponding language specific Designer.cs which is empty. In this case try generic
							var designerFileName = Path.GetFileNameWithoutExtension(filename);
							langExt = Path.GetExtension(designerFileName);
							if (!String.IsNullOrEmpty(langExt))
							{
								designerFileName = Path.Combine(Path.GetDirectoryName(filename), Path.GetFileNameWithoutExtension(designerFileName) + ".Designer.cs");
								linkPath = ConvertNamespaceToLinkPath(designerFileName);
							}
						}
						if (linkPath != null)
							writer.WriteElementString("Link", linkPath + langExt + ".resx");
					}
				}
			}

			// if we're writing a content file, append custom copy information
			if (isCopyLocalSupported && fe.CopyLocalOperations.Any())
			{
				Dictionary<string, string> copyMetaData = GetCopyLocalFilesMetaData(fe.Path.Path, fe.CopyLocalOperations);
				foreach (KeyValuePair<string, string> metaData in copyMetaData.OrderBy(meta => meta.Key))
				{
					writer.WriteElementString(metaData.Key, metaData.Value);
				}
				HandledCopyLocalOperations.AddRange(fe.CopyLocalOperations);
			}

			writer.WriteEndElement();   // None/Content/EmbeddedResource/etc
		}

		private BitMask GetFileEntryFlags(FileEntry fe)
		{
			var fileitem = fe.ConfigEntries.First().FileItem;
			return fileitem.Data as BitMask;
		}

		private void WriteToolProperties(IXmlWriter writer, OptionSet options)
		{
			if (options != null)
			{
				IDictionary<string, string> toolProperties = options.GetOptionDictionary("visual-studio-tool-properties");
				if (toolProperties != null)
				{
					foreach (var prop in toolProperties)
					{
						writer.WriteElementString(prop.Key, prop.Value);
					}
				}
			}
		}

		private void WriteToolAttributes(IXmlWriter writer, OptionSet options)
		{
			if (options != null)
			{
				IDictionary<string, string> toolProperties = options.GetOptionDictionary("visual-studio-tool-attributes");
				if (toolProperties != null)
				{
					foreach (var prop in toolProperties)
					{
						writer.WriteAttributeString(prop.Key, prop.Value);
					}
				}
			}
		}

		//IMTODO: complete functionality for link.resourcefiles.nonembed
		private bool LinkResourcesNonEmbed(IModule module)
		{
			return ConvertUtil.ToBooleanOrDefault(module.Package.Project.Properties[module.GroupName + ".csproj.link.resourcefiles.nonembed"], true);
		}

		private void WriteSourceFile(IXmlWriter writer, FileEntry fe, string projectPath, string relPathKey, IDictionary<string, string> resHash, IDictionary<string, string> srcHash, IDictionary<string, string> xomlHash, IDictionary<string, string> xamlHash, IDictionary<string, string> ttHash)
		{
			string filename = fe.Path.Path;
			string codeBehind = Path.GetFileNameWithoutExtension(filename);
			string codeBehindExtension = Path.GetExtension(codeBehind);
			string codeBehindKey = relPathKey.TrimExtension();

			if (ttHash.ContainsKey(relPathKey))
			{
				// If src files picked up generated code behind key we need to skip this source files as it is added to the project in the resources section.
				return;
			}

			writer.WriteStartElement("Compile");
			writer.WriteAttributeString("Include", GetProjectPath(fe.Path));

			bool isGlobalResource = false;


			string subType = null;
			string dependentUpon = null;

			if (xamlHash.ContainsKey(codeBehindKey) && codeBehindExtension == xamlHash[codeBehindKey])
			{
				subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "Code");
				dependentUpon = codeBehind;
			}
			else if (xomlHash.ContainsKey(codeBehindKey) && codeBehindExtension == xomlHash[codeBehindKey])
			{
				subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "Component");
				dependentUpon = codeBehind;
			}
			else if (resHash.ContainsKey(codeBehindKey.TrimExtension()))
			{
				if (codeBehindExtension == ".asax")
				{
					subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "Code");
					dependentUpon = codeBehind;
				}
				else if (codeBehindExtension == ".asmx")
				{
					subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "Component");
					dependentUpon = codeBehind;
				}
				else if (codeBehindExtension == ".aspx")
				{
					subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "ASPXCodeBehind");
					dependentUpon = codeBehind;
				}
				else
				{
					subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "Code");
				}
			}
			else if (resHash.ContainsKey(relPathKey))
			{
				string resourceExts;
				if (resHash.TryGetValue(relPathKey, out resourceExts))
				{
					//IM: I make this code after nantToVSTools, but I'm not sure if subtype=Designer is applicable to .cs files here?
					subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "Designer", allowRemove: true);
				}
				else
				{
					subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "Code");
				}
			}
			else
			{
				subType = _savedCsprojData.GetSubTypeMapping("Compile", projectPath, "Code");
			}

			if (filename.ToLower().EndsWith(".designer.cs"))
			{
				int index = codeBehind.ToLower().LastIndexOf(".designer");

				string partial = codeBehind.Substring(0, index);
				string partialKey = relPathKey.Substring(0, relPathKey.ToLower().LastIndexOf(".designer"));
				string ext;

				if (srcHash.TryGetValue(partialKey, out ext) && ext == ".cs")
				{
					if (partial.EndsWith(".aspx", StringComparison.OrdinalIgnoreCase))
					{
						// VisualStudio 2010 defaults to having the <DependentUpon> element
						// ${sourcefile}.aspx.designer.cs be set to ${sourcefile}.aspx
						dependentUpon = partial;
					}
					else
					{
						dependentUpon = partial + ".cs";
					}
				}
				else
				{
					// nedd to use full path in file exists here because relative path will be evaluated against current dir, not the project OutputDir
					var dependentUponProbe = filename.Substring(0, filename.ToLower().LastIndexOf(".designer"));
					if (File.Exists(dependentUponProbe + ".resx"))
					{
						dependentUpon = partial + ".resx";
						isGlobalResource = true;
					}
					else if (File.Exists(dependentUponProbe + ".settings"))
					{
						dependentUpon = partial + ".settings";
					}
				}
			}

			// figure out subtype for each configuration using "subtype" option from optionset, or all back to subtype calculated above then write out
			// subtype metadata with correct config condition - in the vast majority of cases this is will collapse to a single subtype value with no
			// conditions
			Func<FileEntry.ConfigFileEntry, string> getSubType = (ce) =>
			{
				if (ce.FileItem.OptionSetName != null)
				{
					return ce.Project.NamedOptionSets[ce.FileItem.OptionSetName].GetOptionOrDefault("subtype", subType);
				}
				return subType;
			};
			foreach (IGrouping<string, FileEntry.ConfigFileEntry> subTypeToEntries in fe.ConfigEntries.GroupBy(ce => getSubType(ce)))
			{
				WriteElementStringWithConfigCondition(writer, "SubType", subTypeToEntries.Key, GetConfigCondition(subTypeToEntries.Select(ce => ce.Configuration)));
			}

			writer.WriteNonEmptyElementString("DependentUpon", dependentUpon);

			if (!fe.Path.IsPathInDirectory(OutputDir))
			{
				if (isGlobalResource)
				{
					string linkPath = ConvertNamespaceToLinkPath(filename);
					if (linkPath != null)
						writer.WriteElementString("Link", linkPath + ".Designer.cs");
				}
				else
				{
					// The source is outside the csproj directory.  Link to it, based on the baseDir of the fileSet
					WriteLinkPath(writer, fe);
				}
			}

			writer.WriteEndElement(); //Compile
		}

		/// <summary>
		/// Since LinkPath (the Link tag) is used by VS05 to construct the name of the global resources, so we need
		/// to make the link path same as the namespace structure of the global resource designer file.
		/// For example:
		/// Resources.Designer.cs: namespace DefaultNameSpace.Properties { class Resources ... }
		/// LinkPath (designer): should be Properties\\Resources.Designer.cs (DefaultNameSpace is omitted)
		/// LinkPath (resx)    : should be Properties\\Resources.resx
		/// </summary>
		/// <param name="filename"></param>
		/// <returns></returns>
		protected string ConvertNamespaceToLinkPath(string filename)
		{
			string retnamespace = null;
			string retclassname = null;
			using (StreamReader sr = File.OpenText(filename))
			{
				while (sr.Peek() > -1)
				{
					string str = sr.ReadLine();
					string matchnamespace = @"namespace ((\w+.)*)";
					string matchnamespaceCaps = @"Namespace ((\w+.)*)";
					string matchclassname = @"typeof\((\w+.)\).Assembly";
					Regex matchNamespaceRE = new Regex(matchnamespace);
					Regex matchNamespaceCapsRE = new Regex(matchnamespaceCaps);
					Regex matchClassnameRE = new Regex(matchclassname);

					if (matchNamespaceRE.Match(str).Success)
					{
						Match namematch = matchNamespaceRE.Match(str);
						retnamespace = namematch.Groups[1].Value;
						retnamespace = retnamespace.Replace("{", "");
						retnamespace = retnamespace.Trim();
					}
					else if (matchNamespaceCapsRE.Match(str).Success)
					{
						Match namematch = matchNamespaceCapsRE.Match(str);
						retnamespace = namematch.Groups[1].Value;
						retnamespace = retnamespace.Trim();
					}
					else if (matchClassnameRE.Match(str).Success)
					{
						Match namematch = matchClassnameRE.Match(str);
						retclassname = namematch.Groups[1].Value;
						retclassname = retclassname.Trim();
						break;
					}
				}
			}
			if (retclassname != null && retnamespace != null)
			{
				string namespaceLink = retnamespace.Replace(RootNamespace, "");
				namespaceLink = namespaceLink.Replace(".", " ");
				namespaceLink = namespaceLink.Trim().Replace(" ", "\\");
				if (!String.IsNullOrEmpty(namespaceLink)) // Avoid an unnecessary leading backslash causing generated resource name to be incorrect.
					return namespaceLink + "\\" + retclassname;
				else
					return retclassname;
			}
			return null;
		}

		protected void OnResponceFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} response file  '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
					Log.Minimal.WriteLine(message);
				else
					Log.Status.WriteLine(message);
			}
		}

		protected virtual string CreateResponseFileIfNeeded(Module_DotNet module)
		{
			string responsefile = null;

			if (!String.IsNullOrWhiteSpace(module.Compiler.AdditionalArguments))
			{
				// we need to write response file to contain these additional command line arguments.
				using (IMakeWriter writer = new MakeWriter(writeBOM: false))
				{
					writer.FileName = responsefile = Path.Combine(Package.PackageConfigBuildDir.Path, Name + "_" + module.Configuration.Name + ".rsp");

					writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnResponceFileUpdate);

					writer.WriteLine(module.Compiler.AdditionalArguments);
				}
			}

			return responsefile;
		}

		private void ComputeCodeBehindCache(IEnumerable<FileEntry> fentries, out IDictionary<string, string> srcHash, out IDictionary<string, string> resHash, out IDictionary<string, string> xamlHash, out IDictionary<string, string> xomlHash, out IDictionary<string, string> ttHash)
		{
			srcHash = new Dictionary<string, string>();
			resHash = new Dictionary<string, string>();
			xamlHash = new Dictionary<string, string>();
			xomlHash = new Dictionary<string, string>();
			ttHash = new Dictionary<string, string>();

			foreach (var fe in fentries)
			{
				var ext = Path.GetExtension(fe.Path.Path).ToLowerInvariant();
				// The path key need to be all lower case due to the fact that Windows is case insensitve file system and we have people
				// specifying source files and resource files in different case causing code behind code doesn't match up.
				var relPathKey = GetProjectPath(fe.Path.Path.TrimExtension()).ToLowerInvariant();

				var firstFe = fe.ConfigEntries.First();

				if (firstFe.IsKindOf(Source))
				{
					try
					{
						if (ext == ".xaml")
						{
							xamlHash.Add(relPathKey, ext);

							firstFe.ClearType(Source);
							firstFe.SetType(Xaml);
						}
						else if (ext == ".xoml")
						{
							xamlHash.Add(relPathKey, ext);
							firstFe.ClearType(Source);
							firstFe.SetType(Xoml);
						}
						else if (ext == ".tt")
						{
							//Text templates.Mark them as resource files because they are processed in WriteResource() method.
							firstFe.ClearType(Source);
							firstFe.SetType(Resource);
							ttHash.Add(relPathKey, ext);
						}
						else
						{
							srcHash.Add(relPathKey, ext);
						}
					}
					catch (ArgumentException)
					{
						string oldext;
						string type = "source";
						if (!srcHash.TryGetValue(relPathKey, out oldext))
						{
							type = "xaml";
							if (!xamlHash.TryGetValue(relPathKey, out oldext))
							{
								type = null;
							}
						}
						var msg = String.Format("Duplicate {0} hash key '{1}' for files with extension '{2}' and '{3}'. Likely one of these files is defined in a wrong fileset.", type ?? String.Empty, relPathKey, ext, oldext);
						throw new BuildException(msg);
					}
				}
				else if (firstFe.IsKindOf(Resource))
				{
					if (ext == ".tt")
					{
						ttHash.Add(relPathKey, ext);
					}
					else
					{

						string val = ext;
						if (resHash.TryGetValue(relPathKey, out val))
						{
							val += "\n" + ext;
						}
						else
						{
							val = ext;
						}
						resHash[relPathKey] = val;
					}
				}
			}
		}

		protected virtual void WriteBootstrapperPackageInfo(IXmlWriter writer)
		{
			/*
			writer.WriteStartElement("ItemGroup");
			writer.WriteStartElement("BootstrapperPackage");
			Include="Microsoft.Net.Framework.2.0">
	  <Visible>False</Visible>
	  <ProductName>.NET Framework 2.0 %28x86%29</ProductName>
	  <Install>true</Install>
	</BootstrapperPackage>
	<BootstrapperPackage Include="Microsoft.Net.Framework.3.0">
	  <Visible>False</Visible>
	  <ProductName>.NET Framework 3.0 %28x86%29</ProductName>
	  <Install>false</Install>
	</BootstrapperPackage>
	<BootstrapperPackage Include="Microsoft.Net.Framework.3.5">
	  <Visible>False</Visible>
	  <ProductName>.NET Framework 3.5</ProductName>
	  <Install>false</Install>
	</BootstrapperPackage>
  </ItemGroup>
			*/

		}

		protected virtual void WriteImportStandardTargets(IXmlWriter writer)
		{
			writer.WriteStartElement("Import");
			writer.WriteAttributeString("Project", @"$(MSBuildBinPath)\Microsoft.CSharp.targets");
			writer.WriteEndElement(); //Import
		}

		protected virtual void WriteImportProperties(IXmlWriter writer)
		{
			string filePath = Path.Combine((BuildGenerator.IsPortable) ? BuildGenerator.SlnFrameworkFilesRoot : NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, "data");
			string propsPath = Path.GetFullPath(Path.Combine(Path.Combine(filePath, @"FrameworkMsbuild.props")));
			if (!string.IsNullOrEmpty(writer.FileName) && BuildGenerator.IsPortable)
			{
				propsPath = PathUtil.RelativePath(propsPath, Path.GetDirectoryName(writer.FileName));
			}

			writer.WriteStartElement("Import");
			writer.WriteAttributeString("Project", GetProjectPath(propsPath));
			writer.WriteEndElement(); //Import
		}

		protected virtual void WriteUsingTaskOverrides(IXmlWriter writer)
		{
			if (TargetFrameworkFamily == DotNetFrameworkFamilies.Framework && Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.sln.use-xaml-compile-override", true))
			{
				string filePath = Path.Combine((BuildGenerator.IsPortable) ? BuildGenerator.SlnFrameworkFilesRoot : NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, "bin");
				if (!string.IsNullOrEmpty(writer.FileName) && BuildGenerator.IsPortable)
				{
					filePath = PathUtil.RelativePath(filePath, Path.GetDirectoryName(writer.FileName));
				}

				writer.WriteStartElement("UsingTask");
				writer.WriteAttributeString("TaskName", "Microsoft.Build.Tasks.Windows.MarkupCompilePass1");
				writer.WriteAttributeString("AssemblyFile", Path.Combine(filePath, "EAPresentationBuildTasks.dll"));
				writer.WriteEndElement();

				writer.WriteStartElement("UsingTask");
				writer.WriteAttributeString("TaskName", "Microsoft.Build.Tasks.Windows.MarkupCompilePass2");
				writer.WriteAttributeString("AssemblyFile", Path.Combine(filePath, "EAPresentationBuildTasks.dll"));
				writer.WriteEndElement();

				writer.WriteStartElement("UsingTask");
				writer.WriteAttributeString("TaskName", "Microsoft.Build.Tasks.Windows.ResourcesGenerator");
				writer.WriteAttributeString("AssemblyFile", Path.Combine(filePath, "EAPresentationBuildTasks.dll"));
				writer.WriteEndElement();

				writer.WriteStartElement("UsingTask");
				writer.WriteAttributeString("TaskName", "Microsoft.Build.Tasks.Windows.FileClassifier");
				writer.WriteAttributeString("AssemblyFile", Path.Combine(filePath, "EAPresentationBuildTasks.dll"));
				writer.WriteEndElement();
			}
		}

		protected virtual void WriteImportTargets(IXmlWriter writer)
		{
			if (!UseNewCSProjFormat())
			{
				WriteImportStandardTargets(writer);
			}

			WriteImportFrameworkTargets(writer);

			foreach (string importProject in StartupModule.ImportMSBuildProjects.ToArray())
			{
				writer.WriteStartElement("Import");
				writer.WriteAttributeString("Project", importProject.TrimQuotes());
				writer.WriteEndElement(); //Import
			}

			if (StartupModule.IsProjectType(DotNetProjectTypes.WebApp))
			{
				if (Version == "10.00")
				{
					writer.WriteStartElement("Import");
					writer.WriteAttributeString("Project", @"$(MSBuildExtensionsPath32)\Microsoft\VisualStudio\v10.0\WebApplications\Microsoft.WebApplication.targets");
					writer.WriteEndElement(); //Import
				}
				else
				{
					writer.WriteStartElement("Import");
					writer.WriteAttributeString("Project", @"$(VSToolsPath)\WebApplications\Microsoft.WebApplication.targets");
					writer.WriteAttributeString("Condition", "'$(VSToolsPath)' != ''");
					writer.WriteEndElement(); //Import
				}

				writer.WriteStartElement("ProjectExtensions");
				writer.WriteStartElement("VisualStudio");
				writer.WriteStartElement("FlavorProperties");
				writer.WriteAttributeString("GUID", "{349c5851-65df-11da-9384-00065b846f21}");
				writer.WriteStartElement("WebProjectProperties");

				writer.WriteElementString("UseIIS", "False");
				writer.WriteElementString("AutoAssignPort", "True");
				writer.WriteElementString("DevelopmentServerPort", "53813");
				writer.WriteElementString("DevelopmentServerVPath", "/");
				writer.WriteElementString("IISUrl", "\n");
				writer.WriteElementString("NTLMAuthentication", "False");
				writer.WriteElementString("UseCustomServer", "False");
				writer.WriteElementString("CustomServerUrl", "\n");
				writer.WriteElementString("SaveServerSettingsInUserFile", "False");

				writer.WriteEndElement(); //WebProjectProperties
				writer.WriteEndElement(); //FlavorProperties
				writer.WriteEndElement(); //VisualStudio
				writer.WriteEndElement(); //ProjectExtensions
			}

			if (StartupModule.IsProjectType(DotNetProjectTypes.Workflow))
			{
				writer.WriteStartElement("Import");
				writer.WriteAttributeString("Project", String.Format(@"$(MSBuildExtensionsPath)\Microsoft\Windows Workflow Foundation\{0}\Workflow.Targets", TargetFrameworkVersion));
				writer.WriteEndElement(); //Import
			}

			if (EnableFxCop())
			{
				var customdictionary = Package.Project.Properties.GetPropertyOrDefault(StartupModule.GroupName + ".fxcop.customdictionary",
										Package.Project.Properties.GetPropertyOrDefault("package.fxcop.customdictionary", String.Empty)).TrimWhiteSpace();

				if (!String.IsNullOrEmpty(customdictionary))
				{
					customdictionary = PathNormalizer.Normalize(customdictionary);

					writer.WriteStartElement("ItemGroup");
					writer.WriteStartElement("CodeAnalysisDictionary");
					writer.WriteAttributeString("Include", GetProjectPath(customdictionary));

					writer.WriteElementString("Link", Path.GetFileName(customdictionary));

					writer.WriteEndElement();//CodeAnalysisDictionary
					writer.WriteEndElement();//ItemGroup
				}
			}

			if (EnableStyleCop())
			{
				TaskUtil.Dependent(StartupModule.Package.Project, "StyleCop", Project.TargetStyleType.Use);

				writer.WriteStartElement("Import");
				writer.WriteAttributeString("Project", StartupModule.Package.Project.Properties["package.StyleCop.MSBuildTargets"]);
				writer.WriteEndElement(); //Import
			}
		}

		private void WriteImportFrameworkTargets(IXmlWriter writer)
		{
			string filePath = Path.Combine((BuildGenerator.IsPortable) ? BuildGenerator.SlnFrameworkFilesRoot : NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, "data");
			string taskPath = Path.GetFullPath(Path.Combine(filePath, "FrameworkMsbuild.tasks"));
			if (!string.IsNullOrEmpty(writer.FileName) && BuildGenerator.IsPortable)
			{
				taskPath = PathUtil.RelativePath(taskPath, Path.GetDirectoryName(writer.FileName));
			}
			writer.WriteStartElement("Import");
			writer.WriteAttributeString("Project", GetProjectPath(taskPath));
			writer.WriteEndElement(); //Import
		}

		protected void WriteWebConfigCopyStep(IXmlWriter writer)
		{
			if (_webConfigPath != null)
			{
				writer.WriteStartElement("Target");
				if (!UseNewCSProjFormat())
				{
					writer.WriteAttributeString("Name", "BeforeBuild");
				}
				else
				{
					writer.WriteAttributeString("Name", "WebConfigBeforeBuild");
					writer.WriteAttributeString("BeforeTargets", "BeforeBuild");
				}

				writer.WriteStartElement("FrameworkCopyWithAttributes");
				writer.WriteAttributeString("SourceFiles", _webConfigPath);
				writer.WriteAttributeString("DestinationFiles", "Web.config");
				writer.WriteAttributeString("SkipUnchangedFiles", "true");
				writer.WriteAttributeString("UseHardlinksIfPossible", "true");
				writer.WriteAttributeString("UseSymlinksIfPossible", "false");
				writer.WriteAttributeString("ProjectPath", "$(MSBuildProjectDirectory)");
				writer.WriteEndElement(); // FrameworkCopyWithAttributes
				writer.WriteEndElement(); //Target

				writer.WriteStartElement("Target");
				if (!UseNewCSProjFormat())
				{
					writer.WriteAttributeString("Name", "BeforeClean");
				}
				else
				{
					writer.WriteAttributeString("Name", "WebConfigBeforeClean");
					writer.WriteAttributeString("BeforeTargets", "BeforeClean");
				}

				writer.WriteStartElement("Delete");
				writer.WriteAttributeString("Files", "Web.config");
				writer.WriteAttributeString("TreatErrorsAsWarnings", "true");
				writer.WriteEndElement(); //Copy

				writer.WriteEndElement(); //Target
			}
		}

		protected override void WriteUserFile()
		{
			if (null != Modules.Cast<ProcessableModule>().FirstOrDefault(m => m.AdditionalData != null && m.AdditionalData.DebugData != null))
			{
				string userFilePath = Path.Combine(OutputDir.Path, UserFileName);
				var userFileDoc = new XmlDocument();

				try
				{
					if (File.Exists(userFilePath))
					{
						userFileDoc.Load(userFilePath);
					}
				}
				catch (Exception ex)
				{
					Log.Warning.WriteLine("Failed to load existing '.user' file '{0}': {1}. Recreating user file.", UserFileName, ex.Message);
					userFileDoc = new XmlDocument();
				}

				var userFileEl = userFileDoc.GetOrAddElement("Project", Xmlns);
				userFileEl.SetAttribute("ToolsVersion", ToolsVersion);

				foreach (ProcessableModule module in Modules)
				{
					WriteUserFileConfiguration(userFileEl, module);
				}

				using (MakeWriter writer = new MakeWriter())
				{
					writer.CacheFlushed += new CachedWriterEventHandler(OnProjectFileUpdate);
					writer.FileName = userFilePath;
					writer.Write(userFileDoc.OuterXml);
				}
			}
		}

		protected void WriteUserFileConfiguration(XmlNode rootEl, ProcessableModule module)
		{
			if (module.AdditionalData != null && module.AdditionalData.DebugData != null)
			{
				var configCondition = String.Format("'$(Configuration)|$(Platform)' == '{0}'", GetVSProjConfigurationName(module.Configuration));
				var propertyGroupEl = rootEl.GetOrAddElementWithAttributes("PropertyGroup", "Condition", configCondition);

				if (module.AdditionalData.DebugData.EnableUnmanagedDebugging)
				{
					var unmanagedDebuggingEl = propertyGroupEl.GetOrAddElement("EnableUnmanagedDebugging");
					unmanagedDebuggingEl.InnerText = module.AdditionalData.DebugData.EnableUnmanagedDebugging.ToString();
				}

				var workingDirEl = propertyGroupEl.GetOrAddElement("StartWorkingDirectory");
				workingDirEl.InnerText = GetProjectPath(module.AdditionalData.DebugData.Command.WorkingDir);

				if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.RemoteMachine))
				{
					var remoteDebugEnabledEl = propertyGroupEl.GetOrAddElement("RemoteDebugEnabled");
					remoteDebugEnabledEl.InnerText = "true";

					var remoteDebugMachineEl = propertyGroupEl.GetOrAddElement("RemoteDebugMachine");
					remoteDebugMachineEl.InnerText = module.AdditionalData.DebugData.RemoteMachine;
				}

				var startArgsEl = propertyGroupEl.GetOrAddElement("StartArguments");
				startArgsEl.InnerText = module.AdditionalData.DebugData.Command.Options.ToString(" ");

				if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.Command.Executable.Path))
				{
					var startActionEl = propertyGroupEl.GetOrAddElement("StartAction");
					startActionEl.InnerText = "Program";

					var startProgramEl = propertyGroupEl.GetOrAddElement("StartProgram");
					startProgramEl.InnerText = GetProjectPath(module.AdditionalData.DebugData.Command.Executable);
				}
				else
				{
					var startActionEl = propertyGroupEl.GetOrAddElement("StartAction");
					startActionEl.InnerText = "Project";
				}
			}
		}

		#endregion Write Methods

		protected override void AddModuleGenerators(IModule module)
		{
			if (module.Package.Project.Properties.GetBooleanPropertyOrDefault("visualstudio.hide.unsupported.csharp", false))
			{
				// when hiding csharp modules we need to check all of the configs to see if there are any supported configs
				foreach (Configuration config in AllConfigurations)
				{
					if (config.System == "pc" || config.System == "pc64")
					{
						if (!BuildGenerator.ModuleGenerators.ContainsKey(Key))
						{
							BuildGenerator.ModuleGenerators.Add(Key, this);
						}
					}
				}
			}
			else
			{
				base.AddModuleGenerators(module);
			}
		}

		protected void ExecuteExtensions(IModule module, Action<IDotNetVisualStudioExtension> action)
		{
			foreach (var ext in GetExtensionTasks(module as ProcessableModule))
			{
				action(ext);
			}
		}

		protected IEnumerable<IDotNetVisualStudioExtension> GetExtensionTasks(ProcessableModule module)
		{
			List<IDotNetVisualStudioExtension> extensions = new List<IDotNetVisualStudioExtension>();
			if (module == null)
			{
				return extensions;
			}
			// Dynamically created packages may use existing projects from different packages. Skip such projects
			if (module.Configuration.Name != module.Package.Project.Properties["config"])
			{
				return extensions;
			}

			var extensionTaskNames = new List<string>() { module.Configuration.Platform + "-visualstudio-extension" };

			extensionTaskNames.AddRange(module.Package.Project.Properties[module.Configuration.Platform + "-visualstudio-extension"].ToArray());

			extensionTaskNames.AddRange(module.Package.Project.Properties[module.GroupName + ".visualstudio-extension"].ToArray());

			foreach (var extensionTaskName in extensionTaskNames)
			{
				Task task = module.Package.Project.TaskFactory.CreateTask(extensionTaskName, module.Package.Project);
				if (task is IDotNetVisualStudioExtension extension)
				{
					extension.Module = module;
					extension.Generator = this;
					extensions.Add(extension);
				}
				else if (task != null && !(task is IMCPPVisualStudioExtension))
				{
					Log.Warning.WriteLine("Visual Studio generator extension Task '{0}' does not implement IDotNetVisualStudioExtension. Task is ignored.", extensionTaskName);
				}

			}
			return extensions;
		}

		protected virtual DotNetCompiler.Targets OutputType
		{
			get
			{
				var targets = Modules.Cast<Module_DotNet>().Select(m => m.Compiler.Target).Distinct();

				if (targets.Count() > 1)
				{
					throw new BuildException(String.Format("DotNet Module '{0}' has different targets ({1}) defined in different configurations. VisualStudio does not support this feature.", ModuleName, targets.ToString(", ", t => t.ToString())));
				}
				return targets.First();
			}
		}

		private void WriteConfigCondition(IXmlWriter writer, IEnumerable<FileEntry> files)
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

		internal virtual string GetLinkPath(FileEntry fe)
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

		protected virtual string GetConfigPlatformMapping(Module_DotNet module)
		{
			var platform = module.Compiler.GetOptionValue("platform:");

			switch (platform)
			{
				case "x86": return "Win32";
				case "x64": return "x64";
				default:
				case "anycpu": return "AnyCPU";
			}
		}

		protected virtual IEnumerable<BuildstepGroup> GroupBuildSteps()
		{
			var groups = new Dictionary<string, BuildstepGroup>();

			foreach (var m in Modules)
			{
				var module = (Module)m;
				var cmd = GetBuildStepsCommands(module);
				if (cmd != null)
				{
					var key = cmd.Aggregate(new StringBuilder(), (k, cm) => k.AppendLine(cm.Cmd)).ToString();

					BuildstepGroup bsgroup;

					if (!groups.TryGetValue(key, out bsgroup))
					{
						bsgroup = new BuildstepGroup(cmd);
						groups.Add(key, bsgroup);
					}
					if (!bsgroup.Configurations.Contains(module.Configuration))
					{
						bsgroup.Configurations.Add(module.Configuration);
					}
				}
			}

			return groups.Values;
		}


		IList<BuildStepCmd> GetBuildStepsCommands(Module module)
		{
			List<BuildStepCmd> commands = null;

			foreach (var buildstep in module.BuildSteps)
			{
				var cmd = GetBuildStepCommand(module, buildstep);
				if (cmd != null)
				{
					if (commands == null)
						commands = new List<BuildStepCmd>();
					commands.Add(cmd);
				}
			}
			return commands;
		}

		private BuildStepCmd GetBuildStepCommand(IModule module, BuildStep buildstep)
		{
			var cmd = buildstep.Commands.Aggregate(new StringBuilder(), (sbx, c) => sbx.AppendLine(c.CommandLine)).ToString();

			if (BuildGenerator.IsPortable)
			{
				cmd = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(cmd, OutputDir.Path);
			}


			if (buildstep.Commands.Count == 0 && buildstep.TargetCommands.Count > 0)
			{
				var sb = new StringBuilder();
				foreach (var targetCommand in buildstep.TargetCommands)
				{
					if (!targetCommand.NativeNantOnly)
					{
						Func<string, string> normalizeFunc = null;
						if (BuildGenerator.IsPortable)
						{
							normalizeFunc = (X) => BuildGenerator.PortableData.NormalizeIfPathString(X, OutputDir.Path);
						}
						sb.AppendLine(NantInvocationProperties.TargetToNantCommand(module, targetCommand, addGlobal: true, normalizePathString: normalizeFunc, isPortable: BuildGenerator.IsPortable));
					}
				}
				cmd = sb.ToString();
			}
			cmd = cmd.TrimWhiteSpace();

			if (!String.IsNullOrEmpty(cmd))
			{
				return new BuildStepCmd(module.Configuration, buildstep, cmd);
			}
			return null;
		}

		protected class BuildStepCmd
		{
			internal BuildStepCmd(Configuration config, BuildStep buildstep, string cmd)
			{
				Config = config;
				BuildStep = buildstep;
				Cmd = cmd;
			}
			internal readonly Configuration Config;
			internal readonly BuildStep BuildStep;
			internal readonly string Cmd;
		}


		protected class BuildstepGroup
		{
			internal BuildstepGroup(IEnumerable<BuildStepCmd> buildsteps)
			{
				BuildSteps = buildsteps;
				Configurations = new List<Configuration>();
			}
			internal readonly IEnumerable<BuildStepCmd> BuildSteps;
			internal List<Configuration> Configurations;
			internal string ConfigSignature
			{
				get
				{
					if (_configSignature == null)
					{
						_configSignature = Configurations.Aggregate(new StringBuilder(), (s, c) => s.AppendLine(c.Name)).ToString();
					}
					return _configSignature;
				}
			}
			private string _configSignature;
		}

		internal virtual IDictionary<string, IList<Tuple<ProjectRefEntry, uint>>> GroupProjectReferencesByConfigConditions(IEnumerable<ProjectRefEntry> projectReferences)
		{
			var groups = new Dictionary<string, IList<Tuple<ProjectRefEntry, uint>>>();

			foreach (var projRef in projectReferences)
			{
				foreach (var group in projRef.ConfigEntries.GroupBy(pr => pr.Type))
				{
					if (group.Count() > 0)
					{
						var condition = GetConfigCondition(group.Select(ce => ce.Configuration));

						IList<Tuple<ProjectRefEntry, uint>> configEntries;

						if (!groups.TryGetValue(condition, out configEntries))
						{
							configEntries = new List<Tuple<ProjectRefEntry, uint>>();
							groups.Add(condition, configEntries);
						}
						configEntries.Add(Tuple.Create(projRef, group.First().Type));
					}
				}
			}
			return groups;
		}

		private void GetReferences(out IEnumerable<ProjectRefEntry> projectReferences, out IDictionary<PathString, FileEntry> references)
		{
			var projectRefs = new List<ProjectRefEntry>();
			projectReferences = projectRefs;

			// Collect project referenced assemblies by configuration:
			var projectReferencedAssemblies = new Dictionary<Configuration, ISet<PathString>>();

			foreach (VSProjectBase depProject in Dependents)
			{
				var refEntry = new ProjectRefEntry(depProject);
				projectRefs.Add(refEntry);

				foreach (IModule module in Modules)
				{
					// Find dependent project configuration that corresponds to this project config:
					IEnumerable<KeyValuePair<Configuration, Configuration>> mapEntry = ConfigurationMap.Where(e => e.Value == module.Configuration);
					if (mapEntry.Count() > 0 && ConfigurationMap.TryGetValue(mapEntry.First().Key, out Configuration dependentConfig))
					{
						IModule depModule = depProject.Modules.SingleOrDefault(m => m.Configuration == dependentConfig);
						if (depModule != null)
						{
							PathString assembly = null;
							if (depModule.IsKindOf(Module.DotNet))
							{
								if (depModule is Module_DotNet dotNetModule)
								{
									assembly = PathString.MakeCombinedAndNormalized(dotNetModule.OutputDir.Path, dotNetModule.Compiler.OutputName + dotNetModule.Compiler.OutputExtension);
								}
								else if (depModule is Module_ExternalVisualStudio externalDotNetModule)
								{
									assembly = externalDotNetModule.VisualStudioProjectOutput;
								}
							}
							else if (depModule.IsKindOf(Module.Managed))
							{
								if (depModule is Module_Native nativeModule && nativeModule.Link != null)
								{
									assembly = PathString.MakeCombinedAndNormalized(nativeModule.Link.LinkOutputDir.Path, nativeModule.Link.OutputName + nativeModule.Link.OutputExtension);
								}
							}

							if (assembly != null)
							{
								if (!projectReferencedAssemblies.TryGetValue(module.Configuration, out ISet<PathString> assemblies))
								{
									assemblies = new HashSet<PathString>();
									projectReferencedAssemblies.Add(module.Configuration, assemblies);
								}

								assemblies.Add(assembly);
							}

							Dependency<IModule> moduleDependency = module.Dependents.FirstOrDefault(d => d.Dependent.Key == depModule.Key);
							uint flags = 0;
							if ((depModule.IsKindOf(Module.DotNet) || depModule.IsKindOf(Module.Managed)) && moduleDependency != null && moduleDependency.IsKindOf(DependencyTypes.Link))
							{
								flags |= ProjectRefEntry.ReferenceOutputAssembly;
							}

							refEntry.TryAddConfigEntry(module.Configuration, flags);
						}
					}
				}
			}

			references = new Dictionary<PathString, FileEntry>();

			foreach (var module in Modules.Cast<Module_DotNet>())
			{
				bool usePackageReferences = module.Package.Project.Properties.GetBooleanPropertyOrDefault(Project.NANT_PROPERTY_USE_VS_PACKAGE_REFERENCES, false);

				ISet<PathString> assemblies = null;
				projectReferencedAssemblies.TryGetValue(module.Configuration, out assemblies);

				foreach (var assembly in GetDefaultSystemAssemblies(module))
				{
					var key = new PathString(assembly);
					UpdateFileEntry(module, references, key, new FileItem(key, asIs: true), String.Empty, module.Configuration, flags: 0);
				}

				bool isDotNetCoreModule = module.TargetFrameworkFamily == DotNetFrameworkFamilies.Core;
				foreach (FileItem assembly in module.Compiler.Assemblies.FileItems)
				{
					// exclude any assemblies that will be implicitly added by PackageReference when 
					// package reference is envaled
					if (usePackageReferences)
					{
						if (assembly.OptionSetName != null)
						{
							OptionSet assemblyOptions = module.Package.Project.NamedOptionSets[assembly.OptionSetName];
							if (assemblyOptions?.Options["nuget-reference-assembly"] == "true")
							{
								continue;
							}
						}
					}

					// Exclude any assembly that is already included through project references:
					if (assemblies != null && assemblies.Contains(assembly.Path))
					{
						continue;
					}

					if (isDotNetCoreModule && assembly.AsIs && !Path.IsPathRooted(assembly.Path.Path))
					{
						// In .Net Core Solution build, it doesn't like you explicitly specify system assemblies or you will get
						// a lot of build warnings.  You are supposed to setup the proper SDK setting and MSBuild seems to 
						// just add ALL the system assemblies for you in that SDK.
						continue;
					}

					uint copyLocal = 0;
					if (module.CopyLocal == CopyLocalType.True && !assembly.AsIs)
					{
						copyLocal = FileEntry.CopyLocal;
					}

					var assemblyCopyLocal = assembly.GetCopyLocal(module);
					if (assemblyCopyLocal == CopyLocalType.True)
					{
						copyLocal = FileEntry.CopyLocal;
					}
					else if (assemblyCopyLocal == CopyLocalType.False)
					{
						copyLocal = 0;
					}

					var key = assembly.Path;    // In the above code at beginning of the loop, we clearly used the full path as the key.
					UpdateFileEntry(module, references, key, assembly, module.Compiler.Assemblies.BaseDirectory, module.Configuration, flags: copyLocal);
				}

				foreach (var assembly in module.Compiler.DependentAssemblies.FileItems)
				{
					// Exclude any assembly that is already included through project references:
					if (assemblies != null && assemblies.Contains(assembly.Path))
					{
						continue;
					}

					if (isDotNetCoreModule && assembly.AsIs && !Path.IsPathRooted(assembly.Path.Path))
					{
						// In .Net Core Solution build, it doesn't like you explicitly specify system assemblies or you will get
						// a lot of build warnings.  You are supposed to setup the proper SDK setting and MSBuild seems to 
						// just add ALL the system assemblies for you in that SDK.
						continue;
					}

					uint copyLocal = 0;
					if ((module.CopyLocal == CopyLocalType.True || module.CopyLocal == CopyLocalType.Slim) && !assembly.AsIs)
					{
						copyLocal = FileEntry.CopyLocal;
					}

					var assemblyCopyLocal = assembly.GetCopyLocal(module);
					if (assemblyCopyLocal == CopyLocalType.True)
					{
						copyLocal = FileEntry.CopyLocal;
					}
					else if (assemblyCopyLocal == CopyLocalType.False)
					{
						copyLocal = 0;
					}

					var key = assembly.Path;    // In the above code at beginning of the loop, we clearly used the full path as the key.
					UpdateFileEntry(module, references, key, assembly, module.Compiler.DependentAssemblies.BaseDirectory, module.Configuration, flags: copyLocal);
				}
			}
		}

		protected virtual IEnumerable<string> GetDefaultSystemAssemblies(Module_DotNet module)
		{
			if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Standard 
				|| module.TargetFrameworkFamily == DotNetFrameworkFamilies.Core)
			{
				// Most of the default assemblies are contained within the .net standard library
				// which gets added as a dependency automatically when the target framework is netstandard
				return new List<string>();
			}

			// First add some system assemblies:
			var defaultSystemAssemblies = new List<string>() { "System.dll", "System.Data.Dll" };

			if (module.IsProjectType(DotNetProjectTypes.UnitTest) && Version.StrCompareVersions("15") < 0)
			{
				// while vs 2017 contains the above assembly, the default behaviour of creating a unit test project is
				// to make a nuget reference to the MSTest.TestFramework package - this handled much earlier to deal
				// with the automatic download and copy local setup
				defaultSystemAssemblies.Add("Microsoft.VisualStudio.QualityTools.UnitTestFramework.dll");
			}

			if (module.IsProjectType(DotNetProjectTypes.WebApp))
			{
				defaultSystemAssemblies.Add("System.Core.dll");
				defaultSystemAssemblies.Add("System.Configuration.dll");
				defaultSystemAssemblies.Add("System.EnterpriseServices.dll");
				defaultSystemAssemblies.Add("System.Data.DataSetExtensions.dll");
				defaultSystemAssemblies.Add("System.Web.dll");
				defaultSystemAssemblies.Add("System.Web.ApplicationServices.dll");
				defaultSystemAssemblies.Add("System.Web.DynamicData.dll");
				defaultSystemAssemblies.Add("System.Web.Entity.dll");
				defaultSystemAssemblies.Add("System.Web.Extensions.dll");
				defaultSystemAssemblies.Add("System.Web.Services.dll");
				defaultSystemAssemblies.Add("System.Xml.Linq.dll");
				defaultSystemAssemblies.Add("Microsoft.CSharp.dll");
			}
			if (module.IsProjectType(DotNetProjectTypes.Workflow))
			{
				defaultSystemAssemblies.Add("System.Core.dll");
				defaultSystemAssemblies.Add("System.WorkflowServices.dll");
				defaultSystemAssemblies.Add("System.Workflow.Activities.dll");
				defaultSystemAssemblies.Add("System.Workflow.ComponentModel.dll");
				defaultSystemAssemblies.Add("System.Workflow.Runtime.dll");
			}
			return defaultSystemAssemblies;
		}

		protected virtual void PrepareSubTypeMappings(IXmlWriter writer)
		{
			if (File.Exists(writer.FileName))
			{
				try
				{
					XmlDocument doc = new XmlDocument();
					doc.Load(writer.FileName);

					foreach (XmlNode itemGroup in doc.DocumentElement.ChildNodes)
					{
						if (!(itemGroup.NodeType == XmlNodeType.Element && itemGroup.Name == "ItemGroup"))
						{
							continue;
						}

						foreach (XmlNode tool in itemGroup.ChildNodes)
						{
							if (itemGroup.NodeType == XmlNodeType.Element)
							{
								var includeAttr = tool.Attributes["Include"];
								string subType = null;
								foreach (XmlNode snode in tool.ChildNodes)
								{
									if (snode.NodeType == XmlNodeType.Element)
									{
										if (snode.Name == "SubType")
										{
											subType = snode.InnerText.TrimWhiteSpace();
											break;
										}
									}
								}

								if (includeAttr != null && !String.IsNullOrEmpty(includeAttr.Value))
								{
									if (!String.IsNullOrEmpty(subType))
									{
										_savedCsprojData.AddSubTypeMapping(tool.Name, includeAttr.Value, subType);
									}
									else
									{
										_savedCsprojData.AddSubTypeMapping(tool.Name, includeAttr.Value, String.Empty);
									}
								}
							}
						}

					}
				}
				catch (Exception ex)
				{
					Log.Debug.WriteLine(LogPrefix + "Failed to load existing project file '{0}', reason {1}", writer.FileName, ex.Message);
				}
			}
		}

		private void ParseTextTemplate(FileEntry fe, out string ext, out string action, out OptionSet options, out bool addService)
		{
			var ttFile = fe.Path.Path;

			ext = ".cs";
			action = "Compile";
			addService = false;

			try
			{
				if (File.Exists(ttFile))
				{
					using (var reader = new StreamReader(ttFile))
					{
						int matches = 0;
						string line = null;
						while ((line = reader.ReadLine()) != null)
						{
							// Find extension: <#@ output extension=".txt" #>
							var match = Regex.Match(line, @"<#@\s* output .*extension=""(.*)""");
							if (match.Success && match.Groups.Count > 1)
							{
								ext = match.Groups[1].Value;
								matches++;
							}
							else
							{
								// Find <#@ template ..... hostspecific="true" ... #>
								match = Regex.Match(line, "<#@\\s* template .*hostspecific=\"([^\"]*)\"");
								if (match.Success && match.Groups.Count > 1)
								{
									addService = String.Equals("true", match.Groups[1].Value.TrimWhiteSpace(), StringComparison.OrdinalIgnoreCase);
									matches++;
								}
							}

							if (matches > 1)
							{
								break;
							}
						}
					}
				}
			}
			catch (Exception ex)
			{
				Log.Warning.WriteLine("Failed to parse text template file {0}: {1}", ttFile.Quote(), ex.Message);
			}

			options = GetFileEntryOptionset(fe);
			string vstool = null;


			if (options != null)
			{
				vstool = options.Options["visual-studio-tool"].TrimWhiteSpace();
			}

			if (!String.IsNullOrEmpty(vstool))
			{
				action = vstool;
			}
			else
			{
				switch (ext)
				{
					case ".cs":
					case ".vb":
						action = "Compile";
						break;
					default:
						action = "Content";
						break;
				}
			}

		}

		protected bool EnableStyleCop(IModule module = null)
		{
			if (module == null)
			{
				foreach (var mod in Modules)
				{
					if (EnableStyleCop(mod))
					{
						return true;
					}
				}
				return false;
			}

			return module.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".stylecop",
					module.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".csproj.stylecop",
					module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.stylecop.enable", false)));
		}

		protected bool EnableFxCop(IModule module = null)
		{
			if (module == null)
			{
				foreach (var mod in Modules)
				{
					if (EnableFxCop(mod))
					{
						return true;
					}
				}
				return false;
			}
			return module.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".fxcop",
				   module.Package.Project.Properties.GetBooleanPropertyOrDefault(StartupModule.GroupName + ".csproj.fxcop",
				   module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.fxcop.enable", false)));
		}

		protected override IXmlWriterFormat ProjectFileWriterFormat
		{
			get { return _xmlWriterFormat; }
		}

		private static readonly IXmlWriterFormat _xmlWriterFormat = new XmlFormat(
			identChar: ' ',
			identation: 2,
			newLineOnAttributes: false,
			encoding: new UTF8Encoding(false) // no byte order mask!
			);

		protected class SavedCsprojData
		{
			public SavedCsprojData()
			{
				_subTypeMappings = new Dictionary<string, string>();
			}

			public void AddSubTypeMapping(string Element, string include, string subType)
			{
				var key = Element + ";#;" + include;

				_subTypeMappings[key] = subType;
			}

			public string GetSubTypeMapping(string Element, string include, string defaultSubType, bool allowRemove = false)
			{
				string subType;
				var key = Element + ";#;" + include;

				if (!_subTypeMappings.TryGetValue(key, out subType))
				{
					subType = defaultSubType;
				}
				else if (String.IsNullOrEmpty(subType))
				{
					subType = allowRemove ? String.Empty : defaultSubType;
				}
				return subType;
			}


			private readonly IDictionary<string, string> _subTypeMappings;
		}

		private string _webConfigPath = null;
		protected readonly SavedCsprojData _savedCsprojData = new SavedCsprojData();

		/// <summary>
		/// This class is used to track XAML files with the same name within an assembly. There is a bug in the
		/// WPF build tasks for .Net Core and .Net 5 that makes linked XAML files with the same name stomp each
		/// in the IntermediateOuputPath. When this issue has been resolved this class can be safely removed.
		/// https://github.com/dotnet/wpf/issues/3292
		/// </summary>
		private class DuplicateXamlFileTracker : IDisposable
		{
			private readonly Dictionary<string, List<FileEntry>> m_xamlFilesByName = new Dictionary<string, List<FileEntry>>();
			private readonly bool m_isTargetingNetCoreStandard;

			internal DuplicateXamlFileTracker(bool isTargetingNetCoreStandard)
			{
				m_isTargetingNetCoreStandard = isTargetingNetCoreStandard;
			}

			internal void Add(FileEntry fe)
			{
				// Only track files on .Net Core builds
				if (!m_isTargetingNetCoreStandard)
					return;

				var xamlFileName = Path.GetFileName(fe.Path.Path);
				if (!m_xamlFilesByName.TryGetValue(xamlFileName, out var xamlFiles))
				{
					xamlFiles = new List<FileEntry>();
					m_xamlFilesByName.Add(xamlFileName, xamlFiles);
				}

				xamlFiles.Add(fe);
			}

			public void Dispose()
			{
				StringBuilder builder = new StringBuilder();
				foreach (var xamlFile in m_xamlFilesByName)
				{
					if (xamlFile.Value.Count == 1)
						continue;

					if (builder.Length == 0)
						builder.AppendLine("Multiple XAML files found with the same name, this will cause build failures in .Net Core builds.");

					builder.AppendLine($"{xamlFile.Key} found at the following path:");

					foreach (var xamlFilePath in xamlFile.Value)
						builder.AppendLine(xamlFilePath.Path.Path);
				}

				if (builder.Length != 0)
				{
					builder.AppendLine($"Rename these files to be unique if you need to build with .Net Core");
					throw new BuildException(builder.ToString());
				}
			}
		}
	}
}
