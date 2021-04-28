---
title: appdesigner-folder
weight: 150
---

 `appdesigner-folder` property defines App Designer folder.

## Usage ##

Define SXML `<appdesignerfolder>` element or one of the following properties to set App Designer folder:

 - {{< url groupname >}} `.csproj.appdesigner-folder` - for C# modules<br><br>{{< url groupname >}} `.fsproj.appdesigner-folder` - for F# modules
 - {{< url groupname >}} `.appdesigner-folder`

Framework checks properties in the order they are listed above and takes first one found.

Default value is  `"Properties"` 

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <appdesignerfolder value="Properties" />
      ...
    </CSharpProgram>

```
In traditional syntax, appdesigner-folder can be set through property.


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.appdesigner-folder" value="Properties"/>
```
