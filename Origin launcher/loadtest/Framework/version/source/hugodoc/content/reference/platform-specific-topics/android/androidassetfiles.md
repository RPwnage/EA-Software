---
title: Android Asset Files
weight: 273
---

This page answers specific questions related to asset files on android

<a name="compression"></a>
## Asset File Compression ##

Asset files that are added to the APK file may be compressed.
Users have reported that they have trouble reading the compressed files with libraries such as rwfilesystem.

However, there is a way to disable compression for specific file extensions.
This is done by passing the argument &#39;-0&#39; to the android packaging tool.
You can pass arguments to the android packaging tool using the property{{< url groupname >}} `.aaptargs` to specify arguments pre module.
You can also specify global aapt arguments with `package.AndroidSDK.aaptargs` , but the per module setting will be used instead if it exists.


```xml
<!-- Disable compression of certain asset files by extension -->
<property name="runtime.EAMTestApp.aaptargs" value="-0 otf -0 ttf -0 dds -0 jpg -0 png -0 pvr -0 mp4 -0 vp6 -0 wmv" />
```
