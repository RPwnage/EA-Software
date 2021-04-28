---
title: assemblies
weight: 79
---

This element is used to set assemblies exported by this package.

## Usage ##

Set filesets

 - `package. *PackageName* . *ModuleName* .assemblies` to set assemblies at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined.<br><br>For each module that has assemblies &#39;.assemblies&#39; fileset must be defined.<br>{{% /panel %}}
 - or filesets `package. *PackageName* .assemblies` to set assemblies at the package level.


{{% panel theme = "info" header = "Note:" %}}
assemblies are transitively propagated
{{% /panel %}}
## Example ##


```xml
{{%include filename="/reference/packages/initialize.xml-file/filesetassemblies/assemblies.xml" /%}}

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
