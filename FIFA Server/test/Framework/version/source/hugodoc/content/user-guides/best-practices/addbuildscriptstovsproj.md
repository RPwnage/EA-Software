---
title: Automatically add build scripts to generate Visual Studio projects
weight: 28
---

Automatically including build scripts in generated Visual Studio projects can simplify development.

<a name="BPAutoUncludeBuildScripts"></a>
## Adding build scripts into VS projects ##

 - Set property<br><br><br>```xml<br><br>generator.includebuildscripts = true<br><br>```
 - Build file is included into generated VS project
 - Initialize.xml is included into generated project

![Auto Add Build Scripts ToVSProject]( autoaddbuildscriptstovsproject.png )