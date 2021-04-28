---
title: Java Sourcefiles
weight: 280
---

This page describes the steps required to add Java source files to android packages.

<a name="Section1"></a>
## Overview ##

To include Java files in an android package you need to add them to the{{< url groupname >}} `.java.sourcefiles` fileset. But you must also make sure to add an AndroidManifest.xml file or else the APK file wont be generated.


{{% panel theme = "warning" header = "Warning:" %}}
When no Java source files are included the AndroidManifest file will be generated automatically, however
since it is not generated when Java source files are included, you must include your own copy of the manifest
files. This is important since APK file generation is a post build step that depends on the AndroidManifest file existing.
{{% /panel %}}
## Example ##

Using Structured XML Syntax:


```xml
<Program name="MyModule">
  ...
  <java>
    <sourcefiles basedir="${package.dir}\source">
      <includes name="${package.dir}\source\*.java"/>
      <includes name="${package.dir}\source\**\*.java"/>
    </sourcefiles>
  </java>
  ...
  <buildsteps>
    <packaging packagename="MyPackageName">
      <manifestfile basedir="${package.dir}\resource">
        <includes name="${package.dir}\resource\AndroidManifest.xml"/>
      </manifestfile>
    </packaging>
  </buildsteps>
  ...
</Program>
```
Using Old Style Build Script Specification:


```xml
<fileset name="runtime.MyModule.java.sourcefiles" basedir="${package.dir}\source">
 <includes name="${package.dir}\source\*.java"/>
 <includes name="${package.dir}\source\**\*.java"/>
</fileset>

<fileset name="runtime.MyModule.manifestfile.android" basedir="${package.dir}\resource">
 <includes name="${package.dir}\resource\AndroidManifest.xml"/>
</fileset>
```
