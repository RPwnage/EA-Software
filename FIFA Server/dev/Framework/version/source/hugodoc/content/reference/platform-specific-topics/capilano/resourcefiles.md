---
title: Resource Files
weight: 287
---

This topic describes how to setup resource files for capilano

<a name="sections"></a>
## How to Add Localized Resource Files ##

To setup localized image files you need to first override the default appxmanifestoptions to add the supported languages and the path to the image files.

The appxmanifestoptions can be changed per module using the optionset name{{< url groupname >}} `.appxmanifestoptions` .


```xml
<optionset name="runtime.mymodule.appxmanifestoptions">
  <option name="resource.languages" value="en-us fr-fr"/>
  <option name="properties.logo" value="${package.dir}/resources/StoreLogo.png"/>
  <option name="visualelements.logo" value="${package.dir}/resources/Logo.png"/>
  <option name="visualelements.smalllogo" value="${package.dir}/resources/SmallLogo.png"/>
  <option name="visualelements.splashscreen" value="${package.dir}/resources/SplashScreen.png"/>
</optionset>
```
The paths to the image files must be absolute paths and should point to a default image file which is located one directory above the language directories.
Framework will use the path to this file and combine it with the languages provided to determine the path to the image files for each language.

In addition to setting the path in the appxmanifestoptions you must also add the image files to the resourcefiles fileset.


```xml
<Program name="mymodule">
<resourcefiles>
    <includes name="${package.dir}/resources/**" basedir="${package.dir}/resources/"/>
</resourcefiles>
. . .
```
<a name="sections"></a>
## How to Move Localized Image Files to a Separate Sub-Directory ##

When you add localized image files you may find that your project&#39;s layout directory becomes cluttered with many locale directories.
To help make your files more organized there is a way to move all of these locale directories into a separate sub-directory.
This can be done by adding the option &quot;resource.image-directory&quot; to the same &quot;appxmanifestoptions&quot; used for setting the languages.
The value of this option should be set as the name of the sub-directory, such as &quot;Images&quot; or &quot;Assets\Images&quot; etc.


```xml
<optionset name="runtime.mymodule.appxmanifestoptions">
<option name="resource.languages" value="en-us fr-fr"/>
<option name="resource.image-directory" value="Images"/>
. . .
```
