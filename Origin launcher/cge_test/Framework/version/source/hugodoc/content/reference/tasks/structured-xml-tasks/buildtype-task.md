---
title: < BuildType > Task
weight: 1010
---
## Syntax
```xml
<BuildType from="" name="" failonerror="" verbose="" if="" unless="">
  <remove />
  <option />
</BuildType>
```
## Summary ##
The build type task can be used to create custom build types, derived from existing build types but with custom settings.

### Example ###
This is an example of how to generate and use a build type using the Build Type task:


```xml

<BuildType name="Library-Custom" from="Library">
  <option name="optimization" value="off"/>
</BuildType>
             
<Library name="MyLibrary" buildtype="Library-Custom">
  . . .
</Library>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| from | The name of a buildtype (&#39;Library&#39;, &#39;Program&#39;, etc.) to derive new build type from. | String | True |
| name | Sets the name for the new buildtype | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.RemoveBuildOptions" "remove" >}}| Define options to remove from the final optionset | {{< task "EA.Eaconfig.Structured.RemoveBuildOptions" >}} | False |

## Full Syntax
```xml
<BuildType from="" name="" failonerror="" verbose="" if="" unless="">
  <remove>
    <defines if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </defines>
    <cc.options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </cc.options>
    <as.options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </as.options>
    <link.options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </link.options>
    <lib.options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </lib.options>
    <echo />
    <do />
  </remove>
  <option />
</BuildType>
```
