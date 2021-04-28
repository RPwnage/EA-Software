---
title: resourcefiles
weight: 145
---

This topic describes how to add resource files for a Native (C/C++) module.

## Usage ##

Use `<resourcefiles>`  in SXML or define {{< url fileset >}} with name {{< url groupname >}} `.resourcefiles` in your build script.


{{% panel theme = "info" header = "Note:" %}}
Embedded resource path is computed relative to the fileset base directory. Set appropriate base directory value.
{{% /panel %}}
To pass additional include directories to the resource compiler use property{{< url groupname >}} `.res.includedirs` 

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <WindowsProgram name="Hello">
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
        <resourcefiles basedir="${package.dir}/sources"
           includedirs="specify additional include directories for resource compiler">
          <includes name="source\hello\*.ico" basedir="${package.dir}/sources/hello"/>
          <includes name="source\hello\*.rc"/>
        </resourcefiles>
    </WindowsProgram>

```

##### Related Topics: #####
-  [res.includedirs]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/resourceincludedirs.md" >}} ) 
-  [res.outputdir]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/resoutputdir.md" >}} ) 
-  [res.name]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/resname.md" >}} ) 
