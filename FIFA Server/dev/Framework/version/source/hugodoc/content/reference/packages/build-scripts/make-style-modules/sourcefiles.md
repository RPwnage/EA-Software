---
title: sourcefiles
weight: 191
---

This topic describes how to set sourcefiles for a MakeStyleModule

## Usage ##

Use `<sourcefiles>`  in SXML or define  a {{< url fileset >}} with name {{< url groupname >}} `.sourcefiles` in your build script.


{{% panel theme = "info" header = "Note:" %}}
Any{{< url optionset >}}assigned to files is ignored. MakeStyle module does not build these files.
{{% /panel %}}
## Example ##


```xml
<MakeStyle name="MyModule">
 <sourcefiles basedir="${package.dir}\src\">
  <includes name="**"/>
 </sourcefiles>
</MakeStyle>
```
