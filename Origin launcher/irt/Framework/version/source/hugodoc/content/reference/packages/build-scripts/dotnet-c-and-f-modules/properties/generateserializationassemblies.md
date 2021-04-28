---
title: generateserializationassemblies
weight: 157
---

 `generateserializationassemblies` property used to set &#39;Generate serialization assembly&#39; field in Visual Studio.

## Usage ##

Property used to set &#39;Generate serialization assembly&#39; value:

{{< url groupname >}} `.generateserializationassemblies` 

Valid values are

 - **None**
 - **Auto**
 - **On**
 - **Off**

Default value is `None` 

## Example ##


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.generateserializationassemblies" value="On"/>
```
