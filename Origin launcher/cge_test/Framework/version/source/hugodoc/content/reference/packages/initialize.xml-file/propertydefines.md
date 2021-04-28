---
title: defines
weight: 73
---

This element is used to set preprocessor defines exported by this package.

## Usage ##

Set property

 - `package. *PackageName* . *ModuleName* .defines` to set defines at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined<br>when using per module definitions.<br><br>For each module that defines module specific defines  should define property &#39;.defines&#39;.<br>{{% /panel %}}
 - or property `package. *PackageName* .defines` to set defines at the package level.

Every preprocessor definition should be on a separate line


{{% panel theme = "info" header = "Note:" %}}
defines are not transitively propagated
{{% /panel %}}
## Example ##


```xml

<project xmlns="schemas/ea/framework3.xsd">

  <publicdata packagename="MyPackage" >

    <module name="MyModule">
      <defines>
        MY_DEFINE=1
        <do if="${config-system}==pc64">
          MY_DEFINE_PC
        </do>
      </defines>
      . . . .
    </module>
  </publicdata>

</project>

```

##### Related Topics: #####
-  [Everything you wanted to know about Framework defines, but were afraid to ask]( {{< ref "user-guides/everythingaboutdefines.md" >}} ) 
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
