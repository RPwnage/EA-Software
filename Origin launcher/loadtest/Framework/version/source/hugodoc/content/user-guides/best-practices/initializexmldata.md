---
title: Correctly set Initialize.xml data
weight: 26
---

Properly define public data for each module

<a name="SXMLInitializeXml"></a>
## Initialize.xml ##

See [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) for more information.

<a name="AvoidScriptInit"></a>
### Avoid using “ScriptInit” task in Initialize.xml ###

Name of the task is misleading and many engineers misuse it:

![Avoid Script Init]( avoidscriptinit.png )<a name="InitializeXmlDataForEachModule"></a>
### Define data for each module instead of data for package as a whole ###

![Initialize Xml Data For Each Module]( initializexmldataforeachmodule.png )<a name="InitializeXmlAvailableData"></a>
### What data can be set in initialize.xml ###

 - Defines
 - Includedirs
 - Usingdirs
 - Libs
 - Dlls
 - Assemblies
 - Assetfiles – currently android only

