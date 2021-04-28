---
title: Change Log
weight: 5
---

The following is the changelog for the Framework package.

# NEXT RELEASE (This release is currently in development and all items here are subject to change.)

### Changed

### Removed

### Added

### Fixed


# 8.07.01

### Removed

* Removed FSharp support from Framework.  This has not been supported for years however the code was still in place.
* Removed lingering references to EASharp

### Added

* Updated masterconfig file's `<config>` element to support the following extra attributes to allow host OS specific settings: `default-osx`, `includes-osx`, `excludes-osx`, `default-unix`, `includes-unix`, and `excludes-osx`.  IMPORTANT NOTE:  Once your masterconfig added these new attributes, you cannot use old version of Framework to load this masterconfig.  Old Framework versions will complain about invalid attributes.

### Fixed

* Fixed error with Framework's rmdir.exe accidentally got removed since Framework 8.06.00 release.  It is now added back in.


# 8.07.00

### Changed

* Ondemand download settings are now able to be set in masterconfig fragments.  ```<packageserverondemand>false</packageserverondemand>``` and ```<ondemand>false</ondemand>``` as an example.
* Ensuring that environment variable names set via the exec.env property are converted to all uppercase when passed to the exec task's environment. Property names are case insensitive in framework and stored as all lowercase but environment variable names are case sensitive on linux and typically all uppercase. This won't handle mixed case environment variable names but should fix the majority of cases for now.
* The nugetreferences block in the build files are now a property instead of a fileset.
* VisualStudioProject tasks (which give the ability to depend upon existing VS projects instead of generating them) now require a ProjectOutput parameter (which contains the path to the output assembly).
* VisualStudioProject tasks take in an optional debug symbols path.
* Copy operations in .vcxproj files now use exact timestamp comparison and will copy (preserving timetamp) if destination is different. This gives a more reliable up to date check for copies. Any .csproj files still use newer timestamp checks and update timestamps on each copy to avoid issue with Visual Studio update to date checking. Note this will not be required when new .csproj format is adopted.

### Removed

* package.VisualStudio.legacy-properties and the legacy-properties.xml file associated with it.  These legacy Properties were disabled for use in frostbite for nearly 2 years now, so we are confident they are not used anywhere.
* Removed the sln-taskdef target, which was previously used for generating a solution for taskdef files but its used much anyone and not very well maintained.
* Removed deprecated generateDefaultBuildLayout task.  This was marked deprecated via warnings in April of 2019.

### Added

* [iOS] Added EA_PLATFORM_IPHONE=1 define to all iOS build (to be consistent to all other platforms).
* [iOS/OSX] Added 'codesign-entitlements' platform property and 'xcode-buildsettings' platform optionset in module build script setting to support setting property `[group].[module].${config-system}-codesign-entitlements` and optionset `[group].[module].${config-system}-xcode-buildsettings`.
* [iOS/OSX] Added `objective_c_cpp` build option to all objective c/c++ build optionsets.
* [iOS/OSX] Added `app-extension` build option to `ObjCAppExtension` and `ObjCppAppExtension` build optionsets.
* [iOS] Added `framework-bundle` build option to iphone config's `DynamicLibrary` build optionset.
* [OSX] Added 'lib' prefix to dynamic library (.dylib) build output to be consistent with typical OSX dynamic library name format.
* [OSX] Added buildtypes `OsxConsoleProgram`, `OsxCConsoleProgram`, `OsxObjCppConsoleProgram`, `OsxObjCConsoleProgram` (derived from corresponding Program build types to add a `osx-console` build option).
* [OSX] Added basic support for building against osx-arm64 architecture.
* [Outsourcing Tool] Added support for property 'package.[package_name].PrebuiltExternalIncludeSourceBaseDir' to allow people list files in 'package.[package_name].PrebuiltIncludeSourceFiles' that is outside it's own package.  Added this support mainly to deal with another hack that people do in creating a dummy package (like AudioFb) that just re-direct everything to another folder.
* [Outsourcing Tool] Added support for export property 'package.[package_name].internalIncludeDirs' and 'package.[package_name].[module_name].internalIncludeDirs'.  Frostbite is currently using this for includes files that are intended for "Frostbite" team internal references but they get referenced by "public" headers which means these headers actually need to be made public to outsource partners as well.

### Fixed

* Fixed crash in <synctargetdir> task when fileset-names was unspecified.
* Fixing a potential linux nant build error when the compiler generated dependency (.d) file contains multiple file in single line.
* Fixed bug in eapm where on linux it would fail to post packages because it was trying to use windows style slashes in the path

# 8.06.01

### Fixed

* Update PS5/Kettle run logic to account for nullable exit codes (CL 9439902)

# 8.06.00

### Added
* Implemented markOldOfficialAsAccepted for v2 package server api.
* Retry get download URL in case some timeouts occur (may be seen periodically if connection to server fails), added greater error logging to help discover this. 
* Added ability to turn off packageserver on demand package downloads.  set ```<packageserverondemand>false</packageserverondemand>``` in your masterconfig.xml to use this feature.  The default is "true" if you have ```<ondemand>true</ondemand>``` set.

### Changed
* Made PackageServerIncomingFolder path use platform agnostic directory separator char.
* Package posting recognises a manifest file in lower casing.
* Modules that explicitly export their primary output file (such as a .dll) will have optionset and basedir preserved even if global path remapping is used. This allows copy options ot be set for example.

### Removed
* Incorrect help message in eapm post command.

### Fixed
* Package Server token refreshed upon use with new token provided from package server
* Fix error with Framework.build itself.  It was attempting to include script/CustomTargets.xml file that no longer exists.
* Fixed .Net Core command build to automatically include Microsoft.Desktop.App's reference directory on PC build.
* Updated .Net Core's command line build to automatically create the apphost exe and a basic minimum runtimeconfigs.json file which is required to execute the application.
* Fixed NAnt.NuGet module to use NuGet.PackageManagement instead of NuGet.PackageManagement.NetStandard if targetting .Net Framework build.  Otherwise, it can cause issues with
  classes being defined in multiple assemblies build error.
* Small Framework changes to allow it to be built on OSX host with mono using "Visual Studio 2019 for Mac" and debug Framework on a Mac.  NOTE: if you want to debug and 
  set breakpoint to external package's "taskdef" source code, you need to do the followings:
  * Create a .csproj file for that external package then manually add that to the Framework solution.
  * After you added the csproj to Framework solution, you will need to close it and reopen the solution for Visual Studio to reload the dependent NuGet packages.
  * Locally modify the taskdef output location to match up with your csproj's output location (see below example).  Then do a full rebuild first. 
    ```
    For example, if your csproj has:
    <OutputPath>$(SolutionDir)Local\sln-taskdefs\bin\</OutputPath>
    then locally change your <taskdef> to output to same location:
    <taskdef assembly="${nant.location}/../Local/sln-taskdefs/bin/XcodeProjectizer.dll">
    ```
  * Depending on which version of Visual Studio 2019 for Mac you are using, you may also need to locally update the ```<BaseIntermediateOutputPath>``` field in the csproj
    or Directory.Builds.props file to put in a hard coded path instead of using $(SolutionDir).  There is a bug with NuGet package restore not respecting the $(SolutionDir) info
    when creating the MSBuild files.  https://developercommunity.visualstudio.com/content/problem/1256608/looks-like-visual-studio-for-mac-doesnt-respect-th.html#
  * After this change, you should be able to set breakpoint and start the debugging.
* Fixed NX build to not incorrectly add NRO dependency to AdditionalNRODependencies field in vcxproj that came from "interface" only depedency chain.
* Made Eapm Exists and History commands work without having to define -pathtopackageservertoken.
* Get all releases will now actually return all releases of a package.
* Modified Framework's copy.exe to use the same named mutex guard as Framework's MSBuild copylocal task's named mutex to guard around both process trying to do copy to
  same destination during build by different build threads.

# 8.05.00

### Changed
* Only validate the package server once during the application lifetime
* The Framework Installer has been deprecated and removed after conversations with various TDs across EA have confirmed they no longer use it.
* Updated module debug symbol file copying logic to be platform agnostic. Any module with a 'linkoutputpdbname' option that outputs a binary will also have a non-failing copy added for the debug symbol file in that path.
* Enabled Stadia stripping of binaries when building from Visual Studio. This will create a separate .debug file alongside your binaries. Transitive binary copies will also have debug files copied.
* Enabled dead code stripping for Stadia (compile: -ffunction-sections -fdata-sections, link: --gc-sections).
* Changed default symbol visibility for *static* Stadia builds (compile: -fvisibility=hidden). This can improve binary size, performance and helps ensure API boundaries are maintained however will required code that needs to be dynamically linked to be marked with visibility attributes. In shared Stadia builds it was decided not to enable this due to too much code existing that is not setup with these attributes.
* Libraries libvulkan.so, libpulse.so are no longer linked by default for Stadia platform.
* [iOS] Updated to no longer add OpenGLES to list of default iOS framework to link with.
* [iOS/OSX] Added "projectize-groupall" target to generate Xcode projects that contains all build groups.
* [OSX] Added compiler option -msse4.2 for OSX build.  There are projects needed that option to be added to default.
* [iOS/OSX] Updated to allow generating buildlayout file for icepick.
* Updated Framework's copy.exe to accept a -c switch to not preserve readonly attribute after copy.
* Small update to Framework's copy.exe/mkdir.exe/rmdir.exe to run properly on osx/linux host machines.
  * Updated Framework's copy.exe/mkdir.exe/rmdir.exe to not use forward slash '/' as command line switch indicator on osx host or linux host machine.  
    The foward slash is a starting full path character on linux type host machine and cannot be used as a command line switch indicator.
  * Updated auto prepand of 'mono' in property ${nant.copy}/${nant.mkdir}/${nant.rmdir} to provide the full path to mono on osx host.  (When Xcode execute shell script, it does not preserve user's PATH environment).
* Add FrostingReporter capability to test-run-fast (using the target test-run-fast-frosting). Currently only for PC.
* Prompt for authentication with Package server now uses same form as prompt for Perforce Credentials in cases where token is missing from credstore or invalid.
* Updated the remove define support for BuildType task to also remove defines with assignment (rather than just expecting simple exact match):
  ```
  <BuildType name="MyBuildLibrary" from="Library">
      <remove>
          <!-- The following will now remove "-D ABC" and "-D ABC=..." from compiler defines options. -->
          <defines>
              ABC
          </defines>
      </remove>
  </BuildType>
  ```
* [Outsourcing Tool] Fixed a situation where a module is declaring publicbuilddependency or publicdependency to a module that doesn't have 
  ```package.[package_name].[module_name].includedirs``` property setup.  This normally show up due to user build script error where people 
  inappropriately add C# module to the list (C# modules has no header files and no includedirs, so should not be listed in publicbuilddependency
  or publicdependency sections).  For now we just ignore this property if it is null.
* Nant & eapm use V2 authentication for package server
* If files in the ```<custombuildfiles>``` fileset are assigned an optionset name for an optionset that does not exist this will throw an error. Previously this was only a uncoded (never thrown as error) warning.

### Added
* Credstore - allow user to output package server v2 auth token to file and/or to console output.
* Credstore - added nant options -usePackageServerV1 to set whether to use the V1 package server which will soon be deprectated
* Credstore - added nant option -packageservertoken: to allow the auth token for package server be passed with the gen
* EAPM - added option to set to use V1 within eapm.exe.config
* [iOS/OSX] - Added structures to support specifying setting 'skip-install', 'install-owner', 'install-group' and archive ExportOptions.plist content through 'archive-export-plist-data' or 'archive-export-plist-file'.
  ```
  <Program name="...">
    <platform>
      <iphone>
        <!-- These new settings require latest version of XcodeProjectizer -->
        <skip-install>false</skip-install>           <!-- Need to set this to false to allow this program module to create archive export .ipa file -->
        <install-owner>...</install-owner>           <!-- If not provided new XcodeProjectizer will now create an empty field as default -->
        <install-group>...</install-group>           <!-- If not provided new XcodeProjectizer will now create an empty field as default -->
        <archive-export-plist-data>                  <!-- User specified extra felds to add/update default generated ExportOptions.plist -->
          <option name="thinning" value="&lt;none&gt;"/>  <!-- needs to be set to none (default) in order to create a single .ipa file -->
        </archive-export-plist-data>
        <archive-export-plist-file if="${frostbite.variant}==retail">  <!-- Allows user to provide their own ExportOptions.plist.  Using this will ignore 'archive-export-plist-data' settings. -->
          ${package.dir}/data/AppStore-ExportOptions.plist
        </archive-export-plist-file>
      </iphone>
    </platform>
  </Program>
  ```
* [iOS/OSX] Added target 'xcode-archive-export' to use XcodeProjectizer's 'build-xcode-archive-export' target to build the archive and then export the archive as ipa.
* [iOS/OSX] Added target 'generate-ipa-from-app' to use XcodeProjectizer's 'generate-ipa-from-appbundle' target to generate ipa from previously built app bundle.
* Arbitrary attributes are now supported on the masterconfig's grouptype nodes and can be accessed in taskdef code through a dictionary.

### Fixed
* Fix incorrect ProjectReference setup when we have solution with mixed platforms.
* Fixed Linux vcxproj creation not setting the correct "Linux" project keyword.  It was caused by incorrect testing of Visual Studio version.  That check is now removed.  If people are creating Linux project on
  Visual Studio 2017, they will need to explicitly set global property 'eaconfig.unix.use-vs-make-support' to false as the new Linux project format is only for Visual Studio 2019.
* Fixed the "package.all" property creation.  It was previously returning the object class name instead of the actual package list.
* Fixed typo in package server post v2 that was introduced in prior refactor. 

# Frostbite 2020.Alpha

### Fixed

* Updated stadia_copy to ignore ".sig" and ".debug" files.
* Added a null check before comparing legacy version (package server release may not always have legacy version).
* Override MS link resolve targets with a verison that respects the LinkLibraryDependencies metadata - prevents an error where sometimes transitive dependencies get linked when they shouldn't
* Add env var opt in msbuild itemgroup metadata debug dump task - useful for reverse engineering wierd VSI issues
* Fixed serveral stadia deploy issues.
* Fixing the issue where taskdefs are compiled with an old version of C#
* Updated nant to set the TargetDeviceFamily attribute in the microsoftgame.config to allow us to run XB1GDK apps on a XBSX kit.
* [Outsourcing Tool] Fix an issue with some package's "publicdependencies" and "publicbuilddependencies" usage between two interdependent packages causing a circular loop.

# 8.04.05

### Changed

* The validate-visual-studio-components task no longer skips the check if package.VisualStudio.use-non-proxy-build-tools is true. It is now up to the caller to differentiate.
* Changed C# build on osx host and unix host machine to use Microsoft Visual C# compiler 'csc' directly (if available) instead of using mono's C# compiler 'mcs'.  'csc' is now available in newer version of mono and it is needed for some of the newer NuGet download packages.
* The number of temporary folders for nant telemetry have been limited to 30. It will reuse the least recently touched when looking for a new temporary folder. Any older folders in excess of 30 will be deleted.
* Added compatibility with Package Server API V2 for getting latest package & package list information. Added credential storage for token for authentication with package server interactions. 
* capilano config-options-common and config-options-program now have exceptions enabled only if you are building for xb1 xdk, if you are building with the GDK exceptions are now disabled.
* xbsx config-options-common and config-options-program now have exceptions set to the same default value as other platforms (off).
* [NX] Updated eaconfig's ```<nxRun>``` task to detect local host PC is setup using htc-gen2 (new host-target-communication gen2 protocol available starting from nxsdk 10.4.0) and then
  launch the appropriate HTC background app before launching a run on the console.
* Remove post-load step for eaconfig, which occurred after config extensions were loaded. Merge target-internal.xml, load-post-cache.xml and target-postcache-init.xml config files into targets.xml for eaconfig.
* [Mono] Disable warning as error in ```<taskdef>``` compile operation.  Taskdef's code compile uses old version of CSharpCodeProvided class from an older version of System.dll assembly while PC version uses a newer version which is capible of dealing with newer assemblies version resolve from NuGet download.
* [Mono] Changed ```<taskdef>``` up to date check to ignore the .dep dependency file if the file is missing and building under mono.  This file can go missing for some compiler warning that is being triggered.
  We need to suppress this to avoid parallel process build in Xcode trying to rebuild the same task at the same time.
* Internal parallelism changes for package loading to use far less thread pool tasks. Should help with nuget download which were slow sometimes to the point of failing due to pool starvation.
* Removed 'visualstudio.use-versioned-vstmp' property. This property was intended to allow VisualStudio version switching by including verison in Visual Studio intermediate directory but never worked reliably.
* Changed Visual Studio intermediate path, .tlog path and .pch path to all be in a short name folder in module intermediate directory (i.e /v/, /v.tlog/, /v.pch/). This gives some more play with regard to long paths by stopiing Visual Studio introducing folder to avoid collisions ni the same way Framework as already done at a higher level.
* Changed default folder for generated files in Visual Studio from __generated__ to _gen_ to reduced path lengths.
* Updated the solution generation output *.user file to have proper XML formatting.

### Added

* Allow user to create new package when submitting release
* Allow for V2 posting to the Package server enabling direct uploads to storage.
* Added the ability to target .net standard and .net core. Package maintainers can set the targetframeworkversion to make their module target .net standard or .net core.
nugetreferences can also be added to modules.
  ```
  <CSharpLibrary name="MyLib">
    <config>
      <targetframeworkversion>netstandard2.0</targetframeworkversion>
    </config>
    <nugetreferences>
        <includes name="System.ComponentModel.Composition" asis="true"/>
    </nugetreferences>
  </CSharpLibrary>
  ```
And the versions of NuGet packages specified in as NuGet references are listed in a new section of the masterconfig.
  ```
	<nugetpackages>
		<nugetpackage name="System.ComponentModel.Composition" version="4.7.0"/>
	</nugetpackages>
  ```
Since this feature is brand new it is still somewhat experimental. For the moment, we would like people to try it out and provide feedback.

* Added support to allow people to specify a list of modules (in dependency spec format [package]/[group]/[module_name]) to force
  disable optimization build switches in global property "eaconfig.${config-name}.modulesForceDisableOptimization" or "eaconfig.modulesForceDisableOptimization".  For example:
  ```
  nant.exe ... -G:eaconfig.modulesForceDisableOptimization="Math/Engine.Math EAThread"
  ```
* Framework targets which override targets, can now call their 'base' targets by using the ```<call-base/>``` task.
* Added new failonmissing attribute to the ValidateVisualStudioComponents task which, if false, will cause the task to not fail if VS isn't installed (default is true).

### Fixes

* Fixed eapm install command to allow you to pass properties to it when specifying a masterconfig.  This allows the masterconfig exception blocks to be evaluated when determining what version of a package in the specified masterconfig should be installed.
* Fixed eapm where command to allow you to pass -buildroot parameter.  This is sometimes required if the masterconfig you are using uses the ```<buildroot allow-default="false"/>``` option.
* Fixed package.VisualStudio.useCustomMSVC property when generating projects that didn't explicitly have MSBuildTools in their dependency chain. Also fixed some warning spam that occurred if you're using an older version of MSBuildTools that doesn't support this feature.
* Fixed capilano and stadia's config setup to automatically define property vsversion.${config-system} and updated Framework's internal code to test for vsversion.${config-system} property before checking vsversion property.
* Fix error with "eaconfig.enable-pch" global property setting being broken.
* [NX] Fixed typo with solution generation when global property 'nx.application_program_format' was used and explicitly set to 'raw'.
* Adding packageId field when using the new csproj format so that NuGet doesn't fail with ambiguous module name warning when modules in different build groups have the same name
* Fixed webapp C# projects to work with the new csproj format. With the new csproj format instead of overriding the BeforeBuild target, to add the step that copies the web.config file, it should have a target that runs after the BeforeBuild target using the BeforeTargets attribute.
* [Outsourcing Tool] Fix an error where if source package didn't provide a Manifest.xml, the output prebuilt package was missing a Manifest.xml(which is necessary when we change the output version name).
* [Outsourcing Tool] Fix an issue with not preserving the link="false" attribute in  ```<auto link="false">``` and  ```<build link="false">``` dependency specification for the generated prebuilt package.
* [Outsourcing Tool] Updated the preserve-properties setup to not create that property unnecessary in the final prebuilt .build file if that property's value is empty.
* [SharedPch] Updated to detect if a module's shared pch binary setting is changed from previousl solution during incremental gensln.  If this happen, it will automatically delete old incompatible *.obj and the compiler pdb file.
* Fixed Stadia solution generation's ExecutablePath setup added incorrect path to the toolchain directory.  Also corrected some MSBuild path property variables to have consistent backslash and consistent backslash ending that the VSI was expecting.
* Corrected OSX/linux host's csc version detection log output.
* Fixed Linux build having incorrect TargetName and TargetExt setup caused by earlier stadia build fix.
* Fixed Linux build having incorrect Working Directory setup when earlier Framework release attempt to add a default $(OutDir) to working directory.
* Fixed masterconfig writer to also write out uri info in the exception / condition fields.

# 8.04.04
### Fixes

* Fixed an issue where an invalid .androidproj file would be generated for .aar projects if the AndroidManifest.xml file was in a non-standard location even if correctly referenced from build.gradle file.

# 8.04.03

### Changed

* The <XdkEditionTarget> global property is now set in all GDK projects, to ensure we target the correct GDK version when building, in case the user has multiple versions installed. This was previously done by the GDK non-proxy package, but this property should also be set when using the proxy package.
* [ios/osx] Refactored ios/osx config setup to be more closely in-line with other platforms and remove support for deprecated properties such as iphone-base-sdk-version and osx-base-sdk-version.
* [ios/osx] Added precompiled header support for IOS/OSX command line build.  (If using XcodeProjectizer, requires version 3.08.04).
* [ios/osx] Integrated changes to support setting -flto=thin compiler and linker switch if build option 'optimization.ltcg' is turned on.
* [SharedPch] Changed the default for property ${config-system}.support-shared-pch from true to false. (ie From now on, for platforms that support shared pch binary, this property needs to be explicitly provided).
* [SharedPch] Fixed copying of shared pch's pdb file to do more explicit timestamp compare of the shared pch's pdb vs module's pdb before copying to avoid corrupting pdb file.
* [stadia] Small change to staida platform's precompiled header support to attempt to find the pch header file's directory if the information is not provided.
* Properties with nested elements will now result in an error instead of a warning.

### Fixes

* Fixed an internal nant error when setting package.VisualStudio.useCustomMSVC to true without also setting package.VisualStudio.use-non-proxy-build-tools to true. Now, an info message will be printed and useCustomMSVC will be ignored.

# 8.04.02

### Fixes
* Fixed a possible null exception on start up when using -D:eaconfig.includegroupinbuildroot=true.

# 8.04.01

### Fixes
* Fixed Android issue where Program modules with a custom Gradle file but no custom manifest wouldn't generate the default AndroidManifest file.
rnal nant error when setting package.VisualStudio.useCustomMSVC to true without also setting package.VisualStudio.use-non-proxy-build-tools to true. Now, an info message will be printed and useCustomMSVC will be ignored.

# 8.04.00

### Changed
* Configuration loading order has been changed so that configuration properties (such as ```config-system``` are resolved at the start of .build files and in the case of masterconfig, initialized before first package location is resolved. This allows packages selected by ```-buildpackage:``` to use masterconfig exceptions based upon config properties. 
** NOTE: This also applies to masterconfig global property conditionals. For example, ```vsversion``` property can be set based on config-system.
* Visual Studio version is now initialized on a per configuration basis allowing for different Visual Studio versions to exist in the same build graph. NOTE: You will still get an error if you try to generate a single solution file with different Visual Studio versions but this change allows a buildgraph for multiple configurations with different Visual Studio versions to be generate or sub ```<nant>``` calls for a diffrent configuration to switch to a different vsversion. Visual Studio version must still evaluate to a single version for each configration however, sub ```<nant>``` calls for the same configuration with a different Visual Studio version are not supported without custom configurations.
* Framework will now always accept the top level version of package (i.e. one that is implied by working directory or ```-buildfile:```) even if it conflicts with masterconfig version. To ensure masterconfig version is used use ```-buildpackage:```. This is equivalent to old nant.usetoplevelversion=true behaviour (this property is now ignored).
* Adjusted a change in Framework 8.03.00 that required top level build file to conform to package directory structure. This rule is ignored for top level build files (even if they run <package> task).
* Platform config packages (<platform>_config e.g. android_config) will no longer be implicitly search for when configuration extension packages are not used.  To add addtional configs please use configuration extension packages.
* The 'extra-config-dir' attribute of masterconfig ```<config>``` block is no longer supported and will be ignored with a warning. To add addtional configs please use configuration extension packages.
* The ```<gameconfig>``` element of masterconfigs is no longer supported and will be ignored with a warning. To add addtional configs please use configuration extension packages.
* Removed hardcoded "disable deprecation warnings as errors" on clang platforms in the solution generation (nant) (except for objective-c and objective-c++ files).
* The android_config package now comes with bundled with Framework - versions specified in the masterconfig will be ignored with a warning.
* [IOS/OSX] Updated the default ios and osx setting to build against c++17. If you need to revert back to old default c++14, set global properties cc.std.iphone.default=c++14 or cc.std.osx.default=c++14 as appropriate.
* [Android] Updated the default android setting to build against c++17. If you need to revert back to old default c++14, set global properties cc.std.android.default=c++14.
* Updated IMCPPVisualStudioExtension class to allow people insert extra platform property sheets under "PropertySheets" ImportGroup in Visual Studio project generation.
* Nuget protocol layer will preserve the marker files and internal nuget folders from older Framework releases. If this Framework release clean downloads a package it will still be delete and redownloaded by older releases but older releases downloads will be in place made compatible with this release. This allows this Framework release to share nuget packages with older releases.
* Changed Stadia mount / unmount command to use the 'instance' keyword instead of the deprecated 'gamelet' keyword.
* Updated Visual Studio C++ project generation to properly trimmed file level compile override elements that is actually identical to project level.
* Changed clang platform's 'optimization.ltcg.mode' optionset's 'usesampledprofile' mode to detect if 'debugsymbols' is also turned on.  If not, it will now have to build error to make sure the usesampleprofile option is set correct.
* We now define _GAMING_XBOX_XBOXONE on xb1, _GAMING_XBOX_SCARLETT on xbsx, and _GAMING_XBOX on all Xbox platforms, as recommended by Microsoft. This also requires updating EABase to at least 2.09.13, since EABase previously used the wrong defines to differentiate between platforms.

### Added

* New csproj format can be generated using the global property new-csproj-format=true. This is an opt-in feature as we continue to test it. It can also be toggled per package with the property package.(packagename).new-csproj-format.
* Custom masterconfig fragments can be added via the command line now.  Use for example -masterconfigfragments:"C:\my_personal_fragment.xml C:\my_teams_fragment.xml" to add masterconfig fragments without needing to adjust/change your masterconfig.xml directly.
* AndroidAar build type and structured module definition task has been added. This functions very similarly to JavaLibrary however it will creae an Android project in Visual Studio to package up your library into an .aar that is output to the package lib folder.
* JavaLibrary build type will now create a Utility Visual Studio project with no build steps, but that contains .java files for reference.
* Added new sys.localnetworkaddress property that contains the host machine's network address (the first one in the list returned by IPUtil.GetUserIPAddresses()), or String.Empty if we couldn't get the address.
* Added new sys.localnetworkaddresses property that contains the host machine's network addresses (as returned by NetworkInterface.GetAllNetworkInterfaces()), or String.Empty if we couldn't get the address.
* Added SPGO support for XB1
* Added ability for user to specify copylocal of pdb files from external dlls fileset items by providing an optionset with option name 'copy-pdb' and value 'true'.  For example:
  ```
	<optionset name="copy-pdb-on-retail">
		<option name="copy-pdb" value="true"/>
	</optionset>
	<Program name="mymodule">
		...
		<dlls>
			<includes name="${package.CapilanoSDK.extensionsdkdir}\Xbox.Game.Chat.2.Cpp.API\8.0\Redist\CommonConfiguration\neutral\GameChat2.dll" optionset="copy-pdb-on-retail"/>
		</dlls>
	</Program>
  ```
* Updated precompiled header support to allow using compiler forced include syntax so that people don't need to modify all their source code with ```#include "pch.h"```.  
  To activate this feature, user build script need to provide use-forced-include and pchheaderdir attributes.  Example:
  ```
	<Program name="mymodule">
		<!-- set use-forced-include attribute to true and must set pchheaderdir attribute to provide the path to 'pchheader'. -->
		<pch enable="true" pchheader="forced_inc_stdafx.h" use-forced-include="true" pchheaderdir="${package.MyPackage.dir}\include\pch"/>
		...
	</Program>
  ```
  Note that when forced include is used, it will apply to all source files.  If you have mixed filetypes that compile differently in your project where the header should not be used, you need to re-assign those
  files with original optionset without 'use-pch' option in the optionset (or have it set to 'off').
* Added support for using shared precompiled headers binary (for platforms that support it, namely pc/pc64, ps4, ps5, unix, and xbsx).  Created a new
  module type ```<SharedPch>```, public data ```<sharedpchmodule>```, and ```<usesharedpch>``` build module element under a regular module definition. 
  1. Example Initialize.xml to define shared pch module:
  ```
	<publicdata packagename="BasePackage">
		<runtime>
			<sharedpchmodule 
				name="Base.SharedPch" pchheader="BasePch.h" 
				pchheaderdir="${package.BasePackage.dir}/includes/Base.SharedPch" 
				pchsource="${package.BasePackage.dir}/PchSource/BasePch.cpp">
				<!-- 
				If any intended target module build files uses any defines that affect the BasePch.h content, 
				then this pch binary needs to be built using same define as well or that target module cannot
				use this shared pch binary.
				-->
				<defines>
					UTF_USE_EAASSERT=1
				</defines>
				<publicdependencies>
					EABase
				</publicdependencies>
			</sharedpchmodule>
		</runtime>
	</publicdata>
  ```
  2. Example build file to define shared pch module:
  ```
	<SharedPch name="Base.SharedPch"/>
  ```
  3. Example usage of shared pch module.
  ```
	<Library name="MyLibrary">
		<config>
			<pch enable="true"/>
		</config>
		<!-- If pch is disabled above, only the header of Base.SharedPch will get forced include to current module --> 
		<usesharedpch module="BasePackage/Base.SharedPch"/>
		...
	</Library>
  ```
* The outsourcing tool ('create-prebuilt-package' task) has also been updated to recognize the SharedPch module and not package the SharedPch module's output.
  The generated precompiled header (pch) file contains full path to the original header being used and cannot be cached and reused on different machines with different path structure.
* Updated vcxproj generation for snowcache to recognize a SharedPch module and try not to cache SharedPch's output (same reason as the outsourcing tool update above).
* Added precompiled header support for stadia platform.  However, due to the nature of Stadia's VSI, in order to use precompiled header, the module need to provide
  a new header file directory input.  It can be set as a property with "[build_group].[module_name].pch-header-file-dir" or as a ```<pch>``` element's 'pchheaderdir' attribute.
* Added some new fields to the MicrosoftGame.config template, to reflect changes introduced in the latest GDK.
These are optional fields that can be set by using the following options in the config-options-gameconfigoptions optionset.
The options virtualmachine.titlememory.xb1x and virtualmachine.titlememory.anaconda replace title.xboxonemaxmemory and title.xboxonexmaxmemory as ways of specifying memory.
The fields ContentIDOverride and EKBIDOverride can also now be set with the options title.ekbidoverride and title.contentidoverride, see the GDK docs for more details.

### Fixes

* [NX] Fix NX vcxproj generation to use correct PlatformToolset settings (should have been v141 for VS 2017 and v140 for VS 2015 instead of clang)
* [NX] Fix NX vcxproj to import $(NintendoSdkRoot)\Build\Vc\NintendoSdkVcProjectSettings.props at correct sequence.  Incorrect sequence can cause impact to using prebuilt libraries when prebuilt library contains override functions like nninitStartup().
* [NX] Changed NX's c++ standard default to use -std=gnu++17 instead of -std=c++17.
* [NX] Updated NX vcxproj generation to support changes in NX VSI version 10.4.3+REV29683 (specifically in the way on how CppStandardLanguage field is set).
* [NX] Updated NX vcxproj generation to support parsing the warning disable compiler switches (ie -Wno-XX) to DisableSpecificWarnings field in vcxproj.
* [NX] Updated compiler switch parsing for Visual Studio solution generation to support setting up CppLanguageStandard flag for the latest NX VSI based on our -std= switch usage.
* [NX] Updated project generation to copy the .nrs files as well to allow getting runtime callstack.  To disable copying the .nrs files, set global property 'nx.disable-nrs-copy' to true.
* [NX] Fixed NX release builds linking against the wrong system library (nnSdk.nss vs nnSdkEn.nss), causing crashes on boot due to invalid assembly.
* [IOS] Corrected ObjectiveCPPLibrary build optionset to actually set object c++ build info.
* If a CSharp or managed module listed ```<assemblies>``` fileset in their .build file, it will now check if that assembly has .config file and will do copylocal of that .config as well.
* [Android] Corrected forced include compiler switch parsing during solution generation.
* Fixed MasterConfigWriter not writing out masterconfig fragment include's 'if' attribute when it exists.

# 8.03.00

### Changed

* [IOS] Changed iphone-deployment-target-version from 9.0 to 11.0.  Some c++17 features are no longer feasible for devices with iOS older than 11.
* [OSX] Changed osx-deployment-target-version from 10.9 to 10.14.  Some c++17 features are no longer feasible for machines with osx older than 10.14.
* [NX] Changed NX platform to build against c++17 (-std=c++17) by default.  (ie set 'cc.std.nx.default' property to c++17).
* Configs must now specify config-name, config-system, config-compiler, config-platform, config-processor and config-os in their configuration-definition optionset or you will receive an error.  
* The platform config file found in the config/platform folder for config packages must match the format <config-system>-<config-processor>-<config-compiler>.xml or you will receive an error message.
* Some dead unused configurations were also removed.  OSX 32bit configs, iOS/iPhone 32bit configs, and unix-x64-clang-* configs.  Use unix64-clang-* configs instead which are identical.
* Framework now does not support '-dev-' in config names, please update your build command line to use the non -dev- version of the configs.  For backward compatibility framework will automatically re-map -dev- configs passed in on its commandline to the non -dev- versions so that it will not break people.  This re-direction will be removed in 6 months.  If you are in the frostbite environment and have config packages of your own you can use 'fb update_framework_configs' command to automatically find and update/rename thigns for you.
* Remapped frostbite specific configs to eaconfig ones.  In some cases we need to add the release + final config in the eaconfig side to allow 1-1 mapping.  Framework now will auto-detect a fb config and re-map it to a eaconfig one when you run framework to give people time to transition their build-scripts.
* Standardized how to set c++ version across all platforms.  It will now use the following override/evaluation order:
  1. User provided global property "cc.std.${config-system}" which will override ALL modules for specific platform
  2. User provided their own build options with cc.std.$(config-system) to control only specific modules for specific platform
  3. User provided global property "cc.std" which will override ALL modules for all platforms (except above platform specific setting)
  4. User provided their own build options with cc.std to control specific modules for all platform (except above platform specific setting)
  5. Use setting in global property "cc.std.${config-system}.default" property (initialized in config package) which user can override
  6. Hard code to c++14 (except ps4, ps5, unix, and Visual Studio compiler platforms)
* Changed default Visual Studio version from 2017 to 2019.
* Default working directory for executables now set to output bin dir.

### Added

* [NX] Added support for 'cc.std.nx.default' global property to allow people set default value back to original settings (gnu++14).
* [NX] Added structured XML support to allow setting custom nmeta file or custom nmetaoption optionset override for the default nmeta file generation:
  ```
	<Program name="mymodule">
		<platforms>
			<nx>
				<nmeta32file>
					<includes name=""/>
				</nmeta32file>
				<nmeta64file>
					<includes name=""/>
				</nmeta64file>
				<nmetaoptions>
					<!-- 
					If nmeta32file or nmeta64file is specificed, 
					the provided file will get used instead of the 
					override options provide here! 
					-->
					<option name="" value=""/>
				</nmetaoptions>
			</nx>
		</platforms>
	</Program>
  ```

### Fixes

* Trying to better handle the case where a group is defined in multiple fragments and they have different attribute values. 
Now it will take the first non-null setting and if there are two non-null settings that are different it will fail.
* [NX] Fix NX vcxproj generation not properly updating AdditionalNSODependencies field for dependent module's ```<dlls-external>``` filesets that are triggered through "publicdependencies" (or publicbuilddependencies).
* [NX] Fixed NX solution generation to allow people change ApplicationProgramFormat in generated solution to NSP output (.nsp) instead of RAW (.nspd_root))
* [NX] Removed copylocal of dependent module's .nrr files to final program module output's .nrr folder.  It was causing build eroor message about "Nro file registered with Nrr Hash Lists wasn't found" when targetting NSP output during incremental build.
* [NX] Added support for global property nx.application_program_format (accepts value 'nsp' or 'raw') which will affect creating Visual Studio solution that automatically target NSP vs Raw
* [NX] Added a work around for NX VSI intellisense missing the very first line in AdditionalIncludeDirectories field.
* [Outsourcing Tool/NX] Fixed outsourcing tool build for NX platform.  The NX dll modules stub libs (.nrs files) need to be placed side by side the dlls (.nro files) itself as NX's VSI is hard coded to expect .nrs being located side by side .nro files.
* [OSX host] Fixed nant to no longer execute 'uname' to determine if it is running on OSX host. It how use RuntimeInformation.IsOSPlatform() (available starting with .Net 4.7.1) to determine if we're on OSX host.  (Note that Framework is already building against .Net 4.7.2 since version 8.00.02).
* [IOS/OSX] Fixed the old ObjectiveCppLibrary, etc optionsets got 'enable_objc_arc' option turned on during previous 8.0 release.  Any new code that has been updated to allow building with -fobjc-arc compiler flags should swith to use the following optionsets:
  * ObjCAppExtension
  * ObjCLibrary
  * ObjCProgram
  * ObjCppAppExtension
  * ObjCppLibrary
  * ObjCppProgram
* Re-tweak Framework's copylocal timing and retries mechanism.  Also updated the copy to use named mutex lock to handle multi-process msbuild execution when solution is being built.
* fixed null reference bug that was occuring when trying to run buildinfo
* Adjusting the package root code so that if one of the normal package roots matches the ondemand root framework will treat the first occurance as the ondemand root and not add a duplicate value

# 8.02.01

### Fixes

* Fixed an issue with the shaderfiles where it would fail if an optionset was not added to the file item.
* Fixed issues with shaderfiles related to an error message about there being too many files. 
Build script writers can now set the option psslc.embed to true in their optionset to enable the embed option which makes this error go away.

### Changed

* Updated 'dlls-external' / 'assemblies-external' fileset export support in Initialize.xml to allow user specify optionset 'copylocal-relative-to-basedir' 
  (or any optionset with option 'preserve-basedir' set to 'on') which will instruction Framework to setup copylocal to preserve relative path information 
  when performing copylocal.  For example:
  ```
      <publicdata packagename="test_package">
          <module name="test_module" buildtype="Library">
              <libs-external if="${config-system}==kettle">
                  <includes basedir="${package.kettlesdk.libdir}" name="libSceNpToolkit2_stub_weak.a"/>
              </libs-external>
              <dlls-external if="${config-system}==kettle">
                  <includes basedir="${package.kettlesdk.sdkdir}/target" name="sce_module/libSceNpToolkit2.prx" optionset="copylocal-relative-to-basedir"/>
              </dlls-external>
          </module>
      </publicdata>
  ```
* ps5 build will no longer automatically copy ALL prx in SDK's target\sce_module folder.  Only target\sce_module\libc.prx will be automatically copied for you.  Any other sce_module prx usage should be setup by user's build files to indicate their module need that prx.
* Changed ps4 (kettle configs) build to automatically SDK's target\sce_module\libc.prx to build program's output package as well.
* Removed usage of the VisualStudio\installed folder in Framework and removed that folder.  Any missing components during non-proxy build should just update the MSBuildTools package.

# 8.02.00

### Added

* A new shaderfiles fileset has been added to allow users to add shaderfiles to their module. It currently supports shaders for pc and sony platforms.

### Fixes

* Fixed mono location detection on osx machines to explicitly test for typical install path at /Library/Frameworks/Mono.framework/Versions/Current/Commands

# 8.01.02

### Changed

* Removed the unnecessary warning messages from @{RetrieveConfigInfo()} function.
* If the <config> element in the masterconfig has the attribute include rather than includes it will now fail, since that is the wrong spelling and previously it was just silently doing the wrong thing.
* Added a bit more information to the message about two masterconfig fragments setting different versions. It now lists what the versions are and which fragments are setting the versions.
* Updated custom build tool (optionset setup for custombuildfiles fileset) to support macro %outputbindir% and %outputlibdir%.  (The regular %outputdir% points to intermediate dir during compile phase when custom build tool is executed).

### Fixes

* Prevent null exception when using the 'eaconfig.includegroupinbuildroot' feature.
* Prevent null exception when using the 'nant.usetoplevelversion' feature with a masterconfig that contains metapackages.
* Giving the telemetry directory a unique name to prevent issues with simultaneously running instances of different versions of framework from copying incompatible files to the directory.
* Framework will no longer fail if it can't find packages like android_config even if they are not listed as active config extensions. 
  This was happening because of old code in framework that was depending on all packages listed in the masterconfig that ended in _config even if they were not enabled config extensions, now we only depend on config extensions.
* Fixing showmasterconfig so that it displays the fragment path for packages listed in the meta package masterconfig.

# 8.01.01

### Changed
* Changed the default setup of -std=x switch for c++ compile in Common_Clang_Option() to get executed only if we don't have platform specific code already set this option.
  The intent of this change is to allow the following precedence order: platform global property cc.std.[platform] -> platform config option cc.std.[platform] -> global
  property cc.std -> config option cc.std -> default as set in Framework.
* Unix/Linux configs now use c++17 standard by default.  You can use the cc.std property to override this back to c++14/c++11.

### Added

* Added new IMCPPVisualStudioExtension function WriteExtensionCompilerToolProperties() to allow platform specific setup of compiler tool properties in Visual Studio project file.
* Integrated all configs from nx_config-dev (at CL 5143796) to eaconfig.  The followings are fixed along with this integration:
  * Fixed precompiled header support for building in nx platform (requires an update to Framework's IMCPPVisualStudioExtensio class to allow extension update to compiler properties.
  * Corrected compiler command line setup to not inject -std=c++14 (Requires the above Common_Clang_Option() fixes in Framework).  Visual Studio Integration for NX platform actually will 
    inject -std=gnu++14 (forcibly) for you and you have no choice about that.  This setting is also being listed as a "required option" from official Nintendo documentation as of SDK 9.3.0
    We should avoid creating conflicting setup.  If future SDK allow this to be set differently, we can change the default at that time.
  * Corrected dynamic library module (.nro)'s import stub library setup.  The import stub library is supposed to be in the same output folder as .nro and it should be using .nrs extension.
  * Updated copylocal settings to also copy dependent NRO's corresponding NRR files to application's ".nrr" folder.
  * Updated copylocal to properly copy dll (nro files) to NRODeployPath folder and the rest of the copylocal files to ApplicationDataDir location.
  * Added platform structured XML extension to allow user specify the following fields (in module definition in .build files)
    ```
	<Program name="mymodule">
		<platforms>
			<nx>
				<application-data-dir>%outputbindir%\dataSrc</application-data-dir>
				<nro-deploy-path>%outputbindir%\dataSrc\nro</nro-deploy-path>           <!-- Must contain application-data-dir above -->
				<nrr-file-name>%outputbindir%\dataSrc\.nrr\mymodule.nrr</nrr-file-name> <!-- Must contain .nrr folder under application-data-dir above -->
				<authoring-user-command> ... </authoring-user-command>
			</nx>
		</platforms>
	</Program>
    ```
  * Add support for adding AdditionalNSODependencies.  For any dependent modules that uses NSO modules, they can specified this in that package's Initialize.xml's module declaration as "dlls-external" fileset.
    For program modules that has code that uses the NSO directly, these can be specified in the package's .build file's module definition under "dlls" fileset.
  * Removed usage of -fshort-wchar compiler switch.  This is marked as prohitbited compiler option in official documentation.
  * Added support for a few extra compiler switch to vcxproj translation (options_msbuild.xml file) to avoid duplicate and conflicting compiler option settings.
  * Removed default addition of glslc.nss and nnPcm.nss to default Program's link library list.  To properly use glslc.nso and nnPcm.nso, tbese should be added to the dlls fileset (see above 
    entry on AdditionalNSODependencies) as the build process need to properly copy those nso to [Application].nspd\program0.ncd\code\subsdk[x] location.
  * Updated NxRun's "ControlTarget.exe connect" execution to use --force argument to do a force connection.

### Fixes

* masterconfig ```<conditions>``` inherit localondemand, autobuildclean, autocleantarget and autobuildtarget from parent if not overidden but NOT other metadata
* big increase to directory move timeout to allow aggressive anti-virus more time to scan large packages with many binaries
* run package validation in p4 download before install
* fix grouptype parsing in masterconfig - can now correctly reference full groups in exceptions
* add warning when ps5sdk dependent quietly fails in pipeline to make it clear what is happening
* Fixed symlink creation to not recusrively delete through old symlinks - the api calls we are using don't do this but our code was previously modifying file attributes recursively through the link in preparation for the delete causing perforce to throw errors in noclobber clients
* Outsourcing tool (create-prebuilt-package task):  Fix rare instance where module dependency setup for re-created build file was missing module constrain info.
* Fixing 'ps5-clang-postprocess' task's copying of sce_module directory's filelist didn't get added to the _Files.txt file list manifest.
* fix Ps5Run.cs to call target info properly with the new 0.90 SDK TM.


# Frostbite 2020.1-PR2

### Changed
* Playstation 4 and 5 now use c++17 standard by default.  You can use the cc.std property to override this back to c++14/c++11.

# 8.01.00

### Added

* Updated eaconfig's outsourcing tool ("create-prebuilt-package" task) ability to package up extra build files from original package and insert instructions at end of new build file to do
  include of those extra build files. IMPORTANT NOTE: These extra build files CANNOT contain module definitions like `<Library>`, etc.
* When using VisualStudioProject module type with a pre-made .vdproj file the correct project type guid will now be added in solution files.

### Changed

* Modified the meaning of (-dev-debug-opt) configs to use option iteratorboundschecking = on rather than off (define '_ITERATOR_DEBUG_LEVEL' = 2 rather than 0). This is to make it consistent with the usedebuglibs option as debug Microsoft system libraries usually build with this set and on some platforms this will cause linker warnings about the mismatch between different link sources.
* When searching for local packages within package roots Framework will now:
  * Accept ```<Root>```/```<PackageName>```/```<PackageName>```.build as a valid indicator that ```<Root>```/```<Package>```/ is a package folder for PackageName package marked "flattened" in masterconfig without the presence of a Manifest.xml.
  * Accept ```<Root>```/```<PackageName>```/```<Version>```/```<PackageName>```.build as a valid indicator that ```<Root>```/```<PackageName>```/```<Version>```/ is a package folder for PackageName with the version Version if that is the version the masterconfig specifies.
* When searching for .build or Manifest.xml files in order to find packages Framework will looks for the filename case insensitively regardless of file system case sensitivity.
* The ```<package>``` task must now be executed in the .build file associated to the root of the package that the masterconfig indicates. This happens by default in all but very atypical cases anyway but is now enforced.
* The ```<package>``` task no longer requries the name="" attribute as with the above change it can be derived automatically.
* When checking ```<compatibility>``````<dependent>``` elements in Manifest.xml that specify a minversion or maxversion compatibility the version checked against in the dependent package is the Manifest.xml ```<verisionName>``` falling back to the masterconfig version. Previously masterconfig version was always used which was problematic for "flattened" packages.
* Updated SnowCache solution generation to support solution level metrics during command line msbuild.exe build.
* Updated outsourcing tool to insert a filecopystep task to copy to build destination folder for generated Utility module 
  if original module is a program module that export package.[package_name].[module_name].programs fileset.
* Updated Visual Studio solution generation for Linux project to support setting RemoteDebuggerWorkingDirectory field in vcxproj.user file when property eaconfig.unix.vcxproj-debug-on-wsl is true and the module's buildsteps/run/workingdir information is provided.
* eaconfig's run target is also updated to pass -dir argument to EARunner when running under WSL enabled.
* Small update to copylocal functions to allow copying prebuilt (ie Utility) module's pdb files (from the corresponding dynamic libraries and assemblies export).
  This ability is turned off by default to preserve existing behaviour.  To enable it, set global property eaconfig.prebuilt.skip-copylocal-pdb to false.
* Small update to copylocal functions to allow copying prebuilt (ie Utility) module's .config file for an assembly or program if the .config file is present.
* Small update to copylocal functions to copy buildable dependent DotNet module's .config file if this module's resource fileset has "app.config" listed.
* Bumped default setup of iphone-deployment-target-version property from 8.0 to 9.0 to enable support for newer c++ features like thread local (requested by Foundation team).
  If anyone still need to target iOS 8, you can provide that property in your global property section of masterconfig and set it back to 8.0.  However, you would also need to check with
  Foundation team as their code now target iOS 9 devices and up only.  
* When targeting windows _WIN32_IE define is no longer derived from 'eaconfig.defines.winver' and is left unset by default. To set an explicit version use 'eaconfig.defines.win32_ie_version'.
* Following PackageMap APIs have been marked Obsolete. Deprecation messages detail alternative functions to use:
  * GetMasterRelease
  * GetMasterVersion
  * FindReleaseByNameAndVersion
* Following PackageMap API overloads have been marked Obsolete. Deprecation messages detail the correct overloads to use:
  * TryGetMasterPackage
  * GetMasterGroup
  * TryGetMasterPackage
* Propagating properties using ```<properties>``` under ```<grouptype>``` (an undocumented feature) in the masterconfig is no longer supported as it was never used.
* Minor update to not throw warning about invalid path in LIB environment variable when running ```<taskdef>```.

### Removed

* Removed any reference to PS4's Facebook libraries - these were removed with PS4 SDK 7.0, and are no longer supported.

### Fixed

* Removed an unnecessary warning message about "unexported p4port" for an input that is actually valid.
* Fixed issue where solution folder setup was ignored if the setup was done in a package where all modules in that package are being excluded from solution itself using package.[package_name].[module_name].exclude-from-solution property.
  This typically happens when user have a top level driver package containing only dummy Utility modules that drive the build but don't really want the dummy utility modules being added to generated solution.
* Fix an error with setting ondemand flag under masterconfig fragment has no effect.
* Corrected the PathToWsl(string path) function to not change the entire path to lower case.
* Fixed outsourcing tool's create-prebuilt-package target to test for presences of assemblies and .Net program's .config file and copy the .config file as well.
* Fixed outsourcing tool's packaging of content files that is not under package root should be packaged flattened.
* Fixed a small issue with nant.exe call propagation's pass through of -buildroot argument to make sure path doesn't end with backslash.  Otherwise, when the path is quoted, the last backslash will get treated as an escape character.
* Fixed issue in p4 uri syncing with using non-unicode servers in combination with unicode registry default for perforce. Explicitly passing "-C none" for servers without a charset setting.
* Fixed makefile generation's clean task to be able to delete read only files as well (happens when we have checked in data files that is being copied to output directory).
* Fixed the eaconfig.exception global property so that it correctly turns off exceptions when the global property is set to off and properly handles the /EHr option.

# 8.00.03

### Fixed

* Fixed issue in p4 uri syncing with using non-unicode servers in combination with unicode registry default for perforce. Explicitly passing "-C none" for servers without a charset setting.

# 8.00.02-1

### Removed

* Removed any reference to PS4's Facebook libraries - these were removed with PS4 SDK 7.0, and are no longer supported.

# Frostbite 2020.1-PR2 / 8.00.02

### Added

* Added a way to specify the configset name explicitly in a config's configuration-definition optionset rather than using the
various on/off switches. This will make it easier to extend with custom config types.
* Added a property that contains all of the active config extension names. 'nant.active-config-extensions'
This has been designed so that it can easily be use in a loop if users want to try to loop over all of the config packages.
* Added new String function, StrSelectIf, that selects one of two strings based on whether or not a given expression evaluates to true.
* -showmasterconfig parameter can now accept a list of packages to narrow down the scope of what showmasterconfig returns.  This is primarily for speed since it is executed in several cmd line tools.  
  Example: adding -showmasterconfig:"CapilanoSDK kettlesdk WindowsSDK" would only compute and return the results of the masterconfig for those packages.
* Added XB1 GSDK support.
* Upgrade from NuGet 2 to NuGet 5.
* NuGet packages that require a greater than 2.x client version will no longer error.
* NuGet packages can now fallback to .NET Standard if .NET Framework isn't supported by the package.
* NuGet packages will required re-downloading because of above changes.
* Framework locking optimizations to prevent thread pool stalling when waiting on many downloads. A side effect of this change is implicitly loaded Initialize.xml files from <publicdependencies> and <publicbuilddependencies> are loaded at a later stage. This may expose issues with missing <dependent> tasks when you are using properties from another package (e.g "Failed to evaluate property 'package.MyPackage.dir'." because there is no <dependent name="MyPackage"/>).

### Changed

* Updated Framework's error message when the given "target" name doesn't exist.  It will now provide extra information on the closest target
  name matches suggestion.
* Behaviour of 'PackageGetMasterDir' is slightly modified.  Previously, if package is not installed, the function will throw an exception.
  Now, if package is not installed, it will attempt to install the package first before throwing exception.
* Updated support for ```<option>``` element body to handle ```<do>``` condition blocks and also throw build exception if it encounter XML elements that it doesn't support.
* Updated "eapm install" with -masterconfigfile argument to provide a warning message if the requested package is actually not listed in the masterconfig file and also list
  any possible close matches.
* Updated outsourcing tool (the create-prebuilt-package task) to insert a "dependent" task to external package when there are packages that uses publicdependencies to include external packages.  
  This fix is needed due to Framework's update to it's implementation to publicdependencies.
  
### Removed

* Removed old changelog entries (pre-7.00.00) to shorten the total changelog size

### Fixed

* Fixed a makefile generation error in the way it handle copy-local targets.  Any prerequisites to "copy-local" targets in the makefile should be listed under "OrderOnlyPrerequisites" section.
  Otherwise, it would always trigger a rebuild during incremental build.
* The include attribute in the masterconfig config section was not working when using config extensions but has now been fixed.
* Removed a dependency check from LoadPackageTask - it was unnecessary since the check is done by other code, and it could raise issue with modules not in the build graph.
* Fix error with Framework not properly split up content of "visualstudio.startup.property.import.*" properties into separate line when the property contain multiple files.

# Frostbite 2020.1-PR1

### Changed

* Reworked Visual Studio 2019 Linux makefile project generation's deployment path to match the build output relative path.
  For this to happen properly, user need to setup property '[group].[module].output_rootdir' for their program module.  Otherwise, the default
  deployment path will be '[deployment_root]/$(ProjectName)/$(Platform)/$(Configuration)'.

### Removed

### Deprecated

### Added

* Added a new 'config-vs-full-version' property.  Previously, we only have 'config-vs-version' which only return major version number just as 14.0, 15.0, etc.  Now a new
  property 'config-vs-full-version' is also added to allow people test for full Visual Studio version down to patch version info in case user need to detect features/fixes that
  is only available after specific update.
* Added /Zc:char8_t and /Zc:char8_t- switches when building with msvc toolchain.  This is a new type that is coming in the new c++20 standards.

### Fixed

* Fixed issue with Visual Studio solution generation missing make files for Linux configs when solution has mixed platform containing both Linux and non-Linux platforms.
* Fixed Visual Studio solution generation for .csproj file where resource files and source files aren't properly linked 
  if file path specified in source files and resource files used different case causing mismatch.

# Frostbite 2019-Alpha / 8.00.01-rc2

### Fixed

* Small fix to rare occasion in p4 package download protocol where "p4 changes" return nothing.  The fix will now show you a warning message and proceed to attempt a resync instead of crashing with an exception.
* Fix a directory symlink/junction creation error when people previously created a directory link (say "LinkDir") to target directory (say "SourceDirA"), but that target directory has been deleted and then
  people re-use this directory link path to target a new directory (say "SourceDirB").  Previously, you will get an error.  This scenario is now properly detected.

### Removed

* Removed experimental support for compiling/linking with Link Time optimization (-flto) with Linux build on PC host using WSL.

### Added

* Added the ability to remove dotnet options in structured xml in the same way that they can be removed for c++ modules. This feature was mentioned in the documentation but had never been fully implemented.
    ```
	<CSharpLibrary name="mymodule">
		<config>
			<remove>
				<options>
					-deterministic
				</options>
			</remove>
		</config>
	</CSharpLibrary>
    ```

### Fixed

* fixing an issue in framework where users could not add the builddir as an includedir because all includedir paths are relative to the solution
and since the solution is in the builddir the relative path function was returning an empty string. Now if users put the builddir
as and includedir it will be added as "./"
* Fixing a Nuget bug where .netstandard dlls were not being added to the assemblies fileset and thus not added as assembly references
* Fixed Linux vcxproj generation to properly monitor global property eaconfig.unix.use-vs-make-support to allow user revert back to old format.
* Updated ps4 build to support user using the LTO system libraries when using -flto switch.  Created a new property "platform.sdklibs.lto" 
  (along side platform.sdklibs.regular and platform.sdklibs.debug) which contain either the regular libs or with the *_lto.a version if exist.

# Frostbite 2019-PR7 / 8.00.01-rc1

### Changed

* Removed the need for 'config.targetoverrides' optionset at the start of package build files. It can still be used to exclude targets for backwards compatibility but really it should be removed and instread if you wish to override default targets you can just redeclare the target with the override="true" attribute.
* Framework will throw a warning, or error if warningsaserrors is on, in cases where 
 a module is defined in the initialize.xml public data but not the build script and a module in a different build group exists with the same name.
 This is to catch user mistakes where they define the module in the build script and initialize.xml in different groups. Previous it would fail silently.
 We wanted this to be more general and fail in cases where the module in the initialize.xml is not defined in the build script, but unfortunately this is too common
 as modules in the build script are often wrapped in a condition without the same condition being used in the initialize.xml. So for the moment this case will just be an advise level warning.
* Downgrade NuGet package name case mis-match error to warning message if we have two different NuGets having same internal dependency but specified mis-match case NuGet name.
* Updated Visual Studio solution generation for config that uses Visual Studio compiler (ie PC and XB1), we will setup Visual Studio's "Unity Build" flag to help with
  Visual Studio intellisense / startup performance.  If for any reason you encounter problem, set this global property 'enable-vs-unity-build' to false in your masterconfig
  will revert back to old method.

### Added

* Added a new `validate-visual-studio-components` task that takes a single required argument, `components`, and ensures that the user's Visual Studio installation comes with those components. This task uses the same Visual Studio version/preview checks as the existing get-visual-studio-install-info and set-config-vs-version tasks.

### Fixed

* Fix error with Framework keeps recompiling taskdef assembly thinking there are assembly mis-match between loaded assembly and referenced assembly due to
  case mis-match of file path between loaded assembly and referenced assembly.
* Fix Visual Studio's Linux make style project capibility detection wasn't properly detecting full Visual Studio version info.
* Fix PackageInstaller crashing when user go through the workflow of running the Framework installer exe and then try to download packages
  from the package server website.
* Fix error with initializing web download url setup when the application's .config file doesn't provide an url link override.
* Corrected an internal flag indicating whether a NuGet package is auto created directly from NuGet internal dependency (and not specified in masterconfig).  This error was causing issue
  with user trying to provide a correct case NuGet package name in masterconfig when there are multiple NuGet package listing same dependency with different case.
* Fix makefile generation to properly generate copylocal tasks.  Previously it was implemented in a way that it was performing the same file copy
  multiple times.

# Frostbite 2019-PR6 / 8.00.01-rc1

### Changed

* The <buildable> element in Manifest files defaults to true rather than false. In general, packages can be assumed to be buildable even if they contain no buildable code as they will simply be discarded from the build graph. The buildable flag is only still supported for legacy cases where packages with buildable code have been explicitly marked as non-buildable.
* xge.numjobs property has been replaced by property eaconfig.make-numjobs.  This is because that property actually controls the number of jobs to spawn when running make.  Also XGE is not supported anymore so using xge in the name is misleading.
* Default number of jobs to use on a windows machine for make is changed from default value of `32`.  It is now set to use the environment variable `NUMBER_OF_PROCESSORS*2`, which seems to give a good level of parallelism, vs a hard-coded number 32.  Note this can be overriden by using the eaconfig.make-numjobs property.  For unix or macosx host machines the default is still unchanged at 32.
* All references and support (previously untested and in an unknown state) for XGE builds has been removed.  Only the `incredibuild` targets has been left to be able to build solutions via incredibuild via nant/Framework.
* If packages are defined multiple times in the publicdata section of a package's initialize.xml this will trigger a warning, and if warnings as errors is on this warning will become an error.

### Added

* New warning when buildtype in the build script does not match the build type in the initialize.xml, 
but only checks that the base build types match so they don't need to be exact matches so if the buildscript specifies LibraryCustom but the initialize says Library they will still match.
* Added a cl_wrapper.exe and ml_wrapper.exe to the main purpose of being used by pc-vc makefile build so that the make build will properly generate a .d dependency file like other clang compilers.

### Fixed

* Fixed Framework makefile generation to properly handle pch files using visual studio compiler.
* Fixed Assembler for ps4-clang and pc-vc paths when building via makefiles and nant build.  Was only working via msbuild of visual studio projects.
* Fixed makefile generation for pc-vc not properly expanding the %forceusing-assemblies% token.
* Fixed nant build execution of pre/post build step commands keep leaving new batch files on build output folder every time nant build is executed.
  It is now changed so that if the command batch file execution is successful, the file will be deleted.
* Fixed determinism issue with makefile generation.  The export and unexport statements could get generated in different order each time causing makefiles to change and subsequent builds to take place for no good reason.
* Fixed Visual Studio detection to select preview version if AllowPreview is specified.
* Updated Visual Studio solution generation for Linux to automatically use Visual Studio's Linux make style project instead of generic make style project if you are using correct Visual Studio version 16.1 Preview 3.0 or better.
* Fixed a bug introduced in 8.00.00 that suppressed error when targets were overridden without the appropriate attributes.
* Fixed clang compiler option setup to not add "-x c" or "-x c++" switch if existing optionset already provided a "-x " option.

# Frostbite 2019-PR5 / 8.00.01-rc1

### Changed

* Package name case mismatch between .build file and masterconfig or module name dependency constrains between build files will now throw a 
  build exception and forces you to fix your build file.  There are too many places (including user's custom tasks that is outside Framework's control) 
  uses case sensitive package/module name as 'keys' in their data structure.  So it is safer to enforce consistency when Framework notice error.
* Credential Store storage format has changed due to security concerns about hardcoded encryption key. The old nant_credstore.dat format is obsolete and will be regenerated.
  * Users might have to reenter their passwords when prompted
  * Build farm machines will have to generate a unique credential store file by using the eapm credstore and eapm password commands which now has added user/password parameters to allow non interactive setting of credentials:
    ```
    TnT\Build\Framework\bin\eapm.exe credstore [-credentialfile:<path>] [-user:<username>] [-password:<password>] <p4server|masterconfig>
    TnT\Build\Framework\bin\eapm.exe password <USERNAME WITHOUT DOMAIN> <DOMAIN> <password>
    ```
    On linux, the same command can be used through mono.
* Updated to stop setting the "AssemblyFoldersConfigFile" field in generated vcxproj/csproj if property "package.VisualStudio.reference-assembly-configfile"
  is not set or the configfile it points to doesn't exist.  That setup is only appropriate for building for MSBuildTools and for Visual Studio 2017.

### Added

* Added a DeployFilters property to the BuildLayout class, so .buildlayout files now contain a list of file filters to
  exclude from deploy steps e.g. when pushing an XB1 build, though it's up to whatever is consuming the .buildlayout file
  to respect this list (e.g. Icepick). It uses the module's `layout-extension-filter` property.
* Updated experimental support for using Visual Studio's Linux make project to optimize for debugging using WSL
  (so that the deploy file linux machine can be skipped).  To tty use the experimental feature, set the following two properties 
  on nant.exe command line during solution generation:
  ```
  -G:eaconfig.unix.use-vs-make-support=true
  -G:eaconfig.unix.vcxproj-debug-on-wsl=true
  ```
  and make sure that you have setup a remote host connection in Visual Studio (Tools -> Options -> Cross Platform -> Connection Manager)
* eapm package command will now generate a signature, and this signature can be verified by passing the -verify option when calling eapm package.
  It is important to note that the signature file generated by eapm package is slightly different from the one generated when running nant package.
  The eapm package command does not read build scripts, so any files that previously were excluded are no longer excluded.
  The order of files may also be different. Both of these things can also affect the combined hash.
  The version of signature file is listed at the top of all the signature files, 
  the version generated by nant package is version 2 and the version generated by eapm package is version 3.

### Fixed

* Small fix the viewbuildinfo task where it is not properly showing the use dependent package's info in current selected module column.
* Fixing a bug where Framework wasn't properly re-evaluating the masterconfig globalproperties under "if" condition block.
* Fixed bug where showmasterconfig would not show the correct development root when the user had set the FRAMEWORK_DEVELOPMENT_ROOT environment variable.

# Frostbite 2019-PR3 / 8.00.01-rc1

### Changed

* Installing intellisense for build scripts is now easier than ever. The intellisense schema generator is now its own
  separate executable (bin/NAnt.Intellisense.exe). There is even a shortcut for it in the root of the Framework
  package.
* Added the concept of a gendir for where generated files can be placed that is separate to the builddir. The
  package.gendir.template can be used to adjust the location of the gendir.
* Updated Framework's Visual Studio detection to also test for ATL install and give people warning message if ATL is not
  part of the installed component.

### Added

* Added support to allow deprecation message to be disabled.  All Framework deprecation messages are enabled by default. To disable it in
  masterconfig, add the following section in masterconfig:
    ```
    <deprecations>
      <deprecation id='[number]' enabled='false'/>
    </deprecations>
    ```
  Or specify it on nant.exe command line with
    ```
    -dd:[number]
    ```

### Fixed

* Fixing error with evaluation of masterconfig package version exception when the property being used is not defined.
* Fixed a concurrency issue during xb1 solution generation when package contains multiple test applications causing identical app manifest
  logo image file attemping to be created at the same location concurrently by the multiple test modules.
* Fixed an issue with p4 client setup during p4 sync URI protocal when that command is executed on Windows system with 65001 (UTF-8) code page.
  The Process.StandardInput was sending the UTF-9 byte-order-mark to input stream causing p4 to fail.

# Frostbite 2019-PR3 / 8.00.00-pr7

### Added

* Protoype code caching system included, but not intended for use. System is disabled by default and should not change
  existing behviour in any way when disabled. Solution Generation extensions implemented in partial classes in
  VSSnowCacheProject.cs, new SnowCache target added to targets to communicate with caching Server
* Added the ability to switch to the new package server for early testing.
* Added a quick check to ensure that the masterconfig is valid xml when running the eapm where command to print a better error if it is not.
* Added separate package name and version fields to the build info file, rather than just using a single field that has the name and version separated by a dash.

### Changed

* Updated Framework to use https when communicating with the package server.

### Fixed

* Fixed issue with incremental build where copied local's dll file has original timestamp causing Visual Studio's fast up to date check
  to think there are output files older than input files.  This fix will now update the timestamp of the copied local's dll if hard link
  is not used.  If hard link is used, the copied files in output location will not be used to test for up to date check.
* Fixed issue where autoversion would incorrectly pass -logappend to older Framework versions.
* Escaping quotes in build info file to that is valid yaml and can be parsed by third party tools.

# Frostbite 2019-PR3 / 8.00.00-pr6

### Changed

* VS2019 Preview 2.0 support changes. Preliminary support for PS4 if you have the beta VSI/Debugger installers.

# Frostbite 2019-PR2 / 8.00.00-pr5

### Changed

* Moved get-vs2017-install-info task from VisualStudio package into Framework EA.Tasks.  This avoids circular
  dependency between Framework<->VisualStudio package, and allows Framework to get information about Visual Studio
  installs without needing additional dependencies on other Packages.

### Fixed

* Fixed error with MakeStyle project execution in nant "build" target not preserving system environment variables.
* Fixed the situation where under Windows Subsystem for Linux, Framework wasn't able to retrieve current machine IP
  address to determine optimal P4 server.		
* Fixed system property 'nant.drive' to point to /mnt/\<drive> if Framework is actually running under Windows Subsystem
  for Linux.
* Fixing a bug with the autoversion feature where it would fail to parse the framework version with Version.Parse.
  This is because Version.Parse does not handle letters and special characters which are allowed in framework package
  versions. Instead we will check the Framework assembly version rather than the package version.

# Frostbite 2019-PR2 / 8.00.00-pr4

### Changed

* Moved get-vs2017-install-info task from VisualStudio package into Framework EA.Tasks, and renamed it to
  get-visual-studio-install-info. This avoids circular dependency between Framework and VisualStudio package, and
  allows Framework to get information about Visual Studio installs without needing additional dependencies on
  other Packages.
* Re-ordered config loading slightly. All of the init.xml files are now loaded before all of the initialize.xml files.
  Init.xml is a fairly new addition to the list of files that are loaded during config loading so we are not to
  concerned about moving. There was also not a strong reason for its previous ordering and now this ordering moves
  the Initialize.xml file to be loaded after several important package properties like builddir are setup by
  framework.

### Removed

* Removing references to Sandcastle from Framework's solution generation code and removing the task
  EaconfigSandCastleTask. We have never used this for our own sandcastle projects and are not aware of any users that
  currently generate sandcastle projects with framework.

### Added

* Optional case insensitivity for properties; including the ability to track case mismatches.
* Added support for 'preserve_symlink' attribute to the unzip task to allow special handling of symlink files in the
  zip file.

### Fixed

* Fixed unzip task's support of symlink files to handle situation of symlink to another symlink file. Also fixed the
  symlink creation handling such that if operating system failed to create a symlink file (due to permission issue),
  we will now just make a copy instead of creating symlink.		
* Fixed package.\<packagename>.bindir property setup to take into account there is an outputname-map-options optionset
  override for configbindir.


# Frostbite 2019-PR1 / 8.00.00-pr3

### Changed

* Removed all eaconfig.\<group>.groupname, eaconfig.\<group>.outputfolder, eaconfig.\<group>.sourcedir, eaconfig.\<group>.usepackagelibs properties from being set in eaconfig.  The outputfolder property was already set in Framework, and was marked ReadOnly.  They are now set by default in Framework but are not ReadOnly so that buildscripts can override them.

### Fixed

* Fixed Determinism issue with &quot;generator.includebuildscripts&quot; being enabled.  Buildscripts automatically added to the buildscriptfiles FileSet when this feature is enabled could be pulled out of the FileSet object in a different order causing project files to get re-written randomly.  This would cause the to need to reload project files in Visual Studio fairly often for no reason.


# Frostbite 2019-PR1 / 8.00.00-pr2

### Changed

* Changed the default precompiled header (pch) filename. The default used to be [ModuleName].pch located at
  %intermediatedir%. Now it is changed to %intermediatedir%/%filereldir%/[pch_source_file].pch. Purpose of this change
  is to match up with the default output location between the obj output and the pch output so that if an old config
  package (or a custom user config package) is used that hasn't been updated to support creating the pch output, we will
  still get the compiler generated [name].d dependency file from the obj compile and this [name].d dependency file is
  needed by Framework to track build dependency if people are building using pure command line (ie using Framework
  "build" target).

### Added

* Updated portable solution generation to allow using property 'eaconfig.portable-solution.framework-files-outputdir'
  to specify a destination location where Framework specific's msbuild tasks should be copied to.  If this property is
  set to Framework's package root, no copying will be made.		
* Updated custombuildfiles' optionset to accept %packagename% and %modulename% tokens.
* \<nant> task properties are now initializable from C#, not just XML.
* Added retries to make file copy local commands
* Added multiprocess safety to taskdef assembly generation

### Fixed

* Fixed buildlayout file generation to include [output].config file for the C# modules that listed app.config file in
  resourcefiles filelist to generate the [output].config file.
* Fixed issue with Visual Studio warning with the vcxproj when people generate a solution for mixed platforms.
* Fixed issue with Visual Studio no longer support config conditional setting for ProjectReferences.  In order to
  support project that is specific for a single platform for a multi-platform Visual Studio solution, the project
  reference is setup regardless of config conditional.  The build setting is now controlled by build settings in
  .sln file generation and the LinkLibraryDependencies setting will be setup based on config info.
* Fix for Property elements under <nant> tasks requiring a value attribute.
* BuildLayout entry point missing error is now a warning.
* Changed default for eaconfig.codecoverage property from 'off' to 'false'.


# Frostbite 2019-PR1 / 8.00.00-pr1

Initial changelog version.

### Changed

* The \<group>.buildtype property for packages or modules will no longer default to 'Program' and must be set
  explicitly.
* Assemblies in project base directories are that end in ...Tasks.dll, ...Tests.dll or ...config.dll are no longer
  automatically scanned for nant tasks and functions to add to the project. To define custom elements use the
  \<taskdef> task.
* Command line options -enablecache and -cachedebug have been removed. Optimization work has been done that means
  configuration caching is no longer critical for performance.
* Command line option -enabledmetrics has been removed. This option has been deprecated for a long time and has no
  effect.
* Command line option -find has been removed. Please use a working directory that contains a .build file or use the
  -buildpackage or -buildfile options to have nant locate build files correctly.
* Command line option -timingMetrics has been removed in favour of other profiling techniques.
* Command line option -useoverrides has been removed. This was a specialized option to handle certain migrations and
  is no longer required.
* The \<script> task no longer accepts VB.NET code. C# is now always assumed.
* The \<taskdef> and \<script> tasks no longer automatically reference all loaded assemblies but a "well known" subset,
  i.e. NAnt.Core.dll, EA.Tasks.dll and NAnt.Tasks.dll. System.dll, System.Core.dll, System.Runtime.dll and System.Xml.dll
  are also referenced. This restricted list avoids these tasks having implicit dependencies that may or may not be
  loaded depending on the execution order of code. This may mean additional references need to be added to \<taskdef>.
  The old reference loading can still be enabled with global property `framework.script-compiler.use-legacy-references`
  set to true but this is recommended only as a short term workaround.
* The 'debugbuild' attribute of the \<taskdef> task has changed meaning. Previously it controlled whether debug symbols
  were generated for custom tasks. Now, debug symbols are always generated and this flag controls whether optimized
  code is generated. This makes `taskdef` code compile in the same way as nant's core code which means the debugging
  experience and runtime callstack resolution is consistent when working with custom tasks. Additionally, <script> tasks
  now also generate debug symbols by default.
* Code compiled as part of a \<script> or \<taskdef> task will now emit warnings to Framework's output at most logging
  levels. The level of warnings is tied to Framework's warning level. Taskdefs that emit warnings will not cache their
  dependencies leading them to be rebuild each time Framework is run.
* Code compiled with \<script> or \<taskdef> tasks will now have executing nant.exe's folder added to assembly search
  path.
* Moved most of the C# tasks and user tasks that were in eaconfig into framework. For C# tasks this will break
  compatibility because there will be duplicates. For user tasks it won't because the user task version in eaconfig
  will still take precedence over the C# task in framework.
* publicdata declaration of all exe program C# and Managed buildtype modules will now have assemblies fileset export
  automatically added.
* You can now specify 'outputname' remapping of a module in Initialize.xml's publicdata module export.  It is
  recommended to set this up in Initialize.xml instead of .build file so that module output path can be properly
  evaluated under all circumstances.
* Updated the GetModuleOutputDirEx() function to take into account if package's Initialize.xml has setup outputdir
  override for the module.

### Removed

* Removed nant.errorsuppression property handling. This allowed user to suppress certain coded errors which was unsafe
  and not recommended. Recoverable errors have been converted to warnings and non-recoverable errors fail as you'd
  expect.
* Removed EASharpLibrary module task.
* Removed EaconfigBuildTask and SortBuildModules tasks. We believe these are internal tasks that are no longer being
  used and so we are removing them.
* Legacy config package loading is no longer supported (using the property config-platform-mode=legacy). This is an
  old hidden feature of framework that we don't think anyone is still using. We are removing it because we are
  redesigning the config package loading and don't want to support this old case.

### Deprecated

* Declaring packages using without modules using unstructured syntax (which causes a single module to be implicitly
  created) is now considered deprecated.
* \<metric> task is now deprecated and has no effect.
* Defining `on-initializexml-load` to target to be run before initialize.xml files are loaded is now considered
  deprecated and will be removed in a future Framework version.
* Modifying a Project object's TargetStyle from C# is considered deprecated. The most common use case from this was
  for custom calls to DependentTask while forcing Use style. Any code that does this can use the TaskUtil.Dependent
  call from the EA.EAConfig namespace.
* Use of BuildMessage class in C# scripts is deprecated. Equivalent APIs now exist on the Log class.
* Use of Log.Write... functions in C# scripts is now deprecated. Use specific logging channels instead
  (e.g Log.Status.Write...).
* Use of PackageMap.Instance.Releases and Package.Instance.Packages in C# is now deprecated. Equivalent APIs now
  exist on the PackageMap class.
* Using Project class as an IDisposable object in C# script is consider deprecated. Projects no longer need to be
  disposed.

### Added

* A new \<warnings> section has been added to the masterconfig that can be used to control individual warning settings.
  This section can be repeated in masterconfig fragments to override these settings or add additional:
  ```xml
  <warnings>
	<warning id="1234" enabled="true" as-error="false"/> <!-- forces warning to be enabled at any warning level but forces it not to be thrown as error at any warning-as-error level -->
  </warnings>
  ```
  This section can be repeated inside <package> elements in \<masterversions> to set warning setting for individual
  packages. Package warning settings will override global settings.
* Added \<deprecate> task. Similar to \<warn> task but allows deprecation messages to be thrown.
* Added command line option `-deprecatelevel:<level> which can be used to control the level of deprecation messages
  that are shown.
* Added command line option `-logappend` which will not overwrite existing log file if file logging is used but instead
  append to it.
* Added command line option `-warnaserrorlevel:<level> which can be used to control the level at which warning are
  thrown as errors. Note for the time being this is overridden by legacy nant.warningsaserrors property if set for
  backwards compatibility.
* Added command line option `-wd:<warningcode> which allows specific warnings to be disabled even if they wouldn't
  normally be disabled at the selected warning level. Note for the time being this is overridden by legacy
  nant.warningsuppression property if set for backwards compatibility.
* Added command line option `-we:<warningcode> which allows specific warnings to be enabled even if they wouldn't
  normally be enabled at the selected warning level. Note for the time being this is overridden by legacy
  nant.warningsuppression property if set for backwards compatibility.
* Added command line option `-wn:<warningcode> which allows specific warnings to be demoted to warning even if they
  would normally be thrown as error. Note for the time being this is overridden by legacy nant.warningsuppression and
  nant.warningsaserrors properties if set for backwards compatibility.
* Added command line option `-wx:<warningcode> which allows specific warnings to be enabled and thrown as error even
  if they wouldn't normally be enabled or thrown as error. Note for the time being this is overridden by legacy
  nant.warningsuppression property if set for backwards compatibility.
* Updated Framework to properly support tracking of SDK system include directories. The followings were added/changed
  for this feature:
  * Added support for `%system-includedirs%` command line template token
  * Added support for cc.template.system-includedir property to store command line switch template and uses token
    `%system-includedir%` as place holder.
  * Updated vcxproj generation to setup IncludePath variable if cc.system-includedirs has been setup.
* Experimental Support for using WSL in linux makefile builds, to allow using LTO when building for linux on windows.

### Fixed

* Fixed issue where command line flag options could be set using a unknown option that was prefixed with a valid
  option.
* Fixed bug where compiler errors were being stripped of new line characters when running native nant builds.
* Fixed the \<csc> task to use 'mcs' compiler on osx/linux machine. The current version of mono that Framework
  requires only ship with 'mcs'. The older 'dmcs' and 'gmcs' are now just script that print out deprecated message
  and forward to 'mcs'.
* Fixed intermediate temp folder creation to not unnecessary create the "v[vsver]t" sub-folder especially for non-pc
  platforms.
* Fixed MasterConfigWriter not writing out conditions information for globaldefine elements.
* Fixed auto generated pch compile option response filename during make-gen to not unnecessary duplicate buildtype name
  in the filename causing unnecessary long file name which could cause error.
* Fixed issue with ContextCarryingExcetion's callstack retrieval crash with INTERNAL ERROR message in some rare cases.


# 7.19.00

### Changed

*   Redesigned Framework's config package loading system so now Framework handles all of the loading of config packages rather than parts of the process being controlled by eaconfig. The new system also allows for the use of the new config extension feature that we are rolling out in Framework 8. This feature is turned off by default in Framework 7 but can be enabled using the global property framework-new-config-system=true. Keep in mind that compatible versions of the config packages must be used if you choose to opt into this feature. For more information about config extensions please see: https://frostbite.ea.com/display/PLAN/Configuration+Extensions
*   Framework's build file and other nant related buildscripts now are no longer used to build framework, to build framework you now simply can run msbuild directly on the Framework.sln file included with the Framework package or simply open it in Visual Studio 2017 and build it from within the IDE. Note that only Visual Studio 2017 is supported.

### Removed

*   Removing a special case where users would not need to have a GenerateOptions task for gcc configs. This is a backported change from Framework 8 in order to reduce the number of differences between the two versions. We have already dropped support for all of our remaining gcc configs and even in those cases they had a GenerateOptions task, so this should have very little impact and in cases where it does it is easy to fix.
*   The config-platform-mode=legacy feature has been removed to reduce the number of backward compatibility cases we need to support as we transition to a new config loading system.

### Fixed

*   Fixed Echo task when writing to a file instead of stdout. It would not convert the line-endings used in the build script to be the line-endings of the platform its executing on. This causes problems when echoing scripts etc. out to files and then trying to run them.
*   Fixed an "Object reference not set to an instance of an object" error when using GetMappedModuleOutputPath() function in EA.Tasks\Build\ModulePath.cs.
*   Fixing erronous warning message about Initialize.xml public data mismatch for managed assembly modules having import lib export.

# 7.18.02

### Added

*   Added "eapm package" command to replace the package nant target. The problem with the package target is that it would load the package's build scripts which would often fail for packages that are intended only for a specific platform or environment. It was too complicated, unreliable and slow for something that was essentially just supposed to create a zip file of a folder, and we also wanted to move this responsibility out of framework which has become too bloated.

    We have removed the concept of signature files from the packaging step as these had been around for a long time and we haven't found any value in them over this time.

    Since "eapm package" no longer reads any build scripts, files that were excluded from a package using a fileset in the package's build script will no longer be excluded. In most cases we find users ensure they have a clean copy of a package before packaging so this step isn't needed but if you do need to exclude files or folders we have provided a way to do that. A file called ".packageignore" can be added to the root of the package and can list files and directories to exclude, the syntax is similar to a .p4ignore or .gitignore file but slightly different as it is based on the syntax used by Framework fileset wildcards.

	```
    # an example .packageignore file

    # excluding all files with a particular extension
    **.tmp

    # excluding all files in a directory
    temp/**

    # including a specific file in a directory that has been excluded
    !temp/file.txt
	```

*   Added a new eapm command, "eapm installall", which takes a masterconfig as an argument and ondemand installs everything in the masterconfig, including all conditional versions of packages. This is to allow users to install the packages they need first so they can run their builds with ondemand turned off. This is an experimental new command and may change significantly in future versions, feel free to try out the command but don't start relying on it just yet.

### Changed

*   The EA.SharpZipLib code that was in the NAnt.Tasks dll is now in NAnt.Core and NAnt.Core now has a dependency on the SharpZipLib NuGet library instead of NAnt.Tasks.

# 7.18.01

### Fixed

*   Fixed error introduced in 7.18.00 release where package setting up a "publicbuilddependencies" to it's own package is causing build graph creation error.
*   Updated to use SharpZipLib NuGet version 1.0.0\. The old version 0.86.0 is no longer compatible with the latest Microsoft.Build versions being used!

# 7.18.00

### Changed

*   Removed Framework's internal include dependency generator. We now let the compiler generate the include dependency info.
*   The task runner task <task> can now be used to call regular C# tasks, although user tasks defined with <createtask> will take precedence.
*   When setting a structured C# module to unittest="true" when using Visual Studio 2017 a reference to Visual Studio's bundled Microsoft.VisualStudio.QualityTools.UnitTestFramework.dll is no longer automatically added. Instead nuget packages MSTest.TestFramework and MSTest.TestAdapter are added as auto, copylocal dependencies. This is in line with Visual Studio 2017's default behaviour when creating a new unit test C# project. If not using structured syntax this dependency must be added manually. The added dependency requires that versions for MSTest.TestFramework and MSTest.TestAdapter be added to the masterconfig. If using ondemand the following package entries can be added:

    ```	<package name="MSTest.TestFramework" version="1.3.2" uri="nuget-https://www.nuget.org/api/v2/"/>
    	<package name="MSTest.TestAdapter" version="1.3.2" uri="nuget-https://www.nuget.org/api/v2/"/>```

*   The step that loads the config file has been changed slightly so that it checks eaconfig for the config file before looking in a platform config package. This is to facilitate moving the kettle and capilano configs to eaconfig.
*   Custom game team config files in an extra-config-dir listed in the masterconfig can use the optionset format used by platform config packages even if that platform is part of eaconfig. So game teams will not need to modify their custom configs once kettle and capilano configs are in eaconfig. A side benefit of this change is that the optionset format can now be used in config files located within eaconfig to. This may be helpful to users with divergences who are not familiar with the differences between the two formats.
*   Converted some eaconfig user tasks to C# tasks in framework. This won't break compatibility with eaconfig because the user tasks still exist in eaconfig and will take precendence over the new C# versions in framework.

### Fixed

*   Fixed plain nant build keep re-building pch file for clang based configs. It was incorrectly testing for presence of .obj file while during pch generation, the obj file is not created for clang compiler.
*   Updated an exception message to properly let user know which package is having duplicate module names across different build groups.
*   We fixed the System.Web.Razor.dll direct reference and packaging of the dll with Framework. This is now referenced via a Nuget Package, so we don't need to distribute it alongside our source-code anymore.
*   Fixed implementation of "publicdependencies" element in Initialize.xml to not erronously trigger error when listing modules in same package.
*   Fixed bulkbuild setup's auto filtering of the *.mm and *.m files to accidentally removed those files completely from build if the module setup function got run the second time.
*   Fixed issuses with yaml output. Module dependency list was not including package name so different packages with modules with the same name, could appear twice meaning there would be 2 identical keys in the yaml. Script key had values with escape characters, so surrounded the string in quotes to ignore those.
*   Improvements to Intellisense schema generation and installation to try to address user feedback about it being unreliable. Includes message that user must have admin rights to copy the schema to the visual studio folder, removed several internal tasks from the schema, removed the manifest and masterconfig schemas from the installation step.
*   Changed a long filepath exception in framework to say what filepath was too long. Specifically we hit this when trying to package the console sdk packages.
*   Fixing bug where there is no localondemandroot and it still asks the user to set localondemand to false for dev packages

### Added

*   Added a checked in Visual Studio Solution and projects for building nant. In the future we will stop building Framework using Framework. These new projects use the new C# csproj format, and will allow us to use the full potential of Nuget and the C# language.
*   Added a task called ConsoleExecRunner to Framework which contains the common parts of the tasks for deploying and running on kettle and capilano.
*   Fixed issues with casing of boolean values returned by newly added version build script functions.

### Removed

*   Removed the _32.exe variants of nant, rmdir, mkdir and copy. These are no longer needed since we build nant in AnyCPU mode, as well as we primarily only use x64 development environments at EA now.
*   Removed the p4api assemblies/dlls that were distributed with Framework. They were not referenced anywhere.
*   Removed support for the package.perforceintegration property which would add source control related data to the solution file. We decided to remove it because it was very old and we could not determine what it was for or who would be using it. We also had no documentation or tests covering this feature. If a customer is still relying on this feature we would like to know and work with them to figure out the best way to support their specific use case going forward.
*   Removing code related to the eaconfig.externalbuild property. The property was just a hack to easily turn off ltcg, debugsymbols, generatemapfile and bulkbuild. It was probably used for some very specific use case but was not documentated. Each of these options can be turned off individually or for convenience a custom config can be created which has all of these settings turned off.
*   The LoadPlatformConfig task no longer takes a list of include values. We stopped using this feature internally a couple years ago but kept it in case it was potentially used by game config packages. If this is used by game config packages the fix is quite easy, just move the includes to after the LoadPlatformConfig task and check for the existance of the property nant.platform-config-package-name to determine whether or not to include the file.
*   Removed winrt and winprt code paths from solution generation code.

# 7.17.01

### Fixed

*   Made a quick fix dealing with file logger crash under Framework autoversion.
*   Fixed bug with excluded modules where assembly dependency was missing if dotnet module ended up in solution a but dependent dotnet module ended up in solution b. Now dotnet->dotnet dependencies get turned in to assemblydependency over multi-solution environments.
*   Performance improvement update to retrieve compiler defines from clang based on options.
*   Performance improvement update to improve circular dependency detection
*   A few performance optimizations.

# 7.17.00

### Potentially Breaking Change

*   All SubSystem code has been removed from Framework. This could potentially break those using internal C# Framework functions that now take fewer arguments. Otherwise this should not effect other users because support for the SPU subsystem was dropped a long time ago.
*   We have begun removing windows phone related code from Framework. Any build scripts using the <xbox-live-services> structured xml element will fail since it no longer exists.
*   We have removed the Framework 2 assembly name mapping from CompilerBase. This could break people if they are still referencing Framework dlls that have not existed for a long time.
*   There were two duplicate versions of StringUtil in framework, one in the EA.Eaconfig namespace and one in the NAnt.Core.Util namespace. The one in the EA.Eaconfig namespace has been removed.

### Removed

*   P4ToolPackageServer project has been removed from Framework. If you need to continue using it we have put in place a workaround. Set the global property ondemand.packageserver=custom and then ondemand.plugin to the path to the P4ToolPackageServer Dll.
*   GenerateStructuredDocs Task has been deleted. This was a precursor to the newer generated docs that are in the framework documentation.
*   PathFunctions.PathVerifyValid has been removed. This will only impact code if they are relying on this obscure internal framework function.
*   Removing the eaconfig-legacy-initializevariables task, which was an internal task that was only used by the helptoolkit documentation target which we have removed.
*   Removing the nantschematoxml task. This is an internal task that was written in the early stages of the intellisense and documentation project but this task has not been used since.

### Changed

*   We have begun using C# 6 language features in Framework. This means that at a minimum the Framework binaries must be built with VS2015 and .Net 4.6, although we are building with VS2017 and .Net 4.6.1.

### Added

*   Added experimental support to use Visual Studio's native Linux make project support. To use this feature, you will need to do the followings:
    *   Set global property eaconfig.unix.use-vs-make-support=true
    *   Provide global property eaconfig.unix.deploymentroot (must be setup as relative path to home directory)
    *   Set global property eaconfig.unix.vcxproj-deploy-during-build=true. It is necessary if you wish to be able to debug from Visual Studio
    *   Under Visual Studio, you need to setup a connection to remote Linux machine. Make sure you have only one remote machine setup and not more. Visual Studio will make connection to remote Linux machine before build can happen.
*   Fixed makefile pch support to properly test for pch file's actual header dependency as well.
*   Framework will now throw a warning when SDK packages are set to localondemand=true in the masterconfig or when dev branches are set to localondemand=false. SDKs are large and immutable so localondemand should be set to false so they are only synced once, whereas dev versions should always be set to localondemand=false to ensure that local changes are not clobbered when switching between streams that are set to sync different changelists of the dev branch.
*   Added additional string version comparison functions to framework. StrCompareVersions is a bit confusing because it returns a number and you must understand how the number relates to the comparison of the two versions. This often lead to mistakes so I have added new functions that just return a boolean value. StrVersionGreater, StrVersionLess, StrVersionLessOrEqual, StrVersionGreaterOrEqual have been added in order to make comparing versions less confusing.

### Fixed

*   The ondemand metadata file will now indicate the culture of the timestamp to handle the case where the culture is different between the time of writting and the time of reading the metadata file.
*   The ondemand package clean command (eapm prune) will warn but continue if it fails to load a package metadata file.

# 7.16.00

### Behavior Change

*   If there are multiple <buildoptions> blocks in a structured xml module definition they will be appended together just like other structured xml blocks. Previously this block was ignoring all but the last <buildoptions> block defined in the module rather than appending them together. This is a behavior change but is also a bug fix because it is not working the way users are expecting. It is possible a package could be broken if they have multiple buildoptions blocks that were previously being ignore but suddenly start taking effect, but we believe it is very unlikely users would keep buildoptions block around that are not having any affect on the build.

### Changed

*   Automatic runtime dependencies from modules in the test / example group now default to auto dependenencies rather than buildonly dependencies.
*   Framework not correctly represents build-only dependencies between C# modules in it's .csproj output.
*   Modified the output message for warnings as errors to more clearly indicate they are warnings and how to suppress them if needed.
*   Added retries to the copy step preformed when deploying content files. The goal is to prevent failures due to temporarily locked files.

### Added

*   Updated to support new template cc.template.pch.commandline for platforms that needs a new special template for pch generation. Also updated command line setup to use this new template if file item has special "create-pch" and pch-file" option setup.
*   Updated to support converting the token %pchfile% in command line template.
*   Updated makefile generation to monitor file item with precompiled header creation setup and make sure that this creation is executed first before compiling other files.

### Fixed

*   Added support for NuGet packages with .net specific install and init scripts. (for example Microsoft.CodeDom.Providers.DotNetCompilerPlatform)
*   Fixed an issue running NuGet install scripts, where the format of the project properties passed into the script were not what it was expecting. We noticed this issue specifically with the NuGet package Microsoft.CodeDom.Providers.DotNetCompilerPlatform.
*   Fixed issue where Framework's real package download error message got overridden by error message from parallel threads that didn't actually attempt the package download.

# 7.15.00

### Breaking Changes

*   The <sysinfo/> task is now deprecated, this task can still be used but no longer preforms and function. Originally this task would setup some properties but those properties are now setup automatically so you don't need to call this task. This change is only breaking for scripts that were using the prefix argument of the sysinfo task since that will no longer work, but we don't know of a valid use case for this argument so we feel confident that we can remove support for it without much impact.
*   Previous versions of Framework would allow you continue with just a warning when p4uri server connection failed and the connection error met certain criteria to facilitate offline work. This behaviour is still true for packages that don't specify a specific CL number so that Framework can run without network connection if you potentially have the correct version of a package. Packages that that specify a specific CL will always errors out in any circumstance when Framework cannot guarantee they are up-to-date.
*   Removed FASTBuild support

### Added

*   Added a build script function for getting the value of an environment variable, @{GetEnvironmentVariable('VariableName')}. This function is case insensitive and can handle special characters in environment variable names, which were issues in the past when environment variables were stored as properties.

### Changed

*   Changed the messages about masterconfig files overriding versions to be much more concise and ensure they are not printed when the log level is set to minimal.

### Fixed

*   Updated FrameworkCopyLocal to preserve all source file's timestamps after the copy and also updated the copy requirement test to check for timestamp not equal or filesize not equal instead of previously using source file timestamp greater than destination file timestamp. The previous method can potentailly cause new SDK dlls not being copied as expected.
*   Fixed the zip and unzip tasks to preserve the unix file permissions info.
*   Fixed a long standing bug where Framework would generate an entry in the solution file for every configuration for every platform. When generating multiplatform solutions for Visual Studio 2017 this would trigger a warning dialogue about missing configurations and attempt to populate the project files withi invalid configurations.
*   Fixed a false positive error message regarding module has dependency via publicbuilddependency but module wasn't found.

# 7.14.00

### Breaking Changes

*   <taskbuilddef> task has been removed. If users encounter any remaining uses of this task just rename them to <taskdef>. It has been 4 years since we started warning people this would be removed, users have been adequately warned.

### Added

*   Updated the install-visualstudio-intellisense target and by extension "fb gen_nant_intellisense" to install build script intellisense for VS2017.
*   Added a new argument to nant called "-configs:" which is equivalent to "-D:package.configs=" except shorter and it does not require using the global property syntax. The goal of this is mainly to make the command line arguments easier to explain to new users, so we can explain the most common arguments without needing to explain all of the details of what global properties are.
*   External Visual Studio Projects can now be defined in Structured XML with the VisualStudioProject task
*   Added a Framework 101 section for new users to our documentation. We also went through all of our documentation and significantly improved and reorganized it.

### Fixed

*   Disable all unnecessary warning messages when Framework is attempting to retrieve clang compiler's preprocessor defines.
*   Fix package name case sensitivity issue where user build file provided package name with wrong letter casing in dependency specification. Framework will now choose the package name specified in masterconfig file to be the one with correct case and create properties based on that name. A warning message will also be provided to inform user to correct their build file.

# 7.13.02

### Fixed

*   Fixed an issue where FASTBuild Visual Studio projects would not perform Xbox One deployment step leading to deploy errors about incomplete layout directory.

# 7.13.01

### Changed

*   Updated FASTBuild file generation to print file updates at minimal log level when an actual update was required.

### Fixed

*   Fixed bug with output-mapping that was introduced in Version 7.13.00.
*   Updated the Framework taskdeftask to rebuild taskdef dlls if the path to the Framework assemblies is different than the ones listed in the dependency file. Previously Framework was only comparing timestamps and users could get into a situation where they upgraded Framework but since the Framework assemblies still had an older timestamp then their taskdef dll it would not trigger a rebuild.
*   No longer using compiled regexes in Visual Studio generation due to a low-level crash bug in C# system libraries when doing a large number in parallel (becomes an AccessViolationException in managed code).
*   Fixed a bug that caused framework to crash when using the -enablecache argument. Internally in framework a dictionary was trying to use different cases when adding and looking up a file.
*   Fixed an issue with forwarding arguments in -autoversion mode where arguments containing spaces and ending in \ were not correctly escaped.

# 7.13.00

### Breaking Changes

*   The 'clanguage-strict' option is now respected by all compiler option generation code. In rare cases this may cause build errors if .cpp extensions were being defined with C optionsets.

### Changes

*   New option 'cpplanguage-strict' will force files to build as C++ when clanguage = off even if they have .c extension.

### Fixed

*   Fixed error with visualstudio.startup.property.import.${config-system} property not properly being evaluated under single solution multiple platform scenario.
*   Fixed vcxproj file's copylocal setup to use normalized destination path information to avoid possibility of duplicate entries being added.
*   Fixed DotNet module's comassemblies and webreferences setup. It was broken staring in Framework 7.11.00 optimization changes.
*   Fixed solution generation error when a package build script use old style format where build script contain no build module list but has modules in different build groups. It was broken in Framework 7.12.00 release.
*   Fixed a backward compatibility use case where a module is doing an explicit module dependency on another package but that other package did not provide modules information in Initialize.xml. Previously, Framework will give you warning and then switch to using the entire package. This behaviour was broken in Framework 7.12.00 release.
*   Fixed error with "generate-bff" task when optional "generate-type" parameter was not set.
*   Fixed issue when using -autoversion option that prevented nant from allow interactive console operations.
*   Fixed a typo in @{GetModuleOutputDir('lib', ...)} function implemention where property name 'configlindir' was used instead of 'configlibdir'.
*   Fixed error with @{GetModuleOutputDir('bin', ... )} function returning a string package.configbindir instead of actually evaluating the property package.configbindir.

### Added

*   Updated solution generation to support generating FASTBuild makestyle projects directly.
*   Updated MakeStyle project support to allow specifing "MakeOutput" property setup.
*   Added global property eaconfig.shortchar for setting -fshort-wchar on clang/gcc compilers.

# 7.12.02

### Fixed

*   Fixed Backward Compatibility issue with DetectCircularBuildDependencies() function. The signature was changed in 7.12.00 but was not noticed until XCodeProjectizer was being packaged.
*   Fixed an incorrect warning message when the requested P4 server is the same as the detected optimal sync server at detected location.

# 7.12.01

### Fixed

*   Fixed so CopyLocalInfo can store multiple DestinationModules since that is actually the case when doing reversed copy local. (this information is needed by custom tasks in frostbite). There was a bug with which reversed copy local destination was read first, so we need to consider all destinations when operating on this data.

# 7.12.00

### Added

*   Added Support in Framework Perforce API to allow creation of 'readonly' vs. 'writeable' clients/workspaces. Uri syncs via perforce now use readonly clients instead of writeable clients. This has a positive effect on performance on perforce servers. If at any point there is a problem using readonly clients you can use nant.p4protocol.allowreadonlyclient=false in your masterconfig to revert back to creating writable workspaces. Note perforce clients/servers have a minimum version of 2015.2 to use this feature.
*   Added additional validation on copy-local destinations to ensure hardlinking is consistent across solutions.
*   Added 'nant.copylocal-default-use-hardlink' property to set the default behaviour of hardlinking. Default is false to avoid changing behaviour by default. Frostbite has set this to true.
*   Added support for "package.<package>.<module>.exclude-from-solution" property which removes module from solution (and automatically removes all dependencies on it)

### Changed

*   Additional optimizations to framework. Special thanks to Henrik Karlsson.
*   Minor Spelling Mistakes in text output.

### Fixed

*   Minor fixes for FASTBuild environment settings.
*   Ensure that the correct version of the Microsoft.Build nuget assemblies are deployed with framework/nant. There was a bug before where the wrong assemblies would get bundled in the framework release causing Nuget related problems with Visual Studio 2017 solution generation.
*   Updated portable solution support to properly fix up option settings parsed from command line switches that has sdk paths to use MSBuild variables.

# 7.11.00

### Added

*   Added support for manually adding build group constraints to Projects (used to be able to add for example test buildgroup to non top package packages)
*   Added support for custom dependency tasks. If custom dependency task is set on a module this means that the graph dependency calculations will be deferred from wide parallel pass. Task will then execute which makes it possible to actually query all existing module dependencies. This task can then add build dependencies to module. After task is done a second pass will run which calculate graph dependency information for the modules with custom dependency tasks. This feature can be used for modules that should collect modules depending on information collected from the dependency graph. The property to set a custom dependency task is "package.<Package>.<Module>.customdependencytask"

### Changed

*   Changed module loading code to instantiate all modules in dependencies. This may affect load order and property evaluations in some cases.
*   Changed Garbage Collection method to gcServer from gcWorkstation. Improves generation time of large projects significantly. Side effect is that more memory is consumed by nant.exe, however this was deemed acceptable due to the performance gain, and the amount of memory on developers desktops today.
*   Disabled etw trace app config setting to save some time at process shutdown.
*   A handful of targeted optimizations which significantly have reduced solution generation times. Special thanks to Henrik Karlsson.

### Fixed

*   Fixed issue where Framework was not printing full callstack during script error when running at minimal logging level.
*   Minor Spelling Mistakes in text output.

# 7.10.04

### Fixed

*   Fix portable solution's relative path re-mapping to preserve directory separator ending if the input ends with directory separator.
*   Fix a trycatch task bug where it is setting trycatch.error property to an empty string when the originating exception came from deep inside InnerException while the outer exceptions didn't have any messages.
*   Fix an issue where EmbeddedResource and Resource filesets that were marked copy "copy-always" or "copy-if-newer" didn't get copied to output directory. This behaviour appears to be broken since Framework release 7.07.02\. There was a few typos in data\FrameworkMsbuild.props and data\FrameworkMsbuild.tasks.

### Changed

*   Framework now create the property "Dll" on project startup and defaulted to false. If you still have any build script that previously do @{PropertyExists('Dll')}, you will need to convert them to ${Dll??false} which is the propery syntax to test for a property's true/false condition and provide default if property does not exist.
*   Changed the 'echo' task's loglevel=Minimal setting to no longer print out a [warning] tag in the output!
*   Fixed up Visual Studio solution generation to also setup 'TargetPlatformVersion' MSBuild property for non-pc solution. This change is necessary so that for non-pc solution, we can still have correct WindowsSDK version folder being used when necessary.
*   Updated MSBuild property override for 'TargetFrameworkSDKToolsDirectory' to use 64-bit versions when on 64-bit host.
*   Updated Visual Studio solution generation to not do any MSBuild property override for WindowsSDK path re-direction under portable solution generation.
*   Changed the "buildinfo" target to produce valid YAML files, while keeping them nearly as readable as before. The benefit is that one can now easily write quick one-off scripts to dig into build information.
*   Framework's build file no longer override eaconfig's 'package' target. Framework's 'package' target behaviour is reverted to standard behaviour which is to create a package zip. The old Framework installer zip file is now moved to 'package-installer' target. This 'package-installer' target will now create the standard 'Framework-[version].zip' and a new 'FrameworkInstaller-[version].zip'.
*   TRACE is now added for C# by default for all configs. It can be disabled by using eaconfig.trace=off.
*   Updated custombuildfiles support to allow people setup outputs info with empty commands.

### Removed

*   The following events on the Project C# class API now do nothing and attempting to use them will just yield a deprecation warning:

    ```BuildStarted
    BuildFinished   
    TargetStarted    
    TargetFinished   
    TaskStarted      
    TaskFinished     
    UserTaskStarted  
    UserTaskFinished  
    			```

# 7.10.03

### Fixed

*   Fixed an issue with FASTBuild where modules could generate colliding target names in .bff files.

# 7.10.02

### Fixed

*   Fixed an issue with FASTBuild where Durango deployment targets in MSBuild weren't being fully completed leading to missing files in the layout directory.

# 7.10.01

### Fixed

*   Fixed an issue where multiple config .vcxproj files would reference assemblies incorrectly if an assembly with the same name but different path was used in different configs. This situation occurs frequently when declaring a dependency on a prebuilt managed C++ module.

# 7.10.00

### Added

*   Added a check to see if you have adjusted the name of a project file via templating, then double check to see if it should be considered as a duplicately named project when generating.
*   Added new "silent-err" parameter to <exec> task. When set to true will suppress stderr output from exec progam appearing in log.
*   Added a new 'outputdir' attribute to Utility module definition task to allow people provide specific copylocal output location. By default, if it is not provided, it will revert to old behaviour and will be set to IntermediateDir.
*   Added feature to override base partial buildtype in a module's SXML, otherwise framework prints out a warning

### Fixed

*   Fixed GenerateDebugInformation regex in msbuild_options.xml for Visual Studio 2017\. In Visual Studio 2017 the linker flag /Debug will enable fastlink by default on debug builds, and full pdbs in release builds. This is not consistent with Visual Studio 2015\. Now when fastlink is disabled in eaconfig, even debug builds will be forced to have full pdb files.
*   Fixed Bug in Project Name Generation, Duplication type was not reset when iterating though the different duplication groups.
*   Fixed GenerateDebugInformation regex in msbuild_options.xml it was wrongly emitting "Debug" instead of "true".
*   Fixed MasterConfigUpdater's Load function to handle masterconfig fragment include's "if" attribute introduced in release 7.09.00.
*   Fixed MasterConfig's MergeFragment functionality to ignore the "if" attribute when this class is being used without a project space.
*   Fixed a copylocal bug under Utility module where under multi-config vcxproj, only one of the config will execute the copylocal task.
*   Fixed a dll copylocal bug where a build dependency to a Utility module with link attribute set to false and copylocal set to true, the dlls fileset of that Utility module did not get copied.
*   Fixed an issue with solution generations on some freshly setup machines where Nuget environment would attempy invalid numeric comparison on empty "$(VisualStudioVersion)" property.

# 7.09.00

### Added

*   Logger to output some information when the Framework LogLevel is set to 'Minimal'. Previously Minimal would only output warnings. Along with this change a few of the bracketed output statements like the [echo] output have been modified to be more descriptive. In the [echo] case it now outputs details of what package the echo statement is coming from. This is noted in case there are some stdoutput tools that look for [echo] output specifically and need to be adjusted.
*   Updated masterconfig fragments' include element to accept "if" attribute to allow conditionally loading fragment based on property passed in from command line.

### Changed

*   Modified PackageInstaller.exe so that if you manually install a package through package server web page, Framework will not silently add an environment variable FRAMEWORK_ONDEMAND_ROOT for you. Framework will ask you permission first before creating that environment variable for you.
*   Fixed a bug in DynamicLibraryStaticLinkage build optionset setup where it is incorrectly adding defines EA_DLL when property "Dll" is provided and set to false. Before the fix, it was only testing property exists, but it should be testing the property's value and default to false.

### Removed

*   FilterSpuProjects task was removed, as a small cleanup, this was only for ps3 which we do not support anymore.

### Fixed

*   Fixed issue where PythonProgram modules would be included as buildable in generated solution files which would cause MSBuild to fail when trying to build the solution. Python projects have no build step and should not be active in any configuration.
*   Fixed an eapm bug where it was not returning non-zero error code when "eapm post" command failed to post to package server.
*   Fixed an error introduced in release 7.08.02 where non-pc platform native module that has build dependency to a DotNet module attempting to do copy local for that DotNet Module (which was actually marked as skipped for build).
*   Updated Framework doc build to use latest Sandcastle Help File Builder v2017.5.15.0.

# 7.08.02

### Changed

*   Modiified copy local transtive scan to search though native modules to find C# modules. This is required for correct copy local behaviour in native cpp -> managed cpp -> c# dependency chains.

# 7.08.01

### Changed

*   The nant module now has a pre-process step to clear the readonly flag of all of the files in the framework bin directory when generating the framework solution. So people that rebuild framework no longer have to go through an extra step to ensure the binaries are writable.

### Fixed

*   Fixed an issue where C# modules with .obj contentfiles being marked for copy local were tripping up Managed C++ module build when that C# module is being depended on by the Managed C++ module.
*   Fixed an error in 7.08.00 release where the managed assembly module EA.DependencyGenerator.dll was compiled with Visual Studio 2015 runtime dependency. This is now fixed and that dll is now pure MSIL assembly without Visual Studio 2015 runtime dependency.

# 7.08.00

### Changed

*   Framework will now warn if ANY attribute on a package in a masterconfig fragment is different than its original definition unless allowoverride='true' is specified on the fragment definition.

### Added

*   Project generation templates for specific modules can now be controlled at global level by setting '<buildgroup>.<packagename>.<modulename>.gen.name.template'.

*   Added ability to generate a Global Section during solution file generation. This can be done by extending the IVisualStudioSolutionExtension class in a Framework task.

*   Added a FASTBuild clean target and wired up Visual Studio FASTBuild projects to call this target when running clean. This also allows users to run a rebuild through Visual Studio, Visual Studio handles this by running a clean followed by a build.

# 7.07.02

### Fixed

*   Fixed a bug with p4 charset implementation introduced in 7.07.01 release which could potentially cause connection failure or crash.
*   Updated all nant public '@{Directory*()}' functions to catch all C# exceoption and re-throw with nant's BuildException in order to provide better error call stack for better debugging messages.
*   Fixed warning messages about duplicate resource files link for C# modules when openning Visual Studio solution.
*   Fixed an error with p4 sync timeout exception being overwritten by other system exception and user loosing the important information.

# 7.07.01

### Added

*   When using p4uri clients are now created with "auto" charset by default to automatically detect correct perforce char set for the server.
*   Masterconfig syntax for p4uri's accepts new query parameter "charset" e.g "p4://server:1111/package/name/dev?cl=1234&charset=utf8" - note that this charset is only used if the server is not one that Framework automatically remaps using the proxy map (see below). This query parameter cannot be used in masterconfigs with older versions of Framework.
*   P4 proxy map now supports "charset" attribute on <server> entries. This charset will be used when generating P4 client against that server or it's proxies.

    Example:

    ```<server address="go-perforce.rws.ad.ea.com:4487" charset="utf8">
      <proxy location="EAC">eac-p4proxy.eac.ad.ea.com:4487</proxy>
      <proxy location="EARS">go-perforce.rws.ad.ea.com:4487</proxy>
    </server>```

    Older version of Framework will ignore this parameter.

### Fixed

*   Fixed an issue where package download unzip phase would use machine OEM code page however code page under certain language settings is invalid for zips. Now uses utf8 universally.
*   Fixed bug where remote proxymap failing to load from proxy would try to use uninitialize proxy map to get central server
*   Updated ondemand package error messages to make it easier to classify errors.
*   Fixed a bug that could allow default warnings to be enabled even after being explicitly disabled by user code
*   Remove Reference to EnvDTE from Nuget Code in Framework to avoid issue with EnvDTE not being installed on user machines.

# 7.07.00

### Added

*   Added a feature to allow game teams to provide their own custom configs and overrides without the need for branching eaconfig and other config packages. Support for a new 'gameconfig' element is added in masterconfig fragment syntax. Only one masterconfig fragment can have this element. This feature is mainly designed for use in Frostbite's workflow where most game teams provide their own game specific masterconfig fragment override in their own licensee package folder. To use this feature:
    *   Update your game team's masterconfig_fragment.xml and add <gameconfig> block with 'extra-config-dir' attribute to point to a location where the custom configs are going to be located like so: `<gameconfig extra-config-dir='scripts\config' />`
    *   Then in that directory make a copy of the config file from either eaconfig or the X_config packages and rename it to your custom config name and begin your modification.
    *   If you need to make deviation from default eaconfig's optionset setup, you can add the file config-overrides\overrides.xml and place this in the directory specified in the 'extra-config-dir' attribute. During the config setup, eaconfig will load up that file after setting up the default optionset definition but before the final step in creating the buildtypes from those optionsets.
*   Natvis files can now be added to a module or module's public data using a new <natvisfiles> fileset. Natvis files in a module's public data will be transitively propagated to dependent packages with a link step. They will also be added to the linker command line with the /NATVIS argument during fastbuild and native nant builds.
*   Added the property <group>.<module>.buildscript.basedir to control what directory to use as the base when including build scripts in a project.
*   Dot Net modules can now add custom visual studio global properties using Framework's Visual Studio Extensions feature, in the same way that this was previously only possible for native modules.
*   All CSharp modules now add TargetFrameworkSDKToolsDirectory in the visual studio project file to tell Visual Studio to use the dot net files in the WindowsSDK package when using the non-proxy package incase the dot net tools are not installed.
*   Visual Studio Extenions, Framework's feature for adding custom elements to the visual studio project file, can now be added in structured xml. Using the <extensions value=' <taskname>'> inside the <visualstudio> block of a module in structured xml is equivalent to setting the property (group).(module).visualstudio-extension.</taskname>

### Fixed

*   Updated LoadPlatformConfig task to have a proper error message when the loaded config name isn't in the expected format.
*   Fixed issue with fileset patterns that had relative paths and relative base directories.
*   Fixed an issue where .csproj generation would not error if it failed to generated interop assembly for a COM reference.

# 7.06.05

### Changed

*   Allow setting the line endings for the Framework temporary Perforce client specs created that are used for syncing packages via a P4 URI via a property named 'nant.p4protocol.client.lineendoption'. Sample usage: In your masterconfig.xml: <globalproperties> nant.p4protocol.client.lineendoption=unix </globalproperties>

# 7.06.04

### Changed

*   If the local p4 proxy map location is overridden using 'ondemandp4proxymapfile' property then this proxy map will be used instead of the remote proxy map in all cases.

# 7.06.03

### Added

*   Fixed an issue where modules with no sources file would still try to produce a static library Visual Studio project. This bug was introduced in Framework 7.06.00.

# 7.06.02

### Added

*   Added support to monitor presence of the property 'backend.VisualStudio.cc.additional-options' and setup the vcxproj to add extra "AdditionalOptions" defined by that property (expected to be setup by config packages).
*   Added a simple way to bundle up all of the information that is helpful to the CM team when diagnosing support issues. Simply run Framework with the new -dumpdebuginfo argument and a debuginfo.zip file will be created in the build root. We recommend users generate this and send it when emailing our support mailing list to help speed up the process.
*   Added "programs" fileset support for <publicdata>'s <module> export. Note that unlike the "libs", "dlls", etc fileset, if an empty "programs" fileset is provided, a default will not be created for you. You will need to use module's buildtype attribute specification to setup default fileset export.
*   Added an option to the record task to write the output to a file instead of a property. (example <record file='myfile.txt'>) It is recommended to use this instead of using the echo task to write to a file, especially when the output is very long, since this will write to the file as it runs rather than all at once.
*   Added support for new "use-as-default" attribute to masterconfig file's localondemandroot element. By setting this attribute to true, all packages listed will be treated as localondemand download (unless the package has a specific localondemand setting override). The default value is false if not specified.
*   Added a way to specify custom project type guids for dot net modules. Users can now specify exactly which guids they want by using the <projecttypeguids> element in the <visualstudio> block of their structured xml module definition.

### Fixed

*   eapm will no longer crash if it finds a legacy credstore file or environment variable with an encrypted password that is invalid.
*   Fixed a bug where unzipping symlinks in zip files would throw exception trying to incorrectly modify symlink attributes.
*   Fixed LocalOnDemandRoot support to cache ondemand_metadata as well so that eapm.exe prune function can clean packages in that folder.
*   Fixed "eapm.exe prune" function being broken since Framework release 7.03.00.
*   Fixed setting relative paths for Android Java files so that any class that has its package name starting with 'com' has its relative path for 'TargetPath' and 'Link' properly force set to 'src/com/..remainderOfRelativePath...' instead of using the relative path directly. Without this change, Java classes can't be found at runtime when running on Android devices.
*   Fixed if a Manifest.xml file exists for a given package and sets the given package's version, always use the version defined in Manifest.xml even if the package's containing folder name is different (but still warn the user about the containing folder name and Manifest.xml declared package version mismatch).

# 7.06.01

### Fixed

*   The Directory Move function in Framework will now retry if it fails the first time in case there are processes locking the directory. This has been noticed specifically when ondemand package downloads are moved from the temp directory to the final directory.
*   Fixed previous issues with cross server incremental sync. In certain situations when syncing the same package from a different server but with identical files the head revision would be fetched rather than simply leaving the package alone.
*   eapm 'install' command would previously only check if a package was installed. It will now also update it to the correct CL if a p4uri is used.

# 7.06.00

### Added

*   Added alpha support for FASTBuild file generation targets.
*   Added a quiet attribute to each of the p4 tasks. (for example <p4sync files="//packages/capilano_config/..." quiet="true"/>) Some commands like p4sync print the full path of each file being synced, but some users reported that it was faster if no output was printed when syncing a lot of files.
*   Added @{PathGetRandomFileName()} function support to get return of C#'s Path.GetRandomFileName().

### Changed

*   Common_VC_Warning_Options is now moved from Framework's GenerateOptions.cs file into eaconfig's vc-options-common.cs file to help consolidate vc compiler settings for xb1 and pc together.
*   Added error checks to detect circular 'publicdependencies' (or 'publicbuilddependencies') issue when loading a package's Initialize.xml file.
*   Warnings about the perforce server in the URI not matching one in the proxy map has been given its own warning code, 2035, so it can be suppressed separately from more serious package server errors. The warning about the URI using an alternate name for the server was also given its own warning code, 2036.
*   Changed the P4 URI download code to ensure that the incremental cross server sync code path is only taken if the perforce server that the package is being incrementally synced from is different from the perforce server it was previously synced from. This way we garentee that the package is resynced if the URI path to the package is changed. The goal of this change is to improve stability because the incremental cross server sync code is complicated and we are seeing issues that are difficult to debug so we want to reduce the number of cases that take this path.

### Fixed

*   Fix Framework's make style vcxproj generation for unix build to include compiler's preprocess define in the intellisense section and provided an intellisense forced include to undefine the default _WIN32 added by the VC intellisense.
*   Fix a bug with manifest.xml file's compatibility constraints setting didn't get checked when the package is being run as top level package.
*   Fix an error where some additional linker options (such as -fxxx) got mapped to "Additional Dependencies" section in generated PS4 vcxproj.
*   Fix NuGet Protocol download dependency check issue where if the NuGet package has dependency list for multiple .NetFramework group, we would get a crash.
*   Fix an occasional NuGet Protocol download issue (triggered by multi-process scenario) where we see NuGet's auto-dependency was missing dependent version info causing error message saying you must specify a version for the dependent NuGet package in your masterconfig.xml file.
*   Removed usage of all Path.GetTempFileName() call. This function can throw IO Error when the number of temp files reaches 65535 or no unique available file name is available which causes unnecessary build failure.

# 7.05.02

### Fixed

*   Fixed bug that caused functions in masterconfig conditions not to be recognized when using the -showmasterconfig command.
*   Fixed a bug where NuGet packages were missing content files in their generated initialize.xml's contentfiles fileset.
*   Fixed bug where expressions with a '!=' operator would sometimes fail. In certain cases the first argument would be an empty string and Framework's expression evaluator would mistake the '!' symbols as a unary not operator.

# 7.05.01

### Fixed

*   Fixed an issue where local ondemand packages would not be found in development root.
*   Fixed an issue where Framework would not allow publicbuilddependencies declaration between modules in the same package.

# 7.05.00

### Changed

*   Framework is now built with .Net 4.6.1 and Visual Studio 2015. We needed to make this change because it is required by some of the NuGet packages Framework needs for generating Visual Studio 2017 solutions. The main impact of this is that you will need the .Net 4.6.1 runtime installed in order to run Framework. Frostbite users should have this installed already or can install it by running FrostbiteSetup.exe. Some non-frostbite users may not have this installed and may need to install it separately. Note: If you are running Framework with mono you will need at least mono 4.4.0 which is the first version to support .Net 4.6.1. The most common indication that you are using an old version of mono is an error about the method "Array.Empty" being unrecognized.
*   Some Framework 6 APIs which were removed in Framework 7 have been reimplemented using backward compatible redirects. These APIs are not 100% identical but have been added to handle specific known use cases for these APIs.

### Added

*   Added support for a new 'silent' attribute for the <exec> task to allow user suppress all standard output from the execution of the external program.
*   Added precompiled headers (PCH) support for Visual Studio 2015 Android builds, which can be leveraged to speed up game build times.

### Fixed

*   Framework wasn't warning in cases where a module was depending on a module in another package but the dependency was on the wrong group type. For example depending on Package/Module when Package/tool/Module should have been used.
*   Fixed bug that was causing localondemand packages to try to perform incremental updates on packages in the regular ondemand root instead of the package in the local ondemand root.
*   Fix transitive propagation of loading Utility module dependents with transitivelink attribute.
*   Fix potentially circular dependency issue if Utility module has setup "interface" dependency to a module in another package.

# 7.04.02

### Fixed

*   Fixed the Framework installer to include scripts\Initialize.xml and the 'external' folder which is needed if people want to rebuild Framework.
*   Fixed failure during the Framework installer where it failed to register the .eap extension. The Framework installer will now request to run as administrator automatically.
*   -showmasterconfig will no longer fail when invalid custom task assemblies are found in the working directory. Any file ending in tasks.dll, tests.dll or config.dll could cause a failure and since -showmasterconfig is used for a lot of important features in frostbite we would like to ensure it is more robust.
*   Fixed issue where NuGet packages were throwing an error when trying to implicitly add dependencies not declared in the masterconfig.

### Changed

*   Modified how nant build decides which modules to compile. Rather than building all modules in build graph it will now just build (transitive) build dependencies of the top level modules. This is the same logic Visual Studio builds use to determine what to projectize so nant and Visual Studio build should now be more equivalent in terms of what they build. This also avoids certain edge case bugs where nant build will fail to build 'use' dependencies because certain dependency information has not been loaded.
*   Unrecognized elements within the publicdata <module> block will now cause a failure. A small side effect of this is that <do> and <echo> blocks inside a <module> block will now be executed as soon as they are parsed rather than being ignored.
*   Framework will only search for packages with the "switched-from-version" attribute in the developement root directory.
*   Skipping publicdata validation when dependencies occur between modules in the same package. Package owners often omit internal modules from the publicdata and we don't want to throw warnings when internal modules are omitted.
*   Framework will no longer fail if the publicdata block in the initialize.xml file is empty or if all of the modules have conditions that evaluate to false. We felt this was causing too many failure in legitimate use cases.

# 7.04.01

### Fixed

*   Fixed an issue from 7.04.00 release which would cause assemblies in Nuget packages with non-zero build numbers to use incorrect version constraints leading to collisions.
*   Fixed an issue from 7.04.00 release where explicitly disabled warnings in structured <warningsuppression> would still be enabled if eaconfig force promoted default-off MSVC to warning level 4.

# 7.04.00

### Added

*   Updated support for masterconfig file to specify a 'extra-config-dir' attribute under the <config> block to allow game team easily extend and create custom config name based on eaconfig or any X_config packages' standard configs. Note that this feature is meant to be used by final end user (ie game team or private projects that will NOT be redistributed). If you are library developer who needs to distribute your project to another user (like game team for example), you should not use this feature.

    To use this feature, update your game team's masterconfig's <config> block and add this 'extra-config-dir' attribute to point to a location where the custom configs are going to be located like so:

    ```<config ... extra-config-dir='${package.ExampleGame.dir}\scripts\config' ... />
                ```

    Then in that directory make a copy of the config file from either eaconfig or the X_config packages and rename it to your custom config name and begin your modification.

    If you need to make deviation from default eaconfig's optionset setup, you can add the file config-overrides\overrides.xml and place this in the directory specified in the 'extra-config-dir' attribute. During the config setup, eaconfig will load up that file after setting up the default optionset definition but before the final step in creating the buildtypes from those optionsets.

*   Added 2 more Visual Studio Extensions. _WriteExtensionLinkerTool_ and _WriteExtensionLinkerToolProperties_. To be able to extend/modify the _Linker Tool_ during solution generation.

### Fixed

*   Fixed NuGet protocol install Initialize.xml setup error where it accidentally allowed assemblies from multiple .Net profiles to be used simutaneously.

# 7.03.02

### Changed

*   NAnt's perforce layer will now prefer network interfaces with a DNS suffix when trying to determine best proxy location. This should prevent it for incorrectly identifiying user locations when user has special network interfaces which happen to map into an incorrect location's subnet.

### Removed

*   The intellisense schema install task no longer support vs2010.

# 7.03.01

### Fixed

*   Fixed an issue where the readonly property 'package.available.configs' failed to include the list of configs in newly downloaded config package.

# 7.03.00

### Added

*   Added _publicbuilddependencies_ element to the module publicdata for initialize.xml. This allows declaring build dependencies that should be publically used by that module's dependents which will propagate the link dependency one level down.

*   Added a solution for users that are syncing the same version of a package using the URI but at two different changelists in different streams. A user can now specify a <localondemandroot> which they can locate within a stream as a place to sync those stream specific versions. To indicate that an ondemand package should be downloaded to the localondemandroot a user must add the attribute "localondemand=true" to the package element in the masterconfig.

    For example: <package name='mypackage' version='dev' uri='p4://server:1234/packages/mypackage/dev?cl=12345' localondemand='true'/> This will sync the package to the localondemandroot so that if mypackage is synced to the ondemand root at a different changelist it is not overwritten.
*   Added the "defines" attribute to the structured xml resourcefiles fileset to allow users to provide additional defines to the MSVC resource compiler. The "defines" attribute is the equivalent of setting the property "(group).(module).res.defines" and will require the next eaconfig release (greater than 5.02.00).

### Changed

*   Framework will now only search for packages in the development root if those packages have the switched-from-version flag on the package element in the masterconfig.

# 7.02.00

### Fixed

*   Fixing a couple bugs in the install-visualstudio-intellisense target. The log object does not appear to be defined and was throwing a null exception every time it tried to print a warning. The task also didn't nicely catch when trying to form a relative path to a directory that wasn't deep enough.

*   Check the internet connection state on Windows first before emitting a warning that we can't download the Perforce proxy map.

*   Fixed vcproj generation to not write out Masm tool for all modules. This tool will now only get added if modules indeed have asmsourcefiles fileset being defined.

*   Fixing issues where appxmanifest files are using the package name for the identity.name and application.id fields but the package name contains invalid characters like underscores and dashes. Framework will now remove these characters automatically.

*   Fixed a bug for missing (runtime) package use dependency occurs on package which has no declared modules in the runtime group but does declare modules in other groups.

*   Fixed masterconfig updater to handle fragments with fragments.

*   Fix Framework's makefile generation for unix target not properly deleting old library before creating a new one.

### Changed

*   Exposed a few functions / property from ModulePublicData class from private to protected to allow inheritance of this class by user tasks.

# 7.01.00

### Added

*   Added support for a 'skip-if-up-to-date' attribute for the <postbuild-step> element in module's <buildsteps> definition. If this attribute is set to true, the post build step execution will be skipped if the module was up-to-date and didn't need building. This attribute is only supported for nant build and in makefile generations. For build under Visual Studio solution, this attribute has no effect as Visual Studio always skip post build step by default if a project is up-to-date.

    Also corrected make-gen to properly handle buildstep commands containing shell script if-blocks.

### Fixed

*   Fixed copy task error when 'hardlinkifpossible' attriburte is used on linux/osx host.

*   Fixed issue with Framework treating empty folder as valid package folder. This fix now checks for presence of Manifest.xml file to be considered a valid package folder.

*   Fixing an issue where framework was case sensitively comparing the package name to the folder name to determine if the folder was flattened. This was causing commands to fail when the package name in the command line was the wrong case.

*   Fixed bug in generated makefiles where make was interpreting the trailing slash in environment variable paths as a line continuation character. Trailing slashes are now stripped from all environment variable exports before writting them to the makefiles.

*   Fix multi-threaded package server download unzipping issue that got introduced in 7.00.00 release. The downloaded package will now be unziped in a process specific temp folder first. After successful unzip, it will get moved to final destination if there are no exceptions.

### Changed

*   Updated build file/initialize.xml module definition inconsistency warning messages (note that this warning message is only shown when '-warnlevel' nant command line is set to Advice. Also updated missing copylocal dependency warning message to provide better info for reason of the warning.

# 7.00.00

### Breaking Changes

*   eapm list command has been removed.

*   eapm describe command has been removed.

*   Framework 1 compatibility has been removed.

*   Package root minlevel and maxlevel support has been removed. Framework now assumes packages either are flattened or have a version directory.

*   Framework package root is no longer added.

    This will have the following effects:

    *   Invoking Framework in way that requires on demand download or using 'eapm install' will throw an error unless an explicit on demand root is provided, the masterconfig used specifies an <ondemandroot> or the environment variable FRAMEWORK_ONDEMAND_ROOT is set (must be absolute path when not using masterconfig).
    *   Packages along side the Framework package/installation will no longer be automatically found.
*   Package directories are case insensitive. Package name casing is determined by their references in scripts (ie: masterconfig files, build scripts). This allows, for example, the EASTL package to live in a directory cased "eastl".

*   Several functions relating to package and release queries that have been maintained for prior backwards compatibility scenarios have been removed. Please use the PackageMap.Find* methods for querying locations of packages.

*   Related, with the package system changes, Framework in general no longer scans all package roots for the location of all system packages at startup. Instead, package roots are only queried on demand. The one case where a full scan for particular releases is done is with the PackageMap.FindByName method. This method should be considered slow and should not be used.

*   Package order returned by -showmasterconfig is no longer deterministic. This may break tools that diff generated masterconfig text naively (ie: with the _diff_ tool).

### Changed

*   Optimizations to -showmasterconfig reduce execution time by 33% in Frostbite.

*   Modified -showmasterconfig to suppress loading config packages.

*   Changed eapm.exe install command to suppress loading of config packages or warn against packageroots not found if -masterconfig option is used.

*   Greatly simplified how Framwework finds it's own release directory. Should prevent errors about Framework being incorrectly installed unless it was actually incorrectly installed.

### Fixed

*   Fix interface dependencies on non-runtime group modules. Previously an assumption was made that these dependencies were in the runtime group. This would then fail to pass constraints to be added as a dependent module.

    Fixed an issue in .csproj generation when using nant.postbuildcopylocal which would sometimes cause Visual Studio up-to-date check to think project was always out of date and trigger slower MSBuild dependency checking.
