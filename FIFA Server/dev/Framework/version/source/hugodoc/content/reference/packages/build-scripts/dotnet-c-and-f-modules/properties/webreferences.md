---
title: webreferences
weight: 165
---

This topic describes how to add Web references to DotNet module

## Usage ##

Define an{{< url optionset >}} with name {{< url groupname >}} `.webreferences` in your build script.

Each option in this optionset should contain following

 - **name** - C# namespace for generated Web Reference code
 - **value** - Web Reference URL

## Example ##


```xml

    <CSharpProgram name="MyModule">
      <webreferences>
        <option name="TempConv" value="http://eak-web2.eac.ad.ea.com/TempConvert2008/TCConvert2008.asmx" />
      </webreferences>
      ...
    </CSharpProgram>

```
In traditional syntax, webreferences optionset can be set as follows:


```xml
.          <property name="runtime.MyModule.buildtype" value="CSharpProgram" />
.
.          <optionset name="runtime.MyModule.webreferences" >
.            <option name="TempConv" value="http://eak-web2.eac.ad.ea.com/TempConvert2008/TCConvert2008.asmx" />
.          </optionset>
```
