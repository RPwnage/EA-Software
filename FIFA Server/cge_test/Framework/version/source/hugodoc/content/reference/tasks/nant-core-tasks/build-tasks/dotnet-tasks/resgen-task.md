---
title: < resgen > Task
weight: 1170
---
## Syntax
```xml
<resgen input="" output="" target="" todir="" compiler="" failonerror="" verbose="" if="" unless="">
  <resources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</resgen>
```
## Summary ##
Converts files from one resource format to another (wraps Microsoft&#39;s resgen.exe).


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| input | Input file to process.  It is required if the  `resources`  fileset is empty. | String | False |
| output | Name of the resource file to output.  Default is the input file name replaced by the target type extension. | String | False |
| target | The target type extension (usually resources).  Default is &quot;resources&quot;. | String | False |
| todir | The directory to which outputs will be stored.  Default is the input file directory. | String | False |
| compiler | The full path to the resgen compiler. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "resources" >}}| Takes a list of .resX or .txt files to convert to .resources files. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<resgen input="" output="" target="" todir="" compiler="" failonerror="" verbose="" if="" unless="">
  <resources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </resources>
</resgen>
```
