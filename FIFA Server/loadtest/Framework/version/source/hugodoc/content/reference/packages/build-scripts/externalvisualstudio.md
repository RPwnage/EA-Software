---
title: External Visual Studio Project modules
weight: 196
---

How to add external Visual Studio project to Framework build

<a name="ExternalVisualStudioProject"></a>
## External Visual Studio Project ##

In Framework it is possible to add existing Visual Studio project into generated solution.

To do this you still need a module definition in build script, but using the `VisualStudioProject` type
you can provide a path to the location of the visual studio project.

To define `VisualStudioProject` :

 - Use the `VisualStudioProject` Structured XML Element
 - `ProjectFile` : should contain full path to the Visual Studio Project File.
 - `ProjectConfig` : should contain Visual Studio Project  **Configuration**  name that corresponds to the current Framework  `${config}` value.
 - `ProjectFile` : should contain Visual Studio Project  **Platform**  name that corresponds to the current Framework  `${config}` value.
 - `guid` : should contain Visual Studio Project GUID which is inserted into the generated solution.
 - `managed` : when true this attribute indicates that the project is managed.<br><br>Default value is `false` when omitted.
 - `unittest` : for unit test C# or F# projects<br><br>Default value is `false` when omitted.

<a name="Example"></a>
## Example ##


```c#
<VisualStudioProject name="SXMLTestCSProj1" guid="24BA3CF2-161C-471B-9A7F-655DFFF253F0" managed="false" unittest="false">
 <ProjectFile value="${package.dir}/MyExternalProject.csproj"/>
 <ProjectConfig value="Release"/>
 <ProjectConfig value="Debug" if="${config-name}==debug"/>
 <ProjectPlatform value="AnyCPU"/>
</VisualStudioProject>
```
