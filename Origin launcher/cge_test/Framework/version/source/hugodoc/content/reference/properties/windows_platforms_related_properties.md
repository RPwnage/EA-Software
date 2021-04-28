---
title: Windows Platform Related Properties
weight: 232
---

This page list the properties that are only used when you&#39;re building on Windows platform.

<a name="eaconfig.msvc.version"></a>
## Properties: eaconfig.msvc.version ##

eaconfig.msvc.version is the recommended property for detecting the microsoft visual c compiler version number.
In the past, build script writers would often use the version of the visual studio package to check the compiler version.
However, it is possible for a build to use one version of visual studio with a different version of the compiler.
So it is important to make sure that you are checking the actual compiler version instead of just checking the version of visual studio.

<a name="eaconfig_msvc_version_usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| eaconfig.msvc.version | Yes | Yes | This is set to the version of the visual studio compiler. |

<a name="xaml-add-windows-forms-dll"></a>
## Property: xaml-add-windows-forms-dll ##

Thisxaml-add-windows-forms-dllproperty allows you to add Windows.Forms.dll to the default list
of assemblies to be added to your C# projects that contains XAML files.  The eaconfig package version 2.11.00 or older
used to add Windows.Forms.dll to the default list of assemblies to your C# projects (even if it contains XAML files).
Beginning with eaconfig 2.12.00, this is no longer the case.  To minimize impact to any projects that still reply on
this old behaviour, this global property was added to help user with the transition.

<a name="xaml-add-windows-forms-dll_Usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| xaml-add-windows-forms-dll | No |  | If property is not defined, the default behaviour is treated as false. |

<a name="xaml-add-windows-forms-dll_Example"></a>
### Example ###

You typically set this as global property in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    xaml-add-windows-forms-dll=true
  </globalproperties>
  ...
</project>
```
<a name="eaconfig_defines_winver"></a>
## Properties: eaconfig.defines.winver and [group].[module].defines.winver ##

These properties allow you to set the target Windows OS version to build with using the macros WINVER and _WIN32_WINNT.  You can find the supported WINVER values at this link: [http://msdn.microsoft.com/en-us/library/aa383745.aspx](http://msdn.microsoft.com/en-us/library/aa383745.aspx) 

<a name="eaconfig_defines_winver_usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| eaconfig.defines.winver | No | Yes | If this property is not defined, a default behaviour will set a default WINVER macro.  The default value is different on different condition.  You should check eaconfig package&#39;s eaconfig/dev/config/platform/options/pc-vc-postprocess.cs to see it&#39;s default behaviour. |
| [group].[module].defines.winver | No | No | If this property is not defined, it will check &#39;eaconfig.defines.winver&#39; property and failing that, a default behaviour will set a default WINVER macro.  The default value is different on different condition.  You should check eaconfig package&#39;s eaconfig/dev/config/platform/options/pc-vc-postprocess.cs to see it&#39;s default behaviour. |


{{% panel theme = "info" header = "Note:" %}}
Note that these property will affect all Windows related configs (such as pc, pc64, winrt, and winprt).  So please use this property with care!
{{% /panel %}}
<a name="eaconfig_defines_winver_example"></a>
### Example ###

You can set up this feature using global property eaconfig.defines.winver in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    eaconfig.defines.winver=0x0600  <!-- Setting WINVER to Windows Vista -->
  </globalproperties>
  ...
</project>
```
You can also set up this feature as module specific property in build script like the followings:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <!-- Setting WINVER for module 'MyModule' to Windows XP / Windows Server 2003 -->
  <property name="runtime.MyModule.defines.winver" value="0x0501"/>
  ...
</project>
```
<a name="eaconfig_defines_ntddi_version"></a>
## Properties: eaconfig.defines.ntddi_version and [group].[module].defines.ntddi_version ##

These properties allow you to set the target Windows OS version to build with using the macro NTDDI_VERSION.  You can find the supported NTDDI_VERSION values at this link: [http://msdn.microsoft.com/en-us/library/aa383745.aspx](http://msdn.microsoft.com/en-us/library/aa383745.aspx) 

One advantage of setting the NTDDI_VERSION macro is that you can target for specific OS service Pack, while using WINVER alone does not allow you to target for specific OS service pack.


{{% panel theme = "info" header = "Note:" %}}
Support for this property is only provided in eaconfig 2.20.00 and after.
{{% /panel %}}
<a name="eaconfig_defines_ntddi_version_usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| eaconfig.defines.ntddi_version | No | Yes | If this property is not defined, the default behaviour is to not define NTDDI_VERSION macro and let sdkddkver.h set up this macro for you based on the WINVER setting. |
| [group].[module].defines.ntddi_version | No | No | If this property is not defined, it will check &#39;eaconfig.defines.ntddi_version&#39; property and failing that, the default behaviour is to not define NTDDI_VERSION macro and let sdkddkver.h set up this macro for you based on the WINVER setting. |


{{% panel theme = "info" header = "Note:" %}}
Note that these property will affect all Windows related configs (such as pc, pc64, winrt, and winprt).  So please use this property with care!

Also, if the macro NTDDI_VERSION is use, Windows SDK requires that you setup _WIN32_WINNT macro with the identical OS verson as well.  However, eaconfig provided that support for you.  So if you used
property XXX.defines.ntddi_version to setup target OS version, you don&#39;t need to setup XXX.defines.winver property, eaconfig will setup the appropriate WINVER and _WIN32_WINNT values for you.
{{% /panel %}}
<a name="eaconfig_defines_ntddi_version_example"></a>
### Example ###

You can set up this feature using global property eaconfig.defines.ntddi_version in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    eaconfig.defines.ntddi_version=0x06000100  <!-- Setting NTDDI_VERSION to Windows Vista service pack 1-->
  </globalproperties>
  ...
</project>
```
You can also set up this feature as module specific property in build script like the followings:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <!-- Setting NTDDI_VERSION for module 'MyModule' to Windows XP with service pack 3 -->
  <property name="runtime.MyModule.defines.ntddi_version" value="0x05010300"/>
  ...
</project>
```
