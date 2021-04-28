---
title: Structured XML build scripts
weight: 199
---

Structured XML is a particular style of writing build scripts that was introduced in Framework 3.

<a name="Overview"></a>
## What is Structured XML? ##

Structured XML is a set of [Framework Tasks]( {{< ref "reference/tasks/_index.md" >}} ) with attributes and nested elements used to define various properties, filesets and optionsets.

Structured XML forms a layer on top of the Framework properties and filesets that were traditionally used to define modules.
This makes build scripts more compact and easy to follow.
It also makes build scripts easier to write because all of the options available in a module can be clearly documented and
Visual Studio can provide auto-completion with intellisense.

The Framework structured XML tasks still convert all of the data to the traditional properties and filesets,
which means that a mixture of new and old style build scripts can be used for a more gradual transition.

Here is example of traditional build file with several different types of dependencies between modules:


```xml
.         <project default="build">

.           <package name="example" />

.           <property name="runtime.buildmodules" value="ExampleLib ExampleProgram" />

.           <property name="runtime.ExampleLib.buildtype" value="Library" />
.           <property name="runtime.ExampleLib.usedependencies">
.             OtherPackage
.           </property>
.           <property name="runtime.ExampleLib.includedirs">
.             ${package.dir}/include
.           </property>
.           <fileset name="runtime.ExampleLib.headerfiles">
.             <includes name="${package.dir}\include\ExamplePackage\*.h"/>
.           </fileset>
.           <fileset name="runtime.ExampleLib.sourcefiles">
.             <includes name="source\Lib\ExampleLib.cpp"/>
.           </fileset>

.           <property name="runtime.ExampleProgram.buildtype" value="Program"/>
.           <property name="runtime.ExampleProgram.builddependencies">
.             OtherPackage
.           </property>
.           <property name="runtime.ExampleProgram.runtime.moduledependencies">
.             ExampleLib
.           </property>
.           <property name="runtime.ExampleProgram.OtherPackage.runtime.buildmodules">
.             ModuleInOtherPackage
.           </property>
.           <property name="runtime.ExampleProgram.includedirs">
.             ${package.dir}/include
.           </property>
.           <fileset name="runtime.ExampleProgram.headerfiles">
.             <includes name="${package.dir}\include\ExamplePackage\*.h"/>
.           </fileset>
.           <fileset name="runtime.ExampleProgram.sourcefiles">
.             <includes name="source\App\ExampleProgram.cpp"/>
.           </fileset>
.         </project>
```
Dependencies for the module &quot;ExampleProgram&quot; are defined by the three different properties and some of them have quite complex names:


```xml
.         <!-- build dependency on another package "OtherPackage" -->
.         <property name="runtime.ExampleProgram.builddependencies">
.           OtherPackage
.         </property>
.         <!-- build dependency on another module in runtime group -->
.         <property name="runtime.ExampleProgram.runtime.moduledependencies">
.           ExampleLib
.         </property>
.         <!-- This property says that ExampleProgram depends on a single module from "OtherPackage" package -->.

.         <property name="runtime.ExampleProgram.OtherPackage.runtime.buildmodules">
.           ModuleInOtherPackage
.         </property>
```
The same type of information is expressed much cleaner in structured XML:


```xml
.         <project default="build">

.           <package name="example" />

.           <Library name="ExampleLib">
.             <dependencies>
.               <use>OtherPackage</use>
.             </dependencies>
.             <includedirs>${package.dir}/include</includedirs>
.             <headerfiles>
.               <includes name="${package.dir}/include/ExamplePackage/*.h"/>
.             </headerfiles>
.             <sourcefiles>
.               <includes name="${package.dir}/source/Lib/ExampleLib.cpp"/>
.             </sourcefiles>
.           </Library>

.           <Program name="ExampleProgram">
.             <dependencies>
.               <!-- dependency: package_name/group[optional]/module_name-->
.               <build>
.                 OtherPackage/ModuleInOtherPackage
.                 example/ExampleLib
.               </build>
.             </dependencies>
.             <includedirs>${package.dir}/include</includedirs>
.             <headerfiles>
.               <includes name="${package.dir}/include/ExamplePackage/*.h"/>
.             </headerfiles>
.             <sourcefiles>
.               <includes name="source/App/ExampleProgram.cpp"/>
.             </sourcefiles>
.           </Program>

.         </project>
```
<a name="StructuredXMLTaskHierarchy"></a>
## StructuredXML Task Hierarchy ##

Following Structured XML classes define attributes and elements that can be used in XML

 - **Module** - defines all input for any native module.
 - **Utility** - defines all input for Utility type modules
 - **Makestyle** - defines all input for MakeStyle modules
 - **DotNetModule** - defines all input for C# modules. DotNetModule can not be used directly in XML scripts.
 - **PartialModule** - defines partial module data for native or DotNet types.

Several subtypes of Module task are defined for convenience. These subtypes predefine the{{< url buildtype >}}value. For example &#39;Module&#39; can be used to define library:


```xml
.         <Module name="MyModule" buildtype="Library">
.           . . . . . .
.         </Module>
```
The same can be written using Library task:


```xml
.         <Library name="MyModule">
.           . . . . . .
.         </Library>
```
&#39;buildtype&#39; can still be changed:


```xml
.         <Library name="MyModule" buildtype="MyCustomLibrary">
.           . . . . . .
.         </Library>
```
![Structured Xml Task Hierarchy]( structuredxmltaskhierarchy.png )