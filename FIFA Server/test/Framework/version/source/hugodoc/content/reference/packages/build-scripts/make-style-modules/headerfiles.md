---
title: headerfiles
weight: 193
---

This topic describes how to set headerfiles for a MakeStyleModule

## Usage ##

Use `<headerfiles>`  in SXML or define  a {{< url fileset >}} with name {{< url groupname >}} `.headerfiles` in your build script.


{{% panel theme = "info" header = "Note:" %}}
If `headerfiles` fileset is not provided explicitly Framework will add default values
using following patterns:

 - `"${package.dir}/include/**/*.h"`
 - `"${package.dir}/${module.groupsourcedir}/**/*.h"`

Where GroupSourceDir equals to

 - `${eaconfig.${module.buildgroup}.sourcedir"}/${module.name};` - for packages with modules
 - `${eaconfig.${module.buildgroup}.sourcedir"}` -for packages without modules.
{{% /panel %}}
## Example ##


```xml
<MakeStyle name="MyModule">
 <headerfiles basedir="${package.dir}\include\">
  <includes name="**"/>
 </headerfiles>
</MakeStyle>
```
