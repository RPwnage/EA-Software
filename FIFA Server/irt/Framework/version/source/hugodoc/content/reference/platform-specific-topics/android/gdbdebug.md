---
title: GDB Command Line Debugging
weight: 282
---

This topic describes GDB Debugging support for Android.
GDB allows Android developers to debug code running on the device or in the emulator.
This page will discuss the features and properties related to GDB Debugging
and the steps required to get debugging working on the Android device.

<a name="commandline"></a>
## GDB Command Line Debugging ##

The basic way to use GDB debugging with an android app is to
use the command line utility. To get access to this utility, use
the standard run target that you would use for running your app
but also set the property &quot;android-testrun-with-debugger&quot; to true.
Then when your app starts a GDB console window will appear that
you can type in.


{{% panel theme = "info" header = "Note:" %}}
Currently the application will start running before the debugger has been attached
which means short tests may finish and exit before the user has a chance to set a breakpoint.
So far the only known solution is to add an infinite loop to the beginning of your test that
you can break out of by changing a variable in the debugger.
{{% /panel %}}
When using GDB it is important that the app is built with debug symbols
included. Originally, debug symbols were striped from all builds but
to support GDB debugging we have changed this so that they are not
stripped with using debug configs. You can also override the default
behavior using the property &quot;package.android_config.stripsymbols&quot;.

<a name="features"></a>
## Framework Features to Support GDB debugging ##

As the Framework team we have added features to improve the GDB debugging workflow
by making built packages compatible with GDB.

For users to be able to debug on android a copy of gdbserver must be included in the
apk file. We have added a step to the android build process to copy the gdbserver
files into the appropriate directory within the apk file. This step
is only executed during debug builds but can be overridden using the &quot;includegdb&quot;
property described in the section below.

We have also provided some features for teams working with Android AVDs,
such as properties for selecting the AVD to run against as well
controlling the amount of time before a test times out.


{{% panel theme = "info" header = "Note:" %}}
To debug with gdb we ensure that the built native
libraries, such as those with the &#39;.so&#39; extension, begin with the prefix &#39;lib&#39;.
In some cases users have overridden the linkoutputname option with a custom value which
does not start with &#39;lib&#39;. Our code checks for this and adds &#39;lib&#39; if needed while throwing a warning.
If you see this warning we recommend that you change the value passed to the linkoutputname
option in the config-options-program optionset so that it starts with &#39;lib&#39;.
{{% /panel %}}
<a name="properties"></a>
## Properties ##

These properties have been added to control features related to testing and
debugging on Android.

 - The avdname property allows users to select an alternate vm to run tests against.<br>These avd&#39;s are located in the AndroidSDK package filed under &quot;./data/.android/avd/&quot;.<br>```xml<br><property name="package.AndroidSDK.avdname" value="standard_api17_extraram"/><br>```
 - The includegdb property allows users to toggle whether the gdbserver is copied and<br>included in the built package&#39;s apk file. The gdbserver is necessary for debugging<br>but is only included by default for debug configs, this property allows users to<br>override that condition.<br>```xml<br><property name="package.android_config.includegdb" value="true"/><br>```
 - When this property is set to true using the standard run targets will open a GDB<br>command line window that users can interact with.<br>```xml<br><property name="android-testrun-with-debugger" value="true"/><br>```
 - The stripsymbols property can be used to override the default behavior related<br>to stripping debug symbols from library files. By default symbols are not strip<br>when using debug configs, but otherwise are stripped.<br>```xml<br><property name="package.android_config.stripsymbols" value="true"/><br>```

