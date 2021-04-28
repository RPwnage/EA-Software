---
title: MakeBuildCommand
weight: 188
---

This topic describes how to to set Build Command for a MakeStyleModule

## Usage ##

Use the Structured XML MakeBuildCommand block or define a property{{< url groupname >}} `.MakeBuildCommand` in your build script.
The value of this property should contain executable OS command(s)/script.


{{% panel theme = "info" header = "Note:" %}}
Working directory is set is set to `${package.builddir}` when build command is
executed by NAnt build target or by Visual Studio
{{% /panel %}}
## Example ##


```xml

<MakeStyle name="MyModule">
  ...
  <MakeBuildCommand>
    @echo Executing runtime.MyModule.MakeBuildCommand
    @echo inline const char *GetConfigTestString() { return "${config}"; } > output.txt
    @echo Finished runtime.MyModule.MakeBuildCommand
  </MakeBuildCommand>
  ...
</MakeStyle>

```
