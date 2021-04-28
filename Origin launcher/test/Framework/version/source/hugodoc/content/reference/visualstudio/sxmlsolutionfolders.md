---
title: Visual Studio Solution Folders
weight: 263
---

This topic describes how to add Projects and Loose files into folders in the Solution Explorer view in Visual Studio.

<a name="SXMLSolutionFolders"></a>
## Visual Studio solution folders ##

Visual Studio Solution folders can be defined using the `<SolutionFolders>` task.

To add projects to a folder you must indicate the names of the modules in a `<project xmlns="schemas/ea/framework3.xsd">` block using the syntax `(package)/(buildgroup)/(module)` . The buildgroup name can be omitted for runtime modules.

Loose files can be added using an `<items>` fileset.
These files are not part of a project but can be added to the solution so that they can easily be editted.

<a name="example"></a>
## Example ##


```xml

  <package name="HelloWorldLibrary"/>

  <SolutionFolders>
    <folder name="A">
      <items>
        <includes name="data/hello/*.dat" />
      </items>
      <projects>
        HelloWorldLibrary/Hello
      </projects>
      <folder name="doc">
        <items>
          <includes name="doc/*.txt" />
        </items>
      </folder>
    </folder>

  </SolutionFolders>

  <Library name="Hello">
    <includedirs>
      include
    </includedirs>
    <sourcefiles>
      <includes name="source/hello/*.cpp" />
    </sourcefiles>

  </Library>

</project>

```
