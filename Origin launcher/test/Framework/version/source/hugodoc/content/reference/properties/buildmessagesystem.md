---
title: Build Message System
weight: 222
---

This page describes how to control Framework&#39;s Build Message System,
which is an upgraded version of the logging system introduced in Framework 3.26.00.

<a name="overview"></a>
## Overview ##

The Build Message System is a wrapper for Framework&#39;s logging system that enables some new and more advanced features.
The main feature that this has added is the ability to suppress certain warning and error message.
Warnings and errors that can be suppressed are now printed with a message ID that generally applies to a group of related messages.

Messages can be suppressed using the global properties &quot;nant.warningsuppression&quot; and &quot;nant.errorsuppression&quot;.


```xml
<globalproperties>
  <globalproperty name="nant.warningsuppression">
    2005 2011 2018
  </globalproperty>
</globalproperties>
```
Messages can also be suppressed for individual packages using the properties
&quot;package.[packagename].nant.warningsuppression&quot; and &quot;package.[packagename].nant.errorsuppression&quot;.


```xml
<globalproperties>
  <globalproperty name="package.helloworld.nant.warningsuppression">
    2005 2011 2018
  </globalproperty>
</globalproperties>
```
If you wish to enforce cleanliness in your Framework build scripts you can set the command line property &quot;nant.warningsaserrors&quot; to true which will abort the build when a warning-level condition is met.

