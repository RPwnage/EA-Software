---
title: Partial Modules
weight: 197
---

Partial modules are templates of modules defined using structued xml syntax.
When working with a large number of modules that all have similar options you can simplify your modules by inheriting the common properties from a partial module.
Partial modules are different from custom buildtypes because, well buildtypes allow you to set the build settings, partial modules allow you to set defaults for almost all aspects of a module,
such as setting a default list of source files or dependencies.

<a name="PartialModules"></a>
## Partial Modules ##

To define a partial module use the &#39;PartialModule&#39; element. The same &#39;PartialModule&#39;
element can be used to define partial modules for native NAnt modules and for DotNet (C# and F#). Framework dynamically decides the type depending where the partial module is used.

Partial module is derived from &#39;Module&#39; or from &#39;DotNetModule&#39; and all elements available there can be used in partial modules.

 - For native types Partial module is derived from {{< task "EA.Eaconfig.Structured.ModuleTask" >}}. All &#39;Module&#39; elements can be used in partial module.
 - For C# or F# types Partial module is derived from DotNetModule. All &#39;DotNetModule&#39; elements can be used in partial module.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>&#39;DotNetModule&#39; itself can not be used in XML. For list of elements see any of the following:<br><br>  - {{< task "EA.Eaconfig.Structured.CSharpLibraryTask" >}}<br>  - {{< task "EA.Eaconfig.Structured.CSharpProgramTask" >}}<br>  - {{< task "EA.Eaconfig.Structured.CSharpWindowsProgramTask" >}}<br>{{% /panel %}}

<a name="UsingPartialModules"></a>
### Using Partial Modules ###

To use Partial Module in another Module Definition define the  ** `frompartial` ** attribute:


```xml
.                 <CSharpLibrary name="MyLibrary" frompartial="MyPartialCharpModule">
.                   <rootnamespace>EA.MyLibrary</rootnamespace>
.                   <sourcefiles>
.                     <includes name="**.cs"/>
.                     <includes name="**.xaml"/>
.                   </sourcefiles>
.                 </CSharpLibrary>
```

{{% panel theme = "info" header = "Note:" %}}
** `frompartial` ** can contain multiple partial module names separated by spaces.
Partial modules are applied in the order they are defined in ** `frompartial` ** .
{{% /panel %}}

{{% panel theme = "warning" header = "Warning:" %}}
PartialModule itself can not use ** `frompartial` ** attribute. Partial Modules can not be derived from other partial modules.
{{% /panel %}}
<a name="DefiningPartialModules"></a>
### Defining Partial Modules ###

Partial module can be defined like any other module in Structured XML. The difference is that partial module definition script is not executed
right away. Like in NAnt Task and Target definitions partial module script is stored and executed in the context of the build file for each module that uses
Partial Module. This allows writers to put partial modules in packages other than the package that is using them, for example,
in a game [configuration]( {{< ref "reference/configurationpackage.md" >}} ) package.

Example of the Partial Module definition:


```xml
.                 <PartialModule name="MyPartialModule" buildtype="${MyStaticOrSharedLibrary}">
.                   <config>
.                     <pch enable="${config.enable-pch}" pchheader="Pch.h"/>
.                   </config>
.                   <includedirs>
.                     ${package.builddir}/__Generated__/${config-system}
.                     ${nant.project.basedir}
.                   </includedirs>
.                   <headerfiles>
.                     <!-- Standard platform excludes  -->
.                     <excludes name="**/pc/**" unless="${config-system}==pc or ${config-system}==pc64"/>
.                     <excludes name="**/pc64/**" unless="${config-system}==pc64"/>
.                     <excludes name="**/capilano/**" unless="${config-system}==capilano"/>
.                     <excludes name="**/kettle/**" unless="${config-system}==kettle"/>
.                     <!--  Header platform includes -->
.                     <includes name="**/${config-system}/**.h"/>
.                   </headerfiles>
.                   <sourcefiles>
.                     <includes name="Pch.cpp" optionset="create-precompiled-header"/>
.                     <!-- Standard platform excludes  -->
.                     <excludes name="**/pc/**" unless="${config-system}==pc or ${config-system}==pc64"/>
.                     <excludes name="**/pc64/**" unless="${config-system}==pc64"/>
.                     <excludes name="**/capilano/**" unless="${config-system}==capilano"/>
.                     <excludes name="**/kettle/**" unless="${config-system}==kettle"/>
.                     <!-- Source platform includes -->
.                     <includes name="**/${config-system}/**.cpp"/>
.                   </sourcefiles>
.                   <bulkbuild enable="true"  maxsize="20"/>
.                 </PartialModule>
```
Here is how this partial module can be used:


```xml
.                 <Library name="MyLibrary" frompartial="MyPartialModule">
.                   <config>
.                     <warningsuppression>
.                       -wd4836
.                     </warningsuppression>
.                     <defines>
.                       <do if="${Dll}">
.                         DLL_EXPORTS
.                       </do>
.                     </defines>
.                   </config>
.                   <dependencies>
.                     <use>
.                       EABase
.                       coreallocator
.                     </use>
.                     <auto>
.                       EAAssert
.                       EAIO
.                     </auto>
.                   </dependencies>
.                   <headerfiles>
.                     <includes name="**.h"/>
.                   </headerfiles>
.                   <sourcefiles>
.                     <includes name="**.cpp"/>
.                   </sourcefiles>
.                 </Library>
```
Example of C# PartialModule


```xml
.                 <PartialModule name="MyCSharpModule" buildtype="CSharpLibrary">
.                   <script order="before">
.                     <!-- Generate the csproj in the same folder as the .build file, fixes lots of issues with csc / xaml compiler -->
.                     <property name="package.${package.name}.designermode" value="true" />
.                   </script>
.                   <config>
.                     <targetframeworkversion>4.0</targetframeworkversion>
.                     <buildoptions>
.                       <option name="platform">x64</option>
.                       <option name="warningsaserrors">true</option>
.                     </buildoptions>
.                   </config>
.                   <assemblies>
.                     <includes name="PresentationCore.dll" asis="true"/>
.                     <includes name="PresentationFramework.dll" asis="true"/>
.                     <includes name="System.ComponentModel.Composition.dll" asis="true"/>
.                     <includes name="System.Data.dll" asis="true"/>
.                     <includes name="System.Data.DataSetExtensions.dll" asis="true"/>
.                   </assemblies>
.                   <sourcefiles>
.                     <includes name="**.cs"/>
.                     <includes name="**.xaml"/>
.                   </sourcefiles>
.                 </PartialModule>
```
The definition of a C# module based on a partial module can be as simple as this:


```xml
.                 <CSharpLibrary name="MyAssembly"  frompartial="MyCSharpModule">
.                   <dependencies>
.                     <build>
.                       OtherCSharpMPackage
.                       SamePackage/MyOtherAssembly
.                     </build>
.                   </dependencies>
.                 </CSharpLibrary>
```
