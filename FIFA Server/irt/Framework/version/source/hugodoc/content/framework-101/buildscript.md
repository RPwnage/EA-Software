---
title: Introduction to Build Scripts
weight: 8
---

Build scripts use XML syntax to describe the things you want Framework to build.

<a name="BuildScripts"></a>
## What is a Build Script? ##

A build script or build file is an XML file with the extension .build or .xml.
All build scripts have a root element named project (&lt;project xmlns=&quot;schemas/ea/framework3.xsd&quot;&gt;).
All of the elements within the root element are known as Tasks.

When Framework runs it looks for a build file in the current directory (or uses the -buildfile: or -buildpackage: arguments to find the build file)
The build file is parsed and then Framework begins executing it line by line.
Every Task is defined in Framework as a C# class derived from the parent class Task which Framework executes by calling the tasks Execute method.

Framework build scripts can be split across multiple files using the include Task. (&lt;include file=&quot;scripts/CustomTargets.xml&quot;/&gt;)

###### An example Build script ######

```xml
<?xml version="1.0" ?>
<project xmlns="schemas/ea/framework3.xsd">

 <!-- If you try to build this script Framework will simply print the message Helloworld -->
 <echo message="Helloworld"/>

</project>
```
<a name="Primitives"></a>
## Build Script Primitives: Properties and Functions ##

Build scripts are based on 6 fundamental primitive types: Tasks, Properties, OptionSets, FileSets, Functions and Targets.

At this point you only need a fairly general understanding of most of these and they will be covered in more detail later on.
The previous section mentioned Tasks, which are XML elements that are executed by Framework.
In this section we will specifically focus on properties and functions.

Properties are key-value pairs that are stored in a large dictionary inside of Framework.
There are many properties that are defined automatically be Framework.
Other properties can be passed in on the command line or specified in the masterconfig file.
Build scripts can also define properties within them using the property task: (&lt;property name=&#39;myprop&#39; value=&#39;5&#39;/&gt;)

At this point it is more important to know how to use properties rather than how to set them.
Properties can be used in any field in a build script that accepts text by using the dollar sign and then surrounding the name of the property in curly braces: ${myprop}
Framework will do a text replace on each of these properties and replace it with the value of the property.
Framework will fail if the property cannot be found, however you can use the ?? operator to define a value or another property to use if the property is not defined.

###### An example of using a property ######

```xml
<!-- prints the value of myproperty or the string not defined if the property is not defined -->
<echo message="The Value of My Property: ${myproperty??not defined}"/>
```
Functions are used a lot like properties however they start with the @ symbol instead of the $ symbol.

###### An example of using a function ######

```xml
<!-- in this example value of the property myproperty is passed into the function StrLen which returns the length of a string -->
<echo message="Number of characters in myproperty: @{StrLen('${myproperty}')}"/>
```
