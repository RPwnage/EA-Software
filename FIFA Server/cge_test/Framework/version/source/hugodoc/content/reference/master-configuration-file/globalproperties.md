---
title: globalproperties
weight: 253
---

Global Properties definition, usage and examples 

<a name="Overvies"></a>
## Global Properties Overview ##

<a name="SubSection1"></a>
### What is a Global Property? ###

For each configuration for each {{< url package >}}in the build NAnt creates a separate context called Project.
Package build script is loaded into this context, and{{< url ConfigurationPackage >}} is loaded by a  `<package>` task
in the package build script. As result each package has it&#39;s own data elements like{{< task "NAnt.Core.Tasks.PropertyTask" >}},{{< task "NAnt.Core.Tasks.FileSetTask" >}},
and{{< task "NAnt.Core.Tasks.OptionSetTask" >}}.
No data created in one package Project are shared with other package Projects except for Global Properties.

Global Properties are normal NAnt Properties, there is no separate &quot;global property&quot; type.
When properties are declared as global in masterconfig Framework adds these property names into a separate table. NAnt&#39;s{{< task "NAnt.Core.Tasks.NAntTask" >}}task will take all properties in the current package Project
which names appear in the global property table and copy these properties into dependent package context (Project) adding readonly attribute.


{{% panel theme = "info" header = "Note:" %}}
Global properties are not like static objects in C#, they aren&#39;t shared automatically between all Projects (package contexts).
Properties that appear on the global property list are propagated down the dependency chain.
When propagated to dependent packages Global Properties are assigned the readonly attribute.
{{% /panel %}}
 **Regular NAnt properties will only exist in the package in which it was defined.
When a Global Property is defined in a package build script it will only exist in this package and all dependent packages:** 

![Global Properties 1]( globalproperties_1.png ) **Global Properties defined at the nant command line will propagate from top level package to all dependent packages, but regular NAnt properties defined at the command line will only be defined in the top package:** 

![Global Properties 2]( globalproperties_2.png )
{{% panel theme = "warning" header = "Warning:" %}}
Nant ignores attempts to overwrite read-only properties. In **&quot;-verbose&quot;** mode each overwrite attempt will generate a warning message.
{{% /panel %}}
<a name="DefaultGlobalProperties"></a>
### Default Global Properties ###

Several property names are Global by default, i.e. they are always propagated to dependent packages as readonly properties:

##### config #####
current build configuration (see {{< url config >}}).

##### bulkbuild #####
turns on[off] [bulkbuild]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/_index.md" >}} ) functionality

##### nant.project.masterconfigfile #####
location of the masterconfig file used in the build

##### nant.project.buildroot #####
Full path of the buildroot used in the current build

##### nant.project.temproot #####
Special directory under the buildroot where Framework puts temporary files

##### bulkbuild.always #####
Property allows users to force subsets of packages or modules to always be bulkbuilt (see [bulkbuild.always property]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuildalwaysproperty.md" >}} ) ).

##### bulkbuild.excluded #####
Property allows writers to exclude subsets of packages or modules from bulkbuild (see [bulkbuild.excluded property]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuildexcludedproperty.md" >}} ) ).

##### autobuilduse #####
Turns on/off auto dependencies (see [Overview]( {{< ref "reference/packages/build-scripts/dependencies/overview.md" >}} ) ).

##### nant.transitive #####
Turns on/off transitive dependency propagation (see [nant.transitive]( {{< ref "reference/properties/nant.transitive_property.md" >}} ) ).

##### eaconfig.nantbuild #####


##### cmdtargets.name #####
Names of all targets on the nant command line

##### nant.commandline.properties #####
This is an **optionset** containing all properties specified on the nant command line.

##### nant.project.packageroots #####
Property containing all package root paths (new line separated).

<a name="Usage"></a>
## Usage ##

Masterconfig file  `<globalproperties>` element can be used to declare a property as global, i.e. add a property name to the global properties table.
The global property can also be defined, which means it is declared and an instance of the property is created.

<a name="UsageDeclareGlobalProperty"></a>
### Declaring Global Properties ###

To declare a property as global, list property names as text within the  `<globalproperties>` element:


```xml
.           <globalproperties>
.
.             <!--
.               properties MyProperty1 and MyProperty2 will be propagated by nant task
.               when this properties exist in the calling context
.             -->
.
.             MyProperty1
.             MyProperty2
.           </globalproperties>
```
<a name="UsageDefineGlobalProperty"></a>
### Define Global Properties ###

Global Properties can be defined in the `<globalproperties>` element.
This means that in addition to declaring a property as global its instance is created right away.
This is equivalent to declaring property global and defining it in the top level package build script.


{{% panel theme = "info" header = "Note:" %}}
Global property is first created as writable, it gets &#39;readonly&#39; attribute when propagated to dependent project by{{< task "NAnt.Core.Tasks.NAntTask" >}}task.
{{% /panel %}}
To define a Global property in the `<globalproperties>` element assign a value to it:

 `<globalproperty>` element can be used to define global property. It may be useful when
global property value contains spaces, new line, quote symbols. ** `name` ** is a required attribute.


```xml
.           <globalproperties>
.
.             MyProperty1=true
.
..             MyProperty2="Property value with spaces"
.
..             MyProperty3=
.
.             <globalproperty name="MyProperty3">
.                    Property value with spaces
.                    and multiple lines
.             </globalproperty>
.
.           </globalproperties>
```
<a name="UsageConditionalGlobalProperties"></a>
### Conditional Definitions for Global Properties ###

Global Properties can be declared or defined conditionally. Conditions can contain regular nant expressions and are evaluated
every time Framework accesses global property definitions (in{{< task "NAnt.Core.Tasks.NAntTask" >}}task or{{< task "EA.FrameworkTasks.DependentTask" >}}task).

To define conditions use `<if/>`  element, or  ** `condition` **  attribute in the  `<globalproperty/>` element:


{{% panel theme = "info" header = "Note:" %}}
- Conditions can be nested.
 - If `<globalproperties/>` element contains multiple definitions of the same global property<br>the last definitions will be used. Global properties are processed in order they appear in masterconfig file.
 - Global properties defined on the command line (by  `-D:`  or  `-G:` ) override definitions in masterconfig including conditions.
{{% /panel %}}

```xml

<globalproperties>
   MyProperty1
   <if condition="${my_condition}==true">
     MyProperty2
     MyProperty1="Value set_when_condition is true"
     condition_1_global_prop_init="Test_init_value_cond1"
     <if condition="${my_other_condition}==true">
       MyProperty3
     </if>
   </if>
   <globalproperty name="MyProperty3">
     Value1
     Value2
   </globalproperty>

   <globalproperty name="MyProperty4" condition="${my_other_condition}==true">
     Some Value
   </globalproperty>

 </globalproperties>

```

{{% panel theme = "info" header = "Note:" %}}
Global properties with conditions that contain properties defined in the configuration package (like ` **config-system** ` ) can be used in build scripts after  `<package/>` element.

Accessing such global properties in a build script before `<package/>` task is executed may result in invalid or undefined values of such properties.
{{% /panel %}}
For example, following exception definition:


```xml

<globalproperties>
  MyProperty=PLATFORM_UNDEFINED
  <if condition="${config-system}==pc">
    MyProperty=PLATFORM_PC
  </if>
  <if condition="${config-system}==pc64">
    MyProperty=PLATFORM_PC64
  </if>
</globalproperties>
  
```
And following build script:


```xml

<project default="build" xmlns="schemas/ea/framework3.xsd">

  <echo>
    Before 'package' task MyProperty='${MyProperty}'
  </echo>

  <package name="HelloWorldLibrary"/>

  <echo>
    After 'package' task MyProperty='${MyProperty}'
  </echo>

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
Will produce output:


```
nant slnruntime -configs:"pc-vc-dev-debug pc64-vc-dev-debug"

[echo]
Before 'package' task MyProperty='foo'

[package] HelloWorldLibrary-1.00.00 (pc-vc-dev-debug)  slnruntime

[echo]
After 'package' task MyProperty='PC'

1>        [echo]

Before 'package' task MyProperty='PC'

1>        [package] HelloWorldLibrary-1.00.00 (pc64-vc-dev-debug)  load-package-generate

1>        [echo]
After 'package' task MyProperty='PC64
```
