---
title: Android Resource Files
weight: 279
---

This page describes how resource files can be setup for Android packages and some things to keep in mind when working with resource files.

<a name="Section1"></a>
## Overview ##

Resource files on android are used for things like icons, logos, string localization, and style and layout files.
Resource files usually appear in the APK file under the &#39;res&#39; directory, although some xml files,
for example string localization files, are compressed into a file called &#39;resources.arsc&#39; and thus do not appear in the &#39;res&#39; folder.
Also, resource files will not be included unless you specify a custom android manifest file in your build script.

<a name="example"></a>
## Example ##

Below is an example of how to add resource files to a module in a build script using structured xml:


```xml
<Program name="MyModule">
  <sourcefiles basedir="${package.dir}/source">
    <includes name="${package.dir}/source/**.cpp"/>
  </sourcefiles>
  <resourcefiles basedir="${package.dir}/data/resources">
    <includes name="${package.dir}/data/resources/**.xml"/>
  </resourcefiles>
  <buildsteps>
    <packaging packagename="MyPackageName">
      <manifestfile basedir="${package.dir}/data">
        <includes name="${package.dir}/resource/AndroidManifest.xml"/>
      </manifestfile>
    </packaging>
  </buildsteps>
</Program>
```
<a name="dependents"></a>
## Propagating resource files to dependents ##

There may be cases where packages that have resource files may want to pass these to other packages that dependent on them.
This can be done using the &#39;resourcedir&#39; property in the dependent package&#39;s initialize.xml file.
When a package defines a &#39;resourcedir&#39; then any package that depends on it will add this directory to its list of resource directories.
The list of resource directories is then passed to the aapt.exe tool from the AndroidSDK which bundles a bunch of files together to make the temporary .ap_ file.
The .ap_ is then passed to the apkbuilder tool from the AndroidSDK which uses it to create the final .apk file.

###### The resourcedir property can be set in the Initialize.xml file using structured xml ######

```xml
<publicdata packagename="MyPackage">
  <module name="MyModule">
    <resourcedir value="${package.MyDependentPackage.dir}/data/resources"/>
  </module>
</publicdata>
```
###### The resourcedir property can be defined like this in the Initialize.xml file ######

```xml
<property name="package.MyDependentPackage.resourcedir" value="${package.MyDependentPackage.dir}/data/resources"/>
```
###### The resourcedir property can also be qualified by config-system name ######

```xml
<property name="package.MyDependentPackage.resourcedir.android" value="${package.MyDependentPackage.dir}/data/resources"/>
```
