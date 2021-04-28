---
title: What Is A Framework Package?
weight: 39
---

This topics explains what is a Framework Package and typical usage scenarios

<a name="WhatIsFrameworkPackage"></a>
## What Is Framework Package? ##

The simplest definition of a Framework Package is a directory with{{< url ManifestFile >}}file.
There is also a requirement on the folder structure:

Package should be located in the folder with name equal to the
Package Name and subfolder with name equal to the Package version, names are case sensitive, even if underlying file system is not.


{{% panel theme = "danger" header = "Important:" %}}
The folder name specifies the Package&#39;s name and is restricted to the following characters:

 - alpha-numeric characters
 - underscore character&#39;_&#39;

Package names are case-sensitive.
{{% /panel %}}
<a name="MinimalPackageContent"></a>
## Minimal Package content ##

Minimal structure of a Framework Package containing &quot;My Files.txt&quot; file![Minimal Package]( minimalpackage.jpg )How Package is useful in context of Framework?

 - A Framework script can access package content by executing `dependent` task.<br><br><br>```<br><br>    <dependent name="MyPackage"/><br><br>```<br> `dependent`  task will locate package in the {{< url PackageRoots >}}or autodownload<br>this package from{{< url PackageServer >}}.
 - NAnt property `package.MyPackage.dir` with value of the package directory path is set.<br>The property can then be used in the script to access and manipulate files in the package folder.
 - Packages can have much more complex interface to communicate with Framework to do code builds as described below in the next sections.

<a name="FrameworkPackageElements"></a>
## Framework Package Elements ##

Framework Package consists of a collection of files grouped into directory as well as a .build file and Manifest.xml file,
and Framework Packages usually also contain an Initialize.xml file in a subdirectory namedscripts.
There are conventions and good practices in how to name various subdirectories, where to place source files,
include files, binary etc.

A typical Framework Package will look like this![Simple Package Folder Layout]( simplepackagefolderlayout.png )Framework Package can contain following elements, where at least Manifest.xml orPackage Name.build file is required
for arbitrary directory to be considered a package.

 - ** [Manifest.xml]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) ** file -<br>defines meta information about the Package
 - ** [.build]( {{< ref "reference/packages/build-scripts/_index.md" >}} ) ** file - defines set of data used by Framework to perform a code build or other build action.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>The name of the build file must be same as the name of the Package.<br>{{% /panel %}}
 - **{{< url InitializeXml >}}** file - defines public interface for the Package. During the build process other packages can access data defined in this file,<br>other packages have no visibility into the Package build script.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Initialize.xml must be in subfolder namedscripts<br>{{% /panel %}}
 - Optional{{< url Masterconfig >}}file to define other required for build packages.


{{% panel theme = "info" header = "Note:" %}}
The sub folder name specifies the version number which allows different versions of a Package to co-exist.
Formally the name of the version subfolder can be arbitrary string, but it is strongly advised to follow the following convention accepted at EA:

The version number is comprised of three numeric components in the form ofxx.yy.zzwhich are generally
incremented between versions according to the following guidelines:

 - xx: – The Package contains a major rewrite or architectural change
 - yy: – Changes have been made to interfaces, new features have been added and/or changes have been made to how the code behaves
 - zz: – Bug fixes have been made but there are no changes to interfaces or usage
{{% /panel %}}
<a name="FrameworkPackageDirectory"></a>
## Framework Package Directory ##

TermPackage Directory means  *release*  directory, i.e. ..../package name/versiondirectory

In the  simple_program example above&#39;Package Directory&#39; is C:\packages\simple_program\1.00.00

In the .build script file package directory can be accessed through property `${package.dir}` 

In other packages package directory can be accessed:

 - ###### through property ${package.[package name].dir} after <dependent> task is executed ######<br><br>```xml<br><dependent name="simple_program" ><br><br><echo>simple_program package directory = ${package.simple_program.dir}</echo><br>```
 - ###### Using NAnt function PackageGetMasterDir() or PackageGetMasterDirOrEmpty() ######<br><br>```xml<br><echo>simple_program package directory = &{PackageGetMasterDir('simple_program')}</echo><br>```<br>Execution of &lt;dependent&gt; task is not needed in this case


##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
