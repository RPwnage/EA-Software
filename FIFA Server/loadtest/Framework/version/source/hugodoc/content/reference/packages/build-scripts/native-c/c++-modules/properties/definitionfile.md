---
title: definition-file
weight: 120
---

 *definition-file*  property sets DLL symbol definition file (VC compiler only)

<a name="DefFilenUsage"></a>
## Usage ##

Define property{{< url groupname >}} `.definition-file` in your build script. The property need to point to the def file.

It will be added to the linker as -DEF:${{{< url groupname >}}.definition-file}

