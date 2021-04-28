---
title: Packages and Modules
weight: 9
---

In addition to being a build system Framework is a packaging system.
It provides a structured for allowing users to modularize their code into packages and modules
and handles dependencies between them. This page outlines what Framework packages and modules are
and by the end readers should be able to start creating their own packages.

<a name="WhatIsAPackage"></a>
## What is a Package? ##

A Framework package is a directory of files that has a specific structure, so that framework can find it and knows how to use it.
It can be easily distributed among users because it does not need to exist at a fixed location on any users machine so long as Framework can find it.

The structure of a package is as follows:

 - A root folder that matches the name of the package
 - A folder within the root that matches the package&#39;s version.<br>This folder is considered the root of the package and all of the files within the package are within this folder.<br>(This is recommended but you may encounter cases where the version folder is omitted,<br>this is called a flattened package and the rules surrounding these are described in more detail in other parts of the docs)
 - A Manifest.xml file
 - A build script file named (package name).build
 - An Initialize.xml file, located in either a folder called scripts or next to the build file

The contents of these files will be described in more detail further down the page.

This is an example of a package in perforce. Notice that it has all of requirements described above.

![Sample Package Structure]( samplepackagestructure.png )<a name="Manifest"></a>
## Manifest.xml File ##

All packages require a Manifest.xml file. This file describes Metadata about the package,
such as its name, version, who owns the package, a short description of what is in the package.
It can also be used to declare any compatibility constraints on other packages,
such as specific minimum versions of other packages need to be used or else trigger a Framework failure.

Below is an example of what a good starting Manifest file looks like and shows the fields we recommend all Manifest files should have.
The Manifest file format is quite flexible and there are many other optional fields it can have.
If you would like to read about all of the features of the Manifest.xml file our more detailed reference page: [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 

###### An example Manifest File ######

```xml
<?xml version="1.0" encoding="utf-8"?>
<package>
 <!-- Information about who owns the package so that users know who to contact if they have problems -->
 <contactName>Frostbite.Team.CM</contactName>
 <contactEmail>Frostbite.Team.CM@ea.com</contactEmail>

 <!-- A description of the package so users know what it does -->
 <summary>A description of the package</summary>

 <!-- Version contraints on specific packages -->
 <compatibility package="mypackage" revision="3.04.00">
  <dependent package="osx_config" minrevision="1.11.00.1048024" fail="true" message="osx_config-1.11.00 or higher is needed because it defines properties previously defined by the mono package."/>
  <dependent package="Framework" minrevision="5.01.00" fail="true" message="Framework-5.01.00 or later is needed because of some changed interfaces between the two packages." />
 </compatibility>

 <!-- The name an version of the package, this may seem redundant but the version folder might not exist if the package is flattened -->
 <packageName>mypackage</packageName>
 <versionName>3.04.00</versionName>

 <!-- properties that describe the package as buildable and compliant with framework 2+ style -->
 <frameworkVersion>2</frameworkVersion>
 <buildable>true</buildable>
</package>
```
<a name="WhatIsAModule"></a>
## What is a Module? ##

A Framework package may consist of one or more modules.
A module represents a unit of code that is compiled to a single binary file.
For example a module may be a library that compiles to a single dll.

Modules specify where their files (source files, header files, etc.) are located within the package.
They can declare dependencies on other modules either within the package or within other packages.

A module has both a public and a private declaration.
The private declaration tells framework how to build the module and is declared in the build file, which is loaded by framework when building the package.
The public declaration is declared in the initialize.xml and describes the information the module shares with other modules that depend on it. (includedirs, defines, etc.)

<a name="BuildScript"></a>
## Writing a package Build Script ##

In this section we are going to describe how to write a package build script with multiple modules.

This package is going to have two modules, an executable and a static library.
The executable will depend on the library and link with because it is using functions defined in the library.

###### An example Build script ######

```xml
<?xml version="1.0" encoding="utf-8"?>
<project xmlns="schemas/ea/framework3.xsd">

  <package name="mypackage"/>

  <Library name="LibModule">
   <sourcefiles>
    <includes name="${package.dir}/source/lib/*.cpp"/>
   </sourcefiles>
  </Library>

  <Program name="ProgramModule">
   <dependencies>
    <auto>
     MyPackage/LibModule
    </auto>
   </dependencies>
   <sourcefiles>
    <includes name="${package.dir}/source/exe/*.cpp"/>
   </sourcefiles>
  </Program>

</project>
```
In the example above you can see that there are three child Tasks within the root project element.
Framework executes the Tasks in the order they appear.

The first task is the package task. Every package build file starts with a package task.
The package task does a lot of different things but the two main things it does are loading the configuration package and then loading the package&#39;s initialize.xml file.
By loading the configuration package it loads all of the properties, targets and build settings defined in eaconfig and the platform specific config packages. (android_config, capilano_config, etc.)
Loading the initialize.xml in the package task allows any of the information defined for the public declaration of the package to be used in the build script
and helps reduce some of the redundancy.

After the package task we have two module tasks.
The first task has the name Library and the second has the name Program, these names are called build types and describe how the module will be built.
A Library build type will build a static library, a Program build type will build an executable.
There are other build types like DynamicLibrary that are used to build shared libraries (dll).
A complete list of all of the different build types and a more detailed explanation of how they work can be found in other parts of the docs.

Both of the modules have source files and there is a block inside each of the module tasks that describes which source files belong to which module.
The sourcefiles block is known as a FileSet and all FileSets in framework work similarly in the sense that you can include or exclude individual files or groups of files using a wildcard syntax.
Filesets are described in complete detail in other parts of the docs, but this is as much as you need to know at this point.

Lastly in the Program task there is a dependency between the program module and the library module.
The dependency is declared by specifying the name of the package followed by the name of the module.
This is the recommended way but you will often see modules depending on entire packages.
The dependencies here are inside a block called &quot;auto&quot;, this is a specific type of dependency and is the most commonly used
and determines whether to perform a build dependency depending on what type of module the dependency is on.
You can also do a &quot;build&quot; dependency which means that the dependent package needs to be built.
Another common kind of dependency is an &quot;interface&quot; dependency which means that you just want to use the include directories and headers of the module you are depending on.
A complete description of the different kinds of dependencies can be found here: [Overview]( {{< ref "reference/packages/build-scripts/dependencies/overview.md" >}} ) 

<a name="Initialize"></a>
## Describing Public Data with an Initialize.xml ##

Above we described the build file which explains to Framework how to build the modules in a package.
However, the build file is not loaded when a module in one package depends on a module in another.
Instead there is a separate file called the Initialize.xml that describes the public data that other packages need to know about such as defines, include directories, the location of built binaries, etc.

###### An example Initialize.xml ######

```xml
<?xml version="1.0" encoding="utf-8"?>
<project xmlns="schemas/ea/framework3.xsd">

 <publicdata name='mypackage'>
  <module name='LibModule' buildtype='Library'>
   <includedirs>
    ${package.mypackage.dir}/include
   </includedirs>
  </module>
 </publicdata>

</project>
```
Above is a simple initialize.xml that describes the Library module in our package.
It lists an include directory that is propagated to any packages that depend on it.

Framework has a concept call Transitive dependency propagation which means that information our LibModule module is not only
propagated to modules that directly depend on it but also to all modules further down the dependency tree.
There are some exceptions to this rule, the important ones that are not transitively propagated are defines and includedirs.
The reason for this is that there are so many defines and includedirs that if they accumulate they eventually lead to issues where
compiler/linker command lines exceed there length limits.
So build script writers need to put thought into which includedirs and defines are really needed by dependencies.

This is just a very brief explanation of what an initialize.xml file is,
you can find a complete description of the format in other parts of the documenation: [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 

<a name="WhenToMakeAPackage"></a>
## When should a unit of code be a Package? ##

Know that you know what a package is and how to create one, you might start to wonder when you should create a package.
How many modules should a package have? When should a big package be split into smaller packages or smaller packages merged into a bigger package?
And unfortunately there is no clear cut rule, it ultimately demends on how you want to distribute your code.
If you have a set of features that you want to distribute independently on another set of features they should probably be separate packages.
If two packages are always used in combination and changes to one always require changes to another then may you should consider making them a single package.

