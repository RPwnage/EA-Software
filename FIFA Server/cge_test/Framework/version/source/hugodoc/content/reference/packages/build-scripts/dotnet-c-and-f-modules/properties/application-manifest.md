---
title: Application Manifest
weight: 152
---

 `application-manifest` property defines application manifest file.

## Usage ##

Define SXML `<applicationmanifest>` or one of the following properties to assign input manifest file:

 - {{< url groupname >}} `.csproj.application-manifest` - for C# modules<br><br>{{< url groupname >}} `.fsproj.application-manifest` - for F# modules
 - {{< url groupname >}} `.application-manifest`

Framework checks properties in the order they are listed above and takes first one found.

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <applicationmanifest value="${package.dir}\resources\MyManifest.manifest" />
      ...
    </CSharpProgram>

```
In traditional syntax, application-manifest can be set through property.


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.application-manifest" value="${package.dir}\resources\MyManifest.manifest"/>
```
