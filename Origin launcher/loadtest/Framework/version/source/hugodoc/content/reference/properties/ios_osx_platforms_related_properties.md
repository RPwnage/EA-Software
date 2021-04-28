---
title: OSX / IOS Platforms Related Properties
weight: 234
---

This page list the properties that are used when you&#39;re building on Mac OS (such as ios or osx builds).

<a name="xcode-application-directory"></a>
## Property: xcode-application-directory ##

xcode-application-directoryallows you to specify which version of XCode to use when you have installed multiple versions of XCode on your Mac.

<a name="xcode-application-directory_Usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| xcode-application-directory | ![requirements 1a]( requirements1a.gif ) |  | It would be initialized as any XCode* app bundle folder that is automatically found on your Mac&#39;s /Applications folder. |


{{% panel theme = "info" header = "Note:" %}}
The latest implementation of osx_config assumes that your XCode application is installed in
/Applications folder.  If you have some old system where your XCode isn&#39;t installed to /Applications folder,
you need to do a re-install of XCode to /Applications folder first.
{{% /panel %}}
<a name="xcode-application-directory_Example"></a>
### Example ###

You typically set this as global property in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    xcode-application-directory=XCode5.app
  </globalproperties>
  ...
</project>
```
<a name="base-sdk-version"></a>
## Properties: osx-base-sdk-version and iphone-base-sdk-version ##

Theosx-base-sdk-version and iphone-base-sdk-versionproperties are available
to allow you to specify the base SDK version during build.

<a name="base-sdk-version_Usage"></a>
### Usage ###

Name |Always defined |Has to be Global |Default Value |
--- |--- |--- |--- |
| osx-base-sdk-version | ![requirements 1a]( requirements1a.gif ) |  | This property is initialized to an empty string in osx_config package meaning we will be building with latest SDK version. |
| iphone-base-sdk-version | ![requirements 1a]( requirements1a.gif ) |  | This property is initialized to an empty string in iphonesdk package meaning we will be building with latest SDK version. |

<a name="base-sdk-version_Example"></a>
### Example ###

You typically set these as global properties in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    osx-base-sdk-version=10.6
    iphone-base-sdk-version=7.0
  </globalproperties>
  ...
</project>
```
<a name="deployment-target-version"></a>
## Properties: osx-deployment-target-version and iphone-deployment-target-version ##

Theosx-deployment-target-version and iphone-deployment-target-versionproperties allows you to
build your binaries to target for a minimum OS version.

In general, it is important to set this property to make sure that your binary is able to run on the target platform version
that you intend for it to run on while at the same time set your base SDK version to use latest.

<a name="deployment-target-version_Usage"></a>
### Usage ###

Name |Always defined |Has to be Global |Default Value |
--- |--- |--- |--- |
| osx-deployment-target-version | ![requirements 1a]( requirements1a.gif ) |  | Starting with osx_config 1.07.01, this property is initialized to 10.6 if you are doing NAnt build.  Otherwise,<br>XCodeProjectizer also initialize this to 10.6 if the property wasn&#39;t initialized (ie you&#39;re using older osx_config). |
| iphone-deployment-target-version | ![requirements 1a]( requirements1a.gif ) |  | This is initialized in iphonesdk with 3.2. |

<a name="deployment-target-version_Example"></a>
### Example ###

You typically set these as global properties in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    osx-deployment-target-version=10.6
    iphone-deployment-target-version=3.2
  </globalproperties>
  ...
</project>
```
