---
title: Files with CustomTool in Visual Studio
weight: 176
---

Framework allows you to define files with custom tools in generated Visual Studio projects.  You can also use this
feature to override Framework&#39;s default assignment of the Visual Studio tool being used.

<a name="Section1"></a>
## How to define custom tools ##

To attach custom tool to a file in generated Visual Studio project define an optionset with following options:

 - **visual-studio-tool** - name of the tool
 - **visual-studio-tool-attributes** - Attributes for the tool XML element.<br>Each attribute should be defined on a separate line using format `Name=Value` .
 - **visual-studio-tool-properties** - Nested elements (properties)  for the tool XML element.<br>Each property should be defined on a separate line using format `Name=Value` .

## Example ##

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
The corresponding generated csproj file fragment will look something like this:

###### Generated csproj fragment ######

```xml

<Import Project="..\..\msbuild\MyCustomToolDefinition.Target" />
...
<MyCustomTool Include="..\..\source\test.dat" Condition="'$(BuildingInsideVisualStudio)'!='true'">
  <IncludedInBuild>true</IncludedInBuild>
  <MyParameter>10</MyParameter>
</MyCustomTool>

```
Here&#39;s another example for using it to override the default tool being assigned by Framework.

###### Override Framework's default tool assignment ######

```xml

<optionset name="csproj-force-embededresource-action">
  <option name="visual-studio-tool" value="EmbeddedResource"/>
</optionset>

<CSharpLibrary name="TestLibrary">
  <sourcefiles>
    <includes name="source/**.cs"/>
  </sourcefiles>
  <contentfiles>
    <includes name="${package.dir}/source/*.config"/>
    <!-- Changing the default "Content" tool assignment to "EmbeddedResource" assignment using optionset -->
    <includes name="${package.dir}/source/**.css" optionset="csproj-force-embededresource-action"/>
  </contentfiles>
</CSharpLibrary>

```
The corresponding generated csproj file fragment will look something like this:

###### Generated csproj fragment ######

```xml

<EmbeddedResource Include="..\..\source\test.css">
  ...
</EmbeddedResource>

```
