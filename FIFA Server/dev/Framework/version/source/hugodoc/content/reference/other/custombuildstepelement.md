---
title: CustomBuildStepElement
weight: 1015
---
## Syntax
```xml
<CustomBuildStepElement before="" after="">
  <custom-build-tool />
  <custom-build-outputs />
  <custom-build-dependencies append="" name="" optionset="" />
  <echo />
  <do />
</CustomBuildStepElement>
```
## Summary ##
Defines a custom build step that may execute before or
after another target.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| before | Name of the target a custom build step should run before (build, link, run).<br>Supported by native NAnt and MSBuild. | String | False |
| after | Name of the target a custom build step should run after (build, run).<br>Supported by native NAnt and MSBuild. | String | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "custom-build-tool" >}}| The commands to be executed by the custom build step,<br>either NAnt tasks or OS commands. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | True |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "custom-build-outputs" >}}| A list of files that are added to the step&#39;s output dependencies. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "custom-build-dependencies" >}}| A list of files that are added to the step&#39;s input dependencies. | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |

## Full Syntax
```xml
<CustomBuildStepElement before="" after="">
  <custom-build-tool if="" unless="" value="" append="">
    <do />
    <!--Text-->
    <!--NAnt Script-->
  </custom-build-tool>
  <custom-build-outputs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </custom-build-outputs>
  <custom-build-dependencies append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </custom-build-dependencies>
  <echo />
  <do />
</CustomBuildStepElement>
```
