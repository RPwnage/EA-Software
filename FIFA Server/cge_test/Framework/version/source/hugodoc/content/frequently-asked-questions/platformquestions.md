---
title: Platform Specific Questions
weight: 308
---

This page lists some command questions that is platform specific

<a name="Section1"></a>
## How to add &quot;Android Extension Libraries (Android Packages)&quot;? ##

 - First you need to make sure you are using `android_config`  version  `1.04.00`  or later and  `Framework`  version  `3.15.00` or later.
 - Then check this [Android Extension Libraries]( {{< ref "reference/platform-specific-topics/android/androidextensionlibraries.md" >}} ) page for brief intro on how to do it.
 - Remember to declare your class files, Java archives, and resource directories in your initialize.xml:<br><br>  - [Java.archives]( {{< ref "reference/packages/initialize.xml-file/java.archives.md" >}} )<br>  - [java.build-only-archives]( {{< ref "reference/packages/initialize.xml-file/java.archives.md" >}} )<br>  - [Java.classes]( {{< ref "reference/packages/initialize.xml-file/java.classes.md" >}} )<br>  - [Resourcedir]( {{< ref "reference/packages/initialize.xml-file/resourcedir.md" >}} )
 - Example projects can also be found in Framework&#39;s example/AndroidExpansionLibraries folder.

<a name="section2"></a>
## Now that these platforms are released, why don&#39;t you use the names &quot;ps4&quot; and &quot;xb1&quot; instead of the codewords &quot;kettle&quot; and &quot;capilano&quot; which have no meaning outside of EA? ##

Changing configuration names is a non-trivial process because of the amount code in platform-specific branches and the large amount of
shared technology EA uses.  Adding new configuration names in the CM packages is not a problem.  But before any game teams could start
using these config names, all of EA&#39;s shared technology - hundreds of packages - would need to be updated to work properly with the new
platform names.  We couldn&#39;t just do a global search and replace either because some game teams would move to the new config
names at a slower pace than others so both old and new names would have to be supported during the transition period.  Our shared packages
could be stuck having to support both names until the last game team transitioned to the new names, which might take as long as three years.

In addition to the shared technology, any platform-specific code in the game team&#39;s codebase would have to be moved to the updated
branch name, which would complicate the revision history.

The cost of changing the names of these platforms is simply not worth the benefit, so we plan on keeping the existing ones.

