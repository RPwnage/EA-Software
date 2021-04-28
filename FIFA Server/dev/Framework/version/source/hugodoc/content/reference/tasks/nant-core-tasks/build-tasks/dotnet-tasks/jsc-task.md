---
title: < jsc > Task
weight: 1166
---
## Syntax
```xml
<jsc output="" target="" debug="" targetframeworkfamily="" define="" win32icon="" win32manifest="" compiler="" resgen="" referencedirs="">
  <references defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <resources prefix="" />
  <modules defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</jsc>
```
## Summary ##
Compiles Microsoft JScript.NET programs using jsc.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| output | Output file name. | String | True |
| target | Output type ( `library`  for dll&#39;s,  `winexe`  for GUI apps,  `exe`  for console apps, or  `module`  for, well, modules). | String | True |
| debug | Generate debug output ( `true` / `false` ). Default is false. | Boolean | False |
| targetframeworkfamily | Target the build as &quot;Framework&quot; (.Net Framework), &quot;Standard&quot; (.Net Standard), or &quot;Core&quot; (.Net Core).<br>Default is &quot;Framework&quot; | String | False |
| define | Define conditional compilation symbol(s). Corresponds to  `/d[efine]:`  flag. | String | False |
| win32icon | Icon to associate with the application. Corresponds to  `/win32icon:`  flag. | String | False |
| win32manifest | Manifest file to associate with the application. Corresponds to  `/win32manifest:`  flag. | String | False |
| compiler | The full path to the C# compiler.<br>Default is to use the compiler located in the current Microsoft .Net runtime directory. | String | False |
| resgen | The full path to the resgen .Net SDK compiler. This attribute is required<br>only when compiling resources. | String | False |
| referencedirs | Path to system reference assemblies.  (Corresponds to the -lib argument in csc.exe)<br>Although the actual -lib argument list multiple directories using comma separated,<br>you can list them using &#39;,&#39; or &#39;;&#39; or &#39;\n&#39;.  Note that space is a valid path<br>character and therefore won&#39;t be used as separator characters. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "references" >}}| Reference metadata from the specified assembly files. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.DotNetTasks.ResourceFileSet" "resources" >}}| Set resources to embed. | {{< task "NAnt.DotNetTasks.ResourceFileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "modules" >}}| Link the specified modules into this assembly. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "sources" >}}| The set of source files for compilation. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<jsc output="" target="" debug="" targetframeworkfamily="" define="" win32icon="" win32manifest="" compiler="" resgen="" referencedirs="" failonerror="" verbose="" if="" unless="">
  <references defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </references>
  <resources prefix="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </resources>
  <modules defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </modules>
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </sources>
</jsc>
```
