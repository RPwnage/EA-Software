---
title: assetdependencies
weight: 116
---

This page describes how to use asset dependencies.

<a name="buildscript"></a>
## Defining Asset Dependencies in Build Script ##

To setup an asset dependency you must first define which packages a module will be depending on.
This can be done by using the SXML `<assetdependencies>`  element or a property {{< url groupname >}} `.assetdependencies` in your build script.
The value of this property should be set to a list of names of dependent packages.

###### Helloworld.build with an asset dependency on 3 packages ######

```xml

   . . .
       <Program name="Hello">
         <sourcefiles>
           <includes name="source/hello/*.cs" />
         </sourcefiles>
                
         <dependencies>
  <auto>
    SimpleTestLib1
    SimpleTestLib2
    SimpleTestLib3
    SxmlTestLib1
  </auto>
</dependencies>
                
         <buildsteps>
           <packaging deployassets="true">
             <assetfiles>
               <includes name="Assets/Images/*.jpg"/>
             </assetfiles>
             <assetdependencies>
               SimpleTestLib1
               SimpleTestLib2
               SimpleTestLib3/TestLib3
               SxmlTestLib1
             </assetdependencies>
           </packaging>
         </buildsteps>
       </Program>
   . . .
   
```
<a name="structured"></a>
## Declaring Asset Files in Structured XML ##

If you are writing your Initialize.xml file using Structured XML then you need to be aware that asset file dependencies
behave differently in Structured XML and using a mix of Structured and Non-Structured may produce unexpected results.

The most important difference is that all asset files in Structured XML are associated with a module, there are
no package level asset files. This also means that if you declare the asset dependency in your build file against the whole
package it will include all module asset files. (In non-structured XML it would ignore all module asset files and only use a package level asset fileset)

Also there is no SXML fileset that directly mimics the behavior of assetfiles-set filesets, although you can use &#39;fromfileset&#39; attributes to include one fileset in another.

###### Structured XML Initialize.xml from SxmlTestLib1 ######

```xml

. . .
<publicdata packagename="SxmlTestLib1">
  <runtime>
    <module name="TestLib1">
      <assetfiles basedir="${package.SxmlTestLib1.dir}">
        <includes name="${package.SxmlTestLib1.dir}/assets/sxml-testlib1-data/*.txt"/>
      </assetfiles>
    </module>
  </runtime>
</publicdata>
. . .
          
```
<a name="original"></a>
## Declaring Asset Files in Non-Structured XML ##


{{% panel theme = "warning" header = "Warning:" %}}
Everything after this point describes setting up asset dependencies when the Initialize.xml file is written without any Structured XML.
The behavior of asset dependencies is different between structured and non-structured and mixing the two may cause unexpected results.
{{% /panel %}}
Once a module declares an asset dependency it will look at the dependent package&#39;s initialize.xml file for specific filesets and properties describing the dependent&#39;s exposed asset files.
Use the property `package.packagename.assetfiles` to declare all of the asset files for this package.

###### Initialize.xml of SimpleTestLib1 ######

```xml

. . .
<fileset name="package.SimpleTestLib1.assetfiles" basedir="${package.SimpleTestLib1.dir}">
  <includes name="${package.SimpleTestLib1.dir}/assets/testlib1-data/*.txt"/>
</fileset>
. . .
          
```
It is also possible to declare asset dependencies on specific modules of a dependent package.
Notice in the build script example we declared an asset dependency on SimpleTestLib3/TestLib3.
If an asset dependency is against a specific module then the build will look for properties like `package.packagename.modulename.assetfiles` .

If no module specific assetfiles filesets can be found it will look for a package level fileset,
however, the package level fileset will be ignore if module specific filesets exist.
Also, if the asset dependency is not explicitly defined for a specific module, then only the
package level fileset will be used and all module level filesets will be ignored.

###### Initialize.xml of SimpleTestLib3 ######

```xml

. . .
<!-- package level : used if dependency is on package (ex. SimpleTestLib3) or if no module level filesets exist -->
<fileset name="package.SimpleTestLib3.assetfiles" basedir="${package.SimpleTestLib3.dir}">
  <includes name="${package.SimpleTestLib3.dir}/assets/testlib3-package-data/*.txt"/>
</fileset>

<!-- module level : used if there is a dependency on this specific module (ex. SimpleTestLib3/TestLib3)-->
<fileset name="package.SimpleTestLib3.TestLib3.assetfiles" basedir="${package.SimpleTestLib3.dir}">
  <includes name="${package.SimpleTestLib3.dir}/assets/testlib3-module-data/*.txt"/>
</fileset>
. . .
          
```
Another way for a dependent package to expose its assetfiles is using property `package.packagename.assetfiles-set` to list the names of filesets to include as assetfile filesets.
You can also do this with filesets for specific modules by using `package.packagename.modulename.assetfiles-set` .

###### Initialize.xml of SimpleTestLib2 ######

```xml

. . .
<property name="package.SimpleTestLib2.assetfiles-set" value="SimpleTestLib2-assetfiles fileset-of-stuff"/>
  
<fileset name="SimpleTestLib2-assetfiles" basedir="${package.SimpleTestLib2.dir}">
  <includes name="${package.SimpleTestLib2.dir}/assets/testlib2-data/*.txt"/>
</fileset>

<fileset name="fileset-of-stuff" basedir="${package.SimpleTestLib2.dir}">
  <includes name="${package.SimpleTestLib2.dir}/assets/stuff/*.txt"/>
</fileset>
. . .
          
```
All of these properties are also designed to work with the suffix &quot;.${config-system}&quot; as one way of controlling which system these asset files are associated with.
For example, `package.packagename.modulename.assetfiles.pc` .


##### Related Topics: #####
-  [assetfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/assetfiles.md" >}} ) 
