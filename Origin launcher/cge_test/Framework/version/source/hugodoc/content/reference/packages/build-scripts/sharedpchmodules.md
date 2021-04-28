---
title: Shared Pch Modules
weight: 197
---

Shared Pch modules are special native c/c++ modules to assist in building just the precompiled header binary file (.pch) which will be shared across other modules
so that other individual modules don't need to rebuild the exact same binary.  Note that not all platforms support sharing precompiled header binary (due to various
reasons including special Visual Studio Integration setup for some platforms).  For platforms that do support shared precompiled header binary, that platform's
config setup will have a property `${config-system}.support-shared-pch` created and set to true.


<a name="SharedPchModules"></a>
## Shared Pch Modules ##

To define a shared pch module, use the {{< task "EA.Eaconfig.Structured.SharedPchTask" >}} task.  You only need to provide the module name in this task.  All necessary information is actually
specified as export data in Initialize.xml.

In the Initialize.xml file, you use the {{< task "EA.Eaconfig.Structured.SharedPchPublicData" >}} publicdata task to provide all the necessary export properties.

Then for higher level modules to actually use this shared precompiled header binary, we just need to provide the [&lt;usesharedpch&gt;]( {{< ref "reference/other/usesharedpchelement.md" >}} ) XML element in the module definition
and specify the shared pch module by using regular depenency syntax in module specification, ie (package_name])/(group)/(module_name).

This shared pch module is basically a special Library build type module for platforms that support shared precompiled
header binary.  This module is expected to contain only one source file (set up in {{< task "EA.Eaconfig.Structured.SharedPchPublicData" >}} task's pchsource attribute) to
trigger a precompiled header binary build.  For platforms that doesn't support sharing precompiled header binary (ie with `${config-system}.support-shared-pch` created and set to false or property not provided), 
Framework will automatically set the module's build type as [Utility]( {{< ref "reference/packages/build-scripts/utility-modules/_index.md" >}} ) module. User can provide 
their custom buildtypes (if there are special build compiler flags they need to setup).  This special custom buildtype information needs to be
specified in the &#39;buildtype&#39; attribute in {{< task "EA.Eaconfig.Structured.SharedPchPublicData" >}} publicdata task.

{{% panel theme = "warning" header = "Warning:" %}}
The build options used to create this shared PCH binary must match the build options for the module trying to use it.  If any module has different build options
or contain specific files that has different build options (such as different architecture, language settings, defines that affect the headers), 
these modules cannot use the shared PCH binary (must set the "use-shared-binary" attribute in [&lt;usesharedpch&gt;]( {{< ref "reference/other/usesharedpchelement.md" >}} ) 
to false) or you could get build error.  At the moment, Framework will not do this error check for you.  It is up to you to make sure the build options are compatible!
{{% /panel %}}

{{% panel theme = "warning" header = "Warning:" %}}
On PC, modules using the shared pch binary requires seeding the pdb file from the shared pch module's pdb.  This instruction is inserted in the module's
[custom build step]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/custombuildsteps.md" >}} ).  This means if the module already contains it's own
custom build step, this module cannot use shared pch binary (must set the "use-shared-binary" attribute in 
[&lt;usesharedpch&gt;]( {{< ref "reference/other/usesharedpchelement.md" >}} ) to false).
{{% /panel %}}

{{% panel theme = "info" header = "Internal Details:" %}}
Some platforms (like platforms that use Visual Studio compiler), generating a precompiled header binary will also compile the .cpp source file and generate an .obj object
file.  This object file will ultimately creates a library file and then get linked to the final program module.  However, for most platforms that uses clang compiler 
(like the unix platforms), the precompiled header build actually do not create an .obj object file and therefore will not create a library file.  These platforms will have a
`${config-system}.shared-pch-generate-lib` property created and set to false in the config setup.  When Framework encounter this `${config-system}.shared-pch-generate-lib` property
being set to false, it will setup a build-only dependency to this SharedPch module but will not set up a link dependency to this module.
{{% /panel %}}

<a name="SettingUpAndUsingSharedPchModules"></a>
### Setting Up and using Shared Pch Modules ###

The first thing to do in setting this up is the provide the proper export properties in Initialize.xml as your build file will be setup based on information from this setup.

Example Initialize.xml export data setup for the Shared PCH module using the {{< task "EA.Eaconfig.Structured.SharedPchPublicData" >}} task:

```xml
.        <publicdata packagename="BasePackage">
.            <!-- 
.                The following module attributes will setup these properties:
.                package.(package_name).(module_name).pchheader       - The file name of the header file
.                package.(package_name).(module_name).pchheaderdir    - The directory where the above header file is located
.                package.(package_name).(module_name).pchsource       - The empty source file being used to trigger the precompiled header generation
.                package.(package_name).(module_name).pchfile         - The output precompiled header binary
.            -->
.            <sharedpchmodule name="Base.SharedPch" 
                 pchheader="Base.SharedPch.h" 
                 pchheaderdir="${package.BasePackage.dir}/include/Base.SharedPch"
                 pchsource="${package.BasePackage.dir}/source/Base.SharedPch.cpp"
                 pchfile="${package.BasePackage.libdir}/Base.SharedPch.pch">
.                <!-- 
.                    Any defines that your higher level modules needed, you need
.                    to define them here as well when building the pch binary.
.                    Otherwise, your binary would not be sharable.
.                -->
.                <defines>
.                    UTF_USE_EAASSERT=1
.                </defines>
.                <!--
.                    If your pch header made references to headers of other 
.                    package/module, you need to list them here as well so
.                    that include paths can be properly propagated up.
.                -->
.                <publicdependencies>
.                    EABase
.                </publicdependencies>
.            </sharedpchmodule>
.        </publicdata>
```

Then in the module's build file, just need to provide the module name using the special {{< task "EA.Eaconfig.Structured.SharedPchTask" >}} module task.

Example Shared PCH module build file setup:

```xml
.        <project>
.            <package name="BasePackage"/>
.            <SharedPch name="Base.SharedPch"/>
.        </project>
```

For higher level module that want to use this SharedPch module, just insert the [&lt;usesharedpch&gt;]( {{< ref "reference/other/usesharedpchelement.md" >}} ) element and provide
the module name dependency specification, ie (package_name])/(group)/(module_name).

Example build script for using the above shared pch module:

```xml
.        <Library name="MyLibrary">
.            <config>
.                <pch enable="true"/>
.            </config>
.            <!-- If pch is disabled above, only the header of Base.SharedPch will get forced include to current module --> 
.            <usesharedpch module="BasePackage/Base.SharedPch"/>
.            ...
.        </Library>
```

