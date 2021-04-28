---
title: assemblies
weight: 137
---

This topic describes how to add additional assemblies files for a Native Managed C++ module.

## Usage ##

Sometimes there is a need to pass assemblies (assembly references) to compiler when building MCPP code. Assemblies are added with -FU option.

Use `<assemblies>`  in SXML or define {{< url fileset >}} with name {{< url groupname >}} `.assemblies` in your build script.

Files in the `assemblies` fileset are added to the Visual input.

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <ManagedCPPAssembly name="Hello">
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
        <assemblies>
          <includes name="bin/**.dll"/>
          <includes name="System.dll" asis="true"/>
        </dllfiles>
    </ManagedCPPAssembly>

```
