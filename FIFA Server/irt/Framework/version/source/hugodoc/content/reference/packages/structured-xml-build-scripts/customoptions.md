---
title: How to define custom build options in a module
weight: 203
---

How to specify custom build options in Structured XML without explicitly creating new build type

<a name="SXMLSetCustomBuildOptions"></a>
## Setting custom build options in a module ##

Custom build options can be defined directly in structured XML, in many cases there is no need to specify [custom buildtype]( {{< ref "reference/packages/structured-xml-build-scripts/setbuildtype.md" >}} ) .

To set custom options use `<config>` element. This element lets set

 - Additional defines
 - Warning suppression options
 - Precompiled header settings
 - Set any options that can be set using custom{{< url buildtype >}}. Structured XML task actually creates custom buildtype automatically in this case.


```xml

<Program name="MyProgram" >
  <config>
    <pch enable="true" pchheader="Pch.h"/>
    <defines>
      CUSTOM_DEFINES
      <do if="${Dll}">
        PROGRAM_EXPORTS
      </do>
    </defines>
    <warningsuppression>
      <do if="${config-compiler}==vc">
        -wd4005
      </do>
      <do if="${config-system}==ps3 and ${config-compiler}==sn">
        --diag_suppress=112
        --diag_suppress=1437
      </do>
      <do if="${config-system}==ps3 and ${config-compiler}==gcc">
        -Wno-multichar
      </do>
    </warningsuppression>
  </config>
  <sourcefiles>
    <!-- To use the 'pch' config option, we need to specify the cpp that should be used to create the precompiled header. -->
    <includes name="source/pch.cpp" optionset="create-precompiled-header"/>
    <includes name="source/*.cpp" />
  </sourcefiles>
</Program>
          

```

{{% panel theme = "info" header = "Note:" %}}
`<defines>`  and  `<warningsuppression>` elements should appear once, conditional inclusion can be done using `<do>` element.
{{% /panel %}}
Other options can be set using `<buildoptions>` element. Input is similar to the [<BuildType> task]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md#BuildTypeTask" >}} ) :

 `<buildoptions>` element can contain flags like ` *exceptions* ` , ` *optimization* ` , etc.
These flags are converted to corresponding compiler options on each platform.

Compiler linker and librarian options can be added directly using options

 - ` *buildset.cc.options* ` - C/C++ compiler options
 - ` *buildset.as.options* ` - assembly compiler options
 - ` *buildset.link.options* ` - linker options
 - ` *buildset.lib.options* ` - librarian options


{{% panel theme = "danger" header = "Important:" %}}
When adding options use `${option.value}` to grab options that were already added by config package.
{{% /panel %}}

```xml

<Program name="MyProgram">
  <config>
    <buildoptions>
      <option name="exceptions" value="on"/>
      <option name="incrementallinking" value="off"/>
      <option name="buildset.cc.options" if="${config-compiler}==vc">
        ${option.value}
        -W0
      </option>
      <option name="buildset.cc.options" if="${config-compiler}==sn">
        ${option.value}
        -Xdiag=0
      </option>
    </buildoptions>
  </config>
   . . . .

```
<a name="SXMLChangeExistingBuildOptions"></a>
## Changing existing build options in a module ##

One common mistake is trying to replace options like this:

###### Invalid Usage ######

```xml

    <buildoptions>
      <option name="buildset.cc.options">
        <!-- c++11 not supported when building C source -->
        @{StrReplace(${option.value}, '-Xstd=cpp11', ' ')}
      </option>

```
At the time this script is executed many options that are set based on the flags are not added to the option set.
Replace operation will not have intended result.

To remove or replace options use `<remove>` element:


```xml

  <remove>
    <defines/>
    <cc.options/>
    <as.options/>
    <link.options/>
    <lib.options/>
  </remove>

```
Example of using `<remove>` 


```xml

<Program name="MyProgram">
  <config>
    <buildoptions>
      <option name="exceptions" value="on"/>
    </buildoptions>
    <remove>
      <defines>
        MY_DEFINE
        <do if="${config-compiler}==vc">
           PROGRAM_EXPORTS
        </do>
      </defines>
      <cc.options>
        <do if="${config-compiler}==clang">
           -std=c++11
        </do>
      </cc.options>
    </remove>
  </config>
   . . . .

```

{{% panel theme = "info" header = "Note:" %}}
C++11 support can be turned off by setting flag ` *cc.cpp11* `  to  *off*
{{% /panel %}}
<a name="CustomBuildType"></a>
## How to create custom buildtype optionset ##

Use  `<BuildType>`  task to create new buildtype

See also  [Create Custom BuildType]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md" >}} ) 

Example creating buildtype for SPU. It adds new linker option, and additional module [preprocess step]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/_index.md" >}} )  ` *spu-name-builder* ` 


```xml

<BuildType name="JobBuildtypePdc" from="SpuJob-pdc-O2">
  <option name="preprocess">
    ${option.value}
    spu-name-builder
  </option>
  <option name="buildset.link.options">
    ${option.value??}
    -Wl,--stack-analysis
  </option>
</BuildType>


```
This example shows how to explicitly replace options in custom build type.
This should be done **after** buildtype is created.


```xml

<BuildType name="CustomCLibrary" from="CLibrary">
  <option name="buildset.cc.defines">
    ${option.value}
    MY_DEFINE
  </option>
  <option name="buildset.cc.options">
    ${option.value}
    -c
  </option>
  <option name="buildset.link.options">
    ${option.value}
    -s
  </option>
</BuildType>

<optionset name="CustomCLibrary">
  <!-- disable C++11 support for C files-->
  <option name="cc.cpp11" value="off"/>
  <option name="cc.defines">
    ${option.value}
    STATIC_LIBS
  </option>
  <option name="cc.options" if="${config-compiler}==sn">
    @{StrReplace(${option.value}, '-Os', '-O1')}
  </option>
  <option name="link.options" if="${config-compiler}==vc">
    @{StrReplace(${option.value}, '-FIXED:No', ' ')}
  </option>
</optionset>

```

##### Related Topics: #####
-  [Create Custom BuildType]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md" >}} ) 
-  [Everything you wanted to know about Framework defines, but were afraid to ask]( {{< ref "user-guides/everythingaboutdefines.md" >}} ) 
