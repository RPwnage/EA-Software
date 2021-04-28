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
using System.Text;
using System.Xml;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;
using NAnt.Core.Util;

namespace EA.Eaconfig.Structured
{
	/// <summary>A module buildable by the dot net framework</summary>
	public class DotNetModuleTask : ModuleBaseTask
	{
		public readonly string CompilerName;
		private static readonly string[] _compilers =new string[] {"cc", "as", "csc", "fsc"};

		protected DotNetModuleTask()
			: this(String.Empty, "csc")
		{
		}

		protected DotNetModuleTask(string buildtype, string compiler)
			: base(buildtype)
		{
			CompilerName = compiler;
			Config = new DotNetConfigElement(this);
			BuildSteps = new BuildStepsElement(this);
			mCombinedState = new CombinedStateData();

			if (CompilerName == "csc")
			{
				FrameworkPrefix = "csproj";
			}
			else if (CompilerName == "fsc")
			{
				FrameworkPrefix = "fsproj";
			}
			else
			{
				throw new BuildException("[DotNetModule] Invalid compiler parameter.", Location);
			}
		}

		/// <summary>Overrides the default framework directory where built files are located.</summary>
		[TaskAttribute("outputdir")]
		public String OutputDir { get; set; } = String.Empty;

		/// <summary>Overrides the default name of built files.</summary>
		[TaskAttribute("outputname")]
		public String OutputName { get; set; } = String.Empty;

		/// <summary>Is this a Workflow module.</summary>
		[TaskAttribute("workflow")]
		public bool Workflow { get; set; } = false;

		/// <summary>Is this a UnitTest module.</summary>
		[TaskAttribute("unittest")]
		public bool UnitTest { get; set; } = false;

		/// <summary>Indicates this is a web application project and enables web debugging.</summary>
		[TaskAttribute("webapp")]
		public bool WebApp { get; set; } = false;

		/// <summary>Specifies the Rootnamespace for a visual studio project</summary>
		[Property("rootnamespace", Required = false)]
		public ConditionalPropertyElement RootNamespace { get; set; } = new ConditionalPropertyElement(append: false);

		/// <summary>Specifies the location of the Application manifest</summary>
		[Property("applicationmanifest", Required = false)]
		public ConditionalPropertyElement ApplicationManifest { get; set; } = new ConditionalPropertyElement();

		/// <summary>Specifies the name of the App designer folder</summary>
		[Property("appdesignerfolder", Required = false)]
		public ConditionalPropertyElement AppdesignerFolder { get; set; } = new ConditionalPropertyElement();

		/// <summary>used to enable/disable Visual Studio hosting process during debugging</summary>
		[Property("disablevshosting", Required = false)]
		public ConditionalPropertyElement DisableVsHosting { get; set; } = new ConditionalPropertyElement();

		/// <summary>Additional MSBuild project imports</summary>
		[Property("importmsbuildprojects", Required = false)]
		public ConditionalPropertyElement ImportMSBuildProjects { get; set; } = new ConditionalPropertyElement();

		/// <summary>Postbuild event run condition</summary>
		[Property("runpostbuildevent", Required = false)]
		public ConditionalPropertyElement RunPostbuildEvent { get; set; } = new ConditionalPropertyElement();

		/// <summary>The location of the Application Icon file</summary>
		[Property("applicationicon", Required = false)]
		public ConditionalPropertyElement ApplicationIcon { get; set; } = new ConditionalPropertyElement();

		/// <summary>property enables/disables generation of XML documentation files</summary>
		[Property("generatedoc", Required = false)]
		public ConditionalPropertyElement GenerateDoc { get; set; } = new ConditionalPropertyElement();

		/// <summary>Key File</summary>
		[Property("keyfile", Required = false)]
		public ConditionalPropertyElement KeyFile { get; set; } = new ConditionalPropertyElement();

		[Property("copylocal-dependencies", Required = false)]
		public ConditionalPropertyElement CopyLocalDependencies { get; set; } = new ConditionalPropertyElement();

		/// <summary>Adds the list of sourcefiles</summary>
		[FileSet("sourcefiles", Required = false)]
		public StructuredFileSet SourceFiles { get; } = new StructuredFileSet();

		/// <summary>A list of referenced assemblies for this module</summary>
		[FileSet("assemblies", Required = false)]
		public DotNetAssembliesFileset Assemblies { get; } = new DotNetAssembliesFileset();

		/// <summary>Adds a set of native dynamic/shared libraries. These do not affect build but be copied
		/// to module output directory if copy local is enabled.</summary>
		[FileSet("dlls", Required = false)]
		public StructuredFileSet Dlls { get; set; } = new StructuredFileSet();

		/// <summary>Adds a list of resource files</summary>
		[FileSet("resourcefiles", Required = false)]
		public ResourceFilesElement ResourceFiles { get; } = new ResourceFilesElement();

		/// <summary>Files with associated custombuildtools</summary>
		[FileSet("custombuildfiles", Required = false)]
		public StructuredFileSetCollection CustomBuildFiles { get; } = new StructuredFileSetCollection();

		/// <summary>Adds a list of resource files</summary>
		[FileSet("nonembed-resourcefiles", Required = false)]
		public StructuredFileSet ResourceFilesNonEmbed { get; } = new StructuredFileSet();

		/// <summary>Adds a list of 'Content' files</summary>
		[FileSet("contentfiles", Required = false)]
		public StructuredFileSet ContentFiles { get; } = new StructuredFileSet();

		/// <summary>A list of webreferences for this module</summary>
		[OptionSet("webreferences", Required = false)]
		public StructuredOptionSet WebReferences { get; set; } = new StructuredOptionSet();

		/// <summary>Adds the list of modules</summary>
		[FileSet("modules", Required = false)]
		public StructuredFileSet Modules { get; } = new StructuredFileSet();

		/// <summary>A list of COM assemblies for this module</summary>
		[FileSet("comassemblies", Required = false)]
		public StructuredFileSet ComAssemblies { get; set; } = new StructuredFileSet();

		/// <summary>Sets the configuration for a project</summary>
		[BuildElement("config", Required = false)]
		public DotNetConfigElement Config { get; }

		/// <summary>Sets the build steps for a project</summary>
		[BuildElement("buildsteps", Required = false)]
		public BuildStepsElement BuildSteps { get; }

		/// <summary></summary>
		[BuildElement("visualstudio", Required = false)]
		public VisualStudioDataElement VisualStudioData { get; } = new VisualStudioDataElement();

		/// <summary></summary>
		[BuildElement("platforms", Required = false)]
		public PlatformExtensions PlatformExtensions { get; } = new PlatformExtensions();

		/// <summary></summary>
		[BuildElement("nugetreferences", Required = false)] // NUGET-TODO: not files, should not be a fileset
		public StructuredFileSet NugetReferences { get; } = new StructuredFileSet();

		/// <summary>[Deprecated] Merge module feature is being deprecated.  If you still required to use this feature, please contact Frostbite.Team.CM regarding your requirement.</summary>
		[Property("merge-into", Required = false)]
		public ConditionalPropertyElement MergeInto { get; set; } = new ConditionalPropertyElement();

		/// <summary></summary>
		[Property("assemblyinfo", Required = false)]
		public AssemblyInfoTask GeneratedAssemblyInfo { get; } = new AssemblyInfoTask();

		public override bool GenerateBuildLayoutByDefault
		{
			get
			{
				return UnitTest || BuildTypeInfo.IsProgram;
			}
		}

		protected override void SetupModule()
		{
			SetupConfig();

			SetupBuildData();

			SetupBuildLayoutData();

			SetupDependencies();

			SetupVisualStudio();

			PlatformExtensions.ExecutePlatformTasks(this);

			SetupAfterBuild();
		}

		protected override void SetBuildType()
		{
		}

		private void VerifyDotNetCompilerOPtions(string optionName, XmlNode node)
		{
			var valid_prefix = "buildset." + CompilerName;

			if (!optionName.StartsWith(valid_prefix) && optionName.StartsWith("buildset"))
			{
				if (_compilers.Any(cn => cn != CompilerName && optionName.StartsWith("buildset." + cn + ".")))
				{
					Log.Warning.WriteLine(LogPrefix + "{0}{1}{2}/{3}: 'config' element contains option for the wrong compiler '{4}', expected compiler is '{5}'.", Location.GetLocationFromNode(node), Environment.NewLine, PackageName, ModuleName, optionName, CompilerName);
				}
			}
		}

		private void SetOption(string name, PropertyElement value, bool splitlines = false)
		{
			if (value != null && value.Value != null)
			{
				var text = value.Value.TrimWhiteSpace();
				if (!String.IsNullOrEmpty(text))
				{
					Config.buildOptionSet.Options[name] = (splitlines ? text.LinesToArray() : text.ToArray()).ToString(Environment.NewLine);
				}
			}
		}

		protected void SetupConfig()
		{
			string buildtype = GetModuleProperty("buildtype");

			Config.BuildOptions.VerifyOptionEx += VerifyDotNetCompilerOPtions;

			Config.BuildOptions.InternalInitializeElement(buildtype);

			if (Config.buildOptionSet.Options.Count > 0)
			{
				string finalbuildtype = BuildType = (buildtype ?? BuildType) + "-" + ModuleName;

				Config.buildOptionSet.Options["buildset.name"] = finalbuildtype;
				SetModuleProperty("buildtype", finalbuildtype);

				SetOption("remove." + CompilerName + ".options", Config.RemoveOptions.Options);

				Project.NamedOptionSets[finalbuildtype + "-temp"] = Config.buildOptionSet;

				GenerateBuildOptionset.Execute(Project, Config.buildOptionSet, finalbuildtype + "-temp");
			}

			base.SetBuildType();

			SetModuleProperty("defines", StringUtil.EnsureSeparator(Config.Defines.Value, Environment.NewLine), Config.Defines.Append);
			SetModuleProperty(CompilerName + "-args", Config.AdditionalOptions.Value, Config.AdditionalOptions.Append);
			
			string targetFrameworkFamily = Config.TargetFrameworkFamily.Value;
			if (targetFrameworkFamily != null)
			{
				if (Project.NetCoreSupport)
				{
					SetModuleProperty("targetframeworkfamily", targetFrameworkFamily);
				}
				else
				{
					Log.Status.WriteLine("{0}Config element <targetframeworkfamily> is ignored unless .NET Core support is enabled (property {1}=false).", Location, Project.NANT_PROPERTY_NET_CORE_SUPPORT);
				}
			}

			SetModuleProperty("copylocal", Config.CopyLocal.Value);
			SetModuleProperty("platform", Config.Platform.Value);
			SetModuleProperty("copylocal.dependencies", CopyLocalDependencies.Value);
			SetModuleProperty("allowunsafe", Config.AllowUnsafe.Value);
			SetModuleProperty("languageversion", Config.LanguageVersion.Value);
			SetModuleProperty("usewpf", Config.UseWpf.Value);
			SetModuleProperty("usewindowsforms", Config.UseWindowsForms.Value);
			SetModuleProperty("generateserializationassemblies", Config.GenerateSerializationAssemblies.Value);
			SetModuleProperty("suppresswarnings", StringUtil.EnsureSeparator(Config.Suppresswarnings.Value, Environment.NewLine), Config.Suppresswarnings.Append);
			SetModuleProperty("warningsaserrors.list", StringUtil.EnsureSeparator(Config.WarningsaserrorsList.Value, Environment.NewLine), Config.WarningsaserrorsList.Append);
			SetModuleProperty("warningsaserrors", Config.Warningsaserrors.Value);
			SetModuleProperty("warningsnotaserrors.list", StringUtil.EnsureSeparator(Config.Warningsnotaserrors.Value, Environment.NewLine), Config.Warningsnotaserrors.Append);

			SetModuleProperty("remove.defines", StringUtil.EnsureSeparator(Config.RemoveOptions.Defines.Value, Environment.NewLine), Config.RemoveOptions.Defines.Append);
			SetModuleProperty("remove." + CompilerName + ".options", StringUtil.EnsureSeparator(Config.RemoveOptions.Options.Value, Environment.NewLine), Config.RemoveOptions.Options.Append);

			SetModuleProperty("preprocess", Config.Preprocess.Value, Config.Preprocess.Append);
			SetModuleProperty("postprocess", Config.Postprocess.Value, Config.Postprocess.Append);
		}

		protected void SetupBuildData()
		{
			// outputdir remapping property (package.[pkg].[module].outputdir) setup in Initialize.xml
			string outputdir_remap = Project.Properties.GetPropertyOrDefault("package." + PackageName + "." + ModuleName + ".outputdir", String.Empty);
			if (OutputDir != String.Empty)
			{
				if (String.IsNullOrEmpty(outputdir_remap))
				{
					outputdir_remap = OutputDir;
				}
				else if (OutputDir != outputdir_remap)
				{
					throw new BuildException(String.Format("Package {0}'s Initialize.xml has setup outputdir for module '{1}' to be '{2}'.  But this module's .build file is also trying to setup outputdir but with different value '{3}'. You need to correct this error and specify outputdir remapping in Initialize.xml only!",
						PackageName, ModuleName, outputdir_remap, OutputDir));
				}
			}
			if (!String.IsNullOrEmpty(outputdir_remap))
			{
				SetModuleProperty("outputdir", outputdir_remap);
			}

			// outputname remapping property (package.[pkg].[module].outputname) setup in Initialize.xml
			string outputname_remap = Project.Properties.GetPropertyOrDefault("package." + PackageName + "." + ModuleName + ".outputname", String.Empty);
			if (OutputName != String.Empty)
			{
				if (String.IsNullOrEmpty(outputname_remap))
				{
					outputname_remap = OutputName;
				}
				else if (OutputName != outputname_remap)
				{
					throw new BuildException(String.Format("Package {0}'s Initialize.xml has setup outputname for module '{1}' to be '{2}'.  But this module's .build file is also trying to setup outputname but with different value '{3}'. You need to correct this error and specify outputname remapping in Initialize.xml only!",
						PackageName, ModuleName, outputname_remap, OutputName));
				}
			}
			if (!String.IsNullOrEmpty(outputname_remap))
			{
				SetModuleProperty("outputname", outputname_remap);
			}

			SetModuleProperty(FrameworkPrefix + ".rootnamespace", RootNamespace);
			SetModuleProperty(FrameworkPrefix + ".application-manifest", ApplicationManifest);
			SetModuleProperty(FrameworkPrefix + ".appdesigner-folder", AppdesignerFolder);
			SetModuleProperty(FrameworkPrefix + ".disablevshosting", DisableVsHosting);
			SetModuleProperty(FrameworkPrefix + ".importmsbuildprojects", ImportMSBuildProjects, ImportMSBuildProjects.Append);
			SetModuleProperty(FrameworkPrefix + ".runpostbuildevent", RunPostbuildEvent);

			if (UnitTest)
			{
				
				SetModuleProperty(FrameworkPrefix + ".unittest", "true");

				if (!Project.UseNugetResolution && !Project.NetCoreSupport) // net core support implies nuget res, just documenting that both are needed
				{
					if (SetConfigVisualStudioVersion.Execute(Project).StrCompareVersions("15") >= 0)
					{
						// for VS2017+ we need to add a dependency on the MSTest nuget packages
						string[] msTestAutoDependencies = new string[] { "MSTest.TestFramework", "MSTest.TestAdapter" };
						string dependencyString = String.Join(Environment.NewLine, msTestAutoDependencies);
						SetModuleDependencies(7, "autodependencies", dependencyString);
						SetModuleProperty("copylocal.dependencies", dependencyString, append: true);
					}
				}
			}

			if (WebApp)
			{
				SetModuleProperty(FrameworkPrefix + ".webapp", "true");
			}

			if (Workflow)
			{
				SetModuleProperty(FrameworkPrefix + ".workflow", "true");
			}

			SetModuleProperty("usedefaultassemblies", Assemblies.UseDefaultAssemblies.ToString().ToLowerInvariant());
			SetModuleProperty("win32icon", ApplicationIcon);
			SetModuleProperty(CompilerName + "-doc", GenerateDoc);
			SetModuleProperty("keyfile", KeyFile);
			
			SetModuleFileset("sourcefiles", SourceFiles);
			SetModuleFileset("assemblies", Assemblies);
			SetModuleFileset("modules", Modules);
			SetModuleFileset("comassemblies", ComAssemblies);
			SetModuleFileset("dlls", Dlls);
			SetModuleFileset("nugetreferences", NugetReferences);

			StructuredFileSet custombuildfiles = new StructuredFileSet();
			foreach (StructuredFileSet fs in CustomBuildFiles.FileSets)
			{
				custombuildfiles.AppendWithBaseDir(fs);
				custombuildfiles.AppendBase = fs.AppendBase;
			}
			if (custombuildfiles.BaseDirectory == null) custombuildfiles.BaseDirectory = Project.BaseDirectory;

			SetModuleFileset("custombuildfiles", custombuildfiles);
			
			if (ResourceFiles.Includes.Count > 0 || ResourceFiles.Excludes.Count > 0)
			{
				SetModuleFileset("resourcefiles", ResourceFiles);
				SetModuleProperty("resourcefiles.prefix", GetValueOrDefault(ResourceFiles.Prefix, ModuleName));
				SetModuleProperty("resourcefiles.basedir", SourceFiles.BaseDirectory ?? ResourceFiles.ResourceBasedir);
			}
			if (ResourceFilesNonEmbed.Includes.Count > 0 || ResourceFilesNonEmbed.Excludes.Count > 0)
			{
				SetModuleFileset("resourcefiles.notembed", ResourceFilesNonEmbed);
			}

			if (ContentFiles.Includes.Count > 0 || ContentFiles.Excludes.Count > 0)
			{
				SetModuleFileset("contentfiles", ContentFiles);
			}


			SetModuleOptionSet("webreferences", WebReferences);

			SetModuleTarget(BuildSteps.PrebuildTarget, "prebuildtarget");
			SetModuleTarget(BuildSteps.PostbuildTarget, "postbuildtarget");
			SetModuleProperty(FrameworkPrefix + ".pre-build-step", BuildSteps.PrebuildTarget.Command);
			SetModuleProperty(FrameworkPrefix + ".post-build-step", BuildSteps.PostbuildTarget.Command);

			if (BuildSteps.PostbuildTarget.SkipIfUpToDate)
			{
				SetModuleProperty("postbuild.skip-if-up-to-date", "true", append:false);
			}

			SetModuleProperty("vcproj.custom-build-tool", BuildSteps.CustomBuildStep.Script.Value);
			SetModuleProperty("vcproj.custom-build-outputs", BuildSteps.CustomBuildStep.OutputDependencies.Value);
			SetModuleFileset("vcproj.custom-build-dependencies", BuildSteps.CustomBuildStep.InputDependencies);
			SetModuleProperty("custom-build-before-targets", BuildSteps.CustomBuildStep.Before);
			SetModuleProperty("custom-build-after-targets", BuildSteps.CustomBuildStep.After);
			SetModuleProperty("merge-into", MergeInto.Value);

			if (GeneratedAssemblyInfo.Generate)
			{
				SetModuleProperty("generateassemblyinfo", "true");
				SetModuleProperty("assemblyinfo.company", GeneratedAssemblyInfo.Company);
				SetModuleProperty("assemblyinfo.copyright", GeneratedAssemblyInfo.Copyright);
				SetModuleProperty("assemblyinfo.product", GeneratedAssemblyInfo.Product);
				SetModuleProperty("assemblyinfo.title", GeneratedAssemblyInfo.Title);
				SetModuleProperty("assemblyinfo.description", GeneratedAssemblyInfo.Description);
				SetModuleProperty("assemblyinfo.version", GeneratedAssemblyInfo.VersionNumber);
			}
		}

		protected void SetupVisualStudio()
		{
			SetModuleFileset(FrameworkPrefix + ".excludedbuildfiles", VisualStudioData.ExcludedBuildFiles);
			SetModuleOptionSet("msbuildoptions", VisualStudioData.MsbuildOptions);
			SetModuleProperty("projecttypeguids", VisualStudioData.ProjectTypeGuids);
			SetModuleProperty("visualstudio-extension", VisualStudioData.Extension);

			SetModuleTarget(VisualStudioData.PregenerateTarget, FrameworkPrefix + ".prebuildtarget");

			if(!String.IsNullOrEmpty(VisualStudioData.EnableUnmanagedDebugging.Value.TrimWhiteSpace()))
			{
				var enableval = ConvertUtil.ToNullableBoolean(VisualStudioData.EnableUnmanagedDebugging.Value.TrimWhiteSpace(), onError: (val) => { throw new BuildException("enableunmanageddebugging element accepts only true or false values, value='" + val + "' is invalid", VisualStudioData.EnableUnmanagedDebugging.Location); });

				SetModuleProperty("enableunmanageddebugging", enableval.ToString().ToLowerInvariant());
			}
		}

		protected void SetupAfterBuild()
		{
			// --- Packaging ---
			StringBuilder names = new StringBuilder();
			foreach (StructuredFileSet fs in BuildSteps.PackagingData.Assets.FileSets)
			{
				names.AppendLine(SetModuleFileset("assetfiles", fs));
			}
			if (names.Length > 0)
			{
				SetModuleProperty("assetfiles-set", names.ToString(), true);
			}

			if (BuildSteps.PackagingData._deployAssets != null)
			{
				SetModuleProperty("deployassets", BuildSteps.PackagingData.DeployAssets.ToString().ToLowerInvariant(), false);
			}

			SetModuleProperty("assetdependencies", BuildSteps.PackagingData.AssetDependencies);
			if (!String.IsNullOrEmpty(BuildSteps.PackagingData.AssetConfigBuildDir.Value))
			{
				SetModuleProperty("asset-configbuilddir", TrimWhiteSpace(BuildSteps.PackagingData.AssetConfigBuildDir.Value), false);
			}
			SetModuleFileset("manifestfile", BuildSteps.PackagingData.ManifestFile);

			if (BuildSteps.PackagingData._copyAssetsWithoutSync != null)
			{
				SetModuleProperty("CopyAssetsWithoutSync", BuildSteps.PackagingData.CopyAssetsWithoutSync.ToString().ToLowerInvariant(), false);
			}
			
			// --- Run ---
			SetModuleProperty("workingdir", TrimWhiteSpace(BuildSteps.RunData.WorkingDir), false);
			SetModuleProperty("commandargs", TrimWhiteSpace(BuildSteps.RunData.Args), false);
			SetModuleProperty("commandprogram", TrimWhiteSpace(BuildSteps.RunData.StartupProgram), false);

			SetModuleProperty("run.workingdir", TrimWhiteSpace(BuildSteps.RunData.WorkingDir), false);
			SetModuleProperty("run.args", TrimWhiteSpace(BuildSteps.RunData.Args), false);
		}

		protected override void InitModule()
		{
		}

		protected override void FinalizeModule()
		{
		}

		private string FrameworkPrefix { get; }


		internal class CombinedStateData
		{

			internal static void UpdateBoolean(ref bool val, bool? newval)
			{
				if (newval != null)
				{
					val = (bool)newval;
				}
			}
		}
		internal CombinedStateData mCombinedState;

		/// <summary>Sets various attributes for a config</summary>
		[ElementName("config", StrictAttributeCheck = true, NestedElementsCheck = true)]
		public class DotNetConfigElement : ConditionalElementContainer
		{
			private ModuleBaseTask _module;

			public readonly OptionSet buildOptionSet = new OptionSet();

			public DotNetConfigElement(ModuleBaseTask module)
			{
				_module = module;

				BuildOptions = new BuildTypeElement(_module, buildOptionSet);
			}

			/// <summary>Gets the build options for this config.</summary>
			[BuildElement("buildoptions", Required = false)]
			public BuildTypeElement BuildOptions { get; }

			/// <summary>Gets the macros defined for this config</summary>
			[Property("defines", Required = false)]
			public ConditionalPropertyElement Defines { get; } = new ConditionalPropertyElement();

			/// <summary>Additional commandline options, new line separated (added to options defined through optionset</summary>
			[Property("additionaloptions", Required = false)]
			public ConditionalPropertyElement AdditionalOptions { get; } = new ConditionalPropertyElement();

			/// <summary>Defines whether referenced assemblies are copied into the output folder of the module</summary>
			[Property("copylocal", Required = false)]
			public ConditionalPropertyElement CopyLocal { get; set; } = new ConditionalPropertyElement();

			/// <summary>Specifies the platform to build against</summary>
			[Property("platform", Required = false)]
			public ConditionalPropertyElement Platform { get; set; } = new ConditionalPropertyElement();

			[Property("allowunsafe", Required = false)]
			public ConditionalPropertyElement AllowUnsafe { get; set; } = new ConditionalPropertyElement();

			/// <summary>used to define target .Net framework version</summary>
			[Property("targetframeworkversion", Required = false)]
			public DeprecatedPropertyElement TargetFrameworkVersion { get; set; } = new DeprecatedPropertyElement
			(
				NAnt.Core.Logging.Log.DeprecationId.ModuleTargetFrameworkVersion, NAnt.Core.Logging.Log.DeprecateLevel.Minimal,
				"Target .NET version is now determined by .NET family and the relevant DotNet or DotNetCoreSdk package version in masterconfig."
			);

			/// <summary>Set the C# language version, if not set the compiler default will be used</summary>
			[Property("languageversion", Required = false)]
			public ConditionalPropertyElement LanguageVersion { get; } = new ConditionalPropertyElement();

			/// <summary>option used together with .net core to depend on necessary libraries for WPF</summary>
			[Property("usewpf", Required = false)]
			public ConditionalPropertyElement UseWpf { get; set; } = new ConditionalPropertyElement();

			/// <summary>option used together with .net core to depend on necessary libraries for Windows Forms</summary>
			[Property("usewindowsforms", Required = false)]
			public ConditionalPropertyElement UseWindowsForms { get; set; } = new ConditionalPropertyElement();

			/// <summary>used to indicate .net framework or core or standard</summary>
			[Property("targetframeworkfamily", Required = false)]
			public ConditionalPropertyElement TargetFrameworkFamily { get; set; } = new ConditionalPropertyElement();

			/// <summary>Generate serialization assemblies:  None, Auto, On, Off</summary>
			[Property("generateserializationassemblies", Required = false)]
			public ConditionalPropertyElement GenerateSerializationAssemblies { get; set; } = new ConditionalPropertyElement();

			/// <summary>Gets the warning suppression property for this config</summary>
			[Property("suppresswarnings", Required = false)]
			public ConditionalPropertyElement Suppresswarnings { get; } = new ConditionalPropertyElement();

			/// <summary>List of warnings to treat as errors</summary>
			[Property("warningsaserrors.list", Required = false)]
			public ConditionalPropertyElement WarningsaserrorsList { get; } = new ConditionalPropertyElement();

			/// <summary>Treat warnings as errors</summary>
			[Property("warningsaserrors", Required = false)]
			public ConditionalPropertyElement Warningsaserrors { get; } = new ConditionalPropertyElement();

			/// <summary>List of warnings that should just be treated as warnings, when warningsaserrors is on these warnings will not be errors and will not be suppressed</summary>
			[Property("warningsnotaserrors.list", Required = false)]
			public ConditionalPropertyElement Warningsnotaserrors { get; } = new ConditionalPropertyElement();

			/// <summary>Define options to removefrom the final optionset</summary>
			[Property("remove", Required = false)]
			public RemoveDotNetBuildOptions RemoveOptions { get; } = new RemoveDotNetBuildOptions();

			/// <summary>Preprocess step can be C# task derived from AbstractModuleProcessorTask class or a target. Multiple preprocess steps are supported</summary>
			[Property("preprocess", Required = false)]
			public ConditionalPropertyElement Preprocess { get; } = new ConditionalPropertyElement();

			/// <summary>Preprocess step can be C# task derived from AbstractModuleProcessorTask class or a target. Multiple preprocess steps are supported</summary>
			[Property("postprocess", Required = false)]
			public ConditionalPropertyElement Postprocess { get; } = new ConditionalPropertyElement();
		}

		/// <summary>Sets options to be removed from final configuration</summary>
		[ElementName("remove", StrictAttributeCheck = true, NestedElementsCheck = true)]
		public class RemoveDotNetBuildOptions : ConditionalElementContainer
		{
			/// <summary>Defines to be removed</summary>
			[Property("defines", Required = false)]
			public ConditionalPropertyElement Defines { get; } = new ConditionalPropertyElement();

			/// <summary>Compiler options to be removed</summary>
			[Property("options", Required = false)]
			public ConditionalPropertyElement Options { get; } = new ConditionalPropertyElement();
		}

		/// <summary></summary>
		[ElementName("VisualStudio", StrictAttributeCheck = true, NestedElementsCheck = true)]
		public class VisualStudioDataElement : ConditionalElementContainer
		{
			/// <summary></summary>
			[Property("pregenerate-target", Required = false)]
			public BuildTargetElement PregenerateTarget { get; } = new BuildTargetElement("vcproj.prebuildtarget");

			/// <summary>A list of files that are excluded from the build but are added to the visual studio
			/// project as non-buildable files</summary>
			[FileSet("excludedbuildfiles", Required = false)]
			public StructuredFileSet ExcludedBuildFiles { get; set; } = new StructuredFileSet();

			/// <summary></summary>
			[Property("enableunmanageddebugging", Required = false)]
			public ConditionalPropertyElement EnableUnmanagedDebugging { get; set; } = new ConditionalPropertyElement();

			/// <summary>A list of elements to insert directly into the visualstudio project file in the config build options section.</summary>
			[Property("msbuildoptions", Required = false)]
			public StructuredOptionSet MsbuildOptions { get; set; } = new StructuredOptionSet();

			/// <summary>Allow users to set custom project type guids. Multiple guids should be separated by a semicolon.</summary>
			[Property("projecttypeguids", Required = false)]
			public ConditionalPropertyElement ProjectTypeGuids { get; set; } = new ConditionalPropertyElement();

			/// <summary>The name(s) of a Visual Studio Extension task used for altering the solution generation process 
			/// to allow adding custom elements to a project file.</summary>
			[Property("extension", Required = false)]
			public ConditionalPropertyElement Extension { get; set; } = new ConditionalPropertyElement();
		}
	}

}

