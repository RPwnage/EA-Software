---
title: libs
weight: 77
---

This element is used to set libraries exported by this package.

## Usage ##

Set filesets

 - `package. *PackageName* . *ModuleName* .libs` to set libraries at the module level<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Property containing list of modules `package.[PackageName].buildmodules` must be defined.<br><br>For each module that has libraries &#39;.libs&#39; fileset must be defined.<br>{{% /panel %}}
 - or fileset `package. *PackageName* .libs` to set libraries at the package level.


{{% panel theme = "info" header = "Note:" %}}
libraries are transitively propagated
{{% /panel %}}
## Overview ##

The libs fileset is used to indicate what libs are exported by a package.
Sometimes Framework has enough information to know what libs are exported but there are a few situations, that users often encounter, where it does not.
The following paragraphs explain these situations and how to avoid them.
We have also been updating Framework with warnings when it detects that one of these situations may have occured, although some of these are only visible in verbose mode.

Often situations arise where there are two packages, package A and a library B that it depends on.
In this case the Initialize.xml file for B will usually have a lib fileset that includes its output lib file.
(usually this looks like, *${package.B.libdir}/${lib-prefix}B${lib-suffix}* )

Normally both of these packages will be built and package A will know about the B lib file implicitly if it has a link dependency on B.
However, if B is prebuilt, A won&#39;t know about it implicitly and will fail unless B declares its lib file explicitly.
This problem often happens when users switch their builds to using prebuilt libs and don&#39;t realize the lib needs to be added to the libs fileset.

Another important case is where package B has multiple modules, and package A has a constraint to depend on only a single module from B.
    Originally Initialize.xml files were designed without the concept of modules.
So this was handled by adding libs for each module to the libs fileset and then Framework would be able to detect by name which libs where package, module or other libs.
However, this method is not always reliable since the name of the lib file can be changed so that it does not match the name of the module or package.
Instead it is recommended that build script writers use public data with modules to specify the libs properly for all of their modules to avoid these errors.

## Example ##


```xml

  <project xmlns="schemas/ea/framework3.xsd">

    <publicdata packagename="HelloWorldLibrary" >
    
        <module name="Hello">
          <!-- Empty 'libs' element tells the task to add default library name based on the module name. -->
          <libs/>
          . . .
        </module>

        <module name="Goodbye" >
          <libs>
            <includes name="${package.HelloWorldLibrary.libdir}/${lib-prefix}Goodbye${lib-suffix}"/>
          </libs>
          <libs-external>
            <includes name="${package.HelloWorldLibrary.libdir}/${lib-prefix}PrebuiltLib${lib-suffix}"/>
          </libs-external>          
          . . .
        </module>
    </publicdata>

  </project>

```

##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
