---
title: libdirs
weight: 75
---

This element is used to set library directories exported by this package.

## Usage ##

Set property

 - `package. *PackageName* . *ModuleName* .libdirs` to set library directories at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined<br>when using per module definitions.<br><br>For each module that defines module specific library directories should define property &#39;.libdirs&#39;.<br>{{% /panel %}}
 - or property `package. *PackageName* .libdirs` to set library directories at the package level.

Every library directories definition should be on a separate line


{{% panel theme = "info" header = "Note:" %}}
library directories are transitively propagated
{{% /panel %}}
## Example ##


```xml
{{%include filename="/reference/packages/initialize.xml-file/propertylibdirs/libdirs.xml" /%}}

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
