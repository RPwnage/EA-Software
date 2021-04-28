---
title: Resourcedir
weight: 83
---

This element is used to set Android resource directories exported by this package. This is often used in Android Extension Libraries

## Usage ##

Set property

 - `package. *PackageName* . *ModuleName* .resourcedir` to set Android resource directories at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined<br>when using per module definitions.<br><br>For each module that defines module specific resource directories should define property &#39;.resourcedir&#39;.<br>{{% /panel %}}
 - or property `package. *PackageName* .resourcedir` to set resourcedir at the package level.

Every using resourcedir should be on a separate line. Usually there is only one resource directory per module


{{% panel theme = "info" header = "Note:" %}}
using directories are transitively propagated
{{% /panel %}}
## Example ##

###### The resourcedir property can be set in the Initialize.xml file using structured xml ######

```xml
<publicdata packagename="MyPackage">
  <module name="MyModule">
    <resourcedir value="${package.MyDependentPackage.dir}/data/resources"/>
  </module>
</publicdata>
```
###### An example of setting the resourcedir ######

```xml
{{%include filename="/reference/packages/initialize.xml-file/resourcedir/resourcedir.xml" /%}}

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
-  [Android Extension Libraries]( {{< ref "reference/platform-specific-topics/android/androidextensionlibraries.md" >}} ) 
