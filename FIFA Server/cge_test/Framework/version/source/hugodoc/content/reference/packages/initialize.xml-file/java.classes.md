---
title: Java.classes
weight: 82
---

This element is used to set Java class files (compiled .java files) exported by this package. This is currently used on Android only.

## Usage ##

Set filesets

 - `package. *PackageName* . *ModuleName* .java.classes` to set class files at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined.<br><br>For each module that has &#39;.java.classes&#39; fileset must be defined.<br>{{% /panel %}}
 - or filesets `package. *PackageName* .java.classes` to set class files at the package level.


{{% panel theme = "info" header = "Note:" %}}
class files are transitively propagated
{{% /panel %}}
## Example ##


```xml
{{%include filename="/reference/packages/initialize.xml-file/java.classes/javaclassfiles.xml" /%}}

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
