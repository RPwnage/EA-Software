---
title: shaderfiles
weight: 146
---

This topic describes how to add shaderfiles to a module

## Usage ##

The SXML `<shaderfiles>` fileset can be added to a module, similar to how you would had sourcefiles.

When adding shaderfiles it is often necessary to customize the settings by using specific compiler flags.
These can be added by providing an optionset name along with the fileset items. An example of this can be seen below.
The optionset can have any name, but the options within the optionset must follow a specific naming convention.
The option 'psslc.options' is the name used when providing options for sony platforms.
The option 'fxcompile.options' is the name of the option used when providing options for pc.
These options indicate compiler flags that are passed to the shader compilers for those platforms.
When doing a visual studio build these options are converted to the corresponding project file elements and will appear properly in the property window.

There is a separate option can 'psslc.embed' for sony platforms. We have found that this option is needed when the shader compiler complains about too many input files.
It doesn't correspond to a command line argument and is thus a separate option.

## Example ##

How to use the `<shaderfiles>` element in SXML

```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package name="HelloWorldLibrary"/>

    <optionset name="custom-shader-options">
        <option name="psslc.options" if="${config-system} == 'kettle'">
            -profile sce_vs_vs_orbis 
        </option>
        <!-- The embed flag may need to be used if the shader compiler gives a 'too many input files provided' error -->
        <option name="psslc.embed" value="true"/>
        <option name="fxcompile.options">
            -Vn"g_%filename%"
            -Fh"%intermediatedir%/__Generated__/%filename%%fileext%.h"
            -Zpr
            -Zi
            -Od
            /lib
            /6_3
        </option>
    </optionset>

    <Library name="Hello">
      <shaderfiles>
        <includes name="shaders/**.pssl" if="${config-system} == 'ps4'" 
            optionset="custom-shader-options"/>
        <includes name="shaders/**.hlsl" if="${config-system} == 'pc' and ${config-system} == 'pc64'" 
            optionset="custom-shader-options"/>
      </shaderfiles>
      <sourcefiles>
        <includes name="source/hello/*.cpp" />
      </sourcefiles>
    </Library>

```
