---
title: < al > Task
weight: 1166
---
## Syntax
```xml
<al output="" target="" culture="" template="" failonerror="" verbose="" if="" unless="">
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</al>
```
## Summary ##
Wraps `al` , the assembly linker for the .NET Framework.

## Remarks ##
All specified sources will be embedded using the  `/embed`  flag.  Other source types are not supported.



### Example ###
Create a library containing all icon files in the current directory.


```xml

<project>
    <al output="MyIcons.dll" target="lib">
        <sources>
            <includes name="*.ico"/>
        </sources>
    </al>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| output | The name of the output file for the assembly manifest.<br>This attribute corresponds to the `/out`  flag. | String | True |
| target | The target type (one of &quot;lib&quot;, &quot;exe&quot;, or &quot;winexe&quot;).<br>This attribute corresponds to the `/t[arget]:`  flag. | String | True |
| culture | The culture string associated with the output assembly.<br>The string must be in RFC 1766 format, such as &quot;en-US&quot;.<br>This attribute corresponds to the `/c[ulture]:`  flag. | String | False |
| template | Specifies an assembly from which to get all options except the culture field.<br>The given filename must have a strong name.<br>This attribute corresponds to the `/template:`  flag. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "sources" >}}| The set of source files to embed. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<al output="" target="" culture="" template="" failonerror="" verbose="" if="" unless="">
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </sources>
</al>
```
