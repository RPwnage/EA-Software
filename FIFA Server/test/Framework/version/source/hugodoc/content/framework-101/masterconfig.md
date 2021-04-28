---
title: The Masterconfig File
weight: 10
---

Know that we have our packages defined there is one more thing need before we can start using Framework to build them.
This page provides an introduction the masterconfig file which Framework uses to locate packages, keep track of package versions and control global build settings.

<a name="WhatIsTheMasterconfigFile"></a>
## What is the Masterconfig file ##

The masterconfig file is the file that tells framework which packages to use and where to find them.
Framework does not require packages to exist in a specific location on the users machine because the user will supply Framework with a masterconfig file that tells it where to find the packages.
The masterconfig file is also the central authority for which versions to use.

###### An example masterconfig file ######

```xml
<?xml version="1.0" encoding="utf-8"?>
<project xmlns="schemas/ea/framework3.xsd">

 <globalproperties>
  myproperty=true
 </globalproperties>

 <masterversions>
  <package name="Framework" version="7.13.00" />

  <package name="WindowsSDK" version="10.0.16299" />
 </masterversions>

 <packageroots>
  <packageroot>.\packages</packageroot>
 </packageroots>

 <buildroot>build</buildroot>
 <ondemand>true</ondemand>
 <ondemandroot>${nant.drive}\packages</ondemandroot>
 <config default="pc64-vc-dll-dev-debug"/>
</project>
```
<a name="MasterVersions"></a>
## The Master Versions List ##

The master versions section is where you list all of your packages and the versions of each package want to use.
Whenever one package depends on another, Framework will check this list to figure out what version to use.

In some cases you may want to easily switch between two or more versions of a package, for example if you want to switch between using different versions of visual studio.
In the above example you can see that the Visual Studio package is declared with multiple different versions.
The version that is used changes depending on the value of the global property vsversion.
If the property vsversion is not set or does not match on of the condition versions the default version on the first line will be used.

<a name="ProxyPackages"></a>
## Proxy and Non-Proxy SDK Packages ##

You may notice in masterconfig files that some SDK packages have a &quot;-proxy&quot; suffix.
With SDK packages users often have the choice to install the package on their machine themselves or use a package that has all of the files.

Proxy packages try to detect if the package is installed on a users machine.
They are much smaller because they are usually just a couple build scripts with the logic for how to detect the SDK.
However, there are often cases where installing the package is not practical, such is on a build farm
where you have many machines and installing the necessary software on all of them and keeping it up to date is not practical.

Non-Proxy SDKs contain all of the files you need so you don&#39;t need to install the SDK yourself.
The disadvantages are that these packages are often very big, take a long time to download ondemand, and take up a lot of space on a users machine.
There may also be things that cannot be distributed this way, like parts of visual studio, often for legal reasons.

<a name="PackageRoots"></a>
## Package Roots ##

The package roots section tells Framework where to look on your machine for the packages listed in the masterversions section.
Framework will check each of the folders list in package roots to see if they contain any folders that match the name of the package and then search for a manifest.xml within those folders.
Framework will NOT recursively search subfolders of package roots so you need to specify all of the folders to search.
Another thing to consider is that if you share your masterconfig the package roots you use on your machine may not be the same as the ones on someone elses.
In larger systems with shared masterconfigs like Frostbite, package roots use a relative path.
All relative package root paths are relative to the masterconfig files location.

<a name="GlobalProperties"></a>
## Global Properties ##

The global properties section is a list of Framework properties that will be set when you run Framework with this masterconfig.
Alternatively global properties can be provided to nant on the command line, but it is often more convienient to specify
them in the masterconfig if you are setting a lot of options and it is easier to share with other users who are using the same masterconfig.


##### Related Topics: #####
-  [Master Configuration File]( {{< ref "reference/master-configuration-file/_index.md" >}} ) 
