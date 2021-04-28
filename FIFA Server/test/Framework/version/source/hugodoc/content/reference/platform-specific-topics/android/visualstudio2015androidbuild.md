---
title: Visual Studio 2015 Android Build
weight: 272
---

Building for Android platform using VS2015.

<a name="Overview"></a>
## Overview ##

Starting with VS2015 Android is supported without the need for any extensions. Solutions can be built and generated with Framework using the usual solution targets.
Both clang and gcc toolchains are supported.

Note that vs-android does not support VS2015 so make sure that the &quot;android-use-vs-android&quot; property is set to false.

<a name="ImportantNote"></a>
## Important Note ##

 - The [External Keystores]( {{< ref "reference/platform-specific-topics/android/androidkeystore.md#keystores" >}} ) feature (ie the android.keystore-home-dir property) is currently not supported under VS2015 solution.
 - The [package.android_config.unsigned_apk]( {{< ref "reference/platform-specific-topics/android/androidkeystore.md#unsignapk" >}} ) property is currently not supported under VS2015 solution.
 - The [package.android_config.multidex]( {{< ref "reference/properties/android_platform_related_properties.md#multidex" >}} ) property is currently not supported under VS2015 solution.
 - Visual Studio 2015&#39;s Android build support doesn&#39;t allow you to setup projects with mixed platforms (for example, you cannot create a solution containing<br>both pc-* and android-* configs).  You need to create solution with android-* configs only.
 - If you manually open a VS 2015 Android solution, Visual Studio willl start an adb.exe process which will be running in the background and potentiall lock up your build root folder (even if<br>you shut down Visual Studio).  To get your build root unlock, you need to manually kill that process yourself.

