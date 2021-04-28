---
title: packageroots
weight: 246
---

Package Root folders are locations that Framework will scan to autodiscover Package(s) and Releases.

<a name="Usage"></a>
## Usage ##

 ** `<packageroots>` ** element in masterconfig file can contain an arbitrary number of ** `<packageroot>` elements.** 


{{% panel theme = "info" header = "Note:" %}}
Framework package location is automatically added to the end of packageroots list if it is not explicitly listed in the  `<packageroots>` .

 `<ondemandroot>`  is automatically added to packageroots list if it is not explicitly listed in the  `<packageroots>` .
{{% /panel %}}
##### packageroots #####
 **Nested elements** 

 - ** `packageroot` ** - each package root defines one package root directory as an inner text.

 **Packageroot element** 

##### packageroot #####
Inner text of the &#39;package&#39; root must contain one package root directory.

 - `<packageroot>` can contain full or relative path
 - When relative path is specified it is computed relative to the masterconfig file location.

 **Nested elements** 

 - ** [exception]( {{< ref "#packagerootException" >}} ) ** - exception element allows to redefine package root directory value based on conditions<br>(see [PackageRoot Exception]( {{< ref "#packagerootException" >}} ) ).<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Multiple exception elements are allowed.<br>{{% /panel %}}

## Example ##


```xml
.        <project xmlns="schemas/ea/framework3.xsd">
.         . . . . . . . .
.         <packageroots>
.           <packageroot>F:\packages</packageroot>
.           <packageroot>${nant.location}\..\..\..</packageroot>
.           <packageroot>${nant.project.basedir}..\..\..</packageroot>
.           <packageroot>${nant.project.basedir}..\..</packageroot>
.            <packageroot>..\..\</packageroot>
.         </packageroots>
.
.        </project>
```
<a name="packagerootException"></a>
## PackageRoot Exception ##

 ** `<exception>` **  element can appear under  ** `<packageroot>` ** element.
Exception element defines property name that will be used in conditions.

##### exception #####
 **Attributes:** 

   ** `propertyname` ** (required)- name of the property that is used to evaluate conditions.

 **Nested elements** 

exception can contain one or more conditions

   ** `condition` ** - condition contains two attributes:<br><br>  - ** `value` ** value is compared to the value of property defined by &#39;propertyname&#39; attribute in the &#39;exception&#39; element, and if values match the condition is applied.<br>  - ** `dir` ** New value for package root directory<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Multiple condition elements are allowed in the exception. The first condition that evaluates to true is used.<br>{{% /panel %}}

## Example ##


```xml
.         <packageroots>
.             <packageroot>
.               F:\packages
.               <exception propertyname="use-my-work-packages">
.                 <condition value="true" dir="./packages"/>
.               </exception>
.           </packageroot>
.         </packageroots>
```
