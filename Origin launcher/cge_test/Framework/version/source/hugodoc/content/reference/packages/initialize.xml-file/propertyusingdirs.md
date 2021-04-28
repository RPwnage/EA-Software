---
title: usingdirs
weight: 76
---

This element is used to set using directories exported by this package.

## Usage ##

Set property

 - `package. *PackageName* . *ModuleName* .usingdirs` to set using directories at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined<br>when using per module definitions.<br><br>For each module that defines module specific using directories should define property &#39;.includedirs&#39;.<br>{{% /panel %}}
 - or property `package. *PackageName* .usingdirs` to set includedirectories at the package level.

Every using directory should be on a separate line


{{% panel theme = "info" header = "Note:" %}}
using directories are transitively propagated
{{% /panel %}}
## Example ##


```xml
{{%include filename="/reference/packages/initialize.xml-file/propertyusingdirs/usingdirs.xml" /%}}

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
