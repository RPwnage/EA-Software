---
title: Android Extension Libraries
weight: 276
---

How to use Android Libraries with Framework

<a name="Section1"></a>
## Package for Android Extension Library ##

To use Android Extension Library in Framework builds it needs to be converted to a Framework {{< url package >}}:

 - Place extension library in a folder  *(Package Name)/(Package Version)*
 - Add build file, Manifest file, and Initialize.xml file.<br><br>  - **build**  file<br><br>If Library contains Java sources that should be built, define all necessary input in the build file.<br><br>If library contains embedded resources like strings build file should define these resources so that R.java file will be generated and compiled.<br>  - **Initialize.xml** file<br><br>Declare class files, Java archives, and resource directories that are exported by this library<br>  - **Manifest.xml** file<br><br>Standard Framework 2 manifest

Android Library Package can contain multiple modules, or Android Library can be added to existing package as a separate module.

## Example ##

This example is based on the Google&#39;s play_apk_expansion library that can be found in Android SDK at  *...\sdk\extras\google\play_apk_expansion* 



### play_apk_expansion package. ###

This example setup two modules `downloader_library`  and  `zip_file` for package play_apk_expansion (dev version). Initialize.xml file declares public(export) data for each module.

Below are Framework script files added to the expansion library to make it a Framework package

 - *play_apk_expansion/dev/play_apk_expansion.build* - build script<br><br><br>```xml<br>{{%include filename="/reference/platform-specific-topics/android/androidextensionlibraries/play_apk_expansion.build" /%}}<br><br>```
 - *play_apk_expansion/dev/scripts/Initialize.xml* - This files declares public data<br><br><br>```xml<br>{{%include filename="/reference/platform-specific-topics/android/androidextensionlibraries/initialize.xml" /%}}<br><br>```
 - *play_apk_expansion/dev/Manifest.xml* - Framework manifest file (this is different from Android manifest)<br><br><br>```xml<br>{{%include filename="/reference/platform-specific-topics/android/androidextensionlibraries/manifest.xml" /%}}<br><br>```

### play_apk_expansion_sample package. ###

An example package demonstrates usage of the Android Expansion library package.

All that is necessary is to declare build dependency on the expansion library.

 *play_apk_expansion_sample/dev/play_apk_expansion.build* - build script


```xml
{{%include filename="/reference/platform-specific-topics/android/androidextensionlibraries/play_apk_expansion_sample.build" /%}}

```
### play_licensing package. ###

Package play_licensing-dev is another example of expansion library. play_licensing defines only one module.

Below are Framework script files added to the expansion library to make it a Framework package

 - *play_licensing/dev/play_licensing.build* - build script<br><br><br>```xml<br>{{%include filename="/reference/platform-specific-topics/android/androidextensionlibraries/play_licensing.build" /%}}<br><br>```
 - *play_licensing/dev/scripts/Initialize.xml* - This files declares public data<br><br><br>```xml<br>{{%include filename="/reference/platform-specific-topics/android/androidextensionlibraries/initialize.xml" /%}}<br><br>```
 - *play_licensing/dev/Manifest.xml* - Framework manifest file (this is different from Android manifest)<br><br><br>```xml<br>{{%include filename="/reference/platform-specific-topics/android/androidextensionlibraries/manifest.xml" /%}}<br><br>```


##### Related Topics: #####
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
-  [Dependencies]( {{< ref "reference/packages/build-scripts/dependencies/_index.md" >}} ) 
