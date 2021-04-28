---
title: MakeRebuildCommand
weight: 190
---

This topic describes how to to set ReBuild Command for a MakeStyleModule


{{% panel theme = "info" header = "Note:" %}}
ReBuild commands for MakeStyle module can be used only in Visual Studio
{{% /panel %}}
## Usage ##

Use the Structured XML MakeRebuildCommand block or define a property `[group].[module].MakeRebuildCommand` in your build script.
The value of this property should contain executable OS command(s)/script.


{{% panel theme = "info" header = "Note:" %}}
Working directory is set is set to `${package.builddir}` when clean command is
executed by NAnt &#39;clean&#39; target or by Visual Studio
{{% /panel %}}
## Example ##


{{% panel theme = "info" header = "Note:" %}}
Since structure XML does not set the `MakeCleanCommand`  and  `MakeBuildCommand` the same way you either must either repeat the whole clean and build step code in the rebuild step or create a separate
property to wrap the code in.
{{% /panel %}}

```xml
<MakeStyle name="MyModule">
 ...
 <MakeRebuildCommand>
@echo Executing runtime.MyModule.MakeCleanCommand
@if exist output.txt del /F /Q output.txt
@echo inline const char *GetConfigTestString() { return "${config}"; } > output.txt
@echo Finished runtime.MyModule.MakeCleanCommand
 </MakeRebuildCommand>
 ...
</MakeStyle>
```
