---
title: Application Icon
weight: 151
---

 `win32icon` property defines application icon.

## Usage ##

Define SXML element `<applicationicon>`  or property {{< url groupname >}} `.win32icon` containing path to the icon file.

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <applicationicon value="${package.dir}\resources\MyIcon.ico"/>
      ...
    </CSharpProgram>

```
In traditional syntax, win32icon can be set through property.


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.win32icon" value="${package.dir}\resources\MyIcon.ico"/>
```
