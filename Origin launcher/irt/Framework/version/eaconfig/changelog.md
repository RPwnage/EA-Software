This is the changelog for the eaconfig package.

Note that eaconfig was merged into Framework as of 2019-PR7. This changelog therefore only contains changes to eaconfig up to that point.
Future eaconfig changes will be found in the Framework changelog.


# EWVAN-2019-PR6-CU2

### Changed

* Removed the need for 'config.targetoverrides' optionset at the start of package build files. It can still be used to exclude targets for backwards compatibility but really it should be removed and instread if you wish to override default targets you can just redeclare the target with the override="true" attribute.
* Changed msbuild-all target to use parallel foreach since otherwise it is much slower than buildall.

### Added

* Updated clang optionset to support -fprofile-sameple-use= switch for LTCG profiling.

### Fixed

* Updated the outsourcing tool to help improve long path issues:
  * Added 'output-flattened-package' option to allow creating standalone package being flattened without version folder. (Check example at TnT/Build/FBBuild/scripts/default-outsource-input-param.xml)
  * The generated source/header files are no longer stored in a "GeneratedFiles" sub-folder.  Removing this sub-folder name help shorten some include paths.
  * Fix an error in generated Initialize.xml where it was unnecessary adding publicdependency includedirs property ${package.[package_name].[module_name].includedirs} for module of the same package.
    That property was already expanded and code lead to adding extra paths what actually doesn't exist and caused unnecessary additon to command line length.
* Updated the outsourcing tool ('create-prebuilt-package') task's preserve-properties attribute to support module specific properties.

## EWVAN-2019-PR6-CU1

### Changed

* Changed to use responsefile for cc on pc/xb1/ps4.  Unix already used this and it seemed strange to not have it set for all platforms since it can avoid command line too long errors.
* Changed xb1 and pc define template to not quote the entire define.  This is now consistent with other platforms and defines are not passed by default with quotes.  Some build scripts may need to be updated if the quotes were expected.
* xge.numjobs property has been replaced by property eaconfig.make-numjobs.  This is because that property actually controls the number of jobs to spawn when running make.  Also XGE is not supported anymore so using xge in the name is misleading.
* Default number of jobs to use on a windows machine for make is changed from default value of `32`.  It is now set to use the environment variable `NUMBER_OF_PROCESSORS*2`, which seems to give a good level of parallelism, vs a hard-coded number 32.  Note this can be overriden by using the eaconfig.make-numjobs property.  For unix or macosx host machines the default is still unchanged at 32.
* All references and support (previously untested and in an unknown state) for XGE builds has been removed.  Only the `incredibuild` targets has been left to be able to build solutions via incredibuild via nant/Framework.

### Fixed

* Fixed pc assembler build tool template to not include %usingdir% token.  There are no usingdir assembler command line switch in ml64.exe.

## EWVAN-2019-PR5-CU1

### Added

* Added a new XB1-specific property, `eaconfig.capilano.layout-exclusion-filter` which specifies a set of default
  file filters to exclude from the XB1 deploy step. This gets either overridden by or appended to by the
  `<exclusionfilter>` entry in the Capilano layout definition, e.g. the partial module definitions in FBConfig's
  partial-modules-common.xml. This depends on the `append` attribute of the `<exclusionfilter>` tag (default is
  append=true).
  This value also gets added to the .buildlayout file as a "deployfilters" entry, which can be used by Icepick.

## EWVAN-2019-PR3-CU1

### Changed

* Merged common VC options logic between Capilano and PC into vc-common-options.
* Merged common Clang options logic between Kettle and Unix into clang-options.

# 6.00.00-pr7

### Changed

* osx_config and ios_config have been merged into eaconfig. Please remove ios_config and osx_config from your masterconfig when taking this version.

### Fixed

* Fix for the solution generation target when a solution name contains a space.

# 6.00.00-pr6

### Changed

* Removed legacy build tool properties from the pc and capilano build tools definitions. Note that if you take the new
  visual studio general proxy (100.00.00+) you will need atleast this version of eaconfig.
    
### Removed

* Removing the special version check to see whether the /WorkingDirectory option is supported on kettle, since this was
  added in a very old version of kettlesdk, before we even began posting it to the package server in 2013.
    
### Fixed

* Updated LTCG Profile Guided Optimization linker options for XB1 build to non-deprecated versions and added ability to
  supply a filename for the profile.
* Updated the Outsourcing task to take into account the new gendir property for generated files in case this is set to something different than builddir.

   
# 6.00.00-pr5

### Changed
    
* VS2019 Preview 2.0 support changes.
* Preliminary support for PS4 if you have the beta VSI/Debugger installers.


# 6.00.00-pr4

### Changed
    
* VS2019 support changes. mainly made config-vs-version now get its version from the VisualStudio package, and make
  anything that is VS15 actually be VS15 or greater check since VS2019 (dev16) really is much the same as VS2017.
  No need to create a bunch of new paths in the code when we update versions of Visual Studio. Also the detection
  mechanism in the Visual Studio package can now decipher the correct version etc. This means that dev15 VisualStudio
  package can be used as a single VS Proxy package for multiple VS versions going forward.

### Removed

* Removing the sandcastle target since we have removed sandcastle support from framework.

 
# 6.00.00-pr3

### Changed

* Removed all eaconfig.\<group>.groupname, eaconfig.\<group>.outputfolder, eaconfig.\<group>.sourcedir,
  eaconfig.\<group>.usepackagelibs properties from being set in eaconfig. The outputfolder property was already set in
  Framework, and was marked ReadOnly. They are now set by default in Framework but are not ReadOnly so that
  buildscripts can override them.      
* Use _ITERATOR_DEBUG_LEVEL macro instead of _SECURE_SCL macro for msvc platforms when using the iteratorboundschecking
  eaconfig option. _SECURE_SCL is marked as deprecated by Microsoft.
* Switched xb1 + pc setting for c++ language version from c++latest to c++17.  Microsoft has stated that c++latest is
  not really intended for shipping source code and should only be used for testing out the newest compiler features.
  If you are running Visual Studio Preview builds we still set c++latest by default so you can test the latest Preview
  Visual Studio as well as the Latest Compiler Language specs in the MSVC toolchain.
        
### Fixed

* Fixed the clean target execution of MakeStyle module's MakeCleanCommand could leave temp files on user's temp folder.
* Fixed "create-prebuilt-package" task so that when it is copying the natvis files, it should preserve original
  package's folder structure.
* Updated "create-prebuilt-package" task to provide option to preserve specific properties only created in .build file.
* Updated "create-prebuilt-package" task to provide option to specify the new package version suffix name.


# 6.00.00-pr1

This version contains major breaking changes from 5.xx.xx, please pay particular attention to the Changes section.

### Changes
 
* Kettle and Capilano configs package have been merged into eaconfig and Framework. The kettle_config and
  capilano_config packages are no longer needed. Because of this change, and other refactoring of eaconfig and
  Framework, we require users that take this new version of eaconfig also take a new version of framework with the
  corresponding changes.
* Updated build tool (cc, as, lib, link) property setup to use the new structured build tool tasks in latest Framework.
  Because of this change, updating to this version of eaconfig requires a new version of Framework that provided
  \<compiler>, \<assemblier>, \<librarian>, and \<linker> tasks (Framework 8.00.00-pr1 and up).


* Removed TargetFramework from UnitTest.runsettings, it was hard-coded to .Net Framework 4.5 and Frostbite now builds
  with .NET 4.6.1. Removing the specific TargetFramework setting does not negatively impact our mstests runs, and
  removes warnings from being emitted from vstest.console.exe when we run the mstests with newer versions Visual Studio.
* Updated the verify-includes target because new version of Framework updated the file name for nant generated include
  dependency file.
* The ScriptInit task has been moved to Framework. A new release of framework is required to work with this change.
* The ProgramData environment variable is now propagated to all exec and build tasks by default.
* The kettle and capilano runner tasks used for deploying and running on the consoles have been refactored so that all
  of the common process logic is shared between them and lives within framework.
* Most of the include steps performed inside the config files, like in pc-vc-dev-debug.xml, have been moved to load.xml
  in order to simplify the config files. If users are diverging eaconfig to add new configs they should update their
  configs to make sure that they are not including settings twice.
* Updated pc and kettle config setup to start setting up cc.system-includedirs property and
  buildset.cc.system-includedirs option in config-options-common so that Framework can take advantage of those
  properties / buildset option to properly setup vcxproj's IncludePath info in solution generation and also allow
  Framework to use special compiler switches for system includes when appropriate.
        
### Added

* Add Exclusion Filter and GameOS options to the Layout options available in XB1 platform options dialog in Visual
  Studio. Exclusion Filter allows you to exclude specific files from being deployed to the layout folder, and GameOS
  lets you point to a specific Game OS image that you want to deploy with your title for testing.
* Added wiring to be able to use Xbox.Services.API.Cpp Xbox One SDK Extension via the sdkextensions mechanism
* Added support for Aumid Override for XB1 Visual Studio solution generation.
* Added support for setting Deployment=RegisterNetworkShare in XB1 Visual Studio solution generation.

### Removed

* Removed the version of GenerateBuildOptionset that was defined using \<createtask> and simply called the C#
  implementation of the task. Framework can now run C# tasks with the task runner task so this won't be a breaking
  change however it requires the latest version of framework.
* Removed all uses of sysinfo task from eaconfig now that compatibility with framework has been broken.      
* Removing backward compatibility case for eaconfig.msvc.version property now that compatibility with framework has
  been broken.
* Refactoring Generate options code, specifically unix clang will have common clang settings rather than using gcc
  settings. This requires changes to generate options code in framework and thus breaks compatibility with older
  framework versions.


# 5.14.04

### Fixed

*   Fixed "create-prebuilt-package" task to update the evaluation of the package.[PackageName].PrebuiltIncludeSourceFiles fileset under each config-module level in case the fileset got changed depending on config.
*   Update Generate Build Layout Code to add Module Source Directory path to Build Layout File

# 5.14.03

### Changes

*   Updated with config loading changes. Please be aware that load.xml and combine.xml will be ignore and then deleted in a coming version of eaconfig because the steps in these files will be handled by Framework. Please avoid making divergences to these files. A new pre-combine.xml file has been added for anything that needs to happen before the combine step is executing. In the future Framework will also automatically load a post-combine.xml, if it exists, immediately after running the combine step.

# 5.14.02

### Fixed

*   Removing package signature file code from eaconfig's package targets and adding a warning message that the package target may be removed in the future in favor of the new, simpler and more reliable, "eapm package" command. If you are using your own package target that overrides or extends the eaconfig package target please update your scripts to make your target independent of the one in eaconfig.

# 5.14.00

### Added

*   Created new generate-build-layout.cs file for eaconfig infrastructure to create BuildLayout Files upon building which can be used for code coverage.

### Changed

*   Updated "create-prebuilt-package" task to not ship pdb files by default. If this is not the desired behaviour, user can provide a global property "eaconfig.create-prebuilt.skip-pdb" (or "eaconfig.create-prebuilt.[package_name].skip-pdb" for specific package) and assign it to true to re-activate packaging of pdb files.

### Removed

*   All unix-gcc and winrt configs have been removed from eaconfig.
*   The property eaconfig-run-PROCESSOR_ARCHITECTURE is no longer defined by eaconfig, use sys.env.PROCESSOR_ARCHITECTURE instead to access this environment variable, or define the property yourself if you need it for backward compatibility.
*   The property package.DotNetSDK.64bit is not longer defined by eaconfig. If you need this property for some backward compatibility you can define it yourself.
*   The property package.eaconfig.sdkdir is no longer defined by eaconfig. It was only being used internally for some pre-vs2010 directories so we have removed it. If you need this property for some backward compatibility you can define it yourself.

# 5.13.02

### Added

*   The linker option -Wl,--export-dynamic is now always added for unix builds. This was added to help with resolving callstack information in certain cases, by adding all symbols, not just used ones, to the dynamic symbol table. This can be turned off using the exportdynamic build option or the eaconfig.exportdynamic global property.

# 5.13.01

### Added

*   Added new property eaconfig.run.use-commandprogram that can be set to false to make eaconfig's run targets use the default runner rather than a module's .commandprogram property value if set.
*   The environment variable ProgramData is now automatically propagated through <exec> and other sub-process calls. Many Windows processes do not behave as expected without this set.

# 5.13.00

### Potentially Breaking Changes

*   All code related to pclint has been removed from eaconfig. This is an optional feature that has been available for a long time but it is unclear if anyone still uses so it is unlikely to break anyone.

### Added

*   Updated unix configs to support building with precompiled headers. This feature requires a Framework version after 7.16.00 if you are using makefiles (or Visual Studio) to build unix configs.
*   Add Colorization Option to eaconfig. Clang currently supports colorized output messages so added eaconfig.buildlogcoloring option to toggle this feature on. Unfortunately Visual Studio does not yet support it so we cannot enable it by default.

### Removed

*   Removed HelpToolKit support from the eaconfig doc target.
*   Removed the target generate-xml-from-framework-schema. It is an old target that was likely for our documentation generator but was never used. This should not impact anyone.
*   Removed the winprt run target from eaconfig and the slipstream-winprt-args taskdef that was only used by that target.
*   Removed all winprt configs from eaconfig.

### Changed

*   Make builds and unix configs no longer depend on vstomaketools for vsimake, they now depend on the new vsimake package instead.
*   Updated unix shared library build to use -soname [shared_library_file] to set DT_SONAME field so that during run time any executable linked with this shared lib will load with that name instead of using the dynamic library full path during link time.
*   Updated unix linker options to include -rpath '$ORIGIN/' to allow the built executable to also search for shared library from the executable's location.

# 5.12.00

### Added

*   Added build option to enable Build Timing info when running builds. Useful for collecting build statistics. Set option 'buildtiming' to 'on' in your buildoptions or pass -D:eaconfig.buildtiming=true on the command line to enable for both msvc and clang toolchains. Note that for clang this replaces the previously added clang.time-report property.
*   suppress vc warning 4275 in medium and high warning level: non dll-interface class 'xxx' used as base for dll-interface class 'yyy'
*   suppress vc warning 4251 in medium and high warning level: 'xxx': struct 'yyy' needs to have dll-interface to be used by clients of class 'zzz'

# 5.11.00

### Added

*   Added support for toggling the clang "time-report" option, using the property clang.time-report=true, to assist diagnosing compile time performance issues.
*   Updated the viewbuildinfo task to display property and optionset info in a proper text box so that users can scroll through it.

### Fixed

*   Updating the intellisense auto-copy feature so that it overwrites readonly files.

### Removed

*   Removed all references to FASTBuild.
*   Removing support for VisualGDB

# 5.10.02

### Added

*   Added a step to the opensln target that automatically installs or updates build script intellisense files.

### Fixed

*   updated profile guided optimization switches.

# 5.10.01

### Added

*   Added support to allow user to specify mstest and vstest run's timeout setting by using properties eaconfig.mstest-timeout-msec and eaconfig.vstest-timeout-msec respectively. The timeout parameters are specified in milliseconds.
*   Added the property eaconfig.msvc.version for determining which version of the microsoft visual c compiler is being used. This version can now differ from the installed visual studio version if you are using the compiler version from the separate MSBuildTools package.

### Removed

*   Removing old code paths for visual studio 8 in task-generateslninteropassemblies.xml and task-generatewebreferences.xml. They are using the package version to determine the visual studio version and there is the potential that checking the visual studio version in this way could cause problems in the future.

# 5.10.00

### Added

*   Added new flag "cpplanguge-strict" - if set this will force files to build as C++ when clanguage = off even if they have .c extension.

### Removed

*   Frostbite platform related settings were moved out of eaconfig and back into the FBConfig package. For Frostbite, these changes are only compatible with Frostbite 2018 PR5.

### Added

*   Enabled msvc /Zf (Faster PDB generation) option for Visual Studio 2017 v15.1 and beyond. This flag was exposed to optimize the msvc toolchain RPC communication with mspdbsrv.exe, and has been proven to improve build times. Set eaconfig.fastpdbgeneration=off to disable it if you run into issues. See https://docs.microsoft.com/en-us/cpp/build/reference/zf for the Microsoft documentation on this compiler flag.
*   Added an experimental feature "viewbuildinfo" target to help user interactively view the build info for each modules in a build graph.
*   Added the new flag 'shortchar' for enabling or disabling the -fshort-wchar option on gcc or clang.

### Changed

*   Reduced circular dependencies between FBConfig and eaconfig when building for frostbite configurations.
*   Update to support new PGO workflow for VS2015+
*   Updated FASTBuild solution generation to directly use Framework (version 7.13.00 and above)'s solution generation instead of using FASTBuild's build in solution generation. User can revert back to FASTBuild's build in solution generation using boolean property 'fastbuild.use.builtin.gensln'.

### Fixed

*   Fixed create-prebuilt-package task where it is not properly updating versionName field if there is a TagChecker modification containing old info in comment block.

# 5.09.01

### Fixed

*   Fixed an issue where generating a Unix Visual Studio solution on clean PC gives people an error message about vstomaketools package not being listed in masterconfig.
*   Removed an error check about setting 'config-vs-version' property. This sometimes caused unnecessary error message.
*   Small fix to frostbite.FB.ISAUTOBUILD property usage to not assume we are running in a mode that depends on FBConfig.

# 5.09.00

### Fixed

*   Disabling the VisualStudio package's export build settings step when calling the opensln target. It doesn't seem necessary to setup all of these settings when doing opensln, and if it fails to find the compilers it could fail even if the current platform is not using those compilers.

### Changed

*   Moved frostbite.xml back to FBConfig. This file really is very frostbite specific and better fits inside FBConfig so it is shipped/versioned with other changes made in frostbite, and not in eaconfig.

# 5.08.00

### Added

*   Added support for generating multiple Visual Studio solutions from within a single buildgraph. Frostbite Upgrade Note: This version of eaconfig has changes in frostbite.xml that requires changes in FBConfig to work properly. This was fixed afterwards with the next version of eaconfig, however it still requires changes in FBConfig to be pre-ported in order to work.

### Fixed

*   Fixed the outsourcing tool (the create-prebuilt-package task) to monitor if any package declared 'skip-runtime-dependency' in it's dependency block and preserve this info in the new build file.

### Changed

*   Removed some unused properties from frostbite.xml.

# 5.07.03

### Fixed

*   Fixed a bug where the app packaging copy asset targets was available to top level project only. It needs to be accessable by all projects.

# 5.07.02

### Added

*   You can now specify the pch header file separately from the pch task using the option "pch-header-file".

### Fixed

*   Fixed support of loading game teams custom "Frostbite" specific config override file (config-overrides\frostbite-overrides.xml relative to the 'extra-config-dir' masterconfig setting). This file is used to be loaded near beginning of frostbite.xml. It is now being loaded at the end of that file. Position of this file needs to be moved to the end in case user need to make modification to 'frostbite.defines' property that was setup during the processing of frostbite.xml.

# 5.07.01

### Changed

*   Relocated scope of FB_USERDOMAIN define. Previously, it was defined in "frostbite.defines" property and used by ALL Frostbite modules build. This define is now moved to a single file in Frostbite's Engine.Core module only.
*   Moved Frostbite's private layout folder setting from frostbite.xml in eaconfig to FBConfig.

### Fixed

*   Updated Visual Studio solution generation to not do any MSBuild property override for WindowsSDK path re-direction under portable solution generation.
*   Updated Visual Studio solution generation to also provide MSBuild property override for 'UniversalCRT_PropsPath', 'UniversalCRTSdkDir_10', 'WindowsSdkDir', 'DotNetSdkRoot', 'FrameworkSDKRoot' and 'SDK40ToolsPath'.
*   Fixed error where "custom" warning options settings for vc compiler got -W0 added to compiler options set.

# 5.07.00

### Fixed

*   In Visual Studio 2017 the linker flag /Debug will enable fastlink by default on debug builds, and full pdbs in release builds. This is not consistent with Visual Studio 2015\. Now when fastlink is disabled in eaconfig, even debug builds will be forced to have full pdb files.

### Changed

*   Updated outsourcing tool to export out list of source tree paths that the generated prebuilt packages are referencing to an output property. The tool is also updated to export out list of sources that are skipped without creating a prebuilt. Those packages are typically use dependent or interface dependent packages.

# 5.06.04

### Changed

*   Modified the outsourcing tool to removed support for 'modify-in-place' option but added new feature to allowing user to redirect 'includedirs' property back to original source tree.
*   Frostbite unix configurations now set -fno-omit-frame-pointer to keep the framepointer around to allow for backtraces to be performed. Required for callstack reporting on linux server builds.
*   A second attempt at fixing 'slnruntime-generateinteropassembly' task to not crash when verbose mode is turned on. Previous fix in 5.06.02 release added ${package.DotNet.referencedir} to the PATH before calling tlbimp.exe. We should be using the property ${package.DotNet.bindir} instead which points to the Global Assembly Cache.
*   Disabled debugfastlink by default for Frostbite PC builds.

### Fixed

*   Running msbuild target from eaconfig would fail to evaluate the package.VisualStudio.msbuild.exe property on non Microsoft platforms. This was due to the fact that the VisualStudio package was not depended on by anything prior to its usage. The VisualStudio package was modified to allow eaconfig to access some details it needs to be able to run msbuild.

# 5.06.03

### Changed

*   Slightly modified unix config's Visual Studio solution support. Previously, a task 'generate-unix-build-makefiles-for-vsproj' was inserted to 'backend.VisualStudio.pregenerate' list. It is now modified to no longer use 'backend.VisualStudio.pregenerate' tasks to create the make files. The gensln targets itself is now updated to generate the makefiles before creating the solution.

# 5.06.02

### Added

*   Added clean targets for FASTBuild. Targets are as follows: "fastbuild-clean", "fastbuild-clean-all", "fastbuild- <group>-clean", "fastbuild- <group>-clean-all". These targets will not work when used with Framework versions prior to the change that adds clean targets to Fastbuild Bff files.
*   Update compiler command line template to have substitution rules for force using assemblies (MSVC /FU Option). This requires at least Framework 7.08.00 to handle this subsitution.

### Fixed

*   Fixed COM dll's inter op assembly generation task 'slnruntime-generateinteropassembly' to not crash if verbose mode is on. **Note:** This fix also changed the task to be executed only once per process per output path.

# 5.06.01

### Added

*   Added some more Default Warning suppressions in the frostbite.uselegacywarningsuppression block. After testing the new warning suppression levels in Frostbite 2016_4 it was found that some warnings that were previously suppressed in fb were now triggering.

*   Added Win32 config files for Frostbite.

# 5.06.00

### Added

*   Updated to support loading game teams custom "Frostbite" specific config override file (config-overrides\frostbite-overrides.xml relative to the 'extra-config-dir' masterconfig setting available in latest Framework release). This file will get executed near beginning of config\configset\frostbite.xml to allow game team making game team specific default Frostbite settings.

*   Natvis files will now be added to prebuilt packages.

### Changed

*   Adjusted the default for link time code generation on pc to be turned off by default when doing a Frostbite no master build. Users noticed that the combination of link time code generation and no master builds was resulting in much larger static libraries that lead to linker failures.

### Fixed

*   Frostbite MSVC based configs now build with these warnings enabled wheras they were previously globally suppressed for frostbite configs:

    *   Level 4 Warning 4505 - un-used functionsSetting frostbite.uselegacywarningsuppression=true will restore the warning levels to their previous state. Reminder this legacy backwards compatibility will be removed at a future date, so be sure to fix your code.

    Frostbite Clang based configs now build with these warnings enabled wheras they were previously globally suppressed for frostbite configs:

    *   -Wmultichar - warn on multichar constants. Minor usage in BlazeSDK and Telemetry Extension. EADP will have to fix BlazeSDK
    *   -Wunused-function - warn on un-used functionsSetting frostbite.uselegacywarningsuppression=true will restore the warning levels to their previous state. Reminder this legacy backwards compatibility will be removed at a future date, so be sure to fix your code.
*   Added "Windows Presentation Foundation (WPF)" class GAC assembly folder to the default managed C++ project build. Previously, only the base GAC assembly folder was added, so if people were trying to use the WPF classes assembly, they would get unable to find assembly error message during build.

*   Fixed property package.eaconfig.vcdir to have the correct value when using newer versions of the VisualStudio package, specifically the 15.x versions.

*   Updated the configset\*.xml to not re-define config-name property if it was already defined. This property can be setup to be something different by the LoadPlatformConfig task for user defined configs.

# 5.05.01

### Fixed

*   Removed relative test results directory from default .runsettings. Prevents an issue where generated files we're being output to eaconfig source folder causing breaking taskdef compile.

# 5.05.00

### Added

*   Frostbite Clang based configs now build with these warnings enabled wheras they were previously globally suppressed for frostbite configs:

    *   -Wswitch - switch does not handle all enum values
    *   -Wdelete-non-virtual-dtor - delete called on 'const fb::Bla' that has virtual functions but non-virtual destructor
    *   -Wunused-variableFrostbite MSVC based configs now build with these warnings enabled wheras they were previously globally suppressed for frostbite configs:
    *   W4265 - 'class': class has virtual functions, but destructor is not virtual
    *   W4946 - reinterpret_cast used between related classes: 'class1' and 'class2'Setting frostbite.uselegacywarningsuppression=true will restore the warning levels to their previous state.
*   Frostbite configs now use warning level high for pc/xb1 just like on ps4/unix64.

*   Frostbite ps4 configs now reply on frostbite-clang-common.xml in eaconfig file to share some of the configuration between kettle_config and eaconfig for frostbite clang related builds to avoid some duplication.

*   Frostbite configurations now have a new frostbite.uselegacywarningsuppression property to re-enable the old frostbite global warning suppression. When updating frostbite you may want to set this temporarily in your masterconfig to help ease the transition.

*   Common_VC_Warning_Options is now moved from Framework's GenerateOptions.cs file into eaconfig's vc-options-common.cs file to help consolidate vc compiler settings for xb1 and pc together.

*   Added option "frostbite.user-user-defines" when using frostbite configurations. When set to false this prevents the FB_USER=<username> and FB_USER_<USERNAME> defines from being set outside of specific .cpp files that use those defines as values (this requires some changes to Engine .build files).

*   Added support for VS2017 VC compiler switch '/permissive-'. Permissive- removes MS specific compiler extensions that are not standards conforming. Can be disable via option msvc.permissive=off or by property eaconfig.msvc.permissive=false.

*   Added support for VS2017 VC compiler switch '/diagnostics:'. The value of option msvc.diagnostics or property eaconfig.msvc.diagnostics is used as argument. Default value is 'classic'. See [https://docs.microsoft.com/en-us/cpp/what-s-new-for-visual-cpp-in-visual-studio](https://docs.microsoft.com/en-us/cpp/what-s-new-for-visual-cpp-in-visual-studio) for other values.

*   Enabled setting the VC C++ language specification when using VS2017 via the cc.std property or option - i.e set set cc.std to c++<version> to select the appropriate version. Values 'c++14' and 'c++latest' are supported at the time of writing with 'c++latest' being default.

*   Added alpha versions of FASTBuild generation targets.

*   Added target default initialize for 'visualstudio-open', 'visualstudio-example-open', 'visualstudio-test-open', and 'visualstudio-tool-open'.

### Changed

*   warning 4127 "conditional expression is constant" warning is now suppressed in eaconfig for VC builds as discussed with the Frostbite Engine Leaders group.

*   In Frostbite configurations, -wd4481 - nonstandard extension used: override specifier 'sealed' - has been removed by default.

*   Changed frostbite's buildsln framework target to call the eaconfig "msbuild" target rather than the "visualstudio" target. This prevents large hangs when running 'fb nant <packagename> release buildsln'. This also will noticeably remove the hang when doing fb gensln <platform> if the Ddc3 package is out of date and needs to be built.

*   Modified the linker flow on pc platorm to embed manifest file in binary as part of the link rather than using manifest tool to embed it in a separate post-link step.

*   Cleaned up how pre- and post-build steps that handle creation of import library directory and copying import library to final destination. Directory creation now uses unified directory creation logic rather than adding an explicit call to mkdir into the build flow. Post build import library copy step is only created if necessary (in almost all cases import library is output to final location directly).

*   Link Time Code Generation is now enabled by default for all pc retail builds.

*   Changed unix build to use clang's libc++ library if using UnixClang 3.8.1 (and UnixCrossTools 2.00.00 on pc host). If user need to revert back to use GNU's libstdc++, provide global property 'unix.stdlib' and assign with value 'libstdc++'. If building on a Linux host machine needs to have Clang's libc++ runtime installed. You can install this by using the command: sudo apt-get install libc++1.

*   package.eaconfig.printlinktime=true will now add the flag -verbose:incr in addition to -time, in order to print messages when the linker had to switch to non-incremental linking.
*   The msbuild target has been updated to add /nodeReuse:False command line argument to msbuild.exe. That command line argument was added to prevent msbuild.exe processes hanging around after the build is completed which would sometimes lock up some dll files that were used by the msbuilds and causing that dll file unable to be updated.

### Fixed

*   Fastlink has been enabled by default in Frostbite. The latest version of VSRedist package shipped with frostbite now deploys a working version of dbghelp.dll and dbgcore.dll with the build so that callstack resolves can happen correctly. If you run into any issues you can turn off fastlink with the property "package.eaconfig.usedebugfastlink=false".

*   Fixed msbuild target in eaconfig to work properly with Visual Studio 2017\. Previously it failed to find msbuild.exe when using Visual Studio 2017.

*   Fixed Frostbite's default setup of .Net Framework target version to be based on the DotNet package version being used in masterconfig.

*   Fixed the default clean target implementation where in some scenario, it could have allow accumulation of unnecessary temp files due to usage of @{PathGetTempFileName()}.

### Removed

*   Removed FB_AUTORUN_UNITTESTS option from Frostbite configurations as it was no longer used.

*   Removed supression of deprecated-register warning in frostbite unix64 config. The register keyword is still a reserved but un-supported keyword in C++.

# 5.04.03

### Added

*   When running vstests users can now use the global property "eaconfig.vstest-continue-on-error=true" to continue running tests one fails. Framework will continue running the remaining tests and then fail once all of the tests have run.

*   Added 'exclude-packages' attribute option to <create-outsource-prebuilt-packages-from-buildgraph> task to improve usability for the outsourcing tool.

*   Updated <create-prebuilt-package> task to monitor and package files for 'package.[package_name].[module_name].programs' fileset as well.

### Fixed

*   Avoid running a second unnecessary deploy for Blast builds (by NOT running a separate 'copy-asset-files.pc' postbuild step after running the 'copy-asset-files.blast' postbuild step (which effectively iterates over the same files)).

*   Fixed <create-prebuilt-package> task to also examine packages/modules listed in <publicbuilddependencies> task when tokenizing the includedirs for the prebuilt package.

*   Fixed error with <create-prebuilt-package> when we have mixed platforms use case. Previously, the constructed Initialize.xml were missing some config info for the export data overrides.

*   Fixed <create-prebuilt-package> task to test if the package being created already has output mapping override to point "previously" prebuilt libs using the old Frostbite prebuild system.

*   Fixed <create-prebuilt-package> task to preserve dependency that was originally marked as 'auto' dependency. Note that this fix requires Framework version 7.04.03.

*   Fixed <create-prebuilt-package> task to show warning message instead of fail the build if it encounter modules where it cannot find the prebuilt. We encounter situation where package has declared modules but it was actually not part of the build, so no need to package prebuilt libs that was not built.

*   Fixed <create-prebuilt-for-package> task to allow force packaging a "used" or "interface" dependent package.

*   Fixed <create-prebuilt-for-package> task's handling of exporting includedirs from publicdependencies. The previous fix introduced unnecessary include paths which introduced higher risk of command line too long error during Visual Studio build.

# 5.04.02

### Removed

*   Removed definition of FB_PRESENCE_BLAZE and FB_SERVERBACKEND_BLAZE, which will be defined as Frostbite features.

# 5.04.01

### Fixed

*   Fixed a bug with <create-outsource-prebuilt-packages-from-buildgrap> task where under Frostbite's merge build graph use case where the same module is being used as build dependent in one graph while it is being used as use dependent in another graph, the use dependent use case's setting got skipped.

*   Fixed an outsourcing tool bug with the <create-prebuilt-package> task in cases where packages' Initialize.xml containing publicdependencies and the dependent package's includedirs contain paths to builddir. Previously, this task will tokenize those path to ${package.[dependent].builddir}. However, if those dependent package is another prebuilt package created by this tool, then that will be the wrong token to use. This fix will no longer tokenize those dependent package's include paths and just use the result from publicdependencies task instead.

*   On PC build, removed the empty string compiler switch -Fd " " when debugsymbols is set to off. It was a hack added to get around a bug with very old version of Visual Studio where it was creating pdb even when debug info was set to none.

*   Fixed the 'pc-common-visualstudio-extension' task (being used during Visual Studio solution generation) to properly set the vcxproj ExecutablePath property to use ${package.WindowsSDK.kitdir}\bin\${package.WindowsSDK.TargetPlatformVersion} for WindowsSDK 10.0.15063.0 or higher.

### Removed

*   Removed antrenderextensions from frostbite.defaultAntToolPlugins, to elimiate a divergence made by the animation team.

# 5.04.00

### Added

*   Updated to support loading game teams custom config override file (config-overrides\overrides.xml relative to the 'extra-config-dir' masterconfig setting available in latest Framework release). See Framework's release notes for more info about this feature.

*   Added another task (<create-prebuilt-for-package>) to support another outsource packaging workflow.

### Removed

*   Frostbite configs used to enable mapfile generation by default for all platforms when FB_ISAUTOBUILD was set in the environment (AKA most build machines). This was left over from Gen3 days when we were using map files for Juice, which no longer needs map files, and some Executable Size Tracking tools, which are now deprecated. Mapfile generation is now set to the default value (disabled) for build machines, so it will mirror what is set on developers machines.

### Changes

*   Enabling rtti when using frostbite tool configs. Frostbite globally disables rtti for all configs but tool configs explicitly enable this so we have changed it so that it stays enabled for the frostbite version. We don't believe this change will impact anyone because the frostbite tool configs were only added by us recently for some testing so they should not be widely used yet.

*   Enable specific /wall warnings at high warning level only to maintain backwards compatibility, enabled /wall warnings only when they haven't been explicitly disabled

*   Ensure -opt:ref and -opt:icf are enabled for final + retail builds. For UnitTests when building on the build farm enable -opt:ref and -opt:icf and disable incremental linking to reduce size of executables, thus improving deployment speeds because less data needs to be copied Ensure -opt:noref and -opt:noicf are set for debug and release builds to reduce link times since these 2 configs are worked in a lot by engineers. Should provide a decent improvement in xb1 and pc link times.

### Fixed

*   Fixed a bug with <create-outsource-prebuilt-packages-from-buildgrap> task where it could incorrectly skip over a package if the package's very first module is a Utility module.

*   Fixed an error with <create-prebuilt-package> task where it wasn't properly tokenizing an include path using properties specified in 'extra-token-mappings' attribute.

# 5.03.01

### Added

*   A small update to the outsourcing tool ("create-outsource-prebuilt-packages-from-buildgraph" task) to allow user to specify a list of paths to skip over creating prebuilt packages.

### Fixed

*   Fixed a bug in the SandCastle docs generation target that was creating a relative path to the document source files. It was incorrectly creating a relative path between the eaconfig target file and the dlls rather than with the project file and the dlls.

# 5.03.00

### Added

*   The USE_PIX define is now added for all non-retail frostbite pc configs.

*   Added a way for users to provide defines to the MSVC C++ Resource compiler tool. Defines can be added with the property "(group).(module).res.defines" or with the new structured xml attribute "defines" on the "resourcefiles" fileset.

### Removed

*   We have decided to disable fastlink by default in frostbite because of issues a lot of users are having related to dbghelp callstack. You can still turn on fastlink with the property "package.eaconfig.usedebugfastlink=true".

### Changes

*   Removed dependency or DirectX package in Frostbite windows configs. Now uses platform SDK directly.

# 5.02.00

### Added

*   Allow Explicit disabling of comdat folding with the -opt:noicf linker flag. This flag can be enabled with the global property 'package.eaconfig.disablecomdatfolding=true' or the buildset option 'disable_comdat_folding' set to 'on'. On Madden This saves ~20s link time at a cost of about 9MB of wasted runtime memory. Comdat folding is finding functions and constants with duplicate values and merging (folding) them. As a side effect builds with "-OPT:noicf" have improved debugging in common functions because it can be unsettling to step into a different function in a different module while debugging (even if that function generates the same executable code)

*   Added a way to enable link command timing using the global property 'package.eaconfig.printlinktime=true' to add the linker flag '-time'. This prints timing information about each of the linker commands and can be useful for debugging and optimizing link times.

*   Added option cc.std to control c++ standard used when compiling. Currently only supported on clang based platforms. IE: set cc.std to c++<version> to select the appropriate version. The option cc.cpp11 is now considered deprecated but will continue to be supported for now.

*   Added the ability for eaconfig to detect if the user is trying to run a Unix binary on the Windows Linux Subsystem via the 'test-run' target. If so, eaconfig will configure EARunner to run the binary via the Linux Subsystem rather than assuming nant is running on a native Unix machine.

*   Updated 'create-prebuilt-package' task to allow option to modify package build file and initialize.xml file in place instead of creating a new standalone package.

### Changes

*   Fastlink is now enabled for frostbite PC and XboxOne configs by default. This option provides faster link times but produces non-portable PDB files. Fastlink will be disabled when frostbite.FB_ISAUTOBUILD is true so that build farm builds don't produce non-portable PDB files. It can also be disabled using the property package.eaconfig.usedebugfastlink=false. It is also possible to convert non-portable PDB files back into portable PDB files using a tool included with VisualStudio 2015. (C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\mspdbcmf.exe)

*   Unix-clang changed to use c++14 std instead of c++11 std by default. If you wish to change the c++ std used to compile use the option cc.std and set it to c++11 for example to set it back to c++11.

### Fixes

*   Fixed a few bugs with 'create-prebuilt-package' task:
    *   Fixed issues with having trouble resolving dependent package's publicdependencies settings.
    *   Corrected and preserved property interface/use/build dependency settings for the created prebuilt package.

# 5.01.00

### Changes

*   Changed the warnging level to be high in frostbite for all configs that use the clang compiler rather than just for the kettle and unix64 platforms.

# 5.00.00

### New Feature

*   Moved the <CreateGDBDebuggingConfig> task to eaconfig from FBConfig. This means that users of non-frostbite configs can now use this task for generating xml config files for GDB Debugging.

### Changes

*   [Frostbite specific] Moved FB_XXXX_OR_GREATER defines for C# builds out of eaconfig and moved them into FBConfig.

*   Changed PC and Unix's asset deployment feature to default to copying asset without syncing unless user specified [group].[module].CopyAssetsWithoutSync property.

### Removed

*   Removed eaconfig-eamconfig.transition property which was added back in 2013 when we were transitioning eamobile from eamconfig to eaconfig. This change will require users to upgrade to the latest ios_config and osx_config package versions.

*   Removed the properties 'script-dir' and 'source-dir' that were setup by eaconfig to point to the eaconfig script and source directories.

### Fixes

*   Fixed generation of interop assemblies for same COM references to output the assembly to same location regardless which package trigger the generation. Otherwise, it would cause copylocal collision thinking different assemblies with identical name trying to copy to the same output path.

*   Fixed vc's application of "exceptions" flag in config-common-options to allow user set that flag to "custom" instead "on" or "off" only. This allows user to setup their own custom exception compiler switches.

# 4.05.00

### Changes

*   [Frostbite specific] Removed support for using VisualGDB since we now only support the Microsoft Linux Extension for Visual Studio 2015.

*   Updated upload-to-packageserver target to set uploading account if property 'eaconfig.post.account' is specified.

*   Updated upload-to-packageserver target to mark old 'Official' releases as 'Accepted' if property 'eaconfig.post.mark-old-official-as-accepted' equals 'true'. Requires Framework NEXT.

*   Add a new debugging option type for clang configurations, eaconfig.debugsymbols.linetablesonly, which keeps only the necessary information for deducing callstacks and reduces the amount of debugging information generated.

*   Change clang's debugging information to tune for GDB when building on Linux and LLDB when building on MacOS.

### Removed

*   Removed 'verify-master-signature' target.

# 4.04.00

### New Feature

*   Added an experimental target 'create-prebuilt-package' to help user create a prebuilt version of their package for distribution as a prebuilt package. Basic steps for using this target:

    1.  Do a package build for the list of configs that you want to prebuild.
    2.  Execute the nant target 'create-prebuilt-package' and make sure that the 'package.configs' property is used to specify the list of configs that you have done the prebuild for.
    3.  The default output of the created prebuilt package will be located in '[buildroot]\Prebuilt'. This output location can be overridden by setting the property 'eaconfig.create-prebuilt.outputdir'

    Full detail of what this target does is documented in latest FrameworkGuide.chm.

*   Extended the Xbox One LTCG support to PC.

### Changes

*   Updated frostbite defines PPM_ENABLED=0 to be controlled by property 'package.eaconfig.frostbite-enabled". If that property is not set, the define won't be added.

### Bug Fixes

*   Fixed an intermitten error message with "Failed to evaluate property 'eaconfig.DotNetSDK.includedirs'" if project contains Managed Dot Net modules.

*   Fixed a bug when "custom" optimization setting got used, a -Od flag got incorrectly added. This bug was introduced in release 4.02.01 and fixed in this release.

# 4.03.02

### Changes

*   Add FB_2016_3_OR_GREATER C# define for Frostbite 2016.3 release.

# 4.03.01

### Changes

*   Remove eaconfig.build-MP property setting from frostbite-xxx platform config files and set it inside frostbite-common.xml for all platforms.

# 4.03.00

### New Feature

*   [FB-57009](https://jira.frostbite.ea.com/browse/FB-57009): Framework's generate-single-config functionality was checking to see if this value existed, rather than checking to see if it existed and was true, so in cases when generate-single-config was set to false, Framework was behaving incorrectly.

*   Added shorthand targets to eaconfig, for example 'b' for 'build', 'srbr' for 'slnruntime visualstudio run-fast'. See config/targets/target-shorthands.xml for the complete list.

*   Brought back a change from madden in order to allow custom configs to change the executable and solution names by setting the properties 'frostbite.target.name.decoration' and 'licensee-outputPattern-postfix' in their config files.

### Changes

*   Disabled the Code Coverage settings in the default vstest runsettings file. Code Coverage tools are only included in Visual Studio Enterprise edition and print a warning when they can't be found. If you want to use code coverage tools please setup your own runsettings file and point to it using the property 'eaconfig.vstest-test-settings'.

### Bug Fixes

*   Fixed a null string comparison error when using older version of the Visual Studio proxy package that don't define the property 'package.VisualStudio.InstalledUpdateVersion'.

# 4.02.02

### Bug Fixes

*   Updating the eaconfig-vstest-run-caller target to loop through each test assembly and make a separate call to vstest for each. This is to fix failures in FrostEd tests where the assemblies where conflicting with each other when they were all passed to vstest at once.

# 4.02.01

### Bug Fixes

*   Disabled Visual Studio 2015 Update 3's SSA optimizer as it is generating unexpected failing code.

# 4.02.00

### New Feature

*   Adding win64-vc-trial configs so that users can build a separate trial executable that can be package up by Frosty.

### Changes

*   Launch devenv.com with /safemode so that extensions are not loaded for command line builds.

### Bug Fixes

*   Fixing a bug with the mstest/vstest targets, the workingdir which is set to the default outputdir would not exist in cases where the module was using a custom outputdir. We have fixed this by simply adding a step to create this directory if it doesn't already exist.

*   Fix for EASTL/PPMALLOC user headers path generation. Moved userconfig_ppmalloc.h and userconfig_eastl.h into the FBConfig package to avoid direct dependency on Core/Engine.Core in eaconfig/FBConfig. frostbite.defines now points to these userconfig files in FBConfig.

# 4.01.00

### New Feature

*   Added property eaconfig.mstest-test-category-filter to control the mstest category option.

### Changes

*   Enabled -Wall and -Werror on all Frostbite Linux configs.

    Frostbite configs now depend on the Ddc3 package and the value of the property "frostbite.ddc" points to the ddc.exe file in the bin directory of the ddc package instead of in TnT/Bin.

# 4.00.00

### Removed

*   **Important:** All ps3 and xenon configs have been removed from this version of eaconfig.

### Bug Fixes

*   Converting the slashes used in the frostbite defines EASTL_USER_CONFIG_HEADER and PPMALLOC_USER_CONFIG_HEADER to be all forward slashes to prevent the preprocesser/parser from misinterpreting the forward slashes as escape sequences in some cases.

# 3.05.00

### Changes

*   [ETCM-3383 (FB-38872)](https://jira.frostbite.ea.com/browse/ETCM-3383): Updated Linux build to always use response file for compile/link/library build.

*   [ETCM-3383 (FB-38872)](https://jira.frostbite.ea.com/browse/ETCM-3383): updated 'ar' command line switch and added '-c' option (suppress warning about library had to be created spam message).

### BugFixes

*   [ETCM-3381](https://jira.frostbite.ea.com/browse/ETCM-3381): Corrected a typo of an optionset name 'nant.commandline.properties'. Previously Framework 5.06.01 and older were using property name 'nant.commadline.properties'. The new version of Framework has corrected this spelling mistake. For now, this fix will detect if older version of Framework is being used and the old 'nant.commadline.properties' optionset was created. In future release when we drop support of old Framework, we would remove this backward compatibility support.

# 3.04.01

### Changes

*   Use the output directory as the working directory for tests, if a special output directory is specified.

# 3.04.00

### New Feature

*   A build warning is raised when a clean target is invoked at the same time as a build target. Framework exhibits undefined behavior when this happens which may lead to inconsistent build failures, this error is intended to alert users to patterns that may be unreliable.

*   Added all groups versions of print-referenced-packages-list.

    *   example-print-referenced-packages-list
    *   test-print-referenced-packages-list
    *   tool-print-referenced-packages-list
    *   print-referenced-packages-list-all
*   Adding a new buildtype option for modifying the warning level of a package, called "warninglevel". The default value is "high" which uses the "-Wall" flag. For frostbite configs we set this to "medium" which uses the "-W4" flag. For GCC/Clang compilers we also support the setting "pedantic" which adds the flags "-Wall -Weverything -pendantic". You can override the default value globally using the property "eaconfig.warninglevel". You can also override the option in a custom build type or for a single module in structured xml using the config\buildoptions section.

### Changes

*   The property define "frostbite.create.build.mak" for frostbite build has now been removed.

*   Updated frostbite define for EASTL_USER_CONFIG_HEADER.

*   Changed frostbite's gensln target to use slngroupall instead of slnruntime to create the solution.

*   Updated support for config.targetoverrides to include the frostbite targets (gensln, opensln, etc) so that when this eaconfig is being used in Frostbite 2015 line, the old FBConfig can override the new targets in eaconfig intended for 2016 line.

### Bug Fixes

*   [ETCM-3252](https://jira.frostbite.ea.com/browse/ETCM-3252): For PC/PC64 configs, the generated vcxproj will now include WindowsSDK's executable paths in the ExecutablePath field in the project. This change is necessary so that when Visual Studio spawn resource tools such as rc.exe, the correct version will be used (especially when non-proxy WindowsSDK package were used).

*   Updated pc/pc64 configs vcxproj generation (under WindowsSDK 10 and VS 2015) to override global properties UCRTContentRoot, UniversalCRTSdkDir, WindowsSdkDir_10, NETFXKitsDir, and NETFXSDKDir. We need to override these properties so that default paths in MSBuild will properly use the correct path if WindowsSDK non-proxy version is used. Also, beginning with Visual Studio 2015 update2, the MSbuild files will do error checking if the expected paths are missing.

*   Corrected "defaultFrameworkVersion" property define during frostbite build to use property syntax and set it to v4.6.1 when using Visual Studio 2015 (Note that you will need to have Update 1 installed or your machine need to have .Net 4.6.1 installed separately).

# 3.03.00

### New Feature

*   Added frostbite configs to eaconfig.

    There are 24 new frostbite configs: win64-vc-debug, win64-vc-release, win64-vc-retail, win64-vc-final, win64-vc-dll-debug, win64-vc-dll-release, win64-vc-dll-final, win64-vc-dll-tool-debug, win64-vc-dll-tool-release, win64-vc-dll-tool-final, win64-vc-server-debug, win64-vc-server-release, win64-vc-server-retail, win64-vc-server-final, win64-vc-dll_nomaster-debug, win64-vc-dll_nomaster-release, linux64-clang-debug, linux64-clang-release, linux64-clang-retail, linux64-clang-final, linux64-clang-server-debug, linux64-clang-server-release, linux64-clang-server-retail, linux64-clang-server-final.

    win64 configs are similar to pc64 configs and linux64 are similar to unix64 configs. The debug and retail configs are similar to the non-frostbite debug and retail configs. The release and final configs are similar to the non-frostbite debug-opt and opt configs. Some of the configs such as the server and nomaster don't have a direct non-frostbite equivalent.

    These configs load frostbite specific settings from the files 'config/platform/frostbite.xml' and 'config/platform/frostbite-${config-system}.xml'.

    Using these configs will by default try to load FBConfig as a dependency to load extra settings that are needed only when building in the frostbite environment. However they can be used without FBConfig by setting the global property 'package.eaconfig.frostbite-enabled=false', and then used to test packages outside of frostbite.

    For more information see the documentation page '/Reference/Configuration Package/Frostbite Configuration Files' in the latest Framework release.

# 3.02.00

### New Feature

*   Added dev-retail config set. This new config by default is similar to opt build but added "retailfags" in config-options-common which control adding EA_RETAIL preprocessor define. **NOTE**: this change requies the new Framework version that support this feature.

*   Package target now excludes file beginning with '.' or files in folder beginning with '.' (e.g .git/ .gitignore).

*   Added new target 'upload-to-packageserver' which calls eapm post for the target package.

# 3.01.00

### New Feature

*   [ETCM-3038](https://jira.frostbite.ea.com/browse/ETCM-3038): Added option to run vstest tests instead of mstest (essentially the same thing but is more modern and allows for code coverage data).

### Bug Fixes

*   [ETCM-3051](https://jira.frostbite.ea.com/browse/ETCM-3051): Re-added 'EchoOptionSet' task that was removed in 2.16.00 release as a couple of game teams have re-added this task already.

*   [ETCM-3052](https://jira.frostbite.ea.com/browse/ETCM-3052): Fixed Visual GDB support for Unix config (ie with -G:unix-enable-visualgdb=true). This feature was broken since 3.00.00 release.

# 3.00.03

### Bug Fixes

*   Fixed a bug where post process step wasn't being executed for files with custom optionset. This fix requires you to update to Framework 5.01.00 or later.

# 3.00.02

### Bug Fixes

*   For build types that uses a linker (ie Program, DynamicLibrary, etc), previously we have both compiler and linker save out the pdb file to the same location (bin folder). But the two pdb output actually don't contain the same information. In fact, on Visual Studio 2015, if -debug:fastlink option is used, the two pdb are in fact no longer compatible. So we updated the compiler's pdb output to intermediate folder (just like what the default Visual Studio project would do).

# 3.00.01

### Bug Fixes

*   Fixed "debugfastlink" support for Visual Studio 2015 to only apply -debug:FASTLINK link option for native C++ modules. This option is not supported for managed C++ modules.

# 3.00.00

### Changes

*   This release introduces a dependency on Framework-4.01.00 or later. As such, the major version number of this release has been increased to 3.

*   Added compiler switch -Zc:inline compiler for pc build if Visual Studio 2015 is being used

*   Disabled 2015 vc compiler warning 4577 by default: "'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed".

*   Added "debugfastlink" option to "config-options-common" for pc-vc config to allow people to control choosing between linker switch -debug vs -debug:FASTLINK. This option is only set to on if you're using Visual Studio 2015 AND global property 'package.eaconfig.usedebugfastlink' is set to true.

*   Minor changes to include a number of the MSVC internal compiler defines for NAnt's dependency generator (__MSVC_RUNTIME_CHECKS, _CPPRTTI, _CPPUNWIND, _MT, _DLL, __cplusplus_winrt, __cplusplus_cli, _M_CEE, _MANAGED, __cplusplus, _WIN32, _WIN64, _M_AMD64, _M_IX86, _M_ARM, _MSC_VER)

### Bug Fixes

*   Fixed a bug where -d2Zi+ compiler switch accidentally get used for Visual Studio 2015 instead of -Zo. This bug is now fixed.

*   Fixed a bug where preprocessor macros would be passed to assembler on Unix.

# 2.25.04

### Changes

*   Previously the "default" pc build has compiler option -W4 -Wall on the same command line together. It is now replaced with just -Wall.

*   Excluding p4protocol.sync file from being added to the package server zip file created when running the 'package' target.

# 2.25.03

### Bug Fixes

*   Fixed a resource file build error under Visual Studio 2015 if you use plain nant build (ie instead of building from solution file) and your project contain resourcefiles fileset.

# 2.25.02

### Bug Fixes

*   Fixed an error with VS 2015 support in C# project build. Wrong library directory was used.

### Changes

*   Builds involving ps3-sn configs will now specify -J flags for all of the system include directories so that warnings in these files don't fail builds when warnings as errors is enabled. This flag is the SN compiler equivalent of GCC's -isystem flag which eaconfig has been using for several years.

# 2.25.01

### Bug Fixes

*   Small fixes to the eaconfig.copy-asset-files task to handle specific situation during ios build if assets are being copied using Xcode project's Copy Resource Phase. Xcode will copy them flattened, so that synctargetdir task needs to take that into account. Note that you need to be using ios_config-1.02.01 in order for this fix to work.

# 2.25.00

### Changes

*   Removed the patch for the missing mono property on osx that was added in 2.24.01. The properties are now defined in osx_config-1.11.00. We require that all users upgrade osx_config to that version in order to use this version of eaconfig.

*   Added a new option for PC builds, general-member-fn-ptr-representation, that when set to on will instruct MSVC to not perform size optimizations on member function pointers via /vmg ([MSDN reference](https://msdn.microsoft.com/en-us/library/yad46a6z.aspx)). This option is required for program correctness if member function pointers are created based on types that are declared, not defined.

### Bug Fixes

*   Added buildset.as.options to config-options-common. This is needed so that we can properly setup a buildtype with as.options.

*   Updated to allow building with VS 2015 CTP6/RC (using an appropriate version of WindowsSDK that has been updated for VS 2015 CTP6/RC).

# 2.24.01

### Bug Fixes

*   Fixing a bug in the run target that causes it to crash for osx configs. We removed the dependency on the mono package, but older versions of osx_config still require a property from that package to be defined in order to run.

# 2.24.00

### Changes

*   Adding the /bigobj argument to all pc builds (also affect winrt/winprt builds, however these builds normal set this by default). This argument increases the number of addressable sections in an object file. The only drawback of this is it isn't supported by vc++ 2005, but since we don't support that version we have decided to enable by default.

*   Removed the eaconfig's dependency on the mono package. Properties that were formerly defined by the mono packages are now defined by unix and osx configs.

### Bug Fixes

### Features

# 2.23.00 (Feburary 23, 2015)

### Changes

*   ETCM-2082 Updated to look for the property [group].[module].CopyAssetsWithoutSync and pass that info to the synctargetdir task during asset copy. Note that this requires Framework version 3.29.00 or newer. If you're using an older Framework version, setting up this property will have no effect.

*   ETCM-2077 Added visualstudio-all target. Multi configuration equivalent to visualstudio target.

### Bug Fixes

*   ETCM-2077 visualstudio-open target will now open all solutions if different configs map to different solution files. This target now depends on functionality that was added to Framework-3.29.00.

# 2.22.00 (Feburary 6, 2015)

### Changes

*   Non-Xenon profile configurations (ie: pc-vc-dev-profile) were including the opt configset instead of the profile configset. These configurations now include the profile configset.

*   Native NAnt builds in PC configurations now build module files in parallel with VS2013 or newer.

*   ETCM-1843 Updated to support future version of Framework to igore DotNet modules (such as CSharpProgram, etc) on unsupported configs.

*   Added parameter for dependency graph color scheme.

### Bug Fixes

*   ETCM-2158 Fixed eaconfig's eaconfig-mstest-run-caller target to not use the package.eaconfig.dir property as it is not defined.

*   Fixed NantCommandLineLexer task to prevent EAMSDKTools package from depending on itself.

# 2.21.01 (November 25, 2014)

### Bug Fixes

*   There was a bug in the newly refactored Incredibuild target that was forcing some platforms like kettle and capilano to always use XGE.

# 2.21.00 (November 19, 2014)

### Changes

*   Updated 'validate-code-conventions' target's output to not show eaconfig log message prefix so that people can click on the error output in Visual Studio.

*   ETCM-1868 /sdl- and /gs- interaction. Use property 'disable-security-check-option' to set either -sdl- or -GS- option for pc and winrt configs.

*   ETCM-1806 Updated 'validate-code-conventions' target to also show warning about build script not using structured XML. IMPORTANT: This change has a dependency on Framework release 3.27.00 (or Framework-dev CL 1026938).

*   Modified NantCommandLineLexer task to pass the package build root to Python in order to set the log folder as the build root instead of in the Python package itself.

*   Refactored incredibuild targets and updated them to reflect which platforms support incredibuild. As a result the only platform that will force XGE is android-clang.

### Features

*   ETCM-1859 Added a new targets "verify-includes" to check each source file's include usage do have a proper build script dependency setup. Usage:

    `nant -D:config=<ConfigName> [-G:verify-src-includes.remove-from-includes="path1;path2;..."] verify-includes`

    Use the optional global property "verify-src-includes.remove-from-includes" to remove a specific list of include directories from analysis.

### Bug Fixes

*   ETCM-1916 Fix the task that execute IncrediBuild to assign a working directory to the IncrediBuild temp folder to avoid the situation where IncrediBuild leaving temp files to eaconfig source folder.

# 2.20.00 (October 6, 2014)

### Changes

*   ETCM-1676 The default define for WINVER (minimum target Windows version) is now set to 0x0600 (Windows Vista). Previously, it was 0x0502 (Windows XP SP2). If you require the previous behaviour, you can use global property eaconfig.defines.winver to reset it back to previous setting. Please see Framework's chm help file for more information about this property.

*   ETCM-1436 Updated visualstudio targets to not fail the build if sln file wasn't found because module list is empty. A warning message will be printed instead.

*   ETCM-1715 Updated support for the undocumented VC switch /d2Zi+ to only get used when "debugsymbols" config setting is turned on.

*   ETCM-1777 Added support for /Zo option on PC optimized build if we're using Visual Studio 12 and update 3 or better.

*   Removed the unix build's config-options-dynamiclibrary optionset's template varialbe impliboutputname. That template variable was previously incorrectly set to a stub lib that doesn't exists on unix build.

*   Updated dependency graph feature

*   Updated doxygen target to allow us to add additional include directories. Also updated the include file search to check .h and .cpp only.

*   ETCM-1516 Updated some warning messages with message ID so that these nant messages can be suppressed if desired. See Framework 3.26.00's release notes.

*   ETCM-1728 COM interop assemblies are now generated into intermediate build directory during solution generation and are copied to bin directory at build time.

*   ETCM-1527 Updated the Framework package task to update the manifest file's compatibility revision to the release version number.

### Features

*   ETCM-1676 Updated to allow user specify minimum target Windows version through NTDDI_VERSION macro. You can set this macro through property '[group].[module].defines.ntddi_version' or global property 'eaconfig.defines.ntddi_version'. Note that if you use this option to define minimum target Windows version, don't use 'XXX.defines.winver' and let eaconfig setup the WINVER macro for you based on your NTDDI_VERSION setup. Please review latest Framework's chm help file for more info. One benefit of setting the NTDDI_VERSION macro is that it allows you to target for specific OS service pack while WINVER doesn't allow that.

*   ETCM-1785 Set up a task in EAConfig to validate some frostbite code standards.

    *   Added target 'clang-format' to format source and header C/C++ files according to the code standards. Requires "ClangFormat" package.
    *   Added target'copy-clang-format-file' to copy '.clang-format' file with code standard rules to package roots. This target places rules file to use with 'clang-format' Visual Studio plugin
    *   Added target 'validate-code-conventions', target scans C, C++, C# source files in the package and reports found violations of code conventions.

        NOTE. 'clang-format' does not automatically fix all variable name conventions, 'validate-code-conventions' target may catch violations missed by 'clang-format.

*   Updated the make-build set of targets to allow user setting property "eaconfig.make-build-extra-args" to pass in extra command line arguments to make.

# 2.19.00 (August 11, 2014)

### Changes

*   Windows Phone does not permit invoking application instances with arguments. To be able to parameterize invocations, the run arguments are slipstreamed into the application bundle into a file that is read by EAMain.

*   Changed visualstudio-internal-open target to use detached="true" to avoid sharing I/O handles between processes. This prevent scripts waiting on devenv.exe completion before returning.

*   Updated sign-package to also output package hash and package version used for creating the signature file.

*   Improve target that generates module dependency picture.

### Features

*   Added support to allow getting devenv.com log during Visual Studio build (note that they are not build log, they are debug log of Visual Studio itself). Provide a global property VisualStudio.devenv.logfile to specify to log file location. If this property is specified, the command line switch

    `/Log "${VisualStudio.devenv.logfile}"`

    will be added to devenv.com execution.

### Bug Fixes

*   ETCM-1673 Updated to make sure that the property eaconfig-run-workingdir property in eaconfig-run target get assigned a default value and will not be an empty string.

*   Fix for intermittent error when building with visualstudio 2010 and Incredibuild about not being able to update vc100.pdb.

# 2.18.00 (July 14, 2014)

### Changes

*   ETCM-1375: warning 4571 has been added to the list of warnings that are disabled by default in eaconfig. The warning is: "Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught." We feel this warning is of no value in our current codebase.
*   ETCM-1466: Added an undocumented switch (/d2Zi+) which improves debugging optimized code for Windows. This switch does not affect code generation, but does make some .PDB files about 10-15% larger. Credit Scott Wardle for the suggestion.
*   #### Experimental Feature: Eaconfig static/dynamic runtime library consistency setting and verification

    Currently the multithreadeddynamiclib option is used to control whether a module uses static or dynamic runtime libraries. However errors can occur if dependents and top level modules have inconsistent values. We have added two new features to simplify this situation, however they are both disabled by default for this release so that we are not changing user's settings unexpectedly.

    We have added a step that will go through all modules and try to make dependents consistent with the top level modules. This can be enabled by setting the property "eaconfig.multithreadeddynamiclib.makeconsistent" to true. However, this feature won't always be able to make modules consistent especially if top level modules are inconsistent, since we won't change the settings of top level modules. In these situations you will still need to try to make them consistent your self, either by changing the setting per module or globally changing the value using "eaconfig.multithreadeddynamiclib".

    We have also added a verification step that will look through all of the modules and make sure that they are consistent. This can be enabled by setting the property "eaconfig.multithreadeddynamiclib.verify" to true.

*   Eaconfig is also now smarter about how it handles the build option "multithreadeddynamiclib" when it is set to "off" for managed C++ and other CLR modules. In Visual Studio 2010 and earlier it would automatically correct this, but Visual Studio 2012 and later it fails. Eaconfig will now auto correct this setting for you but print a warning saying that it has done so, since ultimately this setting is incorrect and should be fixed in the build script. Unlike the consistency setting/verify feature, this feature is enabled by default.

### Features

*   EaconfigLint target changed so that per fileitem defines are respected but other build settings inherited from the compiler are ignored.
*   Added a target for generating a clang compilation database, the target is called "clang-compilation-database". This target depends on a task that was added to Framework-3.23.00.

### Bug Fixes

*   ETCM-1471 Fixed bug where when eaconfig.debugsymbols was set to false it would still add the path to the non-existent pdb file to the visual studio project. If the pdb file field is not empty it will look for the pdb file each time before building and do a complete rebuild it not found. So now when debugsymbols is set to off eaconfig will set the pdb file path value to be an empty string.
*   ETCM-1506 Fixed an intermittent solution build failure when the solution contains multiple modules that uses eaconfig's assetfiles post build copy feature.

# 2.17.02-2 (June 18, 2014)

### Bug Fixess

*   ETCM-1506 Fixed an intermittent solution build failure when the solution contains multiple modules that uses eaconfig's assetfiles post build copy feature.

# 2.17.02-1 (prerelease)

### Changes

*   This patch release fixes a problem where pdb file path would be added to visual studio even if eaconfig.debugsymbols is set to false. The problem was that if a pdb file was set visual studio would do a complete rebuild if it couldn't find it, but when debugsymbols is off there never will be a pdb file and so it will always do a complete rebuild. This change has been submitted to dev, but we are releasing this as an urgent patch because it could be a big time saving for our build farm.

# 2.17.02 (June 9, 2014)

### Bug fixes

*   ETCM-734 Added deprecation warning message for removal of custom-build feature.
*   ETCM-1374 Disabled cl.exe warning 4571 (Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.)
*   Disabled cl.exe warning 4987 (nonstandard extension used: 'throw (...)')
*   ETCM-1439 The PackageExclude task only accepted whitespaces as delimiters for the list of exclusions in package.exclusions. This behavior has been changed to respect any standard delimiter such as tab, newline, carriage return or whitespace.

# 2.17.01 (May 12, 2014)

### Changes

*   Task AddNetworkLibsToModule is deprecated. Instead use fileset 'platform.sdklibs.network'. This fileset is always defined, even if empty.

*   ETCM-1193 Updated unix gcc config builds to only turn on c++11 switch if we're using GCC version 4.6 or later.
*   ETCM-1374 Added warning suppression 4571 for PC build (Disable warning: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.)
*   The property package.packageexclusions previously only accepted spaces as delimiters, but now accepts all default delimiters such as tabs and new lines.

### Features

*   ETCM-65 Provided property 'eaconfig.DotNet.tools.dir' to help user get the directory to the DotNet tools. Note that if you're using WindowsSDK 7.x, you will need to specify DotNet version in your masterconfig or this new property will not be defined.
*   ETCM-1244 Exposed network libraries through fileset 'platform.sdklibs.network'. '"AddNetworkLibsToModule' task is deprecated.
*   Modified the package target so that users can completely override the fileset of files to be packaged by using "packagefiles-overrides" instead of needing to completely override the package target.

### Bug Fixes

*   Fixed "visualstudio", "incredibuild", etc targets to take into account of usage where people could have Visual Studio solution config name overrides.
*   ETCM- 1272 Fixed Web service wrapper code (wsdl.exe) being executed multiple times causing failure during Visual Studio solution generation.

# 2.17.00 (Apr 14, 2014)

### Changes

*   Image files for winrt/capilano (Logo, SmallLogo, StoreLogo, SplashScreen) previously would generate a blank file by default, when no other file was specified. However, this caused occasional errors, such as trying to access a locked file. We have added pregenerated image files to the latest Framework package and by default if these files are detected by the build it will copy them instead of trying to generate them.
*   Updated existing asset file copy support's target destination override property ${group}.${module}.asset-configbuilddir to accept relative path as well as full path.

### Features

*   Added VisualGDB support during solution build for Unix cross config on PC. Note that it requires the version of Framework and UnixClang that has added UNIX cross build support in Visual Studio solution generation. For more information about this Unix cross build support and VisualGDB support, please check the UnixClang (proxy version) package's documentation
*   Added Direct MakeFile Generation
*   The makebuild target supports ps4 now.
*   Added Asset file support on PC, Unix, xenon and PS3\. NOTE: If you use this feature on these platforms and your package has multiple program modules that has asset files, make sure that you use a common asset fileset for all these program modules or have different output asset directory ([group].[module].asset-configbuilddir) override for each of these modules. Otherwise, during the asset copy synchronization step, asset files that doesn't belong to the current module being built will get removed.
*   Updated Intellisense target to load all known Structured XML platform extensions.
*   Updated the Intellisense target to copy the Manifest and Masterconfig schema files. (Requires Framework 3.20.00 or later)

### Bug Fixes

*   ETCM-1145 Corrected include path / lib path / linker options for nant-build of Windows Phone build.

# 2.16.00 (Feb 11 2014)

### Changes

*   The property config-type has been remove from all configs. Traditionally this was set to 'd' for debug builds and 'z' for opt builds, but has been marked as deprecated for several years. There is a small chance this may still be being used for setting the names of executables, but this is no longer the supported way of doing this. Please contact "EATech CM Support" if you have concerns.

*   ETCM-723 eaconfig.build-MP has a default value of "true" now, so it is no longer necessary to list it in the global masterconfig file (unless you want to disable it).
*   ETCM-733 removed target-persistent-property-passing from EAConfig
*   ETCM-724 The ScriptInit task is deprecated. Any package that uses it will cause a warning to be printed. If your package initialize.xml script has this:  
	```
    <task name="ScriptInit" packagename="EAThread" />  
	```
    Replace it with this:  
	```
    <publicdata packagename="EAThread" add-defaults="true">  
      <module name="EAThread" />  
    </publicdata>
	```

*   ETCM-726 The codewizard targets have been removed from eaconfig. We do not believe these targets are still in use.
*   ETCM-727 the build-res target has been removed from eaconfig. Use [groupname].[modulename].resourcefiles fileset instead.
*   ETCM-776 Removed 4 obsolete tasks from eaconfig: ApplyConvertSystemIncludesToCCOptions, Resolve_PCH_Templates, UpdateLinkOptionsWithBaseAddAndDefFile, and ToolbuildConfig
*   ETCM-779 config-type property has been removed from eaconfig. It has been deprecated for some time.

### Features

*   ETCM-703. eaconfig's doxygen task can build CHM files now. You need to create an optionset called eaconfig.doxygen.options and set the required option there.  
    For example:
	```
	<optionset name="eaconfig.doxygen.options">
		<option name='GENERATE_HTMLHELP' value='YES' />
	</optionset>` 
    ```

*   ETCM-898. The incredibuild target supports PS4 without going through XGE now, so if you want to enable XGE builds on PS4, make sure that the property incredibuild.usexge is set to true. If you do not set incredibuild.usexge to true, the build will be done with regular Incredibuild (i.e. the same one that builds Microsoft Visual Studio projects) and will require an additional license. XGE builds do support Xbox One at this time, and we do not have plans to add such support.

### Bug Fixes

*   Fixes iOS asset copy. Improved logic in reusing existing build graph. This change requires new functionality added in Framework 3.18.00.
*   ETCM-940 Fixes asset deployment error during XCodeProjectizer build if the project contains multiple application modules. Previously, it could accidentally used the assetfiles fileset from a different app module instead of the intended module.
*   ETCM-119 Added NAnt build support for [groupname].additional-manifest-files and [groupname].input-resource-manifests fileset specification that was already supported in Visual Studio build.
*   ETCM-120 Fixed native NAnt C# build where .Net 4's version of ResGen.exe was used to compile resource when DotNet version was set to version 3.5 in masterconfig. Now, if user is using Windows SDK 7.x, the version of ResGen.exe being used will be determined by the version of DotNet specified. Also, if Windows SDK 8.x is being used and you did not set DotNet to 4.x or later, we will fail the build during native NAnt C# build.
*   ETCM-935 Fixed winrt reference directories:
    *   Added directory '${package.WindowsSDK.appdir}\ExtensionSDKs\Microsoft.VCLibs\${config-vs-version}\References\CommonConfiguration\neutral" for winrt.
    *   Made sure slash is present in paths after ${eaconfig.PlatformSDK.dir}

# 2.15.00 (Jan 13, 2014)

### Changes

*   none.

### Features

*   PlayStation 3 SDK 4.50 compatibility fix. This release disables a fairly useless compiler warning (#2261 - return of void* from method potentially violates strict-aliasing rules) from the SNC compiler.
*   ETCM-601 Adding buildinfo-all target so users can generate build info for all groups with a single target
*   Added options flag "multiprocessorcompilation" to explicitly set MultiPricessorCompilation to false in Visual Studio

### Bug Fixes

*   Fixed PS3's prx modules build to output to correct location.
*   ETCM-792 Added in vstomakefile support for PS4.
*   ETCM-823 Added warning about building with visualstudio on non-windows platforms
*   ETCM-853 Fixed bug with WinPRT managed mode not getting assets. Added deployassets=on for WinRTRuntimeComponent

# 2.14.00 (Dec. 6, 2013)

### Changes

*   Opt configurations on UNIX set flag '-O2' now. Before this change compiler default optimization level was used.  
    NOTE. This change may cause new warnings: -Wunused-result and -Wuninitialized.
*   In an effort to standardize config names, new unix-x64-* config names have been added. The old unix64-* config names will be deprecated by December 2014 and removed. If you have build scripts that references something like:

        ${config-system} == 'unix64'

    please make an effort to update your script to something like the following:

        (${config-system} == 'unix' or ${config-system} == 'unix64') and ${config-processor} == 'x64'

    To test out the new unix-x64-* config names, you need to use the dev branches of UnixGCC or UnixClang packages on perforce.
*   The eaconfig.vc8transition flag is now ignored. Prior to this release, enabling this flag replaced the "-clr" flag with "-clr:oldsyntax" for managed C++ modules. If you have old-style managed code that cannot be updated to the VS2005 or later syntax, the package.eaconfig.vc8transition behavior will have to be replaced with a custom build type. It should look like this:  

    ```
	<BuildType name="ManagedCppAssemblyOldSyntax" from="ManagedCppAssembly">  
      <option name="buildset.cc.options">  
       ${option.value}  
       -clr:oldSyntax  
      </option>  
      <remove>  
       <cc.options>  
        -clr  
       </cc.options>  
      </remove>  
    </BuildType>  
    ```  
    And then set your group module build type to ManagedCppAssemblyOldSyntax.  

    [Here is a migration guide for updating old syntax managed code to /clr.](http://msdn.microsoft.com/en-us/library/ms173265(v=vs.100).aspx)

### Features

*   ETCM-557 How to detect a missing package dependency for a given build target and emit a message  
    Added properties

    ```
	"package.${group}.warn.message"
    "${group}.${module}.warn.message"
	```

    If above properties exist the value is printed as a warning message by build/run targets

    ```
	"package.${group}.error.message"
    "${group}.${module}.error.message"
	```

    If above properties exist the build/run targets will fail with message equal to value of the property.  

    **NOTE.** Module specific property is checked first.  

### Bug Fixes

*   ETCM-588 PRX stub libraries were not being copied to the library output directory for ps3-sn builds. Fixed. This problem did not affect GCC builds.
*   ETCM-284 Caps / WinRT Deployment targets - make consistent
    *   Assetfiles filesets are automatically deployed. Deployment of assetfile can be controlled by [group].[module].deployassets property
    *   Added boolean attribute "deployassets" to packaging element in SXML.
*   ETCM-602 WinRT: Fixed bug where multiple executable modules manifest files override each other.
*   Updated name of SandCastle package to "Sandcastle" (case-sensitivity problem).

# 2.13.00 (November 7, 2013)

### Changes

*   Removed 13 deprecated targets from eaconfig. These targets should not be used by anyone any more but if this breaks your build, please let us know and we will put them back in.  
    Here is the list:

    ```
	csproj[example|test|tool]
    vcproj[all][example|test|tool]
    nantvcproj
	```

    These targets have been superseded by sln[runtime|example|test|tool|group][all].
*   Changed default value for the following eaconfig targets to be excluded by default instead of included:

    ```
	build-copy
	build-copy-program
	build-copy-library
	build-library
	build-program
	```

*   removed support for VisualStudio 2005 & 2008\. VisualStudio 2010 is a minimum required platform for eaconfig 2.13.00 and later.

### Features

*   ETCM-405 - Added new BuildType CSharpWinRTComponent (requires Framework 3.16.00)
*   ETCM-5 - eaconfig has a target for creating Sandcastle Help File Builder projects. set doc.type to 'SandCastle' and execute eaconfig's standard 'doc' target to create this. You will need to add the SandCastle package to your project masterconfig file.
*   ETCM-413 Added support for localized resources on Xbox One.
*   ETCM-121 - Added support for global property xaml-add-windows-forms-dll to allow adding back in System.Windows.Forms.dll to the default assemblies list during WPF project build. The removal was breaking ANT build.
*   added eaconfig.xenon.forceredeploy global property to remove the /d flag from xbcp.
*   Added support for EATestPrintServer 2.0, which changes how it launches in order to guarantee that it has started and is ready to listen before the application is launched.
*   Added new pattern to ___xge_repeatpattern: "Timed out waiting for reply from local connection (after 60 seconds;"
*   added new target: verify-package-versions, which scan through all packages listed in the masterconfig file and notes if they have been marked "deprecated" or "broken" on the package server. Requires Framework 3.16.00 or later.
*   added a makebuild target. It is similar to incredibuild in that it creates makefiles from the solution & project file. However, the build is done by executing vsimake directly instead of going through incredibuild. This target can be helpful if you want to build using makefiles from Visual Studio and suspect there is some Incredibuild-related issue you are trying to work through.
*   Warning C4371: "layout of class may have changed from a previous version of the compiler due to better packing of member" for Windows platform builds.

### Bug Fixes

*   ETCM-397 - copy-asset-files target fixed to not call dependency on the current package
*   ETCM-121 - Added support for global property xaml-add-windows-forms-dll to allow adding back in System.Windows.Forms.dll to the default assemblies list during WPF project build.
*   in the eaconfig-run target, any non-printable character space can now be used as the delimiter for the buildmodules. Previously, it was restricted to the space character only.
*   ETCM-523 vstomaketools is not finding the vc-rccompiler. Fixed.

# 2.12.00 (October 11, 2013)

### Changes

*   ETCM-121 C# Builds Pull in Windows Forms.
    *   Do not add Windows.Forms.dll if xaml files are present
    *   Property [group].[module].usedefaultassemblies is used in C# modules. Before it was used in MCPP only
    *   NOTE: If your existing project breaks because of this change, you can add global property xaml-add-windows-forms-dll=true to your masterconfig to add this dll back in to default assemblies list.

### Features

*   ETCM new "copy-asset-files" target
*   ETCM Changed gcc-postprocess task to check for duplicate linker options

### Bug Fixes

*   ETCM ETCM-325 eaconfig.usemultithreadeddynamiclib incompatible with xenon. Set it to 'off' in xenon configs.

# 2.11.00 (September 3, 2013)

### Changes

*   eaconfig-run.iphone target has been moved to in ios_config.

### Features

*   new target added to eaconfig, "remotebuild", integrated from EAMConfig. This allows building code for Unix systems remotely from a Windows system. Refer to the documentation in the RemoteBuild package for information on usage. To enable this target, it is necessary to define a property eaconfig-eamconfig.transition=true.
*   New targets added (primarily for the build farm): test-group-run-fast, test-group-runall-fast, group-run-fast, example-group-run-fast, tool-group-run-fast

### Bug Fixes

# 2.10.00 (July 22, 2013)

### Changes

*   ETFW-2651 Map file generation issue appears to be fixed in VS2012\. So we're re-enabling map file generation on PC with VS 2012.
*   ETFW-2713 Incredibuild error: Failure to initiate build  
    Updated incredibuild restart pattern with "Timed out waiting for reply from local connection (after 60 seconds)"
*   ETFW-2723 Disabled compiler warning (C4350: behavior change . . ).

### Features

*   Added support for csharp builds to unix (requires Framework higher that 3.12.00).
*   ETFW-2734 Added new option for WMAppManifest for WP8: "app.templateflip.largebackgroundimageuri"

### Bug Fixes

*   ETFW-2753 Removing the config-options-windowsprogram and config-options-managedcppassembly from capilano-vc.xml in CL906161 had the unintended side effect of breaking Xbox One DLL builds. Only define these optionsets in the DLL configset files if they are already defined.
*   ETFW-2730 Fixed doxygen target
*   Made sure that the "--suppressions" option is only passed when using valgrind. If this option is passed to EARunner it can cause EARunner to fail because it is an unrecognized option.
*   ETFW-2656 ignore linker warning LNK4264 when building libraries that contain Windows Runtime metadata. The situation the linker is warning about does not apply to any of our current code. Linker warning also occurs with the librarian tool; suppress it in the config-options-library optionset as well as config-options-program.
*   ETFW-2627 Unix gcc and clang linker options treated as dependent files Added a postprocess step to move linker options in the linker libraries fileset to the linker options optionset. This prevents linker options from being interpreted as missing files in the .dep file, preventing a rebuild in a do nothing build. Added a advise level warning message when options are found in the libraries fileset instead of the options optionset

# 2.09.00 (June 3, 2013)

### Changes

### Features

*   Windows Phone platform - several important fixes were made for EAConfig's run target for the Windows Phone platform. If you are using EAConfig's run target, you should definitely update to this release.
*   On Visual Studio 2012, the /sdl compiler switch is used and the /GS compiler switch is no longer used. If you see warnings about /GS- is overridden by /sdl-, you should update to this version of eaconfig.

### Bug Fixes

*   ETFW-2589 visualstudio-open target is working correctly again.

# 2.08.00 (May 6, 2013)

### Changes

### Features

*   ETFW-2341 Adding Before and After attribute to MSBuild custom build tasks to allow for prerun build steps through Visual Studio and nant builds. Set [groupname].[modulename].custom-build-before-targets and [groupname].[modulename].custom-build-after-targets to use this feature.
*   ETFW-1598 Added support for Python type Visual Studio project generation. Use project build type "PythonProgram". Framework-3.10.00 or later is required to use this feature.

### Bug Fixes

*   ETFW-2493 Windows Phone 8: app.activationpolicy default option is now set to "Resume".

# 2.07.00 (Apr 8, 2013)

### Changes

### Features

### Bug Fixes

*   ETFW-2355
    *   Fixed defect building PRX modules for PS3\. The fundamental problem was that a typo in BuildTaskBase.CreateProcess caused the working directory to be set to the executable directory instead of the build output directory. This affects the PS3 linker when building PRX module because the tool has no option to set the location of the stub output library name or directory - it is always called "prxlib_stub.a" and it is is placed the current working directory. Fixed.
    *   2nd defect: in eaconfig, the linker post build step which copies the stub library file from the build directory to the project library directory assumes it will be executed in the linker output directory. But that directory wasn't being passed to the command when it was created, causing the copy to silently fail. Fixed.
    *   3rd improvement (not required): use Framework's copy_32 tool to copy the stub library instead of Windows' xcopy in the PS3 post-build step (eaconfig change)
*   ETFW-2363 Fixing Paths to specific WindowsSDK tools.
    *   Adding properties to WindowsSDK that provide the path to specific tools in case they change locations in future versions.

*   ETFW-2344 Fixed error running EADotNetMathTests. Package appeared to be failing because visual c++ 2010 or later can only build dlls targeted toward .net 4 or higher. To prevent future confusion I made a change to eaconfig to check the version of visual studio and dot net and fail if .net is lower than 4 with a visual studio version greater than or equal to 10.
*   Grab xcode-ios-project-dir from XcodeProjectizer, so run target knows where to find the xcode project.

# 2.06.00 (March 4, 2013)

### Changes

*   The following deprecated eaconfig properties have been removed:

    ```
	- package.DotNetSDK.version
	- package.DotNetSDK.support64bit (it was always true in previous published versions)
	- package.DotNetSDK.appdir
	- package.DotNetSDK.libdir
	- package.DotNetSDK.includedirs
	```

    These properties have been superseded by package.${WindowsSDKName}.appdir (or equivalent derivatives).

### Features

*   eaconfig's run target for the WinPRT platform always force-kill's devenv to clear a potential existing raised exception dialog box.
*   UNIX assembly source now compute dependency files so incremental builds are faster.
*   enable C++ 11 support on PS3 SDK 4.30 or higher.
*   valgrind exits with an error when leaks or memory overwrites are detected.
*   in the eaconfig-run target, it is now possible to call targets before the run target. Do this by setting a property with the pattern "eaconfig-init-deploy.${config-system}.targets" to the list of targets to be be executed before eaconfig-run. This is required by the latest EAMConfig packages so the correct fileset for packaging for Playbook and BB10 is defined.

### Bug Fixes

*   ETFW-2310 WinPRT -nodefaultlib:ole32.lib option fixed. Linker options to DynamicLibrary type:  

    x86: -nodefaultlib:ole32.lib  
    arm: -nodefaultlib:shell32.lib
*   run target fix: subst %outputname% in linkoutputname template when computing folder.
*   When using Windows SDK 7.x, use resgen tool for .NET 4.x, not 2.x.

# 2.05.00 (February 4, 2013)

### Changes

*   ETFW-2142 WinRT auto-deploy step: move deploy step from visualstudio target to run target

### Features

*   ETFW-2206 Added new Visual Studio Solution Opening Targets:  
    visualstudio-open  
    visualstudio-example-open  
    etc  

*   ETFW-2097 New Build Types Needed for Windows Phone 8
    *   Added new buildtype WinRTRuntimeComponent
    *   Set exact platform (x86 or ARM) for C# projects in winprt
    *   added support for ARM platform in .Net projects
    *   Winphone solutions may require C# projects to reference native projects. This was disabled in sln generation. Now references are created for all dependent types.
*   Added [group].[module].delaysign convenience property
*   ETFW-2097 Added support for C# and MCPP projects for Windows Phone 8
*   ETFW-2274 Added support for F#?  
    Removed dependency on FSharp package

### Bug Fixes

*   ETFW-2244
    *   files ending in .rc or .resx should not be included in the EmbedManagedResourceFile child node of the linker (if they are present, they're ignored by the linker). They are in the EmbeddedResource node.
    *   note: this change will break vstomaketools, which is expecting the rc & resx files in the EmbedManagedResourceFile node, so vstomaketools will require an update to look in the correct location.
*   ETFW-2244
    *   corrected 2nd problem in Max's email, in which an absolute path & and a relative path resolve to the same file, and the copy command fails. I am using the copy_32 utility program Igor added a couple of weeks ago to the Framework bin directory to ignore the copy command if the absolute paths of the source & destination files turn out to be identical.
    *   no vstomaketools binary update yet because the 1st problem still is not fixed.
*   ETFW-2248 C# projects generate their manifests correctly now. Managed C++ still not working; the manifest tool parameters are not being passed correctly to the tool.
*   ETFW-2238 Fixed mscoree.lib location with VS2012
*   ETFW-2242 Fixed compatibility of WindowsSDK 7 and eaconfig
*   ETFW-2019 -FU (ForceUsing assembly) now works correctly. It is no longer necessary to explicitly list the assemblies in the [groupname].[module].forceusing-assemblies fileset.
*   ETFW-2175 Do not apply --write-fself-digest to SPU
*   ETFW-1853 Fixed PS3 PRX generation issue: Prx files are now created in the bin dir, and sub libraries in the lib dir (VS2010)
*   ETFW-2137 Fixed bug where new Xbox 360 gameconfig fails to include the .spa file in final XEX
*   Fix in cc.cpp11 logic: Do not set cpp11 options if flag cc.cpp11 explicitly set to off

# 2.04.03 (December 3, 2012)

### Changes

*   ETFW-1567 - eaconfig sets _CRT_SECURE_NO_WARNINGS define if flag "disable-crt-secure-warnings"=="on". By default it is "on" Explicit warning suppression "-wd4996" (Disable warning: 'strncpy' was declared deprecated. net2005 specific) is removed, deprecation warnings should be suppressed by _CRT_SECURE_NO_WARNINGS
*   extended unit test timeout for winprt.

### Features

*   Added Unix-clang C11 options controlled by a property (cc.cpp11 = on/off)

### Bug Fixes

*   ETFW-2130 Fix for comreferences and Windows SDK 8

# 2.04.02 (November 5, 2012)

### Changes

### Features

*   ETFW-1722 COM assembly references are now added to generated VCXPROJ files.

### Bug Fixes

*   ETFW-2039 Makefiles can now be generated without Incredibuild being installed. Use target "distributed-build" and properties "incredibuild.xgemake=false" and "incredibuild.xgemakefile=true".
*   ETFW-1954 Leave EmbedManifest field empty if it is not explicitly defined. This was breaking the ANT Tool for UFC.
*   ETFW-1722 COM assembly references are now added to generated VCXPROJ files.
*   WINRT: use project package.appxmanifest as default manifest filename for winrt projects but allow users to override it from their .build file with a custom property.
*   WINRT: remove underscores from project name in winrt / winprt app manifest as they are rejected by MSBuild when packaging app.