---
title: assetfiles
weight: 80
---

This element is used to set assetfiles exported by this package. Asset files are used to create application package

## Usage ##

Set filesets

 - `package. *PackageName* . *ModuleName* .assetfiles` to set asset files at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined.<br><br>For each module that has &#39;.assetfiles&#39; fileset must be defined.<br>{{% /panel %}}
 - or filesets `package. *PackageName* .assetfiles` to set assetfiles at the package level.


{{% panel theme = "danger" header = "Important:" %}}
You need to define &#39;assetdependendencies&#39; in the application module for assetfiles fileset to be used.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
assetfiles are NOT transitively propagated
{{% /panel %}}
## Example ##


```xml
{{%include filename="/reference/packages/initialize.xml-file/assetfiles/assetfiles.xml" /%}}

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
