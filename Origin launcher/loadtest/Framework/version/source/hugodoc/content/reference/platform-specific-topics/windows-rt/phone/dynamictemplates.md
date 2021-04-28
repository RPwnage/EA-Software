---
title: Dynamic Razor Templates
weight: 292
---

Dynamic Razor Templates are used to generate application manifest files

<a name="Section1"></a>
## Razor Templates ##

Razor templates are dynamic templates where C# Template object can be accessed from the template code.
Razor is used in ASP .Net and examples of Razor syntacs can be found on the web.

Framework implementation of Razor templates for manifest files exposes following objects to the template C# code:

 - ** `Model` ** <br><br>  - ** `Model.Options` ** - options from the manifest parameters optionset.<br>  - ** `VsProjectName` ** - Name of the Visual Studio Project<br>  - ** `VsProjectDir` ** - Path to the Visual Studio Project file<br>  - ** `VsProjectFileNameWithoutExtension` ** - Name of the Visual Studio Project file (without extension)<br><br>Model exposes following helper methods:<br><br>  - ** `string GetImageRelativePath(string origpath)` ** - returns path to image resource after image is copied to layout directory.<br>  - ** `Guid ParseGuidOrDefault(string input, Guid def)` ** - Creates GUID object from string representation, or returns default value if string parsing fails.
 - ** `Module` ** - Framework Module object.
 - ** `Project` ** - Framework (nant) Project object.

## Examples ##

Standard Framework template for Windows RT Manifest  ( *Framework\[version]\source\EA.Tasks\Data\appxmanifest.template* ):


```xml
{{%include filename="/reference/platform-specific-topics/windows-rt/phone/dynamictemplates/appxmanifest.template" /%}}

```
