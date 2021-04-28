---
title: Setting Visual Studio Version
weight: 1
---

This page describes how to control the Visual Studio version used for your build.

<a name="MajorVersion"></a>
## Setting Major Version (Year) ##

The major version of Visual Studio is controlled by the property 'vsversion'. The value should be the release year of the Visual Studio version you wish to use such as '2017' or '2019'. By default '2017' is used. This property can be set on command line or as a global property in the [masterconfig file]( {{< ref "reference/master-configuration-file/_index.md" >}} ). 

<a name="VersionLimits"></a>
## Setting Limits on Exact Versions ##

If you need to set limits on the minimum or maximum version of Visual Studio you can use the properties **package.VisualStudio.MinimumVersion** and **package.VisualStudio.MaximumVersion**. These should be version strings comparable to Microsoft's versioning scheme. For example the default value for **package.VisualStudio.MinimumVersion** is '15.7.27703.2042'. The values used can be more or less precise than this and are compared using Framework's standard version comparison.

<a name="Preview"></a>
## Using Visual Studio Preview Versions ##

By default Framework will not detected preview Visual Studio versions however this can be overidden by setting the property **package.VisualStudio.AllowPreview** to true via command line or in masterconfig global properties. Enabling this property also disables the **package.VisualStudio.MaximumVersion** version check.