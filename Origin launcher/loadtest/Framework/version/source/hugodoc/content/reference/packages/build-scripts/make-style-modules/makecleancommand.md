---
title: MakeCleanCommand
weight: 189
---

This topic describes how to to set Clean Command for a MakeStyleModule

## Usage ##

Use the Structured XML MakeCleanCommand block or define a property `[group].[module].MakeCleanCommand` in your build script.
The value of this property should contain executable OS command(s)/script.


{{% panel theme = "info" header = "Note:" %}}
Working directory is set is set to `${package.builddir}` when clean command is
executed by NAnt &#39;clean&#39; target or by Visual Studio
{{% /panel %}}
## Example ##


```xml
<MakeStyle name="MyModule">
 ...
 <MakeCleanCommand>
@echo Executing runtime.MyModule.MakeCleanCommand
@if exist output.txt del /F /Q output.txt
@echo Finished runtime.MyModule.MakeCleanCommand
 </MakeCleanCommand>
 ...
</MakeStyle>
```
