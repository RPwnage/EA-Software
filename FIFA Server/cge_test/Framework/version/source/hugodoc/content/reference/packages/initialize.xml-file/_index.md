---
title: Initialize.xml file
weight: 72
---

Initialize.xmlfile represents a package&#39;s public data.
Usually the Initialize.xml file contains a package&#39;s include directories, a list of libraries, assemblies, etc.

<a name="InitializeXmlFile_Overview"></a>
## Overview ##

When a package has a dependency on another package the Initialize.xml file of the dependent package is loaded into the parent package&#39;s project.
Framework targets can use any list of known properties and filesets defined in the Initialize.xml file to find include directories,
libraries and other public data of dependent package.

Initialize.xml file is loaded by the&lt;dependent&gt;task.
When a package declares a dependency on another package the&lt;dependent&gt;task is executed by Framework targets.
When the&lt;dependent&gt;task is invoked in the *.build script, the Initialize.xml file is loaded as well and all data defined in this
file become available in the script.

Before loading the Initialize.xml file the&lt;dependent&gt;task sets several properties:

property name |Description |
--- |--- |
| package. *[Dependent Package Name]* .dir | Directory where package *[Dependent Package Name]* is located |
| package. *[Dependent Package Name]* .version | Version of the package |
| package. *[Dependent Package Name]* .builddir | Build directory of the package |
| package. *[Dependent Package Name]* .sdk-version | Version of the SDK or third party files contained within the package.<br>By default this is just the package version number up to the first dash character,<br>unless it has been overridden in the package&#39;s initialize.xml file.<br>Also, if the package&#39;s version starts with &quot;dev&quot; this property will be undefined. |

<a name="InitializeXmlFile_PublicDataTask"></a>
### Structured XML &lt;publicdata&gt; task. ###

Preferred way to define data in Initialize.xml file is to use Structured XML {{< task "EA.Eaconfig.Structured.PackagePublicData" >}}and{{< task "EA.Eaconfig.Structured.ModulePublicData" >}}tasks.

Public data task sets two additional NAnt properties that can used in the script:

property name |Description |
--- |--- |
| package. *[Dependent Package Name]* .libdir | Standard Framework location for libraries: `${package. *[Dependent Package Name]* .builddir}/${config}/lib`  |
| package. *[Dependent Package Name]* .bindir | Standard Framework location for dlls and executables: `${package. *[Dependent Package Name]* .builddir}/${config}/bin`  |

{{< task "EA.Eaconfig.Structured.PackagePublicData" >}}and{{< task "EA.Eaconfig.Structured.ModulePublicData" >}}tasks provide simple input options for all public data that can be set in Initialize.xml file.
Default values can be filled automatically.

Here is an example usage:


```xml

  <project xmlns="schemas/ea/framework3.xsd">
                
    <publicdata packagename="ExampleLibraries">

      <!-- runtime group is default and can be skipped -->
      <runtime>
                    
        <module name="ExplicitLibsModule">
          <includedirs>
            include
          </includedirs>
          <libs>
            <includes name="${package.ExampleLibraries.libdir}/${lib-prefix}ExplicitLibsModule${lib-suffix}"/>
          </libs>
        </module>

        <module name="DefaultLibsModule">
          <includedirs>
            ${package.ExampleLibraries.dir}/include
          </includedirs>
                        
          <!-- Empty 'libs' element tells the task to add default library name based on the module name. -->
          <libs/>
        </module>
                      
        <!-- build type attribute implies fileset exports should be automatically deduced, for Library type
        Framework will automatically declare libs (similar to empty <libs/>) -->
        <module name="BuildTypeModule" buildtype="Library">
          <includedirs>
            include
          </includedirs>
        </module>

      </runtime>
    </publicdata>
  </project>

```
Arbitrary script can be inserted in the  `<publicdata>` , `<runtime>` ( `<tests>` ,  `<examples>` ),
data tasks


{{% panel theme = "info" header = "Note:" %}}
Arbitrary script can NOT be inserted into  `<module>` element
{{% /panel %}}

```xml

  <project xmlns="schemas/ea/framework3.xsd">
                
    <publicdata packagename="HelloWorldLibrary" >

      <runtime>
                       
        <foreach item="String" in="Hellow Goodbye" property="__modulename">
                      
          <module name="${__modulename}">
            <includedirs>
              include
            </includedirs>
            <libs/>
          </module>
                        
        </foreach>
                      
      </runtime>
    </publicdata>
  </project>

```
### Public dependencies ###

Often a modules interface will be dependent on another module&#39;s interface. For example native module A&#39;s headers might include native module B&#39;s headers.
If a further module C depends on module A it will also need to declare an interface dependency on module B otherwise it will not be able to resolve the
include of module B&#39;s headers from module A&#39;s header.

However this is problematic as module C conceptually only depends on module A. The requirement for a dependency on module B is &quot;leaked&quot; from module A.
If a new version of module A were no longer dependent on module B, module C would be left with an unnecessary dependency declaration on module B.

The &quot;publicdependencies&quot; element of the{{< task "EA.Eaconfig.Structured.ModulePublicData" >}}task
are designed to avoid this problem by allowing modules to declare the module dependencies they impose on their dependents. In the above example module
A would declare a public dependency on module B then module C inherit an interface dependency implicitly. 

### Public build dependencies ###

Public build dependencies are used in a variant of the situation where public dependencies are used. Consider the above example with the C -> A -> B dependency chain. 
If the symbols exposed to C from B's headers via A's headers are declared but not defined until B's compiled files, then B's output (shared libray, static library, or import library) will need to be available to the linker.
In a build graph that is predominantly static libraries the issue does not usually arise as C, A and B are typically transtively propagated to the link of the top level executable. 
However, if C is a module with a binary output (dynamic/shared library or executable), likely the case in a predominantly shared build, then C needs to inherit a *build* dependency rather than an *interface* dependency on B from A.
Public build dependencies are used to do this.

Note: Similar to &lt;auto&gt; dependencies in the .build file, a public build dependency will collapse gracefully to a public dependency in cases with no link step.

###### Public Dependencies Syntax ######

```
1. [Package]
2. [Package]/[Module]
3. [Package]/[group]/[Module]
```
 1. When just [Package] is specified a dependency is declared on all of a package&#39;s public runtime modules or, if no modules are declared on the package<br>itself.
 2. When just [Package] and [Module] is specifed a dependency is declared on a specific module in the runtime group.
 3. When [Package], [group] and [Module] are all specified a dependency is declared on a specific module in a specific group of a package.

###### Public Dependencies Example ######

```xml
<publicdata packagename="PackageA" >
 <runtime>
  <module name="ModuleA">
   <includedirs>
    include
   </includedirs>
   <publicdependencies>
    PackageB
    PacakgeC/ModuleC
    PackageD/test/ModuleDTest
   </publicdependencies>
  </module>
 </runtime>
</publicdata>
```
Note that there is no unstructured equivalent syntax for public dependencies as Framework does not do any additional processing of public dependencies.
Instead the structured task manipulates existing traditional properties.

### Initialize.xml file Properties (traditional syntax) ###

 - [package.[Package Name].defines or package.[Package Name].[Module Name].defines]( {{< ref "reference/packages/initialize.xml-file/propertydefines.md" >}} ) C/C++ defines
 - [package.[Package Name].includedirs or package.[Package Name].[Module Name].includedirs]( {{< ref "reference/packages/initialize.xml-file/propertincludedirs.md" >}} ) - C/C++ include directories
 - [package.[Package Name].libdirs or package.[Package Name].[Module Name].libdirs]( {{< ref "reference/packages/initialize.xml-file/propertylibdirs.md" >}} ) - library directories
 - [package.[Package Name].usingdirs or package.[Package Name].[Module Name].usingdirs]( {{< ref "reference/packages/initialize.xml-file/propertyusingdirs.md" >}} ) - using directories, .Net, MCPP

### Initialize.xml file Filesets ###

 - [package.[Package Name].libs or package.[Package Name].[Module Name].libs]( {{< ref "reference/packages/initialize.xml-file/filesetlibs.md" >}} ) - native libraries
 - [package.[Package Name].dlls or package.[Package Name].[Module Name].dlls]( {{< ref "reference/packages/initialize.xml-file/filesetdlls.md" >}} ) - dynamic libraries
 - [package.[Package Name].assemblies or package.[Package Name].[Module Name].assemblies]( {{< ref "reference/packages/initialize.xml-file/filesetassemblies.md" >}} ) - .Net/MCPP assemblies
 - [package.[Package Name].assetfiles or package.[Package Name].[Module Name].assetfiles]( {{< ref "reference/packages/initialize.xml-file/assetfiles.md" >}} ) - Asset files to add to the application package or deploy

### Android specific ###

 - Fileset [package.[Package Name].java.archives or package.[Package Name].[Module Name].java.archives]( {{< ref "reference/packages/initialize.xml-file/java.archives.md" >}} ) JAR archives. Currently used on android only
 - Fileset [package.[Package Name].java.classes or package.[Package Name].[Module Name].java.classes]( {{< ref "reference/packages/initialize.xml-file/java.classes.md" >}} ) - compiled Java classes
 - Property [package.[Package Name].resourcedir or package.[Package Name].[Module Name].resourcedir]( {{< ref "reference/packages/initialize.xml-file/resourcedir.md" >}} ) - Directory with exported android resources

<a name="InitializeXmlFile_Data"></a>
## Package public data ##

Arbitrary NAnt scripts can be present in the Initialize.xml file, but there is a number of
properties and filesets known to Framework that can be used to expose data needed for the build,
this data describes include directories, libraries, assemblies, etc.

Public data can be described on the package level and on the per-module level.
There is also convenient syntax to describe data for different platforms.

<a name="InitializeXmlFile_Data_Package_Level"></a>
### Package level definitions ###

Package level definitions use following syntax for property or fileset names:

name |Description |
--- |--- |
|  **package. *PackageName* .xxxxx**  |  *PackageName* is the name of dependent package, and xxxxx stands for the type of property or fileset: &quot;includedirs&quot;, &quot;libs&quot;, etc. |

Traditionally public data in Initialize.xml were defined on a per-package basis because older versions of Framework did not support modules.
Thus, public data can be defined all together even when a package has multiple modules, include directories and libraries.

The problem with package level definitions is that a dependency on a single module will take libraries from all modules.
Framework has code that tries to auto-detect which libraries belong to which module, but this is unreliable.
For this reason, it is much better is to define data on a per-module basis.

Note: Similar to 

{{% panel theme = "danger" header = "Important:" %}}
For packages with multiple modules always define data at the module level
{{% /panel %}}
<a name="InitializeXmlFile_Data_Module_Level"></a>
### Module level definitions ###

The same set of properties and filesets can be defined on a per-module basis.
Because information from a dependent package&#39;s build file may not be available when the Initialize.xml is loaded, an additional property
containing a list of modules in dependent packages is required.

name |Description |
--- |--- |
|  **package. *PackageName* .{{< url buildgroup >}}.buildmodules**  | List of module name in the package. |
|  **package. *PackageName* . *ModuleName* .xxxxx**  |  *PackageName* is the name of the dependent package, *ModuleName*  is the name of {{< url modulename >}}in dependent package,<br>and xxxxx stands for the type of property or fileset: &quot;includedirs&quot;, &quot;libs&quot;, etc. |


{{% panel theme = "info" header = "Note:" %}}
If a module level definition for a property or fileset is not found, a package level definition for that property or fileset will be used.

If no module or package level definition for a property or fileset is found then public data will be empty.

Framework automatically adds libraries and assemblies that are built by each module to the public data for this module
{{% /panel %}}
<a name="InitializeXmlFile_Data_KnownFrameworkNames"></a>
### Framework properties and filesets ###

 **Properties** 

 - [---.defines]( {{< ref "reference/packages/initialize.xml-file/propertydefines.md" >}} ) - preprocessor definitions exported by the package/module.
 - [---.includedirs]( {{< ref "reference/packages/initialize.xml-file/propertincludedirs.md" >}} ) - include directories exported by the package/module
 - [---.usingdirs]( {{< ref "reference/packages/initialize.xml-file/propertyusingdirs.md" >}} ) - using directories exported by the package/module
 - [---.libdirs]( {{< ref "reference/packages/initialize.xml-file/propertylibdirs.md" >}} ) - library directories exported by the package/module


{{% panel theme = "info" header = "Note:" %}}
Add every item like define or directory on a separate line
{{% /panel %}}
 **Filesets** 

 - [---.libs (libs.external, libs.spu)]( {{< ref "reference/packages/initialize.xml-file/filesetlibs.md" >}} )
 - [---.dlls (dlls.external, dlls.spu)]( {{< ref "reference/packages/initialize.xml-file/filesetdlls.md" >}} )
 - [---.assemblies (assemblies.external)]( {{< ref "reference/packages/initialize.xml-file/filesetassemblies.md" >}} )

 **Android specific** 

 - **package.DependentPackageName.java.classes** - classes exported by dependent package. These classes are copied in &#39;classes&#39;<br>build folder of parent package and can be referenced in Java code of the parent package.
 - **package.DependentPackageName.java.archives** - classes exported by dependent package. These archives are extracted into &#39;classes&#39;<br>build folder of parent package and can be referenced in Java code of the parent package.
 - **package.DependentPackageName.assetfiles-set.${config-system}** or **package.DependentPackageName.assetfiles-set** - list of assetfilesets defined in this intialize.xml file.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>When *assetfiles-set* property is defined, the second definition for single assetfileset given below is ignored.<br>{{% /panel %}}<br><br>{{% panel theme = "info" header = "Note:" %}}<br>Since FileSet class in Framework 3 supports multiple Base Directories the need to have many assetfilesets does not exist anymore..<br>{{% /panel %}}
 - **package.DependentPackageName.assetfiles.${config-system}** or **package.DependentPackageName.assetfiles** - when there is single assetfileset, it can have one of the above names. In this case *assetfiles-set* property is not required.

<a name="InitializeXmlFile_ScriptInit_Task"></a>
## Task ScriptInit ##


{{% panel theme = "warning" header = "Warning:" %}}
*ScriptInit*  task is deprecated. Use structured XML task  [<publicdata> task]( {{< ref "reference/packages/initialize.xml-file/_index.md#InitializeXmlFile_PublicDataTask" >}} )
{{% /panel %}}
ScriptInit was created to simplify the setting of public data in the Initialize.xml file

This task sets includedirs, libdirs and libs on the package level. It does not do any additional initialization or actions.

When called without parameters this task will add a library with a name equivalent to the package name


```xml
{{%include filename="/reference/packages/initialize.xml-file/_index/initialiescripttask_example1.xml" /%}}

```

{{% panel theme = "warning" header = "Warning:" %}}
Do not invoke ScriptInit without parameters when

 - packages don&#39;t produce a library, like Utility
 - Library name differs from the package name
{{% /panel %}}
Using parameters in the ScriptInit task allows for more flexibility


```xml
{{%include filename="/reference/packages/initialize.xml-file/_index/initialiescripttask_example2.xml" /%}}

```

{{% panel theme = "primary" header = "Tip:" %}}
Do not use ScriptInit task for packages with multiple modules. It would be much better to use [module level definitions]( {{< ref "reference/packages/initialize.xml-file/_index.md#InitializeXmlFile_Data_Module_Level" >}} ) .
{{% /panel %}}
## Examples ##

These examples demonstrate plain Framework syntax. It is recommended to use structured XML task [<publicdata> task]( {{< ref "reference/packages/initialize.xml-file/_index.md#InitializeXmlFile_PublicDataTask" >}} ) 

###### Package level definitions ######

```xml
{{%include filename="/reference/packages/initialize.xml-file/_index/packageleveldefinitions.xml" /%}}

```


###### Module level definitions ######

```xml
{{%include filename="/reference/packages/initialize.xml-file/_index/moduleleveldefinitions.xml" /%}}

```
