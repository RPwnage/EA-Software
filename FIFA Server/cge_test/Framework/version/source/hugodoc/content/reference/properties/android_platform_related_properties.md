---
title: Android Platform Related Properties
weight: 233
---

This page list the properties that are used for the android configs only.

<a name="android-use-target"></a>
## Property: android-use-target ##

android-use-targetproperty tells the Android run target to always use device vs emulator vs auto pick.

<a name="android-use-target_Usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| android-use-target | ![requirements 1a]( requirements1a.gif ) |  | emulator |

Acceptable values are &#39;emulator&#39;, &#39;device&#39;, &#39;auto&#39;, or an IP address.
This property is typically set in the masterconfig file or on the command line and is used to tell the Android run target where to deploy and run the application.
This property replaces the two old properties &#39;android-use-device&#39; and &#39;android-use-emulator&#39; which are no longer supported.

The &#39;emulator&#39; setting tells the build that the program should be run on an emulator.
This will cause the build to start a new emulator, unless &#39;android-runtarget-emulator-restart&#39; is false in which case it will try to connect to a currently running emulator.
Because no information is provided about what emulator to connect to the build will fail if there is more than one.

The &#39;device&#39; settings will make the build system try to use to a connected device.
Because no information is provided about what device to connect to the build will fail if there is more than one.

The &#39;auto&#39; setting will cause the build to try connecting to a device, if that succeeds it will proceed as if the property was set to &#39;device&#39;, if that fails it will proceed as if the property was set to &#39;emulator&#39;.
There is a small amount of additional overhead with this setting because of the time taken to check if a device is connected so this is not recommended for everyone.
This setting is intended for specific situations where the user needs to use the same masterconfig file across a variety of machines where some have connected devices but others don&#39;t.

The property can also be set as an IP address in which case the build will try to connect to the emulator or device currently running at that IP address.

<a name="android-use-target_Example"></a>
### Example ###

Specifying `android-use-target` property on the nant command line:


```
nant test-run -D:config=android-clang-dev-debug -D:android-use-target=device
```

{{% panel theme = "info" header = "Note:" %}}
`android-use-target`  can be defined on nant command line or in {{< url Masterconfig >}}.
{{% /panel %}}
<a name="android-timeout"></a>
## Property: android-timeout ##

android-timeoutproperty helps set a timeout for the run target.

<a name="android-timeout_Usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| android-timeout | ![requirements 1a]( requirements1a.gif ) |  | Specified in milliseconds.  Default to ${eaconfig-run-timeout}(sec) * 1000, where eaconfig-run-timeout is either defaults to 1200 seconds or derived from{{< url groupname >}} `.run.timeout` property. |

<a name="android-timeout_Example"></a>
### Example ###

You typically set this as global property in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    android-timeout=300000       <!-- Set for 5 minutes. -->
  </globalproperties>
  ...
</project>
```

{{% panel theme = "warning" header = "Warning:" %}}
If you have specified module specific timeout (ie{{< url groupname >}} `.run.timeout` property), this property will override your module specific timeout specification.
{{% /panel %}}
<a name="android-deploy-timeout"></a>
## Property: android-deploy-timeout ##

android-deploy-timeoutproperty helps set the timeout for deploying the package or assets to the test device or emulator.

<a name="android-deploy-timeout_Usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| android-deploy-timeout | ![requirements 1a]( requirements1a.gif ) |  | Specified in milliseconds.  Default to ${eaconfig-run-timeout}(sec) * 100, where eaconfig-run-timeout is either defaults to 1200 seconds or derived from{{< url groupname >}} `.run.timeout` property. |

<a name="android-deploy-timeout_Example"></a>
### Example ###

You typically set this as global property in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    android-deploy-timeout=30000        <!-- Set for 30 seconds. -->
  </globalproperties>
  ...
</project>
```
<a name="android-emulator-timeout"></a>
## Property: android-emulator-timeout ##

android-emulator-timeoutproperty sets the timeout specifically for initializing the Android Emulator.

<a name="android-emulator-timeout_Usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| android-emulator-timeout | ![requirements 1a]( requirements1a.gif ) |  | Specified in milliseconds.  Default to ${android-timeout}. |

<a name="android-emulator-timeout_Example"></a>
### Example ###

You typically set this as global property in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    android-emulator-timeout=60000        <!-- Set for 1 minute. -->
  </globalproperties>
  ...
</project>
```
<a name="package-AndroidNDK-toolchain-version"></a>
## Property: package.AndroidNDK.toolchain-version ##

package.AndroidNDK.toolchain-versionproperty allows you to specify the AndroidNDK toolchain version being used to do your build.

<a name="package-AndroidNDK-toolchain-version_Usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| package.AndroidNDK.toolchain-version | ![requirements 1a]( requirements1a.gif ) | Yes | At time of writing this doc, it is defaulted to 4.6.  However, please review your version of AndroidNDK&#39;s initialize.xml script to make sure this isn&#39;t changed. |

<a name="package-AndroidNDK-toolchain-version_Example"></a>
### Example ###

You should set this as global property in masterconfig:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    package.AndroidNDK.toolchain-version=4.8
  </globalproperties>
  ...
</project>
```
<a name="multidex"></a>
## Property: package.android_config.multidex ##

package.android_config.multidexproperty will turn on multidex support when the property is set to true.
Multidexing is an android feature added in AndroidSDK 20 and later that can be used to split classes.dex files into multiple files
once the size limit has been exceeded. The split dex files will be generated into a zip file named classes.dex.zip before been merged into the apk.
The size limit of the classes.dex file can be exceeded for example if your java source code contains more that 65,536 methods.
When the size limit of the dex file has been exceeded you should get an error message which recommends using the multidex feature.

When the multidex property is enabled android_config will do all the necessary work to enable multidexing.
It will add the multidex support jar (only found in AndroidSDK 20 and later).
It will also modify the command line arguments passed to the dex file generator to enable multidexing and to output the dex files to a zip archive.

Note that this feature is currently not supported when building under Visual Studio 2015.

<a name="multidex_Usage"></a>
### Usage ###

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| package.android_config.multidex |  |  | false |

