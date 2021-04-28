---
title: Visual Studio Custom Project File Elements
weight: 266
---

This topic describes how to insert custom elements into a visual studio project file.

<a name="overview"></a>
## Overview ##

When a visual studio project is generated for a module
the settings in the project file are determined automatically based on framework properties that are translated into xml elements.
However, sometimes a user may wish to add a specific element to their project file and there is no other framework property that controls this.

There are a few different ways to do this depending on the type of element that you want to add.

## Visual Studio Extensions ##

Visual Studio Extensions are the most flexible way to modify a project file.
They allow users to write exactly what they want to a project file.

You can add a visual studio extension that applies to just a single module using the property &quot;{{< url groupname >}}.visualstudio-extension&quot;.
It is also possible to apply it to all modules for a single platform using the property &quot;${config-platform}-visualstudio-extension&quot;.

Here is a brief example of how to write a simple visualstudio extension.
For a more complete guide, including a visual representation of all the places in a project file where you can write to, see the page: [Visual Studio Extensions]( {{< ref "reference/visualstudio/visualstudioextensions.md" >}} ) 

###### Adding a visual studio extension in SXML style build script ######

```xml
<taskdef assembly="my_extension.dll">
  <sources>
    <includes name="${package.dir}/scripts/MyCustomVisualStudioExtension.cs"/>
  </sources>
</taskdef>

<Program name="mymodule">
   <sourcefiles>
     <includes name="source/*.cpp"/>
   </sourcefiles>
   <visualstudio>
     <extension>
       my-custom-vc-extension
     </extension>
   </visualstudio>
</Program>
```
###### Adding a visual studio extension in build script ######

```xml
<taskdef assembly="my_extension.dll">
 <sources>
  <includes name="${package.dir}/scripts/MyCustomVisualStudioExtension.cs"/>
 </sources>
</taskdef>

<property name="runtime.mymodule.visualstudio-extension">
 my-custom-vc-extension
</property>

<Program name="mymodule">
 <sourcefiles>
  <includes name="source/*.cpp"/>
 </sourcefiles>
</Program>
```
###### Example C# VisualStudio Extension Task ######

```csharp
using System;
using NAnt.Core;
using NAnt.Core.Attributes;
using EA.Eaconfig.Core;

namespace EA.Eaconfig
{
    [TaskName("my-custom-vc-extension")]
    public class My_Custom_VisualStudioExtension : IMCPPVisualStudioExtension
    {
        // update the vcproj files with the compiler/linker override
        public override void WriteExtensionToolProperties(IXmlWriter writer)
        {
            writer.WriteNonEmptyElementString("MyCustomMsBuildOption", "value");
        }
    }
}
```
<a name="msbuildoptions"></a>
## Custom MSBuild Options ##

One type of option that users my want to inject are MSBuild options which are global settings that usually apply to the entire build.

For example, a user may want to add the element &quot;SomeOption&quot; with the value &quot;SomeValue&quot;, and have it appear it the project file like in the example below:

###### part of a visual studio project file showing a property group with a custom element added ######

```xml
<PropertyGroup Condition=" '$(Configuration)|$(Platform)' == 'pc-vc-dev-debug|Win32' ">
  <DefineConstants>DEBUG;EA_DOTNET2;TRACE</DefineConstants>
  <DebugSymbols>true</DebugSymbols>
  <OutputPath>C:\samples\helloworld\1.00.28\build\helloworld\1.00.28\pc-vc-dev-debug\bin</OutputPath>
  <TreatWarningsAsErrors>true</TreatWarningsAsErrors>
  <WarningLevel>4</WarningLevel>
  <DebugType>full</DebugType>
  <PlatformTarget>x86</PlatformTarget>
  <SomeOption>SomeValue</SomeOption>
</PropertyGroup>
```
Custom build types can add the elements and these will be used by all of the modules that use this build type.

###### Modifying a buildtype ######

```xml
<BuildType name="MyBuildType" from="ManagedCppProgram">
  <option name="msbuildoptions">
    SomeOption=SomeValue
  </option>
</BuildType>
```
Modules can be given a list of options using the &#39;msbuildoptions&#39; optionset in structured xml.

###### As part of a module using Structured XML ######

```xml
<ManagedCppProgram name="MyRuntimeModule">
<visualstudio>
  <msbuildoptions>
    <option name="SomeOption" value="SomeValue"/>
  </msbuildoptions>
</visualstudio>
<sourcefiles basedir="${package.dir}/source">
  <includes name="${package.dir}/source/*.cpp" />
</sourcefiles>
 </ManagedCppProgram>
```
The &#39;msbuildoptions&#39; optionset can also be set without using structured xml.

###### Without using Structured XML ######

```xml
<optionset name="runtime.MyRuntimeModule.msbuildoptions">
  <option name="SomeOption" value="SomeValue"/>
</optionset>
```
<a name="Modifying Assembly References"></a>
## Modifying assembly references ##

Assembly references can be modified to adjust their settings.
We originally added this feature in order to be able to force visual studio to use a specific version of an assembly, but it may have other uses.

###### Modifying an Assembly reference in a csproj file ######

```xml
<optionset name="automation-assembly-settings">
 <option name="visual-studio-assembly-attributes">
  Include=System.Management.Automation, Version=1.0.0.0, Culture=neutral, processorArchitecture=MSIL
 </option>
 <option name="visual-studio-assembly-properties">
  SpecificVersion=True
 </option>
</optionset>
        
<Module name="NAnt.NuGet" buildtype="AnyCPUCSharpLibrary">
 <assemblies basedir="${package.dir}">
  <includes name="System.Management.Automation.dll" asis="true" optionset="automation-assembly-settings"/>
 </assemblies>
</Module>
```
###### The effect the previous build script has on the visual studio project file ######

```xml
<Reference Include="System.Management.Automation, Version=1.0.0.0, Culture=neutral, processorArchitecture=MSIL" Condition="">
  <SpecificVersion>True</SpecificVersion>
  <Name>System.Management.Automation</Name>
  <HintPath>System.Management.Automation.dll</HintPath>
  <Private>False</Private>
</Reference>
```
<a name="customtools"></a>
## Custom Tools ##

Another way to modify how visual studio project file elements are generated is by using custom tools.
You can apply custom tools to resource and content files by applying an optionset to individual file items.
Custom Tools let you set the name of the element, add attributes to the element and add properties within the element.

###### Creating your own tool ######

```xml

<optionset name="my-custom-tool-settings">
  <option name="visual-studio-tool" value="MyCustomTool"/>
  <option name="visual-studio-tool-attributes">
    Condition='$(BuildingInsideVisualStudio)'!='true'
  </option>
  <option name="visual-studio-tool-properties">
    IncludedInBuild=true
    MyParameter=10
  </option>
</optionset>

<CSharpLibrary name="MyLibrary">
  <!-- Include the file where you define your "MyCustomTool" msbuild task. -->
  <importmsbuildprojects value="${package.dir}\msbuild\MyCustomToolDefinition.Target"/>
  <sourcefiles>
    <includes name="source/**.cs"/>
  </sourcefiles>
  <resourcefiles>
    <!-- Specify the above optionset to assign the custom Tool for these files -->
    <includes name="source/**.dat" optionset="my-custom-tool-settings"/>
  </resourcefiles>
</CSharpLibrary>

```
###### Generated csproj fragment ######

```xml

<Import Project="..\..\msbuild\MyCustomToolDefinition.Target" />
...
<MyCustomTool Include="..\..\source\test.dat" Condition="'$(BuildingInsideVisualStudio)'!='true'">
  <IncludedInBuild>true</IncludedInBuild>
  <MyParameter>10</MyParameter>
</MyCustomTool>

```
For more details see: [Files With Custom Tools]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/fileswithcustomtools.md" >}} ) 

