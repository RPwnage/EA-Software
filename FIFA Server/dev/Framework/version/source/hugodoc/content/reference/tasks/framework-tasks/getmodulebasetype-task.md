---
title: < GetModuleBaseType > Task
weight: 1177
---
## Syntax
```xml
<GetModuleBaseType buildtype-name="" Name="" base-buildtype="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Evaluates base buildtype name for the given buildtype optionset.

## Remarks ##
Base build type name can be one of the following
 - Library
 - Program
 - DynamicLibrary
 - WindowsProgram
 - CSharpLibrary
 - CSharpProgram
 - CSharpWindowsProgram
 - ManagedCppAssembly
 - ManagedCppProgram
 - MakeStyle
 - VisualStudioProject
 - Utility



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| buildtype-name | Name of the buildtype optionset to examine | String | False |
| Name | (deprecated) Duplicate of the buildtype-name field for backward compatibility | String | False |
| base-buildtype | Evaluated buildtype. Saved in property &quot;GetModuleBaseType.RetVal&quot; | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<GetModuleBaseType buildtype-name="" Name="" base-buildtype="" failonerror="" verbose="" if="" unless="" />
```
