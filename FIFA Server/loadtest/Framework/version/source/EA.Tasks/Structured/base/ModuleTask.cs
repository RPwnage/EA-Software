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
using System.IO;
using System.Text;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Structured;
using NAnt.Core.Tasks;
using NAnt.Core.Util;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Structured
{
	/// <summary>A standard structured XML Module with user specified buildtype to be used in the .build file (not to be confused with the &lt;module&gt; task for Initialize.xml file)</summary>
	[TaskName("Module", NestedElementsCheck = true)]
	public class ModuleTask : ModuleBaseTask
	{
		public ModuleTask()
			: this(String.Empty)
		{
		}

		protected ModuleTask(string buildtype) : base(buildtype)
		{
			Config = new ConfigElement(this);
			UseSharedPch = new UseSharedPchElement();
			BuildSteps = new BuildStepsElement(this);
			mCombinedState = new CombinedStateData();
		}

		/// <summary>Overrides the default framework directory where built files are located.</summary>
		[TaskAttribute("outputdir")]
		public String OutputDir { get; set; } = String.Empty;

		/// <summary>Overrides the default name of built files.</summary>
		[TaskAttribute("outputname")]
		public String OutputName { get; set; } = String.Empty;

		/// <summary>Adds a set of libraries </summary>
		[FileSet("libraries", Required = false)]
		public StructuredFileSet Libraries { get; set; } = new StructuredFileSet();

		/// <summary>Adds a set of dynamic/shared libraries. DLLs are passed to the linker input.</summary>
		[FileSet("dlls", Required = false)]
		public StructuredFileSet Dlls { get; set; } = new StructuredFileSet();

		/// <summary>Adds the list of object files</summary>
		[FileSet("objectfiles", Required = false)]
		public StructuredFileSet ObjectFiles { get; } = new StructuredFileSet();

		/// <summary>Adds the list of source files</summary>
		[FileSet("sourcefiles", Required = false)]
		public StructuredFileSet SourceFiles { get; } = new StructuredFileSet();

		/// <summary>Adds a list of assembly source files</summary>
		[FileSet("asmsourcefiles", Required = false)]
		public StructuredFileSet AsmSourceFiles { get; } = new StructuredFileSet();

		/// <summary>Adds a list of resource files</summary>
		[FileSet("resourcefiles", Required = false)]
		public ResourceFilesElement ResourceFiles { get; } = new ResourceFilesElement();

		/// <summary>Adds a list of shader files</summary>
		[FileSet("shaderfiles", Required = false)]
		public StructuredFileSet ShaderFiles { get; } = new StructuredFileSet();

		/// <summary>Adds a list of resource files</summary>
		[FileSet("nonembed-resourcefiles", Required = false)]
		public StructuredFileSet ResourceFilesNonEmbed { get; } = new StructuredFileSet();

		/// <summary>Adds a list of natvis files</summary>
		[FileSet("natvisfiles", Required = false)]
		public StructuredFileSet NatvisFiles { get; } = new StructuredFileSet();

		/// <summary>Files with associated custombuildtools</summary>
		[FileSet("custombuildfiles", Required = false)]
		public StructuredFileSetCollection CustomBuildFiles { get; } = new StructuredFileSetCollection();

		/// <summary>Specifies the Rootnamespace for a visual studio project</summary>
		[Property("rootnamespace", Required = false)]
		public ConditionalPropertyElement RootNamespace { get; set; } = new ConditionalPropertyElement(append: false);

		/// <summary>Defines set of directories to search for header files. Relative paths are prepended with the package directory.</summary>
		[Property("includedirs", Required = false)]
		public ConditionalPropertyElement IncludeDirs { get; set; } = new ConditionalPropertyElement();

		/// <summary>Defines set of directories to search for library files. Relative paths are prepended with the package directory.</summary>
		[Property("libdirs", Required = false)]
		public ConditionalPropertyElement LibraryDirs { get; set; } = new ConditionalPropertyElement();

		/// <summary>Defines set of directories to search for forced using assemblies. Relative paths are prepended with the package directory.</summary>
		[Property("usingdirs", Required = false)]
		public ConditionalPropertyElement UsingDirs { get; set; } = new ConditionalPropertyElement();

		/// <summary>Includes a list of header files for a project</summary>
		[FileSet("headerfiles", Required = false)]
		public StructuredFileSet HeaderFiles { get; set; } = new StructuredFileSet();

		/// <summary>A list of COM assemblies for this module</summary>
		[FileSet("comassemblies", Required = false)]
		public StructuredFileSet ComAssemblies { get; set; } = new StructuredFileSet();

		/// <summary>A list of referenced assemblies for this module</summary>
		[FileSet("assemblies", Required = false)]
		public StructuredFileSet Assemblies { get; set; } = new StructuredFileSet();

		[FileSet("force-usingassemblies", Required = false)]
		public StructuredFileSet FuAssemblies { get; set; } = new StructuredFileSet();

		/// <summary>A list of application additional manifest files for native projects.</summary>
		[FileSet("additional-manifest-files", Required = false)]
		public StructuredFileSet AdditionalManifestFiles { get; } = new StructuredFileSet();

		/// <summary>Specified to use shared pch module.</summary>
		[BuildElement("usesharedpch", Required = false)]
		public UseSharedPchElement UseSharedPch { get; }

		/// <summary>Sets the configuration for a project</summary>
		[BuildElement("config", Required = false)]
		public ConfigElement Config { get; }

		/// <summary>Sets the build steps for a project</summary>
		[BuildElement("buildsteps", Required = false)]
		public BuildStepsElement BuildSteps { get; }

		/// <summary>Sets the bulkbuild configuration</summary>
		[BuildElement("bulkbuild", Required = false)]
		public BulkBuildElement BulkBuild { get; } = new BulkBuildElement();

		/// <summary>SDK references (used in MSBuild files)</summary>
		[Property("sdkreferences", Required = false)]
		public ConditionalPropertyElement SdkReferences { get; set; } = new ConditionalPropertyElement();

		/// <summary>MCPP/DotNet specific data</summary>
		[BuildElement("dotnet", Required = false)]
		public DotNetDataElement DotNetDataElement { get; } = new DotNetDataElement();

		/// <summary></summary>
		[BuildElement("visualstudio", Required = false)]
		public VisualStudioDataElement VisualStudioData { get; } = new VisualStudioDataElement();

		/// <summary></summary>
		[BuildElement("java", Required = false)]
		public DeprecatedPropertyElement JavaData { get; } = new DeprecatedPropertyElement(Log.DeprecationId.PrivateJavaElements, Log.DeprecateLevel.Minimal,
			"All java dependencies should now be configured using Gradle files.",
			condition: Project => Project.Properties["config-system"] == "android");

		/// <summary></summary>
		[BuildElement("android-gradle", Required = false)]
		public AndroidGradle AndroidGradle { get; } = new AndroidGradle();

		/// <summary></summary>
		[BuildElement("platforms", Required = false)]
		public PlatformExtensions PlatformExtensions { get; } = new PlatformExtensions();

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
				return BuildTypeInfo.IsProgram;
			}
		}

		public bool PCHEnabled
		{
			get
			{
				return mCombinedState.EnablePch;
			}
		}

		public bool BulkBuildEnabled
		{
			get
			{
				return mCombinedState.BulkBuildEnable;
			}
		}

		protected override void SetupModule()
		{
			SetupConfig();

			SetupBuildData();

			SetupBulkBuildData();

			SetupBuildLayoutData();

			SetupAndroidGradleData();

			SetupDependencies();

			SetupDotNet();

			SetupVisualStudio();

			PlatformExtensions.ExecutePlatformTasks(this);

			SetupAfterBuild();
		}

		private void SetupAndroidGradleData()
		{
			// GRADLE-TODO: should probably be android only
			// GRADLE-TODO: should definitely be program only
			if (AndroidGradle.Project != null) // project is null if element does not exist and thus was never initialized
			{
				SetModuleProperty("has-android-gradle", "true");

				string gradleDir = AndroidGradle.GradleDirectory ?? Project.BaseDirectory;

				SetModuleProperty("gradle-directory", gradleDir);
				SetModuleProperty("gradle-project", AndroidGradle.GradleProject ?? ModuleName);
				SetModuleProperty("gradle-jvm-args", AndroidGradle.GradleJvmArgs ?? "");

				SetModuleProperty("gradle-properties", AndroidGradle.GradleProperties, AndroidGradle.GradleProperties.Append);

				SetModuleFileset("javasourcefiles", AndroidGradle.JavaSourceFiles);
			}
		}

		protected override void SetBuildType()
		{
		}

		// Check that the buildtype in the build script and initialize.xml match, otherwise throw a warning
		protected void ValidateBuildType(string buildtype)
		{
			if (IsPartial) return;

			DoOnce.Execute(Project, PackageName + "." + ModuleName + "." + buildtype + ".buildtype-initialize-comparison", () =>
			{
				string initializeBuildType = Project.Properties["package." + PackageName + "." + ModuleName + ".buildtype"];

				if (!initializeBuildType.IsNullOrEmpty() && !buildtype.IsNullOrEmpty())
				{
					// Compare base build types rather than exact build types otherwise we will get too many warnings
					BuildType baseBuildType = GetModuleBaseType.Execute(Project, buildtype);
					BuildType baseBuildType2 = GetModuleBaseType.Execute(Project, initializeBuildType);

					if (baseBuildType.BaseName != baseBuildType2.BaseName)
					{
						Log.Warning.WriteLine("Module {0}/{1} specifies the buildtype {4} ({2}) in the build script but {5} ({3}) in the initialize.xml. " +
							"These should be the same or you may have incorrect behaviour.",
							PackageName, ModuleName, buildtype, initializeBuildType, baseBuildType.BaseName, baseBuildType2.BaseName);
					}
				}
			});
		}

		protected void SetupConfig()
		{
			string buildtype = GetModuleProperty("buildtype");
			Config.BuildOptions.InternalInitializeElement(buildtype);

			string finalbuildtype = (buildtype ?? BuildType);
			if (Config.buildOptionSet.Options.Count > 0)
			{
				finalbuildtype = finalbuildtype + "-" + ModuleName;

				Config.buildOptionSet.Options["buildset.name"] = finalbuildtype;
				SetModuleProperty("buildtype", finalbuildtype);

				Project.NamedOptionSets[finalbuildtype + "-temp"] = Config.buildOptionSet;

				GenerateBuildOptionset.Execute(Project, Config.buildOptionSet, finalbuildtype + "-temp");
			}
			BuildType = finalbuildtype;

			ValidateBuildType(BuildType);

			base.SetBuildType();

			CombinedStateData.UpdateBoolean(ref mCombinedState.EnablePch, Config.PrecompiledHeader._enable);
			if (!Project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
			{
				mCombinedState.EnablePch = false;
			}

			string sharedPchPackage = null;
			string sharedPchModule = null;
			if (!String.IsNullOrEmpty(UseSharedPch.SharedModule))
			{
				// Get the shared pch module's data.
				DependencyDeclaration sharedPchDep = new DependencyDeclaration(UseSharedPch.SharedModule);
				DependencyDeclaration.Package depPkg = sharedPchDep.Packages.FirstOrDefault();
				if (depPkg == null)
				{
					throw new BuildException(string.Format("Module {0} specified to use shared pch module {1}. But unable to retrieve package information!", ModuleName, UseSharedPch.SharedModule));
				}
				if (depPkg.FullDependency)
				{
					throw new BuildException(string.Format("Module {0} specified to use shared pch module {1}. But it is a full package specification. Please provide the specific shared pch module name.", ModuleName, UseSharedPch.SharedModule));
				}
				sharedPchPackage = depPkg.Name;
				sharedPchModule = depPkg.Groups.Select(g => g.Modules.FirstOrDefault()).Where(m => m != null).FirstOrDefault();
				if (String.IsNullOrEmpty(sharedPchModule))
				{
					throw new BuildException(string.Format("Module {0} specified to use shared pch module {1}. But unable to properly retrieve specific module name.", ModuleName, UseSharedPch.SharedModule));
				}

				// Now, do a dependency to that package to get the export property.
				TaskUtil.Dependent(Project, sharedPchPackage, Project.TargetStyleType.Use);
			}

			if (!string.IsNullOrEmpty(sharedPchModule) && !string.IsNullOrEmpty(sharedPchPackage))
			{
				SetModuleProperty("using-shared-pch", "true", append: false);

				// Even if pch is not enabled, but if shared pch is specified, it may still need
				// the header and all it's interface dependency
				Config.PrecompiledHeader.PchHeaderFile = Project.Properties["package." + sharedPchPackage + "." + sharedPchModule + ".pchheader"]; ;
				Config.PrecompiledHeader.PchHeaderFileDir = Project.Properties["package." + sharedPchPackage + "." + sharedPchModule + ".pchheaderdir"];
				Config.PrecompiledHeader.UseForcedInclude = true;

				if (!mCombinedState.EnablePch)
				{
					SetModuleDependencies(0, "interfacedependencies", UseSharedPch.SharedModule);
				}
				else
				{
					if (HasPartial && !IsPartial)
					{
						// Check if partial module loading already setup sourcefiles with create-pch,
						// if we see that we need to remove it.
						FileSet fs = GetModuleFileset("sourcefiles");
						FileSetItem createPchItem = fs.Includes.Where(
							fitm => !String.IsNullOrEmpty(fitm.OptionSet) &&
							Project.NamedOptionSets.ContainsKey(fitm.OptionSet) &&
							Project.NamedOptionSets[fitm.OptionSet].Options.ContainsKey("create-pch") &&
							(Project.NamedOptionSets[fitm.OptionSet].Options["create-pch"].ToLower() == "on" || Project.NamedOptionSets[fitm.OptionSet].Options["create-pch"].ToLower() == "true"))
							.FirstOrDefault();
						if (createPchItem != null)
							fs.Includes.Remove(createPchItem);
					}
					// Also make sure current module didn't setup a source file with create-pch either.
					FileSetItem createPchFileItem = SourceFiles.Includes.Where(
						fitm => !String.IsNullOrEmpty(fitm.OptionSet) &&
						Project.NamedOptionSets.ContainsKey(fitm.OptionSet) &&
						Project.NamedOptionSets[fitm.OptionSet].Options.ContainsKey("create-pch") &&
						(Project.NamedOptionSets[fitm.OptionSet].Options["create-pch"].ToLower() == "on" || Project.NamedOptionSets[fitm.OptionSet].Options["create-pch"].ToLower() == "true"))
						.FirstOrDefault();
					if (createPchFileItem != null)
						SourceFiles.Includes.Remove(createPchFileItem);

					string configSystem = Project.Properties["config-system"];
					bool canUseSharedPchBinary = Project.Properties.GetBooleanPropertyOrDefault(configSystem + ".support-shared-pch", false)
						&& Project.Properties.GetBooleanPropertyOrDefault("eaconfig.use-shared-pch-binary", true)
						&& UseSharedPch.UseSharedBinary;

					if (ModuleExtensions.ShouldForceDisableOptimizationForModule(Project, PackageName, Group, ModuleName))
					{
						// We probably can check if the config has optimization already disabled.  But if the module
						// is listed to be forced disabled, then most likely optimization is currently on and don't
						// bother wasting more time to test.  These are usually temporary usage.
						canUseSharedPchBinary = false;
					}

					// Cannot use this with modules that has mix of source files that has use pch and not use pch.
					canUseSharedPchBinary = canUseSharedPchBinary && !SourceFiles.Includes.Any(fitm =>
						!String.IsNullOrEmpty(fitm.OptionSet) &&
						Project.NamedOptionSets.ContainsKey(fitm.OptionSet) &&
						!Project.NamedOptionSets[fitm.OptionSet].GetBooleanOptionOrDefault("use-pch", false));

					if (canUseSharedPchBinary)
					{
						Config.PrecompiledHeader.PchFile = Project.Properties["package." + sharedPchPackage + "." + sharedPchModule + ".pchfile"];

						// On vc compiler, we need to do some hacks to add a custom build file to copy the pdb around.
						if (Project.Properties["config-compiler"] == "vc")
						{
							// Add build dependency to this module.  On VC compiler, a lib will be created.
							SetModuleDependencies(3, "builddependencies", UseSharedPch.SharedModule);

							// If the build is creating pdb file, we need to make sure that this module's pdb is
							// seeded from the shared pch's pdb files.  But only do this if the build is creating pdb.
							bool globalOptionBuildPdb =
								Project.Properties.GetBooleanPropertyOrDefault("eaconfig.debugsymbols", true) &&
								!Project.Properties.GetBooleanPropertyOrDefault("eaconfig.c7debugsymbols", false);
							string currModuleBuildOptionSetName = Project.NamedOptionSets.ContainsKey(finalbuildtype + "-temp") ? finalbuildtype + "-temp" : finalbuildtype;
							bool moduleOptionBuildPdb =
								Project.NamedOptionSets[currModuleBuildOptionSetName].GetBooleanOptionOrDefault("debugsymbols", true) &&
								!Project.NamedOptionSets[currModuleBuildOptionSetName].GetBooleanOptionOrDefault("c7debugsymbols", false);
							if (globalOptionBuildPdb && moduleOptionBuildPdb)
							{
								string pchLibsFilesetName = String.Format("package.{0}.{1}.libs", sharedPchPackage, sharedPchModule);
								if (!Project.NamedFileSets.ContainsKey(pchLibsFilesetName))
								{
									// Cannot continue.  The FileSet() constructor below cannot have null object as input argument!
									throw new BuildException(String.Format("[SharedPch Error] Unable to locate dependent shared PCH module '{0}' export fileset specification: {1}.  Please make sure Initialize.xml for package {2} is setup properly for module {0} with correct buildtype info!", sharedPchModule, pchLibsFilesetName, sharedPchPackage));
								}
								FileSet libFileSet = new FileSet(Project.NamedFileSets[pchLibsFilesetName]);
								libFileSet.FailOnMissingFile = false;
								string sharedPchPdb = Path.ChangeExtension(libFileSet.FileItems[0].FullPath, ".pdb");
								string modulePdbPath = null;
								if (BuildTypeInfo.IsLibrary)
								{
									modulePdbPath = "%outputdir%\\%outputname%.pdb";
								}
								else // Could be program or dll
								{
									modulePdbPath = "%intermediatedir%\\%outputname%.pdb";
								}
								SetModuleProperty("custom-build-before-targets", "Build");
								StringBuilder batchScript = new StringBuilder();
								batchScript.AppendLine(String.Format("echo Copy {0} to {1}", Path.GetFileName(sharedPchPdb), Path.GetFileName(modulePdbPath)));
								batchScript.AppendLine(String.Format("{0} -N \"{1}\" \"{2}\"", Project.Properties[Project.NANT_PROPERTY_COPY], sharedPchPdb, modulePdbPath));
								SetModuleProperty("vcproj.custom-build-tool", batchScript.ToString());
								SetModuleProperty("vcproj.custom-build-outputs", modulePdbPath);
								var fileDeps = new StructuredFileSet();
								fileDeps.Includes.Add(new FileSetItem(PatternFactory.Instance.CreatePattern(sharedPchPdb)));
								SetModuleFileset("vcproj.custom-build-dependencies", fileDeps);

								// Internal flag to let SetModuleData process know this module has a custom build steps to do shared pch module's
								// pdb file copying.
								SetModuleProperty("has-sharedpch-pdb-copy", "true");
							}
						}
						else
						{
							if (!Project.Properties.GetBooleanPropertyOrDefault(configSystem + ".shared-pch-generate-lib", true))
							{
								// On most clang platforms, the .cpp that generate the .pch doesn't actually create a .obj file,
								// So no library is created.  But we still need build dependency to setup so that pch will be built.
								// So we set up build only dependency (ie don't link).
								SetModuleDependencies(6, "buildonlydependencies", UseSharedPch.SharedModule);
							}
							else
							{
								// On platforms like nx, they made their VSI to actually still build an obj and create a lib.
								// However, nx doesn't actually allow people to speciy the pch path.
								SetModuleDependencies(3, "builddependencies", UseSharedPch.SharedModule);
							}
						}
					}
					else // PCH is still enabled but cannot use shared pch binary output.
					{
						// If platform don't support shared pch binary, then setup source file to compile with
						string pchsource = Project.Properties["package." + sharedPchPackage + "." + sharedPchModule + ".pchsource"];
						SourceFiles.Include(pchsource, baseDir: Project.Properties["package." + sharedPchPackage + ".dir"], optionSetName: "create-precompiled-header");
						// Add use dependency to this module
						SetModuleDependencies(0, "interfacedependencies", UseSharedPch.SharedModule);
					}
				}
			}

			if (!String.IsNullOrEmpty(Config.PrecompiledHeader.PchFile))
			{
				SetModuleProperty("pch-file", Config.PrecompiledHeader.PchFile, append: false);
			}
			if (!String.IsNullOrEmpty(Config.PrecompiledHeader.PchHeaderFile))
			{
				SetModuleProperty("pch-header-file", Config.PrecompiledHeader.PchHeaderFile, append: false);
			}
			if (!String.IsNullOrEmpty(Config.PrecompiledHeader.PchHeaderFileDir))
			{
				SetModuleProperty("pch-header-file-dir", Config.PrecompiledHeader.PchHeaderFileDir, append: false);
			}
			if (Config.PrecompiledHeader.UseForcedInclude)
			{
				SetModuleProperty("pch-use-forced-include", Config.PrecompiledHeader.UseForcedInclude.ToString(), append: false);
			}

			SetModuleProperty("defines", Config.Defines.Value.LinesToArray().ToString(Environment.NewLine), Config.Defines.Append);

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

			SetModuleProperty("warningsuppression", StringUtil.EnsureSeparator(Config.Warningsuppression.Value, Environment.NewLine), Config.Warningsuppression.Append);
			SetModuleProperty("preprocess", Config.Preprocess.Value, Config.Preprocess.Append);
			SetModuleProperty("postprocess", Config.Postprocess.Value, Config.Postprocess.Append);

			SetModuleProperty("remove.defines", Config.RemoveOptions.Defines.Value.LinesToArray().ToString(Environment.NewLine), Config.RemoveOptions.Defines.Append);
			SetModuleProperty("remove.cc.options", StringUtil.EnsureSeparator(Config.RemoveOptions.CcOptions.Value, Environment.NewLine), Config.RemoveOptions.CcOptions.Append);
			SetModuleProperty("remove.as.options", StringUtil.EnsureSeparator(Config.RemoveOptions.AsmOptions.Value, Environment.NewLine), Config.RemoveOptions.AsmOptions.Append);
			SetModuleProperty("remove.link.options", StringUtil.EnsureSeparator(Config.RemoveOptions.LinkOptions.Value, Environment.NewLine), Config.RemoveOptions.LinkOptions.Append);
			SetModuleProperty("remove.lib.options", StringUtil.EnsureSeparator(Config.RemoveOptions.LibOptions.Value, Environment.NewLine), Config.RemoveOptions.LibOptions.Append);
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
					throw new BuildException(String.Format("Package {0}'s Initialize.xml has setup outputdir for module '{1}' to be '{2}'.  But this module's .build file is also trying to setup outputname but with different value '{3}'. You need to correct this error and specify outputname remapping in Initialize.xml only!",
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

			SetModuleProperty("rootnamespace", RootNamespace);

			// on android - add the special magic that allows us to build an apk from only native code, note 
			// this code isnt' run for partials because we only want it at the last level of module
			if (AndroidGradle.IncludeNativeActivityGlue && !IsPartial && BuildTypeInfo.IsProgram && Project.GetPropertyOrFail("config-system") == "android")
			{
				TaskUtil.Dependent(Project, "AndroidNDK");
				string ndkDir = Project.GetPropertyOrFail("package.AndroidNDK.appdir");
				string appGlueDir = Path.Combine(ndkDir, "sources", "android", "native_app_glue");

				// force include native app glue if this is a program module on android
				SourceFiles.Include(Path.Combine(appGlueDir, "android_native_app_glue.c"), asIs: true, force: true, optionSetName: "CProgram");

				// add native app glue include dir also
				IncludeDirs.Value += Environment.NewLine + PathNormalizer.Normalize(appGlueDir);
			}

			SetModuleProperty("includedirs", ExpandRelativeDirs(IncludeDirs));
			SetModuleProperty("librarydirs", ExpandRelativeDirs(LibraryDirs));
			SetModuleProperty("usingdirs", ExpandRelativeDirs(UsingDirs));
			SetModuleFileset("sourcefiles", SourceFiles);
			SetModuleFileset("asmsourcefiles", AsmSourceFiles);
			SetModuleFileset("objects", ObjectFiles);
			SetModuleFileset("headerfiles", HeaderFiles);
			SetModuleFileset("libs", Libraries);
			SetModuleFileset("dlls", Dlls);

			SetModuleFileset("comassemblies", ComAssemblies);
			SetModuleFileset("assemblies", Assemblies);
			SetModuleFileset("forceusing-assemblies", FuAssemblies);
			SetModuleFileset("additional-manifest-files", AdditionalManifestFiles);
			SetModuleFileset("shaderfiles", ShaderFiles);

			SetModuleProperty("sdkreferences", SdkReferences);

			if (ResourceFiles.Includes.Count > 0 || ResourceFiles.Excludes.Count > 0)
			{
				SetModuleFileset("resourcefiles", ResourceFiles);
				SetModuleProperty("resourcefiles.prefix", GetValueOrDefault(ResourceFiles.Prefix, ModuleName));
				SetModuleProperty("resourcefiles.basedir", SourceFiles.BaseDirectory ?? ResourceFiles.ResourceBasedir);
			}

			SetModuleProperty("res.includedirs", ResourceFiles.ResourceIncludeDirs, append: true);
			SetModuleProperty("res.defines", ResourceFiles.ResourceDefines, append: true);

			if (ResourceFilesNonEmbed.Includes.Count > 0 || ResourceFilesNonEmbed.Excludes.Count > 0)
			{
				SetModuleFileset("resourcefiles.notembed", ResourceFilesNonEmbed);
			}

			SetModuleFileset("natvisfiles", NatvisFiles);

			StructuredFileSet custombuildfiles = new StructuredFileSet();
			foreach (StructuredFileSet fs in CustomBuildFiles.FileSets)
			{
				custombuildfiles.AppendWithBaseDir(fs);
				custombuildfiles.AppendBase = fs.AppendBase;
			}
			if (custombuildfiles.BaseDirectory == null) custombuildfiles.BaseDirectory = Project.BaseDirectory;

			SetModuleFileset("custombuildfiles", custombuildfiles);

			SetModuleTarget(BuildSteps.PrebuildTarget, "prebuildtarget");
			SetModuleTarget(BuildSteps.PostbuildTarget, "postbuildtarget");

			if (BuildSteps.PostbuildTarget.SkipIfUpToDate)
			{
				SetModuleProperty("postbuild.skip-if-up-to-date", "true", append: false);
			}

			SetModuleProperty("vcproj.custom-build-tool", BuildSteps.CustomBuildStep.Script.Value);
			SetModuleProperty("vcproj.custom-build-outputs", ExpandRelativeDirs(BuildSteps.CustomBuildStep.OutputDependencies));
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

		protected void SetupBulkBuildData()
		{
			bool hasManualsourcefiles = BulkBuild.ManualSourceFiles.Includes.Count > 0;

			bool hasBulkBuildFilesets = BulkBuild.SourceFiles.NamedFilesets.Count > 0;

			if (BulkBuild._enable == null && (hasManualsourcefiles || hasBulkBuildFilesets || BulkBuild._partial == true || BulkBuild.MaxSize > 0))
			{
				// If any data are explicitly defined - enable.
				BulkBuild.Enable = true;
			}

			CombinedStateData.UpdateBoolean(ref mCombinedState.BulkBuildEnable, BulkBuild._enable);
			CombinedStateData.UpdateBoolean(ref mCombinedState.HasManualsourcefiles, (hasManualsourcefiles ? true : (bool?)null));
			CombinedStateData.UpdateBoolean(ref mCombinedState.HasBulkBuildFilesets, hasBulkBuildFilesets ? true : (bool?)null);

			if (BulkBuild.Enable)
			{
				if (hasManualsourcefiles)
				{
					SetModuleFileset("bulkbuild.manual.sourcefiles", BulkBuild.ManualSourceFiles);
				}

				if (hasBulkBuildFilesets)
				{
					var sb = new StringBuilder();
					foreach (var entry in BulkBuild.SourceFiles.NamedFilesets)
					{
						entry.Value.Suffix = String.Empty;
						SetModuleFileset("bulkbuild." + entry.Key + ".sourcefiles", entry.Value);
						sb.AppendLine(entry.Key);
					}
					SetModuleProperty("bulkbuild.filesets", sb.ToString());
				}

				SetModuleProperty("enablepartialbulkbuild", BulkBuild.Partial.ToString(), append: false);

				if (BulkBuild.MaxSize > 0)
				{
					SetModuleProperty("max-bulkbuild-size", BulkBuild.MaxSize.ToString(), append: false);
				}

				if (BulkBuild._splitByDirectories != null)
				{
					if (BulkBuild.SplitByDirectories)
					{
						SetModuleProperty("bulkbuild.split-by-directories", "True", append: false);
					}
				}

				if (BulkBuild.MinSplitFiles > 0)
				{
					SetModuleProperty("bulkbuild.min-split-files", BulkBuild.MinSplitFiles.ToString(), append: false);
				}

				if (BulkBuild.DeviateMaxSizeAllowance > 0)
				{
					SetModuleProperty("bulkbuild.deviate-maxsize-allowance", BulkBuild.DeviateMaxSizeAllowance.ToString(), append: false);
				}

				if (BulkBuild.LooseFiles.Includes.Count > 0)
				{
					SetModuleFileset("loosefiles", BulkBuild.LooseFiles);
				}
			}
		}

		protected void SetupDotNet()
		{
			SetModuleOptionSet("webreferences", DotNetDataElement.WebReferences);

			SetModuleProperty("copylocal", DotNetDataElement.CopyLocal);
			SetModuleProperty("allowunsafe", DotNetDataElement.AllowUnsafe);
		}

		protected void SetupVisualStudio()
		{
			string projType = BuildTypeInfo.IsCSharp ? "csproj" : "vcproj";

			SetModuleTarget(VisualStudioData.PregenerateTarget, projType + ".prebuildtarget");

			SetDefaultBuildStep(VisualStudioData.PreBuildStep, BuildSteps.PrebuildTarget);
			SetDefaultBuildStep(VisualStudioData.PostBuildStep, BuildSteps.PostbuildTarget);

			SetModuleProperty(projType + ".pre-build-step", VisualStudioData.PreBuildStep);
			SetModuleProperty(projType + ".post-build-step", VisualStudioData.PostBuildStep);
			SetModuleProperty("visualstudio-extension", VisualStudioData.Extension);

			SetModuleProperty(projType + ".pre-link-step", VisualStudioData.VcProj.PreLinkStep);
			SetModuleFileset(projType + ".additional-manifest-files", VisualStudioData.VcProj.AdditionalManifestFiles);
			SetModuleFileset(projType + ".input-resource-manifests", VisualStudioData.VcProj.InputResourceManifests);
			SetModuleFileset(projType + ".excludedbuildfiles", VisualStudioData.ExcludedBuildFiles);

			SetModuleOptionSet("msbuildoptions", VisualStudioData.MsbuildOptions);
		}


		protected ConditionalPropertyElement ExpandRelativeDirs(ConditionalPropertyElement input)
		{
			if (input != null)
			{
				input.Value = ExpandRelativeDirs(input.Value);
			}
			return input;
		}

		protected string ExpandRelativeDirs(string input)
		{
			if (String.IsNullOrEmpty(input.TrimWhiteSpace()))
			{
				return String.Empty;
			}
			var output = new StringBuilder();
			var packagedir = Properties["package.dir"];
			foreach (var idir in input.LinesToArray())
			{
				try
				{
					output.AppendLine(Path.Combine(packagedir, PathNormalizer.Normalize(idir, getFullPath: false)));
				}
				catch (ArgumentException)
				{
					Log.Error.WriteLine("Invalid path: " + idir);
					throw;
				}
			}
			return output.ToString();
		}

		private void SetDefaultBuildStep(ConditionalPropertyElement step, BuildTargetElement targetProperty)
		{
			if (!String.IsNullOrEmpty(targetProperty.Command.Value) && String.IsNullOrEmpty(step.Value))
			{
				step.Value = targetProperty.Command.Value;
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
			mCombinedState.BulkBuildEnable = BulkBuild.Enable;
			mCombinedState.HasManualsourcefiles = false;
			mCombinedState.HasBulkBuildFilesets = false;

			mCombinedState.EnablePch = Config.PrecompiledHeader.Enable && Project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true);
		}

		protected override void FinalizeModule()
		{
			if (Project.Properties.GetBooleanPropertyOrDefault("eaconfig.enable-pch", true))
			{
				SetModuleProperty("use.pch", mCombinedState.EnablePch ? "true" : "false");
				if (mCombinedState.EnablePch && null == GetModuleProperty("pch-file"))
				{
					string defaultPchName = ModuleName + ".pch";

					// Set the output PCH file name and path to match the obj output in case this Framework version is
					// used in conjuction with an older config package that didn't setup the pch build compiler flags
					// and pch build command line template.
					if (SourceFiles != null)
					{
						var pchSourceFile = SourceFiles.FileItems.Where(fitm => !String.IsNullOrEmpty(fitm.OptionSetName) && Project.NamedOptionSets.Contains(fitm.OptionSetName) && Project.NamedOptionSets[fitm.OptionSetName].Options["create-pch"] == "on");
						if (pchSourceFile.Any())
						{
							// There should be only one.
							defaultPchName = "%intermediatedir%/%filereldir%/" + Path.GetFileName(pchSourceFile.First().FileName) + ".pch";
						}
					}
					SetModuleProperty("pch-file", defaultPchName, append: false);
				}
			}

			if (mCombinedState.BulkBuildEnable && !mCombinedState.HasManualsourcefiles && !mCombinedState.HasBulkBuildFilesets)
			{
				FileSet sourcefiles = Project.GetFileSet(Group + "." + ModuleName + ".sourcefiles");
				FileSet loosefiles = Project.GetFileSet(Group + "." + ModuleName + ".loosefiles");

				// include loose files in source files
				sourcefiles.IncludeWithBaseDir(loosefiles);

				// create bulk fileset
				StructuredFileSet bulkbuildfileset = new StructuredFileSet();
				bulkbuildfileset.AppendWithBaseDir(sourcefiles);
				bulkbuildfileset.Exclude(loosefiles);
				SetModuleFileset("bulkbuild.sourcefiles", bulkbuildfileset);
			}

		}

		internal class CombinedStateData
		{
			internal bool EnablePch;
			internal bool BulkBuildEnable;
			internal bool HasManualsourcefiles;
			internal bool HasBulkBuildFilesets;

			internal static void UpdateBoolean(ref bool val, bool? newval)
			{
				if (newval != null)
				{
					val = (bool)newval;
				}
			}
		}
		internal CombinedStateData mCombinedState;
	}

}
