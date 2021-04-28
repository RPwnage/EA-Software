---
title: includedirs
weight: 74
---

This element is used to set include directories exported by this package.

## Usage ##

Set property

 - `package. *PackageName* . *ModuleName* .includedirs` to set include directories at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined<br>when using per module definitions.<br><br>For each module that defines module specific include directories should define property &#39;.includedirs&#39;.<br>{{% /panel %}}
 - or property `package. *PackageName* .includedirs` to set includedirectories at the package level.

Every include directory should be on a separate line


{{% panel theme = "info" header = "Note:" %}}
include directories are not transitively propagated
{{% /panel %}}
## Example ##


```xml

  <project xmlns="schemas/ea/framework3.xsd">

    <publicdata packagename="HelloWorldLibrary" >
    
        <module name="Hello">
          <includedirs>
            ${package.HelloWorldLibrary.dir}/include
          </includedirs>
          . . .
        </module>

        <module name="Goodbye" >
          <!-- relative paths are prepended with package directory -->
          <includedirs>
            include
          </includedirs>
        
          <!-- NOTE: empty element <includedirs> will result in default include directories added-->
          . . . .
        </module>
    </publicdata>

  </project>

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
