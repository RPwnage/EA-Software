---
title: GenerateDocs
weight: 156
---

 `csc-doc` property enables/disables generation of XML documentation files.

 `doc-file` property defines XML documentation file path (name).

## Usage ##

Define property{{< url groupname >}} `.csc-doc` equal to &#39;true&#39; or &#39;false&#39;.

For F# projects use property  `.fsc-doc` .

Default value is **false** .

To specify non default location of the generated document file use property

{{< url groupname >}} `.doc-file` 

Default value for doc file is set to


```c#
Path.Combine(module.IntermediateDir.Path, compiler.OutputName + ".xml")
```
## Example ##


```xml

    <CSharpProgram name="MyModule">
      <generatedoc value="true"/>
      ...
    </CSharpProgram>

```
In traditional syntax, GenerateDoc can be set through property.


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.csc-doc" value="true"/>
```
