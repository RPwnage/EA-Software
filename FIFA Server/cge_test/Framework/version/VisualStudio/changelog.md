This is the changelog for the VisualStudio package.


# <version>

## <label>

### Changed

### Removed

### Deprecated

### Added

### Fixed

* Fixed issue where we'd stomp the build.env.PATH property rather than append to it. [FB-120070](https://jira.frostbite.ea.com/browse/FB-120070)

### Other


## EWVAN-2019-PR5-CU1

### Fixed

* Removed any references to the "installed" folder (intended for Visual Studio 2017) for build using Visual Studio 2019.
* Fixed setup for property package.VisualStudio.vstestconsole.exe to point to correct location when using with MSBuildTools for Visual Studio 2019.

# 100.00.00

### Changed

Since the VisualStudio proxy package is no longer tied to a specific release we have changed the version naming schema
accordingly.

* The property package.VisualStudio.MinimumVersion and package.VisualStudio.MaximumVersion are now what controls which
  versions can be used. These properties should be set in the masterconfig and configured by those on teams interested
  in enforcing versions.
* Moved get-vs2017-install-info task from VisualStudio package into Framework EA.Tasks, and renamed it to
  get-visual-studio-install-info. This avoids circular dependency between Framework<->VisualStudio package, and allows
  Framework to get information about Visual Studio installs without needing additional dependencies on other Packages.
  This change requires Framework 8.0, and is not supported with Framework 7.0 
* Updated msbuild.exe path property for VS2019 Preview. This will likely change in future versions of vs2019. But for
  now this allows VS2019 preview to be used. Refactored and removed several legacy properties. Some legacy properties
  which we have not yet removed have been configured so that they can be disabled with the property
  package.VisualStudio.legacy-properties=false so that we can test and start fixing cases where these are still used
  and eventually remove these properties.
* In the past, people use VisualStudio proxy package to select which version of VisualStudio to use. This mechanism will
  no longer work.  To select VisualStudio version, you need to explicitly provide a global property 'vsversion' and 
  assign it to 2017 or 2019.

# 15.7.27703.2042-2-proxy

### Changed

* Visual Studio Installation Detection Mechanism has been changed. Our custom vs2017detect application has been removed
  and we instead rely on and use Microsoft's vswhere.exe application to detect Visual Studio Installations.
* Removed build script setup of build.env.SystemRoot, build.env.TMP, and exec.env.TMP properties.
* Updated vs-init-reference-config-override target to make it more robust when run under parallel threads and parallel
  vcxproj post build steps.
* Updated init-common-tools.xml to define property package.VisualStudio.dir if that file is loaded directly by eaconfig
  without doing a dependent to this package.
* Updated build script setup to provide the following properties:
```
package.VisualStudio.compiler
package.VisualStudio.assembler
package.VisualStudio.archiver
package.VisualStudio.linker
package.VisualStudio.includedirs
package.VisualStudio.librarydirs
```
* Removed preprocessor defines for EA_VS_UPDATE_MAJOR_VER, EA_VS_UPDATE_MINOR_VER and EA_VS_UPDATE_PATCH_VER. Visual
  Studio now make frequent update which easily caused inconsistent vcxproj output from developer to developer. If your
  C++ project needs to test for specific version of Visual Studio, use the Microsoft predefined macro _MSC_VER or
  _MSC_FULL_VER defined [here](https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2017).


# 15.7.27703.2042-1-proxy

### Fixed

* Package will no longer warn about the deprecated sysinfo task.
	

# 15.7.27703.2042-proxy

### Changed

* Upgraded to 15.7.27703.2042 expected version.


# 15.6.27428.2002-proxy

*   Upgraded to 15.6.27428.2002 expected version.
*   Added compatibility with MSBuildTools package. Property 'package.VisualStudio.use-non-proxy-build-tools' can be set to true to prevent looking for installed Visual Studio until absolutely necessary (for example visualstudio-xxxx targets). If only using non-visualstudio targets MSBuild, VC++ Compiler and C# Compiler will be used from MSBuildTools package rather than Visual Studio install.

# 15.4.27004.2002-1-proxy

*   Moved some initialization details for VS2017 properties into a separate init-common-tools.xml script. This is primarily so that eaconfig can include this script directly and setup some properties it needs for running msbuild targets.
*   Added ARM64 support
*   Updated build environment PATH settings for invoking cl.exe to find correct cl.exe architecture's dependent dlls.

# 15.4.27004.2002-proxy

*   Use redist default.txt file in the VS2017 installation to properly discover where the vc redist dlls are. Since VS2017 15.4 the Redist txt file is now included in the VS install.
*   Minimum version check set to check for Version 15.4 of Visual Studio 2017
*   Fixed property that dictates where nmake.exe is when building for arm processor.

# 15.0.26430-4-proxy

*   Package now exports 'package.VisualStudio.csc.exe' property which other packages can use to find the correct csc.exe path for the VisualStudio version.

# 15.0.26430-3-proxy

*   package.VisualStudio.vcdir added for compatibility with previous versions of the Visual Studio package
*   package.VisualStudio.bindir added for compatibility with previous versions of the Visual Studio package
*   package.VisualStudio.hosttoolspath exposed for usage in FASTBuild
*   package.VisualStudio.crtredistpath added for FASTBuild to use
*   Added missing atlmfc includedirs and libs
*   Updated error message when MSVC tools directory are not found.

# 15.0.26430-2-proxy

*   Exposed package.VisualStudio.msbuild.exe property to allow other scripts/targets to call VS2017 version of msbuild.exe since it is not installed in the same location as previous releases of Visual Studio, and the registry keys uses in previous Visual Studio Installations have been removed.

# 15.0.26430-1-proxy

*   Defining the environment variables VisualStudioVersion, VSINSTALLDIR and VS150COMNTOOLS. These need to be defined when running some NuGet post generate install scripts.

# 15.0.26430-proxy

*   Minor updates to vs2017detect mechanism to not by default select/use Visual Studio 2017 preview installs. Since Visual Studio 2017 versions can now be installed SxS, we need a way to tell the visual studio package which version to target Now by default the VisualStudio Package will only look for non Preview versions of Visual Studio 2017\. If you want it to use preview editions pass -D:package.VisualStudio.AllowPreview=true on your solution generation command line.
*   Updated Microsoft.VisualStudio.Setup.Configuration.Interop nuget library from version 1.3.269-rc, to offical version 1.10.101
*   Updated Version check to ensure that people have the latest version of Visual Studio 2017 15.2 (15.0.26430 or beyond) installed

# 15.0.25928-proxy

*   Update due to new VS2017 RC install no longer use registry to set install path and version.

# 15.0.25914-proxy

*   Initial version to support VS2017 RC
*   Added package.VisualStudio.nmake.exe property for build scripts to reference to find the path to nmake.exe

# 15.0.25802-proxy

*   Initial version to support VS15 Preview 5

# 14.0.25420-proxy

*   Updated proxy package to test for Visual Studio 2015 Update 3 or newer version.

# 14.0.24720-proxy

*   Updated proxy package to test for Visual Studio 2015 Update 1 or newer version.
*   disable warning C4464: relative include path contains '..'

# 14.0.23107-1-proxy

*   Re-added property 'package.VisualStudio.InstalledUpdateVersion' back to initialize.xml
*   Corrected some of the error messages that still mentions Visual Studio 2013\. The error messages should have mentioned Visual Studio 2015.
*   Important NOTE: You need to use Framework 3.29.00 or newer versions in order to properly create solution file for Visual Studio 2015.

# 14.0.23107-proxy

*   Proxy package for Visual Studio 2015 RTM.
*   Important NOTE: You need to use Framework 3.29.00 or newer versions in order to properly create solution file for Visual Studio 2015.