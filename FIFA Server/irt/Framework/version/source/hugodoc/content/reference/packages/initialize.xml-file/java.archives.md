---
title: Java.archives
weight: 81
---

These element is used to set Java archive (.jar) files exported by this package. Use java.build-only-archives for archives that should not be include in .apk file. This is currently used on Android only.

## Usage ##

Set filesets

 - `package. *PackageName* . *ModuleName* .java.archives` to set JAR files at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined.<br><br>For each module that has &#39;.java.archives&#39; fileset must be defined.<br>{{% /panel %}}
 - or fileset `package. *PackageName* .java.archives` to set JAR files at the package level.
 - `package. *PackageName* . *ModuleName* .java.build-only-archives` can be used to specify module .jar files that should only be used for compilation
 - the fileset `package. *PackageName* .java.build-only-archives` can be used if you wish to specify these files at package level instead

## Example ##

###### Structured XML ######

```xml
{{%include filename="/reference/packages/initialize.xml-file/java.archives/javaarchivefilesstructured.xml" /%}}

```
###### Old Style Syntax ######

```xml
{{%include filename="/reference/packages/initialize.xml-file/java.archives/javaarchivefiles.xml" /%}}

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
