---
title: copylocal
weight: 154
---

 `copylocal` property defines whether referenced assemblies are copied into the output folder of the module.

## Usage ##

Define SXML `<copylocal>`  element or a property {{< url groupname >}} `.copylocal` in your build script.
The value of this property can take one of the following values:

 - **true** - copy all referenced assemblies and assemblies built by dependent modules to output folder.
 - **false** - Don&#39;t copy referenced assemblies to output folder.


{{% panel theme = "info" header = "Note:" %}}
Default value is &#39;false&#39;
{{% /panel %}}
{{< url groupname >}} `.copylocal` property affects all referenced assemblies and all dependencies.

{{< url groupname >}} `.copylocal` overrides per dependency definitions. Do not set this property to &#39;true&#39; if
you want to control this flag on dependency level.


{{% panel theme = "info" header = "Note:" %}}
To set &#39;copylocal&#39; flag per dependency set module &#39;copylocal&#39; property to false.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
copylocal works for both native nant and Visual Studio Solution builds, and Incredibuild builds.
{{% /panel %}}
## Example ##


```xml

    <CSharpProgram name="MyModule">
      <copylocal>true</copylocal>
      ...
    </CSharpProgram>

```
In traditional syntax, copylocal can be set through property.


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.copylocal" value="true"/>
```
