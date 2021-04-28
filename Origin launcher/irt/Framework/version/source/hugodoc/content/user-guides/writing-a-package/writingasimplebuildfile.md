---
title: 1 - Writing A Simple Build File
weight: 15
---
<a name="SimpleBuildFile"></a>

A build file is an XML document that is executed like a linear script. A very simple build file could look like the example below:


```xml
<project xmlns="schemas/ea/framework3.xsd">
 <echo message="Hello world!"/>
</project>
```
Full details of build scripts can be found  [here]( {{< ref "reference/packages/build-scripts/_index.md" >}} ) . This build script can be run by invoking nant in the directory where the .build file is
located or by invoking nant with the -buildfile: argument and specifying a path to the build file.


```
[doc-demo] C:\SimpleBuildFile>nant -buildfile:SimpleBuildFile.build
          [echo--] Hello world!
           [nant] No targets specified.
           [nant] BUILD SUCCEEDED (00:00:00)

[doc-demo] C:\SimpleBuildFile>
```
A build file on it&#39;s own like this isn&#39;t that useful for building but can be handy for experimenting with nant syntax or testing small
snippets of code. In order to get the most out of Framework we need to make the build script [part of a package]( {{< ref "user-guides/writing-a-package/writingasimplepackage.md" >}} ) .

