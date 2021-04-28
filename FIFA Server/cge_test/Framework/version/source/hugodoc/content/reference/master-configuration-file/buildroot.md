---
title: buildroot
weight: 247
---

This element is used to specify the directory that will contain all build output.

<a name="Usage"></a>
## Usage ##

Enter the element in the Masterconfig file and specify directory path as an inner text.

 - If `<buildroot>`  is not specified, default value is  ** *build* ** .
 - `<buildroot>` can contain full or relative path
 - When relative path is specified it is computed relative to the masterconfig file location.

 **Nested elements** 

 - ** [exception]( {{< ref "#BuildrootException" >}} ) ** - exception element allows writers to redefine the build root directory based on conditions<br>(see [BuildRoot Exception]( {{< ref "#BuildrootException" >}} ) ).<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Multiple exception elements are allowed.<br>{{% /panel %}}

## Example ##


```xml
.        <project xmlns="schemas/ea/framework3.xsd">

.            <masterversions>
.              . . . . . .
.            </masterversions>
.
.            <buildroot>.\MyBuild</buildroot>
.
.        </project>
```
<a name="BuildrootException"></a>
## BuildRoot Exception ##

 ** `<exception>` **  element can appear under  ** `<packageroot>` ** element.
Exception element defines property name that will be used in conditions.

##### exception #####
 **Attributes:** 

   ** `propertyname` ** (required)- name of the property that is used to evaluate conditions.

 **Nested elements** 

exception can contain one or more conditions

   ** `condition` ** - condition contains two attributes:<br><br>  - ** `value` ** value is compared to the value of property defined by &#39;propertyname&#39; attribute in the &#39;exception&#39; element, and if values match the condition is applied.<br>  - ** `buildroot` ** New value for the buildroot directory<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Multiple condition elements are allowed in the exception. The first condition that evaluates to true is used.<br>{{% /panel %}}

## Example ##


```xml
.           <buildroot>
.             build
.             <exception propertyname="config-system">
.               <condition value="pc" buildroot="pc_build"/>
.             </exception>
.           </buildroot>
```

##### Related Topics: #####
-  [Redefine Build Output Paths]( {{< ref "reference/packages/redefinebuildoutputpaths.md" >}} ) 
-  [Redefine Visual Studio Solution File Names]( {{< ref "reference/visualstudio/redefine-visual-studio-solution-file-names.md" >}} ) 
