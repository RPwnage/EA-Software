---
title: relationship
weight: 50
---

This element contains meta data for build and test coverage.

## Usage ##

Enter the element in the Manifest file and specify following nested elements: `builds` ,  `platforms` .

##### builds #####
Defines supported build types and targets.

Each `builds`  element must contain one or more  `build` elements that define type of the build supported and targets associated with this type.

##### build #####
Defines targets and groups for particular build type. This is used by the EATech Build Farm to determine which targets to call for building and testing.

 `build` element can contain following attributes

   `name` - can be one of the following values<br><br>  - **solution** - Indicates Visual Studio solution generation is supported.<br><br>Build farm uses this to determine whether or not to build with Visual Studio on platforms where it is available.<br>It is recommended that the target field be left blank so build farm can choose the appropriate one. (usually &quot;slngroupall visualstudio&quot;)<br>  - **nant** - Indicates native nant build is supported.<br><br>Build farm uses this on platforms where Visual Studio builds are not supported in order to determine what targets to run to build the package.<br>It is recommended to have a target for each of the build groups you want to build, for example if the package has runtime and test modules set the targets to &quot;build test-build&quot;.<br>  - **test** - Indicates package has tests that can be executed.<br><br>Build farm uses this to determine what targets to call to run tests.<br>These targets make up the test step which happens after either a solution build or a native nant build.<br>Typically this should use &quot;test-run-fast&quot;.<br>You should not use any of the run targets that do not include the &quot;-fast&quot; suffix because they implicitly execute the build target for that group before running but the files have already been built during the build step.
   `supported`  - Boolean value indicating whether this  `build` is supported.
   `targets` - List of Framework targets supported for this build type.

##### platforms #####
platform information useful for build farm metadata

 `build` element can contain following attributes

Valid options for build and test attributes are

 1. Supported
 2. NotSupported
 3. FutureSupport
 4. Deprecated
 5. KnownFailure

   `name`  - name of the platform as it is defined in config package ( `${config-system}` property).
   `test` - Status of the tests on this platform. See valid values listed above.
   `build` - Status of the build on this platform. See valid values listed above.
   `containsPlatformCode` - Boolean value indicating whether package contains platform specific code.
   `configs` - represents specific configurations for the platform the package only supports (can be truncated as a means of wildcarding).

## Example ##


```xml
<relationship>
   <builds>
        <build name="solution" supported="false" />
        <build name="nant" supported="true" targets="build example-build test-build" />
        <build name="test" supported="true" targets="test-run" />
    </builds>
    <platforms>
        <platform name="pc" test="Supported" build="Supported" configs="pc-vc-dev" />
        <!-- platform support for configs that begin pc-vc-dev, but not for example pc-vc-dll -->
        <platform name="kettle" test="Supported" build="Supported" containsPlatformCode="true" />
        <!-- tests exist -->
        <platform name="capilano" test="Supported" build="Supported" containsPlatformCode="true" />
        <!--  tests exist  -->
        <platform name="android" test="Supported" build="Supported" containsPlatformCode="false" />
        <!--  build, do not test -->
        <platform name="no-test-incoming" test="inprogress" build="Supported" containsPlatformCode="false" />
        <!--  there will be tests, not written yet -->
        <platform name="recent_platform" test="NotSupported" build="NotSupported" containsPlatformCode="false" />
        <!--  package supports this platform, but no platform specific code -->
        <platform name="new_platform" test="NotSupported" build="NotSupported" containsPlatformCode="true" />
        <!--  package supports this platform, platform specific code  -->
    </platforms>
</relationship>
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
