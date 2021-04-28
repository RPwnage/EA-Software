---
title: dlls
weight: 78
---

This element is used to set dynamic (shared) libraries exported by this package.

## Usage ##

Set filesets

 - `package. *PackageName* . *ModuleName* .dlls` to set dynamic libraries at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined.<br><br>For each module that has dlls &#39;.dlls&#39; fileset must be defined.<br>{{% /panel %}}
 - or fileset `package. *PackageName* .dlls` to set dynamic libraries at the package level.

Every library path should be on a separate line


{{% panel theme = "info" header = "Note:" %}}
dynamic libraries are transitively propagated
{{% /panel %}}
## Example ##


```xml
{{%include filename="/reference/packages/initialize.xml-file/filesetdlls/dlls.xml" /%}}

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
