---
title: Copying Files to SD Card
weight: 277
---

This page describes how to specify a list of files to be transfered to an android device&#39;s sdcard.
This is different from [assetfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/assetfiles.md" >}} ) which, on android, are added to the APK file and deployed to the device along with it.

<a name="Section1"></a>
## Specifying files to transfer ##

During a test run, immediately after deploying the APK file, android_config can be setup to deploy a list of files to the device&#39;s sdcard.
To specify which files to copy use the fileset{{< url groupname >}} `.android.sdcard.deploy-contents-fileset` .

To specify the destination directories of specific files you can create an optionset with an option named `android.sdcard.partialdir` and add this option to one of the include elements in the fileset. This way the files described by each include element can be copied to a different directory on the sdcard.

The basedir attribute set on either the fileset or the include element tells android_config how much of the filepath to drop when constructing the destination path.
For example if the include element is set to &quot;${package.dir}/source/**.cpp&quot; and contains a file at &quot;${package.dir}/source/tests/timertest.cpp&quot; and the basedir is set to &quot;${package.dir}/source&quot;
then the destination file path will be &quot;/sdcard/${android.sdcard.partialdir}/tests/timertest.cpp&quot; because everything specified in the basedir is dropped.

The following example shows how the sdcard file deployment options should be used:


```xml
<optionset name="runtime.rwfilesystemtest.android.sdcard.deploy-contents-optionset">
 <option name="android.sdcard.partialdir" value=""/>
</optionset>

<fileset name="runtime.rwfilesystemtest.android.sdcard.deploy-contents-fileset" basedir="${package.dir}/data/">
 <includes name="**" optionset="runtime.rwfilesystemtest.android.sdcard.deploy-contents-optionset"/>
</fileset>
```
<a name="fastdeployment"></a>
## Fast File Deployment ##

File deployment on the android emulator has been known to be significantly slower than when using a physical device.
However, we have discovered through experimentation that file deployment to an emulator goes faster if there is an app running on the device.
It seems like the emulator goes into some kind of low performance idle state when no apps are running.
So when android_config deploys files it runs a small graphical application to keep the emulator busy, and by doing this we have observed a 10-100 times speed improvement.

The graphics application is enabled by default, but disabled for x86 and GPU enabled builds. It can also be disabled by using the property `package.android_config.enable-fast-file-deployment` 

