---
title: Android Command Line Arguments
weight: 275
---

This page describes the command line arguments support that is provided by android_config if you are using the default C++ JNI entry point code provided
in android_config package&#39;s data\jni-template folder. If your package defines property runtime.android-skip-default-entry-point as true none of the following
applies.

## Command Line Arguments Flow ##

The command line argument support for Android is done by including JNI code that can extract command line arguments (following a particular convention) from
an Activity Intent&#39;s extra bundle. The flow is as follows:

 - The test running code in android_config launches ActivityManager with extra arguments --ei argc X --es arg0 &#39;YYY&#39; -es arg1 &#39;ZZZ&#39; ...
 - The entry point calls jni_main.cpp (in android_config&#39;s data folder) via java_main or native_main depending on entry point type.
 - Code in jni_main constructs argc and argv from activity intent bundle (key value pairing from arguments passed to ActivityManager).
 - End of jni_main calls extern linked main(argc, argv) function which user should provide.



## Passing Command Line Arguments To Native Entry Point Using Custom Java ##

If using java entry point Framework will not generate entry point code if a program module contains java code as it assumed user
has provided their own Activity class. However if runtime.android-skip-default-entry-point is not set to true it also assumed that
the jni_main code should still be included for command line parameter support. See the Android Entry Points documentation for more
information on how to pass arguments to native entry from your Activity.


##### Related Topics: #####
-  [Android Entry Points]( {{< ref "reference/platform-specific-topics/android/androidentrypoints.md" >}} ) 
