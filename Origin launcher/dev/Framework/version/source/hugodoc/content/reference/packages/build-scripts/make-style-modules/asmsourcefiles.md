---
title: asmsourcefiles
weight: 192
---

This topic describes how to set asmsourcefiles for a MakeStyleModule

## Usage ##

Use `<asmsourcefiles>`  in SXML or define  a {{< url fileset >}} with name {{< url groupname >}} `.asmsourcefiles` in your build script.


{{% panel theme = "info" header = "Note:" %}}
Any{{< url optionset >}}assigned to files is ignored. MakeStyle module does not build these files.
{{% /panel %}}
## Example ##


```xml
<MakeStyle name="MyModule">
 <asmsourcefiles basedir="${package.dir}\src\">
  <includes name="**.asm"/>
 </asmsourcefiles>
</MakeStyle>
```
