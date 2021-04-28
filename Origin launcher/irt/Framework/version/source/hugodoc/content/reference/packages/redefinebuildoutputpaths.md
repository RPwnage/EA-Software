---
title: Redefine Build Output Paths
weight: 207
---

Framework allows redefinition of standard Framework build folders structure and output file names.

<a name="Overview"></a>
## Overview ##

Framework has rigid structure of the output folders under the build root.
It is possible to redefine some of the standard output locations and/or filenames in the
package build scripts using properties like `[group].[module].outputdir` or options like `linkoutputname` . But in {{< url Framework2 >}}there was no easy way to change globally output structure for all packages
without modifying Initialize.xml files in every package.

{{< url Framework3 >}}has a way to remap output folders and names for all packages in the build. The best place to define such mapping is a game configuration package.

<a name="Usage"></a>
## Usage ##

To set output mapping: define an optionset that can contain following options

 1. define an optionset that can contain following options<br><br>##### configbindir #####<br>Redefine ** `package.configbindir` ** property. This property defines a directory where binaries (usually exe or dll files) go.<br><br>##### configlibdir #####<br>Redefine ** `package.configlibdir` ** property. This property defines a directory where libraries for a given package go.<br><br>##### liboutputname #####<br>Redefine library file name. This is a name without extension.<br><br>##### linkoutputname #####<br>Redefine linker output file name. This is a name without extension.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>All options in the optionset can contain ** `%outputname%` **  template parameter. For each {{< url Module >}}this template parameter is substituted with the value of outputname.<br>Outputname is, by default, set to be equal to the name of the module but can be redefined using property **{{< url groupname >}} `.outputname` **<br>{{% /panel %}}
 2. The name of the optionset should be assigned to the property ** `package.outputname-map-options` ** <br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>It is possible to assign output mapping optionset in the  [Masterconfig grouptype element]( {{< ref "reference/master-configuration-file/masterversions/grouptype.md#OutputnameMapOptions" >}} ) .<br>{{% /panel %}}

<a name="Example"></a>
## Example ##

Following example sets output folders so that all binaries go into a single folder &#39;Bin&#39; and all libraries go into a single folder &#39;Lib&#39;

To prevent binaries and libraries from different configurations to stomp on each other liboutputname and linkoutputname options are set
to give output files unique names


```xml
.       <optionset name="custom-output-names">
.         <option name ="configbindir" value="${nant.project.buildroot}\Bin"/>
.         <option name ="configlibdir" value="${nant.project.buildroot}\Lib"/>
.         <option name ="liboutputname" value="%outputname%-${config}"/>
.         <option name ="linkoutputname" value="%outputname%${config}"/>
.       </optionset>

.       <property name="package.outputname-map-options" value="custom-output-names"/>
```
