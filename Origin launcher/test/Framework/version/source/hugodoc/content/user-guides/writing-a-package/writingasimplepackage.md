---
title: 2 - Writing A Simple Package
weight: 16
---
<a name="SimpleBuildFile"></a>

To write a simple package requires three files: a  [.build file]( {{< ref "reference/packages/build-scripts/_index.md" >}} ) , [a Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) and a [masterconfig file]( {{< ref "reference/packages/build-scripts/_index.md" >}} ) . Note not every package requires it&#39;s own masterconfig,
but you need to specify one in order to work with packages. Framework requires that they .build and Manifest.xml be placed in a folder with
the same name as the package (or nested one folder lower - more on this later).

A package .build file contains a call to the &lt;package&gt; task at or near the top. This task initializes the configuration
for the package and sets up data that can be used by all following tasks. This example also shows some properties that are set up
by the &lt;package&gt; task.

```xml
<project xmlns="schemas/ea/framework3.xsd">
 <package name="SimplePackage"/>

 <echo message="Compiler is ${cc}."/>
 <echo message="Librarian is ${lib}."/>
 <echo message="Linker is ${link}."/>
</project>
```
The next file that is needed is the Manifest.xml. There&#39;s no required content for this file other than a &lt;package&gt; root
element but this existence of this is used to indicated a package. It is recommended that you set information fields such as
&lt;contactName&gt; and &lt;contactEmail&gt;.

```xml
<package>
  <contactName>Frostbite.Team.CM</contactName>
  <contactEmail>Frostbite.Team.CM@ea.com</contactEmail>
</package>
```
The masterconfig file controls the versions of all packages that will be used by nant. In the example below a few particular
elements have been set:

 - ** [masterversions]( {{< ref "reference/master-configuration-file/masterversions/_index.md" >}} ) ** - Only one version is being set here. *WindowSDK* is required dependency of PC builds. This package contains all the necessary components without needing SDK to be installed. The latest versions of packages can be found by browsing [package server](http://packages.ea.com) .
 - ** [ondemand]( {{< ref "reference/master-configuration-file/ondemand/_index.md" >}} ) **  and  ** [ondemandroot]( {{< ref "reference/master-configuration-file/ondemandroot.md" >}} ) ** - Combined these elements enable nant&#39;s ability to acquire packages that aren&#39;t<br>found locally. There are several methods of automatic package acquistion but by default a direct download from package server<br>is used.
 - ** [config]( {{< ref "reference/master-configuration-file/config.md" >}} ) ** - This sets the configuration package to use for the build (other attributes documented elsewhere). Nearly all modern<br>uses of nant use eaconfig.


```
<project xmlns="schemas/ea/framework3.xsd">
 <masterversions>
  <package name="WindowsSDK" version="10.0.16299"/>
 </masterversions>
	
 <ondemand>true</ondemand>
 <ondemandroot>c:\packages</ondemandroot>
</project>
```
This package script can be run by invoking nant in the directory where the .build file is located but in order to use the
&gt;package&lt; task and additional parameter is needed: **-configs:** . This parameter sets for which configs the package target
should be run. In this example we&#39;re using *pc64-vc-dev-debug* which is a standard config provided by eaconfig that builds PC
64 bit with the MSVC compiler and uses build options designed to allow maximal debugging (full debug symbols, few optimizations).

```
[doc-demo] C:\SimplePackage>nant -configs:pc64-vc-dev-debug
          [package] SimplePackage-flattened (pc64-vc-dev-debug)
          [echo-SimplePackage-flattened] Compiler is C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.13.26128\bin\HostX64\x64\cl.exe.
          [echo-SimplePackage-flattened] Librarian is C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.13.26128\bin\HostX64\x64\lib.exe.
          [echo-SimplePackage-flattened] Linker is C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.13.26128\bin\HostX64\x64\link.exe.
           [nant] No targets specified.
           [nant] BUILD SUCCEEDED (00:00:00)

[doc-demo] C:\SimplePackage>
```
This give us a skeleton package but doesn&#39;t actually contain any buildable code. To begin defining our code we need to [start adding modules]( {{< ref "user-guides/writing-a-package/writingasimplepackage.md" >}} ) .

