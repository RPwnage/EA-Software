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
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Structured;

using EA.Eaconfig.Build;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Structured
{
    /// <summary>A structured XML module information export specification to be used in Initialize.xml file (not to be confused with the &lt;Module&gt; task for the .build file)</summary>
    [TaskName("module")]
	public class ModulePublicData : ConditionalTaskContainer
	{
		private PackagePublicData _packageData;
		private OptionSet _buildOptionSet;

		protected BuildType _buildType;

		protected string GroupName { set; get; } = "runtime";

		protected string PackageName { get { return _packageData.PackageName; } }

		/// <summary>
		/// Name of the module.
		/// </summary>
		[TaskAttribute("name", Required = true)]
		public string ModuleName { set; get; }

		/// <summary>
		/// The default output name is based on the module name.  This attribute tells Framework to remap the output name to something different.
		/// </summary>
		[TaskAttribute("outputname", Required = false)]
		public string OutputName { set; get; } = null;

		/// <summary>
		/// List of supported processors to be used in adding default include directories and libraries.
		/// <list type="table">
		/// <listheader> <term>Default include directories</term> </listheader>
		/// <item> <term>"${package.dir}}/include"</term> </item>
		/// <item> <term>"${package.dir}}/include_${processor}"</term> </item>
		/// </list>
		///
		/// <list type="table">
		/// <listheader><term>Default libraries</term></listheader>
		/// <item><term>"${lib-prefix}${module}${lib-suffix}"</term></item>
		/// <item><term>"${lib-prefix}${module}_${processor}${lib-suffix}"</term></item>
		/// </list>
		/// </summary>
		[TaskAttribute("processors", Required = false)]
		public String Processors
		{
			get { return _processors ?? _packageData.Processors; }
			set { _processors = value; }
		} String _processors;

		/// <summary>
		/// (Deprecated) If set to true default values for include directories and libraries will be added to all modules. 
		/// </summary>
		[TaskAttribute("add-defaults", Required = false)]
		public bool AddDefaults
		{
			get { return (_adddefaults ?? _packageData.AddDefaults); }
			set { _adddefaults = value; }
		} internal bool? _adddefaults;

		/// <summary>
		/// If set to true the project will not be added to the solution during solution generation.  This will only really work for utility style modules since modules with actual build dependency requirements will fail visual studio build dependency requirements.
		/// </summary>
		[TaskAttribute("exclude-from-solution", Required = false)]
		public bool ExcludeFromSolution { set; get; } = false;

		private bool AddDefaultsIsUndefined
		{
			get 
			{
				return (_adddefaults == null && _packageData._adddefaults == null);
			} 
		}

		// TODO: remove legacy initialize elements
		// -dvaliant 2016//04/06
		/* think lib dirs isn't really necessry and confuses the logic of publicdata and setting module data,
		be good to remove this in favour of just using the libs fileset */
		/// <summary>
		/// List of additional library names to be added to default set of libraries. Can be overwritten by 'libnames' attribute in 'module' task.
		/// See documentation on Modules for explanation when default values are used.
		/// <list type="table">
		/// <listheader><term>Default libraries</term></listheader>
		/// <item><term>"${lib-prefix}${module}${lib-suffix}"</term></item>
		/// <item><term>"${lib-prefix}${libname}${lib-suffix}"</term></item>
		/// </list>
		/// </summary>
		[TaskAttribute("libnames", Required = false)]
		public string LibNames
		{
			get { return _libnames ?? _packageData.LibNames; }
			set { _libnames = value; }
		} String _libnames;


		/// <summary>
		/// Sets the buildtype used by this module. If specified Framework will try to work out which outputs (libs, dlls, etc) to automatically export.
		/// </summary>
		[TaskAttribute("buildtype", Required = false)]
		public string BuildType { get; set; }

		/// <summary>
		/// Set preprocessor defines exported by this module
		/// </summary>
		[Property("defines")]
		public ConditionalPropertyElement Defines { get; } = new ConditionalPropertyElement();

		/// <summary>
		/// Set include directories exported by this module. Empty element will result in default include directories added.
		/// </summary>
		[Property("includedirs")]
		public ConditionalPropertyElement Includedirs { get; } = new ConditionalPropertyElement();

		/// <summary>
		/// Set internal include directories exported by this module.
		/// </summary>
		[Property("includedirs.internal")]
		public DeprecatedPropertyElement InternalIncludeDirs { get; } = new DeprecatedPropertyElement(Log.DeprecationId.InternalDependencies, Log.DeprecateLevel.Minimal, "Internal dependencies have been removed as no usage of the feature could be found.");

		/// <summary>Declare packages or modules that modules which have an interface dependency on this module will also need to depend on.</summary>
		[Property("publicdependencies", Required = false)]
		public ConditionalPropertyElement PublicDependencies { get; } = new ConditionalPropertyElement();

		/// <summary>Declare packages or modules that modules which have a build dependency on this module will also need to depend on.</summary>
		[Property("publicbuilddependencies", Required = false)]
		public ConditionalPropertyElement PublicBuildDependencies { get; } = new ConditionalPropertyElement();

		/// <summary>Declare packages or modules that modules which have an internal dependency on this module will also need to internally depend on.</summary>
		[Property("internaldependencies", Required = false)]
		public DeprecatedPropertyElement InternalDependencies { get; } = new DeprecatedPropertyElement(Log.DeprecationId.InternalDependencies, Log.DeprecateLevel.Minimal, "Internal dependencies have been removed as no usage of the feature could be found.");

		// TODO: remove legacy initialize elements
		// -dvaliant 2016//04/06
		/* think lib dirs isn't really necessry and confuses the logic of publicdata and setting module data,
		be good to remove this in favour of just using the libs fileset */
		/// <summary>
		/// Set library directories exported by this module
		/// </summary>
		[Property("libdirs")]
		public ConditionalPropertyElement Libdirs { get; } = new ConditionalPropertyElement();

		/// <summary>
		/// Set 'using' (/FU) directories exported by this module
		/// </summary>
		[Property("usingdirs")]
		public ConditionalPropertyElement Usingdirs { get; } = new ConditionalPropertyElement();

		/// <summary>Set libraries exported by this module. If &lt;libs/&gt; is present and empty, default library values are added.</summary>
		[FileSet("libs")]
		public StructuredFileSetCollection Libs { get; } = new StructuredFileSetCollection();

		/// <summary>Set libraries (external to current package) exported by this module.</summary>
		[FileSet("libs-external")]
		public StructuredFileSetCollection LibsExternal { get; } = new StructuredFileSetCollection();

		/// <summary>Set dynamic libraries exported by this module. If &lt;dlls/&gt; is present and empty, default dynamic library values are added.</summary>
		[FileSet("dlls")]
		public StructuredFileSetCollection Dlls { get; } = new StructuredFileSetCollection();

		/// <summary>Set dynamic libraries (external to current package) exported by this module.</summary>
		[FileSet("dlls-external")]
		public StructuredFileSetCollection DllsExternal { get; } = new StructuredFileSetCollection();

		/// <summary>Set assemblies exported by this module. If &lt;assemblies/&gt; is present and empty, default assembly values are added.</summary>
		[FileSet("assemblies")]
		public StructuredFileSetCollection Assemblies { get; } = new StructuredFileSetCollection();

		/// <summary>Set assemblies (external to current package) exported by this module.</summary>
		[FileSet("assemblies-external")]
		public StructuredFileSetCollection AssembliesExternal { get; } = new StructuredFileSetCollection();

		/// <summary>Set natvis files exported by this module.</summary>
		[FileSet("natvisfiles")]
		public StructuredFileSetCollection NatvisFiles { get; } = new StructuredFileSetCollection();

		/// <summary>Set full path to rewrite rules ini files that provide computed-include-rules for SN-DBS. See SN-DBS help documentation regarding "Customizing dependency discovery."</summary>
		[FileSet("sndbs-rewrite-rules-files")]
		public StructuredFileSetCollection SnDbsRewriteRulesFiles { get; } = new StructuredFileSetCollection();

		/// <summary>Set programs exported by this module. Unlike the above filesets, if &lt;programs/&gt; is present and empty, nothing will be added.</summary>
		[FileSet("programs")]
		public StructuredFileSetCollection Programs { get; } = new StructuredFileSetCollection();

		/// <summary>Set assetfiles exported by this module.</summary>
		[FileSet("assetfiles")]
		public FileSet Assetfiles { get; } = new FileSet();

		/// <summary>Set content files exported by this module.</summary>
		[FileSet("contentfiles")]
		public FileSet ContentFiles { get; } = new FileSet();

		/// <summary>set Java archive (.jar) files exported by this module. This is currently used on Android only. .</summary>
		[FileSet("java.archives")]
		public DeprecatedPropertyElement JavaArchives { get; } = new DeprecatedPropertyElement(Log.DeprecationId.PublicJavaElements, Log.DeprecateLevel.Minimal,
			"All java dependencies should now be configured using Gradle files.",
			condition: Project => Project.Properties["config-system"] == "android");

		/// <summary>set Java class files (compiled .java files) exported by this module. This is currently used on Android only. .</summary>
		[FileSet("java.classes")]
		public DeprecatedPropertyElement JavaClasses { get; } = new DeprecatedPropertyElement(Log.DeprecationId.PublicJavaElements, Log.DeprecateLevel.Minimal,
			"All java dependencies should now be configured using Gradle files.",
			condition: Project => Project.Properties["config-system"] == "android");

		/// <summary>Set Android resource directories exported by this module. This is often used in Android Extension Libraries </summary>
		[Property("resourcedir")]
		public DeprecatedPropertyElement Resourcedir { get; } = new DeprecatedPropertyElement(Log.DeprecationId.PublicJavaElements, Log.DeprecateLevel.Minimal,
			"All java dependencies should now be configured using Gradle files.",
			condition: Project => Project.Properties["config-system"] == "android");

		[Property("java.rfile")]
		public DeprecatedPropertyElement JavaRFile { get; private set; } = new DeprecatedPropertyElement(Log.DeprecationId.PublicJavaElements, Log.DeprecateLevel.Minimal,
			"All java dependencies should now be configured using Gradle files.",
			condition: Project => Project.Properties["config-system"] == "android");

		[Property("java.packagename")]
		public DeprecatedPropertyElement JavaPackageName { get; private set; } = new DeprecatedPropertyElement(Log.DeprecationId.PublicJavaElements, Log.DeprecateLevel.Minimal,
			"All java dependencies should now be configured using Gradle files.",
			condition: Project => Project.Properties["config-system"] == "android");

		public ModulePublicData()
			: this(null)
		{ 
		}

		protected ModulePublicData(string buildType)
		{
			BuildType = buildType;
		}

		protected override void ExecuteTask()
		{
			if (!IfDefined || UnlessDefined)
			{
				return;
			}
			
			SetPackageData();
			SetGroupName();
			if (BuildType != null)
			{
				SetModuleProperty("buildtype", BuildType); // so we can compare it to the build type in the build script later and throw a warning if they are different
			}

			base.ExecuteTask();

			Init();

			Project.Properties[GetModulePropName("structured-initialize-xml-used")] = "true";

			var packageDir = Project.GetPropertyValue(GetPackagePropName("dir"));
			var buildLibDir = Project.GetPropertyValue(GetPackagePropName("libdir"));
			var buildBinDir = Project.GetPropertyValue(GetPackagePropName("bindir"));

			if (!String.IsNullOrEmpty(OutputName))
			{
				SetModuleProperty("outputname", OutputName);
			}

			AddBaseDirToRelativePath(packageDir, Includedirs);
			AddBaseDirToRelativePath(buildLibDir, Libdirs);
			AddBaseDirToRelativePath(buildLibDir, Usingdirs);

			SetModuleProperty("defines", Defines);
			SetModuleProperty("includedirs", Includedirs, () => DefaultIncludeDirectories(packageDir));
			SetModuleProperty("exclude-from-solution", ExcludeFromSolution.ToString().ToLower());

			AddBasedirToFileset(Assemblies, buildBinDir);
			AddBasedirToFileset(Dlls, buildBinDir);
			AddBasedirToFileset(Libs, buildLibDir);

			AddBasedirToFileset(Assetfiles, packageDir);

			SetModuleProperty("libdirs", Libdirs);
			SetModuleProperty("usingdirs", Usingdirs);

			// these properties are not used for any module logic, all that is done in SetPropagatedInterfaceProperty, these properties merely
			// record the public dependencies so we can validate them late against actual dependencies
			SetModuleProperty("publicdependencies", PublicDependencies);
			SetModuleProperty("publicbuilddependencies", PublicBuildDependencies);

			SetModuleFileset("libs", Libs, ShouldAutoIncludeLibraries(), () => DefaultLibraries(), respectAddDefaults: true); // only libs respects "AddDefaults" option to preserve backwards compatiblity - we will remove this in future
			SetModuleFileset("libs.external", LibsExternal);
			SetModuleFileset("dlls", Dlls, ShouldAutoIncludeDlls(), () => DefaultDynamicLibraries());
			SetModuleFileset("dlls.external", DllsExternal);
			SetModuleFileset("assemblies", Assemblies, ShouldAutoIncludeAssemblies(), () => DefaultAssemblies());
			SetModuleFileset("assemblies.external", AssembliesExternal);
			SetModuleFileset("natvisfiles", NatvisFiles);
			SetModuleFileset("sndbs-rewrite-rules-files", SnDbsRewriteRulesFiles);
			SetModuleFileset("programs", Programs, ShouldAutoIncludePrograms(), () => DefaultPrograms());
			SetModuleFileset("assetfiles", Assetfiles);
			SetModuleFileset("contentfiles", ContentFiles);

			UpdateBuildmodulesProperty();
		}

		// returns true if we expect the build type specified (if any) for the publicdata is a type
		// that should export an assembly
		private bool ShouldAutoIncludeAssemblies()
		{
			if (_buildType != null)
			{
				if (_buildType.IsCSharp || _buildType.IsManaged)
				{
					return true;
				}
			}
			return false;
		}

		// returns true if we expect the build type specifed (if any) for the publicdata is a type
		// that should export a library
		private bool ShouldAutoIncludeLibraries()
		{
			if (_buildType != null)
			{
				return !_buildType.IsManaged && (_buildType.IsLibrary || _buildType.IsDynamic);
			}
			return false;
		}

		// returns true if we expect the build type specifed (if any) for the publicdata is a type
		// that should export a dynamic library
		private bool ShouldAutoIncludeDlls()
		{
			if (_buildType != null)
			{
				return !_buildType.IsManaged && _buildType.IsDynamic;
			}
			return false;
		}

		// returns true if we expect the build type specified (if any) for the publicdata is a type
		// that should export a program
		private bool ShouldAutoIncludePrograms()
		{
			if (_buildType != null)
			{
				return _buildType.IsProgram && _buildOptionSet != null;
			}
			return false;
		}

		private IList<string> GetPublicDependencyGroupModules(string packageName, BuildGroups group)
		{
			var publicRuntimeModulesProperty = PropertyKey.Create("package.", packageName, ".", group.ToString(), ".buildmodules");
			return (Project.Properties[publicRuntimeModulesProperty] ?? String.Empty).ToArray(); // to array is fw string extension method for processing list properties
		}

		private IEnumerable<string> DefaultLibraries()
		{
			List<string> defaultLibraries = new List<string>();

			foreach (string libName in (ModuleName + " " + LibNames ?? String.Empty).ToArray())
			{
				string defaultLibPath = ModulePath.Public.GetModuleLibPath(
					Project, 
					_packageData.PackageName, 
					libName, 
					GroupName, 
					_buildType,
					outputNameOverride: Project.Properties.GetPropertyOrDefault(GetModulePropName("outputname"), null),
					outputDirOverride: Project.Properties.GetPropertyOrDefault(GetModulePropName("outputdir"), null)
					).FullPath;
				defaultLibraries.Add(defaultLibPath);

				if (!String.IsNullOrEmpty(Processors))
				{
					foreach (string processor in Processors.ToArray())
					{
						string processorLibPath = ModulePath.Public.GetModuleLibPath(
							Project, 
							_packageData.PackageName, 
							String.Format("{0}_{1}", libName, processor), 
							GroupName, 
							_buildType,
							outputNameOverride: Project.Properties.GetPropertyOrDefault(GetModulePropName("outputname"), null),
							outputDirOverride: Project.Properties.GetPropertyOrDefault(GetModulePropName("outputdir"), null)
							).FullPath;
					}
				}
			}
			return defaultLibraries.ToArray();
		}

		private IEnumerable<string> DefaultDynamicLibrariesOrAssemblies(string suffix)
		{
			string defaultDllPath = ModulePath.Public.GetModuleBinPath(
				Project, 
				_packageData.PackageName, 
				ModuleName,
				GroupName, 
				suffix, 
				_buildType, 
				outputNameOverride: Project.Properties.GetPropertyOrDefault(GetModulePropName("outputname"),null), 
				outputDirOverride: Project.Properties.GetPropertyOrDefault(GetModulePropName("outputdir"), _packageData.ConfigBinDir)
				).FullPath;
			return new string[] { defaultDllPath };
		}

		private IEnumerable<string> DefaultDynamicLibraries()
		{
			string dynamicLibrarySuffix = Project.Properties["dll-suffix"];
			return DefaultDynamicLibrariesOrAssemblies(dynamicLibrarySuffix);
		}

		private IEnumerable<string> DefaultAssemblies()
		{
			const string ASSEMBLY_SUFFIX = ".dll";
			return DefaultDynamicLibrariesOrAssemblies(ASSEMBLY_SUFFIX);
		}

		private IEnumerable<string> DefaultPrograms()
		{
			if (_buildType == null || _buildOptionSet == null)
			{
				// Unlike the other filesets, we're not going to support auto default with empty programs fileset
				// because we can't guarantee that it is correct without buildtype info.  The capilano config
				// build for example have config-options-programs defined 'linkoutputname' option to
				// override build output path.  Without buildtype info, we can't get the 'linkoutputname'
				// option info to get the proper path.  So we're not going to support it.
				return new string[] { };
			}
			else
			{
				string programSuffix = Project.Properties["exe-suffix"];
				string defaultExePath = ModulePath.Public.GetModuleBinPath(
					Project, 
					_packageData.PackageName, 
					ModuleName, 
					GroupName, 
					programSuffix, 
					_buildType, 
					outputNameOverride: Project.Properties.GetPropertyOrDefault(GetModulePropName("outputname"), null),
					outputDirOverride: Project.Properties.GetPropertyOrDefault(GetModulePropName("outputdir"), _packageData.ConfigBinDir)
					).FullPath;
				return new string[] { defaultExePath };
			}
		}

		private string DefaultIncludeDirectories(string packageDir)
		{
			var sb = new StringBuilder();
			sb.AppendLine(packageDir + "/include");
			if(!String.IsNullOrEmpty(Processors))
			{
				foreach (var processor in Processors.ToArray())
				{
					sb.AppendLine(packageDir + "/include_"+processor);
				}
			}
			return sb.ToString().TrimWhiteSpace();
		}

		private void UpdateBuildmodulesProperty()
		{
			var propKey = PropertyKey.Create("package.", _packageData.PackageName, ".", GroupName, ".buildmodules");

			Project.Properties.SetOrAdd(propKey, (oldValue) =>
			{
				if (oldValue.Contains(ModuleName))
				{
					// More presize test
					if (oldValue.ToArray().Contains(ModuleName))
					{
						Project.Log.ThrowWarning(Log.WarningId.DuplicateModuleInBuildModulesProp, Log.WarnLevel.Normal, "{0} Module {2} is specified more than once in your publicdata SXML block.  It could also mean that Property '{1}' already contains module '{2}' in classic Framework 2 syntax.", Location.ToString(), propKey.AsString(), ModuleName);
						return oldValue;
					}
				}

				return oldValue + Environment.NewLine + ModuleName;
			});
		}


		private void AddBaseDirToRelativePath(string basedir, PropertyElement val)
		{
			if (!String.IsNullOrWhiteSpace(val.Value))
			{
				var sb = new StringBuilder();
				foreach (var dir in val.Value.LinesToArray())
				{
					sb.AppendLine(Path.Combine(basedir, Project.ExpandProperties(dir)));
				}
				val.Value = sb.ToString().TrimWhiteSpace();
			}
		}

		private void AddBasedirToFileset(FileSet fileset, string baseDir)
		{
			if (fileset.BaseDirectory == null) fileset.BaseDirectory = baseDir;
		}

		private void AddBasedirToFileset(StructuredFileSetCollection structuredFilesets, string baseDir)
		{
			foreach (StructuredFileSet fileset in structuredFilesets.FileSets)
			{
				AddBasedirToFileset(fileset, baseDir);
			}
		}

		

		protected void SetModuleProperty(string name, PropertyElement value, Func<string> defaultValue = null)
		{
			if (value != null)
			{
				string val = null;

				if (value.Value != null)
				{
					val = value.Value.TrimWhiteSpace();
				}

				// If add-defaults is undefined, we only want to add the defaults if the property isn't already specified.
				if (defaultValue != null && (AddDefaults || (AddDefaultsIsUndefined && value.Project != null && value.Value == null)))
				{
					val = (val??String.Empty) + Environment.NewLine + defaultValue();
				}

				if (val != null)
				{
					SetModuleProperty(name, val);
				}
			}
		}

		protected void SetModuleProperty(string name, string val)
		{
			Project.Properties[GetModulePropName(name)] = Project.ExpandProperties(val);
		}

		private void SetModuleFileset(string name, StructuredFileSetCollection filesets, bool includePatternsByDefault = false, Func<IEnumerable<string>> patterns = null, bool respectAddDefaults = false)
		{
			if (filesets != null)
			{
				if (filesets.FileSets.Count > 0)
				{
					FileSet aggregateFileSet = new FileSet();
					foreach (FileSet fileSet in filesets.FileSets)
					{
						FileSet resolvedSet = ResolveFileSet(fileSet, includePatternsByDefault, patterns, respectAddDefaults);
						if (resolvedSet != null)
						{
							aggregateFileSet.Append(resolvedSet);
						}
					}
					if (aggregateFileSet.Includes.Count + aggregateFileSet.Excludes.Count > 0)
					{
						Project.NamedFileSets[CreateModuleKeyName(name)] = aggregateFileSet;
					}
				}
				else
				{
					// Originally when a fileset was not defined we would still add the defaults, this case is simply to preserve that behavior for backward compatibility
					FileSet resolvedSet = new FileSet();
					resolvedSet = ResolveFileSet(resolvedSet, includePatternsByDefault, patterns, respectAddDefaults);
					if (resolvedSet != null)
					{
						Project.NamedFileSets[CreateModuleKeyName(name)] = resolvedSet;
					}
				}
			}
		}

		private void SetModuleFileset(string name, FileSet fileset)
		{
			FileSet resolvedSet = ResolveFileSet(fileset, false, null); // single filesets can't be resolved with defaults
			if (resolvedSet != null)
			{
				Project.NamedFileSets[CreateModuleKeyName(name)] = fileset;
			}
		}

		// for and individual fileset figures out if we should include it or use defaults
		private FileSet ResolveFileSet(FileSet fileset, bool includePatternsByDefault, Func<IEnumerable<string>> patterns, bool respectAddDefaults = false)
		{
			if (fileset != null)
			{
				// conditionals stop element from being initilaized, but it is still a valid fileset check conditionals here because we don't
				// want to add default if conditions are false e.g <libs unless="true"/>
				if (fileset.IfDefined && !fileset.UnlessDefined)
				{
					// check if fileset was declared empty, this syntax means user wants defaults to be added
					bool wasDefined = fileset.Project != null; // user specified this fileset
					bool wasEmpty = !fileset.IncludesDefined; // user specified no includes, regardless of if/unless
					bool fileSetWasDefinedEmpty = (!respectAddDefaults || AddDefaultsIsUndefined) && wasDefined && wasEmpty;

					// if build type should export these then include defaults. fileSet.Project will be null if option was left empty.
					bool buildTypeIncludesPatternByDefault = (!respectAddDefaults || AddDefaultsIsUndefined) && !wasDefined && includePatternsByDefault;

					// include defaults if applicable
					if (patterns != null && ((AddDefaults && respectAddDefaults) || fileSetWasDefinedEmpty || buildTypeIncludesPatternByDefault))
					{
						IEnumerable<string> includePaths = patterns();
						foreach (string pattern in includePaths)
						{
							fileset.Include(pattern);
						}
					}

					if (fileset.Includes.Count + fileset.Excludes.Count > 0)
					{
						return fileset;
					}
				}
			}
			return null;
		}

		protected string CreateModuleKeyName(string name)
		{
			return "package." + _packageData.PackageName + "." + ModuleName + "." + name;
		}

		protected PropertyKey GetModulePropName(string name)
		{
			return PropertyKey.Create(CreateModuleKeyName(name));
		}

		protected PropertyKey GetPackagePropName(string name)
		{
			return PropertyKey.Create("package.", _packageData.PackageName, ".", name);
		}

		private void SetPackageData()
		{
			_packageData = FindParentTask<PackagePublicData>();
		}

		private void SetGroupName()
		{
			var group = FindParentTask<BuildGroupBaseTask>();
			if (group != null)
			{
				GroupName = group.GroupName;
			}
		}

		private void Init()
		{
			if (_packageData == null)
			{
				throw new BuildException("Module public data task <module> must be defined inside <publicdata> task", Location);
			}			
			
			if (_adddefaults.HasValue)
			{
				Log.ThrowDeprecation
				(
					Log.DeprecationId.PublicDataDefaults, Log.DeprecateLevel.Normal,
					DeprecationUtil.TaskLocationKey(this),
					"{0} Use of 'add-defaults' attribute in <{1}> task in is deprecated. To add default libs use empty libs fileset '<libs/>' instead.", 
					Location, Name
				);
			}

			_buildType = BuildType != null ? GetModuleBaseType.Execute(Project, BuildType) : null;
			_buildOptionSet = _buildType != null ? OptionSetUtil.GetOptionSet(Project, _buildType.Name) : null;
		}
	}

	/*[TaskName("PublicCSharpLibrary")]
	public class CSharpLibraryPublicData : ModulePublicData
	{
		public CSharpLibraryPublicData() : base("CSharpLibrary") { }
	}

	[TaskName("PublicManagedCppAssembly")]
	public class ManagedCppAssemblyPublicData : ModulePublicData
	{
		public ManagedCppAssemblyPublicData() : base("ManagedCppAssembly") { }
	}*/
}
