---
title: Android Keystores and Signing
weight: 278
---

This page describes some of the options available for controlling APK signing and keystores.

<a name="keystores"></a>
## About Keystores and how to use External Keystores ##

By default android_config creates a temporary keystore for each executable it signs.
This is because we want nant (or incredibuild) to be able to build multiple executables
in parallel and Android Tools fail when there is concurrent access to a single keystore.

The default location for the keystore is the modules intermediate directory.
(ex. ${package.dir}/build/${package.version}/${config}/build/${modulename}/)

However some users might want to provide there own keystore for signing.
And in most case these users won&#39;t be concerned about parallel execution because they are only building a single executable.
In this case we have added a property for controlling the location where android_config looks for the keystore.


```xml
<property name="android.keystore-home-dir" value="C:/Users/username/"/>
```
The example above will cause the build to look for a keystore in &quot;C:/Users/username/.android&quot;
and if no keystore is found it will generate a new one.

Note that this feature is currently not supported when building under Visual Studio 2015.

<a name="unsignapk"></a>
## Creating an unsigned APK file ##

Some users have found they need to sign their APK files with a release keystore for shipping and to do this they need to build an unsigned APK file.
We have added a property for building an unsigned APK file:


```xml
<property name="package.android_config.unsigned_apk" value="true"/>
```
Note that this feature is currently not supported when building under Visual Studio 2015.

<a name="playstore"></a>
## Additional Steps Required to Sign for Submission to Google Play Store ##

The keystore properties described above were requested by a team that was submitting to Google play store.
Below are some other steps that they need to complete in order to sign there package for submission.

They needed to provide a custom android manifest file with the field &quot;debuggable&quot; set to false and the correct versionName value set.

They also needed to use the [TNT Jar Signing Service](http://eaiwiki.eamobile.ad.ea.com/display/TNT/TNT+Jar+Signing+Service) Tool to sign an unsigned apk file with a shipping keystore.
The tool also needs to be used to make the APK file zipaligned.

