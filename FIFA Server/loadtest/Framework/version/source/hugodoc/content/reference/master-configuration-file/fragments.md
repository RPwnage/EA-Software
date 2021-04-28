---
title: fragments
weight: 256
---

Framework includes support for masterconfig file fragments. Fragments are separate masterconfig files,
information from these files is merged into masterconfig data.

<a name="Usage"></a>
## Usage ##

 ** `<fragments>` ** element can appear under ** `<project xmlns="schemas/ea/framework3.xsd">` ** element.

##### fragments #####
 **Nested elements** 

   **include** - one or more include elements each must contain single attribute `name` unless it also contains exception elements in which case name is optional.<br><br>&#39;include&#39; elements define path to masterconfig fragments<br><br>    **name** - value of this attribute defines a file name pattern to include. Syntax for file name pattern is the same as in{{< task "NAnt.Core.Tasks.FileSetTask" >}}.<br><br>When relative path is specified it is computed relative to the masterconfig file location.

##### exception #####
Multiple exception elements can be defined inside a fragment include. They allow the include path to be changed based on a property value.

 **Attributes:** 

   ** `propertyname` ** (required) - name of the property that is used to evaluate conditions.

 **Nested elements** 

exception can contain one or more conditions

   ** `condition` ** - condition contains two attributes:<br><br>  - ** `value` ** value is compared to the value of property defined by &#39;propertyname&#39; attribute in the &#39;exception&#39; element, and if values match the condition is applied.<br>  - ** `name` ** Include will use this path instead.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Multiple condition elements are allowed in the exception.<br><br>The first condition that evaluates to true is used.<br>{{% /panel %}}

<a name="Fragments"></a>
### Fragments format ###

Masterconfig fragments can contain following elements:

 - [masterversions]( {{< ref "reference/master-configuration-file/masterversions/_index.md" >}} ) - with all valid masterversion nested elements
 - [packageroots]( {{< ref "reference/master-configuration-file/packageroots.md" >}} )
 - [globalproperties]( {{< ref "reference/master-configuration-file/globalproperties.md" >}} )

## Example ##


```xml
.		<project xmlns="schemas/ea/framework3.xsd">
.			<masterversions>
.			  ...
.			</masterversions>
.
.			<fragments>
.
.				<!-- include a masterconfig from a relative path, name is required in this instance -->
.				<include name="./*/masterconfig_fragment.xml"/>
.
.				<!-- choose between to different fragments, alternate_fragment will be used if property "use-alternate" has a value of "true" -->
.			  	<include name="./*/default_fragment.xml">
.					<exception propertyname="use-alternate">
.						<condition value="true" name"./*/alternate_fragment.xml"/>
.					</exception>
.				 </include>
.
.				<!-- optionally include a fragment, name is not required as long as at least one exception is specifed -->
.			  	<include>
.					<exception propertyname="include-optional">
.						<condition value="true" name"./*/optional_fragment.xml"/>
.					</exception>
.				 </include>
.			</fragments>
.		</project>
```
