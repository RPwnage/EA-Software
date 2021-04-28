---
title: rootnamespace
weight: 159
---

 `rootnamespace` property defines &lt;RootNamespace&gt; element in Visual Studio project.

## Usage ##

Define property:

{{< url groupname >}} `.csproj.rootnamespace` - for C# modules

Default value is set to the{{< url Module >}}name.

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <rootnamespace value="EA.MyModule"/>
      ...
    </CSharpProgram>

```
In traditional syntax, rootnamespace can be set through property.


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.csproj.rootnamespace" value="EA.MyModule"/>
```
