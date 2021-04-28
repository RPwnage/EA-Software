---
title: VisualStudio Settings File Generator
weight: 267
---

This topic describes how to add a generator for Visual Studio .Net &#39;*.settings&#39; files using an optionset.

<a name="Section1"></a>
## Example ##


```xml
<CSharpLibrary name="Example">
    <sourcefiles>
        <includes name="source/**.cs" />
    </sourcefiles>
    <resourcefiles>
        <includes name="source/**.settings" optionset="settings-generator" />
    </resourcefiles>
</CSharpLibrary>

<optionset name="settings-generator">
    <option name="Generator" value="PublicSettingsSingleFileGenerator" />
</optionset>
```
